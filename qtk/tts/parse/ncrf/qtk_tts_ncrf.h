#ifndef __QTK_TTS_NCRF_H__
#define __QTK_TTS_NCRF_H__

#include "qtk_tts_ncrf_cfg.h"
#include "wtk/core/math/wtk_mat.h"
#include "tts-mer/wtk-extend/nn/wtk_nn.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct {
    qtk_tts_ncrf_cfg_t *cfg;
    wtk_heap_t *uheap;
    //charrep
    wtk_matf_t *char_embedding;
    wtk_matf_t **feature_embedding;
    wtk_matf_t **linear_weight;
    wtk_vecf_t **linear_bias;
    //pw
    wtk_nn_lstm_t **lstm_pw;
    wtk_matf_t *linear_pw_weight;
    wtk_vecf_t *linear_pw_bias;
    wtk_matf_t *crf_pw_transitions;
    wtk_matf_t *pw_pred_embedding;
    //pp
    wtk_matf_t *pp_fnn_weight;
    wtk_vecf_t *pp_fnn_bias;
    wtk_nn_lstm_t **lstm_pp;
    wtk_matf_t *linear_pp_weight;
    wtk_vecf_t *linear_pp_bias;
    wtk_matf_t *crf_pp_transitions;
    wtk_matf_t *pp_pred_embedding;
    //ip
    wtk_matf_t *ip_fnn_weight;
    wtk_vecf_t *ip_fnn_bias;
    wtk_nn_lstm_t **lstm_ip;
    wtk_matf_t *linear_ip_weight;
    wtk_vecf_t *linear_ip_bias;
    wtk_matf_t *crf_ip_transitions;
}qtk_tts_ncrf_t;


qtk_tts_ncrf_t* qtk_tts_ncrf_new(qtk_tts_ncrf_cfg_t *cfg);
qtk_tts_ncrf_t* qtk_tts_ncrf_new2(qtk_tts_ncrf_cfg_t *cfg, wtk_rbin2_t *rbin);
int qtk_tts_ncrf_delete(qtk_tts_ncrf_t *ncrf);
int qtk_tts_ncrf_process(qtk_tts_ncrf_t *ncrf,wtk_mati_t *in,wtk_veci_t *out);

#ifdef __clpusplus
};
#endif

#endif
