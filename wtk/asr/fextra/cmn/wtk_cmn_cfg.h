#ifndef WTK_CYBER_CODE_WTK_CMN_CFG_H_
#define WTK_CYBER_CODE_WTK_CMN_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/math/wtk_vector.h"
#include "wtk/core/math/wtk_math.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_cmn_cfg wtk_cmn_cfg_t;
struct wtk_cmn_cfg
{
	char *cmn_fn;			//!< cepstral mean normalize file name.
	wtk_vector_t *cmn_def;	//!< default cmn vector.
	int start_min_frame;			//!< min frame to update mean vector.
	int post_left_frame;
	int post_update_frame;
	int post_update_frame2;
	int left_seek_frame;
	int min_flush_frame;
	int max_cmn_frame;
	float alpha;
	unsigned use_whole:1;
	unsigned smooth:1;
	unsigned save_cmn:1;
	unsigned use_hist:1;
};

int wtk_cmn_cfg_init(wtk_cmn_cfg_t *cfg);
int wtk_cmn_cfg_clean(wtk_cmn_cfg_t *cfg);
int wtk_cmn_cfg_update_local(wtk_cmn_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_cmn_cfg_update(wtk_cmn_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif
