#include "config.h"

#ifndef NNET_DATA_H
#define NNET_DATA_H
#include "nnet.h"

typedef struct {
  const DenseLayer *fc1;
  const Conv1DLayer *conv1;
  const Conv1DLayer *conv2;
  const GRULayer *gru1;
  const GRULayer *gru2;
  const DenseLayer *fc_gb;

#if WITH_REFERENCE
  const DenseLayer *fc2;
  const Conv1DLayer *conv3;
  const GRULayer *gru3;
  const GRULayer *gru4;
  const DenseLayer *fc_gb2;
#endif
} RNNModel;

/*gru state must be initialized in  denoise.cpp -> rnnoise_create*/
typedef struct {
  const RNNModel *model;
  float *first_conv1d_state;
  float *second_conv1d_state;
  float *gru1_state;
  float *gru2_state;
#if WITH_REFERENCE
  float *third_conv1d_state;
  float *gru3_state;
  float *gru4_state;
#endif
} RNNState;
#endif

#ifndef BACKBONE_EXPORT
#if defined(WIN32)
#if defined(BACKBONE_BUILD) && defined(DLL_EXPORT)
#define BACKBONE_EXPORT __declspec(dllexport)
#else
#define BACKBONE_EXPORT
#endif
#elif defined(__GNUC__) && defined(BACKBONE_BUILD)
#define BACKBONE_EXPORT __attribute__((visibility("default")))
#else
#define BACKBONE_EXPORT
#endif
void compute_rnn_lite(RNNState *rnn, float *gains, const float *input);

#endif


