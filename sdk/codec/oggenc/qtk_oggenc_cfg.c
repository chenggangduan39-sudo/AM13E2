#include "qtk_oggenc_cfg.h"

int qtk_oggenc_cfg_init(qtk_oggenc_cfg_t *cfg) {
    cfg->spx_quality = 8;
    cfg->spx_complexity = 2;
    cfg->spx_vbr = 0;
    return 0;
}

int qtk_oggenc_cfg_clean(qtk_oggenc_cfg_t *cfg) { return 0; }

int qtk_oggenc_cfg_update_local(qtk_oggenc_cfg_t *cfg, wtk_local_cfg_t *lc) {
    wtk_string_t *v;

    wtk_local_cfg_update_cfg_i(lc, cfg, spx_quality, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, spx_complexity, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, spx_vbr, v);
    return 0;
}

int qtk_oggenc_cfg_update(qtk_oggenc_cfg_t *cfg) { return 0; }
