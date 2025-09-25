#ifndef QTK_LINEAR_CONV
#define QTK_LINEAR_CONV
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/core/wtk_heap.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_linear_conv qtk_linear_conv_t;

struct qtk_linear_conv {
    wtk_drft_t *drft;
    wtk_complex_t *weight;
    wtk_complex_t *OUTPUT_CACHE;
    wtk_complex_t *out;
    float *output_cache;
    float *win;
    float *in;
    int hop_size;
    int len_weight;
    int B;
    int N_BLK;
};

qtk_linear_conv_t *qtk_linear_conv_new(float *weight, int len, int hop_size);
qtk_linear_conv_t *qtk_linear_conv_new2(float *weight, int len, int hop_size,float rt60);
void qtk_linear_conv_delete(qtk_linear_conv_t *lc);
void qtk_linear_conv_reset(qtk_linear_conv_t *lc);
void qtk_linear_conv_conv1d_calc(qtk_linear_conv_t* lc, float *input);
int qtk_linear_conv_idx_find(float *weight, int len);
#ifdef __cplusplus
};
#endif
#endif
