#ifndef SDK_AUDIO_QTK_AUIN_CFG
#define SDK_AUDIO_QTK_AUIN_CFG

#include "wtk/core/cfg/wtk_local_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_auin_cfg qtk_auin_cfg_t;
struct qtk_auin_cfg
{
	unsigned debug:1;
	unsigned err_exit:1;
};

int qtk_auin_cfg_init(qtk_auin_cfg_t *cfg);
int qtk_auin_cfg_clean(qtk_auin_cfg_t *cfg);
int qtk_auin_cfg_update_local(qtk_auin_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_auin_cfg_update(qtk_auin_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
