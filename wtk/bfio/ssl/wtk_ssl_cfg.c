#include "wtk_ssl_cfg.h" 

int wtk_ssl_cfg_init(wtk_ssl_cfg_t *cfg)
{
	wtk_stft2_cfg_init(&(cfg->stft2));
	wtk_aspec_cfg_init(&(cfg->aspec));

    cfg->theta_step=1;
	cfg->phi_step=1;

	cfg->max_extp=3;
	cfg->min_thetasub=30;

	cfg->specsum_fs=0;
    cfg->specsum_fe=8000;
	cfg->specsum_thresh=0.0;

	cfg->lf=6;
    cfg->lt=1;

	cfg->max_theta=359;
	cfg->max_phi=0;

	cfg->rate=16000;

	cfg->notify_time = 1000;
	cfg->min_energy = 5000;

	cfg->use_stft2=1;

	cfg->use_line=0;

	return 0;
}

int wtk_ssl_cfg_clean(wtk_ssl_cfg_t *cfg)
{
	wtk_stft2_cfg_clean(&(cfg->stft2));
	wtk_aspec_cfg_clean(&(cfg->aspec));
	return 0;
}

int wtk_ssl_cfg_update_local(wtk_ssl_cfg_t *cfg,wtk_local_cfg_t *lc)
{

	wtk_string_t *v;
	wtk_local_cfg_t *m;
	int ret;

	wtk_local_cfg_update_cfg_i(lc,cfg,max_theta,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,max_phi,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,theta_step,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,phi_step,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,max_extp,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,min_thetasub,v);

    wtk_local_cfg_update_cfg_f(lc,cfg,specsum_fs,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,specsum_fe,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,specsum_thresh,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,lt,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,lf,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_line,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);
	wtk_local_cfg_update_cfg_f(lc, cfg, notify_time, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, min_energy, v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_stft2,v);

	cfg->notify_len = cfg->notify_time * cfg->rate / 1000;
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

	ret=0;
end:
	return ret;
}

int wtk_ssl_cfg_update(wtk_ssl_cfg_t *cfg)
{
	int ret;

    ret=wtk_stft2_cfg_update(&(cfg->stft2));
    if(ret!=0){goto end;}
    ret=wtk_aspec_cfg_update(&(cfg->aspec));
    if(ret!=0){goto end;}

	cfg->specsum_ns=floor(cfg->specsum_fs/(cfg->rate*1.0/cfg->stft2.win));
	cfg->specsum_ns=max(1, cfg->specsum_ns);
    cfg->specsum_ne=floor(cfg->specsum_fe/(cfg->rate*1.0/cfg->stft2.win));
	cfg->specsum_ne=min(cfg->stft2.win/2-1, cfg->specsum_ne);

	if(cfg->use_line)
	{
		cfg->max_theta=180;
		cfg->max_phi=0;
	}else
	{
		cfg->max_theta=359;
	}

	ret=0;
end:
	return ret;
}

int wtk_ssl_cfg_update2(wtk_ssl_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

    ret=wtk_stft2_cfg_update(&(cfg->stft2));
    if(ret!=0){goto end;}
    ret=wtk_aspec_cfg_update(&(cfg->aspec));
    if(ret!=0){goto end;}

	cfg->specsum_ns=floor(cfg->specsum_fs/(cfg->rate*1.0/cfg->stft2.win));
	cfg->specsum_ns=max(1, cfg->specsum_ns);
    cfg->specsum_ne=floor(cfg->specsum_fe/(cfg->rate*1.0/cfg->stft2.win));
	cfg->specsum_ne=min(cfg->stft2.win/2-1, cfg->specsum_ne);

	if(cfg->use_line)
	{
		cfg->max_theta=180;
		cfg->max_phi=0;
	}else
	{
		cfg->max_theta=359;
	}

	ret=0;
end:
	return ret;
}

wtk_ssl_cfg_t* wtk_ssl_cfg_new(char *cfg_fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_ssl_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_ssl_cfg,cfg_fn);
	cfg=(wtk_ssl_cfg_t*)main_cfg->cfg;
	cfg->main_cfg=main_cfg;
	return cfg;
}

void wtk_ssl_cfg_delete(wtk_ssl_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_ssl_cfg_t* wtk_ssl_cfg_new_bin(char *bin_fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_ssl_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_ssl_cfg,bin_fn,"./cfg");
	cfg=(wtk_ssl_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_ssl_cfg_delete_bin(wtk_ssl_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
