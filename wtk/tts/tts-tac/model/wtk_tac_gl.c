#include "wtk_tac_gl.h"

static void denormalize(wtk_tac_hparams_t *hp, float *p, int len)
{
    int is_symmetric_mels = hp->symmetric_mels;
    float
        max_abs_value = hp->max_abs_value,
        min_level_db = hp->min_level_db,
        *pcur,
        *pe = p+len;
    if (!is_symmetric_mels)
    {
        wtk_debug("未实现的分支: not symmetric_mels \n");
        wtk_exit(1);
    }
    if (hp->allow_clip_in_norm)
    {
        if (is_symmetric_mels)
        {
            wtk_float_set_bound(-max_abs_value, max_abs_value, p, len);
        } else {
            wtk_float_set_bound(0, max_abs_value, p, len);
        }
    }
    if (is_symmetric_mels)
    {
        pcur = p;
        while (pcur<pe)
        {
            (*pcur) = ((*pcur)+max_abs_value)*(-min_level_db)/(2*max_abs_value) + min_level_db;
            pcur++;
        }
    } else {}
}

static void db_to_map(float *p, int len, float beta)
{
    float *pe = p+len;
    while(p<pe){
        *p = powf(10, (*p+beta) * 0.05);
        p++;
    }
}

static void istft( wtk_rfft_t *rf, int n_fft, int n_frame, int hop_size, int win_size, float *hann_win_pad, float *hann_win_sum_square, wtk_nn_complex_t *in, float *y)
{/* 
已验证同步python库 librosa.istft(in, hop_size, win_length=win_size, window="hann", center = False)

np.conj() 共轭
hann_win: np.pad(np.hanning( win_size), n_fft, mode = 'constant')
hann_win_sum_square: 
    参考 ibrosa.filters.window_sumsquare
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
    if (hann_win_sum_square[i]>0)
    { y[i] /= hann_win_sum_square[i];}
}

 */
    wtk_heap_t *heap = wtk_heap_new(1024);
    int i, j, k
      , sample
      , n_freq = n_fft/2 + 1
      , y_len = hop_size*(n_frame - 1) + n_fft;
    size_t
        spec_stlen = sizeof(float)*n_fft,
        nfft_half = spec_stlen/2;
    float 
        *fft_cache = wtk_heap_malloc( heap, sizeof(float)*n_fft*2),
        *spec_real = wtk_heap_malloc( heap, spec_stlen*2),
        *spec_imag = spec_real + n_fft;
    float 
        *in_real = in->real,
        *in_imag = in->imag;
    for (i=0; i<n_frame; ++i, in_real+=n_freq, in_imag+=n_freq)
    {
        sample = i*hop_size;
        memcpy(spec_real, in_real, sizeof(float)*n_freq);
        memcpy(spec_imag, in_imag, sizeof(float)*n_freq);
        for (j=n_freq-2, k=n_freq; j>0; --j, ++k)
        {
            spec_real[k] = in_real[j];
            spec_imag[k] = -in_imag[j]; // 共轭
        }
        // sptk_ifft_float(spec_real, spec_imag, n_fft);
        wtk_nn_rfft_ifft(rf, fft_cache, spec_real, spec_imag);
        for (j=sample, k=0; k<n_fft; ++k, ++j)
        {
            y[j] += hann_win_pad[k] * spec_real[k];
        }
    }
    for (i=0; i<y_len; ++i)
    {
        if (fabsf(hann_win_sum_square[i])>0)
        { y[i] /= hann_win_sum_square[i];}
    }
    memset(y, 0, nfft_half);
    memset(y+y_len-n_fft/2, 0, nfft_half);
    wtk_heap_delete(heap);
}

static void stft( wtk_rfft_t *rf, int n_fft, int n_frame, int hop_size, float *hann_win, float *y_frame, float *in, wtk_nn_complex_t *out)
{/* 
已验证同步python库 librosa.stft(in, hop_size, win_length=win_size, window="hann", center = False)

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
 */
    wtk_heap_t *heap = wtk_heap_new(4096);
    int n_freq = 1+n_fft/2
      , i, j, k;
    float 
        *y_mf = y_frame,
        *fft_cache = wtk_heap_malloc( heap, sizeof(float)*n_fft*2),
        *fft_x = wtk_heap_malloc( heap, sizeof(float)*n_fft*2),
        *fft_y = fft_x + n_fft,
        *out_real = out->real,
        *out_imag = out->imag;

    for (j=0; j<n_frame; ++j)
    {
        for (k=0; k<n_fft; ++k)
        {
            (*y_mf) = in[j*hop_size+k];
            y_mf++;
        }
    }

    y_mf = y_frame;
    for (i=0; i<n_frame; ++i)
    {
        memset(fft_y, 0, sizeof(float)*n_fft);
        for (k=0; k<n_fft; ++k, ++y_mf)
        {
            fft_x[k] = (*y_mf) * hann_win[k];
        }
        // sptk_rfft_float(rf, fft_cache, fft_x, fft_y);
        wtk_nn_rfft(rf, fft_cache, fft_x, fft_y);
        memcpy(out_real, fft_x, sizeof(float)*n_freq);
        memcpy(out_imag, fft_y, sizeof(float)*n_freq);
        out_real+=n_freq;
        out_imag+=n_freq;
    }
    wtk_heap_delete(heap);
}

