#ifndef __QTK_HOTWORD_CFG_H__
#define __QTK_HOTWORD_CFG_H__

#include "sdk/httpc/qtk_httpc_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_hw_cfg qtk_hw_cfg_t;
struct qtk_hw_cfg {
    qtk_httpc_cfg_t httpc_cfg;
};

int qtk_hw_cfg_init(qtk_hw_cfg_t *cfg);
int qtk_hw_cfg_clean(qtk_hw_cfg_t *cfg);
int qtk_hw_cfg_update_local(qtk_hw_cfg_t *cfg, wtk_local_cfg_t *main);
int qtk_hw_cfg_update(qtk_hw_cfg_t *cfg);

#ifdef __cplusplus
};
#endif
#endif