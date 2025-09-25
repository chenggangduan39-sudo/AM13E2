#include "wtk_svprint_cfg.h"
#include "wtk/asr/fextra/kparm/knn/wtk_knn_cfg.h"
#include "wtk/asr/xvprint/wtk_xvprint_file.h"
#include "wtk/asr/xvprint/wtk_xvprint_file2.h"
#include "wtk/asr/fextra/kparm/wtk_kparm_cfg.h"
#include "wtk/asr/fextra/kcmn/wtk_kcmn_cfg.h"

int wtk_svprint_cfg_read_enroll_data(wtk_svprint_cfg_t *cfg,wtk_source_t *src);

int wtk_svprint_cfg_init(wtk_svprint_cfg_t *cfg)
{
    wtk_nnet3_xvector_compute_cfg_init(&cfg->xvector);
    wtk_ivector_plda_scoring_cfg_init(&cfg->plda_scoring);
    // wtk_vad_cfg_init(&cfg->vad);

    cfg->score_thresh=-290.0f;//-29

    cfg->enroll_fn=NULL;
    cfg->pool_fn=NULL;
    wtk_queue_init(&(cfg->enroll_q));

    cfg->vad_fn=NULL;
    // cfg->main_vad_cfg=NULL;
    cfg->vad_energy_thresh=5.5;//5.5;
    cfg->vad_energy_mean_scale=0.5;//0.6;//0.5;
    cfg->vad_proportion_threshold=0.12;//0.12;
    cfg->vad_frames_context=2;

    cfg->load_enroll_fn=1;
    cfg->use_vad_cut=0;
    cfg->use_plda = 0;
    cfg->use_distance = 1;
    cfg->max_spks = 60;
    return 0;
}

int wtk_svprint_cfg_clean(wtk_svprint_cfg_t *cfg)
{
    wtk_queue_node_t *qn,*qn2;
    wtk_svprint_cfg_feat_node_t *node;

    if(cfg->use_vad_cut)
    {
        wtk_vad_cfg_clean(&cfg->vad);
    }

    wtk_nnet3_xvector_compute_cfg_clean(&cfg->xvector);
    wtk_ivector_plda_scoring_cfg_clean(&cfg->plda_scoring);

    for(qn=cfg->enroll_q.pop;qn;qn=qn2)
    {
        qn2=qn->next;
        node=data_offset2(qn,wtk_svprint_cfg_feat_node_t,qn);
        wtk_svprint_cfg_node_delete(node);
    }

    // if(cfg->main_vad_cfg)
    // {
    //     wtk_mbin_cfg_delete(cfg->main_vad_cfg);
    // }
	return 0;
}

int wtk_svprint_cfg_update_local(wtk_svprint_cfg_t *cfg, wtk_local_cfg_t *main)
{
    wtk_local_cfg_t *lc,*lc2;
	wtk_string_t *v;
	int ret;

	lc=main;

    wtk_local_cfg_update_cfg_str(lc,cfg,enroll_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,pool_fn,v);
//     printf("%s\n",cfg->enroll_fn);
    wtk_local_cfg_update_cfg_str(lc,cfg,vad_fn,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,vad_energy_thresh,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,vad_energy_mean_scale,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,vad_proportion_threshold,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,vad_frames_context,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,score_thresh,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,load_enroll_fn,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_vad_cut,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_plda,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_distance,v);

    lc2=wtk_local_cfg_find_lc_s(lc,"xvector");
    if(lc2)
    {
        wtk_nnet3_xvector_compute_cfg_update_local(&cfg->xvector,lc2);
    }

    lc2=wtk_local_cfg_find_lc_s(lc,"scoring");
    if(lc2)
    {
        wtk_ivector_plda_scoring_cfg_update_local(&cfg->plda_scoring,lc2);
    }

    lc2=wtk_local_cfg_find_lc_s(lc,"vad");
    if(lc2)
    {
        wtk_vad_cfg_init(&cfg->vad);
        wtk_vad_cfg_update_local(&cfg->vad,lc2);
        cfg->use_vad_cut=1;
    }

    ret=0;
    return ret;
}

