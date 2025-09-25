#include "qtk_gain_controller.h"
// void complex_dumpx(wtk_complex_t *c, int len){
//     int i;
//     for(i = 0; i < len; i++){
//         printf("%.10f %.10fj\n,", c[i].a, c[i].b);
//     }
// }
// static float db2gain(float in){
//     return powf(10.0,in/10);
// }

static float pwr2db(float in){
    return 10 * log10f(in);
}

static float db2val(float in){
    return powf(10.0,in/20);
}

void qtk_ahs_limiter_init(qtk_ahs_limiter_t *limiter,float ta, float tr, float rho, float threshold, int sample_rate){
    limiter->Aa = expf(-2.3/sample_rate/ta);
    limiter->Ar = expf(-2.3/sample_rate/tr);
    limiter->p = rho;
    limiter->c0 = threshold;
    limiter->g1 = 0.0;
    limiter->c = 0.0;
    limiter->G = 0.0;
}

void qtk_ahs_limiter_process(qtk_ahs_limiter_t *limiter, float *data, int len){
    int i;
    float *p = data;
    float tmp,g0;

    for(i = 0; i < len; i++){
        tmp = abs(*p);
        if (tmp > limiter->c){
            limiter->c = limiter->Aa * limiter->c + (1.0 - limiter->Aa) * tmp;
        }else{
            limiter->c = limiter->Ar * limiter->c + (1.0 - limiter->Ar) * tmp;
        }

        g0 = limiter->g1;

        if(limiter->c == 0.0){
            limiter->g1 = 1;
        }else if(limiter->c >= limiter->c0){
            limiter->g1 = powf(limiter->c/limiter->c0, limiter->g1);
        }else{
            limiter->g1 = 1;
        }

        if(limiter->g1 >= g0){
            limiter->G = limiter->Aa * limiter->G + (1 - limiter->Aa) * limiter->g1;
        }else{
            limiter->G = limiter->Ar * limiter->G + (1 - limiter->Ar) * limiter->g1;
        }

        *p *= limiter->G;
        p++;
    }
}

void qtk_gain_controller_reset(qtk_ahs_gain_controller_t *gc){
    if(gc->voice_buf){
        memset(gc->voice_buf, 0, sizeof(float)*gc->cfg->voice_frame);
    }
    gc->voice_idx = 0;
    gc->voice_eng = 0.0;
    gc->voice_cnt = 0;
}

static void _km_init(qtk_ahs_gain_controller_t *gc){
    qtk_ahs_gain_controller_cfg_t *cfg = gc->cfg;
    qtk_gc_kalman_t *km = &gc->kalman;

    km->alpha = cfg->alpha;
    km->update_thresh = cfg->update_thresh;
    km->R_k = cfg->pvorg;
    km->P_k = cfg->pworg;
    km->X_k = 1.0;
    km->Z_k = gc->G_cali;
    km->Q_k = 10.0;
    km->H_K = 0.0;
    km->beta = cfg->beta;
}

qtk_ahs_gain_controller_t *qtk_gain_controller_new(qtk_ahs_gain_controller_cfg_t* cfg){
    qtk_ahs_gain_controller_t *gc = (qtk_ahs_gain_controller_t*)wtk_malloc(sizeof(qtk_ahs_gain_controller_t));
    int N;

    gc->cfg = cfg;
    gc->fft_length = (int)cfg->hop_size * 2;
    gc->ft_sample_rate = cfg->sample_rate/cfg->hop_size;

    //TOD truncation limitor
    gc->prev_half_win = wtk_calloc(sizeof(float),cfg->hop_size);

    gc->drft = wtk_drft_new2(gc->fft_length);
    gc->frm = wtk_calloc(sizeof(float),gc->fft_length);

    gc->test_mode = 0;
    gc->G_cali = 0.007156706032120891;
    gc->init_Intensity = 0.004327824849573125;
    gc->galis = wtk_strbuf_new(1024,1);
    gc->frm_idx = 0;
    gc->Maximun_Atten = cfg->Maximun_Atten;
    _km_init(gc);

    N = 300;
    gc->auto_calibration.static_size = cfg->static_size;
    gc->auto_calibration.nfrm_delay = ceilf(cfg->vad_delay/(cfg->hop_size/16));
    gc->auto_calibration.buffer4delay = wtk_calloc(sizeof(float),gc->auto_calibration.nfrm_delay);
    gc->auto_calibration.buffer4static = wtk_calloc(sizeof(float),gc->auto_calibration.static_size);
    gc->auto_calibration.N = N;
    gc->auto_calibration.milestones = wtk_malloc(sizeof(float) * N);
    gc->auto_calibration.cdf = wtk_malloc(sizeof(float) * (N - 1));
    gc->auto_calibration.pdf = wtk_malloc(sizeof(float) * (N - 1));
    gc->auto_calibration.bin_cnt = wtk_malloc(sizeof(int)* (N - 1));
    gc->auto_calibration.tmp = wtk_malloc(sizeof(int)* (N - 1));
    gc->auto_calibration.tmp2 = wtk_malloc(sizeof(int)* (N - 1));
    gc->auto_calibration.tmp3 = wtk_malloc(sizeof(int)* (N - 1));
    gc->auto_calibration.bins = wtk_calloc(sizeof(int),gc->auto_calibration.static_size);
    gc->auto_calibration.frm_counter = 0;

    gc->voice_buf = NULL;
    if(cfg->use_voice_prob){
        gc->voice_buf = wtk_malloc(sizeof(float)*cfg->voice_frame);
    }
    qtk_gain_controller_reset(gc);
    qtk_ahs_limiter_init(&gc->limiter, 0.002, 0.01, 0.01, 0.99, gc->ft_sample_rate);
    return gc;
}


void qtk_gain_controller_delete(qtk_ahs_gain_controller_t *gc){
    wtk_strbuf_delete(gc->galis);
    wtk_free(gc->frm);
    wtk_free(gc->prev_half_win);
    wtk_drft_delete2(gc->drft);
    wtk_free(gc->auto_calibration.milestones);
    wtk_free(gc->auto_calibration.cdf);
    wtk_free(gc->auto_calibration.pdf);
    wtk_free(gc->auto_calibration.bin_cnt);
    wtk_free(gc->auto_calibration.tmp);
    wtk_free(gc->auto_calibration.tmp2);
    wtk_free(gc->auto_calibration.tmp3);
    wtk_free(gc->auto_calibration.buffer4static);
    wtk_free(gc->auto_calibration.buffer4delay);
    wtk_free(gc->auto_calibration.bins);
    if(gc->voice_buf){
        wtk_free(gc->voice_buf);
    }
    wtk_free(gc);
}

static float _qtk_gain_controller_km(qtk_gc_kalman_t *km, float in, float p_frame){
    if(in > km->update_thresh){
        km->H_K = (1 - km->beta) * km->H_K + km->beta * in;
        float error = km->Z_k - km->H_K * km->X_k;
        km->R_k = km->alpha * km->R_k + (1 - km->alpha) * p_frame * p_frame * fabs(error) * fabs(error);
        float r1 = km->P_k * km->H_K;
        float r2 = 1.0 / (km->H_K * r1 + km->R_k);
        float K = r1 * r2;
        km->X_k += K * error;

        km->Q_k = km->alpha * km->Q_k + (1 - km->alpha) * p_frame * km->Z_k;
        km->P_k += -K * km->H_K * km->P_k + km->Q_k;
    }
    return km->X_k;
}

static float _calc_energy(wtk_complex_t *data, int len, int nfft){
    float sum = 0.0;
    int i;
    //wtk_debug("%d %d\n",len,nfft);
    sum += data[0].a * data[0].a + data[0].b * data[0].b;
    for(i = 1; i < len; i++){
        sum += (data[i].a * data[i].a + data[i].b * data[i].b)*2;
    }
    // wtk_debug("%f\n",sum);
    return sum/nfft;
}