static void griffin_lim( wtk_tac_hparams_t *hp, wtk_rfft_t *rf, float *hann_win, float *hann_win_sum, int n_frame, float *in, int in_len, float *y, int y_len)
{
    wtk_heap_t *heap = wtk_heap_new(4096);
    int griffin_lim_iters = hp->griffin_lim_iters
      , num_freq = hp->num_freq
      , hop_size = hp->hop_size
      , win_size = hp->win_size
      , n_fft = (num_freq-1)*2
      , sample
      , i, j, k;
    float
        *istft_out = y,
        *hann_win_sum_square = wtk_heap_malloc( heap, sizeof(float)*y_len),
        *y_frame = wtk_heap_malloc( heap, sizeof(float)*n_fft*n_frame),
        cpx_abs,
        fr,
        fi;
    wtk_nn_complex_t *cpx = wtk_nn_complex_new(in_len);
    wtk_nn_complex_zero(cpx);

    memset(hann_win_sum_square, 0, sizeof(float)*y_len);
    for (i=0; i<n_frame; ++i)
    {
        sample = i*hop_size;
        for (j=sample, k=0; k<n_fft; ++k, ++j)
        {
            hann_win_sum_square[j] += hann_win_sum[k];
        }
    }
    // wtk_matf_t gim;
    // gim.p = istft_out;
    // gim.row = 1;
    // gim.col = y_len;

    // wtk_exit(1);
    wtk_debug("计算 griffin_lim \n");
    memcpy(cpx->real, in, sizeof(float)*in_len);
    // wtk_mer_matf_write_file(&gim, "../tac_data/gim.cpx.c.txt", 0);
    // wtk_mer_matf_write_file(&gim, "../tac_data/gim.cpx.c2.txt", 0);
    for (i=0; i<griffin_lim_iters; ++i)
    {
        istft( rf, n_fft, n_frame, hop_size, win_size, hann_win, hann_win_sum_square, cpx, istft_out);
        // if (i==0) 
        // {
        //     // wtk_mer_matf_write_file(&gim2, "../tac_data/gim.c.txt", 0);
        //     // wtk_mer_matf_write_file(&gim, "../tac_data/gim.cpx.c.txt", 0);
        //     // wtk_mer_matf_write_file(&gim, "../tac_data/gim.cpx.c2.txt", 0);
        //     // wtk_exit(1);
        // }
        stft( rf, n_fft, n_frame, hop_size, hann_win, y_frame, istft_out, cpx);
        for (j=0; j<in_len; ++j)
        {
            fr = cpx->real[j];
            fi = cpx->imag[j];
            cpx_abs = max(1E-8, sqrtf(fr*fr+ fi*fi));
            cpx->real[j] = in[j] * fr / cpx_abs;
            cpx->imag[j] = in[j] * fi / cpx_abs;
        }
        // if (i==2)
        // {
        //     wtk_mer_matf_write_file(&gim, "../tac_data/gim.cpx.c.txt", 0);
        //     // wtk_mer_matf_write_file(&gim, "../tac_data/gim.cpx.c2.txt", 0);
        //     // wtk_exit(1);
        // }
    }
    istft( rf, n_fft, n_frame, hop_size, win_size, hann_win, hann_win_sum_square, cpx, istft_out);
    // wtk_mer_matf_write_file(&gim2, "../tac_data/gim.c.txt", 0);
    wtk_nn_complex_delete(cpx);
    wtk_heap_delete(heap);
}

static void foutput(void *src, size_t size, int n, void **dst, char type)
{
    int s;
    FILE *fp;
    switch (type)
    {
      case 'w':
        fp = (FILE*)(*dst);
        fwrite(src, size, n, fp);
        break;
      case 'm':
        s = size*n;
        memcpy(*dst, src, s);
        (*dst) = (char*)(*dst) + s;
        break;
    }
}

