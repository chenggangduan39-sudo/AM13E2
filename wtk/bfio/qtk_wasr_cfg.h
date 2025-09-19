#ifndef __WTK_BFIO_QTK_WASR_CFG_H__
#define __WTK_BFIO_QTK_WASR_CFG_H__
#include "wtk/asr/vad/wtk_vad_cfg.h"
#include "wtk/asr/wakeup/wtk_kwake_cfg.h"
#include "wtk/asr/wfst/kaldifst/qtk_decoder_wrapper_cfg.h"
#include "wtk/bfio/qform/wtk_qmmse_cfg.h"
#include "wtk/core/fft/wtk_stft2_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_wasr_cfg qtk_wasr_cfg_t;

struct qtk_wasr_cfg {
    wtk_vad_cfg_t vad;
    qtk_decoder_wrapper_cfg_t decoder;
    wtk_kwake_cfg_t wake;
    wtk_qmmse_cfg_t qmmse;
    wtk_stft2_cfg_t stft2;
    int audio_cache_ms;
    int audio_sample_rate;
    float wake_fe_adjust;

    unsigned use_qmmse : 1;
    unsigned use_vad_start : 1;
};

int qtk_wasr_cfg_init(qtk_wasr_cfg_t *cfg);
int qtk_wasr_cfg_clean(qtk_wasr_cfg_t *cfg);
int qtk_wasr_cfg_update(qtk_wasr_cfg_t *cfg);
int qtk_wasr_cfg_update_local(qtk_wasr_cfg_t *cfg, wtk_local_cfg_t *lc);

#ifdef __cplusplus
};
#endif
#endif
