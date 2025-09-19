#ifndef QTK_RIR_ESTIMATE2
#define QTK_RIR_ESTIMATE2
#include "wtk/bfio/ahs/estimate/qtk_rir_estimate2_cfg.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/core/wtk_heap.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_rir_estimate2_conv1d qtk_rir_estimate2_conv1d_t;
typedef struct qtk_rir_estimate2 qtk_rir_estimate2_t;

struct qtk_rir_estimate2_conv1d{
    float *cache;
    float *weight;
    float *out;
    int l1;
    int l2;
    int b;
};

struct qtk_rir_estimate2 {
    qtk_rir_estimate2_cfg_t *cfg;

    wtk_strbuf_t **log_sweep;
    qtk_rir_estimate2_conv1d_t *conv_sweep;
    int *recommend_delay;
    int hop_size;
    int rate;
    int st;

    float *code;
    float *r_code;
    short *pv_code;
    float *window;
    float *res;
    int res_len;
};

qtk_rir_estimate2_t *qtk_rir_estimate2_new(qtk_rir_estimate2_cfg_t *cfg);
void qtk_rir_estimate2_delete(qtk_rir_estimate2_t *pem_afc_b);
void qtk_rir_estimate2_start(qtk_rir_estimate2_t *rir_es);
void qtk_rir_estimate2_reset(qtk_rir_estimate2_t *pem_afc_b);
int qtk_rir_estimate2_feed_float(qtk_rir_estimate2_t *rir_es, float *data, int len, int is_end);
int qtk_rir_estimate2_feed(qtk_rir_estimate2_t *rir_es, short *data, int len, int is_end);
int qtk_estimate2_rir(qtk_rir_estimate2_t *rir_es);
float *qtk_estimate2_code_generate(qtk_rir_estimate2_t *rir_es);
#ifdef __cplusplus
};
#endif
#endif
