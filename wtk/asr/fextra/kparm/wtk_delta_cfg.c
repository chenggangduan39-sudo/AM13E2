#include "wtk_delta_cfg.h" 

int wtk_delta_cfg_init(wtk_delta_cfg_t *cfg)
{
	cfg->order=2;
	cfg->win=2;
	return 0;
}

int wtk_delta_cfg_clean(wtk_delta_cfg_t *cfg)
{
	return 0;
}

int wtk_delta_cfg_update_local(wtk_delta_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_i(lc,cfg,order,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,win,v);
	return 0;
}

int wtk_delta_cfg_update(wtk_delta_cfg_t *cfg)
{
	return 0;
}
