#include "wtk_syn_dwin_cfg.h" 

int wtk_syn_dwin_cfg_init(wtk_syn_dwin_cfg_t *cfg)
{
	cfg->fn=NULL;
	return 0;
}

int wtk_syn_dwin_cfg_clean(wtk_syn_dwin_cfg_t *cfg)
{
	return 0;
}

int wtk_syn_dwin_cfg_update_local(wtk_syn_dwin_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	cfg->fn=wtk_local_cfg_find_array_s(lc,"fn");
	return 0;
}

int wtk_syn_dwin_cfg_update(wtk_syn_dwin_cfg_t *cfg)
{
	return 0;
}
