#ifndef __QTK_NN_BATCHNORM_H__
#define __QTK_NN_BATCHNORM_H__

#include "wtk/core/math/wtk_mat.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_nn_batchnorm{
    wtk_vecf_t *gamma;
    wtk_vecf_t *beta;
    wtk_vecf_t *mean;
    wtk_vecf_t *variance;
    float epsilon;
}qtk_nn_batchnorm_t;

qtk_nn_batchnorm_t* qtk_nn_batchnorm_new(int n,float );
int qtk_nn_batchnorm_forward_inplace(qtk_nn_batchnorm_t *batchnorm,wtk_matf_t *in);
int qtk_nn_batchnorm_delete(qtk_nn_batchnorm_t *batchnorm);
int qtk_nn_batchnorm_load_file(qtk_nn_batchnorm_t *batchnorm,char *gamma,char *beta,char *mean,char *var);


#ifdef __cplusplus
};
#endif

#endif