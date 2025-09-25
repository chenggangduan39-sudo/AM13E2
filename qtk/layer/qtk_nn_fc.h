#ifndef __QTK_NN_FC_H__
#define __QTK_NN_FC_H__

#include "wtk/core/math/wtk_mat.h"
#include "qtk_nn_comm.h"

#ifdef __cplusplus
extern "C"{
#endif

/*
全连接层
*/
typedef struct {
    wtk_matf_t *kernel;
    wtk_vecf_t *bias;

    qtk_nn_activation_f activation;
    int use_kernel_trans;   //kernel 使用转置好的
}qtk_nn_fc_t;

qtk_nn_fc_t *qtk_nn_fc_new(int inl,int outl,QTK_NN_ACTINATION_TYPE_T actination_type, 
                                                                                                                        int use_bias, int use_kernel_trans);
int qtk_nn_fc_forward(qtk_nn_fc_t *layer,wtk_matf_t *in,wtk_matf_t *out);
int qtk_nn_fc_delete(qtk_nn_fc_t *layer);
int qtk_nn_fc_load_file(qtk_nn_fc_t *layer, char *weight, char *bias);


#ifdef __cplusplus
};
#endif

#endif