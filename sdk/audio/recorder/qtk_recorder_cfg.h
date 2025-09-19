#ifndef SDK_AUDIO_QTK_RECORDER_CFG
#define SDK_AUDIO_QTK_RECORDER_CFG

#include "wtk/core/cfg/wtk_main_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_recorder_cfg qtk_recorder_cfg_t;
struct qtk_recorder_cfg
{
	char *snd_name;
	int buf_time;
	int buf_size;
	int channel;
	int *skip_channels;
	int nskip;
	int mic_gain;
    int cb_gain;
	int sample_rate;
	int bytes_per_sample;
	int vendor_id;
	int product_id;
	int max_read_fail_times;
	int device_number;
	float mic_shift;
	unsigned use_for_bfio:1;
	unsigned use_gain_set:1;
	unsigned use_log_ori_audio:1;
};

int qtk_recorder_cfg_init(qtk_recorder_cfg_t *cfg);
int qtk_recorder_cfg_clean(qtk_recorder_cfg_t *cfg);
int qtk_recorder_cfg_update_local(qtk_recorder_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_recorder_cfg_update(qtk_recorder_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
