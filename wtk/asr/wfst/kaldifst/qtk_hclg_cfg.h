#ifndef QTK_HCLG_CFG_H_
#define QTK_HCLG_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_hclg_cfg qtk_hclg_cfg_t;


struct qtk_hclg_cfg
{
	char* hclg_fst_fn;
};

int qtk_hclg_cfg_init(qtk_hclg_cfg_t *cfg);
int qtk_hclg_cfg_clean(qtk_hclg_cfg_t *cfg);
int qtk_hclg_cfg_update_local(qtk_hclg_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_hclg_cfg_update(qtk_hclg_cfg_t *cfg);
void qtk_hclg_cfg_print(qtk_hclg_cfg_t *cfg);

#ifdef __cplusplus
};
#endif
#endif
