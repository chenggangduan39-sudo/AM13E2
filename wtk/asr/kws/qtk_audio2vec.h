#ifndef __WTK_ASR_KWS_QTK_AUDIO2VEC_H__
#define __WTK_ASR_KWS_QTK_AUDIO2VEC_H__
#pragma once
#include "wtk/asr/kws/qtk_audio2vec_cfg.h"
#include "wtk/asr/fextra/nnet3/wtk_nnet3_xvector_compute.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_audio2vec qtk_audio2vec_t;

struct qtk_audio2vec {
    qtk_audio2vec_cfg_t *cfg;
    wtk_svprint_t *svprint;
    wtk_kvad_t *vad;
    wtk_strbuf_t *wav_buf;
};

qtk_audio2vec_t *qtk_audio2vec_new(qtk_audio2vec_cfg_t *cfg);
void qtk_audio2vec_start(qtk_audio2vec_t *a);
void qtk_audio2vec_delete(qtk_audio2vec_t *a);
int qtk_audio2vec_feed(qtk_audio2vec_t *a, short *audio, int len);
int qtk_audio2vec_feed_end(qtk_audio2vec_t *a);
void qtk_audio2vec_get_result(qtk_audio2vec_t *a, float **vec, int *len);
void qtk_audio2vec_reset(qtk_audio2vec_t *a);
float qtk_audio2vec_eval(qtk_audio2vec_t *a, float *sv1, float *sv2, int len);
#ifdef __cplusplus
};
#endif
#endif
