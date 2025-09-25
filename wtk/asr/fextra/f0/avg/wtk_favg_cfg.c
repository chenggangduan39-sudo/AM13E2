#include "wtk_favg_cfg.h"


int wtk_favg_cfg_init(wtk_favg_cfg_t *cfg)
{
	cfg->win=2;
	cfg->avg_prior=191.7574;
	cfg->norm_avg_prior=-10.6;
	cfg->norm_var_prior=26.18;
	cfg->alpha=0.8;
	return 0;
}

int wtk_favg_cfg_clean(wtk_favg_cfg_t *cfg)
{
	return 0;
}

int wtk_favg_cfg_update_local(wtk_favg_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_i(lc,cfg,win,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,avg_prior,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,norm_avg_prior,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,norm_var_prior,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,alpha,v);
	return 0;
}

int wtk_favg_cfg_update(wtk_favg_cfg_t *cfg)
{
	return 0;
}
