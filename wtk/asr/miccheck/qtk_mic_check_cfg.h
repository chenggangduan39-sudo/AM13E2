#ifndef QTK_MIC_CHECK_CFG_H_
#define QTK_MIC_CHECK_CFG_H_
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/fft/wtk_stft2_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_mic_check_cfg qtk_mic_check_cfg_t;

struct qtk_mic_check_cfg {
    wtk_stft2_cfg_t stft2;
    char *filterA_fn;
    char *filterB_fn;
    double *filt_a;
    double *filt_b;
    float thresh;
};

int qtk_mic_check_cfg_init(qtk_mic_check_cfg_t *cfg);
int qtk_mic_check_cfg_clean(qtk_mic_check_cfg_t *cfg);
int qtk_mic_check_cfg_update(qtk_mic_check_cfg_t *cfg);
int qtk_mic_check_cfg_update2(qtk_mic_check_cfg_t *cfg, wtk_source_loader_t *sl);
int qtk_mic_check_cfg_update_local(qtk_mic_check_cfg_t *cfg, wtk_local_cfg_t *lc);
#ifdef __cplusplus
};
#endif
#endif
