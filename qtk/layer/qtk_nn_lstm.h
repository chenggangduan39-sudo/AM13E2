#ifndef __QTK_NN_LSTM_H__
#define __QTK_NN_LSTM_H__

#include "wtk/core/math/wtk_mat.h"
#include "qtk_nn_comm.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_nn_lstm{
    wtk_matf_t *kernel;
    wtk_vecf_t *bias;
    wtk_matf_t *lstm_in;
    wtk_matf_t *lstm_out;
    wtk_vecf_t *prev_c;
    wtk_vecf_t *prev_h;
    wtk_vecf_t *new_c;
    wtk_vecf_t *new_h;
    wtk_vecf_t *i;
    wtk_vecf_t *j;
    wtk_vecf_t *f;
    wtk_vecf_t *o;
    wtk_heap_t *heap;
    int is_forward;
    int lstm_units;
}qtk_nn_lstm_t;

qtk_nn_lstm_t* qtk_nn_lstm_new(int input_col, int hidden_size);
int qtk_nn_lstm_delete(qtk_nn_lstm_t *lstm);
int qtk_nn_lstm_reset(qtk_nn_lstm_t *lstm);
int qtk_nn_lstm_forward(qtk_nn_lstm_t *cell,wtk_matf_t *input,wtk_matf_t *output);
int qtk_nn_lstm_cell_forward(qtk_nn_lstm_t *lstm,wtk_matf_t *input,wtk_matf_t *output);
int qtk_nn_lstm_loadfile(qtk_nn_lstm_t *lstm,char *kernel_fn,char *bias_fn);


#ifdef __cplusplus
};
#endif

#endif