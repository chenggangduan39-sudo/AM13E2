#include "qtk/ult/qtk_ult_track_cfg.h"
#include "qtk/mdl/qtk_sonicnet_cfg.h"
#include "qtk/ult/qtk_ult_msc2d_cfg.h"
#include "qtk/ult/qtk_ult_ofdm_cfg.h"
#include "qtk/ult/qtk_ult_perception_cfg.h"
#include "wtk/core/cfg/wtk_local_cfg.h"

int qtk_ult_track_cfg_init(qtk_ult_track_cfg_t *cfg) {
    qtk_ult_ofdm_cfg_init(&cfg->ofdm);
    qtk_ultm2_cfg_init(&cfg->ultm2);
    qtk_ult_perception_cfg_init(&cfg->perception);
    qtk_sonicnet_cfg_init(&cfg->sonicnet);
    qtk_mot_sort_cfg_init(&cfg->sort);
    qtk_ult_msc2d_cfg_init(&cfg->msc2d);
    qtk_ult_msc2d_cfg_init(&cfg->y_msc2d);
    qtk_ult_fmcw_cfg_init(&cfg->fmcw);
    cfg->channel = 1;
    cfg->upsamp = 8;
    cfg->range_prob_alpha = 0.5;
    cfg->aptd_dynamic_alpha = 0.7;
    cfg->left_ctx = 1;
    cfg->right_ctx = 1;
    cfg->nn_prob = 0.3;
    cfg->max_tgt = 1;
    cfg->nn_vad_thresh = 0.5;
    cfg->use_latest_cir = 1;
    cfg->gamma = 2.0;
    cfg->range_search_width = 3;
    cfg->angle_search_width = 2;
    cfg->act_cir_amp_e = 2;
    cfg->hist_alpha = 0.8;
    cfg->inactive_halt = 1;
    cfg->max_tracklet = 1;
    cfg->startup_warmup = 0;
    cfg->warmup_cir_thresh = 0.01;
    cfg->height_search_width = 5;
    cfg->use_fmcw = 0;
    cfg->use_perception = 0;
    return 0;
}

int qtk_ult_track_cfg_clean(qtk_ult_track_cfg_t *cfg) {
    qtk_ult_ofdm_cfg_clean(&cfg->ofdm);
    qtk_ultm2_cfg_clean(&cfg->ultm2);
    qtk_ult_perception_cfg_clean(&cfg->perception);
    qtk_sonicnet_cfg_clean(&cfg->sonicnet);
    qtk_ult_msc2d_cfg_clean(&cfg->msc2d);
    qtk_ult_msc2d_cfg_clean(&cfg->y_msc2d);
    qtk_ult_fmcw_cfg_clean(&cfg->fmcw);
    return 0;
}

int qtk_ult_track_cfg_update(qtk_ult_track_cfg_t *cfg) {
    qtk_ult_ofdm_cfg_update(&cfg->ofdm);
    qtk_ultm2_cfg_update(&cfg->ultm2);
    qtk_sonicnet_cfg_update(&cfg->sonicnet);
    qtk_ult_perception_cfg_update(&cfg->perception);
    qtk_ult_msc2d_cfg_update(&cfg->msc2d);
    if (cfg->with_height) {
        qtk_ult_msc2d_cfg_update(&cfg->y_msc2d);
    }
    if (cfg->use_fmcw) {
        qtk_ult_fmcw_cfg_update(&cfg->fmcw);
    }
    return 0;
}

int qtk_ult_track_cfg_update2(qtk_ult_track_cfg_t *cfg, wtk_source_loader_t *loader) {
    return qtk_ult_track_cfg_update(cfg);
}

