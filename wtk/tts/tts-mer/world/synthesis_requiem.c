#include "synthesis_requiem.h"
#include "wtk_world_plural.h"
#include "tts-mer/sptk/SPTK.h"

typedef struct
{
    double *pulse;
    double *noise;
    int prow;
    int pcol;
    int nrow;
    int ncol;
} wtk_world_pulse_noise_t;

static int randint(int m, int n)
{ // rand()%(n - m + 1) + m; [m,n]
    return rand()%(n - m + 1) + m;
    // return m + 1;
}

static int *generate_short_velvet_noise(int len)
{
    int *n = malloc(sizeof(int)*len);
    int td = 4, i;
    int r = (int)(len / td + 0.5);
    int *safety_rand = malloc(sizeof(int)*r);

    // printf("r: %d \n", r);
    memset(n, 0, sizeof(int) * len);
    for (i = 0; i < r; i++)
    {
        safety_rand[i] = i >= r / 2 ? -2 : 2;
    }

    int tmp_index, tmp;
    for (i = 0; i < r; i++)
    { /* 切记不用改异或, 因为tmp_index有可能等于i */
        tmp_index = randint(0, r - 1);
        tmp = safety_rand[tmp_index];
        safety_rand[tmp_index] = safety_rand[i];
        safety_rand[i] = tmp;
    }
    // printf("\n");
    int t;
    int a;
    for (i = 0; i < r; i++)
    {
        a = randint(0, td-1);
        t = td * i + a;
        n[t] = safety_rand[i];
        // if (t>=len) {printf("t: %d len: %d r: %d a:%d\n", t, len, r, a);}
    }
    // printf("\n");
    free(safety_rand);
    return n;
}

static double *generate_modified_velvet_noise(int n, int fs)
{
    int i, index, v_len, size = 3;
    int base_period[3] = {8, 30, 60}, short_period[3];

    for (i = 0; i < size; i++)
    {
        short_period[i] = 8 * matlab_round(base_period[i] * fs / 48000.0);
    }

    int noise_len = n + short_period[size - 1] + 1;
    int *noise = malloc(sizeof(int)*noise_len);
    double *dst = malloc(sizeof(*dst)*n);
    int *tmp, short_n;

    memset(noise, 0, noise_len);
    index = 0;
    for (;;)
    {
        v_len = randint(0, size - 1);
        short_n = short_period[v_len];
        tmp = generate_short_velvet_noise(short_n);
        memcpy(noise + index, tmp, sizeof(int) * short_n);
        index += short_n;
        free(tmp);
        if (index >= n - 1)
            break;
    }
    for (i = 0; i < n; i++)
    {
        dst[i] = (double)(noise[i]);
    }
    free(noise);
    return dst;
}

void wtk_world_pulse_noise_delete(wtk_world_pulse_noise_t *p)
{
    free(p);
}

wtk_world_pulse_noise_t* wtk_world_pulse_noise_new(int prow, int pcol, int nrow, int ncol)
{
    wtk_world_pulse_noise_t *dst;
    size_t s = sizeof(*dst);
    int plen = prow*pcol;
    char *p = (char*)malloc(s + sizeof(double)*plen + sizeof(double)*nrow*ncol);
    dst = (wtk_world_pulse_noise_t*)p;
    dst->pulse = (double*)(p+s);
    dst->noise = dst->pulse+plen;
    dst->prow = prow;
    dst->pcol = pcol;
    dst->nrow = nrow;
    dst->ncol = ncol;
    return dst;
}

static double accumulate(double *p, double *pe, double total)
{
    while (p<pe)
    {
        total += *p++;
    }
    return total;
}

wtk_world_pulse_noise_t* get_seeds_signals(int fs, int fft_size/* 0 */, int noise_len/* 0 */)
{
    int i, j, k;

    if (fft_size == 0)
    {/* 512 */
        fft_size = (int)(1024 * pow(2, ceil(log(fs / 48000.0) / log(2.0))));
    }
    if (noise_len == 0)
    {
        noise_len = (int)(pow(2, ceil(log(fs / 2.0) / log(2.0))));
    }

    int w_len = fft_size / 2 + 1;
    double *w = malloc(sizeof(*w)*w_len);
    for (i = 0; i < w_len; i++)
    {
        w[i] = i * fs * 1.0 / fft_size;
    }

    int frequency_interval = 3000
      , frequency_range = frequency_interval * 2
      , upper_limit = 15000
      , number_of_aperiodicities = (int)(2 + floor(min(upper_limit, fs / 2 - frequency_interval) * 1.0 / frequency_interval)); // 3

    double *modified_velvet_noise = generate_modified_velvet_noise(noise_len, fs);

    wtk_world_pulse_noise_t *pulse_noise = wtk_world_pulse_noise_new(fft_size, number_of_aperiodicities, noise_len, number_of_aperiodicities);
    wtk_world_plural_t
        *spec_n = wtk_world_plural_new(noise_len),
        *spec_pl = wtk_world_plural_new(fft_size),
        *pulse_fft = wtk_world_plural_new(noise_len);
    
    int r_spec_len = spec_pl->size
      , last_i = number_of_aperiodicities - 1;
    double
        // *pl = pulse_noise->pulse,
        pi2 = 2 * PI,
        *pulse = pulse_noise->pulse,
        *noise = pulse_noise->noise,
        *spec = malloc(sizeof(*spec)*w_len),
        *spec_shift = malloc(sizeof(*spec_shift)*r_spec_len);

    memcpy(spec_n->real, modified_velvet_noise, sizeof(double) * noise_len);
    sptk_fft(spec_n->real, spec_n->imag, spec_n->size);

    for (i = 0; i < number_of_aperiodicities; ++i)
    {
        int f0 = frequency_interval * i
          , f1 = frequency_interval * (i + 1)
          , f2 = frequency_interval * (i - 1);
        for (j = 0; j < w_len; j++)
        {
            double w_tmp;
            w_tmp = w[j];
            spec[j] = 0.5 + 0.5 * cos(((w_tmp - f0) / frequency_range) * pi2);
            if (w_tmp > f1)
                spec[j] = 0;
            else if (w_tmp < f2)
                spec[j] = 0;

            if (i == last_i)
            {
                if (w_tmp > f0)
                    spec[j] = 1;
            }
        }
        // pulse[:,i] = fftshift(ifft(np.r_[spec, spec[-2:0:-1]]).real)
        wtk_world_plural_zero(spec_pl);
        memcpy(spec_pl->real, spec, sizeof(*spec) * w_len);
        for (k = w_len - 2, j = 0; k--; j++)
        {
            spec_pl->real[w_len + j] = spec[k+1];
        }
        sptk_ifft(spec_pl->real, spec_pl->imag, spec_pl->size);
        fftshift(spec_pl->real, spec_pl->size, spec_shift);
        for (j = 0; j < r_spec_len; j++)
        {/* 转置存储 */
            pulse[j * number_of_aperiodicities + i] = spec_shift[j];
            // *pl++ = spec_shift[j];
        }

        // noise[:,i] = ifft(spec_n * fft(pulse[:,i], noise_length)).real
        wtk_world_plural_zero(pulse_fft);
        memcpy(pulse_fft->real, spec_shift, r_spec_len * sizeof(double));
        sptk_fft(pulse_fft->real, pulse_fft->imag, pulse_fft->size);
        wtk_world_plural_mul(spec_n, pulse_fft, pulse_fft);
        sptk_ifft( pulse_fft->real, pulse_fft->imag, pulse_fft->size);
        for (j=0; j<noise_len; j++)
        {
            noise[j*number_of_aperiodicities+i] = pulse_fft->real[j];
        }
    }

    double 
        *h = matlab_hanning(fft_size, 0),
        // h_mean = std::accumulate(h, h+fft_size, 0.0)/fft_size,
        h_mean = accumulate(h, h+fft_size, 0.0)/fft_size,
        pulse_mean=0, pulse_sum=0;
    for (j=0; j<fft_size; j++) {
        pulse_sum += pulse[j*number_of_aperiodicities];
    }
    pulse_mean = pulse_sum / fft_size;
    for (j=0; j<fft_size; j++)
    {
        double tmp = pulse[j*number_of_aperiodicities];
        pulse[j*number_of_aperiodicities] = tmp - pulse_mean*h[j]/h_mean;
    }
    free(spec_shift);
    free(spec);
    free(w);
    free(modified_velvet_noise);
    free(h);
    wtk_world_plural_delete(spec_n);
    wtk_world_plural_delete(spec_pl);
    wtk_world_plural_delete(pulse_fft);

    return pulse_noise;
}

static void aperiodicity_generation(double *temporal_positions, int f0_len, float *band_aperiodicity, int number_of_aperiodicities, double *time_axis, int y_len, float *multi_aperiodicity)
{
    int i, j;
    double
        *coarse_aperiodicity = malloc(sizeof(*coarse_aperiodicity)*f0_len);

    for (j=0; j<number_of_aperiodicities; j++)
    {
        for (i=0; i<f0_len; i++)
        {
            coarse_aperiodicity[i] = pow(10.0, band_aperiodicity[j*f0_len+i]/10.0);
        }
        interp1(temporal_positions, coarse_aperiodicity,
    f0_len, time_axis, y_len,
        multi_aperiodicity+j*y_len);
    }
    free( coarse_aperiodicity);
}

static void generate_noise(int y_len, double *noise_seed, int nrow, int ncol, int *current_index, int frequency_band, float *n)
{
    int noise_len = nrow
      , index
      , tmp
      , i;

    // printf("noise_len: %d \n", noise_len);
    tmp = current_index[frequency_band];
    for (i=0; i<y_len; i++)
    {
        index = (tmp+i)%noise_len*ncol + frequency_band;
        // n[i] = noise_seed[index[i]*ncol+frequency_band];
        n[i] = noise_seed[index];
    }
    // current_index[frequency_band] = index[y_len-1];
    current_index[frequency_band] = (tmp+y_len-1)%noise_len;
}

static void get_one_periodic_excitation(int number_of_aperiodicities, double *pulse_seed, int prow, int pcol, float *aperiodicity, float noise_size, float *response)
{
    int i, j
      , n = prow-4;
    float tmp;
    for (i=0; i<number_of_aperiodicities; ++i)
    {
        tmp = 1 - aperiodicity[i];
        // for (j=0; j<n; j+=4, pulse_seed+=4)
        for (j=0; j<prow; ++j)
        {
            response[j] += pulse_seed[j*pcol+i] * tmp;
            // response[j] += (*pulse_seed++) * tmp;
            // response[j] += pulse_seed[0] * tmp;
            // response[j+1] += pulse_seed[1] * tmp;
            // response[j+2] += pulse_seed[2] * tmp;
            // response[j+3] += pulse_seed[3] * tmp;
        }
        // for (;j<prow; ++j)
        // {
        //     response[j] += (*pulse_seed++) * tmp;
        // }
    }
    for (j=0; j<n; j+=4)
    {
        response[j] *= noise_size;
        response[j+1] *= noise_size;
        response[j+2] *= noise_size;
        response[j+3] *= noise_size;
    }
    for (; j<prow; ++j)
    {
        response[j] *= noise_size;
    }
}

static void get_excitation_signal(double *temporal_positions, float *f0, int f0_len, int fs, int y_len, float *ap, int ap_len, wtk_world_pulse_noise_t *seeds_signals, double frame_period, float *excitation_signal)
{
    int fft_size = seeds_signals->prow // 此处fft_size只有一半, 是512
      , number_of_aperiodicities = seeds_signals->pcol
      , *base_index = malloc(sizeof(int)*fft_size)
      , *output_buffer_index = malloc(sizeof(int)*fft_size)
      , *vuv = malloc(sizeof(int)*f0_len)
      , *pulse_locations_index = malloc(sizeof(int)*y_len)
      , number_of_pulses
      , nrow = seeds_signals->nrow
      , ncol = seeds_signals->ncol
      , prow = seeds_signals->prow
      , pcol = seeds_signals->pcol
      , *cur_index = malloc(sizeof(int)*ncol)
      , i, j;
    float
        *p,
        *periodic_component = calloc( y_len, sizeof(float)),
        *aperiodic_component = calloc( y_len, sizeof(float)),
        *noise = malloc(sizeof(float)*y_len),
        *interpolated_vuv = malloc(sizeof(float)*y_len),
        *interpolated_aperiodicity = calloc( number_of_aperiodicities*y_len, sizeof(float)),
        *band_aperiodicity = malloc(sizeof(float)*ap_len*number_of_aperiodicities),
        *aperiodicity = malloc(sizeof(float)*number_of_aperiodicities),
        *response = malloc(sizeof(float)*prow), //512;
        noise_size;
    double 
        *pulse_seed = seeds_signals->pulse,
        *noise_seed = seeds_signals->noise,
        *time_axis = malloc(sizeof(*time_axis)*y_len);

    for (i=fft_size/2, j=0; i; i--, j++ )
    {
        base_index[j] = -1*i+1;
        base_index[fft_size-1-j] = i;
    }
    // printf("y_len: %d \n", y_len);
    
    for (i=0; i<y_len; i++)
    {
        // time_axis[i] = 1.0*i/fs;
        time_axis[i] = i / (double)(fs);
    }
    for (i=0; i<f0_len; i++)
    {
        vuv[i] = f0[i] > 0 ? 1:0;
    }

    frame_period /= 1000.0;
    number_of_pulses = GetTimeBase(
        f0, f0_len, fs, frame_period, time_axis, y_len,
        pulse_locations_index, interpolated_vuv);
    // printf("number_of_pulses %d f0_len: %d\n", number_of_pulses, f0_len);
    // exit(1);
    j = f0_len + f0_len;
    for (i=0; i<ap_len; i++)
    {
        int flag = (ap[i] > 0.00001 || ap[i] < -0.00001) ? 1:0;
        band_aperiodicity[i] = flag? -60: -1e-12;
        band_aperiodicity[f0_len+i] = flag? ap[i]: -1e-12;
        band_aperiodicity[j+i] = flag? -1e-12: -1e-12;
    }

    aperiodicity_generation(temporal_positions, f0_len, band_aperiodicity, number_of_aperiodicities, time_axis, y_len, interpolated_aperiodicity);

    memset(cur_index, 0, sizeof(int)*ncol);
    for (i=0; i<number_of_aperiodicities; i++)
    {
        generate_noise(y_len, noise_seed, nrow, ncol, cur_index, i, noise);
        p = interpolated_aperiodicity + i*y_len;
        for (j=0; j<y_len; j++)
        {
            aperiodic_component[j] += noise[j] * p[j];
        }
    }
    // printf("%d \n", number_of_pulses);
    // exit(1);
    int a0 = 0
      , a1 = y_len
      , a2 = y_len + y_len;
    for (i=0; i<number_of_pulses; i++)
    {
        int pli = pulse_locations_index[i];
        int ai = pli - 1;
        if (interpolated_vuv[pli-1] <= 0.5 || interpolated_aperiodicity[pli-1] > 0.999) {continue;}

        noise_size = pulse_locations_index[min(number_of_pulses-1, i+1)] - pli;
        noise_size = sqrtf(max(1, noise_size));

        for (j=0; j<fft_size; j++)
        {
            int tmp;
            tmp = min(y_len, base_index[j] + pli);
            tmp = max(1, tmp);
            output_buffer_index[j] = tmp;
        }
        // aperiodicity = interpolated_aperiodicity[:, pulse_locations_index[i]-1]
        aperiodicity[0] = interpolated_aperiodicity[a0 + ai];
        aperiodicity[1] = interpolated_aperiodicity[a1 + ai];
        aperiodicity[2] = interpolated_aperiodicity[a2 + ai];

        memset(response, 0, sizeof(*response)*prow);

        get_one_periodic_excitation(number_of_aperiodicities, pulse_seed, prow, pcol, aperiodicity, noise_size, response);
        // periodic_component[output_buffer_index.astype(int)-1] += response
        // 如果存在同索引.  只加最后一次结果
        int tmpi = 0
          , tmpi2;
        for (j=fft_size-1; j>-1; --j)
        {
            tmpi2 = output_buffer_index[j]-1;
            if (tmpi == tmpi2)
            { continue; }
            else {
                tmpi = tmpi2;
                periodic_component[tmpi] += response[j];
            }
            // if (output_buffer_index[j]-1 == y_len-1)
            // { printf("%lf ", periodic_component[output_buffer_index[j]-1]); }
        }
    }
    for (i=0; i<y_len; i++)
    {
        excitation_signal[i] = periodic_component[i] + aperiodic_component[i];
        // if (y_len-i<10)
        // { printf("%f %lf %lf \n", excitation_signal[i], periodic_component[i], aperiodic_component[i]); }
    }

    // print_float2(excitation_signal, 10);
    // print_float2(excitation_signal+y_len-10, 10);
    free(base_index);
    free(output_buffer_index);
    free(vuv);
    free(pulse_locations_index);
    free(cur_index);
    free(noise);
    free(time_axis);
    free(periodic_component);
    free(aperiodic_component);
    free(interpolated_vuv);
    free(interpolated_aperiodicity);
    free(band_aperiodicity);
    free(aperiodicity);
    free(response);
}


static void get_waveform_float(int fft_size, int fs, float *win, int win_len, wtk_rfft_t *rf, float *excitation_signal, float *y, int y_len, float *sp, int sprow, int frame_period_sample, int f0_len)
{/* frame_period_sample 针周期样本 */
    int *latter_index = malloc(sizeof(int)*(fft_size/2))
      , half_win_len = frame_period_sample -1
      , origin
      , *safe_index =  malloc(sizeof(int)*win_len)
      , i, j, k;

    float
        *tmp = malloc(sizeof(float)*win_len),
        *spec = malloc(sizeof(float)*sprow),
        *tmp_cepstrum,
        *tmp_complex_cepstrum,
        *response;
    float *fft_cache =  malloc(sizeof(float)*fft_size*2);
    wtk_world_fplural_t 
        *periodic_spectrum = wtk_world_fplural_new(fft_size),
        *spectrum = wtk_world_fplural_new(fft_size),
        *tmp_fft = wtk_world_fplural_new(fft_size);
    memset(y, 0, sizeof(*y)*y_len);

    for (i=fft_size/2+1, j=0; i<=fft_size; ++i, ++j)
    {
        latter_index[j] = i;
    }
    // printf("frame_period_sample %d fs: %d fft_size: %d\n", frame_period_sample, fs, fft_size);
    
    for (i=2; i<f0_len-1; ++i)
    {
        origin = (i-1)*frame_period_sample - half_win_len;
        for (j=0; j<win_len; j++)
        {
            safe_index[j] = min(y_len, origin+j);
            tmp[j] = excitation_signal[safe_index[j]-1] * win[j];
        }
        wtk_world_fplural_zero(periodic_spectrum);
        for (j=0; j<sprow; j++)
        {
            spec[j] = sp[j*f0_len+i-1];
            periodic_spectrum->real[j] = log(fabs(spec[j]))/2;
        }
        // periodic_spectrum = np.r_[spec, spec[-2:0:-1]]
        
        for (k = sprow - 2, j = 0; k--; j++)
        {
            // double t = fabs(spec[k+1]);
            periodic_spectrum->real[sprow + j] = log(fabs(spec[k+1]))/2;
        }
        sptk_rfft_float( rf, fft_cache, periodic_spectrum->real, periodic_spectrum->imag);

        tmp_cepstrum = periodic_spectrum->real;
        wtk_world_fplural_zero(spectrum);
        tmp_complex_cepstrum = spectrum->real;
        for (j=0; j<fft_size/2; j++)
        {
            int cur = latter_index[j]-1;
            tmp_complex_cepstrum[ cur] = tmp_cepstrum[cur]*2;
            // if (i==200)printf("%lf ", tmp_complex_cepstrum[ cur]);
        }
        tmp_complex_cepstrum[0] = tmp_cepstrum[0];
        sptk_ifft_float( spectrum->real, spectrum->imag, spectrum->size);
        for (j=0; j<fft_size; j++)
        {
            double x = exp(spectrum->real[j]);
            spectrum->real[j] = x*cos(spectrum->imag[j]);
            spectrum->imag[j] = x*sin(spectrum->imag[j]);
        }
        
        wtk_world_fplural_zero(tmp_fft);
        memcpy(tmp_fft->real, tmp, sizeof(*tmp)*win_len);
        sptk_rfft_float( rf, fft_cache, tmp_fft->real, tmp_fft->imag);
        wtk_world_fplural_mul(tmp_fft, spectrum, tmp_fft);
        // sptk_ifft_float( tmp_fft->real, tmp_fft->imag, tmp_fft->size);
        sptk_rfft_ifft_float( rf, fft_cache, tmp_fft->real, tmp_fft->imag);
        // wtk_world_fplural_print(tmp_fft, 1);
        // exit(1);
        response = tmp_fft->real;
        // y[safe_index-1] += response
        // 如果index 相同. 只取最后一次结果
        int tmpj = 0;
        for (j=fft_size-1; j>-1; --j)
        {
            int si = min(y_len, origin+j);
            if (tmpj == si) {
                continue;
            } else {
                tmpj = si;
                y[si-1] += response[j];
            }
        }
    }
    // y[y_len-1] = 0.0;
    // print_float2(y+y_len-10, 10);
    free(latter_index);
    free(safe_index);
    free(tmp);
    free(spec);
    free(fft_cache);
    wtk_world_fplural_delete(periodic_spectrum);
    wtk_world_fplural_delete(spectrum);
    wtk_world_fplural_delete(tmp_fft);
}

void wtk_world_synthesis_requiem(wtk_rfft_t *rf, float *win, int fft_size, float *f0, int f0_len, int fs, float *y, int y_len, float *ap, int ap_len, float *sp, double frame_period)
{
    wtk_world_pulse_noise_t *seeds_signals = get_seeds_signals(fs, fft_size/2, 0);
    int i
      , sprow = (fft_size/2+1)
      , frame_period_sample = (int)(fs*0.005)/* 针周期样本 */
      , win_len = frame_period_sample * 2 -1;
    double
        tem=0,
        *temporal_positions = malloc(sizeof(double)*f0_len);
    float
        *excitation_signal = malloc(sizeof(float)*y_len);

    for (i=0; i<f0_len; i++)
    {
        temporal_positions[i] = tem;
        tem += 0.005;
    }
    
    get_excitation_signal(temporal_positions, f0, f0_len, fs, y_len, ap, ap_len, seeds_signals, frame_period, excitation_signal);

    get_waveform_float(fft_size, fs, win, win_len, rf, excitation_signal, y, y_len, sp, sprow, frame_period_sample, f0_len);

    free(temporal_positions);
    free(excitation_signal);
    wtk_world_pulse_noise_delete(seeds_signals);
}
