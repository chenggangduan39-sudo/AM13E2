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
    float x, y, z;
} Vector3D;  // 三维向量结构体

typedef struct {
    Vector3D* positions;  // 位置数组
    Vector3D* normals;    // 法向量数组
    float* weights;      // 权重数组
    int count;            // 数组元素数量
} LinearArray;  // 线性阵列结构体

struct qtk_soundfield_syntheis
{
	qtk_soundfield_syntheis_cfg_t *cfg;
    wtk_complex_t *driving_func;
    float *prev_half_win;
    wtk_strbuf_t *input;
    float *output_buf;
    short **output;

    wtk_drft_t *drft;
    float *fft_in;
    wtk_complex_t *fft_buf;
    wtk_complex_t *ifft_in;
    float *ifft_buf;

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