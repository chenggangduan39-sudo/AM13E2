#ifndef WTK_NN_SPARSE_H
#define WTK_NN_SPARSE_H
#include "wtk/tts-mer/wtk-extend/wtk_tts_common.h"
#ifdef __cplusplus
extern "C" {
#endif

/* 定义了一种非常有效的稀疏结构, 非转置矩阵下按行, 以4的倍数进行mask */

typedef struct
{/* 稀疏矩阵
不转置, 稀疏块按行分块,  块大小是4的倍数
 */
    int *idx; // cols3, i1, i2, i3, cols4, i1,i2,i3,i4
    float *weight; // 需要根据idx索引使用
    float *weight_diag; // 矩阵对角线数据
    int cols;
    int size16; // 块数量
} wtk_nn_matf_sparse_t;

wtk_nn_matf_sparse_t *wtk_nn_matf_sparse_heap_new( wtk_heap_t *heap, wtk_matf_t *w);
void wtk_nn_matf_sparse_sgemm( wtk_nn_matf_sparse_t *kernel, wtk_matf_t *in, wtk_matf_t *out);

#ifdef __cplusplus
}
#endif
#endif