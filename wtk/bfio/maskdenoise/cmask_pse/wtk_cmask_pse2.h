#ifndef WTK_BFIO_MASKDENOISE_CMASK_PSE2_WTK_CMASK_PSE2_H
#define WTK_BFIO_MASKDENOISE_CMASK_PSE2_WTK_CMASK_PSE2_H
#include "qtk/nnrt/qtk_nnrt.h"
#include "qtk/nnrt/qtk_nnrt_value.h"
#include "wtk/bfio/maskdenoise/cmask_pse/wtk_cmask_pse2_cfg.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/wtk_strbuf.h"
#ifdef ONNX_DEC
#include "wtk/asr/fextra/onnxruntime/qtk_onnxruntime.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_cmask_pse2 wtk_cmask_pse2_t;
typedef void(*wtk_cmask_pse2_notify_f)(void *ths,short *output,int len);
typedef void(*wtk_cmask_pse2_notify_f2)(void *ths,float *output,int len);

struct wtk_cmask_pse2 {
    wtk_cmask_pse2_cfg_t *cfg;

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
	wtk_cmask_pse2_notify_f notify;
	void *ths2;
	wtk_cmask_pse2_notify_f2 notify2;

#ifdef ONNX_DEC
    qtk_onnxruntime_t *emb;
	OrtValue **emb_caches;
    int *emb_out_len;
#endif
    wtk_fbank_t *fbank;
    wtk_strbuf_t *fbank_buf;
    wtk_strbuf_t *vp_buf;
    float *fbank_mean;
    int fbank_len;
    int fbank_frame;

    float *emb_feat;
    float *gb_feat;
    float *feat;

    int feat_len;

    float *pse_in;
    float *mask;

    int feed_frame;
    int frame_pos;

    int nframe;

    int sp_silcnt;
    int mic_silcnt;

    float bs_scale;
    float bs_last_scale;
    int bs_max_cnt;
    qtk_nnrt_t *pse_rt;
    qtk_nnrt_value_t *pse_rt_input;
    void **pse_rt_input_data;
    size_t *ninput_elems;

    wtk_mask_bf_t *mask_bf;

    unsigned sp_sil:1;
    unsigned mic_sil:1;
};

wtk_cmask_pse2_t *wtk_cmask_pse2_new(wtk_cmask_pse2_cfg_t *cfg);
void wtk_cmask_pse2_delete(wtk_cmask_pse2_t *cmask_pse2);
void wtk_cmask_pse2_start(wtk_cmask_pse2_t *cmask_pse2);
void wtk_cmask_pse2_reset(wtk_cmask_pse2_t *cmask_pse2);
void wtk_cmask_pse2_feed(wtk_cmask_pse2_t *cmask_pse2, short *data, int len, int is_end);
void wtk_cmask_pse2_set_notify(wtk_cmask_pse2_t *cmask_pse2,void *ths,wtk_cmask_pse2_notify_f notify);

void wtk_cmask_pse2_start_vp_feat(wtk_cmask_pse2_t *cmask_pse2, float *feat, int len);

void wtk_cmask_pse2_new_vp(wtk_cmask_pse2_t *cmask_pse2);
void wtk_cmask_pse2_delete_vp(wtk_cmask_pse2_t *cmask_pse2);
void wtk_cmask_pse2_start_vp(wtk_cmask_pse2_t *cmask_pse2);
void wtk_cmask_pse2_reset_vp(wtk_cmask_pse2_t *cmask_pse2);
void wtk_cmask_pse2_feed_vp(wtk_cmask_pse2_t *cmask_pse2, short *data, int len, int is_end);
void wtk_cmask_pse2_set_notify2(wtk_cmask_pse2_t *cmask_pse2,void *ths,wtk_cmask_pse2_notify_f2 notify);

#ifdef __cplusplus
};
#endif
#endif
