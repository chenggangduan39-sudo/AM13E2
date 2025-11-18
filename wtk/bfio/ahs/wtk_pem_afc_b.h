#ifndef WTK_BFIO_AHS_WTK_PEM_AFC_B
#define WTK_BFIO_AHS_WTK_PEM_AFC_B
#include "wtk/bfio/ahs/wtk_pem_afc_b_cfg.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/core/fft/pffft.h"
#ifdef ONNX_DEC
#include "wtk/asr/fextra/onnxruntime/qtk_onnxruntime.h"
#endif
#include "qtk/nnrt/qtk_nnrt.h"
#include "wtk/core/wtk_heap.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_pem_afc_b wtk_pem_afc_b_t;
typedef struct wtk_pem_afc_b_conv1d wtk_pem_afc_b_conv1d_t;

typedef void(*wtk_pem_afc_b_notify_f)(void *ths,short *output,int len);

struct wtk_pem_afc_b_conv1d{
    float *cache;
    float *weight;
    float *out;
    int l1;
    int l2;
    int b;
};

struct wtk_pem_afc_b {
    wtk_pem_afc_b_cfg_t *cfg;

    wtk_strbuf_t *mic;
	wtk_strbuf_t *sp;

    PFFFT_Setup *pffft1;
    PFFFT_Setup *pffft2;
    wtk_drft_t *drft3;
    PFFFT_Setup *pffft4;
    PFFFT_Setup *pffft5;
    float *analysis_window;
    float *synthesis_window;
    float *analysis_mem;
	float *synthesis_mem;

    int f1_L1;
    int f1_L2;
    int f2_L1;
    int f2_L2;
    int m_L1;
    int m_L2;
    int u_L1;
    int u_L2;

    int fft1_len;
    int fft2_len;
    int fft3_len;
    int fft4_len;
    int fft5_len;

    float *ar_a;
    float *ar_R;
    float *ar_rou;
    float *ar_w;
    float *afc_w;

    float *filtering_conv1_w;
    float *filtering_conv2_w;
    float *whiten_m_conv_w;
    float *whiten_u_conv_w;

    int filtering_conv1_w_len;
    int filtering_conv2_w_len;
    int whiten_m_conv_w_len;
    int whiten_u_conv_w_len;

    float *filtering_conv1_cache;
    float *filtering_conv2_cache;
    float *whiten_m_conv_cache;
    float *whiten_u_conv_cache;

    int T_buffer_size;
    int u_buffer_size;
    int m_buffer_size;
    int T_buffer_len;
    int u_buffer_len;
    int m_buffer_len;
    float *Truncation_indicator_buffer;
    float *u_whiten_buffer;
    float *m_whiten_buffer;

    float *prev_m;
    float *prev_u;

    float *power;

    float *out;

    float *error;
    float *error_soft_clipped;
    float *Truncation_indicator;
    float *m_whiten;
    float *u_whiten;

    float *Yk;
    float *Xk;

    float *tmp;
    float *fft_tmp;
    float *ifft_tmp;

    int nframe;

    wtk_pem_afc_b_notify_f notify;
    void *ths;
    wtk_equalizer_t *eq;

#ifdef ONNX_DEC
    qtk_onnxruntime_t *onnx;
	OrtValue **cache;
#endif
	wtk_strbuf_t *pem;

    int nbin;
    wtk_drft_t *drft6;
    float *analysis_mem_mic;
    float *analysis_mem_sp;
    float *analysis_mem_pem;
	float *synthesis_mem2;

    wtk_complex_t *mic_fft;
    wtk_complex_t *sp_fft;
    wtk_complex_t *pem_fft;
    wtk_complex_t *fftx;

    float *mag_Y;
    float *pha_Y;
    float *mag_R1;
    float *pha_R1;
    float *mag_R2;
    float *pha_R2;

    qtk_nnrt_t *rt;
    void **cache_value;
    char **in_name;
    char **out_name;
    int num_in;
    int num_out;
    int num_cache;
    int ncnn_nframe;
    int *in_index;
    int *out_index;
    wtk_heap_t *heap;

    qtk_rir_estimate_t *r_est;
    int sweep_ok;
    int recommend_delay;
};

wtk_pem_afc_b_t *wtk_pem_afc_b_new(wtk_pem_afc_b_cfg_t *cfg);
void wtk_pem_afc_b_delete(wtk_pem_afc_b_t *pem_afc_b);
void wtk_pem_afc_b_start(wtk_pem_afc_b_t *pem_afc_b);
void wtk_pem_afc_b_reset(wtk_pem_afc_b_t *pem_afc_b);
void wtk_pem_afc_b_set_notify(wtk_pem_afc_b_t *pem_afc_b,void *ths,wtk_pem_afc_b_notify_f notify);
void wtk_pem_afc_b_feed(wtk_pem_afc_b_t *pem_afc_b, short *data, int len, int is_end);
void wtk_pem_afc_b_feed_float(wtk_pem_afc_b_t *pem_afc_b, float *data, int len, int is_end);
int wtk_pem_afc_b_ref_write(wtk_pem_afc_b_t* pem, char *fn);
int wtk_pem_afc_b_ref_load(wtk_pem_afc_b_t* pem, char *fn);
#ifdef __cplusplus
};
#endif
#endif