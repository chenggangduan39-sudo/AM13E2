#include "qtk_qkws_cfg.h"
int qtk_qkws_cfg_init(qtk_qkws_cfg_t *cfg)
{
    wtk_vad_cfg_init(&(cfg->vad));
    cfg->cfg_fn = NULL;
    cfg->vad_fn = NULL;
    cfg->xvad = NULL;
    cfg->kws_cfg = NULL;
    cfg->sc_cfg = NULL;
    cfg->cmask_pse_cfg = NULL;
    cfg->vbox_cfg = NULL;
    cfg->vbox_fn = NULL;
    cfg->aec_cfg = NULL;
    cfg->aec_fn = NULL;

    cfg->main_cfg = NULL;
    cfg->mbin_cfg = NULL;
    cfg->use_bin = 1;
    cfg->use_kws = 0;
    cfg->use_sc = 0;
    cfg->use_cmask_pse = 0;
    cfg->use_vad = 0;
    cfg->use_mode = 0;
    cfg->result_dur = -1.0;
    cfg->channel = 1;
    cfg->use_vboxebf = 0;
    cfg->use_aec=0;
    cfg->use_add_zero = 0;
#ifdef USE_DESAIXIWEI
    cfg->use_add_zero = 1;
#endif

    return 0;
}

int qtk_qkws_cfg_clean(qtk_qkws_cfg_t *cfg)
{
    if(cfg->kws_cfg){
        cfg->use_bin ? qtk_kws_cfg_delete_bin(cfg->kws_cfg) : qtk_kws_cfg_delete(cfg->kws_cfg);
    }
    if(cfg->sc_cfg){
        cfg->use_bin ? qtk_sond_cluster_cfg_delete_bin(cfg->sc_cfg) : qtk_sond_cluster_cfg_delete(cfg->sc_cfg);
    }
    if(cfg->cmask_pse_cfg){
        cfg->use_bin ? wtk_cmask_pse_cfg_delete_bin(cfg->cmask_pse_cfg) : wtk_cmask_pse_cfg_delete(cfg->cmask_pse_cfg);
    }
    if(cfg->vbox_cfg){
        qtk_vboxebf_cfg_delete(cfg->vbox_cfg);
    }
    if(cfg->aec_cfg){
        qtk_aec_cfg_delete(cfg->aec_cfg);
    }
    if(cfg->use_vad){
        if (cfg->xvad) {
            wtk_vad_cfg_delete_bin(cfg->xvad);
        } else {
            wtk_vad_cfg_clean(&(cfg->vad));
        }
    }
    return 0;
}

int qtk_qkws_cfg_update_local(qtk_qkws_cfg_t *cfg, wtk_local_cfg_t *main) {
    wtk_local_cfg_t *lc;
    wtk_string_t *v;
    int ret = 0;

    lc = main;
    wtk_local_cfg_update_cfg_str(lc, cfg, cfg_fn, v);
    wtk_debug("==================>>>>>>>>>>>cfg_fn=%s\n",cfg->cfg_fn);
    wtk_local_cfg_update_cfg_str(lc, cfg, vad_fn, v);
    wtk_local_cfg_update_cfg_str(lc, cfg, vbox_fn, v);
    wtk_local_cfg_update_cfg_str(lc, cfg, aec_fn, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, result_dur, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, use_mode, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, channel, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_bin, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_kws, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_sc, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_cmask_pse, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_vad, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_vboxebf, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_aec, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_add_zero, v);
    
    if (!cfg->vad_fn && cfg->use_vad) {
        lc = wtk_local_cfg_find_lc_s(main, "vad");
        if (lc) {
            wtk_vad_cfg_update_local(&(cfg->vad), lc);
        }
    }

    return ret;
}

int qtk_qkws_cfg_update(qtk_qkws_cfg_t *cfg)
{
    int ret = -1;

    if(cfg->use_kws){
        cfg->kws_cfg = cfg->use_bin ? qtk_kws_cfg_new_bin(cfg->cfg_fn, "./kws.cfg") : qtk_kws_cfg_new(cfg->cfg_fn);
        if(cfg->kws_cfg){
            ret=0;
        }
    }
    if(cfg->use_sc){
        cfg->sc_cfg = cfg->use_bin ? qtk_sond_cluster_cfg_new_bin(cfg->cfg_fn, "./cls.cfg") : qtk_sond_cluster_cfg_new(cfg->cfg_fn);
        if(cfg->sc_cfg){
            ret=0;
        }
    }
    if(cfg->use_cmask_pse){
        cfg->cmask_pse_cfg = cfg->use_bin ? wtk_cmask_pse_cfg_new_bin(cfg->cfg_fn) : wtk_cmask_pse_cfg_new(cfg->cfg_fn);
        if(cfg->cmask_pse_cfg){
            ret=0;
        }
    }

    if(cfg->use_vboxebf){
        cfg->vbox_cfg = qtk_vboxebf_cfg_new(cfg->vbox_fn);
        if(cfg->vbox_cfg){
            ret = 0;
        }
    }
    if(cfg->use_aec){
        cfg->aec_cfg = qtk_aec_cfg_new(cfg->aec_fn);
        if(cfg->aec_cfg){
            ret = 0;
        }
    }

    if(cfg->use_vad){
        if (cfg->vad_fn && cfg->xvad == NULL) {
            cfg->xvad = wtk_vad_cfg_new_bin2(cfg->vad_fn);
        } else {
            wtk_vad_cfg_update(&(cfg->vad));
        }
    }

    return ret;
}

int qtk_qkws_cfg_update_vad(qtk_qkws_cfg_t *cfg, wtk_source_t *s) {
    int ret = 0;
    wtk_rbin2_item_t *item;

    item = (wtk_rbin2_item_t *)s->data;
    cfg->xvad = wtk_vad_cfg_new_bin3(item->rb->fn, item->pos);
    if (!cfg->xvad) {
        ret = -1;
    }
    return ret;
}

int qtk_qkws_cfg_update2(qtk_qkws_cfg_t *cfg, wtk_source_loader_t *sl)
{
    int ret;

    if(cfg->use_vad){
        if (cfg->vad_fn) {
            ret = wtk_source_loader_load(
                sl, cfg, (wtk_source_load_handler_t)qtk_qkws_cfg_update_vad,
                cfg->vad_fn);
            if (ret != 0) {
                goto end;
            }
        } else {
            wtk_vad_cfg_update2(&(cfg->vad), sl);
        }
    }
    qtk_qkws_cfg_update(cfg);
end:
    return ret;
}


qtk_qkws_cfg_t *qtk_qkws_cfg_new(char *cfg_fn) {
    wtk_main_cfg_t *main_cfg;
    qtk_qkws_cfg_t *cfg;

    main_cfg = wtk_main_cfg_new_type(qtk_qkws_cfg, cfg_fn);
    if (!main_cfg) {
        return NULL;
    }
    cfg = (qtk_qkws_cfg_t *)main_cfg->cfg;
    cfg->main_cfg = main_cfg;
    return cfg;
}

void qtk_qkws_cfg_delete(qtk_qkws_cfg_t *cfg) {
    wtk_main_cfg_delete(cfg->main_cfg);
}

qtk_qkws_cfg_t *qtk_qkws_cfg_new_bin(char *bin_fn) {
    wtk_mbin_cfg_t *mbin_cfg;
    qtk_qkws_cfg_t *cfg;

    mbin_cfg = wtk_mbin_cfg_new_type(qtk_qkws_cfg, bin_fn, "./cfg");
    if (!mbin_cfg) {
        return NULL;
    }
    cfg = (qtk_qkws_cfg_t *)mbin_cfg->cfg;
    cfg->mbin_cfg = mbin_cfg;
    return cfg;
}

void qtk_qkws_cfg_delete_bin(qtk_qkws_cfg_t *cfg) {
    if(cfg->xvad && cfg->use_vad){
        wtk_vad_cfg_delete_bin2(cfg->xvad);
        cfg->xvad = NULL;
    }
    wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
