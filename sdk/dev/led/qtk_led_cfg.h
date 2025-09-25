#ifndef __QTK_LED_CFG_H__
#define __QTK_LED_CFG_H__
#include "wtk/core/cfg/wtk_local_cfg.h"

typedef struct qtk_led_cfg{
	int brightness;
	unsigned use_302:1;
}qtk_led_cfg_t;

int qtk_led_cfg_init(qtk_led_cfg_t *cfg);
int qtk_led_cfg_clean(qtk_led_cfg_t *cfg);
int qtk_led_cfg_update_local(qtk_led_cfg_t *cfg,wtk_local_cfg_t *main);
int qtk_led_cfg_update(qtk_led_cfg_t *cfg);

#endif
