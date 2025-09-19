#ifndef WTK_TTS_SYN_WTK_SYN_DWIN_CFG
#define WTK_TTS_SYN_WTK_SYN_DWIN_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_syn_dwin_cfg wtk_syn_dwin_cfg_t;

struct wtk_syn_dwin_cfg
{
	wtk_array_t *fn;
};

int wtk_syn_dwin_cfg_init(wtk_syn_dwin_cfg_t *cfg);
int wtk_syn_dwin_cfg_clean(wtk_syn_dwin_cfg_t *cfg);
int wtk_syn_dwin_cfg_update_local(wtk_syn_dwin_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_syn_dwin_cfg_update(wtk_syn_dwin_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
