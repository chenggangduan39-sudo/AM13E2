#ifndef WTK_CORE_FFT_WTK_FIXFFT2
#define WTK_CORE_FFT_WTK_FIXFFT2
#include "wtk/core/wtk_type.h" 
#include "wtk/core/wtk_fixpoint.h"
#include "wtk/core/wtk_str.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_fixfft2 wtk_fixfft2_t;

typedef int wtk_fix2_t;

struct wtk_fixfft2
{
	int N;
	int order;
	wtk_fix2_t *ccc;
	wtk_fix2_t *sss;

	wtk_fix2_t *iccc;
	wtk_fix2_t *isss;

	wtk_fix2_t *ti;
};

wtk_fixfft2_t* wtk_fixfft2_new(int N);
void wtk_fixfft2_delete(wtk_fixfft2_t *fft);

void wtk_fixfft2_fft(wtk_fixfft2_t *fft,wtk_fix2_t *x);
void wtk_fixfft2_fft2(wtk_fixfft2_t *fft,wtk_fix2_t *x,wtk_fix2_t *f);
void wtk_fixfft2_fft3(wtk_fixfft2_t *fft,short *x,wtk_fix2_t *f,int shift);

void wtk_fixfft2_ifft(wtk_fixfft2_t *fft,wtk_fix2_t *f,wtk_fix2_t *x);

void wtk_fixfft2_print(wtk_fix2_t *fft,int N,int shift);

void wtk_fixfft2_test();

void wtk_fixfft2_to_rfft(wtk_fix2_t *ifft,float *fft,int shift,int N);

wtk_fix2_t wtk_fix2_mulx(wtk_fix2_t a,wtk_fix2_t b,int a1,int a2,int a3);

void wtk_fixfft2_test_FFT(float *x,float *y,int n,int sign);
#ifdef __cplusplus
};
#endif
#endif
