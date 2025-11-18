#include "qtk_etinybf_cfg.h"
#include "qtk/core/qtk_timer.h"
#include "wtk/bfio/qform/wtk_qform9_cfg.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"

int qtk_etinybf_cfg_init(qtk_etinybf_cfg_t *cfg) {
    cfg->use_qform9 = 0;
    cfg->phi = 0;
    cfg->theta = 0;
    cfg->main_cfg = NULL;
    cfg->input_channel = 4;
    wtk_qform9_cfg_init(&cfg->qform9);
    cfg->use_vboxebf = 0;
    wtk_gainnet_bf_stereo_cfg_init(&cfg->vboxebf);

    return 0;
}

int qtk_etinybf_cfg_clean(qtk_etinybf_cfg_t *cfg) {
    wtk_qform9_cfg_clean(&cfg->qform9);
    wtk_gainnet_bf_stereo_cfg_clean(&cfg->vboxebf);

    return 0;
}

int qtk_etinybf_cfg_update_local(qtk_etinybf_cfg_t *cfg,
                                 wtk_local_cfg_t *main) {
    wtk_local_cfg_t *lc;
    wtk_string_t *v;

    wtk_local_cfg_update_cfg_b(main, cfg, use_qform9, v);
    wtk_local_cfg_update_cfg_f(main, cfg, theta, v);
    wtk_local_cfg_update_cfg_f(main, cfg, phi, v);
    wtk_local_cfg_update_cfg_i(main, cfg, input_channel, v);

    wtk_local_cfg_update_cfg_b(main, cfg, use_vboxebf, v);

    if (cfg->use_qform9) {
        lc = wtk_local_cfg_find_lc_s(main, "qform9");
        if (lc) {
            if (wtk_qform9_cfg_update_local(&cfg->qform9, lc)) {
                goto err;
            }
        }
    }
    if (cfg->use_vboxebf) {
        lc = wtk_local_cfg_find_lc_s(main, "vboxebf");
        if (lc) {
            if (wtk_gainnet_bf_stereo_cfg_update_local(&cfg->vboxebf, lc)) {
                return -1;
            }
        }
    }

    return 0;
err:
    return -1;
}

int qtk_etinybf_cfg_update(qtk_etinybf_cfg_t *cfg) {
    if (cfg->use_qform9) {
        wtk_qform9_cfg_update(&cfg->qform9);
    } else if (cfg->use_vboxebf) {
        wtk_gainnet_bf_stereo_cfg_update(&cfg->vboxebf);
    }

    return 0;
}

int qtk_etinybf_cfg_update2(qtk_etinybf_cfg_t *cfg, wtk_source_loader_t *sl) {
    if (cfg->use_qform9) {
        wtk_qform9_cfg_update2(&cfg->qform9, sl);
    } else if (cfg->use_vboxebf) {
        wtk_gainnet_bf_stereo_cfg_update2(&cfg->vboxebf, sl);
    }

    return 0;
}

qtk_etinybf_cfg_t *qtk_etinybf_cfg_new(const char *cfn) {
    qtk_etinybf_cfg_t *cfg;
    wtk_main_cfg_t *main_cfg;

    main_cfg = wtk_main_cfg_new_type(qtk_etinybf_cfg, cast(char *, cfn));
    cfg = cast(qtk_etinybf_cfg_t *, main_cfg->cfg);
    cfg->main_cfg = main_cfg;

    return cfg;
}

qtk_etinybf_cfg_t *qtk_etinybf_cfg_new_bin(const char *cfn) {
    qtk_etinybf_cfg_t *cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    mbin_cfg =
        wtk_mbin_cfg_new_type(qtk_etinybf_cfg, cast(char *, cfn), "./main.cfg");
    cfg = cast(qtk_etinybf_cfg_t *, mbin_cfg->cfg);
    cfg->mbin_cfg = mbin_cfg;

    return cfg;
}

void qtk_etinybf_cfg_delete(qtk_etinybf_cfg_t *cfg) {
    wtk_main_cfg_delete(cfg->main_cfg);
}

void qtk_etinybf_cfg_delete_bin(qtk_etinybf_cfg_t *cfg) {
    wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
