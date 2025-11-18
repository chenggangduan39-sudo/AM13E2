#ifndef QTK_RIR_ESTIMATE
#define QTK_RIR_ESTIMATE
#include "wtk/bfio/ahs/estimate/qtk_rir_estimate_cfg.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/core/wtk_heap.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_rir_estimate_conv1d qtk_rir_estimate_conv1d_t;
typedef struct qtk_rir_estimate qtk_rir_estimate_t;

struct qtk_rir_estimate_conv1d{
    float *cache;
    float *weight;
    float *out;
    int l1;
    int l2;
    int b;
};

struct qtk_rir_estimate {
    qtk_rir_estimate_cfg_t *cfg;

    wtk_strbuf_t *log_sweep;
    qtk_rir_estimate_conv1d_t *conv_sweep;
    int recommend_delay;
    int hop_size;
    int rate;
    int st;
};

qtk_rir_estimate_t *qtk_rir_estimate_new(qtk_rir_estimate_cfg_t *cfg, int hop_size, int rate);
void qtk_rir_estimate_delete(qtk_rir_estimate_t *pem_afc_b);
void qtk_rir_estimate_reset(qtk_rir_estimate_t *pem_afc_b);
int qtk_rir_estimate_feed_float(qtk_rir_estimate_t *rir_es, float *data, int len, int is_end);
int qtk_rir_estimate_feed(qtk_rir_estimate_t *rir_es, short *data, int len, int is_end);
int qtk_rir_estimate_feed2(qtk_rir_estimate_t *rir_es, short *data, int len, int is_end);
int qtk_rir_estimate_ref_write(qtk_rir_estimate_t* pem, char *fn);
int qtk_rir_estimate_ref_load(qtk_rir_estimate_t* pem, char *fn);
void qtk_rir_estimate_conv1d_calc(qtk_rir_estimate_t* rir_est, float *input);
int qtk_estimate_rir(qtk_rir_estimate_t *rir_es);
#ifdef __cplusplus
};
#endif
#endif