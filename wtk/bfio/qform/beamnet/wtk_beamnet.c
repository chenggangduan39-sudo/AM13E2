#include "wtk/bfio/qform/beamnet/wtk_beamnet.h"
#include "qtk/math/qtk_vector.h"
#ifndef WTK_WAV_SHORT_TO_FLOAT
#define WTK_WAV_SHORT_TO_FLOAT(f) ((f) > 0? (f/32767.0) : (f/32768.0))
#endif
#ifndef WTK_WAV_FLOAT_TO_SHORT
#define WTK_WAV_FLOAT_TO_SHORT(f) ((f) > 0? floorf(f*32767.0+0.5) : floorf(f*32768.0+0.5))
#endif

void wtk_beamnet_compute_channel_covariance(wtk_beamnet_t *beamnet, wtk_complex_t **fft, wtk_complex_t **covar_mat, int norm)
{
    int nmicchannel = beamnet->cfg->nmicchannel;
    int nbin = beamnet->nbin;
    int i, j, k;
    wtk_complex_t *mean_mat = beamnet->mean_mat;
    wtk_complex_t **input_norm = beamnet->input_norm;
    int idx;

    if(norm==1){
        memset(mean_mat, 0, nbin * sizeof(wtk_complex_t));
        for(k=0;k<nbin;++k){
            for(i=0;i<nmicchannel;++i){
                mean_mat[k].a += fft[i][k].a;
                mean_mat[k].b += fft[i][k].b;
            }
            mean_mat[k].a *= 1.0/nmicchannel;
            mean_mat[k].b *= 1.0/nmicchannel;
            for(i=0;i<nmicchannel;++i){
                input_norm[i][k].a = fft[i][k].a - mean_mat[k].a;
                input_norm[i][k].b = fft[i][k].b - mean_mat[k].b;
            }
        }
    }else{
        for(i=0;i<nmicchannel;++i){
            memcpy(input_norm[i], fft[i], nbin * sizeof(wtk_complex_t));
        }
    }

    for(k=0;k<nbin;++k){
        idx=0;
        for(i=1;i<nmicchannel;++i){
            for(j=0;j<i;++j){
                covar_mat[idx][k].a = input_norm[i][k].a * input_norm[j][k].a + input_norm[i][k].b * input_norm[j][k].b;
                covar_mat[idx][k].b = input_norm[i][k].b * input_norm[j][k].a - input_norm[i][k].a * input_norm[j][k].b;
                idx++;
            }
        }
    }
}

void wtk_beamnet_compute_multi_channel_phase_shift(wtk_beamnet_t *beamnet, wtk_complex_t **phase_shift, float theta, float phi)
{
    int wins = beamnet->cfg->wins;
    int nmicchannel = beamnet->cfg->nmicchannel;
    int nbin = beamnet->nbin;
    int rate = beamnet->cfg->rate;
    float sv = beamnet->cfg->sv;
    float **mic_pos = beamnet->cfg->mic_pos;
    float *time_delay = beamnet->time_delay;
    float *mic;
    float x, y, z;
    int i, k;
    float t;

    theta -= 180.0;
    theta = theta < -180.0 ? theta + 360.0 : theta;
    theta = theta > 180.0  ? theta - 360.0 : theta;

    theta = theta * PI / 180.0;
    phi = phi * PI / 180.0;
    x = cos(phi) * cos(theta);
    y = cos(phi) * sin(theta);
    z = sin(phi);
    x = fabs(x) < 1e-7 ? 0 : x;
    y = fabs(y) < 1e-7 ? 0 : y;
    z = fabs(z) < 1e-7 ? 0 : z;

    for(i=0;i<nmicchannel;++i){
        mic = mic_pos[i];
        time_delay[i] = (mic[0] * x + mic[1] * y + mic[2] * z)/sv;
    }
	for(k=0;k<nbin;++k)
	{
		t=2*PI*rate*1.0/wins*k;
		for(i=0;i<nmicchannel;++i)
		{
			phase_shift[i][k].a=cos(t*time_delay[i]);
			phase_shift[i][k].b=-sin(t*time_delay[i]);
		}
	}
}

