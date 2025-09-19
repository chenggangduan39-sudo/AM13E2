#ifndef WTK_BFIO_QFORM_WTK_BF
#define WTK_BFIO_QFORM_WTK_BF
#include "wtk_bf_cfg.h"
#include "wtk/core/wtk_type.h" 
#include "wtk/core/wtk_complex.h"
#include "wtk/core/fft/wtk_stft.h"
#include "wtk/core/fft/wtk_stft2.h"
#include "wtk/bfio/eig/wtk_eig.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef void(*wtk_bf_notify_f)(void *ths,float *data,int len,int is_end);

typedef struct wtk_bf wtk_bf_t;
struct wtk_bf
{
	wtk_bf_cfg_t *cfg;

	wtk_complex_t **sdcov;
	wtk_complex_t **ovec;
	wtk_complex_t **w;
	wtk_complex_t *fft;

	wtk_complex_t **scov;
	wtk_complex_t **ncov;

	wtk_complex_t *tmp_cov;
	wtk_complex_t *tmp;
	wtk_complex_t *eigtmp;
	wtk_dcomplex_t *tmp2;

	wtk_complex_t *vac;
	float *val;

	wtk_eig_t *eig;

	float *acorr;
	float *corr;

	int channel;
	int channelx2;
	int nbin;
	int win;

	long nframe;
	int lst_pos;
	float *pad;

	void *ths;
	wtk_bf_notify_f notify;
};

wtk_bf_t* wtk_bf_new(wtk_bf_cfg_t *cfg,int win);

void wtk_bf_delete(wtk_bf_t *bf);

void wtk_bf_reset(wtk_bf_t *bf);

void wtk_bf_set_notify(wtk_bf_t *bf,void *ths,wtk_bf_notify_f notify);


void wtk_bf_update_ovec(wtk_bf_t *bf,float theta,float phi);

void wtk_bf_update_ovec3(wtk_bf_t *bf,float theta,float phi,wtk_complex_t **ovec);

void wtk_bf_update_ovec2(wtk_bf_t *bf,float theta,float phi);

void wtk_bf_update_ovec_orth(wtk_bf_t *bf,wtk_complex_t **ovec2,int c);

void wtk_bf_update_ovec_orth2(wtk_complex_t **ovec, wtk_complex_t **ovec_orth, int channel, int nbin, int c);
void wtk_bf_update_ovec4(float theta,float phi,int channel,int nbin,int rate,float speed,float **mic_pos,wtk_complex_t **ovec);
void wtk_bf_orth_ovec(wtk_complex_t ***ovec,int m,int nbin,int channel, int need_norm);

void wtk_bf_update_ovec5(float theta,float phi,int channel,int nbin,int rate,float speed,float **mic_pos,wtk_complex_t **ovec, float sdb_alpha, float Q_eye);

void wtk_bf_update_scov(wtk_bf_t *bf,wtk_complex_t **scov,int k);

void wtk_bf_update_ncov(wtk_bf_t *bf,wtk_complex_t **ncov,int k);

void wtk_bf_update_w(wtk_bf_t *bf,int k);

void wtk_bf_update_w2(wtk_bf_t *bf,int k,int nch);

void wtk_bf_init_w(wtk_bf_t *bf);

void wtk_bf_update_mvdr_w(wtk_bf_t *bf,int sd,int k);
void wtk_bf_update_mvdr_w_f(wtk_bf_t *bf,int sd,int k);

void wtk_bf_output_msg(wtk_bf_t *m,wtk_stft_msg_t *msg,wtk_stft_t *stft,int is_end);

void wtk_bf_output_msg2(wtk_bf_t *m,wtk_stft_msg_t *msg,wtk_stft_t *stft,int is_end,float *cohv);

void wtk_bf_output_msg3(wtk_bf_t *m,wtk_stft_msg_t *msg,wtk_stft_t *stft,int is_end,float *cohv);

wtk_complex_t *wtk_bf_output_fft_msg(wtk_bf_t *m,wtk_stft_msg_t *msg);

void wtk_bf_output_fft_k(wtk_bf_t *m,wtk_complex_t *fft, wtk_complex_t *out, int k);

wtk_complex_t *wtk_bf_output_fft_msg2(wtk_bf_t *m,wtk_stft_msg_t *msg,float *cohv);

wtk_complex_t *wtk_bf_output_fft_msg2_2(wtk_bf_t *m,wtk_stft_msg_t *msg,float cohv);

wtk_complex_t *wtk_bf_output_fft_msg3(wtk_bf_t *m,wtk_stft_msg_t *msg,float *cohv);


wtk_complex_t *wtk_bf_output_fft2_msg(wtk_bf_t *m,wtk_stft2_msg_t *msg);

wtk_complex_t *wtk_bf_output_fft2_msg2(wtk_bf_t *m,wtk_stft2_msg_t *msg,float *cohv);

// fft |nbin*channel|
wtk_complex_t *wtk_bf_output_fft2(wtk_bf_t *m,wtk_complex_t **fft,float *cohv);
#ifdef __cplusplus
};
#endif
#endif
