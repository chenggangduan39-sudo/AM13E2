#include "wtk_covm_cfg.h" 

int wtk_covm_cfg_init(wtk_covm_cfg_t *cfg)
{
    cfg->ncov_alpha=0.1;
    cfg->init_ncovnf=100;

    cfg->scov_alpha=0.1;
    cfg->init_scovnf=100;

	cfg->use_covhist=1;
	cfg->scov_hist=1;
    cfg->ncov_hist=1;

    cfg->ncov_flush_delay=0;

    cfg->use_scov=0;

    return 0;
}

int wtk_covm_cfg_clean(wtk_covm_cfg_t *cfg)
{
	return 0;
}

int wtk_covm_cfg_update_local(wtk_covm_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    wtk_string_t *v;

    wtk_local_cfg_update_cfg_b(lc,cfg,use_scov,v);

    wtk_local_cfg_update_cfg_b(lc,cfg,use_covhist,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,scov_hist,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,ncov_hist,v);

    wtk_local_cfg_update_cfg_i(lc,cfg,ncov_flush_delay,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,init_ncovnf,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,ncov_alpha,v);

    wtk_local_cfg_update_cfg_i(lc,cfg,init_scovnf,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,scov_alpha,v);

    return 0;
}

int wtk_covm_cfg_update(wtk_covm_cfg_t *cfg)
{
    return 0;
}

int wtk_covm_cfg_update2(wtk_covm_cfg_t *cfg,wtk_source_loader_t *sl)
{
	return wtk_covm_cfg_update(cfg);
}