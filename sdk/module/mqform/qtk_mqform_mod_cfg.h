#ifndef __SDK_MQFORM_MOD_CFG_H__
#define __SDK_MQFORM_MOD_CFG_H__
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "sdk/audio/recorder/qtk_recorder_cfg.h"
#include "sdk/audio/player/qtk_player_cfg.h"

#ifdef USE_FOR_DEV
#include "sdk/dev/led/qtk_led_cfg.h"
#endif
#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_mqform_mod_cfg{
    qtk_recorder_cfg_t rcd;
	qtk_player_cfg_t usbaudio;
	qtk_player_cfg_t lineout;

#ifdef USE_RECORD_PLAYER
	qtk_recorder_cfg_t two_rcd;
	qtk_player_cfg_t two_ply;
#endif

#ifdef USE_FOR_DEV
	qtk_led_cfg_t led;
#endif
	wtk_string_t uart_fn;
    wtk_string_t vboxebf_fn;
	wtk_string_t ssl_fn;
	wtk_string_t cache_path;
	int bfkasr_type; //1 vboxebf  2 qformssl 3 eqform 4 bfkasr
	int audio_type; //1 vboxebf  2 qformssl 3 eqform 4 bfkasr
	int ssl_type;
	int resample_rate;
	int spknum;
	int skip_head_tm;
	int max_output_length;
	int max_add_count;
	float mic_shift;
	float spk_shift;
	float echo_shift;
	unsigned int use_ssl:1;
	unsigned int use_aec:1;
	unsigned int use_lineout:1;
	unsigned int use_usbaudio:1;
	unsigned int use_uart:1;
	unsigned int debug:1;
	unsigned int use_log:1;
	unsigned int use_record_player:1;
	unsigned int use_log_wav:1;
	unsigned int use_resample:1;
	unsigned int use_in_resample:1;
}qtk_mqform_mod_cfg_t;

int qtk_mqform_mod_cfg_init(qtk_mqform_mod_cfg_t *cfg);
int qtk_mqform_mod_cfg_clean(qtk_mqform_mod_cfg_t *cfg);
int qtk_mqform_mod_cfg_update_local(qtk_mqform_mod_cfg_t *cfg,wtk_local_cfg_t *main);
int qtk_mqform_mod_cfg_update(qtk_mqform_mod_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
