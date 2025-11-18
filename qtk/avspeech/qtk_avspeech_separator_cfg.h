#ifndef G_53F7DFEB0F3449618361B4A740F4D0E3
#define G_53F7DFEB0F3449618361B4A740F4D0E3
#include "qtk/avspeech/qtk_avspeech_lip_cfg.h"
#include "qtk/avspeech/qtk_avspeech_visual_voice_cfg.h"
#include "wtk/bfio/agc/wtk_agc_cfg.h"
#include "wtk/bfio/maskdenoise/cmask_pse/wtk_cmask_pse_cfg.h"
#include "wtk/bfio/qform/wtk_qform9_cfg.h"
#include "wtk/core/cfg/wtk_local_cfg.h"

typedef struct qtk_avspeech_separator_cfg qtk_avspeech_separator_cfg_t;

struct qtk_avspeech_separator_cfg {
    qtk_avspeech_lip_cfg_t lip;
    qtk_nnrt_cfg_t dnsmos;
    wtk_agc_cfg_t agc;
    wtk_qform9_cfg_t qform9;
    qtk_avspeech_visual_voice_cfg_t visual_voice;
    int sampling_rate;
    float *mean;
    float *std;
    int image_max_free;
    int lip_result_max_free;
    int tracklet_max_free;

    int patch_bytes;
    int audio_frame_bytes;
    int video_segment_bytes;
    int audio_segment_bytes;
    int height;
    int width;
    int sliding_factor;
    char *vad_fn;
    char *save_video_path;
    int dnsmos_nsample;
    wtk_cmask_pse_cfg_t pse;
    char *vp_extractor_vad_fn;
    float vp_extractor_duration_min_s;
    float vp_dnsmos_thresh;

    // post cfg
    float thresh_low_conf;
    float thresh_high_conf;
    float mid_conf_reserve_factor;
    float reback_thresh;

    unsigned use_mpp : 1;
    unsigned use_vad : 1;
    unsigned use_qform9 : 1;
    unsigned use_post : 1;
    unsigned sync_audio : 1;
    unsigned use_agc : 1;
    unsigned use_vpdenoise : 1;
};

int qtk_avspeech_separator_cfg_init(qtk_avspeech_separator_cfg_t *cfg);
int qtk_avspeech_separator_cfg_clean(qtk_avspeech_separator_cfg_t *cfg);
int qtk_avspeech_separator_cfg_update(qtk_avspeech_separator_cfg_t *cfg);
int qtk_avspeech_separator_cfg_update_local(qtk_avspeech_separator_cfg_t *cfg,
                                            wtk_local_cfg_t *lc);
int qtk_avspeech_separator_cfg_update2(qtk_avspeech_separator_cfg_t *cfg,
                                       wtk_source_loader_t *sl);

#endif
