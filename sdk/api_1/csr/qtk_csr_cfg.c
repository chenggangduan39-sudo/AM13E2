#include "qtk_csr_cfg.h"

//wtk_main_cfg_new2 会将内存calloc,不需要你memset
int qtk_csr_cfg_init(qtk_csr_cfg_t *cfg) {
    wtk_vad_cfg_init(&(cfg->vad));
    qtk_asr_cfg_init(&(cfg->asr));

    cfg->main_cfg = NULL;
    cfg->mbin_cfg = NULL;
    cfg->vad_fn = NULL;
    cfg->xvad = NULL;
    return 0;
}

int qtk_csr_cfg_clean(qtk_csr_cfg_t *cfg) {
    qtk_asr_cfg_clean(&(cfg->asr));
    if (cfg->xvad) {
        wtk_vad_cfg_delete_bin(cfg->xvad);
    } else {
        wtk_vad_cfg_clean(&(cfg->vad));
    }
    return 0;
}

int qtk_csr_cfg_update_local(qtk_csr_cfg_t *cfg, wtk_local_cfg_t *main) {
    wtk_local_cfg_t *lc;
    wtk_string_t *v;

    lc = main;
    wtk_local_cfg_update_cfg_str(lc, cfg, vad_fn, v);

    lc = wtk_local_cfg_find_lc_s(main, "asr");
    if (lc) {
        qtk_asr_cfg_update_local(&(cfg->asr), lc);
    }

    if (!cfg->vad_fn) {
        lc = wtk_local_cfg_find_lc_s(main, "vad");
        if (lc) {
            wtk_vad_cfg_update_local(&(cfg->vad), lc);
        }
    }
    return 0;
}

int qtk_csr_cfg_update(qtk_csr_cfg_t *cfg) {
    if (cfg->vad_fn) {
        cfg->xvad = wtk_vad_cfg_new_bin2(cfg->vad_fn);
    } else {
        wtk_vad_cfg_update(&(cfg->vad));
    }

    qtk_asr_cfg_update(&(cfg->asr));
    return 0;
}

int qtk_csr_cfg_update_vad(qtk_csr_cfg_t *cfg, wtk_source_t *s) {
    int ret = 0;
    wtk_rbin2_item_t *item;

    item = (wtk_rbin2_item_t *)s->data;
    cfg->xvad = wtk_vad_cfg_new_bin3(item->rb->fn, item->pos);
    if (!cfg->xvad) {
        ret = -1;
    }
    return ret;
}

int qtk_csr_cfg_update2(qtk_csr_cfg_t *cfg, wtk_source_loader_t *sl) {
    int ret;

    if (cfg->vad_fn) {
        ret = wtk_source_loader_load(
            sl, cfg, (wtk_source_load_handler_t)qtk_csr_cfg_update_vad,
            cfg->vad_fn);
        if (ret != 0) {
            goto end;
        }
    } else {
        wtk_vad_cfg_update2(&(cfg->vad), sl);
    }
    qtk_asr_cfg_update2(&(cfg->asr), sl);
end:
    return ret;
}

void qtk_csr_cfg_update_params(qtk_csr_cfg_t *cfg, wtk_local_cfg_t *params) {
    qtk_asr_cfg_update_params(&(cfg->asr), params);
}

void qtk_csr_cfg_update_option(qtk_csr_cfg_t *cfg, qtk_option_t *opt) {
    qtk_asr_cfg_update_option(&(cfg->asr), opt);
}

qtk_csr_cfg_t *qtk_csr_cfg_new(char *cfg_fn) {
    wtk_main_cfg_t *main_cfg;
    qtk_csr_cfg_t *cfg;

    main_cfg = wtk_main_cfg_new_type(qtk_csr_cfg, cfg_fn);
    cfg = (qtk_csr_cfg_t *)main_cfg->cfg;
    cfg->main_cfg = main_cfg;
    return cfg;
}

void qtk_csr_cfg_delete(qtk_csr_cfg_t *cfg) {
    wtk_main_cfg_delete(cfg->main_cfg);
}

qtk_csr_cfg_t *qtk_csr_cfg_new_bin(char *bin_fn) {
    wtk_mbin_cfg_t *mbin_cfg;
    qtk_csr_cfg_t *cfg;

    mbin_cfg = wtk_mbin_cfg_new_type(qtk_csr_cfg, bin_fn, "./cfg");
    cfg = (qtk_csr_cfg_t *)mbin_cfg->cfg;
    cfg->mbin_cfg = mbin_cfg;
    return cfg;
}

void qtk_csr_cfg_delete_bin(qtk_csr_cfg_t *cfg) {
    if(cfg->xvad){
        wtk_vad_cfg_delete_bin2(cfg->xvad);
        cfg->xvad = NULL;
    }
    wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
