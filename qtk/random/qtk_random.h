#ifndef __QTK_RANDOM_H__
#define __QTK_RANDOM_H__

#include "wtk/core/wtk_alloc.h"
#ifdef __cplusplus
extern "C"{
#endif

//暂定为1024个值 为了保证使用的时候线程安全 保存使用
typedef struct qtk_random{
    unsigned i;
    int n;
    int s;
    int *nums;
}qtk_random_t;

qtk_random_t* qtk_random_new(int seed);
int qtk_random_rand(qtk_random_t* rand);
void qtk_random_reset(qtk_random_t *rand);
int qtk_random_delete(qtk_random_t *rand);


//int qtk_rand_normal_mat(qtk_blas_matrix_t* mat, float mean, float stdc);
#ifdef __cplusplus
};
#endif


#endif