wtk_beamnet_t *wtk_beamnet_new(wtk_beamnet_cfg_t *cfg) {
    wtk_beamnet_t *beamnet;

    beamnet = (wtk_beamnet_t *)wtk_malloc(sizeof(wtk_beamnet_t));
    beamnet->cfg = cfg;
    beamnet->ths = NULL;
    beamnet->notify = NULL;
    beamnet->mic = wtk_strbufs_new(beamnet->cfg->nmicchannel);
    beamnet->sp = wtk_strbufs_new(beamnet->cfg->nspchannel);

    beamnet->nbin = cfg->wins / 2 + 1;
    beamnet->analysis_window = wtk_malloc(sizeof(float) * cfg->wins);  /// 2);
    beamnet->synthesis_window = wtk_malloc(sizeof(float) * cfg->wins); /// 2);
    beamnet->analysis_mem = wtk_float_new_p2(cfg->nmicchannel, beamnet->nbin - 1);
    beamnet->analysis_mem_sp = wtk_float_new_p2(cfg->nspchannel, beamnet->nbin - 1);
    beamnet->synthesis_mem = wtk_malloc(sizeof(float) * (beamnet->nbin - 1));
    beamnet->rfft = wtk_drft_new(cfg->wins);
    beamnet->rfft_in = (float *)wtk_malloc(sizeof(float) * (cfg->wins));

    beamnet->fft = wtk_complex_new_p2(cfg->nmicchannel, beamnet->nbin);
    beamnet->fft_sp = wtk_complex_new_p2(cfg->nspchannel, beamnet->nbin);
    beamnet->fftx = (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * beamnet->nbin);

    beamnet->out = wtk_malloc(sizeof(float) * (beamnet->nbin - 1));

#ifdef ONNX_DEC
    beamnet->seperator = NULL;
    beamnet->beamformer = NULL;
    beamnet->sep_caches = NULL;
    beamnet->beam_caches = NULL;
    beamnet->seperator_out_len = NULL;
    beamnet->beamformer_out_len = NULL;
    if(cfg->use_onnx){
        beamnet->seperator = qtk_onnxruntime_new(&(cfg->seperator));
        beamnet->sep_caches = wtk_calloc(sizeof(OrtValue *), beamnet->seperator->num_in - cfg->seperator.outer_in_num);
        if (beamnet->seperator->num_in - cfg->seperator.outer_in_num != beamnet->seperator->num_out - cfg->seperator.outer_out_num) {
            wtk_debug("err inner_item\n");
            exit(0);
        }
        beamnet->seperator_out_len = (int *)wtk_malloc(sizeof(int) * (cfg->seperator.outer_out_num));
        beamnet->beamformer = qtk_onnxruntime_new(&(cfg->beamformer));
        beamnet->beam_caches = wtk_calloc(sizeof(OrtValue *), beamnet->beamformer->num_in - cfg->beamformer.outer_in_num);
        if (beamnet->beamformer->num_in - cfg->beamformer.outer_in_num != beamnet->beamformer->num_out - cfg->beamformer.outer_out_num) {
            wtk_debug("err inner_item\n");
            exit(0);
        }
        beamnet->beamformer_out_len = (int *)wtk_malloc(sizeof(int) * (cfg->seperator.outer_out_num));
    }
    beamnet->speech_est_real=NULL;
    beamnet->speech_est_imag=NULL;
    beamnet->noise_est_real=NULL;
    beamnet->noise_est_imag=NULL;
    beamnet->speech_mask=NULL;
    beamnet->noise_mask=NULL;
    beamnet->src_est_real=NULL;
    beamnet->src_est_imag=NULL;
#endif
    beamnet->feature = wtk_float_new_p2(cfg->feature_len, beamnet->nbin);
    beamnet->x = (float *)wtk_malloc(sizeof(float) * cfg->feature_len * beamnet->nbin);
    beamnet->mix_spec = (float *)wtk_malloc(sizeof(float) * (cfg->nmicchannel) * 2 * beamnet->nbin);
    beamnet->time_delay = (float *)wtk_malloc(sizeof(float) * beamnet->nbin);
    beamnet->mic_aptd = (float *)wtk_malloc(sizeof(float) * beamnet->nbin);
    beamnet->echo_aptd = (float *)wtk_malloc(sizeof(float) * beamnet->nbin);
    beamnet->mic_covar = wtk_complex_new_p2(cfg->out_channels, beamnet->nbin);
    beamnet->ideal_phase_covar = wtk_complex_new_p2(cfg->out_channels, beamnet->nbin);
    beamnet->ideal_phase_shift = wtk_complex_new_p2(cfg->nmicchannel, beamnet->nbin);
    beamnet->freq_covar = wtk_complex_new_p2(cfg->out_channels, beamnet->nbin);
    beamnet->freq_covar_sum = (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * beamnet->nbin);
    beamnet->LPS = (float *)wtk_malloc(sizeof(float) * beamnet->nbin);
    beamnet->echo_LPS = (float *)wtk_malloc(sizeof(float) * beamnet->nbin);
    beamnet->IPDs = wtk_float_new_p2(cfg->out_channels, beamnet->nbin);
    beamnet->FDDF = (float *)wtk_malloc(sizeof(float) * beamnet->nbin);
    beamnet->mean_mat = (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * beamnet->nbin);
    beamnet->input_norm = wtk_complex_new_p2(cfg->nmicchannel, beamnet->nbin);

    wtk_beamnet_reset(beamnet);

    return beamnet;
}
void wtk_beamnet_delete(wtk_beamnet_t *beamnet) {
    wtk_strbufs_delete(beamnet->mic, beamnet->cfg->nmicchannel);
    wtk_strbufs_delete(beamnet->sp, beamnet->cfg->nspchannel);

    wtk_free(beamnet->analysis_window);
    wtk_free(beamnet->synthesis_window);
    wtk_float_delete_p2(beamnet->analysis_mem, beamnet->cfg->nmicchannel);
    wtk_float_delete_p2(beamnet->analysis_mem_sp, beamnet->cfg->nspchannel);
    wtk_free(beamnet->synthesis_mem);
    wtk_free(beamnet->rfft_in);
    wtk_drft_delete(beamnet->rfft);
    wtk_complex_delete_p2(beamnet->fft, beamnet->cfg->nmicchannel);
    wtk_complex_delete_p2(beamnet->fft_sp, beamnet->cfg->nspchannel);

    wtk_free(beamnet->fftx);

    wtk_free(beamnet->out);
#ifdef ONNX_DEC
    if(beamnet->cfg->use_onnx){
        {
            int n = beamnet->seperator->num_in - beamnet->seperator->cfg->outer_in_num;
            if (beamnet->sep_caches[0]) {
                int i;
                for (i = 0; i < n; i++) {
                    beamnet->seperator->api->ReleaseValue(beamnet->sep_caches[i]);
                }
            }
        }
        if (beamnet->seperator) {
            qtk_onnxruntime_delete(beamnet->seperator);
        }
        wtk_free(beamnet->seperator_out_len);
        wtk_free(beamnet->sep_caches);
        {
            int n = beamnet->beamformer->num_in - beamnet->beamformer->cfg->outer_in_num;
            if (beamnet->beam_caches[0]) {
                int i;
                for (i = 0; i < n; i++) {
                    beamnet->beamformer->api->ReleaseValue(beamnet->beam_caches[i]);
                }
            }
        }
        if (beamnet->beamformer) {
            qtk_onnxruntime_delete(beamnet->beamformer);
        }
        wtk_free(beamnet->beamformer_out_len);
        wtk_free(beamnet->beam_caches);
    }
#endif
    wtk_float_delete_p2(beamnet->feature, beamnet->cfg->feature_len);
    wtk_free(beamnet->x);
    wtk_free(beamnet->mix_spec);
    wtk_free(beamnet->time_delay);
    wtk_free(beamnet->mic_aptd);
    wtk_free(beamnet->echo_aptd);
    wtk_complex_delete_p2(beamnet->mic_covar, beamnet->cfg->out_channels);
    wtk_complex_delete_p2(beamnet->ideal_phase_covar, beamnet->cfg->out_channels);
    wtk_complex_delete_p2(beamnet->ideal_phase_shift, beamnet->cfg->nmicchannel);
    wtk_complex_delete_p2(beamnet->freq_covar, beamnet->cfg->out_channels);
    wtk_free(beamnet->freq_covar_sum);
    wtk_free(beamnet->LPS);
    wtk_free(beamnet->echo_LPS);
    wtk_float_delete_p2(beamnet->IPDs, beamnet->cfg->out_channels);
    wtk_free(beamnet->FDDF);
    wtk_free(beamnet->mean_mat);
    wtk_complex_delete_p2(beamnet->input_norm, beamnet->cfg->nmicchannel);

    if(beamnet->speech_est_real){
        wtk_free(beamnet->speech_est_real);
    }
    if(beamnet->speech_est_imag){
        wtk_free(beamnet->speech_est_imag);
    }
    if(beamnet->noise_est_real){
        wtk_free(beamnet->noise_est_real);
    }
    if(beamnet->noise_est_imag){
        wtk_free(beamnet->noise_est_imag);
    }
    if(beamnet->speech_mask){
        wtk_free(beamnet->speech_mask);
    }
    if(beamnet->noise_mask){
        wtk_free(beamnet->noise_mask);
    }
    if(beamnet->src_est_real){
        wtk_free(beamnet->src_est_real);
    }
    if(beamnet->src_est_imag){
        wtk_free(beamnet->src_est_imag);
    }

    wtk_free(beamnet);
}

