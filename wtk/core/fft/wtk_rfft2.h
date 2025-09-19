#ifndef WTK_CORE_FFT_WTK_RFFT2
#define WTK_CORE_FFT_WTK_RFFT2
#include "wtk/core/wtk_type.h" 
#include "wtk_rfft2_cfg.h"
#include "wtk_rfft.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_rfft2 wtk_rfft2_t;

typedef struct
{
	wtk_queue_node_t q_n;
	float *f;
	int idx;
}wtk_fft_item_t;

typedef void(*wtk_rfft2_notify_f)(void *ths,float *fft);

struct wtk_rfft2
{
	wtk_rfft2_cfg_t *cfg;
	wtk_rfft_t *rfft;
	float *buf;
	float *input;
	float *fft;
	int pos;
	void *notify_ths;
	wtk_rfft2_notify_f notify;
};

wtk_rfft2_t* wtk_rfft2_new(wtk_rfft2_cfg_t *cfg);
void wtk_rfft2_delete(wtk_rfft2_t *fft);
void wtk_rfft2_reset(wtk_rfft2_t *fft);
void wtk_rfft2_set_notify(wtk_rfft2_t *fft,void *ths,wtk_rfft2_notify_f notify);
void wtk_rfft2_feed(wtk_rfft2_t *fft,float *data,int len,int is_end);
void wtk_rfft2_feed2(wtk_rfft2_t *senn,short *data,int len,int is_end);


wtk_fft_item_t* wtk_fft_item_new(int n);
void wtk_fft_item_delete(wtk_fft_item_t *item);
#ifdef __cplusplus
};
#endif
#endif
