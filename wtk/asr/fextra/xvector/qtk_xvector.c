#include "qtk_xvector.h"

char qtk_xvector_kind_vad_callback(qtk_xvector_t *v,wtk_kfeat_t *feat)
{
    char state;
    char *ptr;

    ptr=wtk_larray_get(v->state_l,v->vad_pop_index);

    state=*ptr;
    v->vad_pop_index++;

    return state;
}

void qtk_xvector_kind_notify(qtk_xvector_t *v,wtk_kfeat_t *feat)
{
    wtk_robin_t *r=v->vrobin;
    float energy_thresh;
    int win=v->cfg->vad_frames_context;
    int i;
    int den_cnt;
    int num_cnt;
    wtk_kfeat_t *f;
    char state;

//#ifdef USE_VAD
    // printf("%d %p %p %d\n",__LINE__,v,feat,feat->index);
    // exit(0);
    v->log_energy+=feat->v[0];
    v->feat_push_index++;

    wtk_robin_push2(r,feat);
    if (r->used <= win)
    {
        return;
    }

    energy_thresh=v->cfg->vad_energy_thresh+(v->log_energy*v->cfg->vad_energy_mean_scale/v->feat_push_index);
    den_cnt=0;
    num_cnt=0;
    for(i=0;i<r->used;++i)
    {
        den_cnt++;
        f=wtk_robin_at(r,i);
        if(f->v[0]>energy_thresh)
        {
            num_cnt++;
        }
    }

    // printf("numcnt %d %f\n",num_cnt,den_cnt*v->cfg->vad_proportion_threshold);
    if(num_cnt>=den_cnt*v->cfg->vad_proportion_threshold)
    {
        //process first two frame
        //do pv[win]
        state=1;
        wtk_larray_push2(v->state_l,&state);
    }else
    {
        state=0;
        wtk_larray_push2(v->state_l,&state);
    }

    f=wtk_robin_at(r,(r->used-(win+1)));
    wtk_kxparm_feed_feat2(v->x->kxparm,f,0);
//#else
//   v->log_energy+=feat->v[0];
//    wtk_queue_push(&v->feat_q,&feat->q_n);
//#endif
}

qtk_xvector_t* qtk_xvector_new(qtk_xvector_cfg_t *cfg)
{
	qtk_xvector_t* x = (qtk_xvector_t*)wtk_malloc(sizeof(qtk_xvector_t));
	x->cfg=cfg;
	x->x=wtk_nnet3_xvector_compute_new(&cfg->xvector);

    wtk_nnet3_xvector_compute_set_kind_notify(x->x,x,(wtk_nnet3_xvector_compute_kxparm_notify_f)qtk_xvector_kind_notify);
    wtk_queue_init(&x->vad_q);
    x->v=NULL;
    {
        x->v=wtk_vad_new(&cfg->vad,&x->vad_q);
        wtk_kxparm_set_vad_callback(x->x->kxparm,x,(wtk_kxparm_callback_f)qtk_xvector_kind_vad_callback);
        x->vrobin=wtk_robin_new((cfg->vad_frames_context<<1)+1);
        x->state_l=wtk_larray_new(1024,sizeof(char));
    }	return x;
}

int qtk_xvector_delete(qtk_xvector_t *x)
{
    if(x->state_l)
    {
        wtk_larray_delete(x->state_l);
    }

    if(x->vrobin)
    {
        wtk_robin_delete(x->vrobin);
    }
    if(x->v)
    {
        wtk_vad_delete(x->v);
    }
    wtk_nnet3_xvector_compute_delete(x->x);
    wtk_free(x);

	return 0;
}

void qtk_xvector_reset(qtk_xvector_t *x)
{
    if(x->v)
    {
        wtk_robin_reset(x->vrobin);
        wtk_vad_reset(x->v);
    }
    wtk_nnet3_xvector_compute_reset(x->x);
}

void qtk_xvector_reset2(qtk_xvector_t *v)
{
    if(v->v)
    {
        wtk_robin_reset(v->vrobin);
    }
    wtk_nnet3_xvector_compute_reset(v->x);
}

void qtk_xvector_normalize_reset(qtk_xvector_t *x)
{
	wtk_nnet3_xvector_compute_normalize_reset(x->x);
}

void qtk_xvector_start(qtk_xvector_t *x)
{
	wtk_nnet3_xvector_compute_start(x->x);
    if(x->v)
    {
        x->vad_pop_index=0;
        x->feat_push_index=0;
        wtk_larray_reset(x->state_l);
        x->log_energy=0.0f;
        x->vad_state=0;
        wtk_vad_start(x->v);
        // wtk_nnet3_xvector_compute_set_parm_notify_state(v->x,1);
    }
}

int qtk_xvector_start2(qtk_xvector_t *v)
{
    int ret;

    ret=wtk_nnet3_xvector_compute_start(v->x);
    if(v->v)
    {
        v->vad_pop_index=0;
        v->feat_push_index=0;
        wtk_larray_reset(v->state_l);
        v->log_energy=0.0f;
        v->vad_state=0;
    }
    return ret;
}