void wtk_beamnet_start(wtk_beamnet_t *beamnet, float theta, float phi) {
    wtk_complex_t **ideal_phase_shift = beamnet->ideal_phase_shift;
    wtk_complex_t **ideal_phase_covar = beamnet->ideal_phase_covar;
    beamnet->theta = theta;
    wtk_beamnet_compute_multi_channel_phase_shift(beamnet, ideal_phase_shift, theta, phi);
    wtk_beamnet_compute_channel_covariance(beamnet, ideal_phase_shift, ideal_phase_covar, 1);
}
void wtk_beamnet_reset(wtk_beamnet_t *beamnet) {
    int wins = beamnet->cfg->wins;
    int i;

    wtk_strbufs_reset(beamnet->mic, beamnet->cfg->nmicchannel);
    wtk_strbufs_reset(beamnet->sp, beamnet->cfg->nspchannel);

    for (i = 0; i < wins; ++i) {
        beamnet->analysis_window[i] = sin((0.5 + i) * PI / (wins));
    }
    wtk_drft_init_synthesis_window(beamnet->synthesis_window,
                                   beamnet->analysis_window, wins);

    wtk_float_zero_p2(beamnet->analysis_mem, beamnet->cfg->nmicchannel,
                      (beamnet->nbin - 1));
    wtk_float_zero_p2(beamnet->analysis_mem_sp, beamnet->cfg->nspchannel,
                      (beamnet->nbin - 1));
    memset(beamnet->synthesis_mem, 0, sizeof(float) * (beamnet->nbin - 1));

    wtk_complex_zero_p2(beamnet->fft, beamnet->cfg->nmicchannel,
                         (beamnet->nbin));
    wtk_complex_zero_p2(beamnet->fft_sp, beamnet->cfg->nspchannel,
                         (beamnet->nbin));
    memset(beamnet->fftx, 0, sizeof(wtk_complex_t) * (beamnet->nbin));
#ifdef ONNX_DEC
    if(beamnet->cfg->use_onnx){
        qtk_onnxruntime_reset(beamnet->seperator);
        {
            int n = beamnet->seperator->num_in - beamnet->seperator->cfg->outer_in_num;
            if (beamnet->sep_caches[0]) {
                int i;
                for (i = 0; i < n; i++) {
                    beamnet->seperator->api->ReleaseValue(beamnet->sep_caches[i]);
                }
                memset(beamnet->sep_caches, 0, sizeof(OrtValue *) * n);
            }
        }
        memset(beamnet->seperator_out_len, 0, sizeof(int) * (beamnet->seperator->cfg->outer_out_num));
        qtk_onnxruntime_reset(beamnet->beamformer);
        {
            int n = beamnet->beamformer->num_in - beamnet->beamformer->cfg->outer_in_num;
            if (beamnet->beam_caches[0]) {
                int i;
                for (i = 0; i < n; i++) {
                    beamnet->beamformer->api->ReleaseValue(beamnet->beam_caches[i]);
                }
                memset(beamnet->beam_caches, 0, sizeof(OrtValue *) * n);
            }
        }
        memset(beamnet->beamformer_out_len, 0, sizeof(int) * (beamnet->beamformer->cfg->outer_out_num));
    }
#endif
    wtk_float_zero_p2(beamnet->feature, beamnet->cfg->feature_len,
                      (beamnet->nbin));
    memset(beamnet->x, 0, sizeof(float) * beamnet->cfg->feature_len * beamnet->nbin);
    memset(beamnet->mix_spec, 0, sizeof(float) * (beamnet->cfg->nmicchannel) * 2 * beamnet->nbin);
    memset(beamnet->time_delay, 0, sizeof(float) * beamnet->nbin);
    memset(beamnet->mic_aptd, 0, sizeof(float) * beamnet->nbin);
    memset(beamnet->echo_aptd, 0, sizeof(float) * beamnet->nbin);
    wtk_complex_zero_p2(beamnet->mic_covar, beamnet->cfg->out_channels,
                         (beamnet->nbin));
    wtk_complex_zero_p2(beamnet->ideal_phase_covar, beamnet->cfg->out_channels,
                         (beamnet->nbin));
    wtk_complex_zero_p2(beamnet->ideal_phase_shift, beamnet->cfg->nmicchannel,
                         (beamnet->nbin));
    wtk_complex_zero_p2(beamnet->freq_covar, beamnet->cfg->out_channels,
                         (beamnet->nbin));
    memset(beamnet->freq_covar_sum, 0, sizeof(float) * beamnet->nbin);
    memset(beamnet->LPS, 0, sizeof(float) * beamnet->nbin);
    memset(beamnet->echo_LPS, 0, sizeof(float) * beamnet->nbin);
    wtk_float_zero_p2(beamnet->IPDs, beamnet->cfg->out_channels,
                      (beamnet->nbin));
    memset(beamnet->FDDF, 0, sizeof(float) * beamnet->nbin);
    memset(beamnet->mean_mat, 0, sizeof(wtk_complex_t) * beamnet->nbin);
    wtk_complex_zero_p2(beamnet->input_norm, beamnet->cfg->nmicchannel,
                         (beamnet->nbin));

    beamnet->theta = 90.0;
}
void wtk_beamnet_set_notify(wtk_beamnet_t *beamnet, void *ths,
                              wtk_beamnet_notify_f notify) {
    beamnet->notify = notify;
    beamnet->ths = ths;
}

void wtk_beamnet_compute_feature(wtk_beamnet_t * beamnet, wtk_complex_t **fft, wtk_complex_t **fft_sp, float **feature, int theta, int feature_type)
{
    int nbin = beamnet->nbin;
    int nspchannel = beamnet->cfg->nspchannel;
    float *mic_aptd = beamnet->mic_aptd;
    float *echo_aptd = beamnet->echo_aptd;
    wtk_complex_t **mic_covar = beamnet->mic_covar;
    wtk_complex_t **ideal_phase_covar = beamnet->ideal_phase_covar;
    wtk_complex_t **freq_covar = beamnet->freq_covar;
    wtk_complex_t *freq_covar_sum = beamnet->freq_covar_sum;
    float *LPS = beamnet->LPS;
    float *echo_LPS = beamnet->echo_LPS;
    float **IPDs = beamnet->IPDs;
    float *FDDF = beamnet->FDDF;
    int out_channels = beamnet->cfg->out_channels;
    float tmp;
    float eps = 1.1920928955078125e-07;
    int i, k;

    for(k=0;k<nbin;++k){
        mic_aptd[k] = sqrtf(fft[0][k].a * fft[0][k].a + fft[0][k].b * fft[0][k].b) + eps;
    }
    if(nspchannel>0){
        for(k=0;k<nbin;++k){
            echo_aptd[k] = sqrtf(fft_sp[0][k].a * fft_sp[0][k].a + fft_sp[0][k].b * fft_sp[0][k].b) + eps;
        }
    }

    if(feature_type==0 || feature_type==1 || feature_type==2 || feature_type==3){
        for(k=0;k<nbin;++k){
            LPS[k] = 20.0 * log10f(mic_aptd[k]);
        }
        wtk_beamnet_compute_channel_covariance(beamnet, fft, mic_covar, 1);

        for(k=0;k<nbin;++k){
            for(i=0;i<out_channels;++i){
                freq_covar[i][k].a = mic_covar[i][k].a * ideal_phase_covar[i][k].a + mic_covar[i][k].b * ideal_phase_covar[i][k].b;
                freq_covar[i][k].b = mic_covar[i][k].b * ideal_phase_covar[i][k].a - mic_covar[i][k].a * ideal_phase_covar[i][k].b;
            }
        }

        if(feature_type==0){
            memset(FDDF, 0, sizeof(float) * nbin);
            for(k=0;k<nbin;++k){
                for(i=0;i<out_channels;++i){
                    IPDs[i][k] = atan2f(mic_covar[i][k].b, mic_covar[i][k].a);
                }
                for(i=0;i<out_channels;++i){
                    FDDF[k] += atan2f(freq_covar[i][k].b, freq_covar[i][k].a);
                }
                feature[0][k] = LPS[k];
                for(i=0;i<out_channels;++i){
                    feature[i+1][k] = IPDs[i][k];
                }
                feature[out_channels+1][k] = FDDF[k];
            }
        }else if(feature_type==1 || feature_type==2){
            for(k=0;k<nbin;++k){
                for(i=0;i<out_channels;++i){
                    tmp = sqrtf(mic_covar[i][k].a * mic_covar[i][k].a + mic_covar[i][k].b * mic_covar[i][k].b)+eps;
                    mic_covar[i][k].a = mic_covar[i][k].a / tmp;
                    mic_covar[i][k].b = mic_covar[i][k].b / tmp;
                }
            }
            memset(freq_covar_sum, 0, sizeof(wtk_complex_t) * nbin);
            for(k=0;k<nbin;++k){
                for(i=0;i<out_channels;++i){
                    freq_covar_sum[k].a += freq_covar[i][k].a;
                    freq_covar_sum[k].b += freq_covar[i][k].b;
                }
                tmp = sqrtf(freq_covar_sum[k].a * freq_covar_sum[k].a + freq_covar_sum[k].b * freq_covar_sum[k].b)+eps;
                freq_covar_sum[k].a = freq_covar_sum[k].a / tmp;
                freq_covar_sum[k].b = freq_covar_sum[k].b / tmp;
            }
            if(feature_type==1){
                for(k=0;k<nbin;++k){
                    feature[0][k] = LPS[k];
                    for(i=0;i<out_channels;++i){
                        feature[i+1][k] = mic_covar[i][k].a;
                    }
                    for(i=0;i<out_channels;++i){
                        feature[i+1+out_channels][k] = mic_covar[i][k].b;
                    }
                    feature[2*out_channels+1][k] = freq_covar_sum[k].a;
                    feature[2*out_channels+2][k] = freq_covar_sum[k].b;
                }
            }else{
                for(k=0;k<nbin;++k){
                    echo_LPS[k] = 20 * log10f(echo_aptd[k]);
                }
                for(k=0;k<nbin;++k){
                    feature[0][k] = LPS[k];
                    feature[1][k] = echo_LPS[k];
                    for(i=0;i<out_channels;++i){
                        feature[i+2][k] = mic_covar[i][k].a;
                    }
                    for(i=0;i<out_channels;++i){
                        feature[i+2+out_channels][k] = mic_covar[i][k].b;
                    }
                    feature[2*out_channels+2][k] = freq_covar_sum[k].a;
                    feature[2*out_channels+3][k] = freq_covar_sum[k].b;
                }
            }
        }else if(feature_type==3){
            memset(freq_covar_sum, 0, sizeof(wtk_complex_t) * nbin);
            for(k=0;k<nbin;++k){
                for(i=0;i<out_channels;++i){
                    freq_covar_sum[k].a += freq_covar[i][k].a;
                    freq_covar_sum[k].b += freq_covar[i][k].b;
                }
                feature[0][k] = mic_aptd[k];
                for(i=0;i<out_channels;++i){
                    feature[i+1][k] = mic_covar[i][k].a;
                }
                for(i=0;i<out_channels;++i){
                    feature[i+1+out_channels][k] = mic_covar[i][k].b;
                }
                feature[2*out_channels+1][k] = freq_covar_sum[k].a;
                feature[2*out_channels+2][k] = freq_covar_sum[k].b;
            }
        }
    }
}

