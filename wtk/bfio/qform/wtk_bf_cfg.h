#ifndef WTK_BFIO_QFORM_WTK_BF_CFG
#define WTK_BFIO_QFORM_WTK_BF_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_bf_cfg wtk_bf_cfg_t;

struct wtk_bf_cfg
{
	int rate;
	float speed;
	int nmic;
	int real_nmic;
	float **mic_pos;
	float eye;
	float sd_eye;

	float mu;
	int qrank;

	float eig_iter_eps;
	int eig_iter_max_iter;

	float post_gramma;

	unsigned use_ncov_eig:1;
	unsigned use_scov_evd:1;
	unsigned use_eig_ovec:1;

	unsigned use_mvdr:1;
	unsigned use_gev:1;
	unsigned use_sdw_mwf:1;
	unsigned use_r1_mwf:1;
	unsigned use_vs:1;

	unsigned use_post:1;
	unsigned use_norm:1;
	unsigned use_ovec_norm:1;
};

int wtk_bf_cfg_init(wtk_bf_cfg_t *cfg);
int wtk_bf_cfg_clean(wtk_bf_cfg_t *cfg);
int wtk_bf_cfg_update_local(wtk_bf_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_bf_cfg_update(wtk_bf_cfg_t *cfg);

void wtk_bf_cfg_set_channel(wtk_bf_cfg_t *cfg, int channel);
#ifdef __cplusplus
};
#endif
#endif
