#ifndef WTK_EVAL_POST_STRESS_SVM_WTK_SVM_CFG_H_
#define WTK_EVAL_POST_STRESS_SVM_WTK_SVM_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk_svm.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_svm_cfg wtk_svm_cfg_t;
struct wtk_svm_cfg
{
	char *svm_fn;
	wtk_svm_t *svm;
};

int wtk_svm_cfg_init(wtk_svm_cfg_t *cfg);
int wtk_svm_cfg_clean(wtk_svm_cfg_t *cfg);
int wtk_svm_cfg_update_local(wtk_svm_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_svm_cfg_update(wtk_svm_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif
