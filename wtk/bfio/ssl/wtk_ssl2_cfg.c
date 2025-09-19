#include "wtk_ssl2_cfg.h" 

int wtk_ssl2_cfg_init(wtk_ssl2_cfg_t *cfg)
{
	wtk_aspec_cfg_init(&(cfg->aspec));

	cfg->wins=320;

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

	cfg->use_line=0;

	cfg->use_online=0;
	cfg->online_tms=500;
	cfg->online_frame=0;

	return 0;
}

int wtk_ssl2_cfg_clean(wtk_ssl2_cfg_t *cfg)
{
	wtk_aspec_cfg_clean(&(cfg->aspec));
	return 0;
}

int wtk_ssl2_cfg_update_local(wtk_ssl2_cfg_t *cfg,wtk_local_cfg_t *lc)
{

	wtk_string_t *v;
	wtk_local_cfg_t *m;
	int ret;

	wtk_local_cfg_update_cfg_i(lc,cfg,wins,v);

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

	wtk_local_cfg_update_cfg_b(lc,cfg,use_online,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,online_tms,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);
	m=lc;
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

int wtk_ssl2_cfg_update(wtk_ssl2_cfg_t *cfg)
{
	int ret;

    ret=wtk_aspec_cfg_update(&(cfg->aspec));
    if(ret!=0){goto end;}

	cfg->specsum_ns=floor(cfg->specsum_fs/(cfg->rate*1.0/cfg->wins));
	cfg->specsum_ns=max(1, cfg->specsum_ns);
    cfg->specsum_ne=floor(cfg->specsum_fe/(cfg->rate*1.0/cfg->wins));
	cfg->specsum_ne=min(cfg->wins/2-1, cfg->specsum_ne);
	
	cfg->online_frame=floor(cfg->online_tms*1.0/1000*cfg->rate/(cfg->wins/2));

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
