#ifndef ACADB119_A37B_4B89_9689_145D6B917D22
#define ACADB119_A37B_4B89_9689_145D6B917D22

#include "qtk/mdl/qtk_sonicnet_cfg.h"
#include "qtk/nnrt/qtk_nnrt.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/wtk_robin.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_thread.h"
#include "wtk/core/fft/wtk_cfft.h"

typedef struct qtk_sonicnet qtk_sonicnet_t;

struct qtk_sonicnet {
    qtk_sonicnet_cfg_t *cfg;
    qtk_nnrt_t *rt;
    qtk_nnrt_t *vad_rt;
    int vad_feat_dim;
    int vad_attn_dim;
    uint32_t vad_utt_frame[2];
    float vad_exist_prob;
    wtk_robin_t *ctx;
    wtk_robin_t *chunks;
    wtk_heap_t *heap;
    wtk_complex_t *cir_tmp;
    wtk_complex_t *dyn_cir;
    wtk_cfft_t *fft;
    int nframe;
    int right_context;
    int *subsamples;
    int *in_channels;
    int nout;
    void **cache_value;
    float *vad_chunk;
    int vad_chunk_pos;
    wtk_thread_t vad_thread;
    wtk_blockqueue_t vad_in;
    wtk_lock_t vad_result_guard;
    unsigned left_padding : 1;
    unsigned cur_act_hint : 1;
};

qtk_sonicnet_t *qtk_sonicnet_new(qtk_sonicnet_cfg_t *cfg);
void qtk_sonicnet_delete(qtk_sonicnet_t *sn);
int qtk_sonicnet_feed(qtk_sonicnet_t *sn, wtk_complex_t *cfr, int *nframe,
                      float **prob);
int qtk_sonicnet_feed_end(qtk_sonicnet_t *sn, int *nframe, float **prob);
int qtk_sonicnet_get_vad_result(qtk_sonicnet_t *sn, uint32_t *chunk_s,
                                uint32_t *chunk_e, float *prob);

#endif /* ACADB119_A37B_4B89_9689_145D6B917D22 */
