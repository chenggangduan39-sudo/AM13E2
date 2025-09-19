#ifndef QTK_GCC_ESTIMATE
#define QTK_GCC_ESTIMATE
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/core/fft/wtk_rfft.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_gcc_estimate qtk_gcc_estimate_t;

struct qtk_gcc_estimate {
    wtk_strbuf_t *mic;
    wtk_strbuf_t *play;    
    float *xn_;
    float *yn_;
    wtk_drft_t *drft;
    wtk_complex_t *xk_conj;
    wtk_complex_t *yk_;
    float *res;

    int st;
    int N;
    int n;
    int fs;
    
    int recommend_delay;
};

qtk_gcc_estimate_t *qtk_gcc_estimate_new(int rate);
void qtk_gcc_estimate_delete(qtk_gcc_estimate_t *d_est);
void qtk_gcc_estimate_reset(qtk_gcc_estimate_t *d_est);
int qtk_gcc_estimate_feed_float(qtk_gcc_estimate_t *d_est, float *data, int len, int is_end);
int qtk_gcc_estimate_feed(qtk_gcc_estimate_t *d_est, short *data, int len, int is_end);
int qtk_gcc_estimate(qtk_gcc_estimate_t *d_est);
#ifdef __cplusplus
};
#endif
#endif
