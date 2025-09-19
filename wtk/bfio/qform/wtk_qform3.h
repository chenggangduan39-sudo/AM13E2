#ifndef WTK_BFIO_qform3_WTK_qform3
#define WTK_BFIO_qform3_WTK_qform3
#include "wtk_qform3_cfg.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/core/wtk_queue.h"
#include "wtk/core/wtk_robin.h"
#include "wtk/bfio/preemph/wtk_preemph.h"
#include "wtk/core/wtk_sort.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef void(*wtk_qform3_notify_f)(void *ths, short *data, int len, int is_end);
typedef struct wtk_qform3 wtk_qform3_t;

struct wtk_qform3
{
	wtk_qform3_cfg_t *cfg;

	wtk_strbuf_t **input;

    int end_pos;
    wtk_stft2_t *stft2;
    wtk_admm2_t *admm;

    wtk_complex_t ***ovec;

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
    wtk_aspec_t *aspec;
    wtk_complex_t *cov;
    wtk_complex_t *inv_cov;
	wtk_dcomplex_t *tmp;
    float aspec_mask;
    int spec_cnt;
    FILE *cohv_fn;

	void *ths;
	wtk_qform3_notify_f notify;
};

wtk_qform3_t* wtk_qform3_new(wtk_qform3_cfg_t *cfg);

void wtk_qform3_delete(wtk_qform3_t *qform3);

void wtk_qform3_reset(wtk_qform3_t *qform3);

void wtk_qform3_set_notify(wtk_qform3_t *qform3,void *ths,wtk_qform3_notify_f notify);

void wtk_qform3_start(wtk_qform3_t *qform3,float theta,float phi);

void wtk_qform3_feed(wtk_qform3_t *qform3,short **data,int len,int is_end);


wtk_qform3_t* wtk_qform3_new2(wtk_qform3_cfg_t *cfg, wtk_stft2_t *stft2);

void wtk_qform3_delete2(wtk_qform3_t *qform3);

void wtk_qform3_reset2(wtk_qform3_t *qform3);

void wtk_qform3_start2(wtk_qform3_t *qform3,float theta,float phi);

void wtk_qform3_feed_smsg(wtk_qform3_t *qform3,wtk_stft2_msg_t *msg,int pos,int is_end);

#ifdef __cplusplus
};
#endif
#endif
