#ifndef WTK_BFIO_QFORM_CMASK_BFSE_WTK_QFORM_CMASK_BFSE_H
#define WTK_BFIO_QFORM_CMASK_BFSE_WTK_QFORM_CMASK_BFSE_H
#include "wtk/bfio/qform/cmask_bfse/wtk_cmask_bfse_cfg.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/bfio/qform/wtk_qmmse.h"
#include "wtk/core/wtk_fring.h"
#ifdef ONNX_DEC
#include "wtk/asr/fextra/onnxruntime/qtk_onnxruntime.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_cmask_bfse wtk_cmask_bfse_t;
typedef enum wtk_cmask_bfse_state wtk_cmask_bfse_state_e;
typedef enum wtk_cmask_bfse_event_type wtk_cmask_bfse_event_type_e;
typedef void(*wtk_cmask_bfse_notify_f)(void *ths,short *output,int len);

struct wtk_cmask_bfse {
    wtk_cmask_bfse_cfg_t *cfg;

	wtk_strbuf_t **mic;
	wtk_strbuf_t **sp;

    float *analysis_window;
    float *synthesis_window;
	wtk_drft_t *rfft;
	float *rfft_in;
	int nbin;
    float **analysis_mem;
    float **analysis_mem_sp;
	float **synthesis_mem;

	wtk_complex_t **fft;
	wtk_complex_t **fft_sp;
	wtk_complex_t **fftx;
    wtk_complex_t **ffttmp;

    float **n_out;
	float *out;

	void *ths;
	wtk_cmask_bfse_notify_f notify;

#ifdef ONNX_DEC
    qtk_onnxruntime_t *sep;
    qtk_onnxruntime_t *bfse;
	OrtValue **bfse_caches;
	OrtValue **sep_caches;
    int *bfse_out_len;
    int *sep_out_len;
#endif

	wtk_covm_t **covm;
	wtk_bf_t **bf;
    wtk_qmmse_t **qmmse;
    wtk_qmmse_t **qmmse2;
    wtk_equalizer_t **eq;
    wtk_aspec_t **aspec;

    float *bs_scale;
    float *bs_last_scale;
    int *bs_max_cnt;

    float *entropy_E;
    float *entropy_Eb;

    float **time_delay;
    wtk_complex_t ***phase_shift;
    float **mic_mag;
    float **LPS;
    wtk_complex_t **mic_covar;
    wtk_complex_t ***ideal_phase_covar;
    wtk_complex_t ***freq_covar;
    wtk_complex_t **freq_covar_sum;
    wtk_complex_t *mean_mat;
    wtk_complex_t **input_norm;
    float *feat;
    float *s_real;
    float *s_imag;
    float *n_real;
    float *n_imag;
    float *s_cov;
    float *n_cov;

    int *entropy_in_cnt;
    int *entropy_silcnt;
    int *entropy_sil;
    int *mic_silcnt;
    int *mic_sil;

    wtk_complex_t *cov;
    wtk_complex_t *inv_cov;
    wtk_dcomplex_t *tmp;
    float **cohv;
	wtk_complex_t **gcc_fft;
    wtk_fring_t **q_fring;

    float *specsum;
    int *right_nf;
    float *q_spec;
};

wtk_cmask_bfse_t *wtk_cmask_bfse_new(wtk_cmask_bfse_cfg_t *cfg);
void wtk_cmask_bfse_delete(wtk_cmask_bfse_t *cmask_bfse);
void wtk_cmask_bfse_start(wtk_cmask_bfse_t *cmask_bfse);
void wtk_cmask_bfse_reset(wtk_cmask_bfse_t *cmask_bfse);
void wtk_cmask_bfse_feed(wtk_cmask_bfse_t *cmask_bfse, short *data, int len, int is_end);
void wtk_cmask_bfse_set_notify(wtk_cmask_bfse_t *cmask_bfse,void *ths,wtk_cmask_bfse_notify_f notify);
#ifdef __cplusplus
};
#endif
#endif
