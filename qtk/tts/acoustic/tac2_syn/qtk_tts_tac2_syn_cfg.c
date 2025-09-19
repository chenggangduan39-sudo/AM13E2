#include "qtk_tts_tac2_syn_cfg.h"


int qtk_tts_tac2_syn_cfg_init(qtk_tts_tac2_syn_cfg_t *cfg)
{
    memset(cfg, 0, sizeof(qtk_tts_tac2_syn_cfg_t));
    qtk_tts_tac2_syn_enc_cfg_init(&cfg->enc_cfg);
    qtk_tts_dfsmn_cfg_init(&cfg->dfsmn_enc_cfg);
    qtk_tts_tac2_syn_dec_cfg_init(&cfg->dec_cfg);
    qtk_tts_tac2_syn_postnet_cfg_init(&cfg->postnet);
    qtk_tts_tac2_syn_gmmdec_cfg_init(&cfg->gmmdec_cfg);
    cfg->use_postnet = 1;
    return 0;
}

int qtk_tts_tac2_syn_cfg_clean(qtk_tts_tac2_syn_cfg_t *cfg)
{
    if(cfg->use_dfsmn == 0){
        qtk_tts_tac2_syn_enc_cfg_clean(&cfg->enc_cfg);
    }else{
        qtk_tts_dfsmn_cfg_clean(&cfg->dfsmn_enc_cfg);
    }
    if(cfg->use_gmmdec == 0){
        qtk_tts_tac2_syn_dec_cfg_clean(&cfg->dec_cfg);
    }else{
        qtk_tts_tac2_syn_gmmdec_cfg_clean(&cfg->gmmdec_cfg);
    }
    if(cfg->use_postnet){
        qtk_tts_tac2_syn_postnet_cfg_clean(&cfg->postnet);
    }
    return 0;
}

int qtk_tts_tac2_syn_cfg_update_local(qtk_tts_tac2_syn_cfg_t *cfg,wtk_local_cfg_t *main_lc)
{
    wtk_local_cfg_t *lc = NULL;
    wtk_string_t *v = NULL;

    cfg->embedding_dim = wtk_local_cfg_find_int_array_s(main_lc,"embedding_dim");
    wtk_local_cfg_update_cfg_str(main_lc,cfg,embedding_fn,v);
    wtk_local_cfg_update_cfg_b(main_lc,cfg,use_postnet,v);
    wtk_local_cfg_update_cfg_b(main_lc,cfg,use_dfsmn,v);
    wtk_local_cfg_update_cfg_b(main_lc,cfg,use_gmmdec,v);
    lc = wtk_local_cfg_find_lc_s(main_lc,"encoder");
    if(lc){
        qtk_tts_tac2_syn_enc_cfg_update_local(&cfg->enc_cfg,lc);
    }
    //dfsmn
    lc = wtk_local_cfg_find_lc_s(main_lc,"DFSMN");
    if(lc){
        qtk_tts_dfsmn_cfg_update_local(&cfg->dfsmn_enc_cfg,lc);
    }
    lc = wtk_local_cfg_find_lc_s(main_lc,"decoder");
    if(lc){
        qtk_tts_tac2_syn_dec_cfg_update_local(&cfg->dec_cfg,lc);
    }
    //gmm_decoder
    lc = wtk_local_cfg_find_lc_s(main_lc,"gmm_decoder");
    if(lc){
        qtk_tts_tac2_syn_gmmdec_cfg_update_local(&cfg->gmmdec_cfg,lc);
    }
    lc = wtk_local_cfg_find_lc_s(main_lc,"postnet");
    if(lc && cfg->use_postnet){
        qtk_tts_tac2_syn_postnet_cfg_update_local(&cfg->postnet,lc);
    }
    return 0;
}

int qtk_tts_tac2_syn_cfg_update(qtk_tts_tac2_syn_cfg_t *cfg)
{
    if(cfg->use_dfsmn == 0){
        qtk_tts_tac2_syn_enc_cfg_update(&cfg->enc_cfg);
    }else{
        qtk_tts_dfsmn_cfg_update(&cfg->dfsmn_enc_cfg);
    }
    if(cfg->use_gmmdec == 0){
        qtk_tts_tac2_syn_dec_cfg_update(&cfg->dec_cfg);
    }else{
        qtk_tts_tac2_syn_gmmdec_cfg_update(&cfg->gmmdec_cfg);
    }
    if(cfg->use_postnet){
        qtk_tts_tac2_syn_postnet_cfg_update(&cfg->postnet);
    }
    return 0;
}
