#ifndef WTK_NN_H
#define WTK_NN_H
#include "tts-mer/wtk-extend/wtk_extend.h"
#include "tts-mer/wtk-extend/wtk_blas.h"
#include "wtk_nn_fft.h"
#include "wtk_nn_activation.h"
#include "wtk_nn_sparse.h"
#include "wtk_nn_pad.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    enum_conv_npod,
    enum_conv_same,
    enum_conv_valid,
} enum_conv;

typedef enum
{
    wtk_nn_enum_type_tensorflow,
    wtk_nn_enum_type_pytorch,
} wtk_nn_enum_type_t;

typedef struct
{/*  i = input_gate, j = new_input, f = forget_gate, o = output_gate */
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
    float forget_bias;
    int is_zoneout;
    float zoneout_cell;
    float zoneout_output;
    float (*activation)(float);
} wtk_nn_lstm_t;

typedef struct
{
    wtk_heap_t *heap;
    wtk_nn_enum_type_t type;
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
    wtk_nn_matf_sparse_t *weight_hh_smf;
    wtk_vecf_t *r_state;
    wtk_vecf_t *prev_h;
    wtk_vecf_t *new_h;
    wtk_vecf_t *r;
    wtk_vecf_t *u;
    void (*activation)(float*, int);
    int is_forward;
    int num_units;
} wtk_nn_rnngru_t;

#ifndef NORM_EPSILON
#define NORM_EPSILON    (1E-5)
#endif

/* tensorflow   tf.nn */
void wtk_nn_relu(float *p, int len);
void wtk_nn_softmax(float *p, int len);
float wtk_nn_sigmoid_inline(float f);

int wtk_nn_constant_pad1d(wtk_matf_t *in,wtk_matf_t *out,int left,int right);

void wtk_nn_layer_norm_1dim(wtk_matf_t *input,wtk_vecf_t *gamma,wtk_vecf_t *beta,float epsilon);
void wtk_nn_layer_norm(wtk_matf_t *input,wtk_vecf_t *gamma,wtk_vecf_t *beta,float epsilon);
void wtk_nn_batch_norm(wtk_matf_t *in, wtk_vecf_t *gamma, wtk_vecf_t *beta, wtk_vecf_t *mean, wtk_vecf_t *variance, float epsilon);
void wtk_nn_layer_dense( wtk_matf_t *input, wtk_matf_t *kernel, wtk_vecf_t *bias, void activation(float*, int), wtk_matf_t *output);
void wtk_nn_conv1d(wtk_matf_t *kernel, int kernel_size, enum_conv padding, wtk_matf_t *input, wtk_matf_t *dst);
void wtk_nn_conv1d2(wtk_matf_t *kernel, int kernel_size, enum_conv padding, wtk_matf_t *input, wtk_matf_t *dst);
int wtk_nn_conv1d_group(wtk_matf_t *kernel, int kernel_size,int groups, wtk_matf_t *input, wtk_matf_t *dst);
float wk_nn_arr_max_inline(float **p, int len, int i);
void wtk_nn_max_pool1d(int pool_size, int stride, enum_conv padding, wtk_matf_t *in, wtk_matf_t *dst);


wtk_nn_lstm_t* wtk_nn_lstm_new(int is_forward, int input_col, int lstm_units, float forget_bias, float activation(float), float zoneout[]);
void wtk_nn_lstm(wtk_nn_lstm_t *cell, wtk_matf_t *input, wtk_matf_t *output);
void wtk_nn_lstm_batch(wtk_nn_lstm_t *cell, int batch_size, wtk_matf_t *input, wtk_matf_t *output, wtk_matf_t *c_inmf, wtk_matf_t *h_inmf, wtk_matf_t *c_outmf, wtk_matf_t *h_outmf);
void wtk_nn_lstm_batch2(wtk_nn_lstm_t *cell, int batch_size, int *dim_arr, wtk_matf_t *input, wtk_matf_t *output, wtk_matf_t *c_inmf, wtk_matf_t *h_inmf, wtk_matf_t *c_outmf, wtk_matf_t *h_outmf);
void wtk_nn_lstm_cell(wtk_nn_lstm_t *cell, wtk_matf_t *input, wtk_matf_t *output);
void wtk_nn_lstm_multi_cell(wtk_nn_lstm_t **cell, int layer_len, wtk_matf_t *cell_in, wtk_matf_t *cell_out);
void wtk_nn_lstm_reset(wtk_nn_lstm_t *cell);
void wtk_nn_lstm_delete(wtk_nn_lstm_t *cell);

wtk_nn_rnngru_t* wtk_nn_rnngru_new(int is_forward, int incol, int num_units, void activation(float*, int));
wtk_nn_rnngru_t* wtk_nn_rnngru_new2( wtk_nn_enum_type_t type, int is_forward, int incol, int num_units, void activation(float*, int));
void wtk_nn_rnngru(wtk_nn_rnngru_t *cell, wtk_matf_t *in, wtk_matf_t *out);
void wtk_nn_rnngru_batch(wtk_nn_rnngru_t *cell, int batch_size, wtk_matf_t *in, wtk_matf_t *out, wtk_matf_t *h_inmf, wtk_matf_t *h_outmf);
void wtk_nn_rnngru_cell(wtk_nn_rnngru_t *cell, wtk_matf_t *in, wtk_matf_t *out);
void wtk_nn_rnngru_reset(wtk_nn_rnngru_t *cell);
void wtk_nn_rnngru_delete(wtk_nn_rnngru_t *cell);

#ifdef __cplusplus
}
#endif
#endif
