#include "qtk/cv/embedding/qtk_cv_embedding.h"

qtk_cv_embedding_t *qtk_cv_embedding_new(qtk_cv_embedding_cfg_t *cfg) {
    qtk_cv_embedding_t *em;

    em = wtk_malloc(sizeof(qtk_cv_embedding_t));
    em->cfg = cfg;
    em->nnrt = qtk_nnrt_new(&cfg->nnrt);
    em->input = wtk_malloc(sizeof(float) * cfg->width * cfg->height * 3);
    em->nnrt_output = NULL;
    return em;
}

void qtk_cv_embedding_delete(qtk_cv_embedding_t *em) {
    wtk_free(em->input);
    if (em->nnrt_output) {
        qtk_nnrt_value_release(em->nnrt, em->nnrt_output);
    }
    qtk_nnrt_delete(em->nnrt);
    wtk_free(em);
}

int qtk_cv_embedding_process(qtk_cv_embedding_t *em, uint8_t *data,
                             float **embedding) {
    int i;
    int ret = -1;
    qtk_nnrt_value_t input;
    int n = em->cfg->width * em->cfg->height;
    int64_t shape[4] = {1, 3, em->cfg->height, em->cfg->width};
    if (em->nnrt_output) {
        qtk_nnrt_value_release(em->nnrt, em->nnrt_output);
    }
    for (i = 0; i < n; i++) {
        em->input[i] = (data[i * 3 + 0] - 127.5) / 128.0;
        em->input[i + n] = (data[i * 3 + 1] - 127.5) / 128;
        em->input[i + (n * 2)] = (data[i * 3 + 2] - 127.5) / 128;
    }
    input = qtk_nnrt_value_create_external(em->nnrt, QTK_NNRT_VALUE_ELEM_F32,
                                           shape, 4, em->input);
    qtk_nnrt_feed(em->nnrt, input, 0);
    qtk_nnrt_run(em->nnrt);
    qtk_nnrt_get_output(em->nnrt, &em->nnrt_output, 0);
    if (qtk_nnrt_value_get_shape(em->nnrt, em->nnrt_output, shape, 4) != 2) {
        goto end;
    }
    *embedding = qtk_nnrt_value_get_data(em->nnrt, em->nnrt_output);
    ret = shape[1];
end:
    qtk_nnrt_value_release(em->nnrt, input);
    return ret;
}
