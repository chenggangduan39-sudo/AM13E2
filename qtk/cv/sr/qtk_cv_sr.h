#ifndef G_80E391D8E3B24B8D8559EAFC0284AC4E
#define G_80E391D8E3B24B8D8559EAFC0284AC4E

#include "qtk/cv/sr/qtk_cv_sr_cfg.h"
#include "qtk/nnrt/qtk_nnrt.h"

typedef struct qtk_cv_sr qtk_cv_sr_t;

struct qtk_cv_sr {
    qtk_cv_sr_cfg_t *cfg;
    qtk_nnrt_t *nnrt;
    float *input;
    int input_size;
    uint8_t *output;
    int output_size;
};

qtk_cv_sr_t *qtk_cv_sr_new(qtk_cv_sr_cfg_t *cfg);
void qtk_cv_sr_delete(qtk_cv_sr_t *m);
uint8_t *qtk_cv_sr_feed(qtk_cv_sr_t *m, uint8_t *img, int width, int height);

#endif
