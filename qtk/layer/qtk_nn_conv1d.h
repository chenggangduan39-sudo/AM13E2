#ifndef __QTK_NN_CONV1D_H__
#define __QTK_NN_CONV1D_H__

//#include "wtk/core/math/wtk_mat.h"
#ifdef __cplusplus
extern "C"{
#endif
typedef struct qtk_nn_conv1d {
    wtk_matf_t *kernel; //(out_dim,in_dim*size)
    wtk_vecf_t *bias;       //bias
    int out_dim;
    int in_dim;
    int size;
    int stride;
    unsigned padding;
}qtk_nn_conv1d_t;

qtk_nn_conv1d_t *qtk_nn_conv1d_new(int in_dim,int out_dim,int kernel_size,int padding,int bias);
int qtk_nn_conv1d_forward(qtk_nn_conv1d_t *conv, wtk_matf_t *in, wtk_matf_t *out);
int qtk_nn_conv1d_forward_blk(qtk_nn_conv1d_t *conv, wtk_matf_t *in, wtk_matf_t *out);
int qtk_nn_conv1d_load_file(qtk_nn_conv1d_t *conv,char *kernel_fn,char *bias_fn);
int qtk_nn_conv1d_delete(qtk_nn_conv1d_t *conv);
int qtk_nn_conv1d_out_row(qtk_nn_conv1d_t *conv,int in_row);
#ifdef __cplusplus
};
#endif
#endif
