#include "wtk_nn_fft.h"
#include "wtk/tts-mer/wtk-extend/wtk_mat.h"
#include "wtk/tts-mer/wtk-extend/nn/wtk_nn_pad.h"

wtk_nn_complex_t *wtk_nn_complex_new(int len)
{
    wtk_nn_complex_t *dst;
    size_t struct_st = sizeof(*dst);
    size_t stlen = sizeof(float)*len;
    char *p = (char*)malloc(struct_st + stlen*2);
    dst = (wtk_nn_complex_t*)p;
    dst->real = (float*)(p+struct_st);
    dst->imag = dst->real + len;
    dst->len = len;
    wtk_nn_complex_zero(dst);
    return dst;
}
void wtk_nn_complex_zero(wtk_nn_complex_t *p)
{
    size_t s = p->len*sizeof(float);
    memset(p->real, 0, s);
    memset(p->imag, 0, s);
}
void wtk_nn_complex_delete(wtk_nn_complex_t *p)
{
    free(p);
}
void wtk_nn_complex_mul(wtk_nn_complex_t *a, wtk_nn_complex_t *b, wtk_nn_complex_t *c)
{/* c = a * b  */
    assert( a->len == b->len && b->len == c->len);
    int i
      , len=a->len;
    for (i=0; i<len; ++i)
    {
        c->real[i]=a->real[i]*b->real[i] - a->imag[i]*b->imag[i];
	    c->imag[i]=a->real[i]*b->imag[i] + a->imag[i]*b->real[i];
    }
}

void wtk_nn_complex_mul2(wtk_nn_dcomplex_t *a, wtk_nn_complex_t *b, wtk_nn_dcomplex_t *c)
{/* c = a * b  */
    assert( a->len == b->len && b->len == c->len);
    int i
      , len=a->len;
    for (i=0; i<len; ++i)
    {
        c->real[i]=a->real[i]*b->real[i] - a->imag[i]*b->imag[i];
	    c->imag[i]=a->real[i]*b->imag[i] + a->imag[i]*b->real[i];
    }
}

void wtk_nn_complex_abs(wtk_nn_complex_t *a, float *out)
{/* 复数取模 sqrt(r^2+i^2) */
    float 
        *real=a->real,
        *imag=a->imag;
    int i
      , len=a->len;
    
    for (i=0; i<len; ++i)
    {
        out[i]=sqrtf( real[i]*real[i]+imag[i]*imag[i] );
    }
}

void wtk_nn_dcomplex_abs(wtk_nn_dcomplex_t *a, double *out)
{/* 复数取模 sqrt(r^2+i^2) */
    double 
        *real=a->real,
        *imag=a->imag;
    int i
      , len=a->len;
    
    for (i=0; i<len; ++i)
    {
        out[i]=sqrt( real[i]*real[i]+imag[i]*imag[i] );
    }
}

void wtk_nn_complex_save(wtk_nn_complex_t *p, char fn[])
{
    int size = p->len;
    int i;
    char fn_path[1024];

    sprintf(fn_path, "%s_real.c.txt", fn);
    FILE *fp1 = fopen(fn_path, "w");
    sprintf(fn_path, "%s_imag.c.txt", fn);
    FILE *fp2 = fopen(fn_path, "w");
    for (i=0; i<size; i++) {
        fprintf(fp1, "%-10.6f ", p->real[i]);
        fprintf(fp2, "%-10.6f ", p->imag[i]);
    }
    fclose(fp1);
    fclose(fp2);
    printf("%s 保存成功 size: %d! \n", fn_path, size);
}

