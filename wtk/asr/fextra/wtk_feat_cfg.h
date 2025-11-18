#ifndef WTK_ASR_FEXTRA_WTK_FEAT_CFG
#define WTK_ASR_FEXTRA_WTK_FEAT_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_feat_cfg wtk_feat_cfg_t;
struct wtk_feat_cfg
{
	short sig_size;
	short dnn_size;
	unsigned use_dnn:1;
};

int wtk_feat_cfg_init(wtk_feat_cfg_t *cfg);
int wtk_feat_cfg_clean(wtk_feat_cfg_t *cfg);
int wtk_feat_cfg_update_local(wtk_feat_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_feat_cfg_update(wtk_feat_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
