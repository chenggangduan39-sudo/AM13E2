#ifndef WTK_VAD_QVAD_WTK_QVAD_CFG
#define WTK_VAD_QVAD_WTK_QVAD_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_qvad_cfg wtk_qvad_cfg_t;
struct wtk_qvad_cfg
{
#ifdef WIN32
	unsigned pass : 1;
#endif
};

int wtk_qvad_cfg_init(wtk_qvad_cfg_t *cfg);
int wtk_qvad_cfg_clean(wtk_qvad_cfg_t *cfg);
int wtk_qvad_cfg_update_local(wtk_qvad_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_qvad_cfg_update(wtk_qvad_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif