#ifndef __QTK_NN_MAX_POOL1D_H__
#define __QTK_NN_MAX_POOL1D_H__

#include "tts-mer/wtk-extend/wtk_mat2.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct{
    int kernel_size;
    int stride;
    int padding;
}qtk_nn_maxpool1d_t;

qtk_nn_maxpool1d_t* qtk_nn_maxpool1d_new(int kernel_size,int stride,int padding);
int qtk_nn_maxpool1d_forward(qtk_nn_maxpool1d_t *,wtk_matf_t *in,wtk_matf_t *out);
int qtk_nn_maxpool1d_delete(qtk_nn_maxpool1d_t *pool);
int qtk_nn_maxpool1d_out_row(qtk_nn_maxpool1d_t *pool,int in_row);

#ifdef __cplusplus
};
#endif 

#endif
