#include "wtk_covm2_cfg.h" 

int wtk_covm2_cfg_init(wtk_covm2_cfg_t *cfg)
{
	cfg->cov_hist=10;
    cfg->eye=0;

    return 0;
}

int wtk_covm2_cfg_clean(wtk_covm2_cfg_t *cfg)
{
	return 0;
}

int wtk_covm2_cfg_update_local(wtk_covm2_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    wtk_string_t *v;

    wtk_local_cfg_update_cfg_i(lc,cfg,cov_hist,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,eye,v);

    return 0;
}

int wtk_covm2_cfg_update(wtk_covm2_cfg_t *cfg)
{
    return 0;
}

int wtk_covm2_cfg_update2(wtk_covm2_cfg_t *cfg,wtk_source_loader_t *sl)
{
	return wtk_covm2_cfg_update(cfg);
}