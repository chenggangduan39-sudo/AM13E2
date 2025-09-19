#include "wtk_qstft_cfg.h" 

int wtk_qstft_cfg_init(wtk_qstft_cfg_t *cfg)
{
	cfg->lt=1;
	cfg->lf=7;
	cfg->fs=0;
	cfg->fe=8000;
	cfg->step=3;
	cfg->use_sub=0;
	return 0;
}

int wtk_qstft_cfg_clean(wtk_qstft_cfg_t *cfg)
{
	return 0;
}

int wtk_qstft_cfg_update_local(wtk_qstft_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_i(lc,cfg,fs,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,fe,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,step,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_sub,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,lt,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,lf,v);
	return 0;
}

int wtk_qstft_cfg_update(wtk_qstft_cfg_t *cfg)
{
	return 0;
}
