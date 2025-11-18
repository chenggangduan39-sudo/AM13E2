#ifndef __QTK_NN_BIGRU_H__
#define __QTK_NN_BIGRU_H__

#include "qtk_nn_gru.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_nn_bigru{
    qtk_nn_gru_t *forward;
    qtk_nn_gru_t *reverse;
}qtk_nn_bigru_t;

qtk_nn_bigru_t* qtk_nn_bigru_new(int in_size,int num_dim,int use_bias);
int qtk_nn_bigru_delete(qtk_nn_bigru_t *bigru);
int qtk_nn_bigru_reset(qtk_nn_bigru_t *bigru);
int qtk_nn_bigru_forward(qtk_nn_bigru_t *bigru,wtk_matf_t *in,wtk_matf_t *out);
int qtk_nn_bigru_loadfile(qtk_nn_bigru_t *gru,char **gate_weight_fn,char **candidate_weight_fn,char **candidate_hh_weight_fn,
                                                    char **gate_bias_fn,char **candidate_bias_fn,char **candidate_hh_bias_fn);


#ifdef __cplusplus
};
#endif

#endif