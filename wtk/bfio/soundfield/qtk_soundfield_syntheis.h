#ifndef WTK_BFIO_QTK_SOUNDFIELD_SYNTHEIS
#define WTK_BFIO_QTK_SOUNDFIELD_SYNTHEIS
#include "qtk_soundfield_syntheis_cfg.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_soundfield_syntheis qtk_soundfield_syntheis_t;
typedef void (*qtk_soundfield_syntheis_notify_f)(void *upval, short **out, int len);

typedef struct {
    int L1;
    int L2;
    int L;
    int n;
    int fft_n;
    float** cache;
    float** cache_x;
    wtk_complex_t *fft_buf;
    float** weight;
    wtk_complex_t** WEIGHT;
    wtk_drft_t *drft;
    float **output_buf;
    float **output;
}sfs_linear_conv_t;

struct qtk_soundfield_syntheis
{
	qtk_soundfield_syntheis_cfg_t *cfg;
    wtk_strbuf_t *input;
    float *output_buf;
    short **output;
    sfs_linear_conv_t *spliter;
    sfs_linear_conv_t *weight;

    float **weights;

    void *upval;
    qtk_soundfield_syntheis_notify_f notify;

    wtk_complex_t **H_b;
    wtk_complex_t **Z_b;
    wtk_complex_t *fft_buf;
    wtk_drft_t *drft;
    wtk_strbuf_t **rir_bufs;
    float *taps_lowpass;
    float *taps_highpass;
    int filter_length;
    int eval_idx;
    int ref_idx;
    int nbin;
};

qtk_soundfield_syntheis_t *qtk_soundfield_syntheis_new(qtk_soundfield_syntheis_cfg_t *cfg);
void qtk_soundfield_syntheis_delete(qtk_soundfield_syntheis_t *sspot);
void qtk_soundfield_syntheis_reset(qtk_soundfield_syntheis_t *sspot);
void qtk_soundfield_syntheis_feed(qtk_soundfield_syntheis_t *sos, short *data, int len);
void qtk_soundfield_syntheis_set_notify(qtk_soundfield_syntheis_t *sos, void *upval, qtk_soundfield_syntheis_notify_f notify);
void qtk_soundfield_syntheis_feed_rir(qtk_soundfield_syntheis_t *sos, short **rirs, int len, int eval);
void qtk_soundfield_syntheis_calc_weight(qtk_soundfield_syntheis_t *sos);
void set_Hb(qtk_soundfield_syntheis_t *sos, float *H, float *H2);
void set_Zb(qtk_soundfield_syntheis_t *sos, float *H, float *H2);
#ifdef __cplusplus
};
#endif
#endif