#ifndef __QTK_NN_LAYERNORM_H__
#define __QTK_NN_LAYERNORM_H__

#include "tts-mer/wtk-extend/wtk_mat2.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_nn_layernorm{
    float eps;
    wtk_vecf_t *gamm;
    wtk_vecf_t *beta;
    int use_1dim;
}qtk_nn_layernorm_t;

qtk_nn_layernorm_t* qtk_nn_layernorm_new(int len,float eps,int use_1dim);
int qtk_nn_layernorm_forward_inplace(qtk_nn_layernorm_t *layer,wtk_matf_t *in);
int qtk_nn_layernorm_delete(qtk_nn_layernorm_t *);
int qtk_nn_layernorm_load_file(qtk_nn_layernorm_t *layer, char *gamma_fn, char *beta_fn);

#ifdef __cplusplus
};
#endif

#endif
