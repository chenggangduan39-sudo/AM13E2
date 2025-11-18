#include "nnet.h"
#include "arch.h"
#include "common.h"
#include "opus_types.h"
#include "tansig_table.h"
#include "wtk/core/math/wtk_math.h"
#define SOFTMAX_HACK

// #ifdef __AVX__
// #include "vec_avx.h"
// // #elif __ARM_NEON__
// // #include "vec_neon.h"
// #else
// #warning Compiling without any vectorization. This code will be very slow
// // #include "vec.h"
// #endif

static OPUS_INLINE float relu(float x) { return x < 0 ? 0 : x; }

static void sgemv_accum16(float *out, const float *weights, int rows, int cols,
                          int col_stride, const float *x) {
  int i, j;
  for (i = 0; i < rows; i += 16) {
    for (j = 0; j < cols; j++) {
      // const float * restrict w;
      const float *w;
      // float * restrict y ;
      float *y;
      float xj;
      w = &weights[j * col_stride + i];
      xj = x[j];
      y = &out[i];
      y[0] += w[0] * xj;
      y[1] += w[1] * xj;
      y[2] += w[2] * xj;
      y[3] += w[3] * xj;
      y[4] += w[4] * xj;
      y[5] += w[5] * xj;
      y[6] += w[6] * xj;
      y[7] += w[7] * xj;
      y[8] += w[8] * xj;
      y[9] += w[9] * xj;
      y[10] += w[10] * xj;
      y[11] += w[11] * xj;
      y[12] += w[12] * xj;
      y[13] += w[13] * xj;
      y[14] += w[14] * xj;
      y[15] += w[15] * xj;
    }
  }
}

static void sgemv_accum(float *out, const float *weights, int rows, int cols,
                        int col_stride, const float *x) {
  int i, j;
  if (rows % 16 == 0) {
    sgemv_accum16(out, weights, rows, cols, col_stride, x);
  } else {
    for (i = 0; i < rows; i++) {
      for (j = 0; j < cols; j++)
        out[i] += weights[j * col_stride + i] * x[j];
    }
  }
}

static float tansig_approx(float x) {
  int i;
  float y, dy;
  float sign = 1;
  if (x < 0) {
    x = -x;
    sign = -1;
  }
  i = (int)floor(.5f + 25 * x);
  i = IMAX(0, IMIN(200, i));
  x -= .04f * i;
  y = tansig_table[i];
  dy = 1 - y * y;
  y = y + x * dy * (1 - y * x);
  return sign * y;
}

static OPUS_INLINE float sigmoid_approx(float x) {
  return .5f + .5f * tansig_approx(.5f * x);
}

static void vec_sigmoid(float *y, const float *x, int N) {
  int i;
  for (i = 0; i < N; i++) {
    y[i] = sigmoid_approx(x[i]);
  }
}

static void vec_tanh(float *y, const float *x, int N) {
  int i;
  for (i = 0; i < N; i++) {
    y[i] = tansig_approx(x[i]);
  }
}

void compute_activation(float *output, float *input, int N, int activation) {
  int i;
  if (activation == ACTIVATION_SIGMOID) {
    vec_sigmoid(output, input, N);
  } else if (activation == ACTIVATION_TANH) {
    vec_tanh(output, input, N);
  } else if (activation == ACTIVATION_RELU) {
    for (i = 0; i < N; i++)
      output[i] = relu(input[i]);
  } else if (activation == ACTIVATION_SOFTMAX) {
#ifdef SOFTMAX_HACK
    for (i = 0; i < N; i++)
      output[i] = input[i];
#else
    float sum = 0;
    softmax(output, input, N);
    for (i = 0; i < N; i++) {
      sum += output[i];
    }
    sum = 1.f / (sum + 1e-30);
    for (i = 0; i < N; i++)
      output[i] = sum * output[i];
#endif
  } else {
    celt_assert(activation == ACTIVATION_LINEAR);
    for (i = 0; i < N; i++)
      output[i] = input[i];
  }
}

void compute_dense(const DenseLayer *layer, float *output, const float *input) {
  int i;
  int N, M;
  int stride;
  M = layer->nb_inputs;
  N = layer->nb_neurons;
  stride = N;
  celt_assert(input != output);
  for (i = 0; i < N; i++)
    output[i] = layer->bias[i];
  sgemv_accum(output, layer->input_weights, N, M, stride, input);
  compute_activation(output, output, N, layer->activation);
}

void compute_gru(const GRULayer *gru, float *state, const float *input) {
  int i;
  int N, M;
  int stride;
  float tmp[MAX_NEURONS];
  float z[MAX_NEURONS];
  float r[MAX_NEURONS];
  float h[MAX_NEURONS];
  celt_assert(gru->nb_neurons <= MAX_NEURONS);
  celt_assert(input != state);
  M = gru->nb_inputs;
  N = gru->nb_neurons;
  stride = 3 * N;
  /* Compute update gate. */
  for (i = 0; i < N; i++)
    z[i] = gru->bias[i];
  if (gru->reset_after) {
    for (i = 0; i < N; i++)
      z[i] += gru->bias[3 * N + i];
  }
  sgemv_accum(z, gru->input_weights, N, M, stride, input);
  sgemv_accum(z, gru->recurrent_weights, N, N, stride, state);
  compute_activation(z, z, N, ACTIVATION_SIGMOID);

  /* Compute reset gate. */
  for (i = 0; i < N; i++)
    r[i] = gru->bias[N + i];
  if (gru->reset_after) {
    for (i = 0; i < N; i++)
      r[i] += gru->bias[4 * N + i];
  }
  sgemv_accum(r, &gru->input_weights[N], N, M, stride, input);
  sgemv_accum(r, &gru->recurrent_weights[N], N, N, stride, state);
  compute_activation(r, r, N, ACTIVATION_SIGMOID);

  /* Compute output. */
  for (i = 0; i < N; i++)
    h[i] = gru->bias[2 * N + i];
  if (gru->reset_after) {
    for (i = 0; i < N; i++)
      tmp[i] = gru->bias[5 * N + i];
    sgemv_accum(tmp, &gru->recurrent_weights[2 * N], N, N, stride, state);
    for (i = 0; i < N; i++)
      h[i] += tmp[i] * r[i];
    sgemv_accum(h, &gru->input_weights[2 * N], N, M, stride, input);
  } else {
    for (i = 0; i < N; i++)
      tmp[i] = state[i] * r[i];
    sgemv_accum(h, &gru->input_weights[2 * N], N, M, stride, input);
    sgemv_accum(h, &gru->recurrent_weights[2 * N], N, N, stride, tmp);
  }
  compute_activation(h, h, N, gru->activation);
  for (i = 0; i < N; i++)
    h[i] = z[i] * state[i] + (1 - z[i]) * h[i];
  for (i = 0; i < N; i++)
    state[i] = h[i];
}

void compute_conv1d(const Conv1DLayer *layer, float *output, float *mem,
                    const float *input) {
  int i;
  int N, M;
  int stride;
  float tmp[MAX_CONV_INPUTS];
  celt_assert(input != output);
  celt_assert(layer->nb_inputs * layer->kernel_size <= MAX_CONV_INPUTS);
  RNN_COPY(tmp, mem, layer->nb_inputs * (layer->kernel_size - 1));
  RNN_COPY(&tmp[layer->nb_inputs * (layer->kernel_size - 1)], input,
           layer->nb_inputs);
  M = layer->nb_inputs * layer->kernel_size;
  N = layer->nb_neurons;
  stride = N;
  for (i = 0; i < N; i++)
    output[i] = layer->bias[i];
  sgemv_accum(output, layer->input_weights, N, M, stride, tmp);
  compute_activation(output, output, N, layer->activation);
  RNN_COPY(mem, &tmp[layer->nb_inputs],
           layer->nb_inputs * (layer->kernel_size - 1));
}
