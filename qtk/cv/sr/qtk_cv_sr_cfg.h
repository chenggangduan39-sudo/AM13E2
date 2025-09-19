#ifndef G_CA40A54E69C54B1E8BEF2501957F7A5B
#define G_CA40A54E69C54B1E8BEF2501957F7A5B

#include "qtk/nnrt/qtk_nnrt_cfg.h"
#include "wtk/core/cfg/wtk_local_cfg.h"

typedef struct qtk_cv_sr_cfg qtk_cv_sr_cfg_t;

struct qtk_cv_sr_cfg {
    qtk_nnrt_cfg_t nnrt;
};

int qtk_cv_sr_cfg_init(qtk_cv_sr_cfg_t *cfg);
int qtk_cv_sr_cfg_clean(qtk_cv_sr_cfg_t *cfg);
int qtk_cv_sr_cfg_update(qtk_cv_sr_cfg_t *cfg);
int qtk_cv_sr_cfg_update_local(qtk_cv_sr_cfg_t *cfg, wtk_local_cfg_t *lc);
int qtk_cv_sr_cfg_update2(qtk_cv_sr_cfg_t *cfg, wtk_source_loader_t *sl);

#endif
