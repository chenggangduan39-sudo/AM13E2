#ifndef __QTK_TTS_POLYPHONE_H__
#define __QTK_TTS_POLYPHONE_H__

#include "wtk/tts/parser/wtk_tts_def.h"
#include "qtk_tts_polyphone_cfg.h"
#include "wtk/core/wtk_queue.h"
#include "dispoly/qtk_tts_dispoly.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_tts_polyphone{
    qtk_tts_polyphone_cfg_t *cfg;
    qtk_tts_dispoly_t *disply;
    wtk_queue_t *qn;
    wtk_queue_t *ply_qn;
    int ply_num;
}qtk_tts_polyphone_t;

typedef struct qtk_tts_pron{
    wtk_queue_node_t q_n;
    wtk_string_t *wchar;
    wtk_tts_wrd_pron_t *pron;
    int idx;
}qtk_tts_pron_t;

typedef struct qtk_tts_ply_idx{
    wtk_queue_node_t q_n;
    int idx;
}qtk_tts_ply_idx_t;

qtk_tts_polyphone_t *qtk_tts_polyphone_new(qtk_tts_polyphone_cfg_t *cfg, wtk_rbin2_t* rbin);
int qtk_tts_polyphone_process(qtk_tts_polyphone_t *ply,wtk_tts_info_t *info,wtk_tts_lab_t *lab);
int qtk_tts_polyphone_delete(qtk_tts_polyphone_t *ply);
int qtk_tts_polyphone_reset(qtk_tts_polyphone_t *ply);
void qtk_tts_polyphone_print(qtk_tts_polyphone_t *ply);

#ifdef __cplusplus
};
#endif

#endif
