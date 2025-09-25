#ifndef WTK_BFIO_DEREVERB_WTK_DEREVERB2
#define WTK_BFIO_DEREVERB_WTK_DEREVERB2
#include "wtk_dereverb2_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef void(*wtk_dereverb2_notify_f)(void *ths,short *data,int len,int is_end);

typedef struct wtk_dereverb2 wtk_dereverb2_t;
struct wtk_dereverb2
{
    wtk_dereverb2_cfg_t *cfg;
    wtk_stft2_t *stft;
    int nbin;
    int channel;

    float nframe;

    int end_pos;

    wtk_strbuf_t **input;
	float **notch_mem;
	float *memD;
	float memX;

    int nl;
    wtk_complex_t **norig;
    wtk_complex_t ***G;
    wtk_complex_t ***Q;

    wtk_complex_t *K;
    wtk_complex_t *tmp;

    wtk_complex_t **E;
    wtk_complex_t *out;

    wtk_complex_t *Y;
    float *Yf;

    wtk_complex_t **wx;

    wtk_bf_t *bf;

    float *Se;
    float *Sd;
    wtk_complex_t *Sed;
    wtk_qmmse_t *qmmse;

    wtk_dereverb2_notify_f notify;
    void *ths;
};

wtk_dereverb2_t* wtk_dereverb2_new(wtk_dereverb2_cfg_t *cfg);
void wtk_dereverb2_reset(wtk_dereverb2_t *dereverb);
void wtk_dereverb2_delete(wtk_dereverb2_t *dereverb);
void wtk_dereverb2_set_notify(wtk_dereverb2_t *dereverb,void *ths,wtk_dereverb2_notify_f notify);
void wtk_dereverb2_feed(wtk_dereverb2_t *dereverb,short **mic,int len,int is_end);


#ifdef __cplusplus
};
#endif
#endif