#ifndef __QTK_TTS_PROSODY_CFG_H__
#define __QTK_TTS_PROSODY_CFG_H__

#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/wtk_kdict.h"
#include "ncrf/qtk_tts_ncrf_cfg.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct {
    qtk_tts_ncrf_cfg_t ncrf;

    wtk_array_t *symbols2;
    wtk_array_t *symbols3;
    wtk_array_t *symbols4;
    wtk_array_t *symbols5;
    wtk_array_t *filter;
    wtk_kdict_t *dict;
    char *dict_fn;
    int dict_hint;
    char *split_sym;

    unsigned int use_ncrf:1;
    unsigned int use_start_sil:1;
    unsigned int use_end_sil:1;
    unsigned int use_tail_sym:1;
    unsigned int use_segwrd_sym:1;
}qtk_tts_prosody_cfg_t;

int qtk_tts_prosody_cfg_init(qtk_tts_prosody_cfg_t *cfg);
int qtk_tts_prosody_cfg_clean(qtk_tts_prosody_cfg_t *cfg);
int qtk_tts_prosody_cfg_update_local(qtk_tts_prosody_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_tts_prosody_cfg_update(qtk_tts_prosody_cfg_t *cfg);
int qtk_tts_prosody_cfg_update2(qtk_tts_prosody_cfg_t *cfg, wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif

#endif
