#ifndef __QTK_NN_COMM_H__
#define __QTK_NN_COMM_H__

#ifdef __cplusplus
extern "C"{
#endif

typedef enum{
    QTK_NN_ACTINATION_NULL,
    QTK_NN_ACTINATION_RULE,
    QTK_NN_ACTINATION_SOFTMAX,
    QTK_NN_ACTINATION_SIGMOID,
    QTK_NN_ACTINATION_TANH,
    QTK_NN_ACTINATION_SOFTPLUS,
}QTK_NN_ACTINATION_TYPE_T;

typedef void (*qtk_nn_activation_f)(float *, int len);

#ifdef __cplusplus
};
#endif

#endif