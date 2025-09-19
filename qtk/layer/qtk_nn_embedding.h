#ifndef __QTK_NN_EMBEDDING_H__
#define __QTK_NN_EMBEDDING_H__

#include "wtk/core/math/wtk_mat.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct {
    wtk_matf_t *embedding_table;
}qtk_nn_embedding_t;

qtk_nn_embedding_t *qtk_nn_embedding_new(int row,int col);
int qtk_nn_embedding_forward(qtk_nn_embedding_t *layer,wtk_veci_t *in,wtk_matf_t *out);
int qtk_nn_embedding_delete(qtk_nn_embedding_t *layer);
int qtk_nn_embedding_load_file(qtk_nn_embedding_t *layer, char *embedding_fn);

#ifdef __cplusplus
};
#endif

#endif