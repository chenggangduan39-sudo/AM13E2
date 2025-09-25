#ifndef WTK_TTS_PARSER_POS_WTK_TTS_POS_CFG
#define WTK_TTS_PARSER_POS_WTK_TTS_POS_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk/core/cfg/wtk_source.h"
#include "wtk_tts_poshmm.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_tts_pos_cfg wtk_tts_pos_cfg_t;
struct wtk_tts_pos_cfg
{
	wtk_array_t *attr;
	char *hmm_fn;
	char *voc_fn;
	int nwrd;
	wtk_tts_poshmm_t *hmm;
	unsigned use_voc_bin:1;
};

int wtk_tts_pos_cfg_init(wtk_tts_pos_cfg_t *cfg);
int wtk_tts_pos_cfg_clean(wtk_tts_pos_cfg_t *cfg);
int wtk_tts_pos_cfg_update_local(wtk_tts_pos_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_tts_pos_cfg_update(wtk_tts_pos_cfg_t *cfg,wtk_strpool_t *pool);
int wtk_tts_pos_cfg_update2(wtk_tts_pos_cfg_t *cfg,wtk_source_loader_t *sl,wtk_strpool_t *pool);
int wtk_tts_pos_cfg_bytes(wtk_tts_pos_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
