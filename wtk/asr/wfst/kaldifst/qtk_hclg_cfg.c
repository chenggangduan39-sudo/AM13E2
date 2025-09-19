#include "qtk_hclg_cfg.h"

int qtk_hclg_cfg_init(qtk_hclg_cfg_t *cfg)
{
	cfg->hclg_fst_fn=0;

	return 0;
}

int qtk_hclg_cfg_clean(qtk_hclg_cfg_t *cfg)
{
	return 0;
}

int qtk_hclg_cfg_update_local(qtk_hclg_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_str(lc,cfg,hclg_fst_fn,v);

	return 0;
}

int qtk_hclg_cfg_update(qtk_hclg_cfg_t *cfg)
{
	return 0;
}
