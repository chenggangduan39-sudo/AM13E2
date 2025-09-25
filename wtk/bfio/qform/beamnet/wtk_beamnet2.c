#include "wtk/bfio/qform/beamnet/wtk_beamnet2.h"
#include "qtk/math/qtk_vector.h"
#ifndef WTK_WAV_SHORT_TO_FLOAT
#define WTK_WAV_SHORT_TO_FLOAT(f) ((f) > 0? (f/32767.0) : (f/32768.0))
#endif
#ifndef WTK_WAV_FLOAT_TO_SHORT
#define WTK_WAV_FLOAT_TO_SHORT(f) ((f) > 0? floorf(f*32767.0+0.5) : floorf(f*32768.0+0.5))
#endif

void wtk_beamnet2_compute_channel_covariance(wtk_beamnet2_t *beamnet2, wtk_complex_t **fft, wtk_complex_t **covar_mat, int norm)
{
    int nmicchannel = beamnet2->cfg->nmicchannel;
    int nbin = beamnet2->nbin;
    int i, j, k;
    wtk_complex_t *mean_mat = beamnet2->mean_mat;
    wtk_complex_t **input_norm = beamnet2->input_norm;
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

void wtk_beamnet2_compute_multi_channel_phase_shift(wtk_beamnet2_t *beamnet2, wtk_complex_t **phase_shift, float theta, float phi)
{
    int wins = beamnet2->cfg->wins;
    int nmicchannel = beamnet2->cfg->nmicchannel;
    int nbin = beamnet2->nbin;
    int rate = beamnet2->cfg->rate;
    float sv = beamnet2->cfg->sv;
    float **mic_pos = beamnet2->cfg->mic_pos;
    float *time_delay = beamnet2->time_delay;
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

wtk_beamnet2_t *wtk_beamnet2_new(wtk_beamnet2_cfg_t *cfg) {
    wtk_beamnet2_t *beamnet2;

    beamnet2 = (wtk_beamnet2_t *)wtk_malloc(sizeof(wtk_beamnet2_t));
    beamnet2->cfg = cfg;
    beamnet2->ths = NULL;
    beamnet2->notify = NULL;
    beamnet2->mic = wtk_strbufs_new(beamnet2->cfg->nmicchannel);
    beamnet2->sp = wtk_strbufs_new(beamnet2->cfg->nspchannel);

    beamnet2->nbin = cfg->wins / 2 + 1;
    beamnet2->analysis_window = wtk_malloc(sizeof(float) * cfg->wins);  /// 2);
    beamnet2->synthesis_window = wtk_malloc(sizeof(float) * cfg->wins); /// 2);
    beamnet2->analysis_mem = wtk_float_new_p2(cfg->nmicchannel, beamnet2->nbin - 1);
    beamnet2->analysis_mem_sp = wtk_float_new_p2(cfg->nspchannel, beamnet2->nbin - 1);
    beamnet2->synthesis_mem = wtk_malloc(sizeof(float) * (beamnet2->nbin - 1));
    beamnet2->rfft = wtk_drft_new(cfg->wins);
    beamnet2->rfft_in = (float *)wtk_malloc(sizeof(float) * (cfg->wins));

    beamnet2->fft = wtk_complex_new_p2(cfg->nmicchannel, beamnet2->nbin);
    beamnet2->fft_sp = wtk_complex_new_p2(cfg->nspchannel, beamnet2->nbin);
    beamnet2->fftx = (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * beamnet2->nbin);

    beamnet2->out = wtk_malloc(sizeof(float) * (beamnet2->nbin - 1));

    beamnet2->speech_est_real=NULL;
    beamnet2->speech_est_imag=NULL;
    beamnet2->noise_est_real=NULL;
    beamnet2->noise_est_imag=NULL;
    beamnet2->beamformer_x=NULL;
    beamnet2->src_est_real=NULL;
    beamnet2->src_est_imag=NULL;
#ifdef ONNX_DEC
    beamnet2->seperator = NULL;
    beamnet2->beamformer = NULL;
    beamnet2->sep_caches = NULL;
    beamnet2->beam_caches = NULL;
    beamnet2->seperator_out_len = NULL;
    beamnet2->beamformer_out_len = NULL;
    if(cfg->use_onnx){
        beamnet2->seperator = qtk_onnxruntime_new(&(cfg->seperator));
        beamnet2->sep_caches = wtk_calloc(sizeof(OrtValue *), beamnet2->seperator->num_in - cfg->seperator.outer_in_num);
        if (beamnet2->seperator->num_in - cfg->seperator.outer_in_num != beamnet2->seperator->num_out - cfg->seperator.outer_out_num) {
            wtk_debug("err inner_item\n");
            exit(0);
        }
        beamnet2->seperator_out_len = (int *)wtk_malloc(sizeof(int) * (cfg->seperator.outer_out_num));
        beamnet2->beamformer = qtk_onnxruntime_new(&(cfg->beamformer));
        beamnet2->beam_caches = wtk_calloc(sizeof(OrtValue *), beamnet2->beamformer->num_in - cfg->beamformer.outer_in_num);
        if (beamnet2->beamformer->num_in - cfg->beamformer.outer_in_num != beamnet2->beamformer->num_out - cfg->beamformer.outer_out_num) {
            wtk_debug("err inner_item\n");
            exit(0);
        }
        beamnet2->beamformer_out_len = (int *)wtk_malloc(sizeof(int) * (cfg->seperator.outer_out_num));
    }
#endif
    beamnet2->feature = wtk_float_new_p2(cfg->feature_len, beamnet2->nbin);
    beamnet2->x = (float *)wtk_malloc(sizeof(float) * cfg->feature_len * beamnet2->nbin);
    beamnet2->mix_spec = (float *)wtk_malloc(sizeof(float) * (cfg->nmicchannel) * 2 * beamnet2->nbin);
    beamnet2->time_delay = (float *)wtk_malloc(sizeof(float) * beamnet2->nbin);
    beamnet2->mic_aptd = (float *)wtk_malloc(sizeof(float) * beamnet2->nbin);
    beamnet2->echo_aptd = (float *)wtk_malloc(sizeof(float) * beamnet2->nbin);
    beamnet2->mic_covar = wtk_complex_new_p2(cfg->out_channels, beamnet2->nbin);
    beamnet2->ideal_phase_covar = wtk_complex_new_p2(cfg->out_channels, beamnet2->nbin);
    beamnet2->ideal_phase_shift = wtk_complex_new_p2(cfg->nmicchannel, beamnet2->nbin);
    beamnet2->freq_covar = wtk_complex_new_p2(cfg->out_channels, beamnet2->nbin);
    beamnet2->freq_covar_sum = (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * beamnet2->nbin);
    beamnet2->LPS = (float *)wtk_malloc(sizeof(float) * beamnet2->nbin);
    beamnet2->echo_LPS = (float *)wtk_malloc(sizeof(float) * beamnet2->nbin);
    beamnet2->IPDs = wtk_float_new_p2(cfg->out_channels, beamnet2->nbin);
    beamnet2->FDDF = (float *)wtk_malloc(sizeof(float) * beamnet2->nbin);
    beamnet2->mean_mat = (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * beamnet2->nbin);
    beamnet2->input_norm = wtk_complex_new_p2(cfg->nmicchannel, beamnet2->nbin);
    beamnet2->speech_covar = (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * cfg->nmicchannel*cfg->nmicchannel);
    beamnet2->noise_covar = (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * cfg->nmicchannel*cfg->nmicchannel);

    beamnet2->qmmse=NULL;
    if(cfg->use_qmmse){
		beamnet2->qmmse=wtk_qmmse_new(&(cfg->qmmse));
    }

    beamnet2->entropy_E=NULL;
    beamnet2->entropy_Eb=NULL;
    beamnet2->last_fftx=NULL;
    if(cfg->entropy_thresh>0){
        beamnet2->entropy_E=(float *)wtk_malloc(sizeof(float)*beamnet2->nbin);
        beamnet2->entropy_Eb=(float *)wtk_malloc(sizeof(float)*cfg->wins);
    }
    if(cfg->delay_nf>0){
        beamnet2->last_fftx=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*beamnet2->nbin*(cfg->delay_nf+1));
    }

    beamnet2->gf=(float *)wtk_malloc(sizeof(float)*beamnet2->nbin);
    beamnet2->last_gf=NULL;
    if((cfg->use_qmmse && cfg->qmmse.use_agc_mask) || cfg->use_bf){
        if(cfg->delay_nf>0){
            beamnet2->last_gf=(float *)wtk_malloc(sizeof(float)*beamnet2->nbin*(cfg->delay_nf+1));
        }
    }
    beamnet2->bf=NULL;
	beamnet2->covm=NULL;
    if(cfg->use_bf){
        beamnet2->bf = wtk_bf_new(&(cfg->bf), cfg->wins);
        beamnet2->covm = wtk_covm_new(&(cfg->covm), beamnet2->nbin, cfg->nbfchannel);
    }

	beamnet2->sim_scov=NULL;
	beamnet2->sim_ncov=NULL;
	beamnet2->sim_cnt_sum=NULL;
	if(cfg->use_sim_bf){
		beamnet2->sim_scov = (float *)wtk_malloc(sizeof(float)*beamnet2->nbin);
		beamnet2->sim_ncov = (float *)wtk_malloc(sizeof(float)*beamnet2->nbin);
		beamnet2->sim_cnt_sum = (int *)wtk_malloc(sizeof(int)*beamnet2->nbin);
    }

    beamnet2->gc = NULL;
	if(cfg->use_gc){
		beamnet2->gc=qtk_gain_controller_new(&(cfg->gc));
		qtk_gain_controller_set_mode(beamnet2->gc,0);
		beamnet2->gc->kalman.Z_k = cfg->gc_gain * 1.0/1000.0;
	}

	beamnet2->bs_win=NULL;
	if(cfg->use_bs_win)
	{
		beamnet2->bs_win=wtk_math_create_hanning_window2(cfg->wins/2);
	}

    wtk_beamnet2_reset(beamnet2);

    return beamnet2;
}
void wtk_beamnet2_delete(wtk_beamnet2_t *beamnet2) {
    wtk_strbufs_delete(beamnet2->mic, beamnet2->cfg->nmicchannel);
    wtk_strbufs_delete(beamnet2->sp, beamnet2->cfg->nspchannel);

    wtk_free(beamnet2->analysis_window);
    wtk_free(beamnet2->synthesis_window);
    wtk_float_delete_p2(beamnet2->analysis_mem, beamnet2->cfg->nmicchannel);
    wtk_float_delete_p2(beamnet2->analysis_mem_sp, beamnet2->cfg->nspchannel);
    wtk_free(beamnet2->synthesis_mem);
    wtk_free(beamnet2->rfft_in);
    wtk_drft_delete(beamnet2->rfft);
    wtk_complex_delete_p2(beamnet2->fft, beamnet2->cfg->nmicchannel);
    wtk_complex_delete_p2(beamnet2->fft_sp, beamnet2->cfg->nspchannel);

    wtk_free(beamnet2->fftx);

    wtk_free(beamnet2->out);
#ifdef ONNX_DEC
    if(beamnet2->cfg->use_onnx){
        {
            int n = beamnet2->seperator->num_in - beamnet2->seperator->cfg->outer_in_num;
            if (beamnet2->sep_caches[0]) {
                int i;
                for (i = 0; i < n; i++) {
                    beamnet2->seperator->api->ReleaseValue(beamnet2->sep_caches[i]);
                }
            }
        }
        if (beamnet2->seperator) {
            qtk_onnxruntime_delete(beamnet2->seperator);
        }
        wtk_free(beamnet2->seperator_out_len);
        wtk_free(beamnet2->sep_caches);
        {
            int n = beamnet2->beamformer->num_in - beamnet2->beamformer->cfg->outer_in_num;
            if (beamnet2->beam_caches[0]) {
                int i;
                for (i = 0; i < n; i++) {
                    beamnet2->beamformer->api->ReleaseValue(beamnet2->beam_caches[i]);
                }
            }
        }
        if (beamnet2->beamformer) {
            qtk_onnxruntime_delete(beamnet2->beamformer);
        }
        wtk_free(beamnet2->beamformer_out_len);
        wtk_free(beamnet2->beam_caches);
    }
#endif
    wtk_float_delete_p2(beamnet2->feature, beamnet2->cfg->feature_len);
    wtk_free(beamnet2->x);
    wtk_free(beamnet2->mix_spec);
    wtk_free(beamnet2->time_delay);
    wtk_free(beamnet2->mic_aptd);
    wtk_free(beamnet2->echo_aptd);
    wtk_complex_delete_p2(beamnet2->mic_covar, beamnet2->cfg->out_channels);
    wtk_complex_delete_p2(beamnet2->ideal_phase_covar, beamnet2->cfg->out_channels);
    wtk_complex_delete_p2(beamnet2->ideal_phase_shift, beamnet2->cfg->nmicchannel);
    wtk_complex_delete_p2(beamnet2->freq_covar, beamnet2->cfg->out_channels);
    wtk_free(beamnet2->freq_covar_sum);
    wtk_free(beamnet2->LPS);
    wtk_free(beamnet2->echo_LPS);
    wtk_float_delete_p2(beamnet2->IPDs, beamnet2->cfg->out_channels);
    wtk_free(beamnet2->FDDF);
    wtk_free(beamnet2->mean_mat);
    wtk_complex_delete_p2(beamnet2->input_norm, beamnet2->cfg->nmicchannel);
    wtk_free(beamnet2->speech_covar);
    wtk_free(beamnet2->noise_covar);

    if(beamnet2->speech_est_real){
        wtk_free(beamnet2->speech_est_real);
    }
    if(beamnet2->speech_est_imag){
        wtk_free(beamnet2->speech_est_imag);
    }
    if(beamnet2->noise_est_real){
        wtk_free(beamnet2->noise_est_real);
    }
    if(beamnet2->noise_est_imag){
        wtk_free(beamnet2->noise_est_imag);
    }
    if(beamnet2->beamformer_x){
        wtk_free(beamnet2->beamformer_x);
    }
    if(beamnet2->src_est_real){
        wtk_free(beamnet2->src_est_real);
    }
    if(beamnet2->src_est_imag){
        wtk_free(beamnet2->src_est_imag);
    }

    if(beamnet2->qmmse){
        wtk_qmmse_delete(beamnet2->qmmse);
    }
    if(beamnet2->entropy_E){
        wtk_free(beamnet2->entropy_E);
    }
    if(beamnet2->entropy_Eb){
        wtk_free(beamnet2->entropy_Eb);
    }
    if(beamnet2->last_fftx){
        wtk_free(beamnet2->last_fftx);
    }
    if(beamnet2->gf){
        wtk_free(beamnet2->gf);
    }
    if(beamnet2->last_gf){
        wtk_free(beamnet2->last_gf);
    }
    if(beamnet2->bf){
        wtk_bf_delete(beamnet2->bf);
    }
    if(beamnet2->covm){
        wtk_covm_delete(beamnet2->covm);
    }
    if(beamnet2->sim_scov)
	{
        wtk_free(beamnet2->sim_scov);
        wtk_free(beamnet2->sim_ncov);
        wtk_free(beamnet2->sim_cnt_sum);
    }
	if(beamnet2->gc){
		qtk_gain_controller_delete(beamnet2->gc);
	}
	if(beamnet2->bs_win)
	{
		wtk_free(beamnet2->bs_win);
	}

    wtk_free(beamnet2);
}

void wtk_beamnet2_start(wtk_beamnet2_t *beamnet2, float theta, float phi) {
    wtk_complex_t **ideal_phase_shift = beamnet2->ideal_phase_shift;
    wtk_complex_t **ideal_phase_covar = beamnet2->ideal_phase_covar;
    beamnet2->theta = theta;
    wtk_beamnet2_compute_multi_channel_phase_shift(beamnet2, ideal_phase_shift, theta, phi);
    wtk_beamnet2_compute_channel_covariance(beamnet2, ideal_phase_shift, ideal_phase_covar, beamnet2->cfg->norm_channel);

    if(beamnet2->cfg->use_bf){
        wtk_bf_update_ovec(beamnet2->bf,0,0);
        wtk_bf_init_w(beamnet2->bf);
    }
}
void wtk_beamnet2_reset(wtk_beamnet2_t *beamnet2) {
    int wins = beamnet2->cfg->wins;
    int i;

    wtk_strbufs_reset(beamnet2->mic, beamnet2->cfg->nmicchannel);
    wtk_strbufs_reset(beamnet2->sp, beamnet2->cfg->nspchannel);

    for (i = 0; i < wins; ++i) {
        beamnet2->analysis_window[i] = sin((0.5 + i) * PI / (wins));
    }
    wtk_drft_init_synthesis_window(beamnet2->synthesis_window,
                                   beamnet2->analysis_window, wins);

    wtk_float_zero_p2(beamnet2->analysis_mem, beamnet2->cfg->nmicchannel,
                      (beamnet2->nbin - 1));
    wtk_float_zero_p2(beamnet2->analysis_mem_sp, beamnet2->cfg->nspchannel,
                      (beamnet2->nbin - 1));
    memset(beamnet2->synthesis_mem, 0, sizeof(float) * (beamnet2->nbin - 1));

    wtk_complex_zero_p2(beamnet2->fft, beamnet2->cfg->nmicchannel,
                         (beamnet2->nbin));
    wtk_complex_zero_p2(beamnet2->fft_sp, beamnet2->cfg->nspchannel,
                         (beamnet2->nbin));
    memset(beamnet2->fftx, 0, sizeof(wtk_complex_t) * (beamnet2->nbin));
#ifdef ONNX_DEC
    if(beamnet2->cfg->use_onnx){
        qtk_onnxruntime_reset(beamnet2->seperator);
        {
            int n = beamnet2->seperator->num_in - beamnet2->seperator->cfg->outer_in_num;
            if (beamnet2->sep_caches[0]) {
                int i;
                for (i = 0; i < n; i++) {
                    beamnet2->seperator->api->ReleaseValue(beamnet2->sep_caches[i]);
                }
                memset(beamnet2->sep_caches, 0, sizeof(OrtValue *) * n);
            }
        }
        memset(beamnet2->seperator_out_len, 0, sizeof(int) * (beamnet2->seperator->cfg->outer_out_num));
        qtk_onnxruntime_reset(beamnet2->beamformer);
        {
            int n = beamnet2->beamformer->num_in - beamnet2->beamformer->cfg->outer_in_num;
            if (beamnet2->beam_caches[0]) {
                int i;
                for (i = 0; i < n; i++) {
                    beamnet2->beamformer->api->ReleaseValue(beamnet2->beam_caches[i]);
                }
                memset(beamnet2->beam_caches, 0, sizeof(OrtValue *) * n);
            }
        }
        memset(beamnet2->beamformer_out_len, 0, sizeof(int) * (beamnet2->beamformer->cfg->outer_out_num));
    }
#endif
    wtk_float_zero_p2(beamnet2->feature, beamnet2->cfg->feature_len,
                      (beamnet2->nbin));
    memset(beamnet2->x, 0, sizeof(float) * beamnet2->cfg->feature_len * beamnet2->nbin);
    memset(beamnet2->mix_spec, 0, sizeof(float) * (beamnet2->cfg->nmicchannel) * 2 * beamnet2->nbin);
    memset(beamnet2->time_delay, 0, sizeof(float) * beamnet2->nbin);
    memset(beamnet2->mic_aptd, 0, sizeof(float) * beamnet2->nbin);
    memset(beamnet2->echo_aptd, 0, sizeof(float) * beamnet2->nbin);
    wtk_complex_zero_p2(beamnet2->mic_covar, beamnet2->cfg->out_channels,
                         (beamnet2->nbin));
    wtk_complex_zero_p2(beamnet2->ideal_phase_covar, beamnet2->cfg->out_channels,
                         (beamnet2->nbin));
    wtk_complex_zero_p2(beamnet2->ideal_phase_shift, beamnet2->cfg->nmicchannel,
                         (beamnet2->nbin));
    wtk_complex_zero_p2(beamnet2->freq_covar, beamnet2->cfg->out_channels,
                         (beamnet2->nbin));
    memset(beamnet2->freq_covar_sum, 0, sizeof(float) * beamnet2->nbin);
    memset(beamnet2->LPS, 0, sizeof(float) * beamnet2->nbin);
    memset(beamnet2->echo_LPS, 0, sizeof(float) * beamnet2->nbin);
    wtk_float_zero_p2(beamnet2->IPDs, beamnet2->cfg->out_channels,
                      (beamnet2->nbin));
    memset(beamnet2->FDDF, 0, sizeof(float) * beamnet2->nbin);
    memset(beamnet2->mean_mat, 0, sizeof(wtk_complex_t) * beamnet2->nbin);
    wtk_complex_zero_p2(beamnet2->input_norm, beamnet2->cfg->nmicchannel,(beamnet2->nbin));
    memset(beamnet2->speech_covar, 0, sizeof(wtk_complex_t) * beamnet2->cfg->nmicchannel*beamnet2->cfg->nmicchannel);
    memset(beamnet2->noise_covar, 0, sizeof(wtk_complex_t) * beamnet2->cfg->nmicchannel*beamnet2->cfg->nmicchannel);

    if(beamnet2->qmmse){
        wtk_qmmse_reset(beamnet2->qmmse);
    }
    if(beamnet2->entropy_E){
        memset(beamnet2->entropy_E, 0, sizeof(float) * beamnet2->nbin);
    }
    if(beamnet2->entropy_Eb){
        memset(beamnet2->entropy_Eb, 0, sizeof(float) * beamnet2->nbin);
    }
    if(beamnet2->last_fftx){
        memset(beamnet2->last_fftx, 0, sizeof(wtk_complex_t) * (beamnet2->nbin)*(beamnet2->cfg->delay_nf+1));
    }
    if(beamnet2->gf){
        memset(beamnet2->gf, 0, sizeof(float) * beamnet2->nbin);
    }
    if(beamnet2->last_gf){
        memset(beamnet2->last_gf, 0, sizeof(float) * beamnet2->nbin);
    }
    if(beamnet2->bf){
        wtk_bf_reset(beamnet2->bf);
    }
    if(beamnet2->covm){
        wtk_covm_reset(beamnet2->covm);
    }
    if(beamnet2->sim_scov)
	{
		memset(beamnet2->sim_scov, 0, sizeof(float)*beamnet2->nbin);
		memset(beamnet2->sim_ncov, 0, sizeof(float)*beamnet2->nbin);
		memset(beamnet2->sim_cnt_sum, 0, sizeof(int)*beamnet2->nbin);
    }

    if(beamnet2->gc){
        qtk_gain_controller_reset(beamnet2->gc);
    }

    beamnet2->entropy_in_cnt = 0;
    beamnet2->entropy_silcnt = 0;
    beamnet2->entropy_sil = 1;

    beamnet2->delay_nf = beamnet2->cfg->delay_nf;

    beamnet2->theta = 90.0;
    beamnet2->nframe = 0;
	beamnet2->bfflushnf=beamnet2->cfg->bfflush_cnt;
    
	beamnet2->bs_scale=1.0;
	beamnet2->bs_last_scale=1.0;
	beamnet2->bs_real_scale=1.0;
	beamnet2->bs_max_cnt=0;
	beamnet2->gc_cnt=0;
}
void wtk_beamnet2_set_notify(wtk_beamnet2_t *beamnet2, void *ths,
                              wtk_beamnet2_notify_f notify) {
    beamnet2->notify = notify;
    beamnet2->ths = ths;
}

void wtk_beamnet2_compute_feature(wtk_beamnet2_t * beamnet2, wtk_complex_t **fft, wtk_complex_t **fft_sp, float **feature, int theta, int feature_type)
{
    int nbin = beamnet2->nbin;
    int nspchannel = beamnet2->cfg->nspchannel;
    float *mic_aptd = beamnet2->mic_aptd;
    float *echo_aptd = beamnet2->echo_aptd;
    wtk_complex_t **mic_covar = beamnet2->mic_covar;
    wtk_complex_t **ideal_phase_covar = beamnet2->ideal_phase_covar;
    wtk_complex_t **freq_covar = beamnet2->freq_covar;
    wtk_complex_t *freq_covar_sum = beamnet2->freq_covar_sum;
    float *LPS = beamnet2->LPS;
    float *echo_LPS = beamnet2->echo_LPS;
    float **IPDs = beamnet2->IPDs;
    float *FDDF = beamnet2->FDDF;
    int out_channels = beamnet2->cfg->out_channels;
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
        wtk_beamnet2_compute_channel_covariance(beamnet2, fft, mic_covar, beamnet2->cfg->norm_channel);
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

void wtk_beamnet2_feed_seperator(wtk_beamnet2_t *beamnet2, float *x, float *mix_spec)
{
#ifdef ONNX_DEC
    int i, j;
    const OrtApi *api = beamnet2->seperator->api;
    OrtMemoryInfo *meminfo = beamnet2->seperator->meminfo;
    qtk_onnxruntime_t *seperator = beamnet2->seperator;
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
        if(beamnet2->seperator_out_len[j]==0){
            int64_t size = 0, *out_shape;
            int d_len;
            d_len = 1;
            out_shape = qtk_onnxruntime_get_outshape(seperator, j, &size);
            for(int k=0;k<size;++k){
                d_len *= out_shape[k];
            }
            wtk_free(out_shape);
            beamnet2->seperator_out_len[j] = d_len;
        }
        if(!beamnet2->speech_est_real){
            if(j==0){
                beamnet2->speech_est_real = (float *)wtk_malloc(sizeof(float)*beamnet2->seperator_out_len[j]);
            }
        }
        if(!beamnet2->speech_est_imag){
            if(j==1){
                beamnet2->speech_est_imag = (float *)wtk_malloc(sizeof(float)*beamnet2->seperator_out_len[j]);
            }
        }
        if(!beamnet2->noise_est_real){
            if(j==2){
                beamnet2->noise_est_real = (float *)wtk_malloc(sizeof(float)*beamnet2->seperator_out_len[j]);
            }
        }
        if(!beamnet2->noise_est_imag){
            if(j==3){
                beamnet2->noise_est_imag = (float *)wtk_malloc(sizeof(float)*beamnet2->seperator_out_len[j]);
            }
        }
        onnx_out = qtk_onnxruntime_getout(seperator, j);
        if(j==0){
            memcpy(beamnet2->speech_est_real, onnx_out, beamnet2->seperator_out_len[j]*sizeof(float));
        }else if(j==1){
            memcpy(beamnet2->speech_est_imag, onnx_out, beamnet2->seperator_out_len[j]*sizeof(float));
        }else if(j==2){
            memcpy(beamnet2->noise_est_real, onnx_out, beamnet2->seperator_out_len[j]*sizeof(float));
        }else if(j==3){
            memcpy(beamnet2->noise_est_imag, onnx_out, beamnet2->seperator_out_len[j]*sizeof(float));
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

void wtk_beamnet2_feature_extract(wtk_beamnet2_t *beamnet2)
{
    float *speech_est_real=beamnet2->speech_est_real;
    float *speech_est_imag=beamnet2->speech_est_imag;
    float *noise_est_real=beamnet2->noise_est_real;
    float *noise_est_imag=beamnet2->noise_est_imag;
    wtk_complex_t *cov = beamnet2->speech_covar;
    wtk_complex_t *cov1 = beamnet2->noise_covar;
    wtk_complex_t *a, *b, *a1, *b1;

    int k;
    int i,j;
    int nmicchannel = beamnet2->cfg->nmicchannel;
    int nbin = beamnet2->nbin;

    if(beamnet2->cfg->use_mvdr){
        if(!beamnet2->beamformer_x){
            beamnet2->beamformer_x = (float *)wtk_malloc(sizeof(float)*beamnet2->nbin*nmicchannel*nmicchannel*4);
            memset(beamnet2->beamformer_x, 0, sizeof(float)*beamnet2->nbin*nmicchannel*nmicchannel*4);
        }
        for(k=0;k<nbin;++k){
            memset(cov, 0, sizeof(wtk_complex_t) * nmicchannel*nmicchannel);
            memset(cov1, 0, sizeof(wtk_complex_t) * nmicchannel*nmicchannel);
            for(i=0;i<nmicchannel;++i){
                for(j=i;j<nmicchannel;++j){
                    a=cov+i*nmicchannel+j;
                    b=cov1+i*nmicchannel+j;
                    if(i!=j){
                        a->a+=speech_est_real[i*nbin+k]*speech_est_real[j*nbin+k]+speech_est_imag[i*nbin+k]*speech_est_imag[j*nbin+k];
                        a->b+=-speech_est_real[i*nbin+k]*speech_est_imag[j*nbin+k]+speech_est_imag[i*nbin+k]*speech_est_real[j*nbin+k];
                        b->a+=noise_est_real[i*nbin+k]*noise_est_real[j*nbin+k]+noise_est_imag[i*nbin+k]*noise_est_imag[j*nbin+k];
                        b->b+=-noise_est_real[i*nbin+k]*noise_est_imag[j*nbin+k]+noise_est_imag[i*nbin+k]*noise_est_real[j*nbin+k];
                    }else{
                        a->a+=speech_est_real[i*nbin+k]*speech_est_real[j*nbin+k]+speech_est_imag[i*nbin+k]*speech_est_imag[j*nbin+k];
                        b->a+=noise_est_real[i*nbin+k]*noise_est_real[j*nbin+k]+noise_est_imag[i*nbin+k]*noise_est_imag[j*nbin+k];
                    }
                }
            }
            for(i=0;i<nmicchannel;++i){
                for(j=i;j<nmicchannel;++j){
                    a=cov+i*nmicchannel+j;
                    b=cov1+i*nmicchannel+j;
                    if(i!=j)
                    {
                        a1=cov+j*nmicchannel+i;
                        a1->a=a->a;
                        a1->b=-a->b;
                        b1=cov1+j*nmicchannel+i;
                        b1->a=b->a;
                        b1->b=-b->b;
                    }
                }
            }
            for(i=0;i<nmicchannel*nmicchannel;++i){
                beamnet2->beamformer_x[i*nbin+k] = cov[i].a;
                beamnet2->beamformer_x[(i+nmicchannel*nmicchannel)*nbin+k] = cov[i].b;
                beamnet2->beamformer_x[(i+nmicchannel*nmicchannel*2)*nbin+k] = cov1[i].a;
                beamnet2->beamformer_x[(i+nmicchannel*nmicchannel*3)*nbin+k] = cov1[i].b;
            }
        }
    }else{

    }
}

void wtk_beamnet2_feed_beamformer(wtk_beamnet2_t *beamnet2, float *mix_spec)
{
#ifdef ONNX_DEC
    int i, j;
    float *beamformer_x=beamnet2->beamformer_x;
    const OrtApi *api = beamnet2->beamformer->api;
    OrtMemoryInfo *meminfo = beamnet2->beamformer->meminfo;
    qtk_onnxruntime_t *beamformer = beamnet2->beamformer;
    OrtStatus *status;
    int num_in = beamformer->num_in;
    int outer_in_num = beamformer->cfg->outer_in_num;
    int outer_out_num = beamformer->cfg->outer_out_num;
    qtk_onnx_item_t *item;
    void *onnx_out;
    // int64_t size = 0, *out_shape;
    // int step=0;
    for (i = 0; i < outer_in_num; ++i) {
        item = beamformer->in_items + i;
        if (i == 0) {
            memcpy(item->val, beamformer_x, item->bytes * item->in_dim);
        }else if (i == 1) {
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
        if(beamnet2->beamformer_out_len[j]==0){
            int64_t size = 0, *out_shape;
            int d_len;
            d_len = 1;
            out_shape = qtk_onnxruntime_get_outshape(beamformer, j, &size);
            for(int k=0;k<size;++k){
                d_len *= out_shape[k];
            }
            wtk_free(out_shape);
            beamnet2->beamformer_out_len[j] = d_len;
        }
        if(!beamnet2->src_est_real){
            if(j==0){
                beamnet2->src_est_real = (float *)wtk_malloc(sizeof(float)*beamnet2->beamformer_out_len[j]);
            }
        }
        if(!beamnet2->src_est_imag){
            if(j==1){
                beamnet2->src_est_imag = (float *)wtk_malloc(sizeof(float)*beamnet2->beamformer_out_len[j]);
            }
        }
        onnx_out = qtk_onnxruntime_getout(beamformer, j);
        if(j==0){
            memcpy(beamnet2->src_est_real, onnx_out, beamnet2->beamformer_out_len[j]*sizeof(float));
        }else if(j==1){
            memcpy(beamnet2->src_est_imag, onnx_out, beamnet2->beamformer_out_len[j]*sizeof(float));
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
void wtk_beamnet2_feed_model(wtk_beamnet2_t *beamnet2, wtk_complex_t **fft, wtk_complex_t **fft_sp, float **feature)
{
    float *x = beamnet2->x;
    float *mix_spec = beamnet2->mix_spec;
    wtk_complex_t *fftx = beamnet2->fftx;
    int i, j;
    int nbin = beamnet2->nbin;
    int nmicchannel = beamnet2->cfg->nmicchannel;
    int feature_len = beamnet2->cfg->feature_len;

    for(i=0;i<feature_len;++i){
        memcpy(x+i*nbin, feature[i], nbin * sizeof(float));
    }

    for(i=0;i<nmicchannel;++i){
        for(j=0;j<nbin;++j){
            mix_spec[i*nbin+j] = fft[i][j].a;
            mix_spec[(nmicchannel+i)*nbin+j] = fft[i][j].b;
        }
    }
    if(beamnet2->cfg->use_onnx){
        wtk_beamnet2_feed_seperator(beamnet2, x, mix_spec);
        wtk_beamnet2_feature_extract(beamnet2);
        wtk_beamnet2_feed_beamformer(beamnet2, mix_spec);
        for(i=0;i<nbin;++i){
            fftx[i].a = beamnet2->src_est_real[i];
            fftx[i].b = beamnet2->src_est_imag[i];
        }

    }else{
        memcpy(fftx, fft[0], nbin*sizeof(wtk_complex_t));
    }
}

void wtk_beamnet2_feed_decode(wtk_beamnet2_t *beamnet2, wtk_complex_t **fft, wtk_complex_t **fft_sp)
{
    float **feature = beamnet2->feature;
    wtk_beamnet2_compute_feature(beamnet2, fft, fft_sp, feature, beamnet2->theta, beamnet2->cfg->feature_type);
    wtk_beamnet2_feed_model(beamnet2, fft, fft_sp, feature);
}


float wtk_beamnet2_entropy(wtk_beamnet2_t *beamnet2, wtk_complex_t *fftx)
{
    int rate = beamnet2->cfg->rate;
    int wins = beamnet2->cfg->wins;
    int i;
    int fx1 = (250*1.0*wins)/rate;
    int fx2 = (3500*1.0*wins)/rate;
    int km = floor(wins*1.0/8);
    float K = 0.5;
    float *E=beamnet2->entropy_E;
    float P1;
    float *Eb=beamnet2->entropy_Eb;
    float sum;
    float prob;
    float Hb;

    memset(E, 0, sizeof(float) * beamnet2->nbin);
    memset(Eb, 0, sizeof(float) * wins);
    qtk_vector_cpx_mag_squared(fftx + fx1, E + fx1, fx2 - fx1);
    sum = 1e-10;
    for(i=fx1;i<fx2;++i)
    {
        sum += E[i];
    }
    for(i=fx1;i<fx2;++i)
    {
        P1 = E[i]/sum;
        if(P1>=0.9){
            E[i] = 0;
        }
    }
    sum = 0;
    for(i=0;i<km;++i)
    {
        Eb[i] = K;
        Eb[i] += E[i*4]+E[i*4+1]+E[i*4+2]+E[i*4+3];
        sum += Eb[i];
    }
    Hb = 0;
    for(i=0;i<wins;++i)
    {
        prob = Eb[i]/sum;
        Hb += -prob*logf(prob+1e-10);
    }
    // printf("%f\n", Hb);

    return Hb;
}

void wtk_beamnet2_feed_bf(wtk_beamnet2_t *beamnet2, wtk_complex_t **fft)
{
	wtk_complex_t *fftx=beamnet2->fftx;
	int k,nbin=beamnet2->nbin;
    int i;
    // int nmicchannel=beamnet2->cfg->nmicchannel;
	int nbfchannel=beamnet2->cfg->nbfchannel;
	wtk_bf_t *bf=beamnet2->bf;
    wtk_covm_t *covm;
    int b;
    float gf;
    wtk_complex_t fft2[64];
    wtk_complex_t ffts[64];
    wtk_complex_t ffty[64];
	int clip_s=beamnet2->cfg->clip_s;
	int clip_e=beamnet2->cfg->clip_e;
	int bf_clip_s=beamnet2->cfg->bf_clip_s;
	int bf_clip_e=beamnet2->cfg->bf_clip_e;
	int bfflush_cnt=beamnet2->cfg->bfflush_cnt;

	fftx[0].a=fftx[0].b=0;
	fftx[nbin-1].a=fftx[nbin-1].b=0;
	if(beamnet2->cfg->use_fixtheta)
	{
		for(k=1; k<nbin-1; ++k)
		{
			gf=beamnet2->gf[k];
			for(i=0; i<nbfchannel; ++i)
			{
				ffts[i].a=fft[i][k].a*gf;
				ffts[i].b=fft[i][k].b*gf;
            }
			wtk_bf_output_fft_k(bf, ffts, fftx+k, k);
        }
	}else if(beamnet2->cfg->use_sim_bf){
        float *scov;
        float *ncov;
        int *cnt_sum;
        float scov_alpha;
        float ncov_alpha;
        float scov_alpha_1;
        float ncov_alpha_1;
        int init_covnf;
        float w;
        float mu;
        scov = beamnet2->sim_scov;
        ncov = beamnet2->sim_ncov;
        cnt_sum = beamnet2->sim_cnt_sum;
        scov_alpha = beamnet2->covm->cfg->scov_alpha;
        ncov_alpha = beamnet2->covm->cfg->ncov_alpha;
        init_covnf = beamnet2->covm->cfg->init_scovnf;
        scov_alpha_1 = 1.0 - scov_alpha;
        ncov_alpha_1 = 1.0 - ncov_alpha;
		for(k=clip_s+1; k<clip_e; ++k)
		{
			gf=beamnet2->gf[k];
            if(k>=bf_clip_s && k<bf_clip_e){
                mu=beamnet2->cfg->bfmu;
            }else{
                mu=beamnet2->cfg->bfmu2;
            }
			ffts[0].a=fft[0][k].a*gf;
			ffts[0].b=fft[0][k].b*gf;
			ffty[0].a=fft[0][k].a*(1-gf);
			ffty[0].b=fft[0][k].b*(1-gf);

			if(cnt_sum[k] < init_covnf){
				scov[k] += ffts[0].a*ffts[0].a+ffts[0].b*ffts[0].b;
			}else if(cnt_sum[k] == init_covnf){
                scov[k] /= cnt_sum[k];
			}else{
				scov[k] = scov_alpha_1*scov[k] + scov_alpha*(ffts[0].a*ffts[0].a+ffts[0].b*ffts[0].b);
            }
			if(cnt_sum[k] < init_covnf){
				ncov[k] += ffty[0].a*ffty[0].a+ffty[0].b*ffty[0].b;
			}else if(cnt_sum[k] == init_covnf){
                ncov[k] /= cnt_sum[k];
			}else{
				ncov[k] = ncov_alpha_1*ncov[k] + ncov_alpha*(ffty[0].a*ffty[0].a+ffty[0].b*ffty[0].b);
            }
            cnt_sum[k]++;
			if(ncov[k]<1e-10)
			{
				ncov[k]=1e-10;
            }
			w = scov[k]/ncov[k];
			w = w/(mu+w);
            fftx[k].a = ffts[0].a * w;
            fftx[k].b = ffts[0].b * w;
        }
	}else{
		for(k=clip_s+1; k<clip_e; ++k)
		{
			gf=beamnet2->gf[k];
            if(k>=bf_clip_s && k<bf_clip_e){
                bf->cfg->mu=beamnet2->cfg->bfmu;
            }else{
                bf->cfg->mu=beamnet2->cfg->bfmu2;
            }
            covm = beamnet2->covm;
			for(i=0; i<nbfchannel; ++i)
			{
				ffts[i].a=fft[i][k].a*gf;
				ffts[i].b=fft[i][k].b*gf;

				ffty[i].a=fft[i][k].a*(1-gf);
				ffty[i].b=fft[i][k].b*(1-gf);
                // printf("%f %f %f %f %f %f %f\n", ffts[i].a, ffts[i].b, ffty[i].a, ffty[i].b, fft[i][k].a, fft[i][k].b, gf);
            }
            for(i=0; i<nbfchannel; ++i)
            {
                fft2[i]=ffts[i];
            }

			b=0;
			b=wtk_covm_feed_fft3(covm, ffty, k, 1);
			if(b==1)
			{
                wtk_bf_update_ncov(bf, covm->ncov, k);
            }
			if(covm->scov)
			{
				b=wtk_covm_feed_fft3(covm, ffts, k, 0);
				if(b==1)
				{
                    wtk_bf_update_scov(bf, covm->scov, k);
                }
            }
			if(b==1 && beamnet2->bfflushnf==bfflush_cnt)
			{
                wtk_bf_update_w(bf, k);
            }
			wtk_bf_output_fft_k(bf, fft2, fftx+k, k);
            // fftx[k]=fft[0][k];
        }
        --beamnet2->bfflushnf;
		if(beamnet2->bfflushnf==0)
		{
			beamnet2->bfflushnf=bfflush_cnt;
        }
    }
	for(k=0; k<=clip_s; ++k)
	{
		fftx[k].a=fftx[k].b=0;
    }
	for(k=clip_e; k<nbin; ++k)
	{
		fftx[k].a=fftx[k].b=0;
    }
}

void wtk_beamnet2_control_bs(wtk_beamnet2_t *beamnet2, float *out, int len)
{
	float *bs_win=beamnet2->bs_win;
	float max_bs_out = beamnet2->cfg->max_bs_out;
	float out_max;
	int i;

	if(beamnet2->entropy_sil==0)
	{
		out_max=wtk_float_abs_max(out, len);
		if(out_max>max_bs_out)
		{
			beamnet2->bs_scale=max_bs_out/out_max;
			if(beamnet2->bs_scale<beamnet2->bs_last_scale)
			{
				beamnet2->bs_last_scale=beamnet2->bs_scale;
			}else
			{
				beamnet2->bs_scale=beamnet2->bs_last_scale;
			}
			beamnet2->bs_max_cnt=5;
		}
		if(bs_win){
			for(i=0; i<len/2; ++i)
			{
				out[i]*=beamnet2->bs_scale * bs_win[i] + beamnet2->bs_real_scale * (1.0-bs_win[i]);
			}
			for(i=len/2; i<len; ++i){
				out[i]*=beamnet2->bs_scale;
			}
			beamnet2->bs_real_scale = beamnet2->bs_scale;
		}else{
			for(i=0; i<len; ++i){
				out[i]*=beamnet2->bs_scale;
			}
		}
		if(beamnet2->bs_max_cnt>0)
		{
			--beamnet2->bs_max_cnt;
		}
		if(beamnet2->bs_max_cnt<=0 && beamnet2->bs_scale<1.0)
		{
			beamnet2->bs_scale*=1.1f;
			beamnet2->bs_last_scale=beamnet2->bs_scale;
			if(beamnet2->bs_scale>1.0)
			{
				beamnet2->bs_scale=1.0;
				beamnet2->bs_last_scale=1.0;
			}
		}
	}else
	{
		beamnet2->bs_scale=1.0;
		beamnet2->bs_last_scale=1.0;
		beamnet2->bs_max_cnt=0;
	}
} 

void wtk_beamnet2_feed(wtk_beamnet2_t *beamnet2, short *data, int len,
                        int is_end) {
    int i, j, n;
    int nmicchannel = beamnet2->cfg->nmicchannel;
    int nspchannel = beamnet2->cfg->nspchannel;
    int channel = beamnet2->cfg->channel;
	int *mic_channel=beamnet2->cfg->mic_channel;
	int *sp_channel=beamnet2->cfg->sp_channel;
    int wins = beamnet2->cfg->wins;
    int fsize = wins / 2;
    int length;
    wtk_drft_t *rfft = beamnet2->rfft;
    float *rfft_in = beamnet2->rfft_in;
    wtk_complex_t **fft = beamnet2->fft;
    wtk_complex_t **fft_sp = beamnet2->fft_sp;
    wtk_complex_t *fftx = beamnet2->fftx;
    float **analysis_mem = beamnet2->analysis_mem,
          **analysis_mem_sp = beamnet2->analysis_mem_sp;
	float *synthesis_mem=beamnet2->synthesis_mem;
    float *synthesis_window=beamnet2->synthesis_window;
    float *analysis_window = beamnet2->analysis_window;
    float *out = beamnet2->out;
    short *pv = (short *)out;
    wtk_strbuf_t **mic=beamnet2->mic;
    wtk_strbuf_t **sp=beamnet2->sp;
    float fv;
    float entropy=0;
    float entropy_thresh = beamnet2->cfg->entropy_thresh;
    int entropy_cnt = beamnet2->cfg->entropy_cnt;
    float entropy_scale;
    int nbin = beamnet2->nbin;
    wtk_complex_t *last_fftx = beamnet2->last_fftx;
    float *last_gf = beamnet2->last_gf;
    int delay_nf = beamnet2->cfg->delay_nf;
	int clip_s = beamnet2->cfg->clip_s;
	int clip_e = beamnet2->cfg->clip_e;
    float *mic_scale = beamnet2->cfg->mic_scale;
    float mean_gf = 1;
    float mean_gf_thresh = beamnet2->cfg->mean_gf_thresh;

	for(i=0;i<len;++i)
	{
		for(j=0; j<nmicchannel; ++j)
		{
            fv = WTK_WAV_SHORT_TO_FLOAT(data[mic_channel[j]]);
            fv *= mic_scale[j];
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
        ++beamnet2->nframe;
        for(i=0;i<nmicchannel;++i){
            wtk_drft_stft(rfft, rfft_in, analysis_mem[i], fft[i], (float *)(mic[i]->data), wins, analysis_window);
        }
        for(i=0;i<nspchannel;++i){
            wtk_drft_stft(rfft, rfft_in, analysis_mem_sp[i], fft_sp[i], (float *)(sp[i]->data), wins, analysis_window);
        }
        wtk_beamnet2_feed_decode(beamnet2, fft, fft_sp);

        if(beamnet2->gf){
            n = beamnet2->cfg->gf_channel;
            for(i=0;i<nbin;++i){
                beamnet2->gf[i] = (fftx[i].a*fftx[i].a+fftx[i].b*fftx[i].b)/(fft[n][i].a*fft[n][i].a+fft[n][i].b*fft[n][i].b+1e-12);
                beamnet2->gf[i] = max(min(beamnet2->gf[i],1.0),0.0);
                // printf("%f\n", beamnet2->gf[i]);
            }
            mean_gf = wtk_float_abs_mean(beamnet2->gf, nbin);
        }

        if(beamnet2->cfg->use_bf){
            wtk_beamnet2_feed_bf(beamnet2, fft);
        }

        if(entropy_thresh>0){
            entropy=wtk_beamnet2_entropy(beamnet2, fftx);
            if(entropy<entropy_thresh && mean_gf > mean_gf_thresh){
                ++beamnet2->entropy_in_cnt;
            }else{
                beamnet2->entropy_in_cnt = 0;
            }
            if(beamnet2->entropy_in_cnt>=beamnet2->cfg->entropy_in_cnt){
                beamnet2->entropy_sil = 0;
                beamnet2->entropy_silcnt = entropy_cnt;
            }else if(beamnet2->entropy_sil==0){
                beamnet2->entropy_silcnt -= 1;
                if(beamnet2->entropy_silcnt<=0){
                    beamnet2->entropy_sil = 1;
                }
            }
        }
        // printf("%f\n", entropy);
        if(beamnet2->cfg->delay_nf>0){
            memcpy(last_fftx, last_fftx+nbin, delay_nf*nbin*sizeof(wtk_complex_t));
            memcpy(last_fftx+delay_nf*nbin, fftx, nbin*sizeof(wtk_complex_t));
            if(beamnet2->last_gf){
                memcpy(last_gf, last_gf+nbin, delay_nf*nbin*sizeof(float));
                memcpy(last_gf+delay_nf*nbin, beamnet2->gf, nbin*sizeof(float));
            }

            if(is_end){
                while(beamnet2->delay_nf<beamnet2->cfg->delay_nf){
                    memcpy(fftx, last_fftx, nbin*sizeof(wtk_complex_t));
                    if(beamnet2->last_gf){
                        memcpy(beamnet2->gf, last_gf, nbin*sizeof(float));
                    }
                    if(entropy>entropy_thresh && beamnet2->entropy_sil==1){
                        if(beamnet2->entropy_silcnt > 0){
                            entropy_scale = powf(beamnet2->entropy_silcnt * 1.0/entropy_cnt, beamnet2->cfg->entropy_ratio)+beamnet2->cfg->entropy_min_scale;
                        }else{
                            entropy_scale = powf(1.0/entropy_cnt, beamnet2->cfg->entropy_ratio)+beamnet2->cfg->entropy_min_scale;
                        }
                        entropy_scale = min(entropy_scale, 1.0);
                        for(i=0;i<nbin;++i){
                            fftx[i].a*=entropy_scale;
                            fftx[i].b*=entropy_scale;
                        }
                    }
                    if(beamnet2->qmmse){
                        // wtk_qmmse_denoise(beamnet2->qmmse, fftx);
                        wtk_qmmse_feed_mask(beamnet2->qmmse, fftx, beamnet2->gf);
                    }

                    // memcpy(fftx, fft[0], nbin*sizeof(wtk_complex_t));
                    wtk_drft_istft(rfft, rfft_in, synthesis_mem, fftx, out, wins, synthesis_window);

                    for(i=0;i<fsize;++i){
                        pv[i] = WTK_WAV_FLOAT_TO_SHORT(out[i]);
                    }
                    if(beamnet2->notify){
                        beamnet2->notify(beamnet2->ths, pv, fsize);
                    }
                    ++beamnet2->delay_nf;
                }
                if(is_end && length>0){
                    if(beamnet2->notify)
                    {
                        pv=(short *)mic[0]->data;
                        beamnet2->notify(beamnet2->ths,pv,length);
                    }
                }
            }else{
                if(beamnet2->delay_nf>0){
                    --beamnet2->delay_nf;
                    wtk_strbufs_pop(mic, nmicchannel, fsize*sizeof(float));
                    wtk_strbufs_pop(sp, nspchannel, fsize*sizeof(float));
                    length = mic[0]->pos/sizeof(float);
                    continue;
                }else{
                    memcpy(fftx, last_fftx, nbin*sizeof(wtk_complex_t));
                }
            }
        }
        if(entropy>entropy_thresh && beamnet2->entropy_sil==1){
            if(beamnet2->entropy_silcnt > 0){
                entropy_scale = powf(beamnet2->entropy_silcnt * 1.0/entropy_cnt, beamnet2->cfg->entropy_ratio)+beamnet2->cfg->entropy_min_scale;
            }else{
                entropy_scale = powf(1.0/entropy_cnt, beamnet2->cfg->entropy_ratio)+beamnet2->cfg->entropy_min_scale;
            }
            entropy_scale = min(entropy_scale, 1.0);
            for(i=0;i<nbin;++i){
                fftx[i].a*=entropy_scale;
                fftx[i].b*=entropy_scale;
            }
        }

        float gc_mask = wtk_float_abs_mean(beamnet2->gf, nbin);
        // printf("%f\n", gc_mask);
        if(gc_mask > beamnet2->cfg->gc_min_thresh){
			beamnet2->gc_cnt = beamnet2->cfg->gc_cnt;
		}else{
			--beamnet2->gc_cnt;
		}

        if(beamnet2->gc_cnt>=0){
            if(entropy_thresh>0){
                if(beamnet2->entropy_sil==1){
                    if(beamnet2->gf){
                        for(i=0;i<nbin;++i){
                            beamnet2->gf[i] = 0;
                        }
                    }else{
                        memset(fftx, 0, nbin*sizeof(wtk_complex_t));
                    }
                }else if(beamnet2->gc){
                    qtk_gain_controller_run(beamnet2->gc, fftx, fsize, NULL,gc_mask);
                }
            }
        }else{
            memset(fftx, 0, sizeof(wtk_complex_t)*nbin);
        }

        if(beamnet2->qmmse){
            // wtk_qmmse_denoise(beamnet2->qmmse, fftx);
            wtk_qmmse_feed_mask(beamnet2->qmmse, fftx, beamnet2->gf);
        }

    	for(i=0; i<=clip_s; ++i)
        {
            fftx[i].a=fftx[i].b=0;
        }
        for(i=clip_e; i<nbin; ++i)
        {
            fftx[i].a=fftx[i].b=0;
        }

        // memcpy(fftx, fft[0], nbin*sizeof(wtk_complex_t));
        wtk_drft_istft(rfft, rfft_in, synthesis_mem, fftx, out, wins, synthesis_window);

        for(i=0;i<fsize;++i){
            out[i] *= 32768.0;
        }

		wtk_beamnet2_control_bs(beamnet2, out, fsize);

        for(i=0;i<fsize;++i){
            // pv[i] = WTK_WAV_FLOAT_TO_SHORT(out[i]);
            pv[i] = floorf(out[i]+0.5);
        }
        if(beamnet2->notify){
            beamnet2->notify(beamnet2->ths, pv, fsize);
        }
        wtk_strbufs_pop(mic, nmicchannel, fsize*sizeof(float));
        wtk_strbufs_pop(sp, nspchannel, fsize*sizeof(float));
        length = mic[0]->pos/sizeof(float);
    }
    if(is_end && length>0){
        if(beamnet2->notify)
        {
            pv=(short *)mic[0]->data;
            beamnet2->notify(beamnet2->ths,pv,length);
        }
    }
}
