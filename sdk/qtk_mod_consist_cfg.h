#ifndef __QTK_MOD_CONSIST_CFG_H__
#define __QTK_MOD_CONSIST_CFG_H__
#include "wtk/core/cfg/wtk_local_cfg.h"
#ifndef __ANDROID__
#include "qtk/record/qtk_record_cfg.h"
#include "qtk/play/qtk_play_cfg.h"
#endif
#include "wtk/bfio/consist/wtk_consist_cfg.h"
#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_mod_consist_cfg{
#ifndef __ANDROID__
    qtk_record_cfg_t rcd;
	qtk_play_cfg_t play;
#endif

    wtk_string_t consist_fn;
    wtk_consist_cfg_t *consist_cfg;
	wtk_string_t cache_path;
	int mode;
    float eq_offset;
    float nil_er;
    float mic_corr_er;
    float mic_corr_aver;
    float mic_energy_er;
    float mic_energy_aver;

    float spk_corr_er;
    float spk_corr_aver;
    float spk_energy_er;
    float spk_energy_aver;
	int skip_head_tm;

	unsigned int use_xcorr:1;
	unsigned int use_consist_bin:1;
	unsigned int debug:1;
	unsigned int use_log:1;
	unsigned int use_bfio:1;
}qtk_mod_consist_cfg_t;

int qtk_mod_consist_cfg_init(qtk_mod_consist_cfg_t *cfg);
int qtk_mod_consist_cfg_clean(qtk_mod_consist_cfg_t *cfg);
int qtk_mod_consist_cfg_update_local(qtk_mod_consist_cfg_t *cfg,wtk_local_cfg_t *main);
int qtk_mod_consist_cfg_update(qtk_mod_consist_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
