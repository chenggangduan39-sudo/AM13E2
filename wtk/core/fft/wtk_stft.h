#ifndef WTK_CORE_FFT_WTK_STFT
#define WTK_CORE_FFT_WTK_STFT
#include "wtk/core/wtk_type.h" 
#include "wtk/core/wtk_complex.h"
#include "wtk/core/fft/wtk_rfft.h"
#include "wtk/core/wtk_hoard.h"
#include "wtk/core/wtk_riff.h"
#include "wtk_stft_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_stft wtk_stft_t;

#define DEBUG_IDX

typedef struct
{
	wtk_queue_node_t hoard_n;
	wtk_queue_node_t q_n;  //use for third use.
	wtk_complex_t **fft; // |7*129|
#ifdef DEBUG_IDX
	long s;
#endif
	void *hook;
}wtk_stft_msg_t;

typedef void(*wtk_stft_notify_f)(void *ths,wtk_stft_msg_t *msg,int pos,int is_end);
typedef void(*wtk_stft_notify_float_f)(void *ths,float **y,int len);

struct wtk_stft
{
	wtk_stft_cfg_t *cfg;
	wtk_hoard_t msg_hoard;
	wtk_rfft_t *rfft;
	float *win;
	float *synthesis_win;
	float *input_fft;
	float *input_win;
	float **input;
	float **pad;
	float **output;
	int nbin;
	int pos;
	float fft_scale;

	wtk_complex_t *fft;

	wtk_stft_notify_f notify;
	void *notify_ths;
	long xinput;
	int nframe;

	long nsample;

	wtk_stft_notify_float_f notify_float;
	void* notify_float_ths;
	unsigned cancel:1;
};

int wtk_stft_bytes(wtk_stft_t *stft);
wtk_stft_t* wtk_stft_new(wtk_stft_cfg_t *cfg);
void wtk_stft_delete(wtk_stft_t *stft);
void wtk_stft_reset(wtk_stft_t *stft);
void wtk_stft_reset_output_pad(wtk_stft_t *stft);
void wtk_stft_set_notify(wtk_stft_t *stft,void *ths,wtk_stft_notify_f notify);

void wtk_stft_set_float_notify(wtk_stft_t *stft,void *ths,wtk_stft_notify_float_f notify);


void wtk_stft_feed(wtk_stft_t *stft,short **pv,int n,int is_end);
void wtk_stft_feed3(wtk_stft_t *stft,short **pv,int n,int is_end);
void wtk_stft_feed_int(wtk_stft_t *stft,int **pv,int n,int is_end);
void wtk_stft_feed_float2(wtk_stft_t *stft,float **pv,int n,int is_end);

/**
 * multi channel;
 */
void wtk_stft_feed2(wtk_stft_t *stft,short *pv,int n,int is_end);

void wtk_stft_feed_float(wtk_stft_t *stft,float *data,int len,int is_end);

/**
 * fft: [nframe,channel,fft];
 */
void wtk_stft_process_ifft(wtk_stft_t *stft,wtk_complex_t ***fft,int n,int last_pos,int is_end);
int wtk_stft_output_ifft2(wtk_stft_t *stft,wtk_complex_t *c,int channel,int pos,int is_end,float *y,float *pad);
int wtk_stft_output_ifft(wtk_stft_t *stft,wtk_complex_t *c,int channel,int pos,int is_end,float *y);

wtk_stft_msg_t* wtk_stft_pop_msg(wtk_stft_t *stft);
void wtk_stft_push_msg(wtk_stft_t *stft,wtk_stft_msg_t *msg);

wtk_stft_msg_t* wtk_stft_msg_new(wtk_stft_t *stft);
void wtk_stft_msg_delete(wtk_stft_t *stft,wtk_stft_msg_t *msg);
wtk_stft_msg_t* wtk_stft_msg_copy(wtk_stft_msg_t *msg,int channel,int nbin);

void wtk_stft_msg_update_rnn(wtk_stft_msg_t *msg,wtk_complex_t **rnn,int c,int nbin);
void wtk_stft_msg_update_rnn3(wtk_stft_msg_t *msg,wtk_complex_t **rnn,int c,int nbin,float alpha2);
#ifdef __cplusplus
};
#endif
#endif
