#include "wtk_admm_cfg.h" 

int wtk_admm_cfg_init(wtk_admm_cfg_t *cfg)
{
	wtk_stft2_cfg_init(&(cfg->stft));
	wtk_rls_cfg_init(&(cfg->rls));
	cfg->D=2;
	cfg->L=7;
	
	return 0;
}

int wtk_admm_cfg_clean(wtk_admm_cfg_t *cfg)
{
	wtk_stft2_cfg_clean(&(cfg->stft));
	wtk_rls_cfg_clean(&(cfg->rls));

	return 0;
}

int wtk_admm_cfg_update_local(wtk_admm_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;

	lc=main;
	wtk_local_cfg_update_cfg_i(lc,cfg,D,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,L,v);
	lc=wtk_local_cfg_find_lc_s(main,"stft2");
	if(lc)
	{
		wtk_stft2_cfg_update_local(&(cfg->stft),lc);
	}
	lc=wtk_local_cfg_find_lc_s(main,"rls");
	if(lc)
	{
		cfg->rls.step=cfg->stft.win*(1-cfg->stft.overlap);
		wtk_rls_cfg_update_local(&(cfg->rls),lc);
		cfg->rls.N=cfg->stft.channel;
	}

	return 0;
}

int wtk_admm_cfg_update(wtk_admm_cfg_t *cfg)
{
	cfg->rls.channel=cfg->stft.channel;
	cfg->rls.N=cfg->stft.channel;
	cfg->rls.use_wx=0;
	cfg->rls.use_admm=1;
	wtk_stft2_cfg_update(&(cfg->stft));
	wtk_rls_cfg_update(&(cfg->rls));

	return 0;
}