float* wtk_nn_hanning_heap( wtk_heap_t *heap, int size, short itype)
{
    int half, i, idx, n;
    float *w;
    // w = (double*) calloc(N, sizeof(double));
    w = heap==NULL? calloc(sizeof(*w), size): wtk_heap_zalloc( heap, sizeof(*w) * size);
    // memset(w, 0, sizeof(*w) * size);
    if (itype == 1) //periodic function
    {
        n = size - 1;
    }
    else
    {
        n = size;
    }

    if (n % 2 == 0)
    {
        half = n / 2;
        for (i = 0; i < half; i++)
        { //CALC_HANNING Calculates Hanning window samples.
            w[i] = 0.5 * (1 - cosf(2 * PI * (i + 1) / (n + 1)));
        } 
        idx = half - 1;
        for (i = half; i < n; i++)
        {
            w[i] = w[idx];
            idx--;
        }
    }
    else
    {
        half = (n + 1) / 2;
        for (i = 0; i < half; i++)
        { //CALC_HANNING Calculates Hanning window samples.
            w[i] = 0.5 * (1 - cosf(2 * PI * (i + 1) / (n + 1)));
        }
        idx = half - 2;
        for (i = half; i < n; i++)
        {
            w[i] = w[idx];
            idx--;
        }
    }
    if (itype == 1) //periodic function
    {
        for (i = size - 1; i >= 1; i--)
        {
            w[i] = w[i - 1];
        }
        w[0] = 0.0;
    }
    return w;
}

float *wtk_nn_hanning(int size, short itype)
{/* librosa 版 */
    /* function w = hanning(varargin)
% HANNING Hanning window.
% HANNING(N) returns the N-point symmetric Hanning window in a column
% vector. Note that the first and last zero-weighted window samples
% are not included.
%
% HANNING(N,'symmetric') returns the same result as HANNING(N).
%
% HANNING(N,'periodic') returns the N-point periodic Hanning window,
% and includes the first zero-weighted window sample.
%
% NOTE: Use the HANN function to get a Hanning window which has the
% first and last zero-weighted samples.ep
 itype = 1 --> periodic
 itype = 0 --> symmetric
 default itype=0 (symmetric)
*/
    return wtk_nn_hanning_heap( NULL, size, itype);
}
void wtk_nn_hanning_numpy( float *h, int m)
{/* numpy.hanning */
    int i;
    for (i=0; i<m; ++i)
    {
        h[i]=0.5 - 0.5*cosf(2.0*PI*i/(m-1));
    }
}
// wtk_nn_fft_t wtk_nn_fft_new(int nfft,int is_inverse_fft,void * mem,size_t * lenmem )
// {
//     kiss_fft_cfg cfg = kiss_fft_alloc( nfft ,is_inverse_fft ,0,0 );
//     return cfg;
// }
// void wtk_nn_fft(wtk_nn_fft_t cfg, wtk_complex_t *fin, wtk_complex_t *fout)
// {
//     kiss_fft(cfg, (kiss_fft_cpx*)fin, (kiss_fft_cpx*)fout);
// }
// void wtk_nn_fft_delete(wtk_nn_fft_t cfg)
// {
//     kiss_fft_free(cfg);
// }

/* 默认虚部为0
cache = nfft*2
x == real
y == imag
x 即是输入也是输出
 */
void wtk_nn_rfft(wtk_rfft_t *rf, float *cache, float *x, float *y)
{
    int m=rf->len
      , win=rf->win
      , i;
    float 
        *f = cache + m;
    wtk_rfft_process_fft(rf, f, x);
    x[0] = f[0];
    y[0] = 0;
    for (i=1; i<win; ++i)
    {
        x[i]=f[i];
        y[i]=-f[i+win];
    }
    x[win]=f[win];
    y[win]=0;
    for(i=win+1; i<m; ++i)
    {
        x[i]=f[m-i];
        y[i]=f[m-i+win];
    }
}
void wtk_nn_rfft_ifft(wtk_rfft_t *rf, float *cache, float *x, float *y)
{/* 
x: real
y: imag
只能得到实部的值 
*/
    float *f;
    int i
        , n = rf->len
        , half = rf->win;
    f = cache;

    f[0] = x[0];
    for (i=1; i<half; ++i)
    {
        f[i] = x[i];
        f[i+half] = -y[i];
    }
    f[half] = x[half];
    for(i=half+1; i<n; ++i)
    {
        f[n-i] = x[i];
        f[n-i+half] = y[i];
    }
    wtk_rfft_process_ifft(rf, f, x);
}

