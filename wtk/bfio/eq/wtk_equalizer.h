#ifndef WTK_EQUALIZER_H
#define WTK_EQUALIZER_H

#include "wtk_equalizer_cfg.h"

#ifdef __cplusplus
extern "C" { 
#endif

typedef struct wtk_iir_coeffs{
    float beta;
    float alpha;
    float gamma;
    float dummy;
}wtk_iir_coeffs_t;

typedef struct wtk_equalizer_band{
    float gain;
    float value;
    int rate;
    wtk_iir_coeffs_t coeffs;
    float x[3];
    float y[3];
    float x2[3];
    float y2[3];
}wtk_equalizer_band_t;


typedef struct wtk_equalizer{
    wtk_equalizer_cfg_t *cfg;
    wtk_equalizer_band_t *band;
    float dither[256];
    float   preamp;
    float sfreq;    //表示频率
    int di;
    int ni;
    int nj;
    int nk;
}wtk_equalizer_t;


wtk_equalizer_t* wtk_equalizer_new(wtk_equalizer_cfg_t *cfg);
void wtk_equalizer_reset(wtk_equalizer_t *eq);
int wtk_equalizer_feed(wtk_equalizer_t *eq,short *data,int len);
int wtk_equalizer_feed_float(wtk_equalizer_t *eq,float *data,int len);
void wtk_equalizer_delete(wtk_equalizer_t *equalizer);

#ifdef __cplusplus
};
#endif

#endif