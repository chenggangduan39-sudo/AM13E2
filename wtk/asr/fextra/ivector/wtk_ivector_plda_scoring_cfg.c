#include "wtk_ivector_plda_scoring_cfg.h"
#include "wtk/asr/fextra/kparm/knn/wtk_knn_cfg.h"
#ifdef USE_ARM
#include "wtk/os/asm/arm/wtk_neon_math.h"
#endif
wtk_vecf_t* wtk_ivector_plda_scoring_cfg_read_vector(wtk_ivector_plda_scoring_cfg_t *cfg,wtk_source_t *src);
int wtk_ivector_plda_scoring_cfg_read_transform_mat(wtk_ivector_plda_scoring_cfg_t *cfg,wtk_source_t *src);
int wtk_ivector_plda_scoring_cfg_read_plda(wtk_ivector_plda_scoring_cfg_t *cfg,wtk_source_t *src);
int wtk_ivector_plda_scoring_cfg_read_enroll_xvector(wtk_ivector_plda_scoring_cfg_t *cfg,wtk_source_t *src);

int wtk_ivector_plda_scoring_cfg_init(wtk_ivector_plda_scoring_cfg_t *cfg)
{
    cfg->mean=NULL;
    cfg->transform=NULL;
    cfg->psi=NULL;
    cfg->plda_fn=NULL;


    return 0;
}

int wtk_ivector_plda_scoring_cfg_clean(wtk_ivector_plda_scoring_cfg_t *cfg)
{
    if(cfg->mean)
    {
        wtk_vecf_delete(cfg->mean);
    }

    if(cfg->psi)
    {
        wtk_vecf_delete(cfg->psi);
    }

    if(cfg->transform)
    {
        wtk_matf_delete(cfg->transform);
    }

    if(cfg->offset)
    {
        wtk_vecf_delete(cfg->offset);
    }

    return 0;
}

int wtk_ivector_plda_scoring_cfg_update_local(wtk_ivector_plda_scoring_cfg_t *cfg, wtk_local_cfg_t *main)
{
    wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret;

	lc=main;

    wtk_local_cfg_update_cfg_str(lc,cfg,plda_fn,v);

    ret=0;
    return ret;
}

int wtk_ivector_plda_scoring_cfg_update(wtk_ivector_plda_scoring_cfg_t *cfg)
{
    int ret;
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;

    ret=wtk_ivector_plda_scoring_cfg_update2(cfg,&sl);
    if(ret!=0)
    {
        goto end;
    }

    ret=0;
end:    
    return ret;
}

int wtk_ivector_plda_scoring_cfg_update2(wtk_ivector_plda_scoring_cfg_t *cfg,wtk_source_loader_t *sl)
{
    int ret;
    //int i,j;
    //float *p;

    ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)wtk_ivector_plda_scoring_cfg_read_plda,cfg->plda_fn);
    if(ret!=0){goto end;}
/*
    cfg->offset=wtk_vecf_new(cfg->transform->row);
    p=cfg->transform->p;
    for(i=0;i<cfg->transform->row;++i)
    {
        for(j=0;j<cfg->mean->len;++j)
        {
            cfg->offset->p[i]+=cfg->mean->p[j]*p[j]*(-1.0f);
        }
        // printf("%d %f\n",i,cfg->offset->p[i]);
        p+=cfg->transform->col;
    }
#ifdef USE_ARM
    wtk_matf_t *tmp;
	tmp=wtk_neon_math_mat_transf_8float(cfg->transform);
	wtk_matf_delete(cfg->transform);
	cfg->transform=tmp;
#endif*/
    // ret=wtk_nnet3_xvector_compute_cfg_update2(&cfg->xvector,sl);
    // if(ret!=0){goto end;}

    ret=0;
end:
    return ret;
}

int wtk_ivector_plda_scoring_cfg_read_plda(wtk_ivector_plda_scoring_cfg_t *cfg,wtk_source_t *src)
{
    int ret;

    ret=wtk_source_seek_to_s(src,"<Plda>");
	if(ret!=0)
	{
		wtk_debug("expected <Plda> failed\n");
		goto end;
	}

    cfg->psi=wtk_ivector_plda_scoring_cfg_read_vector(cfg,src);
    if(cfg->psi==NULL)
    {
         wtk_debug("read psi failed\n");
         ret=-1;goto end;
    }
    // wtk_vecf_print(cfg->mean);

    ret=wtk_ivector_plda_scoring_cfg_read_transform_mat(cfg,src);
    if(ret!=0)
    {
        wtk_debug("read transf mat failed\n");
        goto end;
    }

    // wtk_matf_print(cfg->transform);

    cfg->offset=wtk_ivector_plda_scoring_cfg_read_vector(cfg,src);
    if(cfg->offset==NULL)
    {
        wtk_debug("read offset failed\n");
        goto end;
    }
    // wtk_vecf_print(cfg->psi);

    ret=wtk_source_seek_to_s(src,"</Plda>");
	if(ret!=0)
	{
		wtk_debug("expected </Plda> failed\n");
		goto end;
	}

    ret=0;
end:
    return ret;
}

int wtk_ivector_plda_scoring_cfg_read_transform_mat(wtk_ivector_plda_scoring_cfg_t *cfg,wtk_source_t *src)
{
    wtk_strbuf_t *buf;
    int ret;

    buf=wtk_strbuf_new(1024,1.0f);

    cfg->transform=wtk_knn_read_matrix(src,buf);
    if(!cfg->transform)
    {
		ret=-1;goto end;
    }

    ret=0;
end:    
    wtk_strbuf_delete(buf);
    return ret;
}

wtk_vecf_t* wtk_ivector_plda_scoring_cfg_read_vector(wtk_ivector_plda_scoring_cfg_t *cfg,wtk_source_t *src)
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
