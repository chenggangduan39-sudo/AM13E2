#ifndef WTK_BFIO_MASKFORM_WTK_GAINNET_BF5
#define WTK_BFIO_MASKFORM_WTK_GAINNET_BF5
#include "wtk/core/wtk_strbuf.h"
#include "wtk_gainnet_bf5_cfg.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/bfio/preemph/wtk_preemph.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_gainnet_bf5 wtk_gainnet_bf5_t;
typedef void(*wtk_gainnet_bf5_notify_f)(void *ths,short *output,int len);
typedef void (*wtk_gainnet_bf5_notify_trfeat_f)(void *ths,float *feat,int len,float *target_g,int g_len);


typedef struct
{
	wtk_gainnet_bf5_cfg_t *cfg;
    int nbin;

    int *eband;

    float  *Ex;

    float *dct_table;
    float **cepstral_mem;
    int memid;
    float **cepstral_mem_sp;
    int memid_sp;

    float *g;
    float *lastg;
    float *gf;
    float *g2;
    float *lastg2;
    float *gf2;

    float *Ly;
    float *features;
    wtk_gainnet_t *gainnet;

    wtk_qmmse_t *qmmse;

    unsigned silence:1;
}wtk_gainnet_bf5_tfmask_t;

struct wtk_gainnet_bf5
{
	wtk_gainnet_bf5_cfg_t *cfg;

	wtk_strbuf_t **mic;
	float **notch_mem;
	float *memD;
	float memX;

    float *window;
    float *synthesis_window;
	wtk_drft_t *rfft;
	float *rfft_in;
	int nbin;
    float **analysis_mem;
	float *synthesis_mem;
	wtk_complex_t **fft;
    wtk_complex_t ***ovec;

	wtk_complex_t *ffts;
    wtk_complex_t *ffty;

    wtk_gainnet_bf5_tfmask_t *gtfmask;

	float *out;

	void *ths;
	wtk_gainnet_bf5_notify_f notify;

    void *ths_tr;
    wtk_gainnet_bf5_notify_trfeat_f notify_tr;

    unsigned training:1;
};

wtk_gainnet_bf5_t* wtk_gainnet_bf5_new(wtk_gainnet_bf5_cfg_t *cfg);
void wtk_gainnet_bf5_delete(wtk_gainnet_bf5_t *gainnet_bf5);
void wtk_gainnet_bf5_start(wtk_gainnet_bf5_t *gainnet_bf5, float theta, float phi);
void wtk_gainnet_bf5_reset(wtk_gainnet_bf5_t *gainnet_bf5);
void wtk_gainnet_bf5_set_notify(wtk_gainnet_bf5_t *gainnet_bf5,void *ths,wtk_gainnet_bf5_notify_f notify);
/**
 * len=mic array samples
 */
void wtk_gainnet_bf5_feed(wtk_gainnet_bf5_t *gainnet_bf5,short **data,int len,int is_end);

void wtk_gainnet_bf5_set_tr_notify(wtk_gainnet_bf5_t *gainnet_bf5,void *ths,wtk_gainnet_bf5_notify_trfeat_f notify);
void wtk_gainnet_bf5_feed_train(wtk_gainnet_bf5_t *gainnet_bf5,short **data,int len, int bb);
void wtk_gainnet_bf5_feed_train2(wtk_gainnet_bf5_t *gainnet_bf5,short **data,short **data2,short **datar,int len, int enr);

#ifdef __cplusplus
};
#endif
#endif
