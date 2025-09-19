#ifndef __SDK_MGAINNET_MOD_CFG_H__
#define __SDK_MGAINNET_MOD_CFG_H__
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "sdk/audio/recorder/qtk_recorder_cfg.h"
#include "sdk/audio/player/qtk_player_cfg.h"
#include "sdk/module/qtk_module_tool.h"
#ifdef USE_FOR_DEV
#include "sdk/dev/led/qtk_led_cfg.h"
#endif
#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_mgainnet_mod_cfg{
    qtk_recorder_cfg_t rcd;
	qtk_player_cfg_t usbaudio;
	qtk_player_cfg_t lineout;

#ifdef USE_FOR_DEV
	qtk_led_cfg_t led;
#endif
	wtk_string_t uart_fn;
	wtk_string_t cache_path;
	char *engine_params;
	wtk_strbuf_t *swap_buf;
    float echo_shift;
	unsigned int use_lineout:1;
	unsigned int use_usbaudio:1;
	unsigned int use_uart:1;
	unsigned int debug:1;
	unsigned int use_log:1;
	unsigned int use_log_wav:1;
	unsigned int use_resample:1;
}qtk_mgainnet_mod_cfg_t;

int qtk_mgainnet_mod_cfg_init(qtk_mgainnet_mod_cfg_t *cfg);
int qtk_mgainnet_mod_cfg_clean(qtk_mgainnet_mod_cfg_t *cfg);
int qtk_mgainnet_mod_cfg_update_local(qtk_mgainnet_mod_cfg_t *cfg,wtk_local_cfg_t *main);
int qtk_mgainnet_mod_cfg_update(qtk_mgainnet_mod_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
