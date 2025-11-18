#include "wtk_wbf_cfg.h" 

int wtk_wbf_cfg_init(wtk_wbf_cfg_t *cfg)
{
	wtk_stft2_cfg_init(&(cfg->stft2));
	wtk_aspec_cfg_init(&(cfg->aspec));
	wtk_bf_cfg_init(&(cfg->bf));
	wtk_qmmse_cfg_init(&(cfg->qmmse));

    cfg->ncov_alpha=0.1;
    cfg->init_ncovnf=100;

    cfg->scov_alpha=0.1;
    cfg->init_scovnf=100;

    cfg->lf=6;
    cfg->lt=1;

	cfg->wbf_cnt=4;
    cfg->range_interval=1;

	cfg->use_post=0;

	cfg->debug=0;

    cfg->use_line=0;

    cfg->flushbfgap=1;

    cfg->specsum_fs=0;
    cfg->specsum_fe=8000;
    cfg->use_specsum_bl=0;
    cfg->specsum_bl=0.01;
    cfg->rate=16000;

	return 0;
}

int wtk_wbf_cfg_clean(wtk_wbf_cfg_t *cfg)
{
	wtk_stft2_cfg_clean(&(cfg->stft2));
	wtk_aspec_cfg_clean(&(cfg->aspec));
	wtk_bf_cfg_clean(&(cfg->bf));
	wtk_qmmse_cfg_clean(&(cfg->qmmse));

	return 0;
}

int wtk_wbf_cfg_update_local(wtk_wbf_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;
    wtk_local_cfg_t *m;
	int ret;

	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,specsum_fs,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,specsum_fe,v);

    wtk_local_cfg_update_cfg_b(lc,cfg,use_specsum_bl,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,specsum_bl,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,init_ncovnf,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,ncov_alpha,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,init_scovnf,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,scov_alpha,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_line,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_post,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,debug,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,lt,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,lf,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,range_interval,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,wbf_cnt,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,flushbfgap,v);

    m=lc;
	lc=wtk_local_cfg_find_lc_s(m,"stft2");
	if(lc)
	{
        ret=wtk_stft2_cfg_update_local(&(cfg->stft2),lc);
        if(ret!=0){goto end;}
    }

    lc=wtk_local_cfg_find_lc_s(m,"aspec");
	if(lc)
	{
        ret=wtk_aspec_cfg_update_local(&(cfg->aspec),lc);
        if(ret!=0){goto end;}
    }
	lc=wtk_local_cfg_find_lc_s(m,"bf");
	if(lc)
	{
        ret=wtk_bf_cfg_update_local(&(cfg->bf),lc);
        if(ret!=0){goto end;}
    }
    lc=wtk_local_cfg_find_lc_s(m,"qmmse");
	if(lc)
	{
        cfg->qmmse.step=cfg->stft2.win/2;
        ret=wtk_qmmse_cfg_update_local(&(cfg->qmmse),lc);
        if(ret!=0){goto end;}
    }

	ret=0;
end:
    return ret;
}

int wtk_wbf_cfg_update(wtk_wbf_cfg_t *cfg)
{
    int ret;

    ret=wtk_stft2_cfg_update(&(cfg->stft2));
    if(ret!=0){goto end;}
	ret=wtk_aspec_cfg_update(&(cfg->aspec));
    if(ret!=0){goto end;}
    ret=wtk_bf_cfg_update(&(cfg->bf));
    if(ret!=0){goto end;}
    ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
    if(ret!=0){goto end;}

    cfg->specsum_ns=floor(cfg->specsum_fs/(cfg->rate*1.0/cfg->stft2.win));
	cfg->specsum_ns=max(1, cfg->specsum_ns);
    cfg->specsum_ne=floor(cfg->specsum_fe/(cfg->rate*1.0/cfg->stft2.win));
	cfg->specsum_ne=min(cfg->stft2.win/2-1, cfg->specsum_ne);

	ret=0;
end:
	return ret;
}


int wtk_wbf_cfg_update2(wtk_wbf_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

    ret=wtk_stft2_cfg_update(&(cfg->stft2));
    if(ret!=0){goto end;}
	ret=wtk_aspec_cfg_update(&(cfg->aspec));
    if(ret!=0){goto end;}
    ret=wtk_bf_cfg_update(&(cfg->bf));
    if(ret!=0){goto end;}
    ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
    if(ret!=0){goto end;}

    cfg->specsum_ns=floor(cfg->specsum_fs/(cfg->rate*1.0/cfg->stft2.win));
	cfg->specsum_ns=max(1, cfg->specsum_ns);
    cfg->specsum_ne=floor(cfg->specsum_fe/(cfg->rate*1.0/cfg->stft2.win));
	cfg->specsum_ne=min(cfg->stft2.win/2-1, cfg->specsum_ne);

	ret=0;
end:
	return ret;
}

wtk_wbf_cfg_t* wtk_wbf_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_wbf_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_wbf_cfg,fn);
	if(!main_cfg)
	{
		return NULL;
	}
	cfg=(wtk_wbf_cfg_t*)main_cfg->cfg;
	cfg->main_cfg = main_cfg;
	return cfg;
}

void wtk_wbf_cfg_delete(wtk_wbf_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_wbf_cfg_t* wtk_wbf_cfg_new_bin(char *fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_wbf_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_wbf_cfg,fn,"./cfg");
	if(!mbin_cfg)
	{
		return NULL;
	}
	cfg=(wtk_wbf_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_wbf_cfg_delete_bin(wtk_wbf_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}