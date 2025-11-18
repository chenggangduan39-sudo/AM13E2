#ifndef WTK_BFIO_QFORM_WTK_RTJOIN3
#define WTK_BFIO_QFORM_WTK_RTJOIN3
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk_rtjoin3_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_rtjoin3 wtk_rtjoin3_t;
typedef void (*wtk_rtjoin3_notify_f)(void *ths, short *output, int len);

struct wtk_rtjoin3 {
    wtk_rtjoin3_cfg_t *cfg;

    wtk_strbuf_t **mic;
    wtk_strbuf_t **sp;

    float *analysis_window;
    float *synthesis_window;
    wtk_drft_t *rfft;
    float *rfft_in;
    float **analysis_mem;
    float **analysis_mem_sp;
    float *synthesis_mem;
    float **mul_synthesis_mem;

    wtk_complex_t **fft;
    wtk_complex_t **fft_sp;
    wtk_complex_t *fftx;
    float *mul_out;
    float *out;

    wtk_qmmse_t *qmmse;

    wtk_equalizer_t *eq;

    float bs_scale;
    float bs_last_scale;
    float bs_real_scale;
    int bs_max_cnt;
    float *bs_win;

    void *ths;
    wtk_rtjoin3_notify_f notify;

    int sp_silcnt;
    int mic_silcnt;

    unsigned sp_sil : 1;
    unsigned mic_sil : 1;
};

wtk_rtjoin3_t *wtk_rtjoin3_new(wtk_rtjoin3_cfg_t *cfg);
void wtk_rtjoin3_delete(wtk_rtjoin3_t *rtjoin3);
void wtk_rtjoin3_start(wtk_rtjoin3_t *rtjoin3);
void wtk_rtjoin3_reset(wtk_rtjoin3_t *rtjoin3);
void wtk_rtjoin3_set_notify(wtk_rtjoin3_t *rtjoin3, void *ths,
                            wtk_rtjoin3_notify_f notify);
/**
 * len=mic array samples
 */
void wtk_rtjoin3_feed(wtk_rtjoin3_t *rtjoin3, short *data, int len, int is_end);

short *wtk_rtjoin3_get_play_signal(wtk_rtjoin3_t *rtjoin3, int *len);
#ifdef __cplusplus
};
#endif
#endif
