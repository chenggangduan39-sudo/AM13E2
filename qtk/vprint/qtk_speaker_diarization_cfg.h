#ifndef G_NJE1SY65_7UZL_6FYV_Z7MD_WYVS173RL1R3
#define G_NJE1SY65_7UZL_6FYV_Z7MD_WYVS173RL1R3
#pragma once
#include "wtk/asr/fextra/nnet3/wtk_nnet3_xvector_compute.h"
#include "wtk/asr/vad/kvad/wtk_kvad_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_speaker_diarization_cfg qtk_speaker_diarization_cfg_t;

struct qtk_speaker_diarization_cfg {
    wtk_nnet3_xvector_compute_cfg_t xvector;
    wtk_kxparm_cfg_t kxparm;
    wtk_kvad_cfg_t kvad;
    int sliding_win_frames;
    int sliding_step_frames;
    int emb_len;
    int feat_dim;
    int vad_frame_ms;
    int feat_frame_ms;
    float spec_pval;
};

int qtk_speaker_diarization_cfg_init(qtk_speaker_diarization_cfg_t *cfg);
int qtk_speaker_diarization_cfg_clean(qtk_speaker_diarization_cfg_t *cfg);
int qtk_speaker_diarization_cfg_update(qtk_speaker_diarization_cfg_t *cfg);
int qtk_speaker_diarization_cfg_update_local(qtk_speaker_diarization_cfg_t *cfg,
                                             wtk_local_cfg_t *lc);

#ifdef __cplusplus
};
#endif
#endif /* G_NJE1SY65_7UZL_6FYV_Z7MD_WYVS173RL1R3 */
