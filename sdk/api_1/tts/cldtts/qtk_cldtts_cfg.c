#include "qtk_cldtts_cfg.h"
int qtk_cldtts_cfg_init(qtk_cldtts_cfg_t *cfg) {
    qtk_spx_cfg_init(&(cfg->spx));
    cfg->main_cfg = NULL;
    cfg->mbin_cfg = NULL;
    cfg->use_split = 1;
    cfg->use_mp3dec = 1;
    return 0;
}

int qtk_cldtts_cfg_clean(qtk_cldtts_cfg_t *cfg) {
    qtk_spx_cfg_clean(&(cfg->spx));
    return 0;
}

int qtk_cldtts_cfg_update_local(qtk_cldtts_cfg_t *cfg, wtk_local_cfg_t *main) {
    wtk_local_cfg_t *lc;
    wtk_string_t *v;

    lc = main;
    wtk_local_cfg_update_cfg_b(lc, cfg, use_split, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_mp3dec, v);

    lc = wtk_local_cfg_find_lc_s(main, "spx");
    if (lc) {
        qtk_spx_cfg_update_local(&(cfg->spx), lc);
    }
    return 0;
}

int qtk_cldtts_cfg_update(qtk_cldtts_cfg_t *cfg) {
    qtk_spx_cfg_update(&(cfg->spx));
    return 0;
}

void qtk_cldtts_cfg_update_params(qtk_cldtts_cfg_t *cfg, wtk_local_cfg_t *lc) {
    qtk_spx_cfg_update_params(&(cfg->spx), lc);
}

int qtk_cldtts_cfg_update2(qtk_cldtts_cfg_t *cfg, wtk_source_loader_t *sl) {
    return qtk_cldtts_cfg_update(cfg);
}

void qtk_cldtts_cfg_update_option(qtk_cldtts_cfg_t *cfg, qtk_option_t *opt) {
    qtk_spx_cfg_update_option(&cfg->spx, opt);
}

qtk_cldtts_cfg_t *qtk_cldtts_cfg_new(char *fn) {
    wtk_main_cfg_t *main_cfg;
    qtk_cldtts_cfg_t *cfg;

    main_cfg = wtk_main_cfg_new_type(qtk_cldtts_cfg, fn);
    cfg = (qtk_cldtts_cfg_t *)main_cfg->cfg;
    cfg->main_cfg = main_cfg;
    return cfg;
}

void qtk_cldtts_cfg_delete(qtk_cldtts_cfg_t *cfg) {
    wtk_main_cfg_delete(cfg->main_cfg);
}

qtk_cldtts_cfg_t *qtk_cldtts_cfg_new_bin(char *bin_fn) {
    wtk_mbin_cfg_t *mbin_cfg;
    qtk_cldtts_cfg_t *cfg;
    char *cfg_fn = "./cfg";

    mbin_cfg = wtk_mbin_cfg_new_type(qtk_cldtts_cfg, bin_fn, cfg_fn);
    cfg = (qtk_cldtts_cfg_t *)mbin_cfg->cfg;
    cfg->mbin_cfg = mbin_cfg;
    return cfg;
}

void qtk_cldtts_cfg_delete_bin(qtk_cldtts_cfg_t *cfg) {
    wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
