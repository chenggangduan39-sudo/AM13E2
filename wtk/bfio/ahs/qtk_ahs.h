#ifndef G_8E71E2E016EA4729B67DE36409507247
#define G_8E71E2E016EA4729B67DE36409507247

#include "qtk/nnrt/qtk_nnrt.h"
#include "wtk/bfio/ahs/qtk_ahs_cfg.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/fft/wtk_rfft.h"
#include "wtk/bfio/qform/wtk_qmmse.h"
#include "wtk/bfio/ahs/qtk_kalmanv3.h"
#include "wtk/bfio/ahs/qtk_wiener.h"
#include "wtk/bfio/ahs/qtk_crnnoise_nnrt.h"
#include "wtk/bfio/ahs/qtk_crnnoise_nnrtv2.h"
#include "wtk/bfio/agc/qtk_gain_controller.h"
#include "wtk/bfio/ahs/estimate/qtk_linear_conv.h"
#include "wtk/os/wtk_thread.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_lockhoard.h"
#include "wtk/core/wtk_vpool2.h"
#include "wtk/bfio/ahs/qtk_freq_shift.h"
#include "wtk/bfio/ahs/qtk_erb.h"
#include "wtk/bfio/masknet/wtk_bbonenet.h"
#include "wtk/asr/vad/kvad/wtk_kvad.h"
typedef struct qtk_ahs qtk_ahs_t;
typedef int (*qtk_ahs_notify_f)(void *upval, short *out, int len);

typedef enum
{
    QTK_AHS_EVT_FEED,
    QTK_AHS_EVT_END
}qtk_ahs_evt_type_t;

typedef struct
{
    wtk_queue_node_t hoard_n;
    wtk_queue_node_t q_n;
    wtk_complex_t x[129];
    wtk_complex_t z[129];
    qtk_ahs_evt_type_t type;
    int index;
}qtk_ahs_evt_t;

typedef struct
{
    wtk_complex_t x[129];
    float a[129];
    float b[129];
    wtk_queue_node_t q_n;
}qtk_ahs_mt_value_t;

struct qtk_ahs {
    qtk_ahs_cfg_t *cfg;
    qtk_nnrt_t *nnrt;
    wtk_strbuf_t *mic;
    wtk_strbuf_t *sp;
    wtk_strbuf_t *dcache;
    wtk_strbuf_t *kalman2_sp;
    int st_idx;
    int stop_idx;

    float *frame_yt;
    float *frame_rt;
    float *frame_yt_tmp;
    float *frame_rt_tmp;
    float *frame_kalman_tmp;
    float *yt_pad_tmp;
    float *rt_pad_tmp;
    float *F;
    float *analysis_mem;
    float *analysis_mem_sp;
    float *synthesis_mem;
    float *synthesis_mem2;// for wiener when use_mask_sp = 0;
    wtk_drft_t *drft;
    wtk_rfft_t *fft;
    wtk_complex_t *est_freq;
    float *out;
    float *x;
    float *y;
    float *mix_spec;
    int pos;
    int chunk_pos;
    qtk_nnrt_value_t cache_h0;
    qtk_nnrt_value_t cache_c0;
    qtk_nnrt_value_t cache[12];
    void *upval;
    qtk_ahs_notify_f notify;
    wtk_qmmse_t *qmmse;
    wtk_qmmse_t *qmmse2;
    wtk_complex_t *fftx;

    qtk_ahs_kalman_t* kalman;
    qtk_ahs_kalman_t* kalman2;
    qtk_crnnoise_nnrt_t* kalman_nnrt;
    qtk_crnnoise_nnrt2_t* kalman_nnrt2;
    wtk_complex_t *km_x;
    wtk_complex_t *km_y;
    wtk_complex_t *km_z;

    qtk_ahs_wiener_t* wiener;

    qtk_ahs_gain_controller_t *gc;

    wtk_equalizer_t *eq;

    qtk_linear_conv_t *conv;
    short *wav_out;
    short *sp_in;

    qtk_linear_conv_t *conv2;//for conv loop

    wtk_thread_t thread;
    wtk_lockhoard_t evt_hoard;
    wtk_blockqueue_t nn_input_q;
    wtk_sem_t end_wait_sem;
    wtk_queue_t mt_q;
    wtk_vpool2_t *pool;
    int run;

    qtk_ahs_freq_shift_t *freq_shift;

    qtk_ahs_erb_t *erb;
    wtk_bbonenet_t *gainnet;
    wtk_kvad_t *vad;
    float *erb_mic;
    float *gainnet_in;
    float *gainnet_out;
    int gainnet_indim;

    int delay;
    int loop_delay;

    int t_vad;
    int a_vad;
    int vad_idx;

    float vad_probs[40];
    int in_offset;
    int out_offset;

    unsigned start : 1;
    unsigned start2 : 1;

    float echo_shift;
    float mic_shift;

    float *eq_gain;
};

qtk_ahs_t *qtk_ahs_new(qtk_ahs_cfg_t *cfg);
void qtk_ahs_delete(qtk_ahs_t *a);
void qtk_ahs_start(qtk_ahs_t *a);
void qtk_ahs_reset(qtk_ahs_t *a);
int qtk_ahs_feed_float(qtk_ahs_t *a, float *yt, float *rt, int len);
int qtk_ahs_feed_short(qtk_ahs_t *a, short *yt, short *rt, int len);
void qtk_ahs_feed(qtk_ahs_t *a, short *data, int len, int is_end);
void qtk_ahs_set_notify(qtk_ahs_t *a, void *upval, qtk_ahs_notify_f notify);
void qtk_ahs_set_ref_compensation(qtk_ahs_t *a,float *rir, int len, float rt60);
#endif
