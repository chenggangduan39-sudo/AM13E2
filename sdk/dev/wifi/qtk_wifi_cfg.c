#include"qtk_wifi_cfg.h"


int qtk_wifi_cfg_init(qtk_wifi_cfg_t *cfg)
{
	wtk_string_set(&cfg->ssid, 0, 0);
	wtk_string_set(&cfg->passwd, 0, 0);
	return 0;
}

int qtk_wifi_cfg_clean(qtk_wifi_cfg_t *cfg)
{
	return 0;
}

int qtk_wifi_cfg_update_local(qtk_wifi_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_string_v(main, cfg, ssid, v);
	wtk_local_cfg_update_cfg_string_v(main, cfg, passwd, v);
	return 0;
}

int qtk_wifi_cfg_update(qtk_wifi_cfg_t *cfg)
{
	return 0;
}