void wtk_beamnet_feed_seperator(wtk_beamnet_t *beamnet, float *x, float *mix_spec)
{
#ifdef ONNX_DEC
    int i, j;
    const OrtApi *api = beamnet->seperator->api;
    OrtMemoryInfo *meminfo = beamnet->seperator->meminfo;
    qtk_onnxruntime_t *seperator = beamnet->seperator;
    OrtStatus *status;
    int num_in = seperator->num_in;
    int outer_in_num = seperator->cfg->outer_in_num;
    int outer_out_num = seperator->cfg->outer_out_num;
    qtk_onnx_item_t *item;
    void *onnx_out;
    // int64_t size = 0, *out_shape;

    for (i = 0; i < outer_in_num; ++i) {
        item = seperator->in_items + i;
        if (i == 0) {
            memcpy(item->val, x, item->bytes * item->in_dim);
        } else if (i == 1) {
            memcpy(item->val, mix_spec, item->bytes * item->in_dim);
        }
    }

    // printf("num_in:\n");
    for (i = 0; i < num_in; ++i) {
        item = seperator->in_items + i;
        status = api->CreateTensorWithDataAsOrtValue(
            meminfo, item->val, item->bytes * item->in_dim, item->shape,
            item->shape_len, item->type, seperator->in + i);
        // printf("%d\n", i);
        // for(j=0;j<item->shape_len;++j){
        // 	printf("%d %ld\n", j, item->shape[j]);
        // }
        // printf("%ld\n", item->bytes*item->in_dim/sizeof(float));
    }

    status = api->Run(seperator->session, NULL,
                        cast(const char *const *, seperator->name_in),
                        cast(const OrtValue *const *, seperator->in), seperator->num_in,
                        cast(const char *const *, seperator->name_out),
                        seperator->num_out, seperator->out);

    for(j=0;j<outer_out_num;++j){
        if(beamnet->seperator_out_len[j]==0){
            int64_t size = 0, *out_shape;
            int d_len;
            d_len = 1;
            out_shape = qtk_onnxruntime_get_outshape(seperator, j, &size);
            for(int k=0;k<size;++k){
                d_len *= out_shape[k];
            }
            wtk_free(out_shape);
            beamnet->seperator_out_len[j] = d_len;
        }
        if(!beamnet->speech_est_real){
            if(j==0){
                beamnet->speech_est_real = (float *)wtk_malloc(sizeof(float)*beamnet->seperator_out_len[j]);
            }
        }
        if(!beamnet->speech_est_imag){
            if(j==1){
                beamnet->speech_est_imag = (float *)wtk_malloc(sizeof(float)*beamnet->seperator_out_len[j]);
            }
        }
        if(!beamnet->noise_est_real){
            if(j==2){
                beamnet->noise_est_real = (float *)wtk_malloc(sizeof(float)*beamnet->seperator_out_len[j]);
            }
        }
        if(!beamnet->noise_est_imag){
            if(j==3){
                beamnet->noise_est_imag = (float *)wtk_malloc(sizeof(float)*beamnet->seperator_out_len[j]);
            }
        }
        if(!beamnet->speech_mask){
            if(j==4){
                beamnet->speech_mask = (float *)wtk_malloc(sizeof(float)*beamnet->seperator_out_len[j]);
            }
        }
        if(!beamnet->noise_mask){
            if(j==5){
                beamnet->noise_mask = (float *)wtk_malloc(sizeof(float)*beamnet->seperator_out_len[j]);
            }
        }
        onnx_out = qtk_onnxruntime_getout(seperator, j);
        if(j==0){
            memcpy(beamnet->speech_est_real, onnx_out, beamnet->seperator_out_len[j]*sizeof(float));
        }else if(j==1){
            memcpy(beamnet->speech_est_imag, onnx_out, beamnet->seperator_out_len[j]*sizeof(float));
        }else if(j==2){
            memcpy(beamnet->noise_est_real, onnx_out, beamnet->seperator_out_len[j]*sizeof(float));
        }else if(j==3){
            memcpy(beamnet->noise_est_imag, onnx_out, beamnet->seperator_out_len[j]*sizeof(float));
        }else if(j==4){
            memcpy(beamnet->speech_mask, onnx_out, beamnet->seperator_out_len[j]*sizeof(float));
        }else if(j==5){
            memcpy(beamnet->noise_mask, onnx_out, beamnet->seperator_out_len[j]*sizeof(float));
        }
    }

    for (i = outer_in_num, j = outer_out_num; i < num_in; ++i, ++j) {
        item = seperator->in_items + i;
        onnx_out = qtk_onnxruntime_getout(seperator, j);
        memcpy(item->val, onnx_out, item->bytes * item->in_dim);
    }
    qtk_onnxruntime_reset(seperator);
    (void)status;
#endif

}

