#ifndef WTK_TTS_SYN_WTK_SYN_FFT
#define WTK_TTS_SYN_WTK_SYN_FFT
#include "wtk/core/wtk_type.h" 
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_syn_fft wtk_syn_fft_t;
struct wtk_syn_fft
{
	int fftlen;
	int p;
	int n;
	float divn;
	unsigned short *kj;
};

wtk_syn_fft_t* wtk_syn_fft_new(int fftlen);
void wtk_syn_fft_delete(wtk_syn_fft_t *f);

int wtk_syn_fft_process(wtk_syn_fft_t *f,float *xRe, float *xIm, int inv);
int wtk_syn_fft_process2(wtk_syn_fft_t *f,float *xRe, float *xIm, int inv);
int wtk_syn_fft_bytes(wtk_syn_fft_t* f);
#ifdef __cplusplus
};
#endif
#endif
