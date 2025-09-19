#include "qtk_tts_tac.h"
void qtk_tts_tac_on_notify(qtk_tts_tac_t *tac,short *data,int len,int is_end);

qtk_tts_tac_t *qtk_tts_tac_new(qtk_tts_tac_cfg_t *cfg)
{
    qtk_tts_tac_t *tac = NULL;
    tac = wtk_calloc(1, sizeof(*tac));
    if(tac == NULL) goto end;
    tac->cfg = cfg;

    tac->parse = qtk_tts_parse_new(&(cfg->parse_cfg));
    if(cfg->use_durian_lpcnet){
        tac->durian_lpcnet = qtk_tts_durian_lpcnet_new(&cfg->durian_lpcnet_cfg);
    }else{
        tac->tac2_lpcnet = qtk_tts_tac2_lpcnet_new(&cfg->syn_cfg);
    }
    tac->wav = wtk_mer_wav_stream_new(sizeof(short)*512*1024,cfg->syn_cfg.sample_rate,5.0f);
    if(cfg->use_durian_lpcnet){
        qtk_tts_durian_lpcnet_set_notify(tac->durian_lpcnet,tac,(qtk_tts_durian_lpcnet_notify_f)qtk_tts_tac_on_notify);
    }else{
        qtk_tts_tac2_lpcnet_set_notify(tac->tac2_lpcnet,tac,(qtk_tts_tac2_lpcnet_notify_f)qtk_tts_tac_on_notify);
    }
    tac->user_data = NULL;
    tac->notify = NULL;
end:
    return tac;
}

int qtk_tts_tac_delete(qtk_tts_tac_t *tac)
{
    if(tac == NULL) goto end;
    if(tac->wav)
        wtk_mer_wav_stream_delete(tac->wav);
    if(tac->parse)
        qtk_tts_parse_delete(tac->parse);
    if(tac->durian_lpcnet)
        qtk_tts_durian_lpcnet_delete(tac->durian_lpcnet);
    if(tac->tac2_lpcnet)
        qtk_tts_tac2_lpcnet_delete(tac->tac2_lpcnet);
    wtk_free(tac);
end:
    return 0;
}

int qtk_tts_tac_reset(qtk_tts_tac_t *tac)
{
    qtk_tts_parse_reset(tac->parse);
    return 0;
}

int qtk_tts_tac_process(qtk_tts_tac_t *tac,char *txt,int len)
{
    int ret = -1;
    int i = 0;
    int nid = 0;
    qtk_tts_parse_process(tac->parse,txt,len);
    if(tac->parse->id_vec == NULL) goto end;
    nid = tac->parse->nid;
    for(i = 0; i < nid; ++i){
        if(tac->cfg->use_durian_lpcnet){
            qtk_tts_durian_lpcnet_process(tac->durian_lpcnet,tac->parse->id_vec[i],(i==nid-1)?1:0);
        }else{
            qtk_tts_tac2_lpcnet_process(tac->tac2_lpcnet,tac->parse->id_vec[i],(i==nid-1)?1:0);
        }
    }
    ret = 0;
end:
    return ret;
}

void qtk_tts_tac_set_notify(qtk_tts_tac_t *tac,void *user_data,qtk_tac_notify_f notify)
{
    tac->user_data = user_data;
    tac->notify = notify;
}

void qtk_tts_tac_on_notify(qtk_tts_tac_t *tac,short *data,int len,int is_end)
{
    if(tac->notify == NULL)
        return;

    tac->notify(tac->user_data,data,len,is_end);
    return;
}
