#ifndef SDK_AUDIO_QTK_AUDIO_CFG
#define SDK_AUDIO_QTK_AUDIO_CFG

#include "player/qtk_player_cfg.h"
#include "recorder/qtk_recorder_cfg.h"
#include "sdk/audio/usb/qtk_usb_cfg.h"
#include "qtk_auout_cfg.h"
#include "qtk_auin_cfg.h"
#include "daemon/qtk_audio_daemon_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_audio_cfg qtk_audio_cfg_t;
struct qtk_audio_cfg
{
	qtk_usb_cfg_t usb;
	qtk_player_cfg_t player;
	qtk_recorder_cfg_t recorder;
	qtk_auin_cfg_t auin;
	qtk_auout_cfg_t auout;
	qtk_audio_daemon_cfg_t daemon;
	int zero_wait_time;
	int daemon_wait_time;
	unsigned use_audio:1;
	unsigned use_daemon:1;
	unsigned use_sys_record:1;
	unsigned use_sys_play:1;
	unsigned use_zero:1;
	unsigned use_dc:1;
	unsigned debug:1;
};

int qtk_audio_cfg_init(qtk_audio_cfg_t *cfg);
int qtk_audio_cfg_clean(qtk_audio_cfg_t *cfg);
int qtk_audio_cfg_update_local(qtk_audio_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_audio_cfg_update(qtk_audio_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