/* stft的hanning窗和numpy.hanning的不一样 */
wtk_nn_stft_t* wtk_nn_stft_heap_new( wtk_heap_t *heap, wtk_rfft_t *rf, int nfft, int hop_len, int win_len)
{
    wtk_nn_stft_t *stft=wtk_heap_malloc( heap, sizeof(*stft));
    int nfreq=1+nfft/2
      , i;
    float 
        *hann=wtk_nn_hanning_heap( heap, win_len, 1),
        // *hann=wtk_heap_zalloc( heap, sizeof(float*)*win_len),
        *win_hann=hann,
        *win_hann_sum=wtk_heap_malloc( heap, sizeof(float)*nfft);
    // wtk_nn_hanning_numpy( hann, win_len);

    if (win_len < nfft)
    {
        win_hann=wtk_heap_malloc( heap, sizeof(*win_hann)*nfft);
        wtk_nn_pad_float( wtk_nn_pad_type_center, hann, win_len, win_hann, nfft, (nfft-win_len)/2);
    }
    for (i=0; i<nfft; ++i)
    {
        win_hann_sum[i] = powf(fabsf(win_hann[i]), 2);
    }

    stft->rf=rf;
    stft->nfft=nfft;
    stft->nfreq=nfreq;
    stft->win_len=win_len;
    stft->hop_len=hop_len;
    stft->win_hann=win_hann;
    stft->win_hann_sum=win_hann_sum;
    stft->fft_cache=wtk_heap_malloc( heap, sizeof(float)*nfft*2);
    stft->fft_x=wtk_heap_malloc( heap, sizeof(float)*nfft*2);
    stft->fft_y= stft->fft_x + nfft;
    return stft;
}
float* wtk_nn_istft_win_sumsquare_heap_new( wtk_heap_t *heap, wtk_nn_stft_t *stft, int n_frame)
{
    int 
        i, k, j,
        sample,
        hop_len=stft->hop_len,
        n_fft=stft->nfft,
        win_len=(n_fft + hop_len * (n_frame - 1));
    float
        *win_hann_sum_square=wtk_heap_zalloc( heap, sizeof(float)*win_len);
    
    for (i=0; i<n_frame; ++i)
    {
        sample = i*hop_len;
        for (j=sample, k=0; k<n_fft; ++k, ++j)
        {
            win_hann_sum_square[j] += stft->win_hann_sum[k];
        }
    }
    return win_hann_sum_square;
}
/* 
已验证同步python库 librosa.stft(in, hop_size, win_length=win_size, window="hann", center = True)
请在此函数外实现 y=np.pad( y, n_fft/2)
n_frame = ((in_len-nfft)/hop_len + 1);

y_frame.shape = [ n_frame, n_fft]
in_len = (n_frame-1)*hop_size

stft_matrix = np.empty((int(1 + n_fft // 2), y_frames.shape[1]),
                           dtype=dtype,
                           order='F')
# how many columns can we fit within MAX_MEM_BLOCK?
n_columns = int(util.MAX_MEM_BLOCK / (stft_matrix.shape[0] *
                                        stft_matrix.itemsize))

for bl_s in range(0, stft_matrix.shape[1], n_columns):
    bl_t = min(bl_s + n_columns, stft_matrix.shape[1])

    stft_matrix[:, bl_s:bl_t] = fft.fft(fft_window *
                                        y_frames[:, bl_s:bl_t],
                                        axis=0)[:stft_matrix.shape[0]]

return stft_matrix
 */