static void wavwrite2(const float *x, int x_length, int fs,
    void *fp) {
  char type = 'm';
  int i;

  char text[4] = {'R', 'I', 'F', 'F'};
  uint32_t long_number = 36 + x_length*2;
  foutput(text, 1, 4, &fp, type);
  foutput(&long_number, 4, 1, &fp, type);

  text[0] = 'W';
  text[1] = 'A';
  text[2] = 'V';
  text[3] = 'E';
  foutput(text, 1, 4, &fp, type);
  text[0] = 'f';
  text[1] = 'm';
  text[2] = 't';
  text[3] = ' ';
  foutput(text, 1, 4, &fp, type);

  long_number = 16;
  foutput(&long_number, 4, 1, &fp, type);
  int16_t short_number = 1;
  foutput(&short_number, 2, 1, &fp, type);
  short_number = 1;
  foutput(&short_number, 2, 1, &fp, type);
  long_number = fs;
  foutput(&long_number, 4, 1, &fp, type);
  long_number = fs * 2;
  foutput(&long_number, 4, 1, &fp, type);
  short_number = 2;
  foutput(&short_number, 2, 1, &fp, type);
  short_number = 16;
  foutput(&short_number, 2, 1, &fp, type);

  text[0] = 'd';
  text[1] = 'a';
  text[2] = 't';
  text[3] = 'a';
  foutput(text, 1, 4, &fp, type);
  long_number = x_length * 2;
  foutput(&long_number, 4, 1, &fp, type);

  int16_t tmp_signal;
  for (i = 0; i < x_length; ++i) {
    tmp_signal = (short)x[i];
    foutput(&tmp_signal, 2, 1, &fp, type);
  }
  if (type == 'w') fclose((FILE*)fp);
}

void save_wav(float *y, int y_len, void *outp)
{
    float 
        *pcur = y,
        *pe = y + y_len,
        maxf;
    maxf = max(0.01, wtk_float_abs_max(y, y_len));
    while(pcur < pe)
    {
        *pcur *= 32767 / maxf;
        pcur++;
    }
    
    // FILE *fp = fopen("output/test.wav", "w");
    // wtk_debug("output/test.wav\n");
    // FILE *fp = fopen("../tac_data/test.wav", "w");
    // wtk_debug("../tac_data/test.wav\n");
    wavwrite2(y, y_len, 22050, outp);
}

static void inv_preemphasis(float *y, int y_len, float k)
{/* signal.lfilter([1], [1, -k], y) 
一阶滤波公式
Y(n)=αX(n) (1-α)Y(n-1)
式中：α=滤波系数；X(n)=本次采样值；Y(n-1)=上次滤波输出值；Y(n)=本次滤波输出值。 
一阶低通滤波法采用本次采样值与上次滤波输出值进行加权，得到有效滤波值，使得输出对输入有反馈作用。
*/
    int i;
    float prev = y[0], a=1-k;
    for (i=0; i<y_len; ++i)
    {
        prev = y[i] = a*y[i] + k*prev;
    }
}

void wtk_tac_griffin_lim(wtk_tac_hparams_t *hp, wtk_mer_wav_stream_t *wav, wtk_rfft_t *rf, float *hann_win, float *hann_win_sum, wtk_matf_t *linear)
{
    int len = linear->row*linear->col
      , n_frame = linear->row
      , hop_size = hp->hop_size
      , num_freq = hp->num_freq
      , n_fft = (num_freq-1)*2
      , y_len = (n_frame-1)*hop_size + n_fft;
    float 
        *p = linear->p,
        *pcur,
        *pe = p+len,
        power = 1/hp->magnitude_power * hp->power,
        ref_level_db = hp->ref_level_db,
        *y = calloc(y_len, sizeof(float));

    wav->len = y_len*sizeof(short)+44;
    if (hp->signal_norm)
    {
        denormalize(hp, p, len);
    }
    db_to_map(p, len, ref_level_db);
    pcur = p;
    while(pcur<pe)
    {
        *pcur = powf(*pcur, power);
        pcur++;
    }
    // wtk_matf_t gim;
    // gim.p = y;
    // gim.row = 1;
    // gim.col = y_len;
    // wtk_mer_matf_write_file(&gim, "../tac_data/gim.c2.txt", 0);
    // wtk_mer_matf_write_file(&gim, "../tac_data/gim.c.txt", 0);
    griffin_lim(hp, rf, hann_win, hann_win_sum, linear->row, linear->p, len, y, y_len);
    inv_preemphasis(y, y_len, hp->preemphasis);
    // wtk_mer_matf_write_file(&gim, "../tac_data/gim.c2.txt", 0);
    // wtk_mer_matf_write_file(&gim, "../tac_data/gim.c.txt", 0);
    // save_wav(y, y_len, wav->data);
    wtk_mer_wav_stream_write_float(wav, y, y_len);
    free(y);
}
