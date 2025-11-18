#include "qtk_eaec_cfg.h"
#include "qtk/core/qtk_timer.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"

int qtk_eaec_cfg_init(qtk_eaec_cfg_t *cfg) {
    cfg->main_cfg = NULL;
    wtk_aec_cfg_init(&cfg->aec);
    cfg->input_channel = cfg->aec.stft.channel;
    cfg->sp_channel = cfg->sp_channel;
    return 0;
}

int qtk_eaec_cfg_clean(qtk_eaec_cfg_t *cfg) {
    wtk_aec_cfg_clean(&cfg->aec);
    return 0;
}

int qtk_eaec_cfg_update_local(qtk_eaec_cfg_t *cfg, wtk_local_cfg_t *main) {
    wtk_local_cfg_t *lc;
    wtk_string_t *v;


    lc = wtk_local_cfg_find_lc_s(main, "aec");
    if (lc) {
        if (wtk_aec_cfg_update_local(&cfg->aec, lc)) {
            goto err;
        }
    }

    cfg->input_channel = cfg->aec.stft.channel;
    cfg->sp_channel = cfg->aec.spchannel;
    wtk_local_cfg_update_cfg_i(main,cfg,input_channel,v);
    wtk_local_cfg_update_cfg_i(main,cfg,sp_channel,v);

    return 0;
err:
    return -1;
}

int qtk_eaec_cfg_update(qtk_eaec_cfg_t *cfg) {

    wtk_aec_cfg_update(&cfg->aec);

    return 0;
}

int qtk_eaec_cfg_update2(qtk_eaec_cfg_t *cfg, wtk_source_loader_t *sl) {

    wtk_aec_cfg_update2(&cfg->aec, sl);

    return 0;
}

qtk_eaec_cfg_t *qtk_eaec_cfg_new(const char *cfn) {
    qtk_eaec_cfg_t *cfg;
    wtk_main_cfg_t *main_cfg;

    main_cfg = wtk_main_cfg_new_type(qtk_eaec_cfg, cast(char *, cfn));
    cfg = cast(qtk_eaec_cfg_t *, main_cfg->cfg);
    cfg->main_cfg = main_cfg;
    return cfg;
}

qtk_eaec_cfg_t *qtk_eaec_cfg_new_bin(const char *cfn) {
    qtk_eaec_cfg_t *cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    mbin_cfg = wtk_mbin_cfg_new_type(qtk_eaec_cfg, cast(char *, cfn), "./cfg");
    cfg = cast(qtk_eaec_cfg_t *, mbin_cfg->cfg);
    cfg->mbin_cfg = mbin_cfg;
    return cfg;
}

void qtk_eaec_cfg_delete(qtk_eaec_cfg_t *cfg) {
    wtk_main_cfg_delete(cfg->main_cfg);
}

void qtk_eaec_cfg_delete_bin(qtk_eaec_cfg_t *cfg) {
    wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
