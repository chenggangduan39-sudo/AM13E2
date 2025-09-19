#ifndef __QTK_WIFI_CFG_H__
#define __QTK_WIFI_CFG_H__
#include "wtk/core/cfg/wtk_local_cfg.h"

typedef struct qtk_wifi_cfg{
	wtk_string_t ssid;
	wtk_string_t passwd;
}qtk_wifi_cfg_t;

int qtk_wifi_cfg_init(qtk_wifi_cfg_t *cfg);
int qtk_wifi_cfg_clean(qtk_wifi_cfg_t *cfg);
int qtk_wifi_cfg_update_local(qtk_wifi_cfg_t *cfg,wtk_local_cfg_t *main);
int qtk_wifi_cfg_update(qtk_wifi_cfg_t *cfg);

#endif
