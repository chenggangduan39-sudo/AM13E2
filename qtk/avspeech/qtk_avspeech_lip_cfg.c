#include "qtk/avspeech/qtk_avspeech_lip_cfg.h"

int qtk_avspeech_lip_cfg_init(qtk_avspeech_lip_cfg_t *cfg) {
    qtk_cv_detection_cfg_init(&cfg->face_det);
    qtk_nnrt_cfg_init(&cfg->landmark);
    qtk_mot_sort_cfg_init(&cfg->mot);
    cfg->H = 88;
    cfg->W = 88;
    cfg->num_landmarks = 5;
    cfg->nskip = 1;
    cfg->use_landmark = 1;
    cfg->camera_angle = 98;
    cfg->selector_policy = QTK_AVSPEECH_LIP_SELECTOR_POLICY_ALL;
    cfg->policy_n = 1;

    cfg->switch_enter_trap = 13;
    cfg->theta_tolerance = 45;
    cfg->border=15;
    return 0;
}

int qtk_avspeech_lip_cfg_clean(qtk_avspeech_lip_cfg_t *cfg) {
    qtk_cv_detection_cfg_clean(&cfg->face_det);
    qtk_mot_sort_cfg_clean(&cfg->mot);
    qtk_nnrt_cfg_clean(&cfg->landmark);
    return 0;
}

int qtk_avspeech_lip_cfg_update(qtk_avspeech_lip_cfg_t *cfg) {
    qtk_cv_detection_cfg_update(&cfg->face_det);
    qtk_nnrt_cfg_update(&cfg->landmark);
    qtk_mot_sort_cfg_update(&cfg->mot);
    if (cfg->use_landmark == 0 && cfg->nskip != 1) {
        wtk_debug("skip not support for none-landmark config\n");
        return -1;
    }
    return 0;
}

int qtk_avspeech_lip_cfg_update_local(qtk_avspeech_lip_cfg_t *cfg,
                                      wtk_local_cfg_t *lc) {
    wtk_local_cfg_t *sub;
    wtk_string_t *v;
    sub = wtk_local_cfg_find_lc_s(lc, "face_det");
    if (sub) {
        qtk_cv_detection_cfg_update_local(&cfg->face_det, sub);
    }
    sub = wtk_local_cfg_find_lc_s(lc, "mot");
    if (sub) {
        qtk_mot_sort_cfg_update_local(&cfg->mot, sub);
    }
    sub = wtk_local_cfg_find_lc_s(lc, "landmark");
    if (sub) {
        qtk_nnrt_cfg_update_local(&cfg->landmark, sub);
    }
    wtk_local_cfg_update_cfg_i(lc, cfg, H, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, W, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, num_landmarks, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, nskip, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, border, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_landmark, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, camera_angle, v);
    v = wtk_local_cfg_find_string_s(lc, "selector_policy");
    if (v) {
        if (wtk_string_equal_s(v, "central")) {
            cfg->selector_policy = QTK_AVSPEECH_LIP_SELECTOR_POLICY_CENTRAL;
        } else if (wtk_string_equal_s(v, "all")) {
            cfg->selector_policy = QTK_AVSPEECH_LIP_SELECTOR_POLICY_ALL;
        } else if (wtk_string_equal_s(v, "at_most")) {
            cfg->selector_policy = QTK_AVSPEECH_LIP_SELECTOR_POLICY_AT_MOST;
        } else if (wtk_string_equal_s(v, "max")) {
            cfg->selector_policy = QTK_AVSPEECH_LIP_SELECTOR_POLICY_MAX;
        } else {
            wtk_debug("unknown policy %s\n", v->data);
            goto err;
        }
    }
    wtk_local_cfg_update_cfg_i(lc, cfg, policy_n, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, switch_enter_trap, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, theta_tolerance, v);
    return 0;
err:
    exit(0);
    return -1;
}

int qtk_avspeech_lip_cfg_update2(qtk_avspeech_lip_cfg_t *cfg,
                                 wtk_source_loader_t *sl) {
    qtk_cv_detection_cfg_update2(&cfg->face_det, sl);
    qtk_mot_sort_cfg_update2(&cfg->mot, sl);
    qtk_nnrt_cfg_update2(&cfg->landmark, sl);
    return 0;
}