void qtk_xvector_comute_feat_vad_flush_end(qtk_xvector_t *v)
{
    wtk_robin_t *r=v->vrobin;
    float energy_thresh;
    wtk_kfeat_t *f;
    int n,i,j;
    int win=v->cfg->vad_frames_context;
    int den_cnt;
    int num_cnt;
    char state;
    char pop;

    // printf("push %d\n",v->feat_push_index);
    energy_thresh=v->cfg->vad_energy_thresh+(v->log_energy*v->cfg->vad_energy_mean_scale/v->feat_push_index);
    n=v->feat_push_index-v->state_l->nslot;
    // printf("nnnnn %d %d\n",n,v->feat_push_index);
    state=0;
    pop=0;
    if(r->used>win+1)
    {
        pop=1;
    }
    for(i=0;i<n;++i)
    {
        if(pop)
        {
            wtk_robin_pop(r);
        }
        den_cnt=0;num_cnt=0;
        for(j=0;j<r->used;++j)
        {
            den_cnt++;
            f=wtk_robin_at(r,i);
            if(f->v[0]>energy_thresh)
            {
                num_cnt++;
            }
        }
        if(num_cnt>=den_cnt*v->cfg->vad_proportion_threshold)
        {
            //process first two frame
            //do pv[win]
            state=1;
            wtk_larray_push2(v->state_l,&state);
        }else
        {
            state=0;
            wtk_larray_push2(v->state_l,&state);
        }
    }

    for(i=0;i<n;++i)
    {
        f=wtk_robin_at(r,r->used-n+i);
        // printf("index %d\n",f->index);
        wtk_kxparm_feed_feat2(v->x->kxparm,f,0);
    }

    wtk_kxparm_feed_feat2(v->x->kxparm,0,1);
}

int qtk_xvector_feed_vad2(qtk_xvector_t *v,short *data,int len,int is_end)
{
    int ret;
    wtk_queue_t *q=v->v->output_queue;
    wtk_vframe_t *vf=NULL;
    wtk_queue_node_t *qn;

    wtk_vad_feed(v->v,(char*)data,len<<1,is_end);
    while(1)
	{
		qn=wtk_queue_pop(q);
		if(!qn){break;}
		vf=data_offset2(qn,wtk_vframe_t,q_n);
        // printf("vfstate %d\n",vf->state);
        switch(v->vad_state)
        {
        case 0:
            if(vf->state!=wtk_vframe_sil)
			{
                //speech frame
                // fwrite(vf->wav_data,vf->frame_step<<1,1,file);
            	qtk_xvector_start2(v);
                wtk_nnet3_xvector_compute_feed(v->x,vf->wav_data,vf->frame_step,0);
                v->vad_state=1;
            }else
            {

            }
            break;
        case 1:
            if(vf->state==wtk_vframe_sil)
            {
                qtk_xvector_comute_feat_vad_flush_end(v);
                qtk_xvector_reset2(v);
                v->vad_state=0;
                ret=2;
                goto end;
            }else
            {
                // fwrite(vf->wav_data,vf->frame_step<<1,1,file);
                //speech frame
                wtk_nnet3_xvector_compute_feed(v->x,vf->wav_data,vf->frame_step,0);
            }
            break;
        default:
            break;
        }
        wtk_vad_push_vframe(v->v,vf);
    }

    ret=0;
end:
    if(is_end)
    {
        if(v->vad_state==1)
        {
        	qtk_xvector_comute_feat_vad_flush_end(v);
        }
    }
    return ret;
}

void qtk_xvector_feed(qtk_xvector_t *x, short *data, int len, int is_end)
{
	if(x->v)
	{
		qtk_xvector_feed_vad2(x,data,len,is_end);
	}else
	{
		wtk_nnet3_xvector_compute_feed(x->x,data,len,is_end);
	}
}

wtk_vecf_t* qtk_xvector_get_result(qtk_xvector_t *x)
{
	wtk_vecf_t *v;

	v=wtk_nnet3_xvector_compute_normalize(x->x);

    return v;
}

void qtk_xvector_write_xvec(wtk_vecf_t *v,char *fn)
{
	wtk_strbuf_t *buf=wtk_strbuf_new(1024,1);
	int i;

	wtk_strbuf_push_c(buf,' ');
	wtk_strbuf_push_c(buf,'[');
	//wtk_strbuf_push_float(buf,v->p,v->len);
	for(i=0;i<v->len;i++)
	{
		wtk_strbuf_push_c(buf,' ');
		wtk_strbuf_push_f(buf,"%f",*(v->p+i));
	}
	wtk_strbuf_push_c(buf,' ');
	wtk_strbuf_push_c(buf,']');
	FILE *f;
	f=fopen(fn,"wb");
	fwrite(buf->data,buf->pos,1,f);
	fclose(f);
	wtk_strbuf_delete(buf);
}
