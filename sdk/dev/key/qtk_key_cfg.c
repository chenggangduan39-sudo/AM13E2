#include "qtk_key_cfg.h"

int qtk_key_cfg_init(qtk_key_cfg_t *cfg)
{
	wtk_string_set_s(&cfg->event_node, "/dev/input/event2");
	return 0;
}

int qtk_key_cfg_clean(qtk_key_cfg_t *cfg)
{

	return 0;
}

int qtk_key_cfg_update_local(qtk_key_cfg_t *cfg, wtk_local_cfg_t *main)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_string_v(main,cfg,event_node,v);
	return 0;
}

int qtk_key_cfg_update(qtk_key_cfg_t *cfg)
{

	return 0;
}




