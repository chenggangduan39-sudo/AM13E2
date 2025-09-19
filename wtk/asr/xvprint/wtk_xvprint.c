#include "wtk_xvprint.h"
#include "wtk_xvprint_file.h"
#include "wtk_xvprint_file2.h"
#include "wtk/asr/fextra/kparm/wtk_kfeat.h"
wtk_vecf_t* wtk_xvprint_compute_feat2(wtk_xvprint_t *v,int *spk_cnt);
void wtk_xvprint_kind_notify(wtk_xvprint_t *v,wtk_kfeat_t *feat);
void wtk_xvprint_compute_feat_vad(wtk_xvprint_t *v);
int wtk_xvprint_feed2(wtk_xvprint_t *v,short *data,int len,int is_end);
int wtk_xvprint_feed_vad(wtk_xvprint_t *v,short *data,int len,int is_end);
int wtk_xvprint_feed_vad2(wtk_xvprint_t *v,short *data,int len,int is_end);
void wtk_xvprint_compute_feat_vad_dnn(wtk_xvprint_t *v);
char wtk_xvprint_kind_vad_callback(wtk_xvprint_t *v,wtk_kfeat_t *feat);
void wtk_xvprint_comute_feat_vad_flush_end(wtk_xvprint_t *v);
void wtk_xvprint_comute_feat_vad_flush_end2(wtk_xvprint_t *v);
int wtk_xvprint_load_enroll_feat(wtk_xvprint_t *v);
void wtk_xvprint_release_enroll_feat(wtk_xvprint_t *v);
// FILE *file;

#define USE_VAD 

int wtk_xvprint_load_enroll_feat(wtk_xvprint_t *v)
{
    wtk_xvprint_cfg_t *cfg=v->cfg;
    wtk_strbuf_t *buf;

    wtk_queue_init(&v->enroll_q);
    buf=wtk_strbuf_new(128,0);

    if(cfg->load_enroll_fn && cfg->enroll_fn)
    {
		wtk_strbuf_push(buf,cfg->enroll_fn,strlen(cfg->enroll_fn));
		wtk_strbuf_push(buf,".idx",sizeof(".idx"));
		// if( cfg->enroll_fn && wtk_file_exist(cfg->enroll_fn)==0 )
		if( cfg->enroll_fn && wtk_file_exist(buf->data)==0 )
		{
			// ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)wtk_xvprint_cfg_read_enroll_data,cfg->enroll_fn);
			// if(ret!=0){goto end;}
			wtk_xvprint_file2_load(cfg->enroll_fn,&v->enroll_q);
		}
    }

    wtk_strbuf_delete(buf);
    return 0;
}

void wtk_xvprint_release_enroll_feat(wtk_xvprint_t *v)
{
    wtk_xvprint_cfg_feat_node_t *node;
    wtk_queue_node_t *qn,*qn2;

    for(qn=v->enroll_q.pop;qn;qn=qn2)
    {
        qn2=qn->next;
        node=data_offset2(qn,wtk_xvprint_cfg_feat_node_t,qn);
        wtk_xvprint_cfg_node_delete(node);
    }
}

wtk_xvprint_t* wtk_xvprint_new(wtk_xvprint_cfg_t *cfg)
{
    wtk_xvprint_t *v;

    v=(wtk_xvprint_t*)wtk_malloc(sizeof(*v));
    v->cfg=cfg;
    v->x=wtk_nnet3_xvector_compute_new(&cfg->xvector);
    v->scoring=wtk_ivector_plda_scoring_new(&cfg->plda_scoring);

    wtk_nnet3_xvector_compute_set_kind_notify(v->x,v,(wtk_nnet3_xvector_compute_kxparm_notify_f)wtk_xvprint_kind_notify);
    wtk_queue_init(&v->feat_q);
    wtk_queue_init(&v->vad_q);
    v->v=NULL;
    {
        v->v=wtk_vad_new(&cfg->vad,&v->vad_q);
        wtk_kxparm_set_vad_callback(v->x->kxparm,v,(wtk_kxparm_callback_f)wtk_xvprint_kind_vad_callback);
        v->vrobin=wtk_robin_new((cfg->vad_frames_context<<1)+1);
        v->state_l=wtk_larray_new(1024,sizeof(char));
    }
    wtk_xvprint_load_enroll_feat(v);
    v->notify_f=NULL;
    v->ths=NULL;
    // printf("xvprint %p\n",v);
    // file=fopen("r.bin","w");

    return v;
}

void  wtk_xvprint_delete(wtk_xvprint_t *v)
{
    // fclose(file);

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
    wtk_nnet3_xvector_compute_delete(v->x);
    wtk_ivector_plda_scoring_delete(v->scoring);
    wtk_xvprint_release_enroll_feat(v);
    wtk_free(v);
}


int wtk_xvprint_start(wtk_xvprint_t *v)
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

void wtk_xvprint_reset(wtk_xvprint_t *v)
{
    if(v->v)
    {
        wtk_robin_reset(v->vrobin);
        wtk_vad_reset(v->v);
    }
    wtk_nnet3_xvector_compute_reset(v->x);
}

int wtk_xvprint_start2(wtk_xvprint_t *v)
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

void wtk_xvprint_reset2(wtk_xvprint_t *v)
{
    if(v->v)
    {
        wtk_robin_reset(v->vrobin);
    }
    wtk_nnet3_xvector_compute_reset(v->x);
}

int wtk_xvprint_feed(wtk_xvprint_t *v,short *data,int len,int is_end)
{
    int ret;

    if(v->v)
    {
        ret=wtk_xvprint_feed_vad2(v,data,len,is_end);
    }else
    {
       ret=wtk_xvprint_feed2(v,data,len,is_end);
    }
    return ret;
}

int wtk_xvprint_feed2(wtk_xvprint_t *v,short *data,int len,int is_end)
{
    int ret;

    ret=wtk_nnet3_xvector_compute_feed(v->x,data,len,is_end);

    return ret;
}


int wtk_xvprint_feed_vad2(wtk_xvprint_t *v,short *data,int len,int is_end)
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
                wtk_xvprint_start2(v);
                wtk_nnet3_xvector_compute_feed(v->x,vf->wav_data,vf->frame_step,0);
                v->vad_state=1;
            }else
            {
                
            }
            break;
        case 1:
            if(vf->state==wtk_vframe_sil)
            {
                wtk_xvprint_comute_feat_vad_flush_end(v); 
                wtk_xvprint_reset2(v);
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
            wtk_xvprint_comute_feat_vad_flush_end(v);   
        }   
    }
    return ret;
}

int wtk_xvprint_feed_vad(wtk_xvprint_t *v,short *data,int len,int is_end)
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
        wtk_xvprint_comute_feat_vad_flush_end(v);
#else        
        wtk_xvprint_comute_feat_vad_flush_end2(v);
#endif        
    }
    return ret;
}

char wtk_xvprint_kind_vad_callback(wtk_xvprint_t *v,wtk_kfeat_t *feat)
{
    char state;
    char *ptr;

    ptr=wtk_larray_get(v->state_l,v->vad_pop_index);

    state=*ptr;
    v->vad_pop_index++;

    return state;
}

void wtk_xvprint_kind_notify(wtk_xvprint_t *v,wtk_kfeat_t *feat)
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

void wtk_xvprint_comute_feat_vad_flush_end(wtk_xvprint_t *v)
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


void wtk_xvprint_comute_feat_vad_flush_end2(wtk_xvprint_t *v)
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

wtk_string_t* wtk_xvprint_feat_likelihood(wtk_xvprint_t *v,wtk_vecf_t *feat)
{
    wtk_xvprint_cfg_feat_node_t *node,*max_node;
    wtk_queue_node_t *qn;
    float likelihood;
    float max_score;

    max_score=-99999.99f;
    max_node=0;

//    printf("aaa %d\n",v->cfg->enroll_q.length);
    for(qn=v->enroll_q.pop;qn;qn=qn->next)
    {
        node=data_offset2(qn,wtk_xvprint_cfg_feat_node_t,qn);
        likelihood=wtk_ivector_plda_scoring_loglikelihood(v->scoring,node->v,node->num,feat);
        if(max_score<likelihood)
        {
            max_score=likelihood;
            max_node=node;
        }
        printf("%.*s %f\n",node->name->len,node->name->data,likelihood);
    }

    if(v->notify_f)
    {
        if(max_node)
        {
            v->notify_f(v->ths,max_node->name,max_score);
        }else
        {
             v->notify_f(v->ths,NULL,-99999.99f);
        }
    }

    if(max_score>v->cfg->score_thresh)
    {

        // printf("%.*s %f\n",max_node->name->len,max_node->name->data,max_score);
        return (max_node->name);
    }
    return NULL;
}

void wtk_xvprint_set_score_notify(wtk_xvprint_t *v,void *ths,wtk_xvprint_score_notify notify)
{
    v->ths=ths;
    v->notify_f=notify;
}

