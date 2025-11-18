#ifndef WTK_ASR_VAD_F0VAD_WTK_F0VAD_CFG
#define WTK_ASR_VAD_F0VAD_WTK_F0VAD_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk/asr/fextra/f0/wtk_f0.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_f0vad_cfg wtk_f0vad_cfg_t;
struct wtk_f0vad_cfg
{
	wtk_f0_cfg_t f0;
};

int wtk_f0vad_cfg_init(wtk_f0vad_cfg_t *cfg);
int wtk_f0vad_cfg_clean(wtk_f0vad_cfg_t *cfg);
int wtk_f0vad_cfg_update_local(wtk_f0vad_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_f0vad_cfg_update(wtk_f0vad_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
