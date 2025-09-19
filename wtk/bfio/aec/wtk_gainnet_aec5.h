#ifndef WTK_BFIO_AEC_WTK_gainnet_aec5
#define WTK_BFIO_AEC_WTK_gainnet_aec5
#include "wtk/core/wtk_strbuf.h"
#include "wtk_gainnet_aec5_cfg.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/bfio/preemph/wtk_preemph.h"
#include "wtk/bfio/maskdenoise/pitch.h"
#include "wtk/bfio/ahs/qtk_kalmanv3.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*wtk_gainnet_aec5_notify_trfeat_f)(void *ths,float *feat,int len,float *feat2,int len2,float *target_agc,float *target_g,float *target_ge,int g_len);
typedef void (*wtk_gainnet_aec5_notify_f)(void *ths,short **data,int len,int is_end);

typedef struct wtk_gainnet_aec5 wtk_gainnet_aec5_t;

typedef struct
{
	wtk_gainnet_aec5_cfg_t *cfg;
    int nbin;

    wtk_bankfeat_t *bank_mic;
    wtk_bankfeat_t *bank_sp;

    float *g;
    float *lastg;
    float *gf;
    float *g2;
    float *gf2;
    float gff2;

    wtk_gainnet5_t *gainnet;
    wtk_gainnet2_t *gainnet2;
    wtk_gainnet6_t *gainnet6;
    wtk_qmmse_t *qmmse;
    
    unsigned agc_on:1;
}wtk_gainnet_single_aec5_t;

typedef struct
{
	wtk_gainnet_aec5_cfg_t *cfg;

    int nbin;
    wtk_complex_t *W;
    wtk_complex_t **X;
	wtk_complex_t *E;
    wtk_complex_t *Y;
	float *power_x;
    
}wtk_gainnet_aec5_nlmsdelay_t;

struct wtk_gainnet_aec5
{
	wtk_gainnet_aec5_cfg_t *cfg;

    int channel;

	wtk_strbuf_t **mic;
	wtk_strbuf_t **sp;
	float **notch_mem;
	float *memD;
	float *memX;

    float *analysis_window;
    float *synthesis_window;
	wtk_drft_t *rfft;
	float *rfft_in;
	int nbin;
    float **analysis_mem;
    float **analysis_mem_sp;
	float **synthesis_mem;

    wtk_gainnet_aec5_nlmsdelay_t *gnlmsd;

	wtk_complex_t **fft;
	wtk_complex_t **fft_sp;

	wtk_rls_t *erls;
    wtk_nlms_t *enlms;
    qtk_ahs_kalman_t *ekalman;
    wtk_complex_t **ffty;

    wtk_gainnet_single_aec5_t *gsaec;

	float **out;

	void *ths;
	wtk_gainnet_aec5_notify_f notify;

    void *ths_tr;
    wtk_gainnet_aec5_notify_trfeat_f notify_tr;

    int train_echo;

    int sp_silcnt;
    unsigned sp_sil:1;
};

wtk_gainnet_aec5_t* wtk_gainnet_aec5_new(wtk_gainnet_aec5_cfg_t *cfg);
void wtk_gainnet_aec5_delete(wtk_gainnet_aec5_t *gainnet_aec5);
void wtk_gainnet_aec5_reset(wtk_gainnet_aec5_t *gainnet_aec5);
void wtk_gainnet_aec5_set_notify(wtk_gainnet_aec5_t *gainnet_aec5,void *ths,wtk_gainnet_aec5_notify_f notify);
void wtk_gainnet_aec5_feed(wtk_gainnet_aec5_t *gainnet_aec5,short **data,int len,int is_end);

void wtk_gainnet_aec5_set_notify_tr(wtk_gainnet_aec5_t *gaec,void *ths,wtk_gainnet_aec5_notify_trfeat_f notify);
void wtk_gainnet_aec5_feed_train(wtk_gainnet_aec5_t *gaec,short **data,int len, int channel, int bb, int stabe);
#ifdef __cplusplus
};
#endif
#endif
