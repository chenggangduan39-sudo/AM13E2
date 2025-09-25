#include "qtk_asr_cfg.h"

int qtk_asr_cfg_init(qtk_asr_cfg_t *cfg) {
    qtk_iasr_cfg_init(&cfg->grammar);
    wtk_lex_cfg_init(&cfg->lex_cfg);
    wtk_string_set(&cfg->lex_fn, 0, 0);
    cfg->use_grammar = 0;

    cfg->iasrs = NULL;
    cfg->iasrs_valid = NULL;
    cfg->n_iasrs = 0;
    cfg->skip_iasrs = 0;

    cfg->main_cfg = NULL;
    cfg->rbin = NULL;
    cfg->cfile = NULL;

    cfg->rec_min_conf = 1.9;
    cfg->gr_min_conf = 2.0;

    cfg->use_json = 1;
    //	cfg->skip_space = 0;
    cfg->use_lex = 0;//新增的字段一律默认为0
    return 0;
}

int qtk_asr_cfg_clean(qtk_asr_cfg_t *cfg) {
    int i;
    wtk_lex_cfg_clean(&cfg->lex_cfg);

    if (cfg->iasrs) {
        for (i = 0; i < cfg->n_iasrs; ++i) {
            if (cfg->iasrs_valid[i]) {
                qtk_iasr_cfg_clean(cfg->iasrs + i);
            }
        }
        wtk_free(cfg->iasrs);
    }
    if (cfg->iasrs_valid) {
        wtk_free(cfg->iasrs_valid);
    }
    if (cfg->use_grammar) {
        qtk_iasr_cfg_clean(&cfg->grammar);
    }
    return 0;
}

int qtk_asr_cfg_update_local(qtk_asr_cfg_t *cfg, wtk_local_cfg_t *main) {
    wtk_local_cfg_t *lc;
    wtk_array_t *iasrs_array;
    wtk_string_t *v, **v1;
    int i;

    lc = main;
    wtk_local_cfg_update_cfg_string_v(lc, cfg, lex_fn, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, rec_min_conf, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, gr_min_conf, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_grammar, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_json, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_lex, v);
    //	wtk_local_cfg_update_cfg_b(lc,cfg,skip_space,v);

    if (cfg->use_grammar) {
        lc = wtk_local_cfg_find_lc_s(main, "grammar");
        if (lc) {
            qtk_iasr_cfg_update_local(&cfg->grammar, lc);
        }
        cfg->use_grammar = lc ? cfg->use_grammar : 0;
    }

    lc = wtk_local_cfg_find_lc_s(main, "lex_cfg");
    if (lc) {
        wtk_lex_cfg_update_local(&cfg->lex_cfg, lc);
    }else{
        lc=main;
    }

    iasrs_array = wtk_local_cfg_find_array_s(lc, "iasrs");
    if (iasrs_array && iasrs_array->nslot > 0) {
        cfg->n_iasrs = iasrs_array->nslot;
        cfg->iasrs =
            (qtk_iasr_cfg_t *)wtk_malloc(sizeof(qtk_iasr_cfg_t) * cfg->n_iasrs);
        cfg->iasrs_valid = (int *)wtk_malloc(sizeof(int) * cfg->n_iasrs);
        v1 = (wtk_string_t **)iasrs_array->slot;
        for (i = 0; i < cfg->n_iasrs; ++i) {
            lc = wtk_local_cfg_find_lc(main, v1[i]->data, v1[i]->len);
            if (lc) {
                qtk_iasr_cfg_init(cfg->iasrs + i);
                qtk_iasr_cfg_update_local(cfg->iasrs + i, lc);
                cfg->iasrs_valid[i] = 1;
            } else {
                cfg->iasrs_valid[i] = 0;
                ++cfg->skip_iasrs;
            }
        }
    }

    return 0;
}

int qtk_asr_cfg_update(qtk_asr_cfg_t *cfg) {
    int i, ret;

    wtk_lex_cfg_update(&cfg->lex_cfg);
    if (cfg->use_grammar) {
        ret = qtk_iasr_cfg_update(&cfg->grammar);
        if (ret != 0) {
            goto end;
        }
    }

    for (i = 0; i < cfg->n_iasrs; ++i) {
        if (cfg->iasrs_valid[i]) {
            ret = qtk_iasr_cfg_update(cfg->iasrs + i);
            if (ret != 0) {
                goto end;
            }
        }
    }
    ret = 0;
end:
    return ret;
}

