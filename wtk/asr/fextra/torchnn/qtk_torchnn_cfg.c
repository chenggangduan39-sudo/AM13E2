#include "qtk_torchnn_cfg.h"

int qtk_torchnn_cfg_init(qtk_torchnn_cfg_t *cfg)
{
	cfg->torchnn_fn = NULL;
	return 0;
}
int qtk_torchnn_cfg_clean(qtk_torchnn_cfg_t *cfg)
{
	return 0;
}
int qtk_torchnn_cfg_update_local(qtk_torchnn_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_str(lc, cfg, torchnn_fn, v);

	return 0;
}
int qtk_torchnn_cfg_update(qtk_torchnn_cfg_t *cfg)
{
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;
	return qtk_torchnn_cfg_update2(cfg,&sl);
}

int qtk_torchnn_cfg_update2(qtk_torchnn_cfg_t *cfg,wtk_source_loader_t *sl)
{
	return 0;
}
