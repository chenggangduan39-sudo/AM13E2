#include "qtk_tts_tac_cfg.h"


int qtk_tts_tac_cfg_init(qtk_tts_tac_cfg_t *cfg)
{
	cfg->use_durian_lpcnet = 0;
    qtk_tts_parse_cfg_init(&(cfg->parse_cfg));
    qtk_tts_tac2_lpcnet_cfg_init(&(cfg->syn_cfg));
    qtk_tts_durian_lpcnet_cfg_init(&cfg->durian_lpcnet_cfg);
    return 0;
}

int qtk_tts_tac_cfg_clean(qtk_tts_tac_cfg_t *cfg)
{
    qtk_tts_parse_cfg_clean(&(cfg->parse_cfg));
    if(cfg->use_durian_lpcnet){
        qtk_tts_durian_lpcnet_cfg_clean(&(cfg->durian_lpcnet_cfg));
    }else{
        qtk_tts_tac2_lpcnet_cfg_clean(&(cfg->syn_cfg));
    }
    return 0;
}

int qtk_tts_tac_cfg_update_local(qtk_tts_tac_cfg_t *cfg,wtk_local_cfg_t *main_cfg)
{
    wtk_local_cfg_t *lc = NULL;
    wtk_string_t *v = NULL;
    int ret = -1;
    lc = wtk_local_cfg_find_lc_s(main_cfg,"parser");
    if(!lc) goto end;
    qtk_tts_parse_cfg_update_local(&(cfg->parse_cfg),lc);
    wtk_local_cfg_update_cfg_i(main_cfg,cfg,use_durian_lpcnet,v);
    lc = wtk_local_cfg_find_lc_s(main_cfg,"syn");
    if(!lc) goto end;
    if(cfg->use_durian_lpcnet){
        qtk_tts_durian_lpcnet_cfg_update_local(&(cfg->durian_lpcnet_cfg),lc);
    }else{
        qtk_tts_tac2_lpcnet_cfg_update_local(&(cfg->syn_cfg),lc);
    }
    ret = 0;
end:
    return ret;
}

int qtk_tts_tac_cfg_update(qtk_tts_tac_cfg_t *cfg)
{
    int ret = 0;
    qtk_tts_parse_cfg_update(&(cfg->parse_cfg));
    if(cfg->use_durian_lpcnet){
        qtk_tts_durian_lpcnet_cfg_update(&(cfg->durian_lpcnet_cfg));
    }else{
        qtk_tts_tac2_lpcnet_cfg_update(&(cfg->syn_cfg));
    }
    return ret;
}
