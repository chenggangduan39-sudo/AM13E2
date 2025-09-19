#ifndef WTK_VITE_MODEL_WTK_HMMSET_CFG_H_
#define WTK_VITE_MODEL_WTK_HMMSET_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/wtk_label.h"
//#include "wtk_hmmset.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_hmmset_cfg wtk_hmmset_cfg_t;
struct wtk_hmmset;

struct wtk_hmmset_cfg
{
	char *hmmlist_fn;
	char *hmm_fn;
	int hmmlist_hint;
	//-------------------------
	//wtk_hmmset_t *hmmset;
	struct wtk_hmmset *hmmset;
	float max_var;
	float max_mean;
	int max_var_i;
	int max_mean_i;
	float var_scale;
	float mean_scale;
	unsigned use_le:1;	//MMF is little endian or not;
	unsigned use_list_bin:1;
	unsigned use_fix:1;
};

int wtk_hmmset_cfg_init(wtk_hmmset_cfg_t *cfg);
int wtk_hmmset_cfg_clean(wtk_hmmset_cfg_t *cfg);
int wtk_hmmset_cfg_update_local(wtk_hmmset_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_hmmset_cfg_update(wtk_hmmset_cfg_t *cfg,wtk_label_t *label);
int wtk_hmmset_cfg_update2(wtk_hmmset_cfg_t *cfg,wtk_label_t *label,wtk_source_loader_t *sl);
int wtk_hmmset_cfg_bytes(wtk_hmmset_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
