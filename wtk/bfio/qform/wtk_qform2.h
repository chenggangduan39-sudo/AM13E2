#ifndef WTK_BFIO_qform2_WTK_qform2
#define WTK_BFIO_qform2_WTK_qform2
#include "wtk_qform2_cfg.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/core/wtk_queue.h"
#include "wtk/core/wtk_robin.h"
#include "wtk/bfio/preemph/wtk_preemph.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef void(*wtk_qform2_notify_f)(void *ths, short *data, int len, int is_end);
typedef struct wtk_qform2 wtk_qform2_t;

struct wtk_qform2
{
	wtk_qform2_cfg_t *cfg;

	wtk_strbuf_t **input;

    int end_pos;
    wtk_stft2_t *stft2;
    wtk_admm2_t *admm;

    wtk_complex_t ***ovec;

    wtk_complex_t **xld;
    wtk_complex_t *xld_2;
    wtk_rls3_t *rls3;
    wtk_nlms_t *nlms;

    float theta;
    float phi;

    int nbin;
    int channel;

    float nframe;

	float notch_mem[10][2];
	float memD[10];
	float memX;

    wtk_complex_t **fft;

    float *Yf;
    wtk_complex_t *out;

    float *Se;
    float *Sd;
    wtk_complex_t *Sed;

    float *fout;
    float *pad;

	wtk_qmmse_t *qmmse;

	void *ths;
	wtk_qform2_notify_f notify;
};

wtk_qform2_t* wtk_qform2_new(wtk_qform2_cfg_t *cfg);

void wtk_qform2_delete(wtk_qform2_t *qform2);

void wtk_qform2_reset(wtk_qform2_t *qform2);

void wtk_qform2_set_notify(wtk_qform2_t *qform2,void *ths,wtk_qform2_notify_f notify);

void wtk_qform2_start(wtk_qform2_t *qform2,float theta,float phi);

void wtk_qform2_feed(wtk_qform2_t *qform2,short **data,int len,int is_end);


wtk_qform2_t* wtk_qform2_new2(wtk_qform2_cfg_t *cfg, wtk_stft2_t *stft2);

void wtk_qform2_delete2(wtk_qform2_t *qform2);

void wtk_qform2_reset2(wtk_qform2_t *qform2);

void wtk_qform2_start2(wtk_qform2_t *qform2,float theta,float phi);

void wtk_qform2_feed_smsg(wtk_qform2_t *qform2,wtk_stft2_msg_t *msg,int pos,int is_end);

#ifdef __cplusplus
};
#endif
#endif
