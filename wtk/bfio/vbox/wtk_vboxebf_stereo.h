#ifndef WTK_BFIO_VBOX_WTK_VBOXBF_STEREO
#define WTK_BFIO_VBOX_WTK_VBOXBF_STEREO
#include "wtk_vboxebf_stereo_cfg.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_strbuf.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_vboxebf_stereo wtk_vboxebf_stereo_t;
typedef void(*wtk_vboxebf_stereo_notify_f)(void *ths,short **output,int len);

typedef struct
{
	wtk_vboxebf_stereo_cfg_t *cfg;
    int nbin;

    wtk_bankfeat_t *bank_mic;
    wtk_bankfeat_t *bank_sp;

    float *feature_sp;

    float *g;
    float *lastg;
    float *gf;

    float *g2;
    float *lastg2;  
    float *gf2;
    
    wtk_gainnet2_t *gainnet2;

    wtk_gainnet_t *agc_gainnet;

    wtk_qmmse_t *qmmse;
}wtk_vboxebf_stereo_edra_t;


struct wtk_vboxebf_stereo
{
	wtk_vboxebf_stereo_cfg_t *cfg;

	wtk_strbuf_t **mic;
	wtk_strbuf_t **sp;

    float *analysis_window;
    float *synthesis_window;
	wtk_drft_t *rfft;
	float *rfft_in;
	int nbin;
    float **analysis_mem;
    float **analysis_mem_sp;
	float **synthesis_mem;

	wtk_complex_t **fft;
	wtk_complex_t **fft_sp;

	wtk_rls_t *erls;

    wtk_vboxebf_stereo_edra_t *vdr;
    wtk_complex_t **fftx;
    wtk_complex_t **ffty;

    wtk_complex_t ***ovec;

	float **out;
    wtk_equalizer_t **eq;

	void *ths;
	wtk_vboxebf_stereo_notify_f notify;

    int sp_silcnt;
    int mic_silcnt;

    float *scale;
    float *last_scale;
	int *max_cnt;

    unsigned sp_sil:1;
    unsigned mic_sil:1;
    unsigned agc_on:1;
};

wtk_vboxebf_stereo_t* wtk_vboxebf_stereo_new(wtk_vboxebf_stereo_cfg_t *cfg);
void wtk_vboxebf_stereo_delete(wtk_vboxebf_stereo_t *vboxebf_stereo);
void wtk_vboxebf_stereo_start(wtk_vboxebf_stereo_t *vboxebf_stereo);
void wtk_vboxebf_stereo_reset(wtk_vboxebf_stereo_t *vboxebf_stereo);
void wtk_vboxebf_stereo_set_notify(wtk_vboxebf_stereo_t *vboxebf_stereo,void *ths,wtk_vboxebf_stereo_notify_f notify);
/**
 * len=mic array samples
 */
void wtk_vboxebf_stereo_feed(wtk_vboxebf_stereo_t *vboxebf_stereo,short *data,int len,int is_end);

#ifdef __cplusplus
};
#endif
#endif