void wtk_beamnet_feed_beamformer(wtk_beamnet_t *beamnet, float *mix_spec)
{
#ifdef ONNX_DEC
    int i, j;
    float *speech_est_real=beamnet->speech_est_real;
    float *speech_est_imag=beamnet->speech_est_imag;
    float *noise_est_real=beamnet->noise_est_real;
    float *noise_est_imag=beamnet->noise_est_imag;
    const OrtApi *api = beamnet->beamformer->api;
    OrtMemoryInfo *meminfo = beamnet->beamformer->meminfo;
    qtk_onnxruntime_t *beamformer = beamnet->beamformer;
    OrtStatus *status;
    int num_in = beamformer->num_in;
    int outer_in_num = beamformer->cfg->outer_in_num;
    int outer_out_num = beamformer->cfg->outer_out_num;
    qtk_onnx_item_t *item;
    void *onnx_out;
    // int64_t size = 0, *out_shape;

    for (i = 0; i < outer_in_num; ++i) {
        item = beamformer->in_items + i;
        if (i == 0) {
            memcpy(item->val, speech_est_real, item->bytes * item->in_dim);
        } else if (i == 1) {
            memcpy(item->val, speech_est_imag, item->bytes * item->in_dim);
        } else if (i == 2) {
            memcpy(item->val, noise_est_real, item->bytes * item->in_dim);
        } else if (i == 3) {
            memcpy(item->val, noise_est_imag, item->bytes * item->in_dim);
        } else if (i == 4) {
            memcpy(item->val, mix_spec, item->bytes * item->in_dim);
        }
    }

    // printf("num_in:\n");
    for (i = 0; i < num_in; ++i) {
        item = beamformer->in_items + i;
        status = api->CreateTensorWithDataAsOrtValue(
            meminfo, item->val, item->bytes * item->in_dim, item->shape,
            item->shape_len, item->type, beamformer->in + i);
        // printf("%d\n", i);
        // for(j=0;j<item->shape_len;++j){
        // 	printf("%d %ld\n", j, item->shape[j]);
        // }
        // printf("%ld\n", item->bytes*item->in_dim/sizeof(float));
    }

    status = api->Run(beamformer->session, NULL,
                        cast(const char *const *, beamformer->name_in),
                        cast(const OrtValue *const *, beamformer->in), beamformer->num_in,
                        cast(const char *const *, beamformer->name_out),
                        beamformer->num_out, beamformer->out);

    for(j=0;j<outer_out_num;++j){
        if(beamnet->beamformer_out_len[j]==0){
            int64_t size = 0, *out_shape;
            int d_len;
            d_len = 1;
            out_shape = qtk_onnxruntime_get_outshape(beamformer, j, &size);
            for(int k=0;k<size;++k){
                d_len *= out_shape[k];
            }
            wtk_free(out_shape);
            beamnet->beamformer_out_len[j] = d_len;
        }
        if(!beamnet->src_est_real){
            if(j==0){
                beamnet->src_est_real = (float *)wtk_malloc(sizeof(float)*beamnet->beamformer_out_len[j]);
            }
        }
        if(!beamnet->src_est_imag){
            if(j==1){
                beamnet->src_est_imag = (float *)wtk_malloc(sizeof(float)*beamnet->beamformer_out_len[j]);
            }
        }
        onnx_out = qtk_onnxruntime_getout(beamformer, j);
        if(j==0){
            memcpy(beamnet->src_est_real, onnx_out, beamnet->beamformer_out_len[j]*sizeof(float));
        }else if(j==1){
            memcpy(beamnet->src_est_imag, onnx_out, beamnet->beamformer_out_len[j]*sizeof(float));
        }
    }
    for (i = outer_in_num, j = outer_out_num; i < num_in; ++i, ++j) {
        item = beamformer->in_items + i;
        onnx_out = qtk_onnxruntime_getout(beamformer, j);
        memcpy(item->val, onnx_out, item->bytes * item->in_dim);
    }
    qtk_onnxruntime_reset(beamformer);
    (void)status;
#endif

}
void wtk_beamnet_feed_model(wtk_beamnet_t *beamnet, wtk_complex_t **fft, wtk_complex_t **fft_sp, float **feature)
{
    float *x = beamnet->x;
    float *mix_spec = beamnet->mix_spec;
    wtk_complex_t *fftx = beamnet->fftx;
    int i, j;
    int nbin = beamnet->nbin;
    int nmicchannel = beamnet->cfg->nmicchannel;
    int feature_len = beamnet->cfg->feature_len;

    for(i=0;i<feature_len;++i){
        memcpy(x+i*nbin, feature[i], nbin * sizeof(float));
    }

    for(i=0;i<nmicchannel;++i){
        for(j=0;j<nbin;++j){
            mix_spec[i*nbin+j] = fft[i][j].a;
            mix_spec[(nmicchannel+i)*nbin+j] = fft[i][j].b;
        }
    }
    if(beamnet->cfg->use_onnx){
        wtk_beamnet_feed_seperator(beamnet, x, mix_spec);
        wtk_beamnet_feed_beamformer(beamnet, mix_spec);
        for(i=0;i<nbin;++i){
            fftx[i].a = beamnet->src_est_real[i];
            fftx[i].b = beamnet->src_est_imag[i];
        }
    }else{
        memcpy(fftx, fft[0], nbin*sizeof(wtk_complex_t));
    }
}

