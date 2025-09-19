#include "wtk_svprint.h"
#include "wtk/asr/xvprint/wtk_xvprint_file.h"
#include "wtk/asr/xvprint/wtk_xvprint_file2.h"
#include "wtk/asr/fextra/kparm/wtk_kfeat.h"
wtk_vecf_t* wtk_svprint_compute_feat2(wtk_svprint_t *v,int *spk_cnt);
void wtk_svprint_kind_notify(wtk_svprint_t *v,wtk_kfeat_t *feat);
void wtk_svprint_compute_feat_vad(wtk_svprint_t *v);
int wtk_svprint_feed2(wtk_svprint_t *v,short *data,int len,int is_end);
int wtk_svprint_feed_vad(wtk_svprint_t *v,short *data,int len,int is_end);
int wtk_svprint_feed_vad2(wtk_svprint_t *v,short *data,int len,int is_end);
void wtk_svprint_compute_feat_vad_dnn(wtk_svprint_t *v);
char wtk_svprint_kind_vad_callback(wtk_svprint_t *v,wtk_kfeat_t *feat);
void wtk_svprint_comute_feat_vad_flush_end(wtk_svprint_t *v);
void wtk_svprint_comute_feat_vad_flush_end2(wtk_svprint_t *v);
int wtk_svprint_load_enroll_feat(wtk_svprint_t *v);
void wtk_svprint_release_enroll_feat(wtk_svprint_t *v);
int wtk_svprint_L2_normalization(wtk_vecf_t *mean, int cnt);
// FILE *file;

#define USE_VAD 

int wtk_svprint_check(wtk_svprint_t *v,wtk_string_t *s)
{
	int ret = 0;
	int i;

	for(i = 0;i < v->spk_cnt;i++)
	{
		if(wtk_str_equal(v->names[i]->data,v->names[i]->len,s->data,s->len))
		{
			return i+1;
		}
	}

	return ret;
}

int wtk_svprint_load_enroll_feat(wtk_svprint_t *v)
{
    wtk_svprint_cfg_t *cfg=v->cfg;
    wtk_strbuf_t *buf;
    int ret;

    wtk_queue_init(&v->enroll_q);
    buf=wtk_strbuf_new(128,0);
    if(cfg->load_enroll_fn && cfg->enroll_fn)
    {
		wtk_strbuf_push(buf,cfg->enroll_fn,strlen(cfg->enroll_fn));
		wtk_strbuf_push(buf,".idx",sizeof(".idx"));
		// if( cfg->enroll_fn && wtk_file_exist(cfg->enroll_fn)==0 )
		if( cfg->enroll_fn && wtk_file_exist(buf->data)==0 )
		{
			// ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)wtk_svprint_cfg_read_enroll_data,cfg->enroll_fn);
			// if(ret!=0){goto end;}
			wtk_xvprint_file2_load(cfg->enroll_fn,&v->enroll_q);

		    wtk_svprint_cfg_feat_node_t *node;
		    wtk_queue_node_t *qn;

		    v->eval_available = 1;
		    for(qn=v->enroll_q.pop;qn;qn=qn->next)
		    {
		        node=data_offset2(qn,wtk_svprint_cfg_feat_node_t,qn);
		        ret = wtk_svprint_check(v,node->name);
		        if(ret == 0)
		        {
		        	v->names[v->spk_cnt] = wtk_string_dup_data(node->name->data,node->name->len);
		        	v->spk_cnt++;
		        	//node->num = v->spk_cnt;
		        	//printf("%.*s %d\n",v->names[v->spk_cnt-1]->len, v->names[v->spk_cnt-1]->data,node->num);
		        }//else
//		        {
//		        	//node->num = ret;
//		        }
		    }
		    if(v->spk_cnt >=v->max_spk)
		    {
		    	v->enroll_available = 0;
		    }
		    if(v->enroll_q.length <=0 )
		    {
		    	v->eval_available = 0;
		    }
		}else
		{
			v->eval_available = 0;
		}
    }

    wtk_strbuf_delete(buf);
    return 0;
}

void wtk_svprint_release_enroll_feat(wtk_svprint_t *v)
{
    wtk_svprint_cfg_feat_node_t *node;
    wtk_queue_node_t *qn,*qn2;
    int i;

    for(qn=v->enroll_q.pop;qn;qn=qn2)
    {
    	if(!qn)
    	{
    		break;
    	}
        qn2=qn->next;
        node=data_offset2(qn,wtk_svprint_cfg_feat_node_t,qn);
        wtk_svprint_cfg_node_delete(node);
    }
    wtk_queue_init(&v->enroll_q);
    for(i=0;i<v->spk_cnt;i++)
    {
    	wtk_string_delete(v->names[i]);
    }
    v->spk_cnt = 0;
}

