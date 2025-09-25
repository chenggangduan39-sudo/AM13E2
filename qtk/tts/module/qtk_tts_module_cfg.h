/*
 * qtk_tts_module_cfg.h
 *
 *  Created on: Apr 24, 2023
 *      Author: dm
 */

#ifndef QTK_TTS_MODULE_QTK_TTS_MODULE_CFG_H_
#define QTK_TTS_MODULE_QTK_TTS_MODULE_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "parse/qtk_tts_parse_cfg.h"
#include "syn/qtk_tts_syn_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct{
	wtk_mbin_cfg_t *mbin_cfg;
    qtk_tts_parse_cfg_t parse;
    qtk_tts_syn_cfg_t syn;

    //param set
	float vol;
	float tempo;
	float pitch;
	float rate;
}qtk_tts_module_cfg_t;

int qtk_tts_module_cfg_init(qtk_tts_module_cfg_t *cfg);
int qtk_tts_module_cfg_clean(qtk_tts_module_cfg_t *cfg);
int qtk_tts_module_cfg_update_local(qtk_tts_module_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_tts_module_cfg_update(qtk_tts_module_cfg_t *cfg);
int qtk_tts_module_cfg_update2(qtk_tts_module_cfg_t *cfg,wtk_source_loader_t *sl);

qtk_tts_module_cfg_t* qtk_tts_module_cfg_new_bin(char *cfg_fn,int seek_pos);
void qtk_tts_module_cfg_delete_bin(qtk_tts_module_cfg_t *cfg);
int qtk_tts_module_cfg_delete(qtk_tts_module_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif /* QTK_TTS_MODULE_QTK_TTS_MODULE_CFG_H_ */
