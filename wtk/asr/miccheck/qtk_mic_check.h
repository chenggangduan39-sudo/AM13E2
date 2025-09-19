#ifndef QTK_MIC_CHECK_H_
#define QTK_MIC_CHECK_H_
#include "qtk_mic_check_cfg.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/core/fft/wtk_stft2.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_mic_check qtk_mic_check_t;

struct qtk_mic_check{
    qtk_mic_check_cfg_t *cfg;
    wtk_stft2_t *stft2;
    wtk_complex_t *freq;
    float *f;
    double *mel;
    double *fb;
    double *medfilt2_out;
    int *res;
    int winsize;
    int hopsize;
    int nframe;
    int numbin;
    int nchannel;
};

qtk_mic_check_t *qtk_mic_check_new(qtk_mic_check_cfg_t *cfg);
void qtk_mic_check_delete(qtk_mic_check_t *mc);
void qtk_mic_check_reset(qtk_mic_check_t *mc);
void qtk_mic_check_feed(qtk_mic_check_t *mc, char *fn);
int* qtk_mic_check_get_result(qtk_mic_check_t *mc, int *n);
#ifdef __cplusplus
};
#endif
#endif
