#include "qtk_semdlg_cfg.h"

int qtk_semdlg_cfg_init(qtk_semdlg_cfg_t *cfg) {
    wtk_queue_init(&cfg->semdlg_q);
    cfg->main_cfg = NULL;
    cfg->mbin_cfg = NULL;
    return 0;
}

int qtk_semdlg_cfg_clean(qtk_semdlg_cfg_t *cfg) {
    qtk_isemdlg_cfg_t *isemdlg;
    wtk_queue_node_t *qn;

    while (1) {
        qn = wtk_queue_pop(&cfg->semdlg_q);
        if (!qn) {
            break;
        }
        isemdlg = data_offset2(qn, qtk_isemdlg_cfg_t, q_n);
        qtk_isemdlg_cfg_clean(isemdlg);
        wtk_free(isemdlg);
    }

    return 0;
}

int qtk_semdlg_cfg_update_local(qtk_semdlg_cfg_t *cfg, wtk_local_cfg_t *main) {
    qtk_isemdlg_cfg_t *isemdlg;
    wtk_cfg_item_t *item;
    wtk_queue_node_t *qn;
    int ret;

    for (qn = main->cfg->queue.pop; qn; qn = qn->next) {
        item = data_offset2(qn, wtk_cfg_item_t, n);
        if (item->type != WTK_CFG_LC) {
            continue;
        }

        isemdlg = (qtk_isemdlg_cfg_t *)wtk_malloc(sizeof(qtk_isemdlg_cfg_t));
        qtk_isemdlg_cfg_init(isemdlg);
        ret = qtk_isemdlg_cfg_update_local(isemdlg, item->value.cfg);
        if (ret != 0) {
            goto end;
        }
        wtk_queue_push(&cfg->semdlg_q, &isemdlg->q_n);
    }

    ret = 0;
end:
    return ret;
}

int qtk_semdlg_cfg_update(qtk_semdlg_cfg_t *cfg) {
    qtk_isemdlg_cfg_t *isemdlg;
    wtk_queue_node_t *qn;
    int ret;

    for (qn = cfg->semdlg_q.pop; qn; qn = qn->next) {
        isemdlg = data_offset2(qn, qtk_isemdlg_cfg_t, q_n);
        ret = qtk_isemdlg_cfg_update(isemdlg);
        if (ret != 0) {
            goto end;
        }
    }

    ret = 0;
end:
    return ret;
}

int qtk_semdlg_cfg_update2(qtk_semdlg_cfg_t *cfg, wtk_source_loader_t *sl) {
    qtk_isemdlg_cfg_t *isemdlg;
    wtk_queue_node_t *qn;
    int ret;

    for (qn = cfg->semdlg_q.pop; qn; qn = qn->next) {
        isemdlg = data_offset2(qn, qtk_isemdlg_cfg_t, q_n);
        ret = qtk_isemdlg_cfg_update2(isemdlg, sl);
        if (ret != 0) {
            goto end;
        }
    }

    ret = 0;
end:
    return ret;
}

void qtk_semdlg_cfg_update_params(qtk_semdlg_cfg_t *cfg,
                                  wtk_local_cfg_t *params) {
    qtk_isemdlg_cfg_t *isemdlg;
    wtk_queue_node_t *qn;

    for (qn = cfg->semdlg_q.pop; qn; qn = qn->next) {
        isemdlg = data_offset2(qn, qtk_isemdlg_cfg_t, q_n);
        qtk_isemdlg_cfg_update_params(isemdlg, params);
    }
}

void qtk_semdlg_cfg_update_option(qtk_semdlg_cfg_t *cfg, qtk_option_t *opt) {
    qtk_isemdlg_cfg_t *isemdlg;
    wtk_queue_node_t *qn;

    for (qn = cfg->semdlg_q.pop; qn; qn = qn->next) {
        isemdlg = data_offset2(qn, qtk_isemdlg_cfg_t, q_n);
        qtk_isemdlg_cfg_update_option(isemdlg, opt);
    }
}

qtk_semdlg_cfg_t *qtk_semdlg_cfg_new(char *fn) {
    wtk_main_cfg_t *main_cfg;
    qtk_semdlg_cfg_t *cfg;

    main_cfg = wtk_main_cfg_new_type(qtk_semdlg_cfg, fn);
    if (!main_cfg) {
        return NULL;
    }

    cfg = (qtk_semdlg_cfg_t *)main_cfg->cfg;
    cfg->main_cfg = main_cfg;

    return cfg;
}

void qtk_semdlg_cfg_delete(qtk_semdlg_cfg_t *cfg) {
    wtk_main_cfg_delete(cfg->main_cfg);
}

qtk_semdlg_cfg_t *qtk_semdlg_cfg_new_bin(char *bin_fn) {
    wtk_mbin_cfg_t *mbin_cfg;
    qtk_semdlg_cfg_t *cfg;

    mbin_cfg = wtk_mbin_cfg_new_type(qtk_semdlg_cfg, bin_fn, "./cfg");
    if (!mbin_cfg) {
        return NULL;
    }

    cfg = (qtk_semdlg_cfg_t *)mbin_cfg->cfg;
    cfg->mbin_cfg = mbin_cfg;

    return cfg;
}

void qtk_semdlg_cfg_delete_bin(qtk_semdlg_cfg_t *cfg) {
    wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
