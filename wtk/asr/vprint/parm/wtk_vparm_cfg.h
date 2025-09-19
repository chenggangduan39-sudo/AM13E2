#ifndef WTK_VITE_VPRINT_WTK_VPARM_CFG
#define WTK_VITE_VPRINT_WTK_VPARM_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk/asr/vad/wtk_vad.h"
#include "wtk/asr/model/wtk_hmmset_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_vparm_cfg wtk_vparm_cfg_t;
struct wtk_vparm_cfg
{
	wtk_vad_cfg_t vad2;
	wtk_fextra_cfg_t parm;
	wtk_hmmset_cfg_t hmmset;
	wtk_hmm_t *speech;
	wtk_label_t *label;
	double min_log_exp;
	unsigned use_vad:1;
	int skip_frame;
};

int wtk_vparm_cfg_init(wtk_vparm_cfg_t *cfg);
int wtk_vparm_cfg_clean(wtk_vparm_cfg_t *cfg);
int wtk_vparm_cfg_update_local(wtk_vparm_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_vparm_cfg_update(wtk_vparm_cfg_t *cfg);
int wtk_vparm_cfg_update2(wtk_vparm_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif
