#ifndef B846D278_F91E_CFBB_B46A_C6B465173D69
#define B846D278_F91E_CFBB_B46A_C6B465173D69

#include "qtk/nnrt/qtk_nnrt_cfg.h"
#include "wtk/core/cfg/wtk_local_cfg.h"

typedef struct qtk_cv_embedding_cfg qtk_cv_embedding_cfg_t;

struct qtk_cv_embedding_cfg {
    qtk_nnrt_cfg_t nnrt;
    int width;
    int height;
};

int qtk_cv_embedding_cfg_init(qtk_cv_embedding_cfg_t *cfg);
int qtk_cv_embedding_cfg_clean(qtk_cv_embedding_cfg_t *cfg);
int qtk_cv_embedding_cfg_update(qtk_cv_embedding_cfg_t *cfg);
int qtk_cv_embedding_cfg_update_local(qtk_cv_embedding_cfg_t *cfg,
                                      wtk_local_cfg_t *lc);
int qtk_cv_embedding_cfg_update2(qtk_cv_embedding_cfg_t *cfg,
                                 wtk_source_loader_t *sl);

#endif /* B846D278_F91E_CFBB_B46A_C6B465173D69 */
