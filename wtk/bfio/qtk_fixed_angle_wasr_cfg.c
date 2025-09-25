#include "wtk/bfio/qtk_fixed_angle_wasr_cfg.h"

int qtk_fixed_angle_wasr_cfg_init(qtk_fixed_angle_wasr_cfg_t *cfg) {
    if (qtk_wasr_cfg_init(&cfg->wasr) || wtk_qform2_cfg_init(&cfg->qform2) ||
        wtk_aec_cfg_init(&cfg->aec)) {
        goto err;
    };
    return 0;
err:
    return -1;
}

int qtk_fixed_angle_wasr_cfg_clean(qtk_fixed_angle_wasr_cfg_t *cfg) {
    if (qtk_wasr_cfg_clean(&cfg->wasr) || wtk_qform2_cfg_clean(&cfg->qform2) ||
        wtk_aec_cfg_clean(&cfg->aec)) {
        goto err;
    };
    return 0;
err:
    return -1;
}

int qtk_fixed_angle_wasr_cfg_update(qtk_fixed_angle_wasr_cfg_t *cfg) {
    if (qtk_wasr_cfg_update(&cfg->wasr) ||
        wtk_qform2_cfg_update(&cfg->qform2) || wtk_aec_cfg_update(&cfg->aec)) {
        goto err;
    };
    return 0;
err:
    return -1;
}

int qtk_fixed_angle_wasr_cfg_update_local(qtk_fixed_angle_wasr_cfg_t *cfg,
                                          wtk_local_cfg_t *m) {
    wtk_local_cfg_t *lc;
    wtk_string_t *v;

    lc = wtk_local_cfg_find_lc_s(m, "wasr");
    if (lc) {
        if (qtk_wasr_cfg_update_local(&cfg->wasr, lc)) {
            goto err;
        }
    }
    lc = wtk_local_cfg_find_lc_s(m, "qform2");
    if (lc) {
        if (wtk_qform2_cfg_update_local(&cfg->qform2, lc)) {
            goto err;
        }
    }
    lc = wtk_local_cfg_find_lc_s(m, "aec");
    if (lc) {
        if (wtk_aec_cfg_update_local(&cfg->aec, lc)) {
            goto err;
        }
    }

    wtk_local_cfg_update_cfg_f(m, cfg, theta, v);
    wtk_local_cfg_update_cfg_f(m, cfg, phi, v);
    wtk_local_cfg_update_cfg_b(m, cfg, use_aec, v);

    return 0;
err:
    return -1;
}
