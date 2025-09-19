#include "wtk_qform3_cfg.h"

int wtk_qform3_cfg_init(wtk_qform3_cfg_t *cfg)
{
    wtk_stft2_cfg_init(&(cfg->stft2));
    wtk_bf_cfg_init(&(cfg->bf));
    wtk_rls3_cfg_init(&(cfg->rls3));
    wtk_nlms_cfg_init(&(cfg->nlms));
    wtk_qmmse_cfg_init(&(cfg->qmmse));
    wtk_admm2_cfg_init(&(cfg->admm));
	wtk_aspec_cfg_init(&(cfg->aspec));

    cfg->use_admm2=0;

    cfg->debug=0;

    cfg->rate=16000;

    cfg->use_preemph=0;

    cfg->use_post=1;

    cfg->XN=1;

    cfg->coh_alpha=0.93;

    cfg->use_nlms=0;

    cfg->use_line=0;

    cfg->use_ls_bf=0;

    cfg->theta_range=20;

    cfg->use_aspec = 0;
    cfg->spec_thresh = 600;
    cfg->spec_cnt = 20;
    cfg->reduce_cnt = 5;
    cfg->min_scale = 0.1;

    return 0;
}


int wtk_qform3_cfg_clean(wtk_qform3_cfg_t *cfg)
{
    wtk_stft2_cfg_clean(&(cfg->stft2));
    wtk_bf_cfg_clean(&(cfg->bf));
    wtk_rls3_cfg_clean(&(cfg->rls3));
    wtk_nlms_cfg_clean(&(cfg->nlms));
    wtk_qmmse_cfg_clean(&(cfg->qmmse));
    wtk_admm2_cfg_clean(&(cfg->admm));
	wtk_aspec_cfg_clean(&(cfg->aspec));

    return 0;
}

int wtk_qform3_cfg_update_local(wtk_qform3_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    wtk_string_t *v;
    wtk_local_cfg_t *m;
    int ret;

    wtk_local_cfg_update_cfg_b(lc,cfg,use_preemph,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_post,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_nlms,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,debug,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_admm2,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_line,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_ls_bf,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,coh_alpha,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,XN,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,theta_range,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_aspec,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,spec_thresh,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,spec_cnt,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,reduce_cnt,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,min_scale,v);

    m=lc;
	lc=wtk_local_cfg_find_lc_s(m,"stft2");
	if(lc)
	{
        ret=wtk_stft2_cfg_update_local(&(cfg->stft2),lc);
        if(ret!=0){goto end;}
    }
    lc=wtk_local_cfg_find_lc_s(m,"bf");
	if(lc)
	{
        ret=wtk_bf_cfg_update_local(&(cfg->bf),lc);
        if(ret!=0){goto end;}
    }
    lc=wtk_local_cfg_find_lc_s(m,"rls");
	if(lc)
	{
        ret=wtk_rls3_cfg_update_local(&(cfg->rls3),lc);
        if(ret!=0){goto end;}
    }
    lc=wtk_local_cfg_find_lc_s(m,"nlms");
	if(lc)
	{
        ret=wtk_nlms_cfg_update_local(&(cfg->nlms),lc);
        if(ret!=0){goto end;}
    }
    lc=wtk_local_cfg_find_lc_s(m,"admm2");
	if(lc)
	{
        ret=wtk_admm2_cfg_update_local(&(cfg->admm),lc);
        if(ret!=0){goto end;}
    }
    lc=wtk_local_cfg_find_lc_s(m,"qmmse");
	if(lc)
	{
        cfg->qmmse.step=cfg->stft2.win/2;
        ret=wtk_qmmse_cfg_update_local(&(cfg->qmmse),lc);
        if(ret!=0){goto end;}
    }
	lc=wtk_local_cfg_find_lc_s(m,"aspec");
	if(lc)
	{
        ret=wtk_aspec_cfg_update_local(&(cfg->aspec),lc);
        if(ret!=0){goto end;}
    }

end:
    return ret;
}

int wtk_qform3_cfg_update(wtk_qform3_cfg_t *cfg)
{
    int ret;

    if(cfg->use_nlms)
    {
        cfg->nlms.channel=1;
        cfg->nlms.M=cfg->XN;
        cfg->nlms.N=1;
        ret=wtk_nlms_cfg_update(&(cfg->nlms));
        if(ret!=0){goto end;}
    }else
    {
        cfg->rls3.channel=1;
        cfg->rls3.L=cfg->XN;
        cfg->rls3.N=1;
        ret=wtk_rls3_cfg_update(&(cfg->rls3));
        if(ret!=0){goto end;}
    }

    ret=wtk_stft2_cfg_update(&(cfg->stft2));
    if(ret!=0){goto end;}
    ret=wtk_bf_cfg_update(&(cfg->bf));
    if(ret!=0){goto end;}
    ret=wtk_admm2_cfg_update(&(cfg->admm));
    if(ret!=0){goto end;}
    ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
    if(ret!=0){goto end;}
    ret=wtk_aspec_cfg_update(&(cfg->aspec));
    if(ret!=0){goto end;}

    ret=0;
end:
    return ret;
}

int wtk_qform3_cfg_update2(wtk_qform3_cfg_t *cfg,wtk_source_loader_t *sl)
{
    int ret;

    if(cfg->use_nlms)
    {
        cfg->nlms.channel=1;
        cfg->nlms.M=cfg->XN;
        cfg->nlms.N=1;
        ret=wtk_nlms_cfg_update(&(cfg->nlms));
        if(ret!=0){goto end;}
    }else
    {
        cfg->rls3.channel=1;
        cfg->rls3.L=cfg->XN;
        cfg->rls3.N=1;
        ret=wtk_rls3_cfg_update(&(cfg->rls3));
        if(ret!=0){goto end;}
    }

    ret=wtk_stft2_cfg_update(&(cfg->stft2));
    if(ret!=0){goto end;}
    ret=wtk_bf_cfg_update(&(cfg->bf));
    if(ret!=0){goto end;}
    ret=wtk_admm2_cfg_update(&(cfg->admm));
    if(ret!=0){goto end;}
    ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
    if(ret!=0){goto end;}
    ret=wtk_aspec_cfg_update(&(cfg->aspec));
    if(ret!=0){goto end;}

	ret=0;
end:
    return ret;
}

wtk_qform3_cfg_t* wtk_qform3_cfg_new(char *cfg_fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_qform3_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_qform3_cfg,cfg_fn);
	cfg=(wtk_qform3_cfg_t*)main_cfg->cfg;
	cfg->main_cfg=main_cfg;
	return cfg;
}

void wtk_qform3_cfg_delete(wtk_qform3_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_qform3_cfg_t* wtk_qform3_cfg_new_bin(char *bin_fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_qform3_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_qform3_cfg,bin_fn,"./cfg");
	cfg=(wtk_qform3_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_qform3_cfg_delete_bin(wtk_qform3_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}

void wtk_qform3_cfg_set_noise_suppress(wtk_qform3_cfg_t *cfg,float noise_suppress)
{
    if(fabs(noise_suppress)>0.0)
    {
        wtk_qmmse_cfg_set_noise_suppress(&(cfg->qmmse),noise_suppress);
        cfg->use_post=1;
    }
}

