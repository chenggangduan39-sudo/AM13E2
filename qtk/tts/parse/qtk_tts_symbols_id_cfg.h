#ifndef __QTK_TTS_SYMBOLS_ID_CFG_H__
#define __QTK_TTS_SYMBOLS_ID_CFG_H__

#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk/core/wtk_kdict.h"
#include "ncrf/qtk_tts_ncrf_cfg.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_tts_symbols_id_cfg 
{
    wtk_array_t *symbols;

    unsigned int is_en:1;
}qtk_tts_symbols_id_cfg_t;

int qtk_tts_symbols_id_cfg_init(qtk_tts_symbols_id_cfg_t *cfg);
int qtk_tts_symbols_id_cfg_clean(qtk_tts_symbols_id_cfg_t *cfg);
int qtk_tts_symbols_id_cfg_update_local(qtk_tts_symbols_id_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_tts_symbols_id_cfg_update(qtk_tts_symbols_id_cfg_t *cfg);

#ifdef __cplusplus
};
#endif

#endif