#ifndef QTK_DELAY_ESTIMATE
#define QTK_DELAY_ESTIMATE
#include "qtk_gcc_estimate.h"
#include "qtk_rir_estimate.h"
#include "qtk_rir_estimate2.h"
#include "qtk_white_noise_estimate.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_delay_estimate qtk_delay_estimate_t;

struct qtk_delay_estimate {
    qtk_rir_estimate_cfg_t *cfg;
    qtk_rir_estimate_t *rir_est;
    qtk_white_noise_estimate_t *wn_est;
    qtk_gcc_estimate_t *gcc_est;
    qtk_rir_estimate2_t *rir_est2;
    int recommend_delay;
    float *rir;  //计算的传递函数
    int rir_len; //传递函数的长度
};

qtk_delay_estimate_t *qtk_delay_estimate_new(qtk_rir_estimate_cfg_t *cfg);
void qtk_delay_estimate_delete(qtk_delay_estimate_t *d_est);
void qtk_delay_estimate_reset(qtk_delay_estimate_t *d_est);
int qtk_delay_estimate_feed_float(qtk_delay_estimate_t *d_est, float *data, int len, int is_end);
int qtk_delay_estimate_feed(qtk_delay_estimate_t *d_est, short *data, int len, int is_end);
int qtk_delay_estimate(qtk_delay_estimate_t *d_est);
int qtk_delay_estimate_playdata(qtk_delay_estimate_t *d_est, short **data);
#ifdef __cplusplus
};
#endif
#endif
