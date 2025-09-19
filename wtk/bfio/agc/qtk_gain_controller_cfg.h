#ifndef WTK_BFIO_AHS_QTK_gain_controller_CFG
#define WTK_BFIO_AHS_QTK_gain_controller_CFG
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_ahs_gain_controller_cfg qtk_ahs_gain_controller_cfg_t;

struct qtk_ahs_gain_controller_cfg {
    float update_thresh;
    float alpha;
    float beta;
    float pworg;
    float pvorg;

    float Maximun_Gain;
    float Maximun_Atten;

    int hop_size;
    int sample_rate;

    int static_size;
    float pvad_threshold;
    float low_energy_threshold;
    float high_energy_threshold;
    int vad_delay;
    int vad_init_frame;

    int voice_init_cnt;
    int voice_frame;
    float voice_precent;
    float voice_alpha;
    float voice_alpha2;
    float voice_prob;
    float voice_thresh;

    unsigned use_evad:1;
    unsigned use_update_gali:1;
    unsigned use_voice_prob:1;
};

int qtk_ahs_gain_controller_cfg_init(qtk_ahs_gain_controller_cfg_t *cfg);
int qtk_ahs_gain_controller_cfg_clean(qtk_ahs_gain_controller_cfg_t *cfg);
int qtk_ahs_gain_controller_cfg_update_local(qtk_ahs_gain_controller_cfg_t *cfg, wtk_local_cfg_t *lc);
int qtk_ahs_gain_controller_cfg_update(qtk_ahs_gain_controller_cfg_t *cfg);
int qtk_ahs_gain_controller_cfg_update2(qtk_ahs_gain_controller_cfg_t *cfg, wtk_source_loader_t *sl);

#ifdef __cplusplus
};
#endif
#endif
