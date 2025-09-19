#include "qtk_tts_tac2_lpcnet_cfg.h"

int qtk_tts_tac2_lpcnet_cfg_init(qtk_tts_tac2_lpcnet_cfg_t *cfg)
{
    qtk_tts_tac2_syn_cfg_init(&cfg->tac2_cfg);
    wtk_tac_cfg_syn_lpcnet_init(&cfg->lpcnet_cfg);
    cfg->sample_rate = 16000;
    return 0;
}

int qtk_tts_tac2_lpcnet_cfg_clean(qtk_tts_tac2_lpcnet_cfg_t *cfg)
{
    qtk_tts_tac2_syn_cfg_clean(&cfg->tac2_cfg);
    wtk_tac_cfg_syn_lpcnet_clean(&cfg->lpcnet_cfg);
    return 0;
}

int qtk_tts_tac2_lpcnet_cfg_update_local(qtk_tts_tac2_lpcnet_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    wtk_local_cfg_t *main_lc = lc;
    lc = wtk_local_cfg_find_lc_s(main_lc,"tac2");
    if(lc){
        qtk_tts_tac2_syn_cfg_update_local(&cfg->tac2_cfg,lc);
    }
    lc = wtk_local_cfg_find_lc_s(main_lc,"lpcnet");
    if(lc){
        wtk_tac_cfg_syn_lpcnet_update_local(&cfg->lpcnet_cfg,lc);
    }
    return 0;
}

int qtk_tts_tac2_lpcnet_cfg_update(qtk_tts_tac2_lpcnet_cfg_t *cfg)
{
    qtk_tts_tac2_syn_cfg_update(&cfg->tac2_cfg);
    wtk_tac_cfg_syn_lpcnet_update(&cfg->lpcnet_cfg);
    return 0;
}