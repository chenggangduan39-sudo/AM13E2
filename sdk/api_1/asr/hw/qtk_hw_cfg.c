#include "qtk_hw_cfg.h"

int qtk_hw_cfg_init(qtk_hw_cfg_t *cfg) {
    qtk_httpc_cfg_init(&cfg->httpc_cfg);
    return 0;
}

int qtk_hw_cfg_clean(qtk_hw_cfg_t *cfg) {
    qtk_httpc_cfg_clean(&cfg->httpc_cfg);
    return 0;
}

int qtk_hw_cfg_update_local(qtk_hw_cfg_t *cfg, wtk_local_cfg_t *main) {
    wtk_local_cfg_t *lc;

    lc = main;
    lc = wtk_local_cfg_find_lc_s(main, "httpc");
    if (lc) {
        qtk_httpc_cfg_update_local(&cfg->httpc_cfg, lc);
    }

    return 0;
}

int qtk_hw_cfg_update(qtk_hw_cfg_t *cfg) {
    qtk_httpc_cfg_update(&cfg->httpc_cfg);
    return 0;
}