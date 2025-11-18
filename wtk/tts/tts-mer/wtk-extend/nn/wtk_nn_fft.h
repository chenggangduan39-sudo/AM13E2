#ifndef WTK_NN_FFT_H_
#define WTK_NN_FFT_H_
#include "tts-mer/wtk-extend/wtk_tts_common.h"
// #include "wtk/tts-mer/kissfft/kiss_fft.h"
#ifdef __cplusplus
extern "C" {
#endif

// typedef kiss_fft_cfg wtk_nn_fft_t;

typedef struct
{
    float *real;
    float *imag;
    int len;
} wtk_nn_complex_t;

typedef struct
{
    double *real;
    double *imag;
    int len;
} wtk_nn_dcomplex_t;

typedef struct
{
    wtk_heap_t *heap;
    wtk_rfft_t *rf;
    int nfft;
    int nfreq;
    int win_len;
    int hop_len;
    float *win_hann;
    float *win_hann_sum;
    float *fft_cache;
    float *fft_x;
    float *fft_y;
} wtk_nn_stft_t;

wtk_nn_complex_t* wtk_nn_complex_new(int len);
void wtk_nn_complex_zero(wtk_nn_complex_t *p);
void wtk_nn_complex_delete(wtk_nn_complex_t *p);
void wtk_nn_complex_mul(wtk_nn_complex_t *a, wtk_nn_complex_t *b, wtk_nn_complex_t *c);
void wtk_nn_complex_abs(wtk_nn_complex_t *a, float *out);
void wtk_nn_complex_save(wtk_nn_complex_t *p, char fn[]);
float* wtk_nn_hanning(int size, short itype);
float* wtk_nn_hanning_heap( wtk_heap_t *heap, int size, short itype);
void wtk_nn_hanning_numpy( float *h, int m);

// wtk_nn_fft_t wtk_nn_fft_new(int nfft,int is_inverse_fft,void * mem,size_t * lenmem );
// void wtk_nn_fft(wtk_nn_fft_t cfg, wtk_complex_t *fin, wtk_complex_t *fout);
// void wtk_nn_fft_delete(wtk_nn_fft_t cfg);

void wtk_nn_rfft(wtk_rfft_t *rf, float *cache, float *x, float *y);
void wtk_nn_rfft_ifft(wtk_rfft_t *rf, float *cache, float *x, float *y);


wtk_nn_stft_t* wtk_nn_stft_heap_new( wtk_heap_t *heap, wtk_rfft_t *rf, int nfft, int hop_len, int win_len);
float* wtk_nn_istft_win_sumsquare_heap_new( wtk_heap_t *heap, wtk_nn_stft_t *stft, int n_frame);
void wtk_nn_stft( wtk_nn_stft_t *stft, float *y, int y_len, wtk_nn_complex_t *out);
void wtk_nn_istft( wtk_nn_stft_t *stft, int n_frame, float *hann_win_sum_square, wtk_nn_complex_t *in, float *y);


void wtk_nn_complex_mul2(wtk_nn_dcomplex_t *a, wtk_nn_complex_t *b, wtk_nn_dcomplex_t *c);
void wtk_nn_dcomplex_abs(wtk_nn_dcomplex_t *a, double *out);
#ifdef __cplusplus
}
#endif
#endif
