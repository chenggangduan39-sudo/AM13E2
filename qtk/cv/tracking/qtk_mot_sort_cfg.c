#include "qtk/cv/tracking/qtk_mot_sort_cfg.h"

int qtk_mot_sort_cfg_init(qtk_mot_sort_cfg_t *cfg) {
    cfg->max_age = 10;
    cfg->min_hits = 1;
    cfg->use_distance = 1;
    cfg->iou_threshold = 0.3;
    cfg->distance_threshold = 150.0;
    cfg->delta_t = 1.0;
    cfg->speed_noise = 0.01;
    return 0;
}

int qtk_mot_sort_cfg_clean(qtk_mot_sort_cfg_t *cfg) { return 0; }

int qtk_mot_sort_cfg_update(qtk_mot_sort_cfg_t *cfg) { return 0; }

int qtk_mot_sort_cfg_update_local(qtk_mot_sort_cfg_t *cfg,
                                  wtk_local_cfg_t *lc) {
    wtk_string_t *v;
    wtk_local_cfg_update_cfg_b(lc, cfg, use_distance, v);
    if (cfg->use_distance) {
        wtk_local_cfg_update_cfg_f(lc, cfg, distance_threshold, v);
    } else {
        wtk_local_cfg_update_cfg_f(lc, cfg, iou_threshold, v);
    }
    wtk_local_cfg_update_cfg_f(lc, cfg, speed_noise, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, delta_t, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, max_age, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, min_hits, v);
    return 0;
}

int qtk_mot_sort_cfg_update2(qtk_mot_sort_cfg_t *cfg, wtk_source_loader_t *sl) {
    return 0;
}
