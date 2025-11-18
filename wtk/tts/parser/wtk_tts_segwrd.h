#ifndef WTK_TTS_PARSER_WTK_TTS_SEGWRD
#define WTK_TTS_PARSER_WTK_TTS_SEGWRD
#include "wtk/core/wtk_type.h" 
#include "wtk/core/wtk_fkv.h"
#include "wtk_tts_segwrd_cfg.h"
#include "wtk_maxseg.h"
#include "wtk_tts_def.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_tts_segwrd wtk_tts_segwrd_t;

struct wtk_tts_segwrd
{
	wtk_tts_segwrd_cfg_t *cfg;
	wtk_segmenter_t *segmenter;
	wtk_maxseg_t seg;
	wtk_fkv_t *kv;
	wtk_str_hash_t *hash;
};

wtk_tts_segwrd_t* wtk_tts_segwrd_new(wtk_tts_segwrd_cfg_t *cfg,wtk_rbin2_t *rbin);
void wtk_tts_segwrd_delete(wtk_tts_segwrd_t *wrd);

void wtk_tts_segwrd_init(wtk_tts_segwrd_t *wrd,wtk_tts_segwrd_cfg_t *cfg,wtk_rbin2_t *rbin);
void wtk_tts_segwrd_clean(wtk_tts_segwrd_t *wrd);
void wtk_tts_segwrd_reset(wtk_tts_segwrd_t *wrd);
int wtk_tts_segwrd_process(wtk_tts_segwrd_t *wrd,wtk_tts_info_t *info,wtk_tts_lab_t *lab);
int wtk_tts_segwrd_process_snt(wtk_tts_segwrd_t *wrd,wtk_tts_info_t *info,wtk_tts_snt_t *s);
#ifdef __cplusplus
};
#endif
#endif
