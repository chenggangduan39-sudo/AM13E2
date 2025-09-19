#include "qtk_auin_cfg.h" 

int qtk_auin_cfg_init(qtk_auin_cfg_t *cfg)
{
	cfg->debug=0;
	cfg->err_exit = 0;
	return 0;
}

int qtk_auin_cfg_clean(qtk_auin_cfg_t *cfg)
{
	return 0;
}

int qtk_auin_cfg_update_local(qtk_auin_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_b(lc,cfg,debug,v);
	return 0;
}

int qtk_auin_cfg_update(qtk_auin_cfg_t *cfg)
{
	return 0;
}
