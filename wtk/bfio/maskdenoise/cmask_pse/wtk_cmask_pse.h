#ifndef WTK_BFIO_MASKDENOISE_CMASK_PSE_WTK_CMASK_PSE_H
#define WTK_BFIO_MASKDENOISE_CMASK_PSE_WTK_CMASK_PSE_H
#include "wtk/bfio/maskdenoise/cmask_pse/wtk_cmask_pse_cfg.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/bfio/qform/wtk_qmmse.h"
#ifdef ONNX_DEC
#include "wtk/asr/fextra/onnxruntime/qtk_onnxruntime.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_cmask_pse wtk_cmask_pse_t;
typedef void(*wtk_cmask_pse_notify_f)(void *ths,short *output,int len);
typedef void(*wtk_cmask_pse_notify_f2)(void *ths,float *output,int len);

struct wtk_cmask_pse {
    wtk_cmask_pse_cfg_t *cfg;

	wtk_strbuf_t **mic;
	wtk_strbuf_t **sp;

    float *analysis_window;
    float *synthesis_window;
	wtk_drft_t *rfft;
	float *rfft_in;
	int nbin;
    float **analysis_mem;
    float **analysis_mem_sp;
	float *synthesis_mem;

	wtk_complex_t **fft;
	wtk_complex_t **fft_sp;
	wtk_complex_t *fftx;
	wtk_complex_t *ffty;

	float *out;

	void *ths;
	wtk_cmask_pse_notify_f notify;

	void *ths2;
	wtk_cmask_pse_notify_f2 notify2;
    void *ths3;
	wtk_cmask_pse_notify_f2 notify3;
#ifdef ONNX_DEC
    qtk_onnxruntime_t *emb;
    qtk_onnxruntime_t *pse;
	OrtValue **emb_caches;
	OrtValue **pse_caches;
    int *emb_out_len;
    int *pse_out_len;
#endif
    wtk_fbank_t *fbank;
    wtk_strbuf_t *fbank_buf;
    wtk_strbuf_t *vp_real_buf;
    wtk_strbuf_t *vp_imag_buf;
    float *fbank_mean;
    int fbank_len;
    int fbank_frame;

    float *emb0;
    float *emb1;
    float *emb2;
    float *emb3;
    float *gamma;
    float *beta;
    float *gamma1;
    float *beta1;
    float *gamma2;
    float *beta2;
    float *emb_mask;
    float *feat;

    int feat_len;
    int emb_mask_len;

    float *pse_in;
    float *pse_out;

    wtk_complex_t *c_onnx_out;
    wtk_complex_t *c_onnx_err;
    wtk_complex_t *c_onnx_raw;
    wtk_complex_t *c_onnx_echo;
    int c_onnx_len;
    int feed_frame;
    int frame_pos;

    float *x_phase;

    int nframe;

    wtk_rls3_t *erls3;
	wtk_covm_t *covm;
    wtk_covm_t *echo_covm;
	wtk_bf_t *bf;
    wtk_qmmse_t *qmmse;
    wtk_equalizer_t *eq;

    wtk_cmask_sv_t *sv;
    wtk_strbuf_t* cache_out_wav;
    float *sv_base;
    float *sv_cur;
    int sv_len;
    int last_flag;
    int flag;
    int flag_mask;
    int64_t check_cnt;
    int cache_idx;
    int res_len;
    float *stable_mask;
    float *mask1;
    float *mask2;
    int state_spk;
    int counter_above_low;
    int counter_below_low;
    int counter_below_high;

    int sp_silcnt;
    int mic_silcnt;
    int entropy_silcnt;
    int entropy_in_cnt;
    int entropy_sp_silcnt;
    int entropy_sp_in_cnt;
    float *power_k;

    int eng_cnt;
    int eng_times;

    float bs_scale;
    float bs_last_scale;
    int bs_max_cnt;

    unsigned sp_sil:1;
    unsigned mic_sil:1;
    unsigned entropy_sil:1;
    unsigned entropy_sp_sil:1;

    float *ee;

    float *entropy_E;
    float *entropy_Eb;
};

wtk_cmask_pse_t *wtk_cmask_pse_new(wtk_cmask_pse_cfg_t *cfg);
void wtk_cmask_pse_delete(wtk_cmask_pse_t *cmask_pse);
void wtk_cmask_pse_start(wtk_cmask_pse_t *cmask_pse);
void wtk_cmask_pse_reset(wtk_cmask_pse_t *cmask_pse);
void wtk_cmask_pse_feed(wtk_cmask_pse_t *cmask_pse, short *data, int len, int is_end);
void wtk_cmask_pse_set_notify(wtk_cmask_pse_t *cmask_pse,void *ths,wtk_cmask_pse_notify_f notify);

void wtk_cmask_pse_start_vp_feat(wtk_cmask_pse_t *cmask_pse, float *feat, int len);

void wtk_cmask_pse_new_vp(wtk_cmask_pse_t *cmask_pse);
void wtk_cmask_pse_delete_vp(wtk_cmask_pse_t *cmask_pse);
void wtk_cmask_pse_start_vp(wtk_cmask_pse_t *cmask_pse);
void wtk_cmask_pse_reset_vp(wtk_cmask_pse_t *cmask_pse);
void wtk_cmask_pse_feed_vp(wtk_cmask_pse_t *cmask_pse, short *data, int len, int is_end);
void wtk_cmask_pse_set_notify2(wtk_cmask_pse_t *cmask_pse,void *ths,wtk_cmask_pse_notify_f2 notify);
#ifdef __cplusplus
};
#endif
#endif
