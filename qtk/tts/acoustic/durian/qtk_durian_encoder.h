#ifndef __QTK_DURIAN_ENCODER_H__
#define __QTK_DURIAN_ENCODER_H__

#include "qtk_durian_encoder_cfg.h"
#include "wtk/core/math/wtk_mat.h"
#include "tts-mer/wtk-extend/nn/wtk_nn.h"
#include "layer/qtk_nn_conv1d.h"
#include "layer/qtk_nn_batchnorm.h"
#include "tts-mer/wtk-extend/wtk_heap2.h"
#include "layer/qtk_nn_maxpool1d.h"
#include "layer/qtk_nn_fc.h"
#include "layer/qtk_nn_bigru.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_durian_batchnormconv{
    qtk_nn_conv1d_t *conv1d;
    qtk_nn_batchnorm_t *batchnorm;
}qtk_durian_batchnormconv_t;

typedef struct qtk_durian_highways{
    qtk_nn_fc_t *H;
    qtk_nn_fc_t *T;
}qtk_durian_highway_t;

typedef struct qtk_durian_encoder{
    wtk_heap_t *heap;
    qtk_durian_encoder_cfg_t *cfg;

    qtk_durian_batchnormconv_t **conv1d_back;
    qtk_nn_maxpool1d_t *maxpool;
    qtk_durian_batchnormconv_t **conv1d_projections;
    qtk_nn_fc_t *pre_highway;
    qtk_durian_highway_t **highway;
    qtk_nn_bigru_t *bigru;
}qtk_durian_encoder_t;

qtk_durian_encoder_t* qtk_durian_encoder_new(qtk_durian_encoder_cfg_t *cfg,wtk_heap_t *heap);
int qtk_durian_encoder_delete(qtk_durian_encoder_t *);
wtk_matf_t* qtk_durian_encoder_process(qtk_durian_encoder_t *enc,wtk_matf_t *input);

////////batch norm conv1d
qtk_durian_batchnormconv_t* qtk_durian_batchnormconv_new(int ,int ,int ,int ,int );
int qtk_durian_batchnormconv_delete(qtk_durian_batchnormconv_t *bnc);
int qtk_durian_batchnormconv_process(qtk_durian_batchnormconv_t *bnc, wtk_matf_t *in, 
                                                                                    wtk_matf_t *out);

int qtk_durian_batchnormconv_loadfile(qtk_durian_batchnormconv_t *bnc,
                                        char *conv1_kenrel_fn,char *conv1d_bias_fn,
                                        char *batchnorm_gamma_fn,char *batchnorm_beta_fn,
                                        char *batchnorm_mean_fn,char *batchnorm_var_fn);

//////////////////highway

qtk_durian_highway_t *qtk_durian_highway_new(int in_size,int out_size);
int qtk_durian_highway_delete(qtk_durian_highway_t *highway);
int qtk_durian_highwav_loadfile(qtk_durian_highway_t *highway, char *H_wfn,char *H_bfn,char *T_wfn,char *T_bfn);
int qtk_durian_highway_process(qtk_durian_highway_t *highway,wtk_matf_t *in,wtk_matf_t *out);

#ifdef __cplusplus
};
#endif

#endif
