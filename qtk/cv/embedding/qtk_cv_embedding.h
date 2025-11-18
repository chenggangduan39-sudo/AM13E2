#ifndef A0F889BD_F20A_3395_734D_8C74CFE8C6BE
#define A0F889BD_F20A_3395_734D_8C74CFE8C6BE

#include "qtk/cv/embedding/qtk_cv_embedding_cfg.h"
#include "qtk/nnrt/qtk_nnrt.h"

typedef struct qtk_cv_embedding qtk_cv_embedding_t;

struct qtk_cv_embedding {
    qtk_cv_embedding_cfg_t *cfg;
    qtk_nnrt_t *nnrt;
    float *input;
    qtk_nnrt_value_t nnrt_output;
};

qtk_cv_embedding_t *qtk_cv_embedding_new(qtk_cv_embedding_cfg_t *cfg);
void qtk_cv_embedding_delete(qtk_cv_embedding_t *em);
int qtk_cv_embedding_process(qtk_cv_embedding_t *em, uint8_t *data,
                             float **embedding);

#endif /* A0F889BD_F20A_3395_734D_8C74CFE8C6BE */
