#ifndef __QTK_FEAT_QTK_PITCH_CFG_H__
#define __QTK_FEAT_QTK_PITCH_CFG_H__
#include "wtk/asr/fextra/pitch/audio/qtk_resample_cfg.h"
#include "wtk/asr/fextra/pitch/feat/qtk_pitch_post_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_pitch_cfg qtk_pitch_cfg_t;

struct qtk_pitch_cfg {
    qtk_linear_resample_cfg_t resamp;
    qtk_arbitrary_resample_cfg_t nccf_resamp;
    qtk_pitch_post_cfg_t post;
    float min_f0;
    float max_f0;
    float soft_min_f0;
    float delta_pitch;
    int frame_shift_ms;
    int frame_length_ms;
    int upsamp_filter_width;
    int recompute_frame;
    int max_frames_latency;
    int frames_per_chunk;

    float penalty_factor;

    unsigned nccf_ballast_online : 1;
    unsigned simulate_first_pass_online : 1;
    unsigned snip_edges : 1;
    unsigned use_post : 1;

    float preemph_coeff;
    float nccf_ballast;

    /* update section */
    int nccf_win_sz;
    int nccf_win_shift;
    float upsamp_cutoff;
};

int qtk_pitch_cfg_init(qtk_pitch_cfg_t *cfg);
int qtk_pitch_cfg_update(qtk_pitch_cfg_t *cfg);
int qtk_pitch_cfg_clean(qtk_pitch_cfg_t *cfg);

#ifdef __cplusplus
};
#endif
#endif
