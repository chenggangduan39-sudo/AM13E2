#ifndef __QTK_TTS_SEGWRD_H__
#define __QTK_TTS_SEGWRD_H__

#include "qtk_tts_segwrd_cfg.h"
#include "wtk/tts/parser/wtk_tts_def.h"
#include "wtk/core/math/wtk_mat.h"
#include "wtk/core/rbin/wtk_rbin2.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct {
    qtk_tts_segwrd_cfg_t *cfg;
    wtk_vecdf_t *modelw;
    wtk_queue_t seg_qn;
}qtk_tts_segwrd_t;

typedef struct {
    wtk_queue_node_t q_n;
    int s;  //词在chars上开始的位置
    int e;  //词再chars上结束的位置
    int pos;    //词的长度
    int isw;    // 是不是分好的词
}qtk_tts_segwrd_node_t;


qtk_tts_segwrd_t* qtk_tts_segwrd_new(qtk_tts_segwrd_cfg_t *cfg, wtk_rbin2_t* rbin);
int qtk_tts_segwrd_process(qtk_tts_segwrd_t *segwrd,wtk_tts_info_t *info,wtk_tts_lab_t *lab);
int qtk_tts_segwrd_delete(qtk_tts_segwrd_t *segwrd);
void qtk_tts_segwrd_queue_print(wtk_string_t **charts,wtk_queue_t *q);


#ifdef __cplusplus
};
#endif

#endif