int wtk_svprint_cfg_update(wtk_svprint_cfg_t *cfg)
{
    int ret;
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;

    ret=wtk_svprint_cfg_update2(cfg,&sl);
    if(ret!=0)
    {
        goto end;
    }

    ret=0;
end:    
    return ret;
}

int wtk_svprint_cfg_update2(wtk_svprint_cfg_t *cfg,wtk_source_loader_t *sl)
{
    int ret;
    wtk_strbuf_t *buf;

    buf=wtk_strbuf_new(128,0);

    ret=wtk_nnet3_xvector_compute_cfg_update2(&cfg->xvector,sl);
    if(ret!=0){goto end;}

    ret=wtk_ivector_plda_scoring_cfg_update2(&cfg->plda_scoring,sl);
    if(ret!=0){goto end;}

    if(cfg->use_vad_cut)
    {
        ret=wtk_vad_cfg_update2(&cfg->vad,sl);
        if(ret!=0){goto end;}
    }

    if(cfg->load_enroll_fn && cfg->enroll_fn)
    {
		wtk_strbuf_push(buf,cfg->enroll_fn,strlen(cfg->enroll_fn));
		wtk_strbuf_push(buf,".idx",sizeof(".idx"));
		// if( cfg->enroll_fn && wtk_file_exist(cfg->enroll_fn)==0 )
		if( cfg->enroll_fn && wtk_file_exist(buf->data)==0 )
		{
			// ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)wtk_svprint_cfg_read_enroll_data,cfg->enroll_fn);
			// if(ret!=0){goto end;}
			wtk_xvprint_file2_load(cfg->enroll_fn,&cfg->enroll_q);
		}
    }

    // if(cfg->vad_fn)
    // {
    //     cfg->main_vad_cfg=wtk_mbin_cfg_new_type(wtk_vad_cfg,cfg->vad_fn,"./vad.cfg.r");
    // }
    // wtk_string_t n;
    // wtk_string_set(&n,"D1216",sizeof("D1216")-1);
    // wtk_xvprint_file2_delete_feat(cfg->enroll_fn,&n);
    // wtk_xvprint_file2_print_head(cfg->enroll_fn);
    // exit(0);
    ret=0;
end:
    wtk_strbuf_delete(buf);
    return ret;
}

int wtk_svprint_cfg_set_vpint_fn(wtk_svprint_cfg_t *cfg,char *fn)
{
	int ret=0;
    wtk_strbuf_t *buf;

    buf=wtk_strbuf_new(128,0);

	cfg->enroll_fn=fn;
	if(cfg->load_enroll_fn)
	{
		wtk_strbuf_push(buf,cfg->enroll_fn,strlen(cfg->enroll_fn));
		wtk_strbuf_push(buf,".idx",sizeof(".idx"));
		// if( cfg->enroll_fn && wtk_file_exist(cfg->enroll_fn)==0 )
		if( cfg->enroll_fn && wtk_file_exist(buf->data)==0 )
		{
			// ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)wtk_svprint_cfg_read_enroll_data,cfg->enroll_fn);
			// if(ret!=0){goto end;}
			ret=wtk_xvprint_file2_load(cfg->enroll_fn,&cfg->enroll_q);
		}
	}

    wtk_strbuf_delete(buf);
	return ret;
}

int wtk_svprint_cfg_set_enroll_stats_fn(wtk_svprint_cfg_t *cfg,char *fn)
{
	int ret=0;
	wtk_source_loader_t sl2;

	cfg->xvector.kxparm.parm.cmvn_stats_fn=fn;
	if(fn && wtk_file_exist(fn)==0)
	{
		sl2.hook=0;
		sl2.vf=wtk_source_load_file_v;
		ret=wtk_source_loader_load(&sl2,&cfg->xvector.kxparm.parm,(wtk_source_load_handler_t)wtk_kparm_cfg_load_cmvn_stats,fn);
	}

	return ret;
}

