#ifndef QTK_XVECTOR_CFG_H_
#define QTK_XVECTOR_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/asr/fextra/nnet3/wtk_nnet3_xvector_compute_cfg.h"
#include "wtk/asr/fextra/ivector/wtk_ivector_plda_scoring_cfg.h"
#include "wtk/asr/vad/wtk_vad_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_xvector_cfg qtk_xvector_cfg_t;
struct qtk_xvector_cfg
{
    wtk_nnet3_xvector_compute_cfg_t xvector;
    wtk_vad_cfg_t vad;

    float vad_energy_thresh;
    float vad_energy_mean_scale;
    float vad_proportion_threshold;
    int vad_frames_context;
};

int qtk_xvector_cfg_init(qtk_xvector_cfg_t *cfg);
int qtk_xvector_cfg_clean(qtk_xvector_cfg_t *cfg);
int qtk_xvector_cfg_update_local(qtk_xvector_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_xvector_cfg_update(qtk_xvector_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
