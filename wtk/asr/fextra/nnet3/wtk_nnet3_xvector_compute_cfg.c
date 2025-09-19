#include "wtk_nnet3_xvector_compute_cfg.h"
int wtk_nnet3_xvector_compute_cfg_read_mean_vector(wtk_nnet3_xvector_compute_cfg_t *cfg,wtk_source_t *src);
int wtk_nnet3_xvector_compute_cfg_read_transform_mat(wtk_nnet3_xvector_compute_cfg_t *cfg,wtk_source_t *src);

int wtk_nnet3_xvector_compute_cfg_init(wtk_nnet3_xvector_compute_cfg_t *cfg)
{
#ifdef ONNX_DEC
        qtk_onnxruntime_cfg_init(&(cfg->onnx));
#endif
    wtk_kxparm_cfg_init(&(cfg->kxparm));
    cfg->mean_vec_fn=NULL;
    cfg->mean_vec=NULL;
    cfg->transform=NULL;
    cfg->transform_fn=NULL;
    cfg->use_onnx=0;
    cfg->use_parm = 1;
    cfg->use_ivector_mean=1;
    cfg->use_ivector_norm_len=1;
    cfg->use_normalize=0;
    return 0;
}

int wtk_nnet3_xvector_compute_cfg_clean(wtk_nnet3_xvector_compute_cfg_t *cfg)
{
#ifdef ONNX_DEC
        qtk_onnxruntime_cfg_clean(&(cfg->onnx));
#endif
    wtk_kxparm_cfg_clean(&(cfg->kxparm));
    if(cfg->mean_vec)
    {
        wtk_vecf_delete(cfg->mean_vec);
    }
    if(cfg->transform)
    {
        wtk_matf_delete(cfg->transform);
    }

    return 0;
}

int wtk_nnet3_xvector_compute_cfg_update_local(wtk_nnet3_xvector_compute_cfg_t *cfg, wtk_local_cfg_t *main)
{
    wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret;

	lc=main;

    wtk_local_cfg_update_cfg_str(lc,cfg,transform_fn,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,mean_vec_fn,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_onnx,v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_parm, v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_ivector_mean,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_ivector_norm_len,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_normalize,v);

#ifdef ONNX_DEC
	lc=wtk_local_cfg_find_lc_s(main,"onnx");
	if(lc)
	{
			ret=qtk_onnxruntime_cfg_update_local(&(cfg->onnx),lc);
			if(ret!=0){goto end;}
	}
#endif

    lc=wtk_local_cfg_find_lc_s(main,"parm");
	if(lc)
    {
        ret=wtk_kxparm_cfg_update_local(&(cfg->kxparm),lc);
        if(ret!=0){goto end;}
    }

    ret=0;
end:
    return ret;
}



int wtk_nnet3_xvector_compute_cfg_update(wtk_nnet3_xvector_compute_cfg_t *cfg)
{
    int ret;
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;

    ret=wtk_nnet3_xvector_compute_cfg_update2(cfg,&sl);
    if(ret!=0)
    {
        goto end;
    }

    ret=0;
end:    
    return ret;
}

int wtk_nnet3_xvector_compute_cfg_update2(wtk_nnet3_xvector_compute_cfg_t *cfg,wtk_source_loader_t *sl)
{
    int ret;

    // wtk_kxparm_cfg_update(&cfg->kxparm);
    wtk_kxparm_cfg_update2(&cfg->kxparm,sl);
    if(cfg->mean_vec_fn)
    {
        ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)wtk_nnet3_xvector_compute_cfg_read_mean_vector,cfg->mean_vec_fn);
		if(ret!=0){goto end;}
    }

    if(cfg->transform_fn)
    {
        ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)wtk_nnet3_xvector_compute_cfg_read_transform_mat,cfg->transform_fn);
	    if(ret!=0){goto end;} 
        
        // int ii,jj;
        // for(ii=0;ii<cfg->transform->row;++ii)
        // {
        //     for(jj=0;jj<cfg->transform->col;++jj)
        //     {
        //         printf("%f ",*(cfg->transform->p+ii*cfg->transform->col+jj));
        //     }
        //     printf("\n");
        // }
    }
#ifdef ONNX_DEC
	ret=qtk_onnxruntime_cfg_update2(&(cfg->onnx),sl->hook);
	if(ret!=0)
	{
			wtk_debug("update onnx failed\n");
			goto end;
	}
#endif


    ret=0;
end:    
    return ret;
}

int wtk_nnet3_xvector_compute_cfg_read_transform_mat(wtk_nnet3_xvector_compute_cfg_t *cfg,wtk_source_t *src)
{
    wtk_strbuf_t *buf;
    int ret;

    buf=wtk_strbuf_new(1024,1.0f);

    cfg->transform=wtk_knn_read_matrix(src,buf);
#ifdef USE_NEON
    wtk_matf_t *m;
    m=wtk_neon_math_mat_transf_8float(cfg->transform);
    wtk_matf_delete(cfg->transform);
    cfg->transform=m;
#endif
    if(!cfg->transform)
    {
        wtk_debug("read transf mat failed\n");
		ret=-1;goto end;
    }

    ret=0;
end:    
    wtk_strbuf_delete(buf);
    return ret;
}

int wtk_nnet3_xvector_compute_cfg_read_mean_vector(wtk_nnet3_xvector_compute_cfg_t *cfg,wtk_source_t *src)
{
    wtk_strbuf_t *buf;
    int ret;

    buf=wtk_strbuf_new(1024,1.0f);

    cfg->mean_vec=wtk_knn_read_vector(src,buf);
	if(!cfg->mean_vec)
	{
		wtk_debug("read mean vec failed\n");
		ret=-1;goto end;
	}

    ret=0;
end:    
    wtk_strbuf_delete(buf);
    return ret;
}
