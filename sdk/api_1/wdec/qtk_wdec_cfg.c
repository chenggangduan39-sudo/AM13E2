#include "qtk_wdec_cfg.h"
int qtk_wdec_cfg_init(qtk_wdec_cfg_t *cfg) 
{
    // wtk_wdec_cfg_init(cfg->wdec_cfg);
    wtk_string_set(&cfg->wdec_fn,0,0);
    wtk_string_set(&cfg->vad_fn,0,0);
    wtk_string_set(&cfg->words, 0, 0);
    cfg->wdec_cfg = NULL;
    cfg->kwdec2_cfg = NULL;
    cfg->vad_cfg = NULL;

    cfg->main_cfg = NULL;
    cfg->mbin_cfg = NULL;
    cfg->use_bin = 0;
    cfg->use_wdec = 1;
    cfg->use_kwdec2 = 0;
    cfg->use_vad = 0;

    cfg->left_margin = 10;
    cfg->right_margin = 0;

    return 0;
}

int qtk_wdec_cfg_clean(qtk_wdec_cfg_t *cfg) 
{
    if(cfg->wdec_cfg)
    {
        cfg->use_bin ? wtk_wdec_cfg_delete_bin(cfg->wdec_cfg) : wtk_wdec_cfg_delete(cfg->wdec_cfg);
    }
    if(cfg->kwdec2_cfg)
    {
        cfg->use_bin ? wtk_kwdec2_cfg_delete_bin(cfg->kwdec2_cfg) : wtk_kwdec2_cfg_delete(cfg->kwdec2_cfg->cfg.main_cfg);
    }
    if (cfg->vad_cfg) {
		wtk_vad_cfg_delete_bin(cfg->vad_cfg);
	}
    return 0;
}

int qtk_wdec_cfg_update_local(qtk_wdec_cfg_t *cfg, wtk_local_cfg_t *main) 
{
    wtk_string_t *v;
    wtk_local_cfg_t *lc = main;

    wtk_local_cfg_update_cfg_string_v(lc, cfg, words, v);
	wtk_local_cfg_update_cfg_string_v(lc, cfg, wdec_fn, v);
	wtk_debug("wdec cfg==>[%.*s]\n",cfg->wdec_fn.len, cfg->wdec_fn.data);
    wtk_local_cfg_update_cfg_string_v(lc, cfg, vad_fn, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_bin, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_wdec, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_kwdec2, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_vad, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, left_margin, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, right_margin, v);

    return 0;
}

int qtk_wdec_cfg_update(qtk_wdec_cfg_t *cfg) 
{
    int ret = -1;
    if(cfg->wdec_fn.len > 0){
        wtk_debug("%s\n", cfg->wdec_fn.data);
        if(cfg->use_wdec)
        {
            cfg->wdec_cfg = cfg->use_bin ? wtk_wdec_cfg_new_bin(cfg->wdec_fn.data) : wtk_wdec_cfg_new(cfg->wdec_fn.data);
            ret = 0;
        }else if(cfg->use_kwdec2)
        {
            cfg->kwdec2_cfg = cfg->use_bin ? wtk_kwdec2_cfg_new_bin2(cfg->wdec_fn.data) : wtk_kwdec2_cfg_new(cfg->wdec_fn.data);
            ret = 0;
        }
	}
    if (cfg->vad_fn.len > 0) {
		cfg->vad_cfg = wtk_vad_cfg_new_bin2(cfg->vad_fn.data);
	}

    return ret;
}

int qtk_wdec_cfg_update2(qtk_wdec_cfg_t *cfg, wtk_source_loader_t *sl) 
{
    return qtk_wdec_cfg_update(cfg);
}

qtk_wdec_cfg_t *qtk_wdec_cfg_new(char *cfg_fn) {
    qtk_wdec_cfg_t *cfg;
    wtk_main_cfg_t *main_cfg;

    main_cfg = wtk_main_cfg_new_type(qtk_wdec_cfg, cfg_fn);
    if(!main_cfg){
		return NULL;
	}
    cfg = (qtk_wdec_cfg_t *)main_cfg->cfg;
    cfg->main_cfg = main_cfg;
    return cfg;
}

void qtk_wdec_cfg_delete(qtk_wdec_cfg_t *cfg) {
    wtk_main_cfg_delete(cfg->main_cfg);
}

qtk_wdec_cfg_t *qtk_wdec_cfg_new_bin(char *bin_fn) {
    qtk_wdec_cfg_t *cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    mbin_cfg = wtk_mbin_cfg_new_type(qtk_wdec_cfg, bin_fn, "./cfg");
    cfg = (qtk_wdec_cfg_t *)mbin_cfg->cfg;
    cfg->mbin_cfg = mbin_cfg;
    return cfg;
}

void qtk_wdec_cfg_delete_bin(qtk_wdec_cfg_t *cfg) {
    wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
