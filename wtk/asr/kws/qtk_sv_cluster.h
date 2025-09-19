#ifndef __QTK_SV_CLUSTER_H__
#define __QTK_SV_CLUSTER_H__

#include "qtk_sv_cluster_cfg.h"
#include "wtk/asr/fextra/nnet3/wtk_nnet3_xvector_compute.h"
#include "wtk/asr/vad/kvad/wtk_kvad.h"
#include "wtk/core/wtk_queue.h"
#include "wtk/asr/fextra/ivector/wtk_ivector_plda_scoring.h"
#include "wtk/core/wtk_wavfile.h"
#ifdef __cplusplus
extern "C"{
#endif

typedef void (qtk_sv_cluster_notify_f)(void *svprintc, int id, float st, float ed);

typedef struct {
	wtk_queue_node_t node;
	wtk_vecf_t *vec;
	int start;
	int end;
	int index;
}sv_node_t;

typedef struct {
	wtk_queue_t queue;
	wtk_vecf_t *mean;
	int cnt;
}sv_cluster_t;

typedef struct {
	qtk_sv_cluster_cfg_t *cfg;

    wtk_nnet3_xvector_compute_t *x;
    wtk_kvad_t *vad;
	qtk_torchnn_t *svprint_nn;
    wtk_ivector_plda_scoring_t *plda_scoring;
    // wtk_queue_t vad_out;
    wtk_strbuf_t *speech_tmp_buf;
    sv_cluster_t **clusters;

    void *user_data;
    qtk_sv_cluster_notify_f *notify;
    wtk_heap_t *heap;

    int vad_state;
    int nvec;

    int ncluster;
    int index;
    int maxn;//max cluster

    //wtk_wavfile_t *log;
}qtk_sv_cluster_t;

qtk_sv_cluster_t *qtk_sv_cluster_new(qtk_sv_cluster_cfg_t *cfg);
int qtk_sv_cluster_delete(qtk_sv_cluster_t*);
int qtk_sv_cluster_start(qtk_sv_cluster_t*);
int qtk_sv_cluster_feed(qtk_sv_cluster_t *svprint,short *data, int len, int is_end);
int qtk_sv_cluster_feed2(qtk_sv_cluster_t *svprintc,short *data, int len, int is_end);
void qtk_sv_cluster_set_notify(qtk_sv_cluster_t *svprint,void *user_data,qtk_sv_cluster_notify_f *notify);
void qtk_sv_cluster_reset(qtk_sv_cluster_t *svprint);
float qtk_sv_cluster_get_likehood(qtk_sv_cluster_t *svprint,float *data1,int len,int cnt, float *data2,int len2);
// int qtk_svprint_test(qtk_svprintc_t *svprintc,float *f,int in_col,int in_row,int is_end);
#ifdef __cplusplus
};
#endif

#endif