int wtk_xvprint_feat_likelihood_node(wtk_xvprint_t *v,wtk_vecf_t *feat,wtk_xvprint_cfg_feat_node_t *node)
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

wtk_vecf_t* wtk_xvprint_compute_feat(wtk_xvprint_t *v)
{
    int cnt;

    return wtk_xvprint_compute_feat2(v,&cnt);
}

wtk_vecf_t* wtk_xvprint_compute_feat2(wtk_xvprint_t *v,int *spk_cnt)
{
    wtk_vecf_t *vec;
    // int i;

    vec=wtk_nnet3_xvector_compute_normalize(v->x);
    *spk_cnt=wtk_net3_xvector_compute_get_spk_cnt(v->x);
    
    wtk_ivector_plda_scoring_transfrom_ivector(v->scoring,vec);
    vec=wtk_ivector_plda_scoring_ivector_normalization(v->scoring,*spk_cnt);

    wtk_nnet3_xvector_compute_normalize_reset(v->x);

    // for(i=0;i<vec->len;++i)
    // {
    //     printf("%d %f\n",i,vec->p[i]);
    // }

    return vec;
}

int wtk_xvprint_enroll2file(wtk_xvprint_t *v,wtk_string_t *name)
{
    wtk_xvprint_cfg_feat_node_t *node;
    wtk_vecf_t *vec;
    int cnt;
    int ret;

    vec=wtk_xvprint_compute_feat2(v,&cnt);
    node=wtk_xvprint_cfg_node_new2(name,vec,cnt);

    ret=wtk_xvprint_file2_append_feat(v->cfg->enroll_fn,node);

    wtk_xvprint_cfg_node_delete(node);
    // printf("enroll cnt %d\n",cnt);
    return ret;
}

int wtk_xvprint_enroll2mem(wtk_xvprint_t *v,wtk_string_t *name)
{
    wtk_xvprint_cfg_feat_node_t *node;
    wtk_vecf_t *vec;
    int cnt;

    // printf("%.*s\n",name->len,name->data);
    // exit(0);
    vec=wtk_xvprint_compute_feat2(v,&cnt);
    // printf("cnttt %d\n",cnt);
    node=wtk_xvprint_cfg_node_new2(name,vec,cnt);
    wtk_queue_push(&v->enroll_q,&node->qn);

    // wtk_string_t n;
    // wtk_string_set(&n,"C0066",sizeof("C0066")-1);
    // node=wtk_xvprint_file_load_feat("./x.bin",&n);
    // wtk_queue_push(&v->cfg->enroll_q,&node->qn);

    // wtk_xvprint_file_print_head("./x.bin");

    // wtk_string_t n;
    // wtk_string_set(&n,"D0234",sizeof("D0234")-1);
    // wtk_xvprint_file_delete_person("./x.bin",&n);

    // wtk_xvprint_file_append("./x.bin",node);
    // wtk_xvprint_file2_insert_feat(v->cfg->enroll_fn,node);
    // wtk_xvprint_file2_print_head(v->cfg->enroll_fn);
    // wtk_xvprint_file_print_head("./x.bin");
    // wtk_vecf_print(node->v);
    // int i;
    // for(i=0;i<node->v->len;++i)
    // {
    //     printf("%d %f\n",i,node->v->p[i]);
    // }

    return 0;
}

int wtk_xprint_dump_memory2file(wtk_xvprint_t *v)
{
    return wtk_xvprint_file2_dump(v->cfg->enroll_fn,&v->enroll_q);
}

wtk_xvprint_cfg_feat_node_t* wtk_xprint_query_memory_node(wtk_xvprint_t *v,wtk_string_t *name)
{
    wtk_xvprint_cfg_feat_node_t *node,*node2;
    wtk_queue_node_t *qn;

    node2=NULL;
    for(qn=v->enroll_q.pop;qn;qn=qn->next)
    {
        node=data_offset2(qn,wtk_xvprint_cfg_feat_node_t,qn);
        if(wtk_string_cmp(name,node->name->data,node->name->len)==0)
        {
            node2=node;
            break;
        }
    }

    return node2;
}

#if 0

void wtk_xvprint_compute_feat_vad_dnn(wtk_xvprint_t *v)
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



void wtk_xvprint_compute_feat_vad4(wtk_xvprint_t *v)
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


void wtk_xvprint_compute_feat_vad2(wtk_xvprint_t *v)
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


void wtk_xvprint_compute_feat_vad3(wtk_xvprint_t *v)
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