void wtk_svprint_set_nnet3_xvec_notify(wtk_svprint_t *v)
{
	wtk_nnet3_xvector_compute_set_nn_notify(v->x);
}

wtk_svprint_t* wtk_svprint_new(wtk_svprint_cfg_t *cfg)
{
    wtk_svprint_t *v;

    v=(wtk_svprint_t*)wtk_malloc(sizeof(*v));
    v->cfg=cfg;
    v->x=wtk_nnet3_xvector_compute_new2(&cfg->xvector);
    v->scoring=wtk_ivector_plda_scoring_new(&cfg->plda_scoring);
    v->x->spk_mean=wtk_vecf_new(256);

    //wtk_nnet3_xvector_compute_set_kind_notify(v->x,v,(wtk_nnet3_xvector_compute_kxparm_notify_f)wtk_svprint_kind_notify);
    wtk_queue_init(&v->feat_q);
    wtk_queue_init(&v->vad_q);
    wtk_queue_init(&v->enroll_q);
    v->v=NULL;
    v->state_l=NULL;
    v->vrobin=NULL;
    v->max_spk=cfg->max_spks;
    if(v->cfg->use_vad_cut)
    {
        v->v=wtk_vad_new(&cfg->vad,&v->vad_q);
        wtk_kxparm_set_vad_callback(v->x->kxparm,v,(wtk_kxparm_callback_f)wtk_svprint_kind_vad_callback);
        v->vrobin=wtk_robin_new((cfg->vad_frames_context<<1)+1);
        v->state_l=wtk_larray_new(1024,sizeof(char));
    }
    v->names = (wtk_string_t **)wtk_malloc(sizeof(wtk_string_t *) * 60);
    v->nbest_res = (wtk_string_t **)wtk_malloc(sizeof(wtk_string_t *) * 4);
    memset(v->score,0,4*sizeof(float));
    v->spk_cnt = 0;
    v->enroll_available = 1;
    v->eval_available = 1;
    wtk_svprint_load_enroll_feat(v);
    v->notify_f=NULL;
    v->ths=NULL;
    v->res_cnt = 0;
    // printf("xvprint %p\n",v);
    // file=fopen("r.bin","w");

    return v;
}

void  wtk_svprint_delete(wtk_svprint_t *v)
{
    // fclose(file);
	int i;
    if(v->state_l)
    {
        wtk_larray_delete(v->state_l);
    }

    if(v->vrobin)
    {
        wtk_robin_delete(v->vrobin);
    }
    if(v->v)
    {
        wtk_vad_delete(v->v);
    }
    for(i=0;i<v->spk_cnt;i++)
    {
    	wtk_string_delete(v->names[i]);
    }
    v->spk_cnt = 0;
    wtk_free(v->names);
    wtk_free(v->nbest_res);
    wtk_nnet3_xvector_compute_delete(v->x);
    wtk_ivector_plda_scoring_delete(v->scoring);
    wtk_svprint_release_enroll_feat(v);
    wtk_free(v);
}

void wtk_svprint_clean(wtk_svprint_t *v)
{
	wtk_svprint_release_enroll_feat(v);
	wtk_xvprint_file2_delete_all(v->cfg->enroll_fn);
}

void wtk_svprint_reload(wtk_svprint_t *v)
{
	wtk_svprint_release_enroll_feat(v);
	wtk_svprint_load_enroll_feat(v);
}

int wtk_svprint_start(wtk_svprint_t *v)
{
    int ret;

    // srand(0);
    ret=wtk_nnet3_xvector_compute_start(v->x);

    if(v->v)
    {
        v->vad_pop_index=0;
        v->feat_push_index=0;
        wtk_larray_reset(v->state_l);
        v->log_energy=0.0f;
        v->vad_state=0;
        wtk_vad_start(v->v);
        // wtk_nnet3_xvector_compute_set_parm_notify_state(v->x,1);
    }
    return ret;
}

void wtk_svprint_reset(wtk_svprint_t *v)
{
    if(v->v)
    {
        wtk_robin_reset(v->vrobin);
        wtk_vad_reset(v->v);
    }
    wtk_nnet3_xvector_compute_reset(v->x);
}