void wtk_nn_stft( wtk_nn_stft_t *stft, float *y, int y_len, wtk_nn_complex_t *out)
{
    wtk_heap_t *heap = wtk_heap_new(4096);
    wtk_rfft_t *rf=stft->rf;
    int n_freq = stft->nfreq
      , n_fft = stft->nfft
      , hop_len = stft->hop_len
      , n_frame = (y_len-n_fft)/hop_len + 1
      , i, j, k;
    float 
        *y_frame = wtk_heap_zalloc( heap, sizeof(float)*n_fft*n_frame),
        *y_mf = y_frame,
        *fft_cache = stft->fft_cache,
        *fft_x = stft->fft_x,
        *fft_y = fft_x + n_fft,
        *win_hann=stft->win_hann,
        *out_real = out->real,
        *out_imag = out->imag;

    for (j=0; j<n_frame; ++j)
    {
        for (k=0; k<n_fft; ++k)
        {
            (*y_mf) = y[j*hop_len+k];
            y_mf++;
        }
    }

    y_mf = y_frame;
    for (i=0; i<n_frame; ++i)
    {
        memset(fft_y, 0, sizeof(float)*n_fft);
        for (k=0; k<n_fft; ++k, ++y_mf)
        {
            fft_x[k] = (*y_mf) * win_hann[k];
        }
        wtk_nn_rfft(rf, fft_cache, fft_x, fft_y);
        memcpy(out_real, fft_x, sizeof(float)*n_freq);
        memcpy(out_imag, fft_y, sizeof(float)*n_freq);
        out_real+=n_freq;
        out_imag+=n_freq;
    }
    wtk_heap_delete(heap);
}
/* 
已验证同步python库 librosa.istft(in, hop_size, win_length=win_size, window="hann", center = True)

y_len=(n_fft + hop_len * (n_frame - 1));

np.conj() 共轭
hann_win: np.pad(np.hanning( win_size), n_fft, mode = 'constant')
win_hann_sum_square: 
    参考 librosa.filters.window_sumsquare
    x = np.zeros(hop_size*(n_frame - 1) + n_fft, dtype=dtype)
    win_sq = abs(hann_win)
    for i in range(n_frames):
        sample = i * hop_sizegth
        x[sample:min(n, sample + n_fft)] += win_sq[:max(0, min(n_fft, n - sample))]

for i in range(n_frames):
    sample = i * hop_sizegth
    spec = stft_matrix[:, i].flatten()
    spec = np.concatenate((spec, spec[-2:0:-1].conj()), 0)
    ytmp = hann_win * fft.ifft(spec).real
    y[sample:(sample + n_fft)] = y[sample:(sample + n_fft)] + ytmp

for (i=0; i<y_len; ++i)
{
    if (win_hann_sum_square[i]>0)
    { y[i] /= win_hann_sum_square[i];}
}

 */
void wtk_nn_istft( wtk_nn_stft_t *stft, int n_frame, float *win_hann_sum_square, wtk_nn_complex_t *in, float *y)
{
    wtk_rfft_t *rf=stft->rf;
    int i, j, k
      , sample
      , n_fft=stft->nfft
      , n_freq=stft->nfreq
      , hop_len=stft->hop_len
      , y_len=hop_len*(n_frame - 1) + n_fft;
    size_t
        spec_stlen = sizeof(float)*n_fft,
        nfft_half = spec_stlen/2;
    float 
        *fft_cache = stft->fft_cache,
        *spec_real = stft->fft_x,
        *spec_imag = stft->fft_y,
        *win_hann_pad = stft->win_hann;
    float 
        *in_real = in->real,
        *in_imag = in->imag;

    memset(y, 0, sizeof(*y)*y_len);
    for (i=0; i<n_frame; ++i, in_real+=n_freq, in_imag+=n_freq)
    {
        sample = i*hop_len;
        memcpy(spec_real, in_real, sizeof(float)*n_freq);
        memcpy(spec_imag, in_imag, sizeof(float)*n_freq);
        for (j=n_freq-2, k=n_freq; j>0; --j, ++k)
        {
            spec_real[k] = in_real[j];
            spec_imag[k] = -in_imag[j]; // 共轭
        }
        wtk_nn_rfft_ifft(rf, fft_cache, spec_real, spec_imag);
        for (j=sample, k=0; k<n_fft; ++k, ++j)
        {
            y[j] += win_hann_pad[k] * spec_real[k];
        }
    }
    for (i=0; i<y_len; ++i)
    {
        if (fabsf(win_hann_sum_square[i])>0)
        { y[i] /= win_hann_sum_square[i];}
    }
    memset(y, 0, nfft_half);
    memset(y+y_len-n_fft/2, 0, nfft_half);
}