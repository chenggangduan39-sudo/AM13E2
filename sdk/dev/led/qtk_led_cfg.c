#include "qtk_led_cfg.h"


int qtk_led_cfg_init(qtk_led_cfg_t *cfg)
{
	cfg->brightness = 128;
	cfg->use_302 = 1;
	return 0;
}
int qtk_led_cfg_clean(qtk_led_cfg_t *cfg)
{
	return 0;
}
int qtk_led_cfg_update_local(qtk_led_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_i(main, cfg, brightness, v);
	wtk_local_cfg_update_cfg_b(main, cfg, use_302, v);
	return 0;
}
int qtk_led_cfg_update(qtk_led_cfg_t *cfg)
{
	return 0;
}