int wtk_svprint_start2(wtk_svprint_t *v)
{
    int ret;

    // srand(0);
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

void wtk_svprint_reset2(wtk_svprint_t *v)
{
    if(v->v)
    {
        wtk_robin_reset(v->vrobin);
    }
    wtk_nnet3_xvector_compute_reset2(v->x);
}

int wtk_svprint_feed(wtk_svprint_t *v,short *data,int len,int is_end)
{
    int ret;
    if(v->v)
    {
        ret=wtk_svprint_feed_vad2(v,data,len,is_end);
    }else
    {
       ret=wtk_svprint_feed2(v,data,len,is_end);
    }
    return ret;
}

int wtk_svprint_feed2(wtk_svprint_t *v,short *data,int len,int is_end)
{
    int ret;

    ret=wtk_nnet3_xvector_compute_feed(v->x,data,len,is_end);

    return ret;
}


int wtk_svprint_feed_vad2(wtk_svprint_t *v,short *data,int len,int is_end)
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
        //printf("vfstate %d\n",vf->state);
        switch(v->vad_state)
        {
        case 0:
            if(vf->state!=wtk_vframe_sil)   
			{
                //speech frame
                // fwrite(vf->wav_data,vf->frame_step<<1,1,file);
                wtk_svprint_start2(v);
                wtk_nnet3_xvector_compute_feed(v->x,vf->wav_data,vf->frame_step,0);
                v->vad_state=1;
            }else
            {
                
            }
            break;
        case 1:
            if(vf->state==wtk_vframe_sil)
            {
                wtk_svprint_comute_feat_vad_flush_end(v); 
                wtk_svprint_reset2(v);
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
            wtk_svprint_comute_feat_vad_flush_end(v);   
        }   
    }
    return ret;
}

int wtk_svprint_feed_vad(wtk_svprint_t *v,short *data,int len,int is_end)
{
    int ret=0;
    wtk_queue_t *q=v->v->output_queue;
    wtk_vframe_t *vf=NULL;
    wtk_queue_node_t *qn;
    
#ifdef USE_VAD
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
                wtk_nnet3_xvector_compute_feed(v->x,vf->wav_data,vf->frame_step,0);
                v->vad_state=1;
            }else
            {
                
            }
            break;
        case 1:
            if(vf->state==wtk_vframe_sil)
            {
                v->vad_state=0;
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
#else
    wtk_nnet3_xvector_compute_feed(v->x,data,len,is_end);
#endif
    
    if(is_end)
    {
#ifdef USE_VAD
        wtk_svprint_comute_feat_vad_flush_end(v);
#else        
        wtk_svprint_comute_feat_vad_flush_end2(v);
#endif        
    }
    return ret;
}

char wtk_svprint_kind_vad_callback(wtk_svprint_t *v,wtk_kfeat_t *feat)
{
    char state;
    char *ptr;

    ptr=wtk_larray_get(v->state_l,v->vad_pop_index);

    state=*ptr;
    v->vad_pop_index++;

    return state;
}

void wtk_svprint_kind_notify(wtk_svprint_t *v,wtk_kfeat_t *feat)
{
    wtk_robin_t *r=v->vrobin;
    float energy_thresh;
    int win=v->cfg->vad_frames_context;
    int i;
    int den_cnt;
    int num_cnt;
    wtk_kfeat_t *f;
    char state;

#ifdef USE_VAD
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
#else
    v->log_energy+=feat->v[0];
    wtk_queue_push(&v->feat_q,&feat->q_n);
#endif
}

void wtk_svprint_comute_feat_vad_flush_end(wtk_svprint_t *v)
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
    //wtk_debug("=========\n");
    wtk_kxparm_feed_feat2(v->x->kxparm,0,1);
}


void wtk_svprint_comute_feat_vad_flush_end2(wtk_svprint_t *v)
{
    wtk_robin_t *r=v->vrobin;
    wtk_queue_node_t *qn;
    float energy_thresh;
    wtk_kfeat_t *feat,*f;
    // wtk_kfeat_t *pv[v->cfg->vad_frames_context*2+1];
    int n,i,j;
    int win=v->cfg->vad_frames_context;
    int den_cnt;
    int num_cnt;
    wtk_queue_t *q=&v->feat_q;
    char state;
    char pop;

    if(q->length==0)
    {
        return;
    }

    energy_thresh=v->cfg->vad_energy_thresh+(v->log_energy*v->cfg->vad_energy_mean_scale/q->length);
    wtk_robin_reset(r);
    // i=1;
    for(qn=v->feat_q.pop;qn;qn=qn->next)
    {
        feat=data_offset2(qn,wtk_kfeat_t,q_n);

        // v->log_energy+=feat->v[0];
        // energy_thresh=v->cfg->vad_energy_thresh+(v->log_energy*v->cfg->vad_energy_mean_scale/i);
        // ++i;

        wtk_robin_push2(r,feat);
        if (r->used <= win) 
        {
            continue;
        }

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
    }

    //set left data sil
    n=v->feat_q.length-v->state_l->nslot;
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

    for(i=0;i<v->state_l->nslot;++i)
    {
        qn=wtk_queue_pop(&v->feat_q);
        feat=data_offset2(qn,wtk_kfeat_t,q_n);
        wtk_kxparm_feed_feat2(v->x->kxparm,feat,0);
    }
    wtk_kxparm_feed_feat2(v->x->kxparm,0,1);
}
//distance
float wtk_svprint_feat_compute_likelihood(wtk_vecf_t* feat1, wtk_vecf_t*feat2)
{
	float res = 0.0,tmp;
	int i;
	float *p1 = feat1->p;
	float *p2 = feat2->p;

	for(i = 0;i < feat1->len; i++,p1++,p2++)
	{
		tmp = (*p1) - (*p2);
		res += tmp*tmp;
	}
	res = pow(res,0.5);

	return res;
}
//cos
float wtk_svprint_feat_compute_likelihood2(wtk_vecf_t* feat1, wtk_vecf_t*feat2)
{
	float res = 0.0;
	int i;
	float *p1 = feat1->p;
	float *p2 = feat2->p;
	float suma=0.0,sumb=0.0;

	for(i = 0;i < feat1->len; i++,p1++,p2++)
	{
		res += (*p1)*(*p2);
		suma += (*p1)*(*p1);
		sumb += (*p2)*(*p2);
	}
	res /= pow(suma,0.5)*pow(sumb,0.5);

	return res;
}

