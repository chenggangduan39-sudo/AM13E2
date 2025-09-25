#ifndef WTK_BFIO_MASKDENOISE_CMASK_SV_WTK_CMASK_SV_H
#define WTK_BFIO_MASKDENOISE_CMASK_SV_WTK_CMASK_SV_H
#include "wtk/bfio/maskdenoise/cmask_pse/wtk_cmask_sv_cfg.h"
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

typedef struct wtk_cmask_sv wtk_cmask_sv_t;
typedef void(*wtk_cmask_sv_notify_f)(void *ths,short *output,int len);

struct wtk_cmask_sv {
    wtk_cmask_sv_cfg_t *cfg;

	void *ths;
	wtk_cmask_sv_notify_f notify;

#ifdef ONNX_DEC
    qtk_onnxruntime_t *emb;
#endif
    wtk_fbank_t *fbank;
    wtk_strbuf_t *fbank_buf;
    wtk_strbuf_t *wav_buf;
    float *fbank_mean;
    float *output;
    int output_len;
    int fbank_len;
    int fbank_frame;

    int normalize;
    int enroll_cycle;
};

wtk_cmask_sv_t* wtk_cmask_sv_new(wtk_cmask_sv_cfg_t *cfg);
void wtk_cmask_sv_delete(wtk_cmask_sv_t *cmask_pse);
void wtk_cmask_sv_reset(wtk_cmask_sv_t *cmask_pse);
void wtk_cmask_sv_feed(wtk_cmask_sv_t *cmask_pse, short *data, int len, int is_end);
void wtk_cmask_sv_feed2(wtk_cmask_sv_t *cmask_pse, short *data, int len, int is_end, int need_feed);
void wtk_cmask_sv_feed_cycle(wtk_cmask_sv_t *cmask_pse, short *data, int len, int is_end);
void wtk_cmask_sv_get_result(wtk_cmask_sv_t *cmask_pse, float **vec, int *len);
float wtk_cmask_sv_eval(wtk_cmask_sv_t *cmask_pse, float* v1, float *v2, int len);
#ifdef __cplusplus
};
#endif
#endif
