#ifndef SDK_AUDIO_daemon_QTK_AUDIO_daemon_CFG
#define SDK_AUDIO_daemon_QTK_AUDIO_daemon_CFG

#include "wtk/core/cfg/wtk_local_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_audio_daemon_cfg qtk_audio_daemon_cfg_t;
struct qtk_audio_daemon_cfg
{
	int vendor_id;
	int product_id;
};

int qtk_audio_daemon_cfg_init(qtk_audio_daemon_cfg_t *cfg);
int qtk_audio_daemon_cfg_clean(qtk_audio_daemon_cfg_t *cfg);
int qtk_audio_daemon_cfg_update_local(qtk_audio_daemon_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_audio_daemon_cfg_update(qtk_audio_daemon_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
