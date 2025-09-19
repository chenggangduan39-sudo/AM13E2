#ifndef WTK_TTS_PARSER_WTK_TTS_SEGSNT_CFG
#define WTK_TTS_PARSER_WTK_TTS_SEGSNT_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk/core/cfg/wtk_source.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_tts_segsnt_cfg wtk_tts_segsnt_cfg_t;
struct wtk_tts_segsnt_cfg
{
	wtk_array_t *end_tok;
	wtk_array_t *end_cand_tok;         //using for avoiding random split whole sentence when overmax_word add by dmd 2017.05.31
	wtk_array_t *ques_sym;
	wtk_array_t *ques_char;
	wtk_array_t *ques_char2;
	wtk_array_t *ques_start2;
	wtk_array_t *sigh_sym;
	wtk_array_t *sigh_char;
	int max_word;
	int rand_step;
	unsigned use_random_tone:1;

	// for alone pron
	char *pick_list;         //words in list file will be picked up from context.
	wtk_str_hash_t *pick_h;
};

int wtk_tts_segsnt_cfg_init(wtk_tts_segsnt_cfg_t *cfg);
int wtk_tts_segsnt_cfg_clean(wtk_tts_segsnt_cfg_t *cfg);
int wtk_tts_segsnt_cfg_update_local(wtk_tts_segsnt_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_tts_segsnt_cfg_update(wtk_tts_segsnt_cfg_t *cfg);
int wtk_tts_segsnt_cfg_update2(wtk_tts_segsnt_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif
