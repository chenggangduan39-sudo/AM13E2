#ifndef WTK_BFIO_AEC_CMASK_AEC_WTK_CMASK_AEC2
#define WTK_BFIO_AEC_CMASK_AEC_WTK_CMASK_AEC2
#include "wtk/bfio/aec/cmask_aec/wtk_cmask_aec2_cfg.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_strbuf.h"
#ifdef ONNX_DEC
#include "wtk/asr/fextra/onnxruntime/qtk_onnxruntime.h"
#endif
#include "qtk/nnrt/qtk_nnrt.h"
#include "wtk/core/wtk_heap.h"
#include "wtk/bfio/preemph/wtk_preemph.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_cmask_aec2 wtk_cmask_aec2_t;
typedef enum wtk_cmask_aec2_state wtk_cmask_aec2_state_e;
typedef enum wtk_cmask_aec2_event_type wtk_cmask_aec2_event_type_e;
typedef void(*wtk_cmask_aec2_notify_f)(void *ths,short *output,int len);
typedef void (*wtk_cmask_aec2_notify_ssl_f)(void *ths, float ts, float te,wtk_ssl2_extp_t *nbest_extp,int nbest);

struct wtk_cmask_aec2 {
    wtk_cmask_aec2_cfg_t *cfg;

	wtk_strbuf_t **mic;
	wtk_strbuf_t **sp;
    wtk_strbuf_t **cmic;

    float *analysis_window;
    float *synthesis_window;
	wtk_drft_t *rfft;
	float *rfft_in;
	int nbin;
    float **analysis_mem;
    float **analysis_mem_sp;
	float *synthesis_mem;

	wtk_complex_t **fft;
	wtk_complex_t *f_fft;
	wtk_complex_t **fft_sp;
	wtk_complex_t **ibf_fft;

	wtk_rls_t *erls;
    wtk_rls3_t *erls3;
    wtk_nlms_t *enlms;

	wtk_covm_t *covm;
    wtk_covm_t *echo_covm;
	wtk_covm_t *icovm;
	wtk_bf_t *bf;
	wtk_bf_t *ibf;

    wtk_complex_t **ovec;

    wtk_complex_t *fftx;
    wtk_complex_t *ffty;

    wtk_qmmse_t *qmmse;
    wtk_qmmse_t *qmmse2;
    qtk_ahs_gain_controller_t *gc;

    wtk_maskssl2_t *maskssl2;
    wtk_maskssl2_t *ibf_ssl;
    wtk_complex_t ***raw_fft;
    float *mask;

	float *out;

    wtk_equalizer_t *eq;

	void *ths;
	wtk_cmask_aec2_notify_f notify;

    void *ssl_ths;
	wtk_cmask_aec2_notify_ssl_f notify_ssl;

    int sp_silcnt;
    int mic_silcnt;
    int wpe_silcnt;
    int entropy_silcnt;
    int entropy_in_cnt;
    int entropy_sp_silcnt;
    int entropy_sp_in_cnt;
    float *power_k;

    int eng_cnt;
    int sp_eng_cnt;
    int eng_times;
    int sp_eng_times;

    float bs_scale;
    float bs_last_scale;
    float bs_real_scale;
    int bs_max_cnt;
    float *bs_win;

    int gc_cnt;
    short *audio_frame;
    int pos;

    unsigned sp_sil:1;
    unsigned mic_sil:1;
    unsigned wpe_sil:1;
    unsigned entropy_sil:1;
    unsigned entropy_sp_sil:1;

#ifdef ONNX_DEC
    qtk_onnxruntime_t *onnx;
	OrtValue **cache;
#endif
    float *errr;
    float *ee;
    wtk_complex_t *c_onnx_out;
    wtk_complex_t *c_onnx_err;
    wtk_complex_t *c_onnx_raw;
    wtk_complex_t *c_onnx_echo;
    wtk_complex_t *c_onnx_gf;
    float *gf;
    int c_onnx_len;
    float *leak;
    int feed_frame;
    int frame_pos;

    float *x_phase;

    qtk_nnrt_t *rt;
    void **cache_value;
    char **in_name;
    char **out_name;
    int num_in;
    int num_out;
    int num_cache;
    int nframe;
    int *in_index;
    int *out_index;
    wtk_heap_t *heap;

    qtk_nnrt_t *dereb_rt;
    void **dereb_cache_value;
    char **dereb_in_name;
    char **dereb_out_name;
    int dereb_num_in;
    int dereb_num_out;
    int dereb_num_cache;
    int dereb_nframe;
    int *dereb_in_index;
    int *dereb_out_index;
    wtk_heap_t *dereb_heap;


    float *dereb_x_mag;
    float *dereb_x_phase;

    wtk_strbuf_t *Y_tilde;
    float **numerator;
    float *denominator;
    float **K;
    float ***inv_R_WPE;
    float **G_WPE;
    float *dereb_tmp;
    float *change_eng;
    int *change_cnt;
    float *change_pre;

    float *change_error;

    int change_idx;
    int change_delay;
    float *change_eng2;

    float *entropy_E;
    float *entropy_Eb;
    qtk_nnrt_value_t input_val[32];
    qtk_nnrt_value_t output_val[32];
    int64_t shape[32][5];
    wtk_complex_t ffttmp[64];
    wtk_complex_t fftsp2[10];

    float theta;
    int *ibf_mic;
    float *mic_diff;
    int *ibf_idx;
    int need_ibf;

    float *hist_energy;
    float *gains;
    float *energy_buf;
    float *prev_gains;
    float *aux_hist;

    int mic_silcnt2;
    unsigned mic_sil2:1;
};

wtk_cmask_aec2_t *wtk_cmask_aec2_new(wtk_cmask_aec2_cfg_t *cfg);
void wtk_cmask_aec2_delete(wtk_cmask_aec2_t *cmask_aec2);
void wtk_cmask_aec2_start(wtk_cmask_aec2_t *cmask_aec2);
void wtk_cmask_aec2_reset(wtk_cmask_aec2_t *cmask_aec2);
void wtk_cmask_aec2_feed(wtk_cmask_aec2_t *cmask_aec2, short *data, int len, int is_end);
void wtk_cmask_aec2_set_notify(wtk_cmask_aec2_t *cmask_aec2,void *ths,wtk_cmask_aec2_notify_f notify);
void wtk_cmask_aec2_set_ssl_notify(wtk_cmask_aec2_t *cmask_aec2,void *ths,wtk_cmask_aec2_notify_ssl_f notify);
#ifdef __cplusplus
};
#endif
#endif
