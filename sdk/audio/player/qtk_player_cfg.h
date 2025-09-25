#ifndef SDK_AUDIO_QTK_PLAYER_CFG
#define SDK_AUDIO_QTK_PLAYER_CFG

#include "wtk/core/cfg/wtk_main_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_player_cfg qtk_player_cfg_t;
struct qtk_player_cfg
{
	char *snd_name;
	int buf_time;    // suggest > 20ms
	int max_write_fail_times;
	int channel;
    int sample_rate;
	int period_time;
	int bytes_per_sample;
	int start_time;
	int stop_time;
	int avail_time;
	int silence_time;
	int volume;
	int device_number;
	unsigned use_for_bfio:1;
	unsigned use_uac:1;
	unsigned use_set_volume:1;
};

int qtk_player_cfg_init(qtk_player_cfg_t *cfg);
int qtk_player_cfg_clean(qtk_player_cfg_t *cfg);
int qtk_player_cfg_update_local(qtk_player_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_player_cfg_update(qtk_player_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
