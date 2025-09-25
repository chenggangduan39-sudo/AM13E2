#ifndef WTK_CORE_FFT_WTK_CFFT
#define WTK_CORE_FFT_WTK_CFFT
#include "wtk/core/fft/wtk_rfft.h" 
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_cfft wtk_cfft_t;
struct wtk_cfft
{
	wtk_rfft_t *fft;
	float *cr;
	float *ci;
	float *fr;
	float *fi;
	wtk_complex_t *fftd;
};

wtk_cfft_t*  wtk_cfft_new(int len);
void wtk_cfft_delete(wtk_cfft_t *fft);
wtk_complex_t* wtk_cfft_fft(wtk_cfft_t *m,wtk_complex_t *x);
wtk_complex_t* wtk_cfft_fft2(wtk_cfft_t *m,wtk_complex_t *frame,int n2);
wtk_complex_t* wtk_cfft_ifft(wtk_cfft_t *m,wtk_complex_t *f);
wtk_complex_t* wtk_cfft_ifft2(wtk_cfft_t *m,wtk_complex_t *frame,int n2);
wtk_complex_t* wtk_cfft_ifft3(wtk_cfft_t *m,wtk_complex_t *frame,int n2,float fx);

void wtk_xcorr_freq_mult2(float *fft1,float *fft2,float *xy,int fft_size);
float wtk_rfft_find_best_value2(float *x1,int len,int margin,float *max_v);
float wtk_float_sum2(float *a,int n);
float wtk_float_sum22(float *a,int n);
float wtk_short_sum2(short *a,int n);
int wtk_short_cuts(short *a,int n);
int wtk_float_find_max_idx(float *pv,int n,int step,float *pf);
int wtk_rfft_xcorr_float2(float *mic,int mic_len,float *sp,int sp_len,float *pt);
#ifdef __cplusplus
};
#endif
#endif
