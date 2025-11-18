#include "qtk/tracy/qtk_tracy_cfg.h"
#include "wtk/core/cfg/wtk_local_cfg.h"

int qtk_tracy_cfg_init(qtk_tracy_cfg_t *cfg) {
    cfg->host = "127.0.0.1";
    cfg->port = 8888;
    return 0;
}

int qtk_tracy_cfg_clean(qtk_tracy_cfg_t *cfg) {
    return 0;
}

int qtk_tracy_cfg_update(qtk_tracy_cfg_t *cfg) {
    return 0;
}

int qtk_tracy_cfg_update_local(qtk_tracy_cfg_t *cfg, wtk_local_cfg_t *lc) {
    wtk_string_t *v;

    wtk_local_cfg_update_cfg_str(lc, cfg, host, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, port, v);
    return 0;
}