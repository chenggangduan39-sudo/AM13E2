#ifndef __QTK_TTS_TAC2_SYN_H__
#define __QTK_TTS_TAC2_SYN_H__

#include "qtk_tts_tac2_syn_cfg.h"
#include "tts-mer/wtk-extend/wtk_mat2.h"
#include "tts-mer/wtk-extend/nn/wtk_nn.h"
#include "acoustic/tac2_syn/qtk_tts_tac2_syn_enc.h"
#include "dfsmn/qtk_tts_dfsmn.h"
#include "qtk_tts_tac2_syn_dec.h"
#include "qtk_tts_tac2_syn_gmmdec.h"

typedef void(*qtk_tts_tac2_syn_notify_f)(void *user_data,wtk_matf_t *m,int is_end);

typedef struct qtk_tts_tac2_syn{
    qtk_tts_tac2_syn_cfg_t *cfg;
    wtk_heap_t *heap;
    wtk_matf_t *embedding;
    //enc 和 dfsmn enc
    qtk_tts_tac2_syn_enc_t *enc;
    qtk_tts_dfsmn_t *dfsmn_enc;
    //dec 和 gmmdec 注意力机制不一样
    qtk_tts_tac2_syn_dec_t *dec;
    qtk_tts_tac2_syn_gmmdec_t *gmm_dec;
    //postnet
    wtk_matf_t **postnet_conv_kernel;
    wtk_vecf_t **postnet_conv_bias;
    wtk_vecf_t **postnet_gamma;
    wtk_vecf_t **postnet_beta;
    wtk_vecf_t **postnet_mean;
    wtk_vecf_t **postnet_var;

    struct timeval time_s;

    void *user_data;
    qtk_tts_tac2_syn_notify_f notify;
}qtk_tts_tac2_syn_t;

qtk_tts_tac2_syn_t *qtk_tts_tac2_syn_new(qtk_tts_tac2_syn_cfg_t *cfg);
int qtk_tts_tac2_syn_delete(qtk_tts_tac2_syn_t *);
void qtk_tts_tac2_syn_set_notify(qtk_tts_tac2_syn_t *syn,void *user_data,qtk_tts_tac2_syn_notify_f cb);
int qtk_tts_tac2_syn_process(qtk_tts_tac2_syn_t *syn,wtk_veci_t *vec,int is_end);

#endif
