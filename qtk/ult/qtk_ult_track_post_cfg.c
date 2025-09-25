#include "qtk/ult/qtk_ult_track_post_cfg.h"

int qtk_ult_track_post_cfg_init(qtk_ult_track_post_cfg_t *cfg) {
    cfg->win_sz = 100;
    cfg->win_step = 50;
    cfg->height_std_thresh = 0.1;
    return 0;
}

int qtk_ult_track_post_cfg_clean(qtk_ult_track_post_cfg_t *cfg) { return 0; }

int qtk_ult_track_post_cfg_update(qtk_ult_track_post_cfg_t *cfg) { return 0; }

int qtk_ult_track_post_cfg_update_local(qtk_ult_track_post_cfg_t *cfg,
                                        wtk_local_cfg_t *lc) {
    wtk_string_t *v;
    wtk_local_cfg_update_cfg_i(lc, cfg, win_sz, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, win_step, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, height_std_thresh, v);
    return 0;
}
