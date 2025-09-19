#ifndef WTK_CORE_FFT_WTK_fixstft
#define WTK_CORE_FFT_WTK_fixstft
#include "wtk/core/wtk_type.h" 
#include "wtk/core/wtk_complex.h"
#include "wtk/core/fft/wtk_fixfft.h"
#include "wtk/core/wtk_hoard.h"
#include "wtk/core/wtk_riff.h"
#include "wtk_stft_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_fixstft wtk_fixstft_t;

#define DEBUG_IDX

typedef struct
{
	wtk_queue_node_t hoard_n;
	wtk_queue_node_t q_n;  //use for third use.
	short *real;
	short *imag;
}wtk_fixstft_msg_t;

typedef void(*wtk_fixstft_notify_f)(void *ths,wtk_fixstft_msg_t *msg,int pos,int is_end);

struct wtk_fixstft
{
	wtk_stft_cfg_t *cfg;
	wtk_hoard_t msg_hoard;
	wtk_fixfft_t *fixfft;
	short *win;
	short *input_fft;
	short *input_win;
	short *input;
	int nbin;
	int pos;
	short fft_scale;

	wtk_fixstft_notify_f notify;
	void *notify_ths;
	long xinput;
	int nframe;
};


wtk_fixstft_t* wtk_fixstft_new(wtk_stft_cfg_t *cfg);
void wtk_fixstft_delete(wtk_fixstft_t *fixstft);
void wtk_fixstft_reset(wtk_fixstft_t *fixstft);

void wtk_fixstft_set_notify(wtk_fixstft_t *fixstft,void *ths,wtk_fixstft_notify_f notify);

void wtk_fixstft_feed(wtk_fixstft_t *fixstft,short *pv,int n,int is_end);
void wtk_fixstft_push_msg(wtk_fixstft_t *fixstft,wtk_fixstft_msg_t *msg);


#ifdef __cplusplus
};
#endif
#endif
