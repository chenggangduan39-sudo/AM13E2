#include "qtk_nn_bigru.h"

qtk_nn_bigru_t* qtk_nn_bigru_new(int in_size,int num_dim,int use_bias)
{
    qtk_nn_bigru_t *bigru = NULL;
    bigru = wtk_malloc(sizeof(*bigru));
    bigru->forward = qtk_nn_gru_new(in_size,num_dim,use_bias);
    bigru->forward->is_forward = 1;
    bigru->reverse = qtk_nn_gru_new(in_size,num_dim,use_bias);
    bigru->reverse->is_forward = 0;
    return bigru;
}

int qtk_nn_bigru_delete(qtk_nn_bigru_t *bigru)
{
    qtk_nn_gru_delete(bigru->forward);
    qtk_nn_gru_delete(bigru->reverse);
    wtk_free(bigru);
    return 0;
}

int qtk_nn_bigru_reset(qtk_nn_bigru_t *bigru)
{
    qtk_nn_gru_reset(bigru->forward);
    qtk_nn_gru_reset(bigru->reverse);
    return 0;
}

int qtk_nn_bigru_forward(qtk_nn_bigru_t *bigru,wtk_matf_t *in,wtk_matf_t *out)
{
    int i = 0;

    wtk_matf_t *gru_fw = wtk_matf_new(in->row,bigru->forward->num_units);
    wtk_matf_t *gru_bw = wtk_matf_new(in->row,bigru->reverse->num_units);
    qtk_nn_gru_reset(bigru->forward);
    qtk_nn_gru_reset(bigru->reverse);
    qtk_nn_gru_forward(bigru->forward,in,gru_fw);
    qtk_nn_gru_forward(bigru->reverse,in,gru_bw);
    for(i = 0;i < in->row; ++i){
        memcpy(out->p+out->col*i,gru_fw->p+gru_fw->col*i,gru_fw->col*sizeof(float));
        memcpy(out->p+out->col*i+gru_fw->col,gru_bw->p+gru_bw->col*i,gru_bw->col*sizeof(float));
    }
    wtk_matf_delete(gru_fw);
    wtk_matf_delete(gru_bw);
    return 0;
}

int qtk_nn_bigru_loadfile(qtk_nn_bigru_t *bigru,char **gate_weight_fn,char **candidate_weight_fn,char **candidate_hh_weight_fn,
                                                    char **gate_bias_fn,char **candidate_bias_fn,char **candidate_hh_bias_fn)
{
    qtk_nn_gru_loadfile(bigru->forward,gate_weight_fn[0],candidate_weight_fn[0],candidate_hh_weight_fn[0],
                                                    gate_bias_fn[0],candidate_bias_fn[0],candidate_hh_bias_fn[0]);
    qtk_nn_gru_loadfile(bigru->reverse,gate_weight_fn[1],candidate_weight_fn[1],candidate_hh_weight_fn[1],
                                                    gate_bias_fn[1],candidate_bias_fn[1],candidate_hh_bias_fn[1]);
    return 0;
}