int qtk_ult_track_cfg_update_local(qtk_ult_track_cfg_t *cfg,
                                   wtk_local_cfg_t *main) {
    wtk_local_cfg_t *lc;
    wtk_string_t *v;
    wtk_local_cfg_update_cfg_i(main, cfg, channel, v);
    wtk_local_cfg_update_cfg_i(main, cfg, upsamp, v);
    wtk_local_cfg_update_cfg_i(main, cfg, left_ctx, v);
    wtk_local_cfg_update_cfg_i(main, cfg, right_ctx, v);
    wtk_local_cfg_update_cfg_i(main, cfg, max_tgt, v);
    wtk_local_cfg_update_cfg_i(main, cfg, max_tracklet, v);
    wtk_local_cfg_update_cfg_f(main, cfg, nn_prob, v);
    wtk_local_cfg_update_cfg_f(main, cfg, nn_vad_thresh, v);
    wtk_local_cfg_update_cfg_f(main, cfg, range_prob_alpha, v);
    wtk_local_cfg_update_cfg_f(main, cfg, aptd_dynamic_alpha, v);
    wtk_local_cfg_update_cfg_f(main, cfg, gamma, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_latest_cir, v);
    wtk_local_cfg_update_cfg_b(main, cfg, inactive_halt, v);
    wtk_local_cfg_update_cfg_i(main, cfg, range_search_width, v);
    wtk_local_cfg_update_cfg_i(main, cfg, angle_search_width, v);
    wtk_local_cfg_update_cfg_f(main, cfg, act_cir_amp_e, v);
    wtk_local_cfg_update_cfg_f(main, cfg, hist_alpha, v);
    wtk_local_cfg_update_cfg_b(main, cfg, startup_warmup, v);
    wtk_local_cfg_update_cfg_b(main, cfg, with_height, v);
    wtk_local_cfg_update_cfg_f(main, cfg, warmup_cir_thresh, v);
    wtk_local_cfg_update_cfg_i(main, cfg, height_search_width, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_fmcw, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_perception, v);
    lc = wtk_local_cfg_find_lc_s(main, "ofdm");
    if (lc) {
        qtk_ult_ofdm_cfg_update_local(&cfg->ofdm, lc);
    }
    lc = wtk_local_cfg_find_lc_s(main, "ultm2");
    if (lc) {
        qtk_ultm2_cfg_update_local(&cfg->ultm2, lc);
    }
    lc = wtk_local_cfg_find_lc_s(main, "perception");
    if (lc) {
        qtk_ult_perception_cfg_update_local(&cfg->perception, lc);
    }
    lc = wtk_local_cfg_find_lc_s(main, "sonicnet");
    if (lc) {
        qtk_sonicnet_cfg_update_local(&cfg->sonicnet, lc);
    }
    lc = wtk_local_cfg_find_lc_s(main, "msc2d");
    if (lc) {
        qtk_ult_msc2d_cfg_update_local(&cfg->msc2d, lc);
    }
    lc = wtk_local_cfg_find_lc_s(main, "sort");
    if (lc) {
        wtk_local_cfg_update_cfg_i(lc, (&cfg->sort), max_age, v);
        wtk_local_cfg_update_cfg_i(lc, (&cfg->sort), min_hits, v);
        wtk_local_cfg_update_cfg_b(lc, (&cfg->sort), use_distance, v);
        wtk_local_cfg_update_cfg_f(lc, (&cfg->sort), delta_t, v);
        v = wtk_local_cfg_find_string_s(lc, "threshold");
        if (v) {
            if (cfg->sort.use_distance) {
                cfg->sort.distance_threshold = atof(v->data);
            } else {
                cfg->sort.iou_threshold = atof(v->data);
            }
        }
    }
    if (cfg->with_height) {
        lc = wtk_local_cfg_find_lc_s(main, "y_msc2d");
        if (lc) {
            qtk_ult_msc2d_cfg_update_local(&cfg->y_msc2d, lc);
        }
    }
    if (cfg->use_fmcw) {
        lc = wtk_local_cfg_find_lc_s(main, "fmcw");
        if (lc) {
            qtk_ult_fmcw_cfg_update_local(&cfg->fmcw, lc);
        }
    }
    return 0;
}
