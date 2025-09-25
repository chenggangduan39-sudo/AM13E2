#include "qtk_isemdlg_cfg.h"

int qtk_isemdlg_cfg_init(qtk_isemdlg_cfg_t *cfg) {
    qtk_spx_cfg_init(&cfg->spx);
    wtk_string_set(&cfg->semdlg_fn, 0, 0);
    wtk_string_set(&cfg->name, 0, 0);
    cfg->semdlg = NULL;
    cfg->attach = NULL;
    cfg->use_bin = 0;
    cfg->use_spx = 0;
    cfg->lc_custom = 0;
    return 0;
}

int qtk_isemdlg_cfg_clean(qtk_isemdlg_cfg_t *cfg) {
    if (cfg->use_spx) {
        qtk_spx_cfg_clean(&cfg->spx);
    } else if (cfg->semdlg) {
        cfg->use_bin ? wtk_semdlg_cfg_delete_bin2(cfg->semdlg)
                     : wtk_semdlg_cfg_delete(cfg->semdlg);
    }
    return 0;
}

int qtk_isemdlg_cfg_update_local(qtk_isemdlg_cfg_t *cfg,
                                 wtk_local_cfg_t *main) {
    wtk_local_cfg_t *lc;
    wtk_string_t *v;
    int ret;

    lc = main;
    wtk_local_cfg_update_cfg_string_v(lc, cfg, semdlg_fn, v);
    wtk_local_cfg_update_cfg_string_v(lc, cfg, name, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_bin, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_spx, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, lc_custom, v);

    if (cfg->use_spx) {
        lc = wtk_local_cfg_find_lc_s(main, "spx");
        if (lc) {
            qtk_spx_cfg_update_local(&cfg->spx, lc);
        }
    } else if (cfg->semdlg_fn.len <= 0) {
        cfg->attach = wtk_local_cfg_find_lc_s(main, "attach");
        ret = -1;
        goto end;
    }

    ret = 0;
end:
    return ret;
}

int qtk_isemdlg_cfg_update(qtk_isemdlg_cfg_t *cfg) {
    int ret;

    if (cfg->use_spx) {
        ret = qtk_spx_cfg_update(&cfg->spx);
        if (ret != 0) {
            goto end;
        }
    } else if (cfg->semdlg_fn.len > 0) {
        cfg->semdlg =
            cfg->use_bin
                ? wtk_semdlg_cfg_new_bin2(cfg->semdlg_fn.data, 0, cfg->attach)
                : wtk_semdlg_cfg_new(cfg->semdlg_fn.data);
        if (!cfg->semdlg) {
            ret = -1;
            goto end;
        }
    } else {
        ret = -1;
        goto end;
    }

    ret = 0;
end:
    return ret;
}

int qtk_isemdlg_cfg_update_semdlg(qtk_isemdlg_cfg_t *cfg, wtk_source_t *s) {
    int ret = 0;
    wtk_rbin2_item_t *item;

    item = (wtk_rbin2_item_t *)s->data;
    cfg->semdlg = wtk_semdlg_cfg_new_bin2(item->rb->fn, item->pos, cfg->attach);
    if (!cfg->semdlg) {
        ret = -1;
    }

    return ret;
}

int qtk_isemdlg_cfg_update2(qtk_isemdlg_cfg_t *cfg, wtk_source_loader_t *sl) {
    int ret;

    if (cfg->use_spx) {
        ret = qtk_spx_cfg_update(&cfg->spx);
        if (ret != 0) {
            goto end;
        }
    } else if (cfg->semdlg_fn.len >= 0) {
        ret = wtk_source_loader_load(
            sl, cfg, (wtk_source_load_handler_t)qtk_isemdlg_cfg_update_semdlg,
            cfg->semdlg_fn.data);
        if (ret != 0) {
            goto end;
        }
    } else {
        ret = -1;
        goto end;
    }

    ret = 0;
end:
    return ret;
}

void qtk_isemdlg_cfg_update_option(qtk_isemdlg_cfg_t *cfg, qtk_option_t *opt) {
    if (cfg->use_spx) {
        qtk_spx_cfg_update_option(&cfg->spx, opt);
    }
}
void qtk_isemdlg_cfg_update_params(qtk_isemdlg_cfg_t *cfg,
                                   wtk_local_cfg_t *params) {
    if (cfg->use_spx) {
        qtk_spx_cfg_update_params(&cfg->spx, params);
    }
}
