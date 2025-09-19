#ifndef QTK_GAIN_ESTIMATE
#define QTK_GAIN_ESTIMATE
#include "qtk_rir_estimate2.h"
#include "qtk_rir_estimate_cfg.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_strbuf.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_gain_estimate qtk_gain_estimate_t;

struct qtk_gain_estimate {
    wtk_strbuf_t *mic;
    wtk_strbuf_t *play;
    wtk_drft_t *drft;
    float gain_mic;
    float gain_echo;
    wtk_complex_t *weight;
    float snr;
    float Decay;
    float rt60;
};

qtk_gain_estimate_t *qtk_gain_estimate_new();
void qtk_gain_estimate_delete(qtk_gain_estimate_t *g_est);
void qtk_gain_estimate_reset(qtk_gain_estimate_t *g_est);
int qtk_gain_estimate_feed_float(qtk_gain_estimate_t *g_est, float *data, int len, int is_end);
int qtk_gain_estimate_feed(qtk_gain_estimate_t *g_est, short *data, int len, int is_end);
int qtk_gain_estimate(qtk_gain_estimate_t *g_est, float A_max, float A_offset);
float qtk_gain_estimate_rir(qtk_gain_estimate_t *g_est, float *rir, int len,
                            int sr, int direct_sound_idx, float A_max);
float qtk_gain_estimate_rir_v2(qtk_gain_estimate_t *g_est, float *rir, int len,
                               int direct_sound_idx, int L);
#ifdef __cplusplus
};
#endif
#endif
