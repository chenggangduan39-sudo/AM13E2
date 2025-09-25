#include "qtk_devicetts_cfg.h"
#include "qtk_tts_tac.h"

int qtk_devicetts_cfg_init(qtk_devicetts_cfg_t *cfg)
{
	memset(cfg,0, sizeof(qtk_devicetts_cfg_t));
	qtk_tts_parse_cfg_init(&(cfg->parse_cfg));
    qtk_tts_dfsmn_cfg_init(&cfg->encoder);
    qtk_devicetts_duration_predictor_cfg_init(&cfg->dur);
    qtk_devicetts_decoder_cfg_init(&cfg->dec);
    qtk_devicetts_postnet_cfg_init(&cfg->postnet);

    return 0;
}

int qtk_devicetts_cfg_clean(qtk_devicetts_cfg_t *cfg)
{
	qtk_tts_parse_cfg_clean(&(cfg->parse_cfg));
    qtk_tts_dfsmn_cfg_clean(&cfg->encoder);
    qtk_devicetts_duration_predictor_cfg_clean(&cfg->dur);
    qtk_devicetts_decoder_cfg_clean(&cfg->dec);
    qtk_devicetts_postnet_cfg_clean(&cfg->postnet);
    return 0;
}

int qtk_devicetts_cfg_update_local(qtk_devicetts_cfg_t *cfg,wtk_local_cfg_t *main_lc)
{
    wtk_string_t *v = NULL;
    wtk_local_cfg_t *lc = NULL;

    cfg->embedding_dim_num = wtk_local_cfg_find_int_array_s(main_lc,"embedding_dim_num");
    wtk_local_cfg_update_cfg_str(main_lc,cfg,embedding_fn,v);

    lc = wtk_local_cfg_find_lc_s(main_lc,"parser");
    if(lc) {
    	qtk_tts_parse_cfg_update_local(&(cfg->parse_cfg),lc);
    }

    lc = wtk_local_cfg_find_lc_s(main_lc,"DFSMN");
    if(lc){
        qtk_tts_dfsmn_cfg_update_local(&cfg->encoder,lc);
    }
    lc = wtk_local_cfg_find_lc_s(main_lc,"duration");
    if(lc){
        qtk_devicetts_duration_predictor_cfg_update_local(&cfg->dur,lc);
    }
    lc = wtk_local_cfg_find_lc_s(main_lc,"decoder");
    if(lc){
        qtk_devicetts_decoder_cfg_update_local(&cfg->dec,lc);
    }
    lc = wtk_local_cfg_find_lc_s(main_lc,"postnet");
    if(lc){
        qtk_devicetts_postnet_cfg_update_local(&cfg->postnet,lc);
    }

    wtk_local_cfg_update_cfg_str(main_lc,cfg,mean_fn,v);
    wtk_local_cfg_update_cfg_str(main_lc,cfg,std_fn,v);

    return 0;
}

int qtk_devicetts_cfg_update(qtk_devicetts_cfg_t *cfg)
{
	qtk_tts_parse_cfg_update(&(cfg->parse_cfg));
    qtk_tts_dfsmn_cfg_update(&cfg->encoder);
    qtk_devicetts_duration_predictor_cfg_update(&cfg->dur);
    qtk_devicetts_decoder_cfg_update(&cfg->dec);
    qtk_devicetts_postnet_cfg_update(&cfg->postnet);
    return 0;
}

int qtk_devicetts_cfg_update2(qtk_devicetts_cfg_t *cfg, wtk_source_loader_t *sl)
{
	qtk_tts_parse_cfg_update2(&(cfg->parse_cfg), sl);
    qtk_tts_dfsmn_cfg_update(&cfg->encoder);
    qtk_devicetts_duration_predictor_cfg_update(&cfg->dur);
    qtk_devicetts_decoder_cfg_update(&cfg->dec);
    qtk_devicetts_postnet_cfg_update(&cfg->postnet);
    return 0;
}
