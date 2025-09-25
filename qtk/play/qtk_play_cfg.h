#ifndef __QTK_PLAY_CFG_H__
#define __QTK_PLAY_CFG_H__
#ifdef __cplusplus
extern "C"{
#endif
#include "wtk/core/cfg/wtk_local_cfg.h"
typedef struct qtk_play_cfg{
	wtk_string_t snd_name;
    int channel;
    int sample_rate;
	int bytes_per_sample;
    int buf_time;
    int period_time;
	int start_time;
	int stop_time;
	int avail_time;
	int silence_time;
    int volume;
    unsigned use_uac:1;
    unsigned use_get_soundcard:1;
    unsigned use_set_volume:1;
}qtk_play_cfg_t;

int qtk_play_cfg_init(qtk_play_cfg_t *cfg);
int qtk_play_cfg_clean(qtk_play_cfg_t *cfg);
int qtk_play_cfg_update_local(qtk_play_cfg_t *cfg,wtk_local_cfg_t *main);
int qtk_play_cfg_update(qtk_play_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