wtk_string_t* wtk_svprint_feat_likelihood(wtk_svprint_t *v,wtk_vecf_t *feat,float *prob)
{
    wtk_svprint_cfg_feat_node_t *node,*max_node;
    wtk_queue_node_t *qn;
    float likelihood;//[10];
    float max_score,min_score;

    max_score=99999.99f;
    min_score = -10.f;
    max_node=0;
    //print_float(feat->p,feat->len);
    for(qn=v->enroll_q.pop;qn;qn=qn->next)
    {
        node=data_offset2(qn,wtk_svprint_cfg_feat_node_t,qn);
        wtk_svprint_L2_normalization(node->v, node->num);
        if(v->cfg->use_distance)
        {
        	likelihood=wtk_svprint_feat_compute_likelihood(node->v,feat);
            if(max_score>likelihood)
            {
                max_score=likelihood;
                max_node=node;
            }
        }else
        {
        	likelihood=wtk_svprint_feat_compute_likelihood2(node->v,feat);
            if(min_score<likelihood)
            {
            	min_score=likelihood;
                max_node=node;
            }
        }

        //printf("%.*s %f %f\n",node->name->len,node->name->data,likelihood,v->cfg->score_thresh);
    }

    if(v->cfg->use_distance)
    {
        *prob = max_score;
    	if(max_score < v->cfg->score_thresh)
    	{
        	return (max_node->name);
    	}
    }else
    {
        *prob = min_score;
    	if(min_score > v->cfg->score_thresh)
    	{
        	return (max_node->name);
    	}
    }
    return NULL;
}

void wtk_svprint_feat_likelihood_Nbest(wtk_svprint_t *v,wtk_vecf_t *feat,int N)
{
    wtk_svprint_cfg_feat_node_t *node;
    wtk_queue_node_t *qn;
    float likelihood;//[10];
    int i,j,k,found;
    wtk_string_t *tmp_name,*tmp_name2;
    float tmp,tmp2;

    //print_float(feat->p,feat->len);
    for(i = 0; i < 4; i++){
        v->nbest_res[i] = 0;
        v->score[i] = -10.f;
    }

    for(qn=v->enroll_q.pop,i=0;qn;qn=qn->next,i++)
    {
        node=data_offset2(qn,wtk_svprint_cfg_feat_node_t,qn);
        wtk_svprint_L2_normalization(node->v, node->num);
        likelihood=wtk_svprint_feat_compute_likelihood2(node->v,feat);
        //wtk_debug("%.*s %f %f\n",node->name->len,node->name->data,likelihood,v->cfg->score_thresh);
        if(i >= 4){
            j = 4;
            v->res_cnt = 4;
        }else{
            j = i + 1;
            v->res_cnt = j;
        }

        found = 0;
        for(k = 0; k < j; k++){
            if(found == 0){
                if(likelihood > v->score[k]){
                    found = 1;
                    if(v->score[k] < -5){
                        v->score[k] = likelihood;
                        v->nbest_res[k] = node->name;
                        break;
                    }else{
                        tmp = v->score[k];
                        tmp_name = v->nbest_res[k];
                        v->score[k] = likelihood;
                        v->nbest_res[k] = node->name;
                    }
                }
            }else{
                tmp2 = v->score[k];
                tmp_name2 = v->nbest_res[k];
                v->score[k] = tmp;
                v->nbest_res[k] = tmp_name;
                tmp = tmp2;
                tmp_name = tmp_name2;
            }
        }
        //printf("%.*s %f %f\n",node->name->len,node->name->data,likelihood,v->cfg->score_thresh);
    }
    // printf("%.*s %f\n",v->nbest_res[0]->len,v->nbest_res[0]->data,v->score[0]);
    // printf("%.*s %f\n",v->nbest_res[1]->len,v->nbest_res[1]->data,v->score[1]);
    // printf("%.*s %f\n",v->nbest_res[2]->len,v->nbest_res[2]->data,v->score[2]);
    // printf("%.*s %f\n",v->nbest_res[3]->len,v->nbest_res[3]->data,v->score[3]);
}

