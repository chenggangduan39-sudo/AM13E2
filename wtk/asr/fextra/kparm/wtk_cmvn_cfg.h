#ifndef WTK_KSR_PARM_WTK_CMVN_CFG
#define WTK_KSR_PARM_WTK_CMVN_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_cmvn_cfg wtk_cmvn_cfg_t;

struct wtk_cmvn_cfg
{
	int left_frame;
	int right_frame;
	int init_frame;
	float alpha;
	int alpha_frame;
	int fix_alpha;
	unsigned use_hist:1;
        unsigned use_array : 1;
        unsigned use_mean:1;
	unsigned use_var:1;
	unsigned use_online:1;
	unsigned use_save_cmn:1;
	unsigned use_sliding:1;
};

int wtk_cmvn_cfg_init(wtk_cmvn_cfg_t *cfg);
int wtk_cmvn_cfg_clean(wtk_cmvn_cfg_t *cfg);
int wtk_cmvn_cfg_update_local(wtk_cmvn_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_cmvn_cfg_update(wtk_cmvn_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
