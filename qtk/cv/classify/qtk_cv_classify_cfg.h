
#ifndef G_D05B7AAA3D3646A2A9187FDD0AFBE275
#define G_D05B7AAA3D3646A2A9187FDD0AFBE275
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "qtk/nnrt/qtk_nnrt_cfg.h"

typedef struct qtk_cv_classify_cfg qtk_cv_classify_cfg_t;

struct qtk_cv_classify_cfg {
    int width;
    int height;
    float *mean;
    float *std;
    wtk_string_t **labels;
    int nlabels;
    qtk_nnrt_cfg_t nnrt;

};

int qtk_cv_classify_cfg_init(qtk_cv_classify_cfg_t *cfg);
int qtk_cv_classify_cfg_clean(qtk_cv_classify_cfg_t *cfg);
int qtk_cv_classify_cfg_update(qtk_cv_classify_cfg_t *cfg);
int qtk_cv_classify_cfg_update_local(qtk_cv_classify_cfg_t *cfg, wtk_local_cfg_t *lc);
int qtk_cv_classify_cfg_update2(qtk_cv_classify_cfg_t *cfg, wtk_source_loader_t *sl);

#endif
