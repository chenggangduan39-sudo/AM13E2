#include "qtk_cldebnf_cfg.h"

int qtk_cldebnf_cfg_init(qtk_cldebnf_cfg_t *cfg) {
    qtk_httpc_cfg_init(&cfg->httpc);
    wtk_string_set(&cfg->url_pre, 0, 0);
    cfg->interval = 500;
    return 0;
}

int qtk_cldebnf_cfg_clean(qtk_cldebnf_cfg_t *cfg) {
    qtk_httpc_cfg_clean(&cfg->httpc);
    return 0;
}

int qtk_cldebnf_cfg_update_local(qtk_cldebnf_cfg_t *cfg,
                                 wtk_local_cfg_t *main) {
    wtk_local_cfg_t *lc;
    wtk_string_t *v;

    lc = main;
    wtk_local_cfg_update_cfg_string_v(lc, cfg, url_pre, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, interval, v);

    lc = wtk_local_cfg_find_lc_s(main, "httpc");
    if (lc) {
        qtk_httpc_cfg_update_local(&cfg->httpc, lc);
    }
    return 0;
}

int qtk_cldebnf_cfg_update(qtk_cldebnf_cfg_t *cfg) {
    qtk_httpc_cfg_update(&cfg->httpc);
    return 0;
}
