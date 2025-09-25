#ifndef SDK_AUDIO_QTK_AUOUT_CFG
#define SDK_AUDIO_QTK_AUOUT_CFG

#include "wtk/core/pitch/wtk_pitch_cfg.h"
// #include "sdk/util/mp3dec/wtk_mp3dec_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_auout_cfg qtk_auout_cfg_t;
struct qtk_auout_cfg
{
	// wtk_mp3dec_cfg_t mp3dec;
	wtk_pitch_cfg_t pitch;
	float pitch_shift;
	float pitch_shift_min;
	float pitch_shift_max;
	float pitch_shift_step;
	float volume_shift;
	float volume_shift_min;
	float volume_shift_max;
	float volume_shift_step;
	int wait_time;
	int play_wait_end_time;
	unsigned err_exit:1;
	// unsigned use_mp3dec:1;
	unsigned debug:1;
};

int qtk_auout_cfg_init(qtk_auout_cfg_t *cfg);
int qtk_auout_cfg_clean(qtk_auout_cfg_t *cfg);
int qtk_auout_cfg_update_local(qtk_auout_cfg_t *cfg,wtk_local_cfg_t *main);
int qtk_auout_cfg_update(qtk_auout_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
