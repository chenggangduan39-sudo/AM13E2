#ifndef _NNET_H_
#define _NNET_H_

#define MAX_NEURONS 512
#define MAX_CONV_INPUTS 1536
#define CONV_DIM 512

#define ACTIVATION_LINEAR 0
#define ACTIVATION_SIGMOID 1
#define ACTIVATION_TANH 2
#define ACTIVATION_RELU 3
#define ACTIVATION_SOFTMAX 4

typedef struct {
  const float *bias;
  const float *input_weights;
  int nb_inputs;
  int nb_neurons;
  int activation;
} DenseLayer;

typedef struct {
  const float *bias;
  const float *input_weights;
  const float *recurrent_weights;
  int nb_inputs;
  int nb_neurons;
  int activation;
  int reset_after;
} GRULayer;

typedef struct {
  const float *bias;
  const float *input_weights;
  int nb_inputs;
  int kernel_size;
  int nb_neurons;
  int activation;
} Conv1DLayer;

void compute_activation(float *output, float *input, int N, int activation);

void compute_dense(const DenseLayer *layer, float *output, const float *input);

void compute_gru(const GRULayer *gru, float *state, const float *input);

void compute_conv1d(const Conv1DLayer *layer, float *output, float *mem,
                    const float *input);

#endif
