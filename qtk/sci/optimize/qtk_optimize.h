#ifndef QBL_SCI_OPTIMIZE_QBL_OPTIMIZE_H
#define QBL_SCI_OPTIMIZE_QBL_OPTIMIZE_H
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int qtk_optimize_linear_sum_assignment(int r, int c, float *cost, int *a,
                                       int *b);

#ifdef __cplusplus
};
#endif
#endif
