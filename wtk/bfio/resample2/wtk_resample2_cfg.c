#include "wtk_resample2_cfg.h"
int wtk_resample2_cfg_init(wtk_resample2_cfg_t *cfg){
    cfg->SR = 16000;
    cfg->new_SR = 48000;

    cfg->is_upsample = 1;
    cfg->upsamp_rate = 3;
    cfg->downsamp_rate = 0;
    cfg->frm_size = 128;
    cfg->window_sz = 256;
    cfg->taps = 126;
    cfg->weight_length = 127;
    cfg->beta = 9.0;
    cfg->cutoff_ratio = 0.15;
    return 0;
}


int wtk_resample2_cfg_clean(wtk_resample2_cfg_t *cfg){
    return 0;
}

int wtk_resample2_cfg_update_local(wtk_resample2_cfg_t *cfg, wtk_local_cfg_t *lc){
    wtk_string_t *v;

    wtk_local_cfg_update_cfg_i(lc, cfg, SR, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, new_SR, v);
    cfg->is_upsample = cfg->SR < cfg->new_SR? 1 : 0;
    if(cfg->is_upsample){
        cfg->upsamp_rate = cfg->new_SR / cfg->SR;
        cfg->downsamp_rate = 0;
    }else{
        cfg->upsamp_rate = 0;
        cfg->downsamp_rate = cfg->SR / cfg->new_SR;
    }
    wtk_local_cfg_update_cfg_i(lc, cfg, frm_size, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, window_sz, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, taps, v);
    cfg->weight_length = cfg->taps + 1;
    wtk_local_cfg_update_cfg_f(lc, cfg, beta, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, cutoff_ratio, v);
    return 0;
}

int wtk_resample2_cfg_update(wtk_resample2_cfg_t *cfg){
    return 0;
}

int wtk_resample2_cfg_update2(wtk_resample2_cfg_t *cfg, wtk_source_loader_t *sl){
   return 0;
}

wtk_resample2_cfg_t *wtk_resample2_cfg_new(char *fn)
{
    wtk_main_cfg_t *main_cfg;
    wtk_resample2_cfg_t *cfg;

    main_cfg=wtk_main_cfg_new_type(wtk_resample2_cfg,fn);
    if(!main_cfg)
    {
        return NULL;
    }
    cfg=(wtk_resample2_cfg_t*)main_cfg->cfg;
    cfg->main_cfg = main_cfg;
    return cfg;
}

void wtk_resample2_cfg_delete(wtk_resample2_cfg_t *cfg)
{
    wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_resample2_cfg_t* wtk_resample2_cfg_new_bin(char *fn)
{
    wtk_mbin_cfg_t *mbin_cfg;
    wtk_resample2_cfg_t *cfg;

    mbin_cfg=wtk_mbin_cfg_new_type(wtk_resample2_cfg,fn,"./cfg");
    if(!mbin_cfg)
    {
        return NULL;
    }
    cfg=(wtk_resample2_cfg_t*)mbin_cfg->cfg;
    cfg->mbin_cfg=mbin_cfg;
    return cfg;
}

void wtk_resample2_cfg_delete_bin(wtk_resample2_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
