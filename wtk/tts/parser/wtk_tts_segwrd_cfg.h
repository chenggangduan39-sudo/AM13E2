#ifndef WTK_TTS_PARSER_WTK_TTS_SEGWRD_CFG
#define WTK_TTS_PARSER_WTK_TTS_SEGWRD_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk/core/wtk_kdict.h"
#include "wtk_tts_def.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/segmenter/wtk_segmenter.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_tts_segwrd_cfg wtk_tts_segwrd_cfg_t;


struct wtk_tts_segwrd_cfg
{
	wtk_segmenter_cfg_t seg;
	char *dict_fn;
	wtk_kdict_t *dict;
	int dict_hint;
	unsigned use_bin:1;
	unsigned use_maxseg:1;
	unsigned use_sp:1;             //old method, space split character
	unsigned use_sp2:1;            //new method, using prosody format symbol
	unsigned en_dict:1;
	unsigned use_upper:1;
	unsigned use_lower:1;
	unsigned pool_out:1;
};

int wtk_tts_segwrd_cfg_init(wtk_tts_segwrd_cfg_t *cfg);
int wtk_tts_segwrd_cfg_clean(wtk_tts_segwrd_cfg_t *cfg);
int wtk_tts_segwrd_cfg_update_local(wtk_tts_segwrd_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_tts_segwrd_cfg_update3(wtk_tts_segwrd_cfg_t *cfg,wtk_strpool_t *pool);
int wtk_tts_segwrd_cfg_update(wtk_tts_segwrd_cfg_t *cfg);
int wtk_tts_segwrd_cfg_bytes(wtk_tts_segwrd_cfg_t *cfg);

wtk_tts_wrd_pron_t* wtk_tts_wrd_pron_parse(wtk_strpool_t *pool,wtk_heap_t *heap,char *v,int v_bytes);
int wtk_tts_segwrd_cfg_update2(wtk_tts_segwrd_cfg_t *cfg,wtk_source_loader_t *sl,wtk_strpool_t *pool);
#ifdef __cplusplus
};
#endif
#endif
