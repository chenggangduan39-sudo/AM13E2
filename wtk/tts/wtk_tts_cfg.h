#ifndef WTK_TFIRE_KEL_TTS_WTK_TTS_CFG_H_
#define WTK_TFIRE_KEL_TTS_WTK_TTS_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/tts/parser/wtk_tts_parser.h"
#include "wtk/tts/syn/wtk_syn.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/pitch/wtk_pitch.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_tts_cfg wtk_tts_cfg_t;

struct wtk_tts_cfg
{
	wtk_tts_parser_cfg_t parser;
	wtk_pitch_cfg_t pitch;
	wtk_syn_cfg_t syn;
	wtk_strpool_t *pool;
	int buf_size;
	int	 min_sil_time;
	int snt_sil_time;
	int max_sil_value;
	float volume_scale;
	unsigned use_thread:1;
	wtk_mbin_cfg_t *bin_cfg;
	wtk_main_cfg_t *main_cfg;
};


int wtk_tts_cfg_init(wtk_tts_cfg_t *cfg);
int wtk_tts_cfg_clean(wtk_tts_cfg_t *cfg);
int wtk_tts_cfg_update_local(wtk_tts_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_tts_cfg_update(wtk_tts_cfg_t *cfg);
int wtk_tts_cfg_update2(wtk_tts_cfg_t *cfg,wtk_source_loader_t *sl);

int wtk_tts_cfg_bytes(wtk_tts_cfg_t *cfg);

wtk_tts_cfg_t* wtk_tts_cfg_new_bin(char *cfg_fn);
wtk_tts_cfg_t* wtk_tts_cfg_new_bin2(char *cfg_fn,int seek_pos);
void wtk_tts_cfg_delete_bin(wtk_tts_cfg_t *cfg);

wtk_tts_cfg_t* wtk_tts_cfg_new(char *cfg_fn);
void wtk_tts_cfg_delete(wtk_tts_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
