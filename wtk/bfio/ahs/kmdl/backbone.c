#include "backbone.h"
#include "arch.h"
#include "common.h"
#include "config.h"
#include "nnet.h"
#include "opus_types.h"
#include "tansig_table.h"
#include "wtk/core/math/wtk_math.h"


#define FEAT_DIM 2 * NB_BANDS + 2
#define DENSE1_OUT_DIM 32
#define CONV1_OUT_DIM 32
#define CONV2_OUT_DIM 48
#define GRU1_OUT_DIM 96
#define GRU2_OUT_DIM 128

#if WITH_REFERENCE
#define REF_DIM NB_BANDS
#define FC2_OUT_DIM 32
#define CONV3_OUT_DIM 32
#define GRU3_OUT_DIM 96
#define GRU4_OUT_DIM 64
#endif

void compute_rnn_lite(RNNState *rnn, float *gains, const float *input) {

  int i;

  // main branch
  float dense_out[DENSE1_OUT_DIM];
  float first_conv1d_out[CONV1_OUT_DIM];
  compute_dense(rnn->model->fc1, dense_out, input);

  compute_conv1d(rnn->model->conv1, first_conv1d_out, rnn->first_conv1d_state,
                 dense_out);

  float second_conv1d_out[CONV2_OUT_DIM];
  compute_conv1d(rnn->model->conv2, second_conv1d_out, rnn->second_conv1d_state,
                 first_conv1d_out);
  compute_gru(rnn->model->gru1, rnn->gru1_state, second_conv1d_out);

  // concat for gru2 input
  float gru2_input[CONV2_OUT_DIM + GRU1_OUT_DIM];
  for (i = 0; i < CONV2_OUT_DIM; i++) {
    gru2_input[i] = second_conv1d_out[i];
  }

  for (i = 0; i < GRU1_OUT_DIM; i++) {
    gru2_input[i + CONV2_OUT_DIM] = rnn->gru1_state[i];
  }
  compute_gru(rnn->model->gru2, rnn->gru2_state, gru2_input);

  // compute gb
  float gb[NB_BANDS];
  compute_dense(rnn->model->fc_gb, gb, rnn->gru2_state);
  for (i = 0; i < NB_BANDS; i++) {
    gains[i] = gb[i];
  };

// refine IRM by reference
#if WITH_REFERENCE
  float ref[REF_DIM];
  for (i = 0; i < REF_DIM; i++) {
    ref[i] = input[FEAT_DIM + i];
  }

  float fc2_out[FC2_OUT_DIM];
  compute_dense(rnn->model->fc2, fc2_out, ref);

  float third_conv1d_out[CONV3_OUT_DIM];
  compute_conv1d(rnn->model->conv3, third_conv1d_out, rnn->third_conv1d_state,
                 fc2_out);

  compute_gru(rnn->model->gru3, rnn->gru3_state, third_conv1d_out);

  // concat for gru4 input
  float gru4_input[NB_BANDS + GRU3_OUT_DIM + CONV3_OUT_DIM];
  for (i = 0; i < NB_BANDS; i++) {
    gru4_input[i] = gb[i];
  };

  for (i = 0; i < GRU3_OUT_DIM; i++) {
    gru4_input[i + NB_BANDS] = rnn->gru3_state[i];
  };

  for (i = 0; i < CONV3_OUT_DIM; i++) {
    gru4_input[i + NB_BANDS + GRU3_OUT_DIM] = third_conv1d_out[i];
  };

  compute_gru(rnn->model->gru4, rnn->gru4_state, gru4_input);

  float gb_refined[NB_BANDS];
  compute_dense(rnn->model->fc_gb2, gb_refined, rnn->gru4_state);
  for (i = 0; i < NB_BANDS; i++) {
    gains[i] = gb_refined[i];
  };
#endif
}