int qtk_asr_cfg_update2(qtk_asr_cfg_t *cfg, wtk_source_loader_t *sl) {
    int i, ret;

    wtk_lex_cfg_update2(&cfg->lex_cfg,sl);
    if (cfg->use_grammar) {
        ret = qtk_iasr_cfg_update2(&cfg->grammar, sl);
        if (ret != 0) {
            goto end;
        }
    }

    for (i = 0; i < cfg->n_iasrs; ++i) {
        if (cfg->iasrs_valid[i]) {
            ret = qtk_iasr_cfg_update2(cfg->iasrs + i, sl);
            if (ret != 0) {
                goto end;
            }
        }
    }
    ret = 0;
end:
    return ret;
}

void qtk_asr_cfg_update_params(qtk_asr_cfg_t *cfg, wtk_local_cfg_t *params) {
    wtk_string_t *v;
    int i;

    wtk_local_cfg_update_cfg_f(params, cfg, rec_min_conf, v);
    wtk_local_cfg_update_cfg_f(params, cfg, gr_min_conf, v);
    wtk_local_cfg_update_cfg_b(params, cfg, use_json, v);
    wtk_local_cfg_update_cfg_b(params, cfg, use_lex, v);
    //	wtk_local_cfg_update_cfg_b(params,cfg,skip_space,v);

    for (i = 0; i < cfg->n_iasrs; ++i) {
        if (cfg->iasrs_valid[i]) {
            qtk_iasr_cfg_update_params(cfg->iasrs + i, params);
        }
    }
}

void qtk_asr_cfg_update_option(qtk_asr_cfg_t *cfg, qtk_option_t *opt) {
    int i;

    if (cfg->use_grammar) {
        qtk_iasr_cfg_update_option(&cfg->grammar, opt);
    }

    for (i = 0; i < cfg->n_iasrs; ++i) {
        qtk_iasr_cfg_update_option(cfg->iasrs + i, opt);
    }
}

qtk_asr_cfg_t *qtk_asr_cfg_new(char *fn) {
    wtk_main_cfg_t *main_cfg;
    qtk_asr_cfg_t *cfg;

    main_cfg = wtk_main_cfg_new_type(qtk_asr_cfg, fn);
    cfg = (qtk_asr_cfg_t *)main_cfg->cfg;
    cfg->main_cfg = main_cfg;

    return cfg;
}

void qtk_asr_cfg_delete(qtk_asr_cfg_t *cfg) {
    wtk_main_cfg_delete(cfg->main_cfg);
}

qtk_asr_cfg_t *qtk_asr_cfg_new_bin(char *bin_fn) {
    qtk_asr_cfg_t *cfg;
    wtk_rbin2_item_t *item;
    wtk_source_loader_t sl;
    char *cfg_fn = "./cfg";
    int ret;

    cfg = (qtk_asr_cfg_t *)wtk_malloc(sizeof(qtk_asr_cfg_t));
    qtk_asr_cfg_init(cfg);

    cfg->rbin = wtk_rbin2_new();
    ret = wtk_rbin2_read(cfg->rbin, bin_fn);
    if (ret != 0) {
        wtk_debug("read failed %s\n", bin_fn);
        goto end;
    }

    item = wtk_rbin2_get2(cfg->rbin, cfg_fn, strlen(cfg_fn));
    if (!item) {
        wtk_debug("%s not found %s\n", cfg_fn, bin_fn);
        ret = -1;
        goto end;
    }

    cfg->cfile = wtk_cfg_file_new();
    wtk_cfg_file_add_var_ks(cfg->cfile, "pwd", ".", 1);
    ret = wtk_cfg_file_feed(cfg->cfile, item->data->data, item->data->len);
    if (ret != 0) {
        goto end;
    }

    ret = qtk_asr_cfg_update_local(cfg, cfg->cfile->main);
    if (ret != 0) {
        goto end;
    }

    sl.hook = cfg->rbin;
    sl.vf = (wtk_source_loader_v_t)wtk_rbin2_load_file;
    ret = qtk_asr_cfg_update2(cfg, &sl);
    if (ret != 0) {
        goto end;
    }

    ret = 0;
end:
    if (ret != 0) {
        qtk_asr_cfg_delete_bin(cfg);
        cfg = NULL;
    }
    return cfg;
}

void qtk_asr_cfg_delete_bin(qtk_asr_cfg_t *cfg) {
    qtk_asr_cfg_clean(cfg);

    if (cfg->cfile) {
        wtk_cfg_file_delete(cfg->cfile);
    }

    if (cfg->rbin) {
        wtk_rbin2_delete(cfg->rbin);
    }

    wtk_free(cfg);
}
