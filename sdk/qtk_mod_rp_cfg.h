#ifndef __SDK_MOD_RP_CFG_H__
#define __SDK_MOD_RP_CFG_H__
#include "wtk/core/cfg/wtk_local_cfg.h"
#ifndef __ANDROID__
#include "qtk/record/qtk_record_cfg.h"
#include "qtk/play/qtk_play_cfg.h"
#endif

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_mod_rp_cfg{
#ifndef __ANDROID__
    qtk_record_cfg_t rcd;
	qtk_play_cfg_t usbaudio;
	qtk_play_cfg_t lineout;
#endif
	wtk_string_t cache_path;

	unsigned int use_lineout:1;
	unsigned int use_usbaudio:1;
	unsigned int use_resample:1;
	unsigned int debug:1;
	unsigned int use_log:1;
	unsigned int use_log_wav:1;
}qtk_mod_rp_cfg_t;

int qtk_mod_rp_cfg_init(qtk_mod_rp_cfg_t *cfg);
int qtk_mod_rp_cfg_clean(qtk_mod_rp_cfg_t *cfg);
int qtk_mod_rp_cfg_update_local(qtk_mod_rp_cfg_t *cfg,wtk_local_cfg_t *main);
int qtk_mod_rp_cfg_update(qtk_mod_rp_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
