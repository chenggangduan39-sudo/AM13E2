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
};

qtk_soundfield_syntheis_t *qtk_soundfield_syntheis_new(qtk_soundfield_syntheis_cfg_t *cfg);
void qtk_soundfield_syntheis_delete(qtk_soundfield_syntheis_t *sspot);
void qtk_soundfield_syntheis_reset(qtk_soundfield_syntheis_t *sspot);
void qtk_soundfield_syntheis_feed(qtk_soundfield_syntheis_t *sos, short *data, int len);
void qtk_soundfield_syntheis_set_notify(qtk_soundfield_syntheis_t *sos, void *upval, qtk_soundfield_syntheis_notify_f notify);
#ifdef __cplusplus
};
#endif
#endif