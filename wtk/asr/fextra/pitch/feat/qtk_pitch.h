#ifndef __QTK_FEAT_QTK_PITCH_H__
#define __QTK_FEAT_QTK_PITCH_H__
#include "wtk/asr/fextra/pitch/audio/qtk_resample.h"
#include "wtk/asr/fextra/pitch/core/qtk_array.h"
#include "wtk/asr/fextra/pitch/feat/qtk_pitch_cfg.h"
#include "wtk/asr/fextra/pitch/feat/qtk_pitch_post.h"
#include "wtk/core/wtk_heap.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_pitch qtk_pitch_t;
typedef void (*qtk_pitch_notify_t)(int idx, float *pitch, void *ud);

struct qtk_pitch {
    qtk_pitch_cfg_t *cfg;

    wtk_heap_t *heap;
    wtk_heap_t *step_heap;

    qtk_linear_resample_t rs;
    qtk_arbitrary_resample_t nccf_resamp;
    qtk_pitch_post_t post;

    double signal_sumsq;
    double signal_sum;
    int64_t downsamp_processed;
    int nccf_first_lag;
    int nccf_last_lag;
    int frame_latency;

    int num_measured_lags;
    int full_frame_length;
    float *window;
    float *inner_prod;
    float *norm_prod;
    float *cur_forward_cost;
    double forward_cost_remainder;

    float *lags;
    float *lags_offset;
    float *forward_cost;
    float *local_cost;
    int lags_dim;
    int cur_frame;
    qtk_array_t *frame_info;
    qtk_array_t *nccf_info;
    qtk_array_t *downsamp_signal_remainder;
    qtk_array_t *tmp_downsamp_signal_remainder;

    qtk_array_t *lag_nccf;

    int64_t *index_info;

    unsigned input_finished : 1;

    void *ud;
    qtk_pitch_notify_t notify;

    int samp_per_chunk;
    int chunk_pos;
    float *chunk;
};

qtk_pitch_t *qtk_pitch_new(qtk_pitch_cfg_t *cfg);
int qtk_pitch_init(qtk_pitch_t *p, qtk_pitch_cfg_t *cfg);
void qtk_pitch_clean(qtk_pitch_t *p);
void qtk_pitch_delete(qtk_pitch_t *p);
int qtk_pitch_feed(qtk_pitch_t *p, float *data, int len);
int qtk_pitch_finish(qtk_pitch_t *p);
void qtk_pitch_reset(qtk_pitch_t *p);
void qtk_pitch_set_notify(qtk_pitch_t *p, qtk_pitch_notify_t notify, void *ud);
int qtk_pitch_get_feat_dim(qtk_pitch_t *p);

#ifdef __cplusplus
};
#endif
#endif