int wtk_svprint_cfg_set_eval_stats_fn(wtk_svprint_cfg_t *cfg,char *fn)
{
	int ret=0;
	wtk_source_loader_t sl2;

	cfg->xvector.kxparm.parm.kcmvn.cmn_fn=fn;

	if(fn && wtk_file_exist(fn)==0)
	{
		sl2.hook=0;
		sl2.vf=wtk_source_load_file_v;
		ret=wtk_source_loader_load(&sl2,&cfg->xvector.kxparm.parm.kcmvn,(wtk_source_load_handler_t)wtk_kcmn_zmean_cfg_load_cmn2,fn);
	}



	return ret;
}

wtk_vecf_t* wtk_svprint_cfg_read_vector(wtk_svprint_cfg_t *cfg,wtk_source_t *src)
{
    wtk_strbuf_t *buf;
    wtk_vecf_t *vec;

    buf=wtk_strbuf_new(1024,1.0f);

    vec=wtk_knn_read_vector(src,buf);
	if(!vec)
	{
		wtk_debug("read plda vec failed\n");
		goto end;
	}

end:    
    wtk_strbuf_delete(buf);
    return vec;
}


wtk_svprint_cfg_feat_node_t* wtk_svprint_cfg_node_new3(int name_len)
{
    wtk_svprint_cfg_feat_node_t *node;

    node=wtk_calloc(1,sizeof(*node));
    node->name=wtk_string_new(name_len);

    return node;
}

wtk_svprint_cfg_feat_node_t* wtk_svprint_cfg_node_new2(wtk_string_t *name,wtk_vecf_t *feat,int cnt)
{
    wtk_svprint_cfg_feat_node_t *node;

    node=wtk_calloc(1,sizeof(*node));
    node->name=wtk_string_dup_data(name->data,name->len);
    node->v=wtk_vecf_dup(feat);
    node->num=cnt;

    return node;
}


wtk_svprint_cfg_feat_node_t* wtk_svprint_cfg_node_new(char *name,int len)
{
    wtk_svprint_cfg_feat_node_t *node;

    node=wtk_calloc(1,sizeof(*node));
    node->name=wtk_string_dup_data(name,len);

    return node;
}

void wtk_svprint_cfg_node_delete(wtk_svprint_cfg_feat_node_t *node)
{
    if(node->name)
    {
        wtk_free(node->name);
    }
    if(node->v)
    {
        wtk_vecf_delete(node->v);
    }
    wtk_free(node);
}

#if 0

int wtk_svprint_cfg_read_enroll_data(wtk_svprint_cfg_t *cfg,wtk_source_t *src)
{
    int ret;
    int nl;
    wtk_strbuf_t *buf;
    wtk_svprint_cfg_feat_node_t *node;

    buf=wtk_strbuf_new(512,0.0);

    while(1)
    {
        ret=wtk_source_read_string(src,buf);
        if(ret!=0)
        {
            if(ret==EOF && buf->pos==0)
            {
                ret=0;
                break;
            }
            goto end;
        }
        nl=0;
		wtk_source_skip_sp(src,&nl);
		if(nl){break;}
        node=wtk_svprint_cfg_node_new(buf->data,buf->pos);
        nl=0;
		wtk_source_skip_sp(src,&nl);
		if(nl){break;}
        ret=wtk_source_read_int(src,&node->num,1,0);
        if(ret!=0){goto end;}
        node->v=wtk_svprint_cfg_read_vector(cfg,src);
        if(!node->v)
        {
            ret=-1;
            goto end;
        }
        wtk_queue_push(&cfg->enroll_q,&node->qn);
    }
    

    ret=0;
end:
    wtk_strbuf_delete(buf);
    return ret;
}

#endif
