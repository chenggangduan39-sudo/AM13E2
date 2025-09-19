#ifndef WTK_ASR_PARM_WTK_KXPARM_CFG
#define WTK_ASR_PARM_WTK_KXPARM_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/asr/fextra/wtk_fextra.h"
#include "wtk_kparm.h"
#include "wtk/asr/fextra/kparm/knn/wtk_knn.h"
#include "wtk/asr/fextra/nnet3/qtk_nnet3.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_kxparm_cfg wtk_kxparm_cfg_t;
struct wtk_kxparm_cfg
{
	wtk_fextra_cfg_t htk;
	wtk_kparm_cfg_t parm;
	wtk_knn_cfg_t knn;
	qtk_nnet3_cfg_t nnet3;
	unsigned use_htk:1;
	unsigned use_knn:1;
	unsigned use_nnet3:1;
};

int wtk_kxparm_cfg_bytes(wtk_kxparm_cfg_t *cfg);
int wtk_kxparm_cfg_init(wtk_kxparm_cfg_t *cfg);
int wtk_kxparm_cfg_clean(wtk_kxparm_cfg_t *cfg);
int wtk_kxparm_cfg_update_local(wtk_kxparm_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_kxparm_cfg_update(wtk_kxparm_cfg_t *cfg);
int wtk_kxparm_cfg_update2(wtk_kxparm_cfg_t *cfg,wtk_source_loader_t *sl);
int wtk_kxparm_cfg_get_win(wtk_kxparm_cfg_t *cfg);
int wtk_kxparm_cfg_get_rate(wtk_kxparm_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif

