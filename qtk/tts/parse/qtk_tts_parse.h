#ifndef __QTK_TTS_PARSE_H__
#define __QTK_TTS_PARSE_H__

#include "qtk_tts_parse_cfg.h"
#include "parser/wtk_tts_norm.h"
#include "parser/wtk_tts_segsnt.h"
#include "parser/wtk_tts_segwrd.h"
#include "qtk_tts_symbols_id.h"
#include "wtk/core/math/wtk_mat.h"
#include "parse/ncrf/qtk_tts_ncrf.h"
#include "qtk_tts_prosody.h"
#include "qtk_tts_polyphone.h"
#include "segwrd/qtk_tts_segwrd.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_tts_parse{
    qtk_tts_parse_cfg_t *cfg;
    
    wtk_tts_norm_t *norm;
    wtk_tts_segsnt_t *segsnt;
    wtk_tts_segwrd_t *segwrd;
    qtk_tts_segwrd_t *segwrd2;
    qtk_tts_symbols_id_t *symbols;
    qtk_tts_prosody_t *prosody;
    qtk_tts_polyphone_t *ply;

    wtk_heap_t *heap;
    wtk_strbuf_t *buf;

    wtk_string_t txt;
    wtk_tts_lab_t *lab;
    wtk_veci_t **id_vec;
    int nid;
}qtk_tts_parse_t;

qtk_tts_parse_t *qtk_tts_parse_new(qtk_tts_parse_cfg_t *cfg);
int qtk_tts_parse_delete(qtk_tts_parse_t *parse);
int qtk_tts_parse_reset(qtk_tts_parse_t *parse);
int qtk_tts_parse_process(qtk_tts_parse_t *parse, char *txt,int len);


#ifdef __cplusplus
};
#endif

#endif