wtk_string_t* wtk_svprint_feat_likelihood_plda(wtk_svprint_t *v,wtk_vecf_t *feat,float *prob)
{
    wtk_svprint_cfg_feat_node_t *node,*max_node;
    wtk_queue_node_t *qn;
    float likelihood;//[10];
    //int id,i;
    float max_score;

    max_score=-99999.99f;
    max_node=0;
    //printf("enroll file length: %d\n",v->cfg->enroll_q.length);
    //print_float(feat->p,feat->len);
    for(qn=v->enroll_q.pop;qn;qn=qn->next)
    {
        node=data_offset2(qn,wtk_svprint_cfg_feat_node_t,qn);
        //id = node->num - 1;
        //wtk_debug("%d\n",node->num);
        likelihood=wtk_ivector_plda_scoring_loglikelihood(v->scoring,node->v,node->num,feat);
        //likelihood[id] += wtk_svprint_feat_compute_likelihood(feat,node->v);
        if(max_score<likelihood)
        {
            max_score=likelihood;
            max_node=node;
        }
        //printf("%.*s %f\n",node->name->len,node->name->data,likelihood);
    }

    *prob = max_score;
    //printf("%.*s %f\n",max_node->name->len,max_node->name->data,max_score);
    if(max_score > v->cfg->score_thresh)
    {
        //printf("%.*s %f\n",v->names[i]->len,v->names[i]->data,min_score);
        return (max_node->name);
    }

    return NULL;
}

wtk_string_t* wtk_svprint_eval(wtk_svprint_t *v, float *prob)
{
	wtk_vecf_t *vec;
	//int spk_cnt=1;

	vec=wtk_nnet3_xvector_compute_normalize(v->x);
	if(v->cfg->use_plda)
	{
	    wtk_ivector_plda_scoring_transfrom_ivector(v->scoring,vec);//t2
	    //vec=wtk_ivector_plda_scoring_ivector_normalization(v->scoring,spk_cnt);
	    //print_float(v->scoring->ivector->p,v->scoring->ivector->len);
	    wtk_nnet3_xvector_compute_normalize_reset(v->x);
	    return wtk_svprint_feat_likelihood_plda(v,v->scoring->ivector,prob);
	}
    v->x->spk_cnt = 0;
	return wtk_svprint_feat_likelihood(v,vec,prob);
}

wtk_string_t* wtk_svprint_eval2(wtk_svprint_t *v, wtk_vecf_t *vec, wtk_vecf_t *dst, int cnt, float *prob)
{
    wtk_svprint_L2_normalization2(vec,dst,cnt);
	if(v->cfg->use_plda)
	{
	    wtk_ivector_plda_scoring_transfrom_ivector(v->scoring,vec);//t2
	    //vec=wtk_ivector_plda_scoring_ivector_normalization(v->scoring,spk_cnt);
	    //print_float(v->scoring->ivector->p,v->scoring->ivector->len);
	    wtk_nnet3_xvector_compute_normalize_reset(v->x);
	    return wtk_svprint_feat_likelihood_plda(v,v->scoring->ivector,prob);
	}
    //v->x->spk_cnt = 0;
	return wtk_svprint_feat_likelihood(v,dst,prob);
}

void wtk_svprint_eval_Nbest(wtk_svprint_t *v, wtk_vecf_t *vec, wtk_vecf_t *dst, int cnt, float *prob)
{
    wtk_svprint_L2_normalization2(vec,dst,cnt);
	wtk_svprint_feat_likelihood_Nbest(v,dst,cnt);
}

void wtk_svprint_set_score_notify(wtk_svprint_t *v,void *ths,wtk_svprint_score_notify notify)
{
    v->ths=ths;
    v->notify_f=notify;
}

int wtk_svprint_feat_likelihood_node(wtk_svprint_t *v,wtk_vecf_t *feat,wtk_svprint_cfg_feat_node_t *node)
{
    float likelihood;
    int ret=0;

    likelihood=wtk_ivector_plda_scoring_loglikelihood(v->scoring,node->v,node->num,feat);
    if(likelihood>v->cfg->score_thresh)
    {
        ret=1;
    }

    return ret;
}

wtk_vecf_t* wtk_svprint_compute_feat(wtk_svprint_t *v)
{
    int cnt;

    return wtk_svprint_compute_feat2(v,&cnt);
}

wtk_vecf_t* wtk_svprint_compute_feat2(wtk_svprint_t *v,int *spk_cnt)
{
    wtk_vecf_t *vec;
    // int i;

    vec = wtk_nnet3_xvector_compute(v->x);
    if(v->cfg->use_plda)
    {
        *spk_cnt=wtk_net3_xvector_compute_get_spk_cnt(v->x);
        vec = wtk_ivector_plda_scoring_transfrom_ivector(v->scoring,vec);
        //vec=wtk_ivector_plda_scoring_ivector_normalization(v->scoring,*spk_cnt);
        wtk_nnet3_xvector_compute_normalize_reset(v->x);
    }

    return vec;
}

int wtk_svprint_enrollvec(wtk_svprint_t *v,wtk_string_t *name,wtk_vecf_t *vec)
{
    wtk_svprint_cfg_feat_node_t *node;
    int ret;

    node = wtk_svprint_cfg_node_new2(name,vec,1);
    ret = wtk_xvprint_file2_append_feat(v->cfg->enroll_fn,(wtk_xvprint_cfg_feat_node_t*)node);
    return ret;
}

// L2范数归一化
int wtk_svprint_L2_normalization(wtk_vecf_t *mean, int cnt) {
    int ret = 0, i = 0;
    float L2 = 0.0;
    float eps = 1e-12;

    if (cnt <= 0) {
        ret = -1;
        goto end;
    }

    for (i = 0; i < mean->len; i++) {
        mean->p[i] /= cnt;
    }
    for (i = 0; i < mean->len; i++) {
        L2 += mean->p[i] * mean->p[i];
    }
    if (L2 <= 0.0) {
        L2 = eps;
    }else{
        L2 = pow(L2, 0.5);
    }

    for (i = 0; i < mean->len; i++) {
        mean->p[i] /= L2;
    }

end:
    return ret;
}

int wtk_svprint_L2_normalization2(wtk_vecf_t *mean,wtk_vecf_t *dst, int cnt) {
    int ret = 0, i = 0;
    float L2 = 0.0;
    float eps = 1e-12;

    if (cnt <= 0) {
        ret = -1;
        goto end;
    }

    for (i = 0; i < mean->len; i++) {
        dst->p[i] = mean->p[i]/cnt;
    }
    for (i = 0; i < mean->len; i++) {
        L2 += dst->p[i] * dst->p[i];
    }
    if (L2 <= 0.0) {
        L2 = eps;
    }else{
        L2 = pow(L2, 0.5);
    }

    for (i = 0; i < mean->len; i++) {
        dst->p[i] /= L2;
    }

end:
    return ret;
}

int wtk_svprint_enroll2file(wtk_svprint_t *v, wtk_string_t *name) {
    wtk_svprint_cfg_feat_node_t *node;
    wtk_vecf_t *mean;
    int i = 0, cnt = 0, ret = 0;

    cnt = wtk_net3_xvector_compute_get_spk_cnt(v->x);
    mean = v->x->spk_mean;
    // wtk_debug("%d\n",cnt);
    // ret = wtk_svprint_L2_normalization(mean, cnt);
    // print_float(v->x->spk_mean->p,v->x->spk_mean->len);
    // vec=wtk_svprint_compute_feat2(v,&cnt);
    // wtk_debug("enroll2file:\n");
    // print_float(vec->p,vec->len);
    for (i = 0; i < mean->len; i++) {
        mean->p[i] = mean->p[i] / cnt;
    }
    node=wtk_svprint_cfg_node_new2(name,mean,cnt);

    ret=wtk_xvprint_file2_append_feat(v->cfg->enroll_fn,(wtk_xvprint_cfg_feat_node_t*)node);

    wtk_svprint_cfg_node_delete(node);
    wtk_vecf_zero(v->x->spk_mean);
    v->x->spk_cnt = 0;
    // printf("enroll cnt %d\n",cnt);
    return ret;
}

int wtk_svprint_enroll2mem(wtk_svprint_t *v,wtk_string_t *name)
{
    wtk_svprint_cfg_feat_node_t *node;
    wtk_vecf_t *mean;
    int cnt,i;

    // exit(0);
    //vec=wtk_svprint_compute_feat2(v,&cnt);
    cnt=wtk_net3_xvector_compute_get_spk_cnt(v->x);
    printf("%.*s %d\n",name->len,name->data,cnt);
	mean = v->x->spk_mean;
	for(i=0;i<mean->len;i++)
	{
		mean->p[i]/=cnt;
	}
    // printf("cnttt %d\n",cnt);
    node=wtk_svprint_cfg_node_new2(name,mean,cnt);
    wtk_queue_push(&v->enroll_q,&node->qn);

    wtk_vecf_zero(v->x->spk_mean);
    return 0;
}

int wtk_svprint_dump_memory2file(wtk_svprint_t *v)
{
    return wtk_xvprint_file2_dump(v->cfg->enroll_fn,&v->enroll_q);
}

wtk_svprint_cfg_feat_node_t* wtk_svprint_query_memory_node(wtk_svprint_t *v,wtk_string_t *name)
{
    wtk_svprint_cfg_feat_node_t *node,*node2;
    wtk_queue_node_t *qn;

    node2=NULL;
    for(qn=v->enroll_q.pop;qn;qn=qn->next)
    {
        node=data_offset2(qn,wtk_svprint_cfg_feat_node_t,qn);
        if(wtk_string_cmp(name,node->name->data,node->name->len)==0)
        {
            node2=node;
            break;
        }
    }

    return node2;
}

#if 0

void wtk_svprint_compute_feat_vad_dnn(wtk_svprint_t *v)
{
    wtk_queue_node_t *qn;
    wtk_kfeat_t *feat,*f;
    // wtk_kfeat_t *pv[v->cfg->vad_frames_context*2+1];
    int n,i,j;
    char state;

    wtk_nnet3_xvector_compute_set_parm_notify_state(v->x,0);

    for(i=0;i<v->state_l->nslot;++i)
    {
        qn=wtk_queue_pop(&v->feat_q2);
        feat=data_offset2(qn,wtk_kfeat_t,q_n);
        state=*((char*)wtk_larray_get(v->state_l,i));
        // printf("%d\n",state);
        wtk_kxparm_feed_feat3(v->x->kxparm,feat,state,0);
    }
    wtk_kxparm_feed_feat3(v->x->kxparm,0,0,1);
}



void wtk_svprint_compute_feat_vad4(wtk_svprint_t *v)
{
    wtk_robin_t *r=v->vrobin;
    wtk_queue_node_t *qn;
    float energy_thresh;
    wtk_kfeat_t *feat,*f;
    // wtk_kfeat_t *pv[v->cfg->vad_frames_context*2+1];
    wtk_kfeat_t *pv[5];
    int pad,i,j;
    int win=v->cfg->vad_frames_context;
    int den_cnt;
    int num_cnt;
    wtk_queue_t *q=&v->feat_q;
    char state;

    if(q->length==0)
    {
        return;
    }


    wtk_nnet3_xvector_compute_set_parm_notify_state(v->x,0);

    energy_thresh=v->cfg->vad_energy_thresh+(v->log_energy*v->cfg->vad_energy_mean_scale/q->length);
    
    // printf("thresh %f %d\n",energy_thresh,q->length);
    wtk_robin_reset(r);
    for(qn=v->feat_q.pop;qn;qn=qn->next)
    {
        feat=data_offset2(qn,wtk_kfeat_t,q_n);
        wtk_robin_push(r,feat);
        if (r->used <= win) 
        {
            continue;
        }

        pad = r->nslot - r->used; i = 0;
        //wtk_debug("r=%p,nslot=%d,used=%d\n",r,r->nslot,r->used);
        if (pad > 0) 
        {
            // add pad to front.
            // * |f0|f1|f2|0|0|  => |f0|f0|f0|f1|f2|
            // * |f0|f1|f2|f3|0| => |f0|f0|f1|f2|f3|
            f = (wtk_kfeat_t *)wtk_robin_at(r, 0);
            for (; i < pad; ++i) 
            {
                pv[i] = f;
            }
        }
        for (j = 0; j < r->used; ++i, ++j) 
        {
            f = ((wtk_kfeat_t *)wtk_robin_at(r, j));
            pv[i] = f;
        }


        // den_cnt=0;
        den_cnt=win*2+1;
        num_cnt=0;
        for(i=0;i<(win*2+1);++i)
        {
            // den_cnt++;
            f=pv[i];
            if(f->v[0]>energy_thresh)
            {
                num_cnt++;
            }
        }
        // if(pad>0)
        // {
        //     den_cnt=num_cnt;
        // }
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
    }

    //set left data sil
    j=v->feat_q.length-v->state_l->nslot;
    state=0;
    for(i=0;i<j;++i)
    {
        wtk_larray_push2(v->state_l,&state);
    }

    for(i=0;i<v->state_l->nslot;++i)
    {
        qn=wtk_queue_pop(&v->feat_q);
        feat=data_offset2(qn,wtk_kfeat_t,q_n);
        state=*((char*)wtk_larray_get(v->state_l,i));
        wtk_kxparm_feed_feat(v->x->kxparm,feat,state,0);
    }
    wtk_kxparm_feed_feat(v->x->kxparm,0,0,1);
}


