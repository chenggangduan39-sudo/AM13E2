#ifndef QTK_AUDIO_QTK_RECORDER_CFG
#define QTK_AUDIO_QTK_RECORDER_CFG

#include "wtk/core/cfg/wtk_main_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_recorder_cfg qtk_recorder_cfg_t;
struct qtk_recorder_cfg {
    char *snd_name;
    int buf_time;
    int buf_size;
    int channel;
    int *skip_channels;
    int nskip;
    int sample_rate;
    int bytes_per_sample;
    int mic_gain;
    unsigned int use_gain_set : 1;
};

int qtk_recorder_cfg_init(qtk_recorder_cfg_t *cfg);
int qtk_recorder_cfg_clean(qtk_recorder_cfg_t *cfg);
int qtk_recorder_cfg_update_local(
    qtk_recorder_cfg_t *cfg, wtk_local_cfg_t *lc); //,int *channels,int nskip);
int qtk_recorder_cfg_update(qtk_recorder_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