void wtk_beamnet_feed_decode(wtk_beamnet_t *beamnet, wtk_complex_t **fft, wtk_complex_t **fft_sp)
{
    float **feature = beamnet->feature;
    wtk_beamnet_compute_feature(beamnet, fft, fft_sp, feature, beamnet->theta, beamnet->cfg->feature_type);
    wtk_beamnet_feed_model(beamnet, fft, fft_sp, feature);
}


void wtk_beamnet_feed(wtk_beamnet_t *beamnet, short *data, int len,
                        int is_end) {
    int i, j;
    int nmicchannel = beamnet->cfg->nmicchannel;
    int nspchannel = beamnet->cfg->nspchannel;
    int channel = beamnet->cfg->channel;
	int *mic_channel=beamnet->cfg->mic_channel;
	int *sp_channel=beamnet->cfg->sp_channel;
    int wins = beamnet->cfg->wins;
    int fsize = wins / 2;
    int length;
    wtk_drft_t *rfft = beamnet->rfft;
    float *rfft_in = beamnet->rfft_in;
    wtk_complex_t **fft = beamnet->fft;
    wtk_complex_t **fft_sp = beamnet->fft_sp;
    wtk_complex_t *fftx = beamnet->fftx;
    float **analysis_mem = beamnet->analysis_mem,
          **analysis_mem_sp = beamnet->analysis_mem_sp;
	float *synthesis_mem=beamnet->synthesis_mem;
    float *synthesis_window=beamnet->synthesis_window;
    float *analysis_window = beamnet->analysis_window;
    float *out = beamnet->out;
    short *pv = (short *)out;
    wtk_strbuf_t **mic=beamnet->mic;
    wtk_strbuf_t **sp=beamnet->sp;
    float fv;

	for(i=0;i<len;++i)
	{
		for(j=0; j<nmicchannel; ++j)
		{
            fv = WTK_WAV_SHORT_TO_FLOAT(data[mic_channel[j]]);
            wtk_strbuf_push(mic[j],(char *)&(fv),sizeof(float));
		}
		for(j=0; j<nspchannel; ++j)
		{
            fv = WTK_WAV_SHORT_TO_FLOAT(data[sp_channel[j]]);
            wtk_strbuf_push(sp[j],(char *)&(fv),sizeof(float));
		}
        data += channel;
    }
    length = mic[0]->pos/sizeof(float);
    while(length>=fsize){
        for(i=0;i<nmicchannel;++i){
            wtk_drft_stft(rfft, rfft_in, analysis_mem[i], fft[i], (float *)(mic[i]->data), wins, analysis_window);
        }
        for(i=0;i<nspchannel;++i){
            wtk_drft_stft(rfft, rfft_in, analysis_mem_sp[i], fft_sp[i], (float *)(sp[i]->data), wins, analysis_window);
        }
        wtk_beamnet_feed_decode(beamnet, fft, fft_sp);

        wtk_strbufs_pop(mic, nmicchannel, fsize*sizeof(float));
        wtk_strbufs_pop(sp, nspchannel, fsize*sizeof(float));
        length = mic[0]->pos/sizeof(float);

        // memcpy(fftx, fft[0], nbin*sizeof(wtk_complex_t));
        wtk_drft_istft(rfft, rfft_in, synthesis_mem, fftx, out, wins, synthesis_window);

        for(i=0;i<fsize;++i){
            pv[i] = WTK_WAV_FLOAT_TO_SHORT(out[i]);
        }
        if(beamnet->notify){
            beamnet->notify(beamnet->ths, pv, fsize);
        }
    }
    if(is_end && length>0){
        if(beamnet->notify)
        {
            pv=(short *)mic[0]->data;
            beamnet->notify(beamnet->ths,pv,length);
        }
    }
}