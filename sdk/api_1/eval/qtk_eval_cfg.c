#include "qtk_eval_cfg.h"
int qtk_eval_cfg_init(qtk_eval_cfg_t *cfg) {
    qtk_spx_cfg_init(&(cfg->spx));
    cfg->engsnt_fn = NULL;
    cfg->engsnt = NULL;

    cfg->main_cfg = NULL;
    cfg->mbin_cfg = NULL;

    cfg->use_phone_ml = 0;
    cfg->use_cldeval = 0;
    return 0;
}

int qtk_eval_cfg_clean(qtk_eval_cfg_t *cfg) {
    if (cfg->use_cldeval) {
        qtk_spx_cfg_clean(&cfg->spx);
    } else if (cfg->engsnt) {
        wtk_engsnt_cfg_delete_bin3(cfg->engsnt);
    }

    return 0;
}

int qtk_eval_cfg_update_local(qtk_eval_cfg_t *cfg, wtk_local_cfg_t *main) {
    wtk_local_cfg_t *lc;
    wtk_string_t *v;
    int ret = 0;

    lc = main;
    wtk_local_cfg_update_cfg_str(lc, cfg, engsnt_fn, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_cldeval, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_phone_ml, v);

    if (cfg->use_cldeval) {
        lc = wtk_local_cfg_find_lc_s(main, "spx");
        if (lc) {
            qtk_spx_cfg_update_local(&(cfg->spx), lc);
        }
    } else if (cfg->engsnt_fn) {

    } else {
        ret = -1;
    }

    return ret;
}

int qtk_eval_cfg_update(qtk_eval_cfg_t *cfg) {
    int ret = 0;

    if (cfg->use_cldeval) {
        qtk_spx_cfg_update(&cfg->spx);
    } else if (cfg->engsnt_fn) {
        cfg->engsnt = wtk_engsnt_cfg_new_bin3(cfg->engsnt_fn, 0);
    } else {
        ret = -1;
    }

    return ret;
}

int qtk_eval_cfg_update_engsnt(qtk_eval_cfg_t *cfg, wtk_source_t *s) {
    int ret = 0;
    wtk_rbin2_item_t *item;

    item = (wtk_rbin2_item_t *)s->data;
    cfg->engsnt = wtk_engsnt_cfg_new_bin3(item->rb->fn, item->pos);
    if (!cfg->engsnt) {
        ret = -1;
    }
    return ret;
}

int qtk_eval_cfg_update2(qtk_eval_cfg_t *cfg, wtk_source_loader_t *sl) {
    int ret;

    if (cfg->use_cldeval) {
        ret = qtk_spx_cfg_update(&(cfg->spx));
        if (ret != 0) {
            goto end;
        }
    } else if (cfg->engsnt_fn) {
        ret = wtk_source_loader_load(
            sl, cfg, (wtk_source_load_handler_t)qtk_eval_cfg_update_engsnt,
            cfg->engsnt_fn);
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

void qtk_eval_cfg_update_option(qtk_eval_cfg_t *cfg, qtk_option_t *opt) {
    if (cfg->use_cldeval) {
        qtk_spx_cfg_update_option(&cfg->spx, opt);
    }
}

void qtk_eval_cfg_update_params(qtk_eval_cfg_t *cfg, wtk_local_cfg_t *params) {
    if (cfg->use_cldeval) {
        qtk_spx_cfg_update_params(&cfg->spx, params);
    }
}

qtk_eval_cfg_t *qtk_eval_cfg_new(char *cfg_fn) {
    wtk_main_cfg_t *main_cfg;
    qtk_eval_cfg_t *cfg;

    main_cfg = wtk_main_cfg_new_type(qtk_eval_cfg, cfg_fn);
    if (!main_cfg) {
        return NULL;
    }
    cfg = (qtk_eval_cfg_t *)main_cfg->cfg;
    cfg->main_cfg = main_cfg;
    return cfg;
}

void qtk_eval_cfg_delete(qtk_eval_cfg_t *cfg) {
    wtk_main_cfg_delete(cfg->main_cfg);
}

qtk_eval_cfg_t *qtk_eval_cfg_new_bin(char *bin_fn) {
    wtk_mbin_cfg_t *mbin_cfg;
    qtk_eval_cfg_t *cfg;

    mbin_cfg = wtk_mbin_cfg_new_type(qtk_eval_cfg, bin_fn, "./cfg");
    if (!mbin_cfg) {
        return NULL;
    }
    cfg = (qtk_eval_cfg_t *)mbin_cfg->cfg;
    cfg->mbin_cfg = mbin_cfg;
    return cfg;
}

void qtk_eval_cfg_delete_bin(qtk_eval_cfg_t *cfg) {
    wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
