#ifndef __QTK_TTS_SYMBOLS_ID_H__
#define __QTK_TTS_SYMBOLS_ID_H__

#include "qtk_tts_symbols_id_cfg.h"
#include "ncrf/qtk_tts_ncrf.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_tts_symbols_id{
    qtk_tts_symbols_id_cfg_t *cfg;
}qtk_tts_symbols_id_t;

qtk_tts_symbols_id_t* qtk_tts_symbols_id_new(qtk_tts_symbols_id_cfg_t *cfg);
int qtk_tts_symbols_id_delete(qtk_tts_symbols_id_t *symbols);
int qtk_tts_symbols_id_get_id(qtk_tts_symbols_id_t *symbols,char *sym,int len);

#ifdef __cplusplus
};
#endif

#endif