#ifndef WTK_TTS_SYN_WTK_SYNDICT_CFG
#define WTK_TTS_SYN_WTK_SYNDICT_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_syndict_cfg wtk_syndict_cfg_t;

struct wtk_syndict_cfg
{
	char *dict_fn;
	int dict_hash_hint;
};

int wtk_syndict_cfg_init(wtk_syndict_cfg_t *cfg);
int wtk_syndict_cfg_clean(wtk_syndict_cfg_t *cfg);
int wtk_syndict_cfg_update_local(wtk_syndict_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_syndict_cfg_update(wtk_syndict_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
