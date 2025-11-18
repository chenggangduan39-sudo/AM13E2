#ifndef __SDK_MULT_MOD_CFG_H__
#define __SDK_MULT_MOD_CFG_H__
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "sdk/audio/recorder/qtk_recorder_cfg.h"
#include "sdk/audio/player/qtk_player_cfg.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_mult_mod_cfg{
    qtk_recorder_cfg_t rcd;
	qtk_player_cfg_t usbaudio;
	qtk_player_cfg_t lineout;

    wtk_string_t vboxebf_fn;
	wtk_string_t cache_path;
	int audio_type; //1 vboxebf  2 gainnetbf 3 ssl 4 eqform
	int max_add_count;
	int max_play_count;
	int max_output_length;
	int sleep_time;
	int resample_rate;
	int spknum;
	int skip_head_tm;
	float restart_time;
    float echo_shift;
	float mic_shift;
	float spk_shift;
	unsigned int use_lineout:1;
	unsigned int use_usbaudio:1;
	unsigned int debug:1;
	unsigned int use_log:1;
	unsigned int use_log_wav:1;
	unsigned int use_resample:1;
	unsigned int use_in_resample:1;
}qtk_mult_mod_cfg_t;

int qtk_mult_mod_cfg_init(qtk_mult_mod_cfg_t *cfg);
int qtk_mult_mod_cfg_clean(qtk_mult_mod_cfg_t *cfg);
int qtk_mult_mod_cfg_update_local(qtk_mult_mod_cfg_t *cfg,wtk_local_cfg_t *main);
int qtk_mult_mod_cfg_update(qtk_mult_mod_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
