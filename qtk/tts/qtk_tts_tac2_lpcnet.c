#include "qtk_tts_tac2_lpcnet.h"

void qtk_tts_tac2_lpcnet_on_tac2(qtk_tts_tac2_lpcnet_t *syn,wtk_matf_t *mels,int is_end);
void qtk_tts_tac2_lpcnet_on_lpcnet(qtk_tts_tac2_lpcnet_t *syn,float *data,int len,int is_end);

qtk_tts_tac2_lpcnet_t *qtk_tts_tac2_lpcnet_new(qtk_tts_tac2_lpcnet_cfg_t *cfg)
{
    qtk_tts_tac2_lpcnet_t *syn = wtk_malloc(sizeof(qtk_tts_tac2_lpcnet_t));
    syn->cfg = cfg;
    syn->tac2 = qtk_tts_tac2_syn_new(&cfg->tac2_cfg);
    qtk_tts_tac2_syn_set_notify(syn->tac2,syn,(qtk_tts_tac2_syn_notify_f)qtk_tts_tac2_lpcnet_on_tac2);
    wtk_tac_lpcnet_set_notify(&cfg->lpcnet_cfg,(wtk_lpcnet_notify_f)qtk_tts_tac2_lpcnet_on_lpcnet,syn);
    return syn;
}

int qtk_tts_tac2_lpcnet_set_notify(qtk_tts_tac2_lpcnet_t *syn,void *user_data,qtk_tts_tac2_lpcnet_notify_f notify)
{
    syn->user_data = user_data;
    syn->notify = notify;
    return 0;
}

int qtk_tts_tac2_lpcnet_process(qtk_tts_tac2_lpcnet_t *syn,wtk_veci_t *vec,int is_end)
{
    if(vec == NULL || vec->len <= 0){
        goto end;
    }
    qtk_tts_tac2_syn_process(syn->tac2,vec,is_end);
end:
    return 0;
}

int qtk_tts_tac2_lpcnet_delete(qtk_tts_tac2_lpcnet_t *syn)
{
    if(syn->tac2){
        qtk_tts_tac2_syn_delete(syn->tac2);
    }
    wtk_free(syn);
    return 0;
}

void qtk_tts_tac2_lpcnet_on_tac2(qtk_tts_tac2_lpcnet_t *syn,wtk_matf_t *mels,int is_end)
{
    wtk_tac_lpcnet_process(&syn->cfg->lpcnet_cfg,mels,NULL,is_end);
    return;
}

void qtk_tts_tac2_lpcnet_on_lpcnet(qtk_tts_tac2_lpcnet_t *syn,float *data,int len,int is_end)
{
    if(syn->notify == NULL)
        return;
    //float >> short data
    short *data_s = wtk_malloc(sizeof(short)*len);
    float maxf = 32766.0 / max(0.01, wtk_float_abs_max(data, len));
    int i = 0;

    for(i = 0; i < len; ++i){
        data_s[i] = data[i]*maxf;
    }

    syn->notify(syn->user_data,data_s,len,is_end);
    free(data_s);
    return;
}