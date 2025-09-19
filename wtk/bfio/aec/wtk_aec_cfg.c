#include "wtk_aec_cfg.h" 

int wtk_aec_cfg_init(wtk_aec_cfg_t *cfg)
{
	wtk_stft2_cfg_init(&(cfg->stft));
	wtk_nlms_cfg_init(&(cfg->nlms));
	wtk_rls_cfg_init(&(cfg->rls));
	wtk_qmmse_cfg_init(&(cfg->qmmse));

	cfg->use_aec_post=0;
	cfg->use_post=0;

	cfg->coh_alpha=0.93;

	cfg->leak_scale=1.0;

	cfg->use_nlms=1;
	cfg->use_rls=0;

	cfg->use_preemph=0;

	cfg->spchannel=1;

	cfg->spenr_thresh=100;
	cfg->spenr_cnt=10;
	
	return 0;
}

int wtk_aec_cfg_clean(wtk_aec_cfg_t *cfg)
{
	wtk_stft2_cfg_clean(&(cfg->stft));
	wtk_nlms_cfg_clean(&(cfg->nlms));
	wtk_rls_cfg_clean(&(cfg->rls));
	wtk_qmmse_cfg_clean(&(cfg->qmmse));

	return 0;
}

int wtk_aec_cfg_update_local(wtk_aec_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;

	lc=main;
	wtk_local_cfg_update_cfg_b(lc,cfg,use_preemph,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_aec_post,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_post,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_nlms,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_rls,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,coh_alpha,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,leak_scale,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,spchannel,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,spenr_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,spenr_cnt,v);
	
	lc=wtk_local_cfg_find_lc_s(main,"stft2");
	if(lc)
	{
		wtk_stft2_cfg_update_local(&(cfg->stft),lc);
	}
	lc=wtk_local_cfg_find_lc_s(main,"nlms");
	if(lc)
	{
		wtk_nlms_cfg_update_local(&(cfg->nlms),lc);
	}
	lc=wtk_local_cfg_find_lc_s(main,"rls");
	if(lc)
	{
		wtk_rls_cfg_update_local(&(cfg->rls),lc);
	}
	lc=wtk_local_cfg_find_lc_s(main,"qmmse");
	if(lc)
	{
		cfg->qmmse.step=cfg->stft.win/2;
		wtk_qmmse_cfg_update_local(&(cfg->qmmse),lc);
	}

	return 0;
}

int wtk_aec_cfg_update(wtk_aec_cfg_t *cfg)
{
	cfg->nlms.channel=cfg->stft.channel-cfg->spchannel;
	cfg->nlms.N=cfg->spchannel;
	cfg->rls.channel=cfg->stft.channel-cfg->spchannel;
	cfg->rls.N=cfg->spchannel;
	wtk_stft2_cfg_update(&(cfg->stft));
	wtk_nlms_cfg_update(&(cfg->nlms));
	wtk_rls_cfg_update(&(cfg->rls));
	wtk_qmmse_cfg_update(&(cfg->qmmse));

	return 0;
}

int wtk_aec_cfg_update2(wtk_aec_cfg_t *cfg,wtk_source_loader_t *sl)
{
	cfg->nlms.channel=cfg->stft.channel-cfg->spchannel;
	cfg->nlms.N=cfg->spchannel;
	cfg->rls.channel=cfg->stft.channel-cfg->spchannel;
	cfg->rls.N=cfg->spchannel;
	wtk_stft2_cfg_update(&(cfg->stft));
	wtk_nlms_cfg_update(&(cfg->nlms));
	wtk_rls_cfg_update(&(cfg->rls));
	wtk_qmmse_cfg_update(&(cfg->qmmse));

	return 0;
}
wtk_aec_cfg_t* wtk_aec_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_aec_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_aec_cfg,fn);
	if(!main_cfg)
	{
		return NULL;
	}
	cfg=(wtk_aec_cfg_t*)main_cfg->cfg;
	cfg->main_cfg = main_cfg;
	return cfg;
}

void wtk_aec_cfg_delete(wtk_aec_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_aec_cfg_t* wtk_aec_cfg_new_bin(char *fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_aec_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_aec_cfg,fn,"./cfg");
	if(!mbin_cfg)
	{
		return NULL;
	}
	cfg=(wtk_aec_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_aec_cfg_delete_bin(wtk_aec_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
