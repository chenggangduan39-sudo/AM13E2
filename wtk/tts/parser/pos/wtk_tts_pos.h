#ifndef WTK_TTS_PARSER_POS_WTK_TTS_POS
#define WTK_TTS_PARSER_POS_WTK_TTS_POS
#include "wtk/core/wtk_type.h" 
#include "wtk_tts_pos_cfg.h"
#include "wtk/tts/parser/wtk_tts_def.h"
#include "wtk/core/math/wtk_mat.h"
#include "wtk/core/wtk_fkv.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_tts_pos wtk_tts_pos_t;
struct wtk_tts_pos
{
	wtk_tts_pos_cfg_t *cfg;
	wtk_fkv_t *fkv;
	int oov;
};

void wtk_tts_pos_init(wtk_tts_pos_t *pos,wtk_tts_pos_cfg_t *cfg,wtk_rbin2_t *rbin);
void wtk_tts_pos_clean(wtk_tts_pos_t *pos);
void wtk_tts_pos_reset(wtk_tts_pos_t *pos);
int wtk_tts_pos_process(wtk_tts_pos_t *pos,wtk_tts_info_t *info,wtk_tts_lab_t *lab);
int wtk_tts_pos_process_snt(wtk_tts_pos_t *pos,wtk_tts_info_t *info,wtk_tts_snt_t *s);
int wtk_tts_pos_process_snt2(wtk_tts_pos_t *pos,wtk_tts_info_t *info,wtk_array_t*wrds_a);
int* wtk_tts_pos_decode(wtk_tts_pos_t *p,wtk_heap_t *heap,int *v,int n);
#ifdef __cplusplus
};
#endif
#endif
