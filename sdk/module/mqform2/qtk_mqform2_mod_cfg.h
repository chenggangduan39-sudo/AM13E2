#ifndef __SDK_MQFORM2_MOD_CFG_H__
#define __SDK_MQFORM2_MOD_CFG_H__
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "sdk/audio/recorder/qtk_recorder_cfg.h"
#ifndef __ANDROID__
#include "sdk/audio/player/qtk_player_cfg.h"
#endif
#ifdef USE_FOR_DEV
#include "sdk/dev/led/qtk_led_cfg.h"
#endif
#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_mqform2_mod_cfg{
    qtk_recorder_cfg_t rcd;
#ifndef __ANDROID__
	qtk_player_cfg_t usbaudio;
	qtk_player_cfg_t lineout;
#endif

#ifdef USE_FOR_DEV
	qtk_led_cfg_t led;
#endif
	wtk_string_t uart_fn;
	wtk_string_t aec_fn;
	wtk_string_t soundscreen_fn;
	wtk_string_t eqform_fn;
	wtk_string_t beamnet_fn;
	wtk_string_t beamnet2_fn;
	wtk_string_t cache_path;
	int audio_type; //1 aecnspickup  2 qformssl 3 eqform 4 bfkasr
	int audio2_type; //1 aecnspickup  2 qformssl 3 eqform 4 bfkasr
    float echo_shift;
	int max_output_length;
	int out_source_channel;
	unsigned int use_aec:1;
	unsigned int use_lineout:1;
	unsigned int use_usbaudio:1;
	unsigned int use_olddevice:1;
	unsigned int use_uart:1;
	unsigned int debug:1;
	unsigned int use_log:1;
}qtk_mqform2_mod_cfg_t;

int qtk_mqform2_mod_cfg_init(qtk_mqform2_mod_cfg_t *cfg);
int qtk_mqform2_mod_cfg_clean(qtk_mqform2_mod_cfg_t *cfg);
int qtk_mqform2_mod_cfg_update_local(qtk_mqform2_mod_cfg_t *cfg,wtk_local_cfg_t *main);
int qtk_mqform2_mod_cfg_update(qtk_mqform2_mod_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
