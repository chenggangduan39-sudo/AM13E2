#include "wtk_eqform_cfg.h"

int wtk_eqform_cfg_init(wtk_eqform_cfg_t *cfg)
{
    wtk_aec_cfg_init(&(cfg->aec));
    wtk_qform9_cfg_init(&(cfg->qform9));
    wtk_qform11_cfg_init(&(cfg->qform11));
    wtk_qform3_cfg_init(&(cfg->qform3));

    cfg->use_post_form=1;
    cfg->use_aec=0;

    cfg->use_enrcheck=0;

    cfg->enrcheck_hist=1000;
    cfg->rate=16000;
    cfg->enr_thresh=20000;
    cfg->use_qform9=1;
    cfg->use_qform11=0;
    cfg->use_qform3=0;

    return 0;
}


int wtk_eqform_cfg_clean(wtk_eqform_cfg_t *cfg)
{
    wtk_aec_cfg_clean(&(cfg->aec));
    wtk_qform9_cfg_clean(&(cfg->qform9));
    wtk_qform11_cfg_clean(&(cfg->qform11));
    wtk_qform3_cfg_clean(&(cfg->qform3));

    return 0;
}

int wtk_eqform_cfg_update_local(wtk_eqform_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    wtk_local_cfg_t *m;
    int ret;
    wtk_string_t *v;

	wtk_local_cfg_update_cfg_b(lc,cfg,use_aec,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_post_form,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_enrcheck,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,enrcheck_hist,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,enr_thresh,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_qform9,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_qform11,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_qform3,v);
    m=lc;
    lc=wtk_local_cfg_find_lc_s(m,"aec");
	if(lc)
	{
        ret=wtk_aec_cfg_update_local(&(cfg->aec),lc);
        if(ret!=0){goto end;}
    }
    if(cfg->use_qform9){
        lc=wtk_local_cfg_find_lc_s(m,"qform9");
        if(lc)
        {
            ret=wtk_qform9_cfg_update_local(&(cfg->qform9),lc);
            if(ret!=0){goto end;}
        }
    }
    if(cfg->use_qform11){
        lc=wtk_local_cfg_find_lc_s(m,"qform11");
        if(lc)
        {
            ret=wtk_qform11_cfg_update_local(&(cfg->qform11),lc);
            if(ret!=0){goto end;}
        }
    }
    if(cfg->use_qform3){
        lc=wtk_local_cfg_find_lc_s(m,"qform3");
        if(lc)
        {
            ret=wtk_qform3_cfg_update_local(&(cfg->qform3),lc);
            if(ret!=0){goto end;}
        }
    }

    ret=0;
end:
    return ret;
}

int wtk_eqform_cfg_update(wtk_eqform_cfg_t *cfg)
{
    int ret;

    ret=wtk_aec_cfg_update(&(cfg->aec));
    if(ret!=0){goto end;}
    if(cfg->use_qform9){
        ret=wtk_qform9_cfg_update(&(cfg->qform9));
        if(ret!=0){goto end;}
    }else if(cfg->use_qform11){
        ret=wtk_qform11_cfg_update(&(cfg->qform11));
        if(ret!=0){goto end;}
    }else if(cfg->use_qform3){
        ret=wtk_qform3_cfg_update(&(cfg->qform3));
        if(ret!=0){goto end;}
    }
    cfg->enrcheck_hist*=cfg->rate/1000;

    ret=0;
end:
    return ret;
}

int wtk_eqform_cfg_update2(wtk_eqform_cfg_t *cfg,wtk_source_loader_t *sl)
{
    int ret;

    ret=wtk_aec_cfg_update(&(cfg->aec));
    if(ret!=0){goto end;}
    if(cfg->use_qform9){
        ret=wtk_qform9_cfg_update(&(cfg->qform9));
        if(ret!=0){goto end;}
    }else if(cfg->use_qform11){
        ret=wtk_qform11_cfg_update(&(cfg->qform11));
        if(ret!=0){goto end;}
    }else if(cfg->use_qform3){
        ret=wtk_qform3_cfg_update(&(cfg->qform3));
        if(ret!=0){goto end;}
    }
    cfg->enrcheck_hist*=cfg->rate/1000;

    ret=0;
end:
    return ret;
}



wtk_eqform_cfg_t* wtk_eqform_cfg_new(char *cfg_fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_eqform_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_eqform_cfg,cfg_fn);
	cfg=(wtk_eqform_cfg_t*)main_cfg->cfg;
	cfg->main_cfg=main_cfg;
	return cfg;
}

void wtk_eqform_cfg_delete(wtk_eqform_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_eqform_cfg_t* wtk_eqform_cfg_new_bin(char *bin_fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_eqform_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_eqform_cfg,bin_fn,"./cfg");
	cfg=(wtk_eqform_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_eqform_cfg_delete_bin(wtk_eqform_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}


void wtk_eqform_cfg_set_theta_range(wtk_eqform_cfg_t *cfg,float theta_range)
{
    wtk_qform9_cfg_set_theta_range(&(cfg->qform9),theta_range);
}

void wtk_eqform_cfg_set_noise_suppress(wtk_eqform_cfg_t *cfg,float noise_suppress)
{
    wtk_qform9_cfg_set_noise_suppress(&(cfg->qform9),noise_suppress);
}
