#ifndef __QTK_NN_GRU_H__
#define __QTK_NN_GRU_H__

#include "wtk/core/math/wtk_mat.h"
#include "qtk_nn_comm.h"

#ifdef __cplusplus
extern "C"{
#endif

//pytroch gru data
typedef struct{
    wtk_heap_t *heap;
    // wtk_nn_enum_type_t type;
    wtk_matf_t *gate_in;
    wtk_matf_t *gate_out;
    wtk_matf_t *gate_kernel;
    wtk_vecf_t *gate_bias;
    wtk_matf_t *candidate_in;
    wtk_matf_t *candidate_out;
    wtk_matf_t *candidate_out_hh;
    wtk_matf_t *candidate_kernel;
    wtk_matf_t *candidate_kernel_hh;
    wtk_vecf_t *candidate_bias;
    wtk_vecf_t *candidate_bias_hh;
    wtk_matf_t *weight_ih;
    wtk_matf_t *weight_hh;
    wtk_vecf_t *bias_ih;
    wtk_vecf_t *bias_hh;
    // wtk_nn_matf_sparse_t *weight_hh_smf;
    wtk_vecf_t *r_state;
    wtk_vecf_t *prev_h;
    wtk_vecf_t *new_h;
    wtk_vecf_t *r;
    wtk_vecf_t *u;
    // void (*activation)(float*, int);
    int is_forward;
    int num_units;
}qtk_nn_gru_t;

qtk_nn_gru_t* qtk_nn_gru_new(int input_size,int hidden_size,int use_bias);
int qtk_nn_gru_forward(qtk_nn_gru_t *gru, wtk_matf_t *in, wtk_matf_t *out);
int qtk_nn_gru_forward_cell(qtk_nn_gru_t *gru, wtk_matf_t *in, wtk_matf_t *out);
int qtk_nn_gru_delete(qtk_nn_gru_t*);
int qtk_nn_gru_reset(qtk_nn_gru_t *gru);
int qtk_nn_gru_loadfile(qtk_nn_gru_t *gru,char *gate_weight_fn,char *candidate_weight_fn,char *candidate_hh_weight_fn,
                                                    char *gate_bias_fn,char *candidate_bias_fn,char *candidate_hh_bias_fn);

#ifdef __cplusplus
};
#endif

#endif