#ifndef G_7708CBFB3C434E0BA5430BAA3BC3C39C
#define G_7708CBFB3C434E0BA5430BAA3BC3C39C

#include "qtk/cv/classify/qtk_cv_classify_cfg.h"
#include "qtk/nnrt/qtk_nnrt.h"

typedef struct qtk_cv_classify qtk_cv_classify_t;

struct qtk_cv_classify {
    qtk_cv_classify_cfg_t *cfg;
    qtk_nnrt_t *nnrt;
    int H;
    int W;
    float *input;
};

qtk_cv_classify_t *qtk_cv_classify_new(qtk_cv_classify_cfg_t *cfg);
void qtk_cv_classify_delete(qtk_cv_classify_t *cls);
int qtk_cv_classify_feed(qtk_cv_classify_t *cls, uint8_t *img);
wtk_string_t *qtk_cv_classify_get_label(qtk_cv_classify_t *cls, int id);

#endif