void wtk_svprint_compute_feat_vad2(wtk_svprint_t *v)
{
    wtk_robin_t *r=v->vrobin;
    wtk_queue_node_t *qn;
    float energy_thresh;
    wtk_kfeat_t *feat,*f;
    wtk_kfeat_t *pv[5];
    int pad,i,j;
    wtk_queue_t *q=&v->feat_q;
    int den_cnt=v->cfg->vad_frames_context*2+1;
    int num_cnt;
    int win=v->cfg->vad_frames_context;
    float energy;
    char state;

    if(q->length==0)
    {
        return;
    }

    energy_thresh=140.0f;
    wtk_nnet3_xvector_compute_set_parm_notify_state(v->x,0);

    wtk_robin_reset(r);
    for(qn=v->feat_q.pop;qn;qn=qn->next)
    {
        feat=data_offset2(qn,wtk_kfeat_t,q_n);

        energy=wtk_float_energy(feat->v,v->x->kxparm->parm->cfg->vec_size2);
        // printf("aa %f\n",energy);
        if(energy>energy_thresh)
        {
            state=1;
            wtk_larray_push2(v->state_l,&state);
        }else
        {
            state=0;
            wtk_larray_push2(v->state_l,&state);
        }
    }

    //set left data sil
    j=v->feat_q.length-v->state_l->nslot;
    state=0;
    for(i=0;i<j;++i)
    {
        wtk_larray_push2(v->state_l,&state);
    }

    for(i=0;i<v->state_l->nslot;++i)
    {
        qn=wtk_queue_pop(&v->feat_q);
        feat=data_offset2(qn,wtk_kfeat_t,q_n);
        state=*((char*)wtk_larray_get(v->state_l,i));
        wtk_kxparm_feed_feat(v->x->kxparm,feat,state,0);
    }
    wtk_kxparm_feed_feat(v->x->kxparm,0,0,1);
}


void wtk_svprint_compute_feat_vad3(wtk_svprint_t *v)
{
    wtk_robin_t *r=v->vrobin;
    wtk_queue_node_t *qn;
    float energy_thresh;
    wtk_kfeat_t *feat,*f;
    wtk_kfeat_t *pv[5];
    int pad,i,j;
    wtk_queue_t *q=&v->feat_q;
    int den_cnt=v->cfg->vad_frames_context*2+1;
    int num_cnt;
    int win=v->cfg->vad_frames_context;
    float energy;
    char state;

    if(q->length==0)
    {
        return;
    }

    energy_thresh=80.0f;
    // wtk_nnet3_xvector_compute_set_parm_notify_state(v->x,0);

    wtk_robin_reset(r);
    for(qn=v->feat_q.pop;qn;qn=qn->next)
    {
        feat=data_offset2(qn,wtk_kfeat_t,q_n);
        wtk_robin_push(r,feat);
        if (r->used <= win) 
        {
            continue;
        }

        pad = r->nslot - r->used; i = 0;
        //wtk_debug("r=%p,nslot=%d,used=%d\n",r,r->nslot,r->used);
        if (pad > 0) 
        {
            // add pad to front.
            // * |f0|f1|f2|0|0|  => |f0|f0|f0|f1|f2|
            // * |f0|f1|f2|f3|0| => |f0|f0|f1|f2|f3|
            f = (wtk_kfeat_t *)wtk_robin_at(r, 0);
            for (; i < pad; ++i) 
            {
                pv[i] = f;
            }
        }
        for (j = 0; j < r->used; ++i, ++j) 
        {
            f = ((wtk_kfeat_t *)wtk_robin_at(r, j));
            pv[i] = f;
        }


        // den_cnt=0;
        num_cnt=0;
        for(i=0;i<(win*2+1);++i)
        {
            // den_cnt++;
            f=pv[i];
            energy=wtk_float_energy(f->v,v->x->kxparm->parm->cfg->vec_size2);
            // printf("aa %f\n",energy);
            if(energy>energy_thresh)
            {
                num_cnt++;
            }
        }
        // if(pad>0)
        // {
        //     den_cnt=num_cnt;
        // }
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
    }

    //set left data sil
    j=v->feat_q.length-v->state_l->nslot;
    state=0;
    for(i=0;i<j;++i)
    {
        wtk_larray_push2(v->state_l,&state);
    }

    for(i=0;i<v->state_l->nslot;++i)
    {
        qn=wtk_queue_pop(&v->feat_q);
        feat=data_offset2(qn,wtk_kfeat_t,q_n);
        state=*((char*)wtk_larray_get(v->state_l,i));
        wtk_kxparm_feed_feat(v->x->kxparm,feat,state,0);
    }
    wtk_kxparm_feed_feat(v->x->kxparm,0,0,1);
}
#endif