static void swap(float* a, float* b) {
    float temp = *a;
    *a = *b;
    *b = temp;
}

static int partition(float* vec, int low, int high) {
    float pivot = vec[high];
    int i = low - 1;
    for (int j = low; j < high; j++) {
        if (vec[j] < pivot) {
            i++;
            swap(&vec[i], &vec[j]);
        }
    }
    swap(&vec[i + 1], &vec[high]);
    return i + 1;
}

static void quicksort(float* vec, int low, int high) {
    if (low < high) {
        int pivot = partition(vec, low, high);
        quicksort(vec, low, pivot - 1);
        quicksort(vec, pivot + 1, high);
    }
}

static void sort_float_vector(float* vec, int size) {
    quicksort(vec, 0, size - 1);
}

void _draw_histgram(qtk_ahs_gain_controller_t *gc, float *p, int cnt){
    int i,j,N = 300;
    qtk_gc_auto_calibration_t *ac = &gc->auto_calibration;

    sort_float_vector(p,cnt);
    int l_idx = 0.02 * cnt;
    int r_idx = 0.98 * cnt;
    float interval = (p[r_idx] - p[l_idx])/(N - 1);

    for(i = 0; i < N; i++){
        ac->milestones[i] = p[l_idx] + interval * i;
    }
    //print_float(milestones,N);
    float *lower_bound = ac->milestones;
    float *upper_bound = ac->milestones + 1;
    int* bins;

    if(cnt == ac->static_size){
        bins = ac->bins;
    }else{
        bins = wtk_malloc(sizeof(int) * cnt);
    }

    memset(bins, 0, sizeof(int) * cnt);
    for(i = 0; i < cnt; i++){
        for(j = 0; j < N - 1; j++){
            if(p[i] >= lower_bound[j] && p[i] < upper_bound[j]){
                bins[i] = j;
            }
        }
    }
    //print_int(bins,cnt);
    memset(ac->bin_cnt, 0, sizeof(int) * (N - 1));
    for(i = 0; i < cnt; i++){
        ac->bin_cnt[bins[i]]++;
    }
    //print_int(bin_cnt,N - 1);
    ac->cdf[0] = ac->bin_cnt[0];
    for(i = 1; i < N - 1; i++){
        ac->cdf[i] = ac->bin_cnt[i] + ac->cdf[i - 1];
    }
    for(i = 0; i < ac->N - 1; i++){
        ac->cdf[i] *= 1.0/cnt;
        ac->pdf[i] = ac->bin_cnt[i] * 1.0/cnt;
    }

    if(cnt != ac->static_size){
        wtk_free(bins);
    }
}

static void _update_gali(qtk_ahs_gain_controller_t *gc,float energy, float vad_prob){
    qtk_gc_auto_calibration_t *ac = &gc->auto_calibration;
    float E_ = ac->buffer4delay[0];
    int idx = 0,i,j;
    int cnt = ac->N - 1;
    memmove(ac->buffer4delay,ac->buffer4delay + 1,sizeof(float)*(ac->nfrm_delay - 1));
    ac->buffer4delay[ac->nfrm_delay-1] = energy;

    if(vad_prob > gc->cfg->pvad_threshold && E_ > gc->cfg->low_energy_threshold){
        idx = ac->frm_counter % ac->static_size;
        ac->buffer4static[idx] = E_;
        ac->frm_counter++;
    }
    if(ac->frm_counter >= ac->static_size && ac->frm_counter % ac->static_size == 0){
        _draw_histgram(gc,ac->buffer4static,ac->static_size);
        ac->tmp[0] = ac->pdf[0] * ac->milestones[0];
        ac->tmp2[0] = ac->pdf[cnt - 2];
        ac->tmp3[0] = ac->tmp[0] + ac->tmp2[0] * ac->milestones[0];
        memset(ac->tmp3, 0, sizeof(float) * (ac->N - 1));
        for(i = 1,j = cnt - 2; i < ac->N - 1; i++,j--){
            ac->tmp[i] = ac->pdf[i] * ac->milestones[i] + ac->tmp[i - 1];
            ac->tmp2[i] = ac->pdf[j] + ac->tmp2[i - 1];
        }

        for(i = 0,j = cnt - 1; i < ac->N - 1; i++,j--){
            ac->tmp3[i] = ac->tmp[i] + ac->tmp2[j] * ac->milestones[i];
        }
        int index = -1;
        float init_intensity = 0.1 * gc->init_Intensity;
        float cali = 0.5 * gc->G_cali;
        int st = -1,ed = -1;
        for(i = 0; i < ac->N - 1; i++){
            if (fabs(ac->tmp3[i] - gc->init_Intensity) < init_intensity){
                if(fabs(ac->milestones[i] - gc->G_cali) < cali){
                    if(st < 0){
                        st = i;
                    }
                    ed = i;
                }
            }
        }

        if(st < 0){
            if(gc->init_Intensity < 0.5 * gc->G_cali){
                gc->G_cali = 0.5 * gc->G_cali;
            }else if(gc->init_Intensity > 1.5 * gc->G_cali){
                gc->G_cali = 1.5 * gc->G_cali;
            }else{
                gc->G_cali = gc->init_Intensity;
            }
        }else{
            index = (st + ed)/2;
            gc->G_cali = ac->milestones[index];
        }
        gc->kalman.Z_k = gc->G_cali;
    }
}

void qtk_gain_controller_voice_prob(qtk_ahs_gain_controller_t *gc, float energy, float vad_prob)
{
    int voice_init_cnt = gc->cfg->voice_init_cnt;
    int voice_frame = gc->cfg->voice_frame;
    float voice_precent = gc->cfg->voice_precent;
    float voice_alpha = gc->cfg->voice_alpha;
    float voice_alpha2 = gc->cfg->voice_alpha2;
    float voice_prob = gc->cfg->voice_prob;
    if(vad_prob > voice_prob){
        if(gc->voice_idx < voice_frame){
            gc->voice_buf[gc->voice_idx++] = energy;
        }else{
            quicksort(gc->voice_buf, 0, voice_frame - 1);
            int idx = (int)(voice_precent * voice_frame);
            if(gc->voice_eng == 0.0){
                gc->voice_eng = gc->voice_buf[idx];
            }else if(gc->voice_cnt < voice_init_cnt){
                gc->voice_eng = gc->voice_eng * voice_alpha + gc->voice_buf[idx] * (1 - voice_alpha);
                ++gc->voice_cnt;
            }else{
                gc->voice_eng = gc->voice_eng * voice_alpha2 + gc->voice_buf[idx] * (1 - voice_alpha2);
            }
            gc->voice_idx = 0;
        }
    }
}

void qtk_gain_controller_run_agc(qtk_ahs_gain_controller_t *gc, wtk_complex_t *data, int len, float *out_wav,float vad_prob){
    int i;//(vad_prob > gc->cfg->pframe_thresh) ? 1 : 0;
    int hop_size = gc->cfg->hop_size;
    float energy;
    float gain;

    energy = _calc_energy(data,len,gc->fft_length);
    //loudness = _caculate_loudness(gc->loudness_weight,data,len,gc->fft_length);
    if(gc->frm_idx < gc->cfg->vad_init_frame){
        gc->frm_idx++;
        if(out_wav){
            wtk_drft_ifft22(gc->drft, data, gc->frm);
            for (i = 0;i < gc->fft_length;++i){
                gc->frm[i] /= gc->fft_length;
            }
            for(i = 0; i < hop_size; i++){
                out_wav[i] = gc->frm[i] + gc->prev_half_win[i];
            }
            //print_float(out_wav,hop_size);
            memcpy(gc->prev_half_win, gc->frm + hop_size, hop_size*sizeof(float));
        }
        return;
    }
    if(gc->cfg->use_update_gali){
        _update_gali(gc,energy,vad_prob);
    }
    // printf("%f\n", vad_prob);
    if(gc->cfg->use_voice_prob){
        qtk_gain_controller_voice_prob(gc, energy, vad_prob);
        if(gc->voice_eng/(energy+1e-9) > gc->cfg->voice_thresh && gc->voice_cnt >= gc->cfg->voice_init_cnt){
            gain = pwr2db(gc->kalman.X_k);
        }else{
            gain = pwr2db(_qtk_gain_controller_km(&gc->kalman, energy, vad_prob));
        }
    }else{

        gain = pwr2db(_qtk_gain_controller_km(&gc->kalman, energy, vad_prob));
    }
    if(gain > gc->cfg->Maximun_Gain){
        gain = gc->cfg->Maximun_Gain;
    }
    if(gain < gc->Maximun_Atten){
        gain = gc->Maximun_Atten;
    }
    gain = db2val(gain);
    // printf("%f\n", gain);
    //wtk_debug("%f\n",gain);
    for(i = 0; i < len; i++){
        data[i].a = data[i].a * gain;
        data[i].b = data[i].b * gain;
    }

    if(out_wav){
        wtk_drft_ifft22(gc->drft, data, gc->frm);
        for (i = 0;i < gc->fft_length;++i){
            gc->frm[i] /= gc->fft_length;
        }
        for(i = 0; i < hop_size; i++){
            out_wav[i] = gc->frm[i] + gc->prev_half_win[i];
        }
        qtk_ahs_limiter_process(&gc->limiter, out_wav, hop_size);
        memcpy(gc->prev_half_win, gc->frm + hop_size, hop_size*sizeof(float));
    }

    gc->frm_idx++;
}

/*
Write the energy of each frame into a file, 
so that it can be directly read from the file next time without the need for recalibration.
*/
void qtk_gain_controller_galis_update(qtk_ahs_gain_controller_t *gc, wtk_complex_t *data, int len,float vad_prob){
    float energy = _calc_energy(data,len,gc->fft_length);
    //wtk_debug("%f\n",energy);
    if(energy > gc->cfg->low_energy_threshold && energy < gc->cfg->high_energy_threshold){
        FILE *fp = fopen("galis","a");//Write to the file in append mode
        fprintf(fp,"%.10f\n",energy);
        fclose(fp);
        wtk_strbuf_push_float(gc->galis,&energy,1);
    }
}


void qtk_gain_controller_run(qtk_ahs_gain_controller_t *gc, wtk_complex_t *data, int len, float *out_wav, float vad_prob){
    if(gc->test_mode){
        qtk_gain_controller_galis_update(gc,data,len,vad_prob);
    }else{
        qtk_gain_controller_run_agc(gc,data,len,out_wav,vad_prob);
    }
}

void qtk_gain_controller_set_mode(qtk_ahs_gain_controller_t *gc, int mode){
    qtk_gc_auto_calibration_t *ac = &gc->auto_calibration;
    gc->test_mode = mode;
    if(mode == 1){
        wtk_strbuf_reset(gc->galis);
    }else{
        if(gc->galis->pos > 0){
            float *p = (float*)gc->galis->data;
            int i,cnt = gc->galis->pos/sizeof(float);
            _draw_histgram(gc,p,cnt);

            int index = -1;
            for(i = 0; i < ac->N - 1; i++){
                if(ac->cdf[i] > 0.9){
                    index = i;
                    break;
                }
            }

            gc->G_cali = ac->milestones[index];
            float sum = 0.0;
            for(i = 0;i < ac->N - 1; i++){
                sum += ac->pdf[i] * ac->milestones[i];
            }
            gc->init_Intensity = sum;//TODO
            //wtk_debug("%f %f\n",gc->G_cali,gc->init_Intensity);
            _km_init(gc);
        }
    }
}

void qtk_gain_controller_update_calibration(qtk_ahs_gain_controller_t *gc, float gain){
    float ratio = gain / gc->gain_cali;
    ratio=ratio*ratio;
    gc->G_cali *= ratio;
    gc->init_Intensity *= ratio;
    gc->kalman.Z_k = gc->G_cali;
}
