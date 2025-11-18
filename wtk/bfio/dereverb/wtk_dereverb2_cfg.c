#include "wtk_dereverb2_cfg.h" 

int wtk_dereverb2_cfg_init(wtk_dereverb2_cfg_t *cfg)
{
	wtk_stft2_cfg_init(&(cfg->stft));
	wtk_bf_cfg_init(&(cfg->bf));
	wtk_qmmse_cfg_init(&(cfg->qmmse));
	cfg->use_post=1;

	cfg->notch_radius=0;
	cfg->preemph=0;

	cfg->lambda=1;

	cfg->D=2;
	cfg->L=7;
	cfg->sigma=1e-10;
	cfg->p=0.75;

	cfg->wx_alpha=0.8;
	cfg->coh_alpha=0.93;
	cfg->leak_scale=1.0;

	cfg->rate=16000;

	cfg->theta=270;
	cfg->phi=0;

	cfg->main_cfg=NULL;
	cfg->mbin_cfg=NULL;
	
	return 0;
}

int wtk_dereverb2_cfg_clean(wtk_dereverb2_cfg_t *cfg)
{
	wtk_stft2_cfg_clean(&(cfg->stft));
	wtk_bf_cfg_clean(&(cfg->bf));
	wtk_qmmse_cfg_clean(&(cfg->qmmse));

	return 0;
}

int wtk_dereverb2_cfg_update_local(wtk_dereverb2_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;

	lc=main;

	wtk_local_cfg_update_cfg_f(lc,cfg,lambda,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,wx_alpha,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,coh_alpha,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_post,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,leak_scale,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,notch_radius,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,preemph,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,D,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,L,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,sigma,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,p,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,theta,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,phi,v);

	lc=wtk_local_cfg_find_lc_s(main,"stft2");
	if(lc)
	{
		wtk_stft2_cfg_update_local(&(cfg->stft),lc);
	}
	lc=wtk_local_cfg_find_lc_s(main,"bf");
	if(lc)
	{
		wtk_bf_cfg_update_local(&(cfg->bf),lc);
	}
	lc=wtk_local_cfg_find_lc_s(main,"qmmse");
	if(lc)
	{
		cfg->qmmse.step=cfg->stft.win/2;
		wtk_qmmse_cfg_update_local(&(cfg->qmmse),lc);
	}

	return 0;
}

int wtk_dereverb2_cfg_update(wtk_dereverb2_cfg_t *cfg)
{
	wtk_stft2_cfg_update(&(cfg->stft));
	wtk_bf_cfg_update(&(cfg->bf));
	wtk_qmmse_cfg_update(&(cfg->qmmse));
	cfg->notch_radius_den=cfg->notch_radius*cfg->notch_radius+0.7*(1-cfg->notch_radius)*(1-cfg->notch_radius);

	return 0;
}


int wtk_dereverb2_cfg_update2(wtk_dereverb2_cfg_t *cfg,wtk_source_loader_t *sl)
{
	return wtk_dereverb2_cfg_update(cfg);
}

wtk_dereverb2_cfg_t* wtk_dereverb2_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_dereverb2_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_dereverb2_cfg,fn);
	if(!main_cfg)
	{
		return NULL;
	}
	cfg=(wtk_dereverb2_cfg_t*)main_cfg->cfg;
	cfg->main_cfg = main_cfg;
	return cfg;
}

void wtk_dereverb2_cfg_delete(wtk_dereverb2_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_dereverb2_cfg_t* wtk_dereverb2_cfg_new_bin(char *fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_dereverb2_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_dereverb2_cfg,fn,"./cfg");
	if(!mbin_cfg)
	{
		return NULL;
	}
	cfg=(wtk_dereverb2_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_dereverb2_cfg_delete_bin(wtk_dereverb2_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}


