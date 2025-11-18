#ifndef WTK_CORE_FFT_WTK_STFT2
#define WTK_CORE_FFT_WTK_STFT2
#include "wtk/core/wtk_type.h" 
#include "wtk/core/wtk_complex.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/core/fft/wtk_rfft.h"
#include "wtk/core/wtk_hoard.h"
#include "wtk/core/wtk_riff.h"
#include "wtk_stft2_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_stft2 wtk_stft2_t;

typedef struct
{
	wtk_queue_node_t hoard_n;
	wtk_queue_node_t q_n;  //use for third use.
	wtk_complex_t **fft; // |nbin*channel|
	float s;
	void *hook;
}wtk_stft2_msg_t;

typedef void(*wtk_stft2_notify_f)(void *ths,wtk_stft2_msg_t *msg,int pos,int is_end);

struct wtk_stft2
{
	wtk_stft2_cfg_t *cfg;
	wtk_hoard_t msg_hoard;
	wtk_rfft_t *rfft;
	float *win;
	float *synthesis_win;
	float *input_fft;
	float *input_win;
	float **input;
	float *output;

	// float **pad;
	int nbin;
	int pos;
	float fft_scale;

	wtk_stft2_notify_f notify;
	void *notify_ths;
	long xinput;
	int nframe;
};

wtk_stft2_t* wtk_stft2_new(wtk_stft2_cfg_t *cfg);
void wtk_stft2_delete(wtk_stft2_t *stft2);
void wtk_stft2_reset(wtk_stft2_t *stft2);

void wtk_stft2_set_notify(wtk_stft2_t *stft2,void *ths,wtk_stft2_notify_f notify);

void wtk_stft2_feed(wtk_stft2_t *stft2,short **pv,int n,int is_end);
void wtk_stft2_feed2(wtk_stft2_t *stft2,short **pv,int n,int is_end);
void wtk_stft2_feed_int(wtk_stft2_t *stft2,int **pv,int n,int is_end);
void wtk_stft2_feed_float(wtk_stft2_t *stft2,float **pv,int n,int is_end);

//y: stft->output
int wtk_stft2_output_ifft(wtk_stft2_t *stft2,wtk_complex_t *c,float *y,float *pad,int pos,int is_end);

wtk_stft2_msg_t* wtk_stft2_pop_msg(wtk_stft2_t *stft2);
void wtk_stft2_push_msg(wtk_stft2_t *stft2,wtk_stft2_msg_t *msg);

wtk_stft2_msg_t* wtk_stft2_msg_new(wtk_stft2_t *stft2);
void wtk_stft2_msg_delete(wtk_stft2_t *stft2,wtk_stft2_msg_t *msg);
wtk_stft2_msg_t* wtk_stft2_msg_copy(wtk_stft2_msg_t *msg,int channel,int nbin);

#ifdef __cplusplus
};
#endif
#endif
