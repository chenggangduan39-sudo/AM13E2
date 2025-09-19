#ifndef WTK_BFIO_CONSIST_WTK_MIC_CHECK_H
#define WTK_BFIO_CONSIST_WTK_MIC_CHECK_H
#include "wtk/bfio/consist/wtk_mic_check_cfg.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/wtk_strbuf.h"
#ifdef USE_NEON
#include "arm_neon.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_mic_check wtk_mic_check_t;
typedef enum {
    WTK_MIC_CHECK_NORMAL = 0,
    WTK_MIC_CHECK_RCD_ERROR,
    WTK_MIC_CHECK_PLAY_ERROR,
} wtk_mic_check_err_type_t;

typedef void (*wtk_mic_check_notify_f)(void *ths,
                                       wtk_mic_check_err_type_t *type,
                                       int nchn);

struct wtk_mic_check {
    wtk_mic_check_cfg_t *cfg;

    wtk_strbuf_t **mic;
    wtk_strbuf_t **sp;

    float *analysis_window;
    float *synthesis_window;
    wtk_drft_t *rfft;
    float *rfft_in;
    int nbin;
    float **analysis_mem;
    float **analysis_mem_sp;

    wtk_complex_t **fft;
    wtk_complex_t **fft_sp;

    float *eng;
    float *var;
    float *sc;
    float *tmp_median;
    int *tmp_state;
    float *eng_median;
    float *var_median;
    float *sc_median;

    int *mic_state;
    int *sp_state;
    int frame;

    float *fft_in;

    void *ths;
    wtk_mic_check_notify_f notify;

    wtk_mic_check_err_type_t *type;

    int sp_silcnt;
    int mic_silcnt;

    float mic_scale;
    float sp_scale;

    unsigned sp_sil : 1;
    unsigned mic_sil : 1;
};

wtk_mic_check_t *wtk_mic_check_new(wtk_mic_check_cfg_t *cfg);
void wtk_mic_check_delete(wtk_mic_check_t *mic_check);
void wtk_mic_check_start(wtk_mic_check_t *mic_check);
void wtk_mic_check_reset(wtk_mic_check_t *mic_check);
void wtk_mic_check_feed(wtk_mic_check_t *mic_check, short *data, int len,
                        float *play_vol, int is_end);
void wtk_mic_check_set_notify(wtk_mic_check_t *mic_check, void *ths,
                              wtk_mic_check_notify_f notify);

void wtk_mic_check_set_micscale(wtk_mic_check_t *mic_check, float scale);
#ifdef __cplusplus
};
#endif
#endif
