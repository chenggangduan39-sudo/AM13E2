#include "qtk_tts_durian_lpcnet_cfg.h"

int qtk_tts_durian_lpcnet_cfg_init(qtk_tts_durian_lpcnet_cfg_t *cfg)
{
    qtk_durian_cfg_init(&cfg->durian_cfg);
    wtk_tac_cfg_syn_lpcnet_init(&cfg->lpcnet_cfg);
    return 0;
}

int qtk_tts_durian_lpcnet_cfg_clean(qtk_tts_durian_lpcnet_cfg_t *cfg)
{
    qtk_durian_cfg_clean(&cfg->durian_cfg);
    wtk_tac_cfg_syn_lpcnet_clean(&cfg->lpcnet_cfg);
    return 0;
}

int qtk_tts_durian_lpcnet_cfg_update_local(qtk_tts_durian_lpcnet_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    wtk_local_cfg_t *main_lc = lc;
    lc = wtk_local_cfg_find_lc_s(main_lc,"durian");
    if(lc){
        qtk_durian_cfg_update_local(&cfg->durian_cfg,lc);
    }
    lc = wtk_local_cfg_find_lc_s(main_lc,"lpcnet");
    if(lc){
        wtk_tac_cfg_syn_lpcnet_update_local(&cfg->lpcnet_cfg,lc);
    }
    return 0;
}

int qtk_tts_durian_lpcnet_cfg_update(qtk_tts_durian_lpcnet_cfg_t *cfg)
{
    qtk_durian_cfg_update(&cfg->durian_cfg);
    wtk_tac_cfg_syn_lpcnet_update(&cfg->lpcnet_cfg);
    return 0;
}