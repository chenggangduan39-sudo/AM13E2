#ifndef __QTK_KEY_CFG_H__
#define __QTK_KEY_CFG_H__
#include "wtk/core/cfg/wtk_local_cfg.h"

typedef struct qtk_key_cfg{
	wtk_string_t event_node;
}qtk_key_cfg_t;

int qtk_key_cfg_init(qtk_key_cfg_t *cfg);
int qtk_key_cfg_clean(qtk_key_cfg_t *cfg);
int qtk_key_cfg_update_local(qtk_key_cfg_t *cfg,wtk_local_cfg_t *main);
int qtk_key_cfg_update(qtk_key_cfg_t *cfg);

#endif

