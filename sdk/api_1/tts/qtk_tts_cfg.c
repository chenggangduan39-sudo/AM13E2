#include "../tts/qtk_tts_cfg.h"
int qtk_tts_cfg_init(qtk_tts_cfg_t *cfg) {
    qtk_cldtts_cfg_init(&(cfg->cldtts));
    cfg->tts_fn = NULL;
    cfg->tts = NULL;
    cfg->use_cldtts = 1;

    cfg->main_cfg = NULL;
    cfg->mbin_cfg = NULL;

    return 0;
}

int qtk_tts_cfg_clean(qtk_tts_cfg_t *cfg) {
    if (cfg->use_cldtts) {
        qtk_cldtts_cfg_clean(&(cfg->cldtts));
    } else if (cfg->tts) {
        wtk_tts_cfg_delete_bin(cfg->tts);
    }
    return 0;
}

int qtk_tts_cfg_update_local(qtk_tts_cfg_t *cfg, wtk_local_cfg_t *main) {
    wtk_string_t *v;
    wtk_local_cfg_t *lc;
    int ret;

    lc = main;
    wtk_local_cfg_update_cfg_str(lc, cfg, tts_fn, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_cldtts, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_bin, v);

    if (cfg->use_cldtts) {
        lc = wtk_local_cfg_find_lc_s(main, "cldtts");
        if (lc) {
            ret = qtk_cldtts_cfg_update_local(&(cfg->cldtts), lc);
            if (ret != 0) {
                goto end;
            }
        }
    } else if (!cfg->tts_fn) {
        ret = -1;
        goto end;
    }

end:
    return 0;
}

int qtk_tts_cfg_update(qtk_tts_cfg_t *cfg) {
    int ret;

    if (cfg->use_cldtts) {
        ret = qtk_cldtts_cfg_update(&(cfg->cldtts));
        if (ret != 0) {
            goto end;
        }
    } else if (cfg->tts_fn) {
        cfg->tts = wtk_tts_cfg_new_bin(cfg->tts_fn);
        if (!cfg->tts) {
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

int qtk_tts_cfg_update_tts(qtk_tts_cfg_t *cfg, wtk_source_t *s) {
    int ret = 0;
    wtk_rbin2_item_t *item;

    item = (wtk_rbin2_item_t *)s->data;
    cfg->tts = wtk_tts_cfg_new_bin2(item->rb->fn, item->pos);
    if (!cfg->tts) {
        ret = -1;
    }
    return ret;
}

int qtk_tts_cfg_update2(qtk_tts_cfg_t *cfg, wtk_source_loader_t *sl) {
    int ret;

    if (cfg->use_cldtts) {
        ret = qtk_cldtts_cfg_update(&(cfg->cldtts));
        if (ret != 0) {
            goto end;
        }
    } else if (cfg->tts_fn) {
        ret = wtk_source_loader_load(
            sl, cfg, (wtk_source_load_handler_t)qtk_tts_cfg_update_tts,
            cfg->tts_fn);
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

void qtk_tts_cfg_update_option(qtk_tts_cfg_t *cfg, qtk_option_t *opt) {
    if (cfg->use_cldtts) {
        qtk_cldtts_cfg_update_option(&cfg->cldtts, opt);
    }
}

void qtk_tts_cfg_update_params(qtk_tts_cfg_t *cfg, wtk_local_cfg_t *params) {
    if (cfg->use_cldtts) {
        qtk_cldtts_cfg_update_params(&(cfg->cldtts), params);
    }
}

qtk_tts_cfg_t *qtk_tts_cfg_new(char *cfg_fn) {
    qtk_tts_cfg_t *cfg;
    wtk_main_cfg_t *main_cfg;

    main_cfg = wtk_main_cfg_new_type(qtk_tts_cfg, cfg_fn);
    cfg = (qtk_tts_cfg_t *)main_cfg->cfg;
    cfg->main_cfg = main_cfg;
    return cfg;
}

void qtk_tts_cfg_delete(qtk_tts_cfg_t *cfg) {
    wtk_main_cfg_delete(cfg->main_cfg);
}

qtk_tts_cfg_t *qtk_tts_cfg_new_bin(char *bin_fn) {
    qtk_tts_cfg_t *cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    mbin_cfg = wtk_mbin_cfg_new_type(qtk_tts_cfg, bin_fn, "./cfg");
    cfg = (qtk_tts_cfg_t *)mbin_cfg->cfg;
    cfg->mbin_cfg = mbin_cfg;
    return cfg;
}

void qtk_tts_cfg_delete_bin(qtk_tts_cfg_t *cfg) {
    wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
