#include "wtk_maskssl2_cfg.h" 

int wtk_maskssl2_cfg_init(wtk_maskssl2_cfg_t *cfg)
{
	wtk_maskaspec_cfg_init(&(cfg->maskaspec));

	cfg->wins=1024;

	cfg->rate=16000;
	cfg->theta_step=1;
	cfg->phi_step=1;

	cfg->max_extp=3;
	cfg->min_thetasub=30;

	cfg->max_theta=359;
	cfg->min_theta=0;
	cfg->max_phi=0;
	cfg->min_phi=0;

	cfg->specsum_fs=0;
    cfg->specsum_fe=8000;
	cfg->specsum_thresh=0.0;
	cfg->specsum_thresh2=0.0;

	cfg->use_line=0;

	cfg->online_frame=31;
	cfg->online_frame_step=2;

	cfg->hook=NULL;

	cfg->smooth_theta=10;
	cfg->smooth_cnt=2;
	cfg->delay_cnt=2;
	cfg->use_smooth=0;
	cfg->use_sil_delay=0;
	cfg->use_each_comp=0;
	cfg->use_simple_phi=0;
	cfg->use_spec_k2=0;

	cfg->freq_skip=1;
	return 0;
}

int wtk_maskssl2_cfg_clean(wtk_maskssl2_cfg_t *cfg)
{
	wtk_maskaspec_cfg_clean(&(cfg->maskaspec));

	return 0;
}

int wtk_maskssl2_cfg_update_local(wtk_maskssl2_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;
	wtk_local_cfg_t *m;
	int ret;

	wtk_local_cfg_update_cfg_i(lc,cfg,wins,v);

    wtk_local_cfg_update_cfg_f(lc,cfg,specsum_fs,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,specsum_fe,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,specsum_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,specsum_thresh2,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,max_theta,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,min_theta,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,max_phi,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,min_phi,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,theta_step,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,phi_step,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,max_extp,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,min_thetasub,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_line,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,online_frame,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,online_frame_step,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,smooth_theta,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,smooth_cnt,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,delay_cnt,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_smooth,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_sil_delay,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_each_comp,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_simple_phi,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_spec_k2,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,freq_skip,v);

	m=lc;
	lc=wtk_local_cfg_find_lc_s(m,"maskaspec");
	if(lc)
	{
        ret=wtk_maskaspec_cfg_update_local(&(cfg->maskaspec),lc);
        if(ret!=0){goto end;}
    }

	ret=0;
end:
	return ret;
}

int wtk_maskssl2_cfg_update(wtk_maskssl2_cfg_t *cfg)
{
	int ret;

    ret=wtk_maskaspec_cfg_update(&(cfg->maskaspec));
    if(ret!=0){goto end;}

	cfg->specsum_ns=floor(cfg->specsum_fs/(cfg->rate*1.0/cfg->wins));
	cfg->specsum_ns=max(1, cfg->specsum_ns);
    cfg->specsum_ne=floor(cfg->specsum_fe/(cfg->rate*1.0/cfg->wins));
	cfg->specsum_ne=min(cfg->wins/2-1, cfg->specsum_ne);

	ret=0;
end:
	return ret;
}

int wtk_maskssl2_cfg_update2(wtk_maskssl2_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

    ret=wtk_maskaspec_cfg_update(&(cfg->maskaspec));
    if(ret!=0){goto end;}

	cfg->specsum_ns=floor(cfg->specsum_fs/(cfg->rate*1.0/cfg->wins));
	cfg->specsum_ns=max(1, cfg->specsum_ns);
    cfg->specsum_ne=floor(cfg->specsum_fe/(cfg->rate*1.0/cfg->wins));
	cfg->specsum_ne=min(cfg->wins/2-1, cfg->specsum_ne);

	ret=0;
end:
	return ret;
}