#ifndef WTK_BFIO_VBOX_WTK_VBOXBF4
#define WTK_BFIO_VBOX_WTK_VBOXBF4
#include "wtk/core/wtk_strbuf.h"
#include "wtk_vboxebf4_cfg.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_strbuf.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_vboxebf4 wtk_vboxebf4_t;
typedef void(*wtk_vboxebf4_notify_f)(void *ths,short *output,int len);
typedef void (*wtk_vboxebf4_notify_ssl_f)(void *ths, float ts, float te,wtk_ssl2_extp_t *nbest_extp,int nbest);
typedef void(*wtk_vboxebf4_notify_eng_f)(void *ths,float energy,float snr);

typedef struct
{
	wtk_vboxebf4_cfg_t *cfg;
    int nbin;

    wtk_bankfeat_t *bank_mic;
    wtk_bankfeat_t *bank_sp;

    float *g;
    float *lastg;
    float *gf;
    float *g2;
    float *gf2;

    wtk_gainnet5_t *gainnet;
    wtk_gainnet6_t *gainnet6;
    wtk_qmmse_t *qmmse;
}wtk_vboxebf4_aec_t;


struct wtk_vboxebf4
{
	wtk_vboxebf4_cfg_t *cfg;

    wtk_strbuf_t **mic;
    wtk_strbuf_t **sp;

    float *analysis_window;
    float *synthesis_window;
	wtk_drft_t *rfft;
	float *rfft_in;
	int nbin;
    float **analysis_mem;
    float **analysis_mem_sp;
	float *synthesis_mem;

	wtk_complex_t **fft;
    wtk_complex_t **fft2;
	wtk_complex_t **fft_sp;

	wtk_rls_t *erls;
    wtk_complex_t **ffty;

    wtk_vboxebf4_aec_t *vaec;

    float pframe;

    wtk_complex_t **ffts;
    wtk_complex_t **ffty2;

	wtk_covm_t *covm;
    wtk_covm_t *echo_covm;
	wtk_bf_t *bf;

    wtk_complex_t *fftx;

    wtk_ssl2_t *ssl2;
    wtk_maskssl_t *maskssl;
    wtk_maskssl2_t *maskssl2;
    
	float *out;

    wtk_equalizer_t *eq;

	void *ths;
	wtk_vboxebf4_notify_f notify;

    void *ssl_ths;
	wtk_vboxebf4_notify_ssl_f notify_ssl;

    void *eng_ths;
    wtk_vboxebf4_notify_eng_f notify_eng;

    float inmic_scale;
    
    float bs_scale;
    float bs_last_scale;
    int bs_max_cnt;

    int sp_silcnt;
    int mic_silcnt;
    unsigned sp_sil:1;
    unsigned mic_sil:1;
    unsigned agc_enable:1;
    unsigned echo_enable:1;   
    unsigned denoise_enable:1;   
    unsigned ssl_enable:1;
};

wtk_vboxebf4_t* wtk_vboxebf4_new(wtk_vboxebf4_cfg_t *cfg);
void wtk_vboxebf4_delete(wtk_vboxebf4_t *vboxebf4);
void wtk_vboxebf4_start(wtk_vboxebf4_t *vboxebf4);
void wtk_vboxebf4_reset(wtk_vboxebf4_t *vboxebf4);
void wtk_vboxebf4_set_notify(wtk_vboxebf4_t *vboxebf4,void *ths,wtk_vboxebf4_notify_f notify);
void wtk_vboxebf4_set_ssl_notify(wtk_vboxebf4_t *vboxebf4,void *ths,wtk_vboxebf4_notify_ssl_f notify);
void wtk_vboxebf4_set_eng_notify(wtk_vboxebf4_t *vboxebf4,void *ths,wtk_vboxebf4_notify_eng_f notify);
/**
 * len=mic array samples
 */
void wtk_vboxebf4_feed(wtk_vboxebf4_t *vboxebf4,short *data,int len,int is_end);

void wtk_vboxebf4_set_micvolume(wtk_vboxebf4_t *vboxebf4,float fscale);
void wtk_vboxebf4_set_agcenable(wtk_vboxebf4_t *vboxebf4,int enable);
void wtk_vboxebf4_set_echoenable(wtk_vboxebf4_t *vboxebf4,int enable);
void wtk_vboxebf4_set_denoiseenable(wtk_vboxebf4_t *vboxebf4,int enable);
void wtk_vboxebf4_ssl_delay_new(wtk_vboxebf4_t *vboxebf4);
#ifdef __cplusplus
};
#endif
#endif
