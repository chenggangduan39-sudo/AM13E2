#ifndef QTK_WHITE_NOISE_ESTIMATE
#define QTK_WHITE_NOISE_ESTIMATE
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "qtk_rir_estimate_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_white_noise_estimate qtk_white_noise_estimate_t;

struct qtk_white_noise_estimate {
    wtk_strbuf_t *mic;
    wtk_strbuf_t *play;
    float *xn_;
    float *yn_;
    wtk_drft_t *drft;
    wtk_complex_t *xk_conj;
    wtk_complex_t *yk_;
    float *ref;
    float *res;

    int st;
    float rt60;
    int N;
    int n;

    int recommend_delay;
    int hop_size;
    int rate;

    FILE *file;
};

qtk_white_noise_estimate_t *qtk_white_noise_estimate_new(qtk_rir_estimate_cfg_t* cfg);
void qtk_white_noise_estimate_delete(qtk_white_noise_estimate_t *d_est);
void qtk_white_noise_estimate_reset(qtk_white_noise_estimate_t *d_est);
int qtk_white_noise_estimate_feed_float(qtk_white_noise_estimate_t *d_est, float *data, int len, int is_end);
int qtk_white_noise_estimate_feed(qtk_white_noise_estimate_t *d_est, short *data, int len, int is_end);
int qtk_white_noise_estimate(qtk_white_noise_estimate_t *d_est);
void qtk_white_noise_estimate_ref_load(qtk_white_noise_estimate_t* d_est,char *fn);
#ifdef __cplusplus
};
#endif
#endif
