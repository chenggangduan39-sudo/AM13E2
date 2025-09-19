#include "wtk/bfio/qform/beamnet/wtk_beamnet4.h"
#include "qtk/math/qtk_vector.h"
#ifndef WTK_WAV_SHORT_TO_FLOAT
#define WTK_WAV_SHORT_TO_FLOAT(f) ((f) > 0? (f/32767.0) : (f/32768.0))
#endif
#ifndef WTK_WAV_FLOAT_TO_SHORT
#define WTK_WAV_FLOAT_TO_SHORT(f) ((f) > 0? floorf(f*32767.0+0.5) : floorf(f*32768.0+0.5))
#endif

void wtk_beamnet4_compute_channel_covariance(wtk_beamnet4_t *beamnet4, wtk_complex_t **fft, wtk_complex_t **covar_mat, int norm, int array)
{
    int nmicchannel = beamnet4->cfg->nmicchannel;
    int nbin = beamnet4->nbin;
    int i, j, k;
    wtk_complex_t *mean_mat = beamnet4->mean_mat;
    wtk_complex_t **input_norm = beamnet4->input_norm;
    int idx;

    if(array != -1){
        nmicchannel = beamnet4->cfg->nmicchannels[array];
    }

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

void wtk_beamnet4_compute_multi_channel_phase_shift(wtk_beamnet4_t *beamnet4, wtk_complex_t **phase_shift, float theta, float phi, int array)
{
    int wins = beamnet4->cfg->wins;
    int nmicchannel = beamnet4->cfg->nmicchannels[array];
    int nbin = beamnet4->nbin;
    int rate = beamnet4->cfg->rate;
    float sv = beamnet4->cfg->sv;
    float **mic_pos = beamnet4->cfg->mic_pos[array];
    float *time_delay = beamnet4->time_delay;
    float *mic;
    float x, y, z;
    int i, k;
    float t;

    theta -= 180.0;  //// 
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
        // printf("%d %d %f %f %f %f\n", array, i, x, y, z, time_delay[i]);
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

void wtk_beamnet4_compute_phase_shift(wtk_beamnet4_t *beamnet4, wtk_complex_t *phase_shift)
{
    int wins = beamnet4->cfg->wins;
    int nbin = beamnet4->nbin;
    int rate = beamnet4->cfg->rate;
    int k;
    float t;
    float time_delay = (beamnet4->cfg->target_delay[4] - beamnet4->cfg->target_delay[0]) / (1.0*rate);

	for(k=0;k<nbin;++k)
	{
		t=2*PI*rate*1.0/wins*k;
        phase_shift[k].a=cos(t*time_delay);
        phase_shift[k].b=-sin(t*time_delay);
    }
}

wtk_beamnet4_t *wtk_beamnet4_new(wtk_beamnet4_cfg_t *cfg) {
    wtk_beamnet4_t *beamnet4;
    int i;

    beamnet4 = (wtk_beamnet4_t *)wtk_malloc(sizeof(wtk_beamnet4_t));
    beamnet4->cfg = cfg;
    beamnet4->ths = NULL;
    beamnet4->notify = NULL;
    beamnet4->mic = wtk_strbufs_new(beamnet4->cfg->nmicchannel);

    beamnet4->nbin = cfg->wins / 2 + 1;
    beamnet4->analysis_window = wtk_malloc(sizeof(float) * cfg->wins);  /// 2);
    beamnet4->synthesis_window = wtk_malloc(sizeof(float) * cfg->wins); /// 2);
    beamnet4->analysis_mem = wtk_float_new_p2(cfg->nmicchannel, beamnet4->nbin - 1);
    beamnet4->synthesis_mem = wtk_malloc(sizeof(float) * (beamnet4->nbin - 1));
    beamnet4->rfft = wtk_drft_new(cfg->wins);
    beamnet4->rfft_in = (float *)wtk_malloc(sizeof(float) * (cfg->wins));

    beamnet4->fft = wtk_complex_new_p2(cfg->nmicchannel, beamnet4->nbin);
    beamnet4->fftx = (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * beamnet4->nbin);

    beamnet4->out = wtk_malloc(sizeof(float) * (beamnet4->nbin - 1));

    beamnet4->speech_est_real=NULL;
    beamnet4->speech_est_imag=NULL;
    beamnet4->noise_est_real=NULL;
    beamnet4->noise_est_imag=NULL;
    beamnet4->beamformer_x=NULL;
    beamnet4->src_est_real=NULL;
    beamnet4->src_est_imag=NULL;
#ifdef ONNX_DEC
    beamnet4->seperator = NULL;
    beamnet4->beamformer = NULL;
    beamnet4->sep_caches = NULL;
    beamnet4->beam_caches = NULL;
    beamnet4->seperator_out_len = NULL;
    beamnet4->beamformer_out_len = NULL;
    if(cfg->use_onnx){
        beamnet4->seperator = qtk_onnxruntime_new(&(cfg->seperator));
        beamnet4->sep_caches = wtk_calloc(sizeof(OrtValue *), beamnet4->seperator->num_in - cfg->seperator.outer_in_num);
        if (beamnet4->seperator->num_in - cfg->seperator.outer_in_num != beamnet4->seperator->num_out - cfg->seperator.outer_out_num) {
            wtk_debug("err inner_item\n");
            exit(0);
        }
        beamnet4->seperator_out_len = (int *)wtk_malloc(sizeof(int) * (cfg->seperator.outer_out_num));
        beamnet4->beamformer = qtk_onnxruntime_new(&(cfg->beamformer));
        beamnet4->beam_caches = wtk_calloc(sizeof(OrtValue *), beamnet4->beamformer->num_in - cfg->beamformer.outer_in_num);
        if (beamnet4->beamformer->num_in - cfg->beamformer.outer_in_num != beamnet4->beamformer->num_out - cfg->beamformer.outer_out_num) {
            wtk_debug("err inner_item\n");
            exit(0);
        }
        beamnet4->beamformer_out_len = (int *)wtk_malloc(sizeof(int) * (cfg->seperator.outer_out_num));
    }
#endif
    beamnet4->local_covar = (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * beamnet4->nbin);
    beamnet4->cross_covar = (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * beamnet4->nbin);
    beamnet4->cross_ideal_phase_shift = (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * beamnet4->nbin);
    beamnet4->feature = wtk_float_new_p2(cfg->feature_len, beamnet4->nbin);
    beamnet4->x = (float *)wtk_malloc(sizeof(float) * cfg->feature_len * beamnet4->nbin);
    beamnet4->mix_spec = (float *)wtk_malloc(sizeof(float) * (cfg->nmicchannel) * 2 * beamnet4->nbin);
    beamnet4->time_delay = (float *)wtk_malloc(sizeof(float) * beamnet4->nbin);
    beamnet4->mic_aptd = wtk_float_new_p2(cfg->narray, beamnet4->nbin);
    beamnet4->mic_covar = wtk_complex_new_p2(cfg->covar_channels, beamnet4->nbin);
    beamnet4->ideal_phase_covar = (wtk_complex_t ***)wtk_malloc(sizeof(wtk_complex_t **)*cfg->narray);
    for(i=0;i<cfg->narray;++i){
        beamnet4->ideal_phase_covar[i] = wtk_complex_new_p2(cfg->out_channels[i], beamnet4->nbin);
    }
    beamnet4->ideal_phase_shift = (wtk_complex_t ***)wtk_malloc(sizeof(wtk_complex_t **)*cfg->narray);
    for(i=0;i<cfg->narray;++i){
        beamnet4->ideal_phase_shift[i] = wtk_complex_new_p2(cfg->nmicchannels[i], beamnet4->nbin);
    }
    beamnet4->freq_covar = wtk_complex_new_p2(cfg->out_channel, beamnet4->nbin);
    beamnet4->freq_covar_sum = (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * beamnet4->nbin);
    beamnet4->LPS = wtk_float_new_p2(cfg->narray, beamnet4->nbin);
    beamnet4->IPDs = wtk_float_new_p2(cfg->covar_channels, beamnet4->nbin);
    beamnet4->FDDF = (float *)wtk_malloc(sizeof(float) * beamnet4->nbin);
    beamnet4->mean_mat = (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * beamnet4->nbin);
    beamnet4->input_norm = wtk_complex_new_p2(cfg->nmicchannel, beamnet4->nbin);
    beamnet4->speech_covar = wtk_complex_new_p2(beamnet4->nbin, cfg->nmicchannel*cfg->nmicchannel);
    beamnet4->noise_covar = wtk_complex_new_p2(beamnet4->nbin, cfg->nmicchannel*cfg->nmicchannel);

    beamnet4->mix_aptd = NULL;
    beamnet4->speech_aptd = NULL;
    beamnet4->noise_aptd = NULL;
    if(cfg->bf_feature_type==1){
        beamnet4->mix_aptd = (float *)wtk_malloc(sizeof(float) * beamnet4->nbin);
        beamnet4->speech_aptd = (float *)wtk_malloc(sizeof(float) * beamnet4->nbin);
        beamnet4->noise_aptd = (float *)wtk_malloc(sizeof(float) * beamnet4->nbin);
    }

    beamnet4->qmmse=NULL;
    if(cfg->use_qmmse){
		beamnet4->qmmse=wtk_qmmse_new(&(cfg->qmmse));
    }

    beamnet4->entropy_E=NULL;
    beamnet4->entropy_Eb=NULL;
    beamnet4->last_fftx=NULL;
    if(cfg->entropy_thresh>0){
        beamnet4->entropy_E=(float *)wtk_malloc(sizeof(float)*beamnet4->nbin);
        beamnet4->entropy_Eb=(float *)wtk_malloc(sizeof(float)*cfg->wins);
        if(cfg->delay_nf>0){
            beamnet4->last_fftx=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*beamnet4->nbin*(cfg->delay_nf+1));
        }
    }

    beamnet4->gf=(float *)wtk_malloc(sizeof(float)*beamnet4->nbin);
    beamnet4->last_gf=NULL;
    if((cfg->use_qmmse && cfg->qmmse.use_agc_mask) || cfg->use_bf){
        if(cfg->delay_nf>0){
            beamnet4->last_gf=(float *)wtk_malloc(sizeof(float)*beamnet4->nbin*(cfg->delay_nf+1));
        }
    }
    beamnet4->bf=NULL;
	beamnet4->covm=NULL;
    if(cfg->use_bf){
        beamnet4->bf = wtk_bf_new(&(cfg->bf), cfg->wins);
        beamnet4->covm = wtk_covm_new(&(cfg->covm), beamnet4->nbin, cfg->nbfchannel);
    }

	beamnet4->sim_scov=NULL;
	beamnet4->sim_ncov=NULL;
	beamnet4->sim_cnt_sum=NULL;
	if(cfg->use_sim_bf){
		beamnet4->sim_scov = (float *)wtk_malloc(sizeof(float)*beamnet4->nbin);
		beamnet4->sim_ncov = (float *)wtk_malloc(sizeof(float)*beamnet4->nbin);
		beamnet4->sim_cnt_sum = (int *)wtk_malloc(sizeof(int)*beamnet4->nbin);
    }

    beamnet4->gc = NULL;
	if(cfg->use_gc){
		beamnet4->gc=qtk_gain_controller_new(&(cfg->gc));
		qtk_gain_controller_set_mode(beamnet4->gc,0);
		beamnet4->gc->kalman.Z_k = cfg->gc_gain * 1.0/1000.0;
	}

	beamnet4->bs_win=NULL;
	if(cfg->use_bs_win)
	{
		beamnet4->bs_win=wtk_math_create_hanning_window2(cfg->wins/2);
	}

    wtk_beamnet4_reset(beamnet4);

    return beamnet4;
}

void wtk_beamnet4_delete(wtk_beamnet4_t *beamnet4) {
    int narray = beamnet4->cfg->narray;
    int i;

    wtk_strbufs_delete(beamnet4->mic, beamnet4->cfg->nmicchannel);

    wtk_free(beamnet4->analysis_window);
    wtk_free(beamnet4->synthesis_window);
    wtk_float_delete_p2(beamnet4->analysis_mem, beamnet4->cfg->nmicchannel);
    wtk_free(beamnet4->synthesis_mem);
    wtk_free(beamnet4->rfft_in);
    wtk_drft_delete(beamnet4->rfft);
    wtk_complex_delete_p2(beamnet4->fft, beamnet4->cfg->nmicchannel);

    wtk_free(beamnet4->fftx);

    wtk_free(beamnet4->out);
#ifdef ONNX_DEC
    if(beamnet4->cfg->use_onnx){
        {
            int n = beamnet4->seperator->num_in - beamnet4->seperator->cfg->outer_in_num;
            if (beamnet4->sep_caches[0]) {
                int i;
                for (i = 0; i < n; i++) {
                    beamnet4->seperator->api->ReleaseValue(beamnet4->sep_caches[i]);
                }
            }
        }
        if (beamnet4->seperator) {
            qtk_onnxruntime_delete(beamnet4->seperator);
        }
        wtk_free(beamnet4->seperator_out_len);
        wtk_free(beamnet4->sep_caches);
        {
            int n = beamnet4->beamformer->num_in - beamnet4->beamformer->cfg->outer_in_num;
            if (beamnet4->beam_caches[0]) {
                int i;
                for (i = 0; i < n; i++) {
                    beamnet4->beamformer->api->ReleaseValue(beamnet4->beam_caches[i]);
                }
            }
        }
        if (beamnet4->beamformer) {
            qtk_onnxruntime_delete(beamnet4->beamformer);
        }
        wtk_free(beamnet4->beamformer_out_len);
        wtk_free(beamnet4->beam_caches);
    }
#endif
    wtk_free(beamnet4->local_covar);
    wtk_free(beamnet4->cross_covar);
    wtk_free(beamnet4->cross_ideal_phase_shift);
    wtk_float_delete_p2(beamnet4->feature, beamnet4->cfg->feature_len);
    wtk_free(beamnet4->x);
    wtk_free(beamnet4->mix_spec);
    wtk_free(beamnet4->time_delay);
    wtk_float_delete_p2(beamnet4->mic_aptd, beamnet4->cfg->narray);
    wtk_complex_delete_p2(beamnet4->mic_covar, beamnet4->cfg->covar_channels);
    for(i=0;i<narray;++i){
        wtk_complex_delete_p2(beamnet4->ideal_phase_covar[i], beamnet4->cfg->out_channels[i]);
    }
    wtk_free(beamnet4->ideal_phase_covar);
    for(i=0;i<narray;++i){
        wtk_complex_delete_p2(beamnet4->ideal_phase_shift[i], beamnet4->cfg->nmicchannels[i]);
    }
    wtk_free(beamnet4->ideal_phase_shift);
    wtk_complex_delete_p2(beamnet4->freq_covar, beamnet4->cfg->out_channel);
    wtk_free(beamnet4->freq_covar_sum);
    wtk_float_delete_p2(beamnet4->LPS, beamnet4->cfg->narray);
    wtk_float_delete_p2(beamnet4->IPDs, beamnet4->cfg->covar_channels);
    wtk_free(beamnet4->FDDF);
    wtk_free(beamnet4->mean_mat);
    wtk_complex_delete_p2(beamnet4->input_norm, beamnet4->cfg->nmicchannel);
    wtk_complex_delete_p2(beamnet4->speech_covar, beamnet4->nbin);
    wtk_complex_delete_p2(beamnet4->noise_covar, beamnet4->nbin);
    if(beamnet4->mix_aptd){
        wtk_free(beamnet4->mix_aptd);
    }
    if(beamnet4->speech_aptd){
        wtk_free(beamnet4->speech_aptd);
    }
    if(beamnet4->noise_aptd){
        wtk_free(beamnet4->noise_aptd);
    }

    if(beamnet4->speech_est_real){
        wtk_free(beamnet4->speech_est_real);
    }
    if(beamnet4->speech_est_imag){
        wtk_free(beamnet4->speech_est_imag);
    }
    if(beamnet4->noise_est_real){
        wtk_free(beamnet4->noise_est_real);
    }
    if(beamnet4->noise_est_imag){
        wtk_free(beamnet4->noise_est_imag);
    }
    if(beamnet4->beamformer_x){
        wtk_free(beamnet4->beamformer_x);
    }
    if(beamnet4->src_est_real){
        wtk_free(beamnet4->src_est_real);
    }
    if(beamnet4->src_est_imag){
        wtk_free(beamnet4->src_est_imag);
    }

    if(beamnet4->qmmse){
        wtk_qmmse_delete(beamnet4->qmmse);
    }
    if(beamnet4->entropy_E){
        wtk_free(beamnet4->entropy_E);
    }
    if(beamnet4->entropy_Eb){
        wtk_free(beamnet4->entropy_Eb);
    }
    if(beamnet4->last_fftx){
        wtk_free(beamnet4->last_fftx);
    }
    if(beamnet4->gf){
        wtk_free(beamnet4->gf);
    }
    if(beamnet4->last_gf){
        wtk_free(beamnet4->last_gf);
    }
    if(beamnet4->bf){
        wtk_bf_delete(beamnet4->bf);
    }
    if(beamnet4->covm){
        wtk_covm_delete(beamnet4->covm);
    }
    if(beamnet4->sim_scov)
	{
        wtk_free(beamnet4->sim_scov);
        wtk_free(beamnet4->sim_ncov);
        wtk_free(beamnet4->sim_cnt_sum);
    }
	if(beamnet4->gc){
		qtk_gain_controller_delete(beamnet4->gc);
	}
	if(beamnet4->bs_win)
	{
		wtk_free(beamnet4->bs_win);
	}

    wtk_free(beamnet4);
}

void wtk_beamnet4_start(wtk_beamnet4_t *beamnet4) {
    wtk_complex_t *cross_ideal_phase_shift = beamnet4->cross_ideal_phase_shift;
    wtk_complex_t ***ideal_phase_shift = beamnet4->ideal_phase_shift;
    wtk_complex_t ***ideal_phase_covar = beamnet4->ideal_phase_covar;
    float *target_theta = beamnet4->cfg->target_theta;
    int narray=beamnet4->cfg->narray;
    int i;

    for(i=0;i<narray-1;++i){
        wtk_beamnet4_compute_phase_shift(beamnet4, cross_ideal_phase_shift);
    }
    for(i=0;i<narray;++i){
        wtk_beamnet4_compute_multi_channel_phase_shift(beamnet4, ideal_phase_shift[i], target_theta[i], 0, i);
        wtk_beamnet4_compute_channel_covariance(beamnet4, ideal_phase_shift[i], ideal_phase_covar[i], beamnet4->cfg->norm_channel, i);
    }
    if(beamnet4->cfg->use_bf){
        wtk_bf_update_ovec(beamnet4->bf,0,0);
        wtk_bf_init_w(beamnet4->bf);
    }
}
void wtk_beamnet4_reset(wtk_beamnet4_t *beamnet4) {
    int wins = beamnet4->cfg->wins;
    int narray = beamnet4->cfg->narray;
    int i;

    wtk_strbufs_reset(beamnet4->mic, beamnet4->cfg->nmicchannel);

    for (i = 0; i < wins; ++i) {
        beamnet4->analysis_window[i] = sin((0.5 + i) * PI / (wins));
    }
    wtk_drft_init_synthesis_window(beamnet4->synthesis_window,
                                   beamnet4->analysis_window, wins);

    wtk_float_zero_p2(beamnet4->analysis_mem, beamnet4->cfg->nmicchannel,
                      (beamnet4->nbin - 1));
    memset(beamnet4->synthesis_mem, 0, sizeof(float) * (beamnet4->nbin - 1));

    wtk_complex_zero_p2(beamnet4->fft, beamnet4->cfg->nmicchannel,
                        (beamnet4->nbin));
    memset(beamnet4->fftx, 0, sizeof(wtk_complex_t) * (beamnet4->nbin));
#ifdef ONNX_DEC
    if(beamnet4->cfg->use_onnx){
        qtk_onnxruntime_reset(beamnet4->seperator);
        {
            int n = beamnet4->seperator->num_in - beamnet4->seperator->cfg->outer_in_num;
            if (beamnet4->sep_caches[0]) {
                int i;
                for (i = 0; i < n; i++) {
                    beamnet4->seperator->api->ReleaseValue(beamnet4->sep_caches[i]);
                }
                memset(beamnet4->sep_caches, 0, sizeof(OrtValue *) * n);
            }
        }
        memset(beamnet4->seperator_out_len, 0, sizeof(int) * (beamnet4->seperator->cfg->outer_out_num));
        qtk_onnxruntime_reset(beamnet4->beamformer);
        {
            int n = beamnet4->beamformer->num_in - beamnet4->beamformer->cfg->outer_in_num;
            if (beamnet4->beam_caches[0]) {
                int i;
                for (i = 0; i < n; i++) {
                    beamnet4->beamformer->api->ReleaseValue(beamnet4->beam_caches[i]);
                }
                memset(beamnet4->beam_caches, 0, sizeof(OrtValue *) * n);
            }
        }
        memset(beamnet4->beamformer_out_len, 0, sizeof(int) * (beamnet4->beamformer->cfg->outer_out_num));
    }
#endif
    memset(beamnet4->local_covar, 0, sizeof(wtk_complex_t) * (beamnet4->nbin));
    memset(beamnet4->cross_covar, 0, sizeof(wtk_complex_t) * (beamnet4->nbin));
    memset(beamnet4->cross_ideal_phase_shift, 0, sizeof(wtk_complex_t) * (beamnet4->nbin));
    wtk_float_zero_p2(beamnet4->feature, beamnet4->cfg->feature_len,
                      (beamnet4->nbin));
    memset(beamnet4->x, 0, sizeof(float) * beamnet4->cfg->feature_len * beamnet4->nbin);
    memset(beamnet4->mix_spec, 0, sizeof(float) * (beamnet4->cfg->nmicchannel) * 2 * beamnet4->nbin);
    memset(beamnet4->time_delay, 0, sizeof(float) * beamnet4->nbin);
    wtk_float_zero_p2(beamnet4->mic_aptd, beamnet4->cfg->narray, beamnet4->nbin);
    wtk_complex_zero_p2(beamnet4->mic_covar, beamnet4->cfg->covar_channels, beamnet4->nbin);
    for(i=0;i<narray;++i){
        wtk_complex_zero_p2(beamnet4->ideal_phase_covar[i], beamnet4->cfg->out_channels[i], beamnet4->nbin);
    }
    for(i=0;i<narray;++i){
        wtk_complex_zero_p2(beamnet4->ideal_phase_shift[i], beamnet4->cfg->nmicchannels[i], beamnet4->nbin);
    }
    wtk_complex_zero_p2(beamnet4->freq_covar, beamnet4->cfg->out_channel,
                        (beamnet4->nbin));
    memset(beamnet4->freq_covar_sum, 0, sizeof(float) * beamnet4->nbin);
    wtk_float_zero_p2(beamnet4->LPS, beamnet4->cfg->narray, beamnet4->nbin);
    wtk_float_zero_p2(beamnet4->IPDs, beamnet4->cfg->covar_channels, beamnet4->nbin);
    memset(beamnet4->FDDF, 0, sizeof(float) * beamnet4->nbin);
    memset(beamnet4->mean_mat, 0, sizeof(wtk_complex_t) * beamnet4->nbin);
    wtk_complex_zero_p2(beamnet4->input_norm, beamnet4->cfg->nmicchannel,(beamnet4->nbin));
    wtk_complex_zero_p2(beamnet4->speech_covar, beamnet4->nbin,beamnet4->cfg->nmicchannel*beamnet4->cfg->nmicchannel);
    wtk_complex_zero_p2(beamnet4->noise_covar, beamnet4->nbin,beamnet4->cfg->nmicchannel*beamnet4->cfg->nmicchannel);

    if(beamnet4->mix_aptd){
        memset(beamnet4->mix_aptd, 0, sizeof(float) * beamnet4->nbin);
    }
    if(beamnet4->speech_aptd){
        memset(beamnet4->speech_aptd, 0, sizeof(float) * beamnet4->nbin);
    }
    if(beamnet4->noise_aptd){
        memset(beamnet4->noise_aptd, 0, sizeof(float) * beamnet4->nbin);
    }

    if(beamnet4->qmmse){
        wtk_qmmse_reset(beamnet4->qmmse);
    }
    if(beamnet4->entropy_E){
        memset(beamnet4->entropy_E, 0, sizeof(float) * beamnet4->nbin);
    }
    if(beamnet4->entropy_Eb){
        memset(beamnet4->entropy_Eb, 0, sizeof(float) * beamnet4->nbin);
    }
    if(beamnet4->last_fftx){
        memset(beamnet4->last_fftx, 0, sizeof(wtk_complex_t) * (beamnet4->nbin)*(beamnet4->cfg->delay_nf+1));
    }
    if(beamnet4->gf){
        memset(beamnet4->gf, 0, sizeof(float) * beamnet4->nbin);
    }
    if(beamnet4->last_gf){
        memset(beamnet4->last_gf, 0, sizeof(float) * beamnet4->nbin);
    }
    if(beamnet4->bf){
        wtk_bf_reset(beamnet4->bf);
    }
    if(beamnet4->covm){
        wtk_covm_reset(beamnet4->covm);
    }
    if(beamnet4->sim_scov)
	{
		memset(beamnet4->sim_scov, 0, sizeof(float)*beamnet4->nbin);
		memset(beamnet4->sim_ncov, 0, sizeof(float)*beamnet4->nbin);
		memset(beamnet4->sim_cnt_sum, 0, sizeof(int)*beamnet4->nbin);
    }

    if(beamnet4->gc){
        qtk_gain_controller_reset(beamnet4->gc);
    }

    beamnet4->entropy_in_cnt = 0;
    beamnet4->entropy_silcnt = 0;
    beamnet4->entropy_sil = 1;

    beamnet4->delay_nf = beamnet4->cfg->delay_nf;

    beamnet4->nframe = 0;
	beamnet4->bfflushnf=beamnet4->cfg->bfflush_cnt;
    
	beamnet4->bs_scale=1.0;
	beamnet4->bs_last_scale=1.0;
	beamnet4->bs_real_scale=1.0;
	beamnet4->bs_max_cnt=0;
	beamnet4->gc_cnt=0;
}

void wtk_beamnet4_set_notify(wtk_beamnet4_t *beamnet4, void *ths,
                             wtk_beamnet4_notify_f notify) {
    beamnet4->notify = notify;
    beamnet4->ths = ths;
}

void wtk_beamnet4_compute_feature(wtk_beamnet4_t * beamnet4, wtk_complex_t **fft, float **feature, int feature_type)
{
    int nbin = beamnet4->nbin;
    float **mic_aptd = beamnet4->mic_aptd;
    wtk_complex_t **mic_covar = beamnet4->mic_covar;
    wtk_complex_t ***ideal_phase_covar = beamnet4->ideal_phase_covar;
    wtk_complex_t **freq_covar = beamnet4->freq_covar;
    wtk_complex_t *freq_covar_sum = beamnet4->freq_covar_sum;
    wtk_complex_t *local_covar = beamnet4->local_covar;
    wtk_complex_t *cross_covar = beamnet4->cross_covar;
    wtk_complex_t *cross_ideal_phase_shift = beamnet4->cross_ideal_phase_shift;
    float **LPS = beamnet4->LPS;
    float **IPDs = beamnet4->IPDs;
    float *FDDF = beamnet4->FDDF;
    int *nmicchannels = beamnet4->cfg->nmicchannels;
    int narray = beamnet4->cfg->narray;
    int *out_channels = beamnet4->cfg->out_channels;
    int out_channel = beamnet4->cfg->out_channel;
    int covar_channels = beamnet4->cfg->covar_channels;
    float scaler = beamnet4->cfg->scaler;
    float tmp;
    float eps = 1.1920928955078125e-07;
    int i, j, k, n, m;

    // FILE *fp = fopen("/home/lixiao/tmp_wav/mic_spec.bin", "rb");
    // float *tmp_mic_spec = (float *)malloc(sizeof(float) * nmicchannel * 2 * nbin);
    // if(fp){
    //     fread(tmp_mic_spec, sizeof(float), nmicchannel * 2 * nbin, fp);
    // }
    // for(i=0;i<nmicchannel;++i){
    //     for(k=0;k<nbin;++k){
    //         fft[i][k].a = tmp_mic_spec[2*k+i*nbin*2];
    //         fft[i][k].b = tmp_mic_spec[2*k+i*nbin*2+1];
    //         // printf("%d %d %f %f\n", i, k, fft[i][k].a, fft[i][k].b);
    //     }
    // }

    wtk_beamnet4_compute_channel_covariance(beamnet4, fft, mic_covar, beamnet4->cfg->norm_channel, -1);

    for(k=0;k<nbin;++k){
        local_covar[k].a = fft[nmicchannels[0]][k].a * fft[0][k].a + fft[nmicchannels[0]][k].b * fft[0][k].b;
        local_covar[k].b = fft[nmicchannels[0]][k].b * fft[0][k].a - fft[nmicchannels[0]][k].a * fft[0][k].b;
    }
    for(k=0;k<nbin;++k){
        cross_covar[k].a = local_covar[k].a * cross_ideal_phase_shift[k].a + local_covar[k].b * cross_ideal_phase_shift[k].b;
        cross_covar[k].b = local_covar[k].b * cross_ideal_phase_shift[k].a - local_covar[k].a * cross_ideal_phase_shift[k].b;
    }

    n = 0;
    for(i=0;i<narray;++i){
        for(k=0;k<nbin;++k){
            mic_aptd[i][k] = sqrtf(fft[n][k].a * fft[n][k].a + fft[n][k].b * fft[n][k].b) + eps;
            LPS[i][k] = 20.0 * log10f(mic_aptd[i][k]);
        }
        n += nmicchannels[i];
    }

    if(feature_type==0 || feature_type==1 || feature_type==2 || feature_type==3 || feature_type == 4 || feature_type == 5){
        if(scaler!=1.0){
            for(k=0;k<nbin;++k){
                for(i=0;i<covar_channels;++i){
                    mic_covar[i][k].a *= scaler;
                    mic_covar[i][k].b *= scaler;
                }
            }
        }

        for(k=0;k<nbin;++k){
            n = 0;
            m = 0;
            for(i=0;i<narray;++i){
                for(j=0;j<out_channels[i];++j){
                    freq_covar[n+j][k].a = mic_covar[n+m*nmicchannels[i]+j][k].a * ideal_phase_covar[i][j][k].a + mic_covar[n+m*nmicchannels[i]+j][k].b * ideal_phase_covar[i][j][k].b;
                    freq_covar[n+j][k].b = mic_covar[n+m*nmicchannels[i]+j][k].b * ideal_phase_covar[i][j][k].a - mic_covar[n+m*nmicchannels[i]+j][k].a * ideal_phase_covar[i][j][k].b;
                }
                n += out_channels[i];
                m += nmicchannels[i];
            }
        }

        if(feature_type==0){
            memset(FDDF, 0, sizeof(float) * nbin);
            for(k=0;k<nbin;++k){
                for(i=0;i<covar_channels;++i){
                    IPDs[i][k] = atan2f(mic_covar[i][k].b, mic_covar[i][k].a);
                }
                for(i=0;i<out_channel;++i){
                    FDDF[k] += atan2f(freq_covar[i][k].b, freq_covar[i][k].a);
                }
                for(i=0;i<narray;++i){
                    feature[i][k] = LPS[i][k];
                }
                for(i=0;i<covar_channels;++i){
                    feature[i+narray][k] = IPDs[i][k];
                }
                feature[narray+covar_channels][k] = FDDF[k];
            }
        }else if(feature_type==1 || feature_type==2 || feature_type==5){
            for(k=0;k<nbin;++k){
                for(i=0;i<covar_channels;++i){
                    tmp = sqrtf(mic_covar[i][k].a * mic_covar[i][k].a + mic_covar[i][k].b * mic_covar[i][k].b)+eps;
                    mic_covar[i][k].a = mic_covar[i][k].a / tmp;
                    mic_covar[i][k].b = mic_covar[i][k].b / tmp;
                }
            }
            memset(freq_covar_sum, 0, sizeof(wtk_complex_t) * nbin);
            for(k=0;k<nbin;++k){
                for(i=0;i<out_channel;++i){
                    freq_covar_sum[k].a += freq_covar[i][k].a;
                    freq_covar_sum[k].b += freq_covar[i][k].b;
                }
                tmp = sqrtf(freq_covar_sum[k].a * freq_covar_sum[k].a + freq_covar_sum[k].b * freq_covar_sum[k].b)+eps;
                freq_covar_sum[k].a = freq_covar_sum[k].a / tmp;
                freq_covar_sum[k].b = freq_covar_sum[k].b / tmp;
            }
            for(k=0;k<nbin;++k){
                tmp = sqrtf(cross_covar[k].a * cross_covar[k].a + cross_covar[k].b * cross_covar[k].b)+eps;
                cross_covar[k].a = cross_covar[k].a / tmp;
                cross_covar[k].b = cross_covar[k].b / tmp;
            }
            if(feature_type==1){
                for(k=0;k<nbin;++k){
                    for(i=0;i<narray;++i){
                        feature[i][k] = LPS[i][k];
                    }
                    for(i=0;i<covar_channels;++i){
                        feature[i+narray][k] = mic_covar[i][k].a;
                    }
                    for(i=0;i<covar_channels;++i){
                        feature[i+narray+covar_channels][k] = mic_covar[i][k].b;
                    }
                    feature[2*covar_channels+narray][k] = freq_covar_sum[k].a;
                    feature[2*covar_channels+narray+1][k] = freq_covar_sum[k].b;
                }
            }else if(feature_type==2){
                for(k=0;k<nbin;++k){
                    for(i=0;i<narray;++i){
                        feature[i][k] = mic_aptd[i][k];
                    }
                    for(i=0;i<covar_channels;++i){
                        feature[i+narray][k] = mic_covar[i][k].a;
                    }
                    for(i=0;i<covar_channels;++i){
                        feature[i+narray+covar_channels][k] = mic_covar[i][k].b;
                    }
                    feature[2*covar_channels+narray][k] = freq_covar_sum[k].a;
                    feature[2*covar_channels+narray+1][k] = freq_covar_sum[k].b;
                }

            }else if(feature_type==5){
                for(k=0;k<nbin;++k){
                    for(i=0;i<narray;++i){
                        feature[i][k] = LPS[i][k];
                    }
                    for(i=0;i<covar_channels;++i){
                        feature[i+narray][k] = mic_covar[i][k].a;
                    }
                    for(i=0;i<covar_channels;++i){
                        feature[i+narray+covar_channels][k] = mic_covar[i][k].b;
                    }
                    feature[2*covar_channels+narray][k] = freq_covar_sum[k].a;
                    feature[2*covar_channels+narray+1][k] = freq_covar_sum[k].b;
                    feature[2*covar_channels+narray+2][k] = cross_covar[k].a;
                    feature[2*covar_channels+narray+3][k] = cross_covar[k].b;
                }
            }
        }else if (feature_type==3){
            // for(k=0;k<nbin;++k){
            //     mic_aptd[k] = sqrtf(mic_aptd[k]);
            //     feature[0][k] = mic_aptd[k];
            //     for(i=0;i<out_channels;++i){
            //         feature[i+1][k] = mic_covar[i][k].a;
            //     }
            //     for(i=0;i<out_channels;++i){
            //         feature[i+1+out_channels][k] = mic_covar[i][k].b;
            //     }
            //     feature[2*out_channels+1][k] = freq_covar_sum[k].a;
            //     feature[2*out_channels+2][k] = freq_covar_sum[k].b;
            // }
        }else if(feature_type==4){
            // memset(freq_covar_sum, 0, sizeof(wtk_complex_t) * nbin);
            // for(k=0;k<nbin;++k){
            //     for(i=0;i<out_channels;++i){
            //         freq_covar_sum[k].a += freq_covar[i][k].a;
            //         freq_covar_sum[k].b += freq_covar[i][k].b;
            //     }
            //     feature[0][k] = mic_aptd[k];
            //     for(i=0;i<out_channels;++i){
            //         feature[i+1][k] = mic_covar[i][k].a;
            //     }
            //     for(i=0;i<out_channels;++i){
            //         feature[i+1+out_channels][k] = mic_covar[i][k].b;
            //     }
            //     feature[2*out_channels+1][k] = freq_covar_sum[k].a;
            //     feature[2*out_channels+2][k] = freq_covar_sum[k].b;
            // }
        }
    }
    // for(i=0;i<beamnet4->cfg->feature_len;++i){
    //     printf("%d\n", i);
    //     for(k=0;k<10;++k){
    //         printf("%f ", feature[i][k]);
    //     }
    //     printf("\n");
    // }
    // getchar();
}

void wtk_beamnet4_feed_seperator(wtk_beamnet4_t *beamnet4, float *x, float *mix_spec)
{
#ifdef ONNX_DEC
    int i, j;
    const OrtApi *api = beamnet4->seperator->api;
    OrtMemoryInfo *meminfo = beamnet4->seperator->meminfo;
    qtk_onnxruntime_t *seperator = beamnet4->seperator;
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
        if(beamnet4->seperator_out_len[j]==0){
            int64_t size = 0, *out_shape;
            int d_len;
            d_len = 1;
            out_shape = qtk_onnxruntime_get_outshape(seperator, j, &size);
            for(int k=0;k<size;++k){
                d_len *= out_shape[k];
            }
            wtk_free(out_shape);
            beamnet4->seperator_out_len[j] = d_len;
        }
        if(!beamnet4->speech_est_real){
            if(j==0){
                beamnet4->speech_est_real = (float *)wtk_malloc(sizeof(float)*beamnet4->seperator_out_len[j]);
            }
        }
        if(!beamnet4->speech_est_imag){
            if(j==1){
                beamnet4->speech_est_imag = (float *)wtk_malloc(sizeof(float)*beamnet4->seperator_out_len[j]);
            }
        }
        if(!beamnet4->noise_est_real){
            if(j==2){
                beamnet4->noise_est_real = (float *)wtk_malloc(sizeof(float)*beamnet4->seperator_out_len[j]);
            }
        }
        if(!beamnet4->noise_est_imag){
            if(j==3){
                beamnet4->noise_est_imag = (float *)wtk_malloc(sizeof(float)*beamnet4->seperator_out_len[j]);
            }
        }
        onnx_out = qtk_onnxruntime_getout(seperator, j);
        if(j==0){
            memcpy(beamnet4->speech_est_real, onnx_out, beamnet4->seperator_out_len[j]*sizeof(float));
        }else if(j==1){
            memcpy(beamnet4->speech_est_imag, onnx_out, beamnet4->seperator_out_len[j]*sizeof(float));
        }else if(j==2){
            memcpy(beamnet4->noise_est_real, onnx_out, beamnet4->seperator_out_len[j]*sizeof(float));
        }else if(j==3){
            memcpy(beamnet4->noise_est_imag, onnx_out, beamnet4->seperator_out_len[j]*sizeof(float));
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

void wtk_beamnet4_feature_extract(wtk_beamnet4_t *beamnet4, float *mix_spec)
{
    float *speech_est_real=beamnet4->speech_est_real;
    float *speech_est_imag=beamnet4->speech_est_imag;
    float *noise_est_real=beamnet4->noise_est_real;
    float *noise_est_imag=beamnet4->noise_est_imag;
    wtk_complex_t **speech_covar = beamnet4->speech_covar;
    wtk_complex_t **noise_covar = beamnet4->noise_covar;
    wtk_complex_t *cov, *cov1;
    wtk_complex_t *a, *b, *a1, *b1;
    float *mix_aptd = beamnet4->mix_aptd;
    float *speech_aptd = beamnet4->speech_aptd;
    float *noise_aptd = beamnet4->noise_aptd;
    float eps = 1.1920928955078125e-07;
    float feat_scaler = beamnet4->cfg->feat_scaler;

    int k;
    int i,j;
    int nmicchannel = beamnet4->cfg->nmicchannel;
    int nbin = beamnet4->nbin;

    // printf("speech_est_real:\n");
    // for(i=0;i<nmicchannel;++i){
    //     for(k=0;k<nbin;++k){
    //         printf("%f ", speech_est_real[i*nbin+k]);
    //     }
    //     printf("\n");
    // }

    if(feat_scaler!=1.0){
        for(i=0;i<nmicchannel*nbin;++i){
            speech_est_real[i] *= feat_scaler;
            speech_est_imag[i] *= feat_scaler;
            noise_est_real[i] *= feat_scaler;
            noise_est_imag[i] *= feat_scaler;
        }
    }

    if(beamnet4->cfg->use_mvdr){
        for(k=0;k<nbin;++k){
            cov=speech_covar[k];
            cov1=noise_covar[k];
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
        }
        if(beamnet4->cfg->bf_feature_type==0){
            if(!beamnet4->beamformer_x){
                beamnet4->beamformer_x = (float *)wtk_malloc(sizeof(float)*beamnet4->nbin*nmicchannel*nmicchannel*4);
                memset(beamnet4->beamformer_x, 0, sizeof(float)*beamnet4->nbin*nmicchannel*nmicchannel*4);
            }
            for(k=0;k<nbin;++k){
                cov=speech_covar[k];
                cov1=noise_covar[k];
                for(i=0;i<nmicchannel*nmicchannel;++i){
                    beamnet4->beamformer_x[i*nbin+k] = cov[i].a;
                    beamnet4->beamformer_x[(i+nmicchannel*nmicchannel)*nbin+k] = cov[i].b;
                    beamnet4->beamformer_x[(i+nmicchannel*nmicchannel*2)*nbin+k] = cov1[i].a;
                    beamnet4->beamformer_x[(i+nmicchannel*nmicchannel*3)*nbin+k] = cov1[i].b;
                }
            }
        }else if(beamnet4->cfg->bf_feature_type==1){
            if(!beamnet4->beamformer_x){
                beamnet4->beamformer_x = (float *)wtk_malloc(sizeof(float)*beamnet4->nbin*(nmicchannel*nmicchannel*4+2));
                memset(beamnet4->beamformer_x, 0, sizeof(float)*beamnet4->nbin*(nmicchannel*nmicchannel*4+2));
            }
            memset(mix_aptd, 0, sizeof(float) * nbin);
            memset(speech_aptd, 0, sizeof(float) * nbin);
            memset(noise_aptd, 0, sizeof(float) * nbin);
            for(k=0;k<nbin;++k){
                cov=speech_covar[k];
                cov1=noise_covar[k];
                for(i=0;i<nmicchannel;++i){
                    mix_aptd[k]+=sqrtf(mix_spec[i*nbin+k]*mix_spec[i*nbin+k]+mix_spec[(nmicchannel+i)*nbin+k]*mix_spec[(nmicchannel+i)*nbin+k]);
                    speech_aptd[k]+=sqrtf(speech_est_real[i*nbin+k]*speech_est_real[i*nbin+k]+speech_est_imag[i*nbin+k]*speech_est_imag[i*nbin+k]);
                    noise_aptd[k]+=sqrtf(noise_est_real[i*nbin+k]*noise_est_real[i*nbin+k]+noise_est_imag[i*nbin+k]*noise_est_imag[i*nbin+k]);
                }
                mix_aptd[k]/=nmicchannel;
                speech_aptd[k]/=nmicchannel;
                noise_aptd[k]/=nmicchannel;
                mix_aptd[k] = mix_aptd[k]*mix_aptd[k]+eps;
                speech_aptd[k] = sqrtf(speech_aptd[k]);
                noise_aptd[k] = sqrtf(noise_aptd[k]);
                beamnet4->beamformer_x[k] = speech_aptd[k];
                beamnet4->beamformer_x[nbin+k] = noise_aptd[k];
                for(i=0;i<nmicchannel*nmicchannel;++i){
                    cov[i].a/=mix_aptd[k];
                    cov[i].b/=mix_aptd[k];
                    cov1[i].a/=mix_aptd[k];
                    cov1[i].b/=mix_aptd[k];
                    beamnet4->beamformer_x[(i+2)*nbin+k] = cov[i].a;
                    beamnet4->beamformer_x[(i+2+nmicchannel*nmicchannel)*nbin+k] = cov[i].b;
                    beamnet4->beamformer_x[(i+2+nmicchannel*nmicchannel*2)*nbin+k] = cov1[i].a;
                    beamnet4->beamformer_x[(i+2+nmicchannel*nmicchannel*3)*nbin+k] = cov1[i].b;
                }
            }
        }
    }else{

    }
    // for(i=0;i<10;++i){
    //     printf("%f ", beamnet4->beamformer_x[i]);
    // }
    // for(i=nmicchannel*nmicchannel*nbin*4+2-10;i<nmicchannel*nmicchannel*nbin*4+2;++i){
    //     printf("%f ", beamnet4->beamformer_x[i]);
    // }
    // printf("\n");
    // getchar();
}

void wtk_beamnet4_feed_beamformer(wtk_beamnet4_t *beamnet4, float *mix_spec)
{
#ifdef ONNX_DEC
    int i, j;
    float *beamformer_x=beamnet4->beamformer_x;
    const OrtApi *api = beamnet4->beamformer->api;
    OrtMemoryInfo *meminfo = beamnet4->beamformer->meminfo;
    qtk_onnxruntime_t *beamformer = beamnet4->beamformer;
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
        if(beamnet4->beamformer_out_len[j]==0){
            int64_t size = 0, *out_shape;
            int d_len;
            d_len = 1;
            out_shape = qtk_onnxruntime_get_outshape(beamformer, j, &size);
            for(int k=0;k<size;++k){
                d_len *= out_shape[k];
            }
            wtk_free(out_shape);
            beamnet4->beamformer_out_len[j] = d_len;
        }
        if(!beamnet4->src_est_real){
            if(j==0){
                beamnet4->src_est_real = (float *)wtk_malloc(sizeof(float)*beamnet4->beamformer_out_len[j]);
            }
        }
        if(!beamnet4->src_est_imag){
            if(j==1){
                beamnet4->src_est_imag = (float *)wtk_malloc(sizeof(float)*beamnet4->beamformer_out_len[j]);
            }
        }
        onnx_out = qtk_onnxruntime_getout(beamformer, j);
        if(j==0){
            memcpy(beamnet4->src_est_real, onnx_out, beamnet4->beamformer_out_len[j]*sizeof(float));
        }else if(j==1){
            memcpy(beamnet4->src_est_imag, onnx_out, beamnet4->beamformer_out_len[j]*sizeof(float));
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
void wtk_beamnet4_feed_model(wtk_beamnet4_t *beamnet4, wtk_complex_t **fft, float **feature)
{
    float *x = beamnet4->x;
    float *mix_spec = beamnet4->mix_spec;
    wtk_complex_t *fftx = beamnet4->fftx;
    int i, j;
    int nbin = beamnet4->nbin;
    int nmicchannel = beamnet4->cfg->nmicchannel;
    int feature_len = beamnet4->cfg->feature_len;

    for(i=0;i<feature_len;++i){
        memcpy(x+i*nbin, feature[i], nbin * sizeof(float));
    }

    for(i=0;i<nmicchannel;++i){
        for(j=0;j<nbin;++j){
            mix_spec[i*nbin+j] = fft[i][j].a;
            mix_spec[(nmicchannel+i)*nbin+j] = fft[i][j].b;
        }
    }
    if(beamnet4->cfg->use_onnx){
        wtk_beamnet4_feed_seperator(beamnet4, x, mix_spec);
        wtk_beamnet4_feature_extract(beamnet4, mix_spec);
        wtk_beamnet4_feed_beamformer(beamnet4, mix_spec);
        for(i=0;i<nbin;++i){
            fftx[i].a = beamnet4->src_est_real[i];
            fftx[i].b = beamnet4->src_est_imag[i];
        }

    }else{
        memcpy(fftx, fft[0], nbin*sizeof(wtk_complex_t));
    }
}

void wtk_beamnet4_feed_decode(wtk_beamnet4_t *beamnet4, wtk_complex_t **fft)
{
    float **feature = beamnet4->feature;
    wtk_beamnet4_compute_feature(beamnet4, fft, feature, beamnet4->cfg->sep_feature_type);
    wtk_beamnet4_feed_model(beamnet4, fft, feature);
}


float wtk_beamnet4_entropy(wtk_beamnet4_t *beamnet4, wtk_complex_t *fftx)
{
    int rate = beamnet4->cfg->rate;
    int wins = beamnet4->cfg->wins;
    int i;
    int fx1 = (250*1.0*wins)/rate;
    int fx2 = (3500*1.0*wins)/rate;
    int km = floor(wins*1.0/8);
    float K = 0.5;
    float *E=beamnet4->entropy_E;
    float P1;
    float *Eb=beamnet4->entropy_Eb;
    float sum;
    float prob;
    float Hb;

    memset(E, 0, sizeof(float) * beamnet4->nbin);
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


void wtk_beamnet4_feed_bf(wtk_beamnet4_t *beamnet4, wtk_complex_t **fft)
{
	wtk_complex_t *fftx=beamnet4->fftx;
	int k,nbin=beamnet4->nbin;
    int i;
    // int nmicchannel=beamnet4->cfg->nmicchannel;
	int nbfchannel=beamnet4->cfg->nbfchannel;
	wtk_bf_t *bf=beamnet4->bf;
    wtk_covm_t *covm;
    int b;
    float gf;
    wtk_complex_t fft2[64];
    wtk_complex_t ffts[64];
    wtk_complex_t ffty[64];
	int clip_s=beamnet4->cfg->clip_s;
	int clip_e=beamnet4->cfg->clip_e;
	int bf_clip_s=beamnet4->cfg->bf_clip_s;
	int bf_clip_e=beamnet4->cfg->bf_clip_e;
	int bfflush_cnt=beamnet4->cfg->bfflush_cnt;

	fftx[0].a=fftx[0].b=0;
	fftx[nbin-1].a=fftx[nbin-1].b=0;
	if(beamnet4->cfg->use_fixtheta)
	{
		for(k=1; k<nbin-1; ++k)
		{
			gf=beamnet4->gf[k];
			for(i=0; i<nbfchannel; ++i)
			{
				ffts[i].a=fft[i][k].a*gf;
				ffts[i].b=fft[i][k].b*gf;
            }
			wtk_bf_output_fft_k(bf, ffts, fftx+k, k);
        }
	}else if(beamnet4->cfg->use_sim_bf){
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
        scov = beamnet4->sim_scov;
        ncov = beamnet4->sim_ncov;
        cnt_sum = beamnet4->sim_cnt_sum;
        scov_alpha = beamnet4->covm->cfg->scov_alpha;
        ncov_alpha = beamnet4->covm->cfg->ncov_alpha;
        init_covnf = beamnet4->covm->cfg->init_scovnf;
        scov_alpha_1 = 1.0 - scov_alpha;
        ncov_alpha_1 = 1.0 - ncov_alpha;
		for(k=clip_s+1; k<clip_e; ++k)
		{
			gf=beamnet4->gf[k];
            if(k>=bf_clip_s && k<bf_clip_e){
                mu=beamnet4->cfg->bfmu;
            }else{
                mu=beamnet4->cfg->bfmu2;
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
			gf=beamnet4->gf[k];
            if(k>=bf_clip_s && k<bf_clip_e){
                bf->cfg->mu=beamnet4->cfg->bfmu;
            }else{
                bf->cfg->mu=beamnet4->cfg->bfmu2;
            }
            covm = beamnet4->covm;
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
			if(b==1 && beamnet4->bfflushnf==bfflush_cnt)
			{
                wtk_bf_update_w(bf, k);
            }
			wtk_bf_output_fft_k(bf, fft2, fftx+k, k);
            // fftx[k]=fft[0][k];
        }
        --beamnet4->bfflushnf;
		if(beamnet4->bfflushnf==0)
		{
			beamnet4->bfflushnf=bfflush_cnt;
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

void wtk_beamnet4_control_bs(wtk_beamnet4_t *beamnet4, float *out, int len)
{
	float *bs_win=beamnet4->bs_win;
	float max_bs_out = beamnet4->cfg->max_bs_out;
	float out_max;
	int i;

	if(beamnet4->entropy_sil==0)
	{
		out_max=wtk_float_abs_max(out, len);
		if(out_max>max_bs_out)
		{
			beamnet4->bs_scale=max_bs_out/out_max;
			if(beamnet4->bs_scale<beamnet4->bs_last_scale)
			{
				beamnet4->bs_last_scale=beamnet4->bs_scale;
			}else
			{
				beamnet4->bs_scale=beamnet4->bs_last_scale;
			}
			beamnet4->bs_max_cnt=5;
		}
		if(bs_win){
			for(i=0; i<len/2; ++i)
			{
				out[i]*=beamnet4->bs_scale * bs_win[i] + beamnet4->bs_real_scale * (1.0-bs_win[i]);
			}
			for(i=len/2; i<len; ++i){
				out[i]*=beamnet4->bs_scale;
			}
			beamnet4->bs_real_scale = beamnet4->bs_scale;
		}else{
			for(i=0; i<len; ++i){
				out[i]*=beamnet4->bs_scale;
			}
		}
		if(beamnet4->bs_max_cnt>0)
		{
			--beamnet4->bs_max_cnt;
		}
		if(beamnet4->bs_max_cnt<=0 && beamnet4->bs_scale<1.0)
		{
			beamnet4->bs_scale*=1.1f;
			beamnet4->bs_last_scale=beamnet4->bs_scale;
			if(beamnet4->bs_scale>1.0)
			{
				beamnet4->bs_scale=1.0;
				beamnet4->bs_last_scale=1.0;
			}
		}
	}else
	{
		beamnet4->bs_scale=1.0;
		beamnet4->bs_last_scale=1.0;
		beamnet4->bs_max_cnt=0;
	}
} 

void wtk_beamnet4_feed(wtk_beamnet4_t *beamnet4, short *data, int len,
                       int is_end) {
    int i, j, k, n;
    int nmicchannel = beamnet4->cfg->nmicchannel;
    int *nmicchannels = beamnet4->cfg->nmicchannels;
    int channel = beamnet4->cfg->channel;
	int **mic_channel=beamnet4->cfg->mic_channel;
    int wins = beamnet4->cfg->wins;
    int fsize = wins / 2;
    int length;
    wtk_drft_t *rfft = beamnet4->rfft;
    float *rfft_in = beamnet4->rfft_in;
    wtk_complex_t **fft = beamnet4->fft;
    wtk_complex_t *fftx = beamnet4->fftx;
    float **analysis_mem = beamnet4->analysis_mem;
	float *synthesis_mem=beamnet4->synthesis_mem;
    float *synthesis_window=beamnet4->synthesis_window;
    float *analysis_window = beamnet4->analysis_window;
    float *out = beamnet4->out;
    short *pv = (short *)out;
    wtk_strbuf_t **mic=beamnet4->mic;
    float fv;
    float entropy=0;
    float entropy_thresh = beamnet4->cfg->entropy_thresh;
    int entropy_cnt = beamnet4->cfg->entropy_cnt;
    float entropy_scale;
    int nbin = beamnet4->nbin;
    wtk_complex_t *last_fftx = beamnet4->last_fftx;
    float *last_gf = beamnet4->last_gf;
    int delay_nf = beamnet4->cfg->delay_nf;
	int clip_s = beamnet4->cfg->clip_s;
	int clip_e = beamnet4->cfg->clip_e;
    int narray = beamnet4->cfg->narray;
    float *mic_scale = beamnet4->cfg->mic_scale;
    float mean_gf = 1;
    float mean_gf_thresh = beamnet4->cfg->mean_gf_thresh;

	for(i=0;i<len;++i)
	{
        k = 0;
        for(n=0;n<narray;++n)
        {
            for(j=0; j<nmicchannels[n]; ++j,++k)
            {
                fv = WTK_WAV_SHORT_TO_FLOAT(data[mic_channel[n][j]]);
                fv *= mic_scale[k];
                wtk_strbuf_push(mic[k],(char *)&(fv),sizeof(float));
            }
        }
        data += channel;
    }
    length = mic[0]->pos/sizeof(float);
    while(length>=fsize){
        ++beamnet4->nframe;
        for(i=0;i<nmicchannel;++i){
            wtk_drft_stft(rfft, rfft_in, analysis_mem[i], fft[i], (float *)(mic[i]->data), wins, analysis_window);
        }
        wtk_beamnet4_feed_decode(beamnet4, fft);


        if(beamnet4->gf){
            n = beamnet4->cfg->gf_channel;
            for(i=0;i<nbin;++i){
                beamnet4->gf[i] = (fftx[i].a*fftx[i].a+fftx[i].b*fftx[i].b)/(fft[n][i].a*fft[n][i].a+fft[n][i].b*fft[n][i].b+1e-12);
                beamnet4->gf[i] = max(min(beamnet4->gf[i],1.0),0.0);
                // printf("%f\n", beamnet4->gf[i]);
            }
            mean_gf = wtk_float_abs_mean(beamnet4->gf, nbin);
        }
        // printf("%f\n", wtk_float_abs_mean(beamnet4->gf, nbin));

        if(beamnet4->cfg->use_bf){
            wtk_beamnet4_feed_bf(beamnet4, fft);
        }

        if(entropy_thresh>0){
            entropy=wtk_beamnet4_entropy(beamnet4, fftx);
            if(entropy<entropy_thresh && mean_gf > mean_gf_thresh){
                ++beamnet4->entropy_in_cnt;
            }else{
                beamnet4->entropy_in_cnt = 0;
            }
            if(beamnet4->entropy_in_cnt>=beamnet4->cfg->entropy_in_cnt){
                beamnet4->entropy_sil = 0;
                beamnet4->entropy_silcnt = entropy_cnt;
            }else if(beamnet4->entropy_sil==0){
                beamnet4->entropy_silcnt -= 1;
                if(beamnet4->entropy_silcnt<=0){
                    beamnet4->entropy_sil = 1;
                }
            }
        }

        // printf("%f\n", entropy);
        // printf("%d\n", beamnet4->entropy_sil);
        if(beamnet4->cfg->delay_nf>0){
            memcpy(last_fftx, last_fftx+nbin, delay_nf*nbin*sizeof(wtk_complex_t));
            memcpy(last_fftx+delay_nf*nbin, fftx, nbin*sizeof(wtk_complex_t));
            if(beamnet4->last_gf){
                memcpy(last_gf, last_gf+nbin, delay_nf*nbin*sizeof(float));
                memcpy(last_gf+delay_nf*nbin, beamnet4->gf, nbin*sizeof(float));
            }

            if(is_end){
                while(beamnet4->delay_nf<beamnet4->cfg->delay_nf){
                    memcpy(fftx, last_fftx, nbin*sizeof(wtk_complex_t));
                    if(beamnet4->last_gf){
                        memcpy(beamnet4->gf, last_gf, nbin*sizeof(float));
                    }
                    if(entropy>entropy_thresh && beamnet4->entropy_sil==1){
                        if(beamnet4->entropy_silcnt > 0){
                            entropy_scale = powf(beamnet4->entropy_silcnt * 1.0/entropy_cnt, beamnet4->cfg->entropy_ratio)+beamnet4->cfg->entropy_min_scale;
                        }else{
                            entropy_scale = powf(1.0/entropy_cnt, beamnet4->cfg->entropy_ratio)+beamnet4->cfg->entropy_min_scale;
                        }
                        entropy_scale = min(entropy_scale, 1.0);
                        for(i=0;i<nbin;++i){
                            fftx[i].a*=entropy_scale;
                            fftx[i].b*=entropy_scale;
                        }
                    }
                    if(beamnet4->qmmse){
                        wtk_qmmse_feed_mask(beamnet4->qmmse, fftx, beamnet4->gf);
                    }

                    // memcpy(fftx, fft[0], nbin*sizeof(wtk_complex_t));
                    wtk_drft_istft(rfft, rfft_in, synthesis_mem, fftx, out, wins, synthesis_window);

                    for(i=0;i<fsize;++i){
                        pv[i] = WTK_WAV_FLOAT_TO_SHORT(out[i]);
                    }
                    if(beamnet4->notify){
                        beamnet4->notify(beamnet4->ths, pv, fsize);
                    }
                    ++beamnet4->delay_nf;
                }
                if(is_end && length>0){
                    if(beamnet4->notify)
                    {
                        pv=(short *)mic[0]->data;
                        beamnet4->notify(beamnet4->ths,pv,length);
                    }
                }
            }else{
                if(beamnet4->delay_nf>0){
                    --beamnet4->delay_nf;
                    wtk_strbufs_pop(mic, nmicchannel, fsize*sizeof(float));
                    length = mic[0]->pos/sizeof(float);
                    continue;
                }else{
                    memcpy(fftx, last_fftx, nbin*sizeof(wtk_complex_t));
                }
            }
        }

        if(entropy>entropy_thresh && beamnet4->entropy_sil==1){
            if(beamnet4->entropy_silcnt > 0){
                entropy_scale = powf(beamnet4->entropy_silcnt * 1.0/entropy_cnt, beamnet4->cfg->entropy_ratio)+beamnet4->cfg->entropy_min_scale;
            }else{
                entropy_scale = powf(1.0/entropy_cnt, beamnet4->cfg->entropy_ratio)+beamnet4->cfg->entropy_min_scale;
            }
            entropy_scale = min(entropy_scale, 1.0);
            for(i=0;i<nbin;++i){
                fftx[i].a*=entropy_scale;
                fftx[i].b*=entropy_scale;
            }
        }

        if(entropy_thresh>0){
            if(beamnet4->entropy_sil==1){
                if(beamnet4->gf){
                    for(i=0;i<nbin;++i){
                        beamnet4->gf[i] = 0;
                    }
                }else{
                    memset(fftx, 0, nbin*sizeof(wtk_complex_t));
                }
            }else if(beamnet4->gc){
                float gc_mask = wtk_float_abs_mean(beamnet4->gf, nbin);
                qtk_gain_controller_run(beamnet4->gc, fftx, fsize, NULL,gc_mask);
            }
        }
        if(beamnet4->qmmse){
            // wtk_qmmse_denoise(beamnet4->qmmse, fftx);
            wtk_qmmse_feed_mask(beamnet4->qmmse, fftx, beamnet4->gf);
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

		wtk_beamnet4_control_bs(beamnet4, out, fsize);

        for(i=0;i<fsize;++i){
            // pv[i] = WTK_WAV_FLOAT_TO_SHORT(out[i]);
            pv[i] = floorf(out[i]+0.5);
        }
        if(beamnet4->notify){
            beamnet4->notify(beamnet4->ths, pv, fsize);
        }

        wtk_strbufs_pop(mic, nmicchannel, fsize*sizeof(float));
        length = mic[0]->pos/sizeof(float);
    }
    if(is_end && length>0){
        if(beamnet4->notify)
        {
            pv=(short *)mic[0]->data;
            beamnet4->notify(beamnet4->ths,pv,length);
        }
    }
}