#ifndef __QTK_TTS_TAC2_SYN_GMMDEC_H__
#define __QTK_TTS_TAC2_SYN_GMMDEC_H__

#include "qtk_tts_tac2_syn_gmmdec_cfg.h"
#include "tts-mer/wtk-extend/wtk_mat2.h"
#include "tts-mer/wtk-extend/nn/wtk_nn.h"
#ifdef __cplusplus
extern "C"{
#endif

typedef void(*qtk_tts_tac2_syn_gmmdec_notify_f)(void *user_data,wtk_matf_t *m,int is_end);

typedef struct{
    qtk_tts_tac2_syn_gmmdec_cfg_t *cfg;
    wtk_heap_t *heap;
    wtk_matf_t **prenet_weight;
    wtk_vecf_t **prenet_bias;
    wtk_nn_lstm_t *atten_rnn;
    wtk_matf_t *omega_delta_sigma;
    wtk_matf_t *atten_query_weight;
    wtk_vecf_t *atten_query_bias;
    wtk_matf_t *atten_v_weight;
    wtk_matf_t *query_p;
    wtk_matf_t *atten_delata;
    wtk_matf_t *atten_sigma;
    wtk_matf_t *mu_prev;
    wtk_matf_t *J;
    wtk_matf_t *atten_alpha;
    wtk_matf_t *linear_weight;
    wtk_vecf_t *linear_bias;
    wtk_nn_lstm_t **lstm_rnn;
    wtk_matf_t *proj_weight;

    void *user_data;
    qtk_tts_tac2_syn_gmmdec_notify_f notify;
}qtk_tts_tac2_syn_gmmdec_t;

qtk_tts_tac2_syn_gmmdec_t* qtk_tts_tac2_syn_gmmdec_new(qtk_tts_tac2_syn_gmmdec_cfg_t *cfg,wtk_heap_t *heap);
int qtk_tts_tac2_syn_gmmdec_delete(qtk_tts_tac2_syn_gmmdec_t *gmm_dec);
int qtk_tts_tac2_syn_gmmdec_process(qtk_tts_tac2_syn_gmmdec_t *gmm_dec,wtk_matf_t *encoder_out,int is_end);
void qtk_tts_tac2_syn_gmmdec_set_notify(qtk_tts_tac2_syn_gmmdec_t *gmm_dec,void *user_data,qtk_tts_tac2_syn_gmmdec_notify_f);

#ifdef __cplusplus
};
#endif

#endif
