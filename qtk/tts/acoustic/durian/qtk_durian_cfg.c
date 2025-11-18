#include "qtk_durian_cfg.h"

int qtk_durian_cfg_init(qtk_durian_cfg_t *cfg)
{
    memset(cfg,0,sizeof(*cfg));
    qtk_durian_encoder_cfg_init(&cfg->encoder_cfg);
    qtk_durian_duration_predictor_cfg_init(&cfg->dp_cfg);
    qtk_durian_decoder_cfg_init(&cfg->decoder_cfg);
    qtk_durian_postnet_cfg_init(&cfg->postnet_cfg);
    return 0;
}

int qtk_durian_cfg_clean(qtk_durian_cfg_t *cfg)
{
    qtk_durian_duration_predictor_cfg_clean(&cfg->dp_cfg);
    qtk_durian_encoder_cfg_clean(&cfg->encoder_cfg);
    qtk_durian_decoder_cfg_clean(&cfg->decoder_cfg);
    qtk_durian_postnet_cfg_clean(&cfg->postnet_cfg);
    return 0;
}

int qtk_durian_cfg_update_local(qtk_durian_cfg_t *cfg,wtk_local_cfg_t *main_lc)
{
    wtk_string_t *v = NULL;
    wtk_local_cfg_t *lc = NULL;

    cfg->embedding_dim_num = wtk_local_cfg_find_int_array_s(main_lc,"embedding_dim_num");
    wtk_local_cfg_update_cfg_str(main_lc,cfg,embedding_fn,v);

    lc = wtk_local_cfg_find_lc_s(main_lc,"encoder");
    if(lc){
        qtk_durian_encoder_cfg_update_local(&cfg->encoder_cfg,lc);
    }
    lc = wtk_local_cfg_find_lc_s(main_lc,"duration_predictor");
    if(lc){
        qtk_durian_duration_predictor_cfg_update_local(&cfg->dp_cfg,lc);
    }
    lc = wtk_local_cfg_find_lc_s(main_lc,"decoder");
    if(lc){
        qtk_durian_decoder_cfg_update_local(&cfg->decoder_cfg,lc);
    }
    lc = wtk_local_cfg_find_lc_s(main_lc,"postnet");
    if(lc){
        qtk_durian_postnet_cfg_update_local(&cfg->postnet_cfg,lc);
    }
    return 0;
}

int qtk_durian_cfg_update(qtk_durian_cfg_t *cfg)
{
    qtk_durian_encoder_cfg_update(&cfg->encoder_cfg);
    qtk_durian_duration_predictor_cfg_update(&cfg->dp_cfg);
    qtk_durian_decoder_cfg_update(&cfg->decoder_cfg);
    qtk_durian_postnet_cfg_update(&cfg->postnet_cfg);
    return 0;
}
