#ifndef __QTK_SVPRINTC_H__
#define __QTK_SVPRINTC_H__

#include "qtk_svprintc_cfg.h"
#include "wtk/asr/fextra/nnet3/wtk_nnet3_xvector_compute.h"
#include "wtk/asr/vad/kvad/wtk_kvad.h"
#include "wtk/core/wtk_queue.h"
#include "wtk/asr/fextra/ivector/wtk_ivector_plda_scoring.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef void (qtk_svprintc_notify_f)(void *svprintc, float *f, int len);

typedef struct qtk_svprintc{
    qtk_svprintc_cfg_t *cfg;

    wtk_nnet3_xvector_compute_t *x;
    wtk_kvad_t *vad;
	qtk_torchnn_t *svprint_nn;
    wtk_ivector_plda_scoring_t *plda_scoring;
    // wtk_queue_t vad_out;

    wtk_strbuf_t *speech_tmp_buf;

    void *user_data;
    qtk_svprintc_notify_f *notify;

    int vad_state;
}qtk_svprintc_t;

qtk_svprintc_t *qtk_svprintc_new(qtk_svprintc_cfg_t *cfg);
int qtk_svprintc_delete(qtk_svprintc_t*);
int qtk_svprintc_start(qtk_svprintc_t*);
int qtk_svprintc_feed(qtk_svprintc_t *svprint,short *data, int len, int is_end);
int qtk_svprintc_feed2(qtk_svprintc_t *svprintc,short *data, int len, int is_end);
void qtk_svprintc_set_notify(qtk_svprintc_t *svprint,void *user_data,qtk_svprintc_notify_f *notify);
void qtk_svprintc_reset(qtk_svprintc_t *svprint);
float qtk_svprintc_get_likehood(qtk_svprintc_t *svprint,float *data1,int len,int cnt, float *data2,int len2);
// int qtk_svprint_test(qtk_svprintc_t *svprintc,float *f,int in_col,int in_row,int is_end);
#ifdef __cplusplus
};
#endif

#endif