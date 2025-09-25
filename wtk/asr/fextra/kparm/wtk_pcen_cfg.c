#include "wtk_pcen_cfg.h"

int wtk_pcen_cfg_clean(wtk_pcen_cfg_t *cfg)
{
	return 0;
}

int wtk_pcen_cfg_update(wtk_pcen_cfg_t *cfg)
{
	return 0;
}

int wtk_pcen_cfg_init(wtk_pcen_cfg_t *cfg)
{
	cfg->alpha=0.98;
	cfg->smooth_factor=0.025;
	cfg->epsilon=0.000001;
	cfg->gamma=0.5;
	cfg->delta=2.0;
	cfg->use_torch = 0;
	return 0;
}

int wtk_pcen_cfg_update_local(wtk_pcen_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_f(lc,cfg,alpha,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,smooth_factor,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,epsilon,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,gamma,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,delta,v);

    wtk_local_cfg_update_cfg_b(lc,cfg,use_torch,v);

	return 0;
}
