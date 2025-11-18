#ifndef __QTK_LINALG_RANK_H__
#define __QTK_LINALG_RANK_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "wtk/core/wtk_type.h"
#include <math.h>
#include <stdio.h>

//约化阶梯矩阵
float qtk_linalg_standard_echelon(float *matrix, const int row, const int col,
                                  int x, int y, int *total,
                                  float *original_matrix);

//计算矩阵的秩
int qtk_linalg_rank(float *matrix, const int row, const int col,
                    float *echelon_matrix, int *total, float *original_matrix);

#ifdef __cplusplus
};
#endif
#endif
