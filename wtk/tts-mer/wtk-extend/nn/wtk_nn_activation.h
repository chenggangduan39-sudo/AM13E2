#ifndef WTK_NN_ACTIVATION_H
#define WTK_NN_ACTIVATION_H
#include "wtk/tts-mer/wtk-extend/wtk_tts_common.h"
#ifdef __cplusplus
extern "C" {
#endif

void wtk_nn_relu(float *p, int len);
void wtk_nn_softmax(float *a, int len);
float wtk_nn_sigmoid_inline(float f);
float wtk_nn_tanh_inline(float f);
void wtk_nn_sigmoid(float *a, int len);
void wtk_nn_tanh(float *a, int len);

void wtk_nn_relu_d(double *p, int len);
void wtk_nn_softplus(float *a, int len);

#ifdef __cplusplus
}
#endif
#endif