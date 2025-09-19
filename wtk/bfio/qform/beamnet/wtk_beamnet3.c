#include "wtk/bfio/qform/beamnet/wtk_beamnet3.h"
#include "qtk/math/qtk_vector.h"
#ifndef WTK_WAV_SHORT_TO_FLOAT
#define WTK_WAV_SHORT_TO_FLOAT(f) ((f) > 0? (f/32767.0) : (f/32768.0))
#endif
#ifndef WTK_WAV_FLOAT_TO_SHORT
#define WTK_WAV_FLOAT_TO_SHORT(f) ((f) > 0? floorf(f*32767.0+0.5) : floorf(f*32768.0+0.5))
#endif

void wtk_beamnet3_compute_channel_covariance(wtk_beamnet3_t *beamnet3, wtk_complex_t **fft, wtk_complex_t **covar_mat, int norm)
{
    int nmicchannel = beamnet3->cfg->nmicchannel;
    int nbin = beamnet3->nbin;
    int i, j, k;
    wtk_complex_t *mean_mat = beamnet3->mean_mat;
    wtk_complex_t **input_norm = beamnet3->input_norm;
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

void wtk_beamnet3_compute_multi_channel_phase_shift(wtk_beamnet3_t *beamnet3, wtk_complex_t **phase_shift, float theta, float phi)
{
    int wins = beamnet3->cfg->wins;
    int nmicchannel = beamnet3->cfg->nmicchannel;
    int nbin = beamnet3->nbin;
    int rate = beamnet3->cfg->rate;
    float sv = beamnet3->cfg->sv;
    float **mic_pos = beamnet3->cfg->mic_pos;
    float *time_delay = beamnet3->time_delay;
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

void wtk_beamnet3_compute_multi_channel_phase_shift2(wtk_beamnet3_t *beamnet3, wtk_complex_t **phase_shift, float theta, float phi)
{
    int wins = beamnet3->cfg->wins;
    int nmicchannel = beamnet3->cfg->nmicchannel;
    int nbin = beamnet3->nbin;
    int rate = beamnet3->cfg->rate;
    float sv = beamnet3->cfg->sv;
    float **mic_pos = beamnet3->cfg->mic_pos;
    float *time_delay = beamnet3->time_delay;
    float *mic;
    float x, y, z;
    int i, k;
    float t;

    // theta -= 180.0;
    // theta = theta < -180.0 ? theta + 360.0 : theta;
    // theta = theta > 180.0  ? theta - 360.0 : theta;

    //theta = theta * PI / 180.0;
    phi = phi * PI / 180.0;
    x = cos(phi) * sin(theta);
    y = cos(phi) * -cos(theta);
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

wtk_beamnet3_t *wtk_beamnet3_new(wtk_beamnet3_cfg_t *cfg) {
    wtk_beamnet3_t *beamnet3;

    beamnet3 = (wtk_beamnet3_t *)wtk_malloc(sizeof(wtk_beamnet3_t));
    beamnet3->cfg = cfg;
    beamnet3->ths = NULL;
    beamnet3->notify = NULL;
    beamnet3->mic = wtk_strbufs_new(beamnet3->cfg->nmicchannel);
    beamnet3->sp = wtk_strbufs_new(beamnet3->cfg->nspchannel);

    beamnet3->nbin = cfg->wins / 2 + 1;
    beamnet3->analysis_window = wtk_malloc(sizeof(float) * cfg->wins);  /// 2);
    beamnet3->synthesis_window = wtk_malloc(sizeof(float) * cfg->wins); /// 2);
    beamnet3->analysis_mem = wtk_float_new_p2(cfg->nmicchannel, beamnet3->nbin - 1);
    beamnet3->analysis_mem_sp = wtk_float_new_p2(cfg->nspchannel, beamnet3->nbin - 1);
    beamnet3->synthesis_mem = wtk_malloc(sizeof(float) * (beamnet3->nbin - 1));
    beamnet3->rfft = wtk_drft_new(cfg->wins);
    beamnet3->rfft_in = (float *)wtk_malloc(sizeof(float) * (cfg->wins));

    beamnet3->fft = wtk_complex_new_p2(cfg->nmicchannel, beamnet3->nbin);
    beamnet3->fft_sp = wtk_complex_new_p2(cfg->nspchannel, beamnet3->nbin);
    beamnet3->fftx = (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * beamnet3->nbin);

    beamnet3->out = wtk_malloc(sizeof(float) * (beamnet3->nbin - 1));

    beamnet3->speech_est_real=NULL;
    beamnet3->speech_est_imag=NULL;
    beamnet3->noise_est_real=NULL;
    beamnet3->noise_est_imag=NULL;
    beamnet3->beamformer_x=NULL;
    beamnet3->src_est_real=NULL;
    beamnet3->src_est_imag=NULL;
#ifdef ONNX_DEC
    beamnet3->seperator = NULL;
    beamnet3->beamformer = NULL;
    beamnet3->sep_caches = NULL;
    beamnet3->beam_caches = NULL;
    beamnet3->seperator_out_len = NULL;
    beamnet3->beamformer_out_len = NULL;
    if(cfg->use_onnx){
        beamnet3->seperator = qtk_onnxruntime_new(&(cfg->seperator));
        beamnet3->sep_caches = wtk_calloc(sizeof(OrtValue *), beamnet3->seperator->num_in - cfg->seperator.outer_in_num);
        if (beamnet3->seperator->num_in - cfg->seperator.outer_in_num != beamnet3->seperator->num_out - cfg->seperator.outer_out_num) {
            wtk_debug("err inner_item\n");
            exit(0);
        }
        beamnet3->seperator_out_len = (int *)wtk_malloc(sizeof(int) * (cfg->seperator.outer_out_num));
        beamnet3->beamformer = qtk_onnxruntime_new(&(cfg->beamformer));
        beamnet3->beam_caches = wtk_calloc(sizeof(OrtValue *), beamnet3->beamformer->num_in - cfg->beamformer.outer_in_num);
        if (beamnet3->beamformer->num_in - cfg->beamformer.outer_in_num != beamnet3->beamformer->num_out - cfg->beamformer.outer_out_num) {
            wtk_debug("err inner_item\n");
            exit(0);
        }
        beamnet3->beamformer_out_len = (int *)wtk_malloc(sizeof(int) * (cfg->seperator.outer_out_num));
    }
#endif
    beamnet3->feature = wtk_float_new_p2(cfg->feature_len, beamnet3->nbin);
    beamnet3->x = (float *)wtk_malloc(sizeof(float) * cfg->feature_len * beamnet3->nbin);
    beamnet3->mix_spec = (float *)wtk_malloc(sizeof(float) * (cfg->nmicchannel) * 2 * beamnet3->nbin);
    beamnet3->time_delay = (float *)wtk_malloc(sizeof(float) * beamnet3->nbin);
    beamnet3->mic_aptd = (float *)wtk_malloc(sizeof(float) * beamnet3->nbin);
    beamnet3->echo_aptd = (float *)wtk_malloc(sizeof(float) * beamnet3->nbin);
    beamnet3->mic_covar = wtk_complex_new_p2(cfg->out_channels, beamnet3->nbin);

    if(cfg->sep_feature_type == 5){
        int i;
        beamnet3->ideal_phase_covar = NULL;
        beamnet3->ideal_phase_shift = NULL;
        beamnet3->ideal_phase_covars = wtk_malloc(sizeof(wtk_complex_t**) * cfg->total_regions);
        for(i = 0; i < cfg->total_regions; i++){
            beamnet3->ideal_phase_covars[i] = wtk_complex_new_p2(cfg->out_channels, beamnet3->nbin);
        }
        beamnet3->ideal_phase_shifts = wtk_malloc(sizeof(wtk_complex_t**) * cfg->total_regions);
        for(i = 0; i < cfg->total_regions; i++){
            beamnet3->ideal_phase_shifts[i] = wtk_complex_new_p2(cfg->nmicchannel, beamnet3->nbin);
        }
    }else{
        beamnet3->ideal_phase_covar = wtk_complex_new_p2(cfg->out_channels, beamnet3->nbin);
        beamnet3->ideal_phase_shift = wtk_complex_new_p2(cfg->nmicchannel, beamnet3->nbin);
        beamnet3->ideal_phase_covars = NULL;
        beamnet3->ideal_phase_shifts = NULL;
    }


    beamnet3->freq_covar = wtk_complex_new_p2(cfg->out_channels, beamnet3->nbin);
    beamnet3->freq_covar_sum = (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * beamnet3->nbin);
    beamnet3->LPS = (float *)wtk_malloc(sizeof(float) * beamnet3->nbin);
    beamnet3->echo_LPS = (float *)wtk_malloc(sizeof(float) * beamnet3->nbin);
    beamnet3->IPDs = wtk_float_new_p2(cfg->out_channels, beamnet3->nbin);
    beamnet3->FDDF = (float *)wtk_malloc(sizeof(float) * beamnet3->nbin);
    beamnet3->mean_mat = (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * beamnet3->nbin);
    beamnet3->input_norm = wtk_complex_new_p2(cfg->nmicchannel, beamnet3->nbin);
    beamnet3->speech_covar = wtk_complex_new_p2(beamnet3->nbin, cfg->nmicchannel*cfg->nmicchannel);
    beamnet3->noise_covar = wtk_complex_new_p2(beamnet3->nbin, cfg->nmicchannel*cfg->nmicchannel);

    beamnet3->mix_aptd = NULL;
    beamnet3->speech_aptd = NULL;
    beamnet3->noise_aptd = NULL;
    if(cfg->bf_feature_type==1){
        beamnet3->mix_aptd = (float *)wtk_malloc(sizeof(float) * beamnet3->nbin);
        beamnet3->speech_aptd = (float *)wtk_malloc(sizeof(float) * beamnet3->nbin);
        beamnet3->noise_aptd = (float *)wtk_malloc(sizeof(float) * beamnet3->nbin);
    }

    beamnet3->qmmse=NULL;
    if(cfg->use_qmmse){
		beamnet3->qmmse=wtk_qmmse_new(&(cfg->qmmse));
    }

    beamnet3->entropy_E=NULL;
    beamnet3->entropy_Eb=NULL;
    beamnet3->last_fftx=NULL;
    if(cfg->entropy_thresh>0){
        beamnet3->entropy_E=(float *)wtk_malloc(sizeof(float)*beamnet3->nbin);
        beamnet3->entropy_Eb=(float *)wtk_malloc(sizeof(float)*cfg->wins);
        if(cfg->delay_nf>0){
            beamnet3->last_fftx=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*beamnet3->nbin*(cfg->delay_nf+1));
        }
    }

    beamnet3->gf=(float *)wtk_malloc(sizeof(float)*beamnet3->nbin);
    beamnet3->last_gf=NULL;
    if((cfg->use_qmmse && cfg->qmmse.use_agc_mask) || cfg->use_bf){
        if(cfg->delay_nf>0){
            beamnet3->last_gf=(float *)wtk_malloc(sizeof(float)*beamnet3->nbin*(cfg->delay_nf+1));
        }
    }
    beamnet3->bf=NULL;
	beamnet3->covm=NULL;
    if(cfg->use_bf){
        beamnet3->bf = wtk_bf_new(&(cfg->bf), cfg->wins);
        beamnet3->covm = wtk_covm_new(&(cfg->covm), beamnet3->nbin, cfg->nbfchannel);
    }

	beamnet3->sim_scov=NULL;
	beamnet3->sim_ncov=NULL;
	beamnet3->sim_cnt_sum=NULL;
	if(cfg->use_sim_bf){
		beamnet3->sim_scov = (float *)wtk_malloc(sizeof(float)*beamnet3->nbin);
		beamnet3->sim_ncov = (float *)wtk_malloc(sizeof(float)*beamnet3->nbin);
		beamnet3->sim_cnt_sum = (int *)wtk_malloc(sizeof(int)*beamnet3->nbin);
    }

    beamnet3->gc = NULL;
	if(cfg->use_gc){
		beamnet3->gc=qtk_gain_controller_new(&(cfg->gc));
		qtk_gain_controller_set_mode(beamnet3->gc,0);
		beamnet3->gc->kalman.Z_k = cfg->gc_gain * 1.0/1000.0;
	}

	beamnet3->bs_win=NULL;
	if(cfg->use_bs_win)
	{
		beamnet3->bs_win=wtk_math_create_hanning_window2(cfg->wins/2);
	}

    beamnet3->raw_mask = NULL;
    if (cfg->use_raw_mask) {
        beamnet3->raw_mask = (float *)wtk_malloc(sizeof(float) * beamnet3->nbin);
    }

    beamnet3->region_mask = NULL;
    if(cfg->sep_feature_type == 5){
        beamnet3->region_mask = (int *)wtk_malloc(sizeof(int) * cfg->total_regions);
        beamnet3->direct_features = (float *)wtk_malloc(sizeof(float) * beamnet3->nbin * 4);
        beamnet3->beamformer_x = (float *)wtk_malloc(sizeof(float)*beamnet3->nbin * 8);
    }

    wtk_beamnet3_reset(beamnet3);

    return beamnet3;
}

void wtk_beamnet3_delete(wtk_beamnet3_t *beamnet3) {
    wtk_strbufs_delete(beamnet3->mic, beamnet3->cfg->nmicchannel);
    wtk_strbufs_delete(beamnet3->sp, beamnet3->cfg->nspchannel);

    wtk_free(beamnet3->analysis_window);
    wtk_free(beamnet3->synthesis_window);
    wtk_float_delete_p2(beamnet3->analysis_mem, beamnet3->cfg->nmicchannel);
    wtk_float_delete_p2(beamnet3->analysis_mem_sp, beamnet3->cfg->nspchannel);
    wtk_free(beamnet3->synthesis_mem);
    wtk_free(beamnet3->rfft_in);
    wtk_drft_delete(beamnet3->rfft);
    wtk_complex_delete_p2(beamnet3->fft, beamnet3->cfg->nmicchannel);
    wtk_complex_delete_p2(beamnet3->fft_sp, beamnet3->cfg->nspchannel);

    wtk_free(beamnet3->fftx);

    wtk_free(beamnet3->out);
#ifdef ONNX_DEC
    if(beamnet3->cfg->use_onnx){
        {
            int n = beamnet3->seperator->num_in - beamnet3->seperator->cfg->outer_in_num;
            if (beamnet3->sep_caches[0]) {
                int i;
                for (i = 0; i < n; i++) {
                    beamnet3->seperator->api->ReleaseValue(beamnet3->sep_caches[i]);
                }
            }
        }
        if (beamnet3->seperator) {
            qtk_onnxruntime_delete(beamnet3->seperator);
        }
        wtk_free(beamnet3->seperator_out_len);
        wtk_free(beamnet3->sep_caches);
        {
            int n = beamnet3->beamformer->num_in - beamnet3->beamformer->cfg->outer_in_num;
            if (beamnet3->beam_caches[0]) {
                int i;
                for (i = 0; i < n; i++) {
                    beamnet3->beamformer->api->ReleaseValue(beamnet3->beam_caches[i]);
                }
            }
        }
        if (beamnet3->beamformer) {
            qtk_onnxruntime_delete(beamnet3->beamformer);
        }
        wtk_free(beamnet3->beamformer_out_len);
        wtk_free(beamnet3->beam_caches);
    }
#endif
    wtk_float_delete_p2(beamnet3->feature, beamnet3->cfg->feature_len);
    wtk_free(beamnet3->x);
    wtk_free(beamnet3->mix_spec);
    wtk_free(beamnet3->time_delay);
    wtk_free(beamnet3->mic_aptd);
    wtk_free(beamnet3->echo_aptd);
    wtk_complex_delete_p2(beamnet3->mic_covar, beamnet3->cfg->out_channels);

    if(beamnet3->cfg->sep_feature_type ==5){
        int i;
        for(i = 0; i < beamnet3->cfg->total_regions; i++){
            wtk_complex_delete_p2(beamnet3->ideal_phase_covars[i], beamnet3->cfg->out_channels);
            wtk_complex_delete_p2(beamnet3->ideal_phase_shifts[i], beamnet3->cfg->nmicchannel);
        }
        wtk_free(beamnet3->ideal_phase_covars);
        wtk_free(beamnet3->ideal_phase_shifts);
        wtk_free(beamnet3->region_mask);
        wtk_free(beamnet3->direct_features);
    }else{
        wtk_complex_delete_p2(beamnet3->ideal_phase_covar, beamnet3->cfg->out_channels);
        wtk_complex_delete_p2(beamnet3->ideal_phase_shift, beamnet3->cfg->nmicchannel);
    }

    wtk_complex_delete_p2(beamnet3->freq_covar, beamnet3->cfg->out_channels);
    wtk_free(beamnet3->freq_covar_sum);
    wtk_free(beamnet3->LPS);
    wtk_free(beamnet3->echo_LPS);
    wtk_float_delete_p2(beamnet3->IPDs, beamnet3->cfg->out_channels);
    wtk_free(beamnet3->FDDF);
    wtk_free(beamnet3->mean_mat);
    wtk_complex_delete_p2(beamnet3->input_norm, beamnet3->cfg->nmicchannel);
    wtk_complex_delete_p2(beamnet3->speech_covar, beamnet3->nbin);
    wtk_complex_delete_p2(beamnet3->noise_covar, beamnet3->nbin);
    if(beamnet3->mix_aptd){
        wtk_free(beamnet3->mix_aptd);
    }
    if(beamnet3->speech_aptd){
        wtk_free(beamnet3->speech_aptd);
    }
    if(beamnet3->noise_aptd){
        wtk_free(beamnet3->noise_aptd);
    }

    if(beamnet3->speech_est_real){
        wtk_free(beamnet3->speech_est_real);
    }
    if(beamnet3->speech_est_imag){
        wtk_free(beamnet3->speech_est_imag);
    }
    if(beamnet3->noise_est_real){
        wtk_free(beamnet3->noise_est_real);
    }
    if(beamnet3->noise_est_imag){
        wtk_free(beamnet3->noise_est_imag);
    }
    if(beamnet3->beamformer_x){
        wtk_free(beamnet3->beamformer_x);
    }
    if(beamnet3->src_est_real){
        wtk_free(beamnet3->src_est_real);
    }
    if(beamnet3->src_est_imag){
        wtk_free(beamnet3->src_est_imag);
    }

    if(beamnet3->qmmse){
        wtk_qmmse_delete(beamnet3->qmmse);
    }
    if(beamnet3->entropy_E){
        wtk_free(beamnet3->entropy_E);
    }
    if(beamnet3->entropy_Eb){
        wtk_free(beamnet3->entropy_Eb);
    }
    if(beamnet3->last_fftx){
        wtk_free(beamnet3->last_fftx);
    }
    if(beamnet3->gf){
        wtk_free(beamnet3->gf);
    }
    if(beamnet3->last_gf){
        wtk_free(beamnet3->last_gf);
    }
    if(beamnet3->bf){
        wtk_bf_delete(beamnet3->bf);
    }
    if(beamnet3->covm){
        wtk_covm_delete(beamnet3->covm);
    }
    if(beamnet3->sim_scov)
	{
        wtk_free(beamnet3->sim_scov);
        wtk_free(beamnet3->sim_ncov);
        wtk_free(beamnet3->sim_cnt_sum);
    }
	if(beamnet3->gc){
		qtk_gain_controller_delete(beamnet3->gc);
	}
	if(beamnet3->bs_win)
	{
		wtk_free(beamnet3->bs_win);
	}
    if(beamnet3->raw_mask)
    {
        wtk_free(beamnet3->raw_mask);
    }

    wtk_free(beamnet3);
}

static void complex_dump(wtk_complex_t *c, int len){
    int i;
    for(i = 0; i < len; i++){
        printf("[%d]=%.6g %.6g\n", i,c[i].a, c[i].b);
    }
}

void wtk_beamnet3_start(wtk_beamnet3_t *beamnet3, float theta, float phi) {
    if(beamnet3->cfg->sep_feature_type == 5){
        int i;
        float theta1,theta2;
        if(beamnet3->cfg->query_region[0] > beamnet3->cfg->query_region[1]){
            theta1 = beamnet3->cfg->query_region[0];
            theta2 = beamnet3->cfg->query_region[1];
        }else{
            theta1 = beamnet3->cfg->query_region[1];
            theta2 = beamnet3->cfg->query_region[0];
        }
        for(i = 0; i < beamnet3->cfg->total_regions; ++i){
            wtk_beamnet3_compute_multi_channel_phase_shift2(beamnet3, beamnet3->ideal_phase_shifts[i], beamnet3->cfg->target_theta[i], phi);
            wtk_beamnet3_compute_channel_covariance(beamnet3, beamnet3->ideal_phase_shifts[i], beamnet3->ideal_phase_covars[i], beamnet3->cfg->norm_channel);
            //complex_dump(beamnet3->ideal_phase_covars[i][0], beamnet3->nbin);
            float theta = beamnet3->cfg->target_theta[i];
            if(theta > theta2 && theta < theta1){
                beamnet3->region_mask[i] = 1;
            }else{
                beamnet3->region_mask[i] = 0;
            }
        }
    }else{
        wtk_complex_t **ideal_phase_shift = beamnet3->ideal_phase_shift;
        wtk_complex_t **ideal_phase_covar = beamnet3->ideal_phase_covar;
        beamnet3->theta = theta;
        wtk_beamnet3_compute_multi_channel_phase_shift(beamnet3, ideal_phase_shift, theta, phi);
        wtk_beamnet3_compute_channel_covariance(beamnet3, ideal_phase_shift, ideal_phase_covar, beamnet3->cfg->norm_channel);
        if(beamnet3->cfg->use_bf){
            wtk_bf_update_ovec(beamnet3->bf,theta,phi);
            wtk_bf_init_w(beamnet3->bf);
        }
    }
}

void wtk_beamnet3_reset(wtk_beamnet3_t *beamnet3) {
    int wins = beamnet3->cfg->wins;
    int i;

    wtk_strbufs_reset(beamnet3->mic, beamnet3->cfg->nmicchannel);
    wtk_strbufs_reset(beamnet3->sp, beamnet3->cfg->nspchannel);

    for (i = 0; i < wins; ++i) {
        beamnet3->analysis_window[i] = sin((0.5 + i) * PI / (wins));
    }
    wtk_drft_init_synthesis_window(beamnet3->synthesis_window,
                                   beamnet3->analysis_window, wins);

    wtk_float_zero_p2(beamnet3->analysis_mem, beamnet3->cfg->nmicchannel,
                      (beamnet3->nbin - 1));
    wtk_float_zero_p2(beamnet3->analysis_mem_sp, beamnet3->cfg->nspchannel,
                      (beamnet3->nbin - 1));
    memset(beamnet3->synthesis_mem, 0, sizeof(float) * (beamnet3->nbin - 1));

    wtk_complex_zero_p2(beamnet3->fft, beamnet3->cfg->nmicchannel,
                        (beamnet3->nbin));
    wtk_complex_zero_p2(beamnet3->fft_sp, beamnet3->cfg->nspchannel,
                        (beamnet3->nbin));
    memset(beamnet3->fftx, 0, sizeof(wtk_complex_t) * (beamnet3->nbin));
#ifdef ONNX_DEC
    if(beamnet3->cfg->use_onnx){
        qtk_onnxruntime_reset(beamnet3->seperator);
        {
            int n = beamnet3->seperator->num_in - beamnet3->seperator->cfg->outer_in_num;
            if (beamnet3->sep_caches[0]) {
                int i;
                for (i = 0; i < n; i++) {
                    beamnet3->seperator->api->ReleaseValue(beamnet3->sep_caches[i]);
                }
                memset(beamnet3->sep_caches, 0, sizeof(OrtValue *) * n);
            }
        }
        memset(beamnet3->seperator_out_len, 0, sizeof(int) * (beamnet3->seperator->cfg->outer_out_num));
        qtk_onnxruntime_reset(beamnet3->beamformer);
        {
            int n = beamnet3->beamformer->num_in - beamnet3->beamformer->cfg->outer_in_num;
            if (beamnet3->beam_caches[0]) {
                int i;
                for (i = 0; i < n; i++) {
                    beamnet3->beamformer->api->ReleaseValue(beamnet3->beam_caches[i]);
                }
                memset(beamnet3->beam_caches, 0, sizeof(OrtValue *) * n);
            }
        }
        memset(beamnet3->beamformer_out_len, 0, sizeof(int) * (beamnet3->beamformer->cfg->outer_out_num));
    }
#endif
    wtk_float_zero_p2(beamnet3->feature, beamnet3->cfg->feature_len,
                      (beamnet3->nbin));
    memset(beamnet3->x, 0, sizeof(float) * beamnet3->cfg->feature_len * beamnet3->nbin);
    memset(beamnet3->mix_spec, 0, sizeof(float) * (beamnet3->cfg->nmicchannel) * 2 * beamnet3->nbin);
    memset(beamnet3->time_delay, 0, sizeof(float) * beamnet3->nbin);
    memset(beamnet3->mic_aptd, 0, sizeof(float) * beamnet3->nbin);
    memset(beamnet3->echo_aptd, 0, sizeof(float) * beamnet3->nbin);
    wtk_complex_zero_p2(beamnet3->mic_covar, beamnet3->cfg->out_channels,
                        (beamnet3->nbin));
    if(beamnet3->cfg->sep_feature_type != 5){
        wtk_complex_zero_p2(beamnet3->ideal_phase_covar, beamnet3->cfg->out_channels,
                         (beamnet3->nbin));
        wtk_complex_zero_p2(beamnet3->ideal_phase_shift, beamnet3->cfg->nmicchannel,
                        (beamnet3->nbin));
    }

    wtk_complex_zero_p2(beamnet3->freq_covar, beamnet3->cfg->out_channels,
                        (beamnet3->nbin));
    memset(beamnet3->freq_covar_sum, 0, sizeof(float) * beamnet3->nbin);
    memset(beamnet3->LPS, 0, sizeof(float) * beamnet3->nbin);
    memset(beamnet3->echo_LPS, 0, sizeof(float) * beamnet3->nbin);
    wtk_float_zero_p2(beamnet3->IPDs, beamnet3->cfg->out_channels,
                      (beamnet3->nbin));
    memset(beamnet3->FDDF, 0, sizeof(float) * beamnet3->nbin);
    memset(beamnet3->mean_mat, 0, sizeof(wtk_complex_t) * beamnet3->nbin);
    wtk_complex_zero_p2(beamnet3->input_norm, beamnet3->cfg->nmicchannel,(beamnet3->nbin));
    wtk_complex_zero_p2(beamnet3->speech_covar, beamnet3->nbin,beamnet3->cfg->nmicchannel*beamnet3->cfg->nmicchannel);
    wtk_complex_zero_p2(beamnet3->noise_covar, beamnet3->nbin,beamnet3->cfg->nmicchannel*beamnet3->cfg->nmicchannel);

    if(beamnet3->mix_aptd){
        memset(beamnet3->mix_aptd, 0, sizeof(float) * beamnet3->nbin);
    }
    if(beamnet3->speech_aptd){
        memset(beamnet3->speech_aptd, 0, sizeof(float) * beamnet3->nbin);
    }
    if(beamnet3->noise_aptd){
        memset(beamnet3->noise_aptd, 0, sizeof(float) * beamnet3->nbin);
    }

    if(beamnet3->qmmse){
        wtk_qmmse_reset(beamnet3->qmmse);
    }
    if(beamnet3->entropy_E){
        memset(beamnet3->entropy_E, 0, sizeof(float) * beamnet3->nbin);
    }
    if(beamnet3->entropy_Eb){
        memset(beamnet3->entropy_Eb, 0, sizeof(float) * beamnet3->nbin);
    }
    if(beamnet3->last_fftx){
        memset(beamnet3->last_fftx, 0, sizeof(wtk_complex_t) * (beamnet3->nbin)*(beamnet3->cfg->delay_nf+1));
    }
    if(beamnet3->gf){
        memset(beamnet3->gf, 0, sizeof(float) * beamnet3->nbin);
    }
    if(beamnet3->last_gf){
        memset(beamnet3->last_gf, 0, sizeof(float) * beamnet3->nbin);
    }
    if(beamnet3->bf){
        wtk_bf_reset(beamnet3->bf);
    }
    if(beamnet3->covm){
        wtk_covm_reset(beamnet3->covm);
    }
    if(beamnet3->sim_scov)
	{
		memset(beamnet3->sim_scov, 0, sizeof(float)*beamnet3->nbin);
		memset(beamnet3->sim_ncov, 0, sizeof(float)*beamnet3->nbin);
		memset(beamnet3->sim_cnt_sum, 0, sizeof(int)*beamnet3->nbin);
    }

    if(beamnet3->gc){
        qtk_gain_controller_reset(beamnet3->gc);
    }
    if (beamnet3->raw_mask) {
        memset(beamnet3->raw_mask, 0, sizeof(float) * beamnet3->nbin);
    }

    beamnet3->entropy_in_cnt = 0;
    beamnet3->entropy_silcnt = 0;
    beamnet3->entropy_sil = 1;

    beamnet3->delay_nf = beamnet3->cfg->delay_nf;

    beamnet3->theta = 90.0;
    beamnet3->nframe = 0;
	beamnet3->bfflushnf=beamnet3->cfg->bfflush_cnt;

	beamnet3->bs_scale=1.0;
	beamnet3->bs_last_scale=1.0;
	beamnet3->bs_real_scale=1.0;
	beamnet3->bs_max_cnt=0;
	beamnet3->gc_cnt=0;
	beamnet3->gc_cnt2=0;
    beamnet3->agc_enable=0;
    beamnet3->denoise_enable=0;
}


wtk_beamnet3_t *wtk_beamnet3_new2(wtk_beamnet3_cfg_t *cfg) {
    wtk_beamnet3_t *beamnet3;

    beamnet3 = (wtk_beamnet3_t *)wtk_malloc(sizeof(wtk_beamnet3_t));
    beamnet3->cfg = cfg;
    beamnet3->ths2 = NULL;
    beamnet3->notify2 = NULL;

    beamnet3->nbin = cfg->wins / 2 + 1;

    beamnet3->fft_sp = wtk_complex_new_p2(cfg->nspchannel, beamnet3->nbin);
    beamnet3->fftx =
        (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * beamnet3->nbin);

    beamnet3->speech_est_real = NULL;
    beamnet3->speech_est_imag = NULL;
    beamnet3->noise_est_real = NULL;
    beamnet3->noise_est_imag = NULL;
    beamnet3->beamformer_x = NULL;
    beamnet3->src_est_real = NULL;
    beamnet3->src_est_imag = NULL;
#ifdef ONNX_DEC
    beamnet3->seperator = NULL;
    beamnet3->beamformer = NULL;
    beamnet3->sep_caches = NULL;
    beamnet3->beam_caches = NULL;
    beamnet3->seperator_out_len = NULL;
    beamnet3->beamformer_out_len = NULL;
    if (cfg->use_onnx) {
        beamnet3->seperator = qtk_onnxruntime_new(&(cfg->seperator));
        beamnet3->sep_caches =
            wtk_calloc(sizeof(OrtValue *), beamnet3->seperator->num_in -
                                               cfg->seperator.outer_in_num);
        if (beamnet3->seperator->num_in - cfg->seperator.outer_in_num !=
            beamnet3->seperator->num_out - cfg->seperator.outer_out_num) {
            wtk_debug("err inner_item\n");
            exit(0);
        }
        beamnet3->seperator_out_len =
            (int *)wtk_malloc(sizeof(int) * (cfg->seperator.outer_out_num));
        beamnet3->beamformer = qtk_onnxruntime_new(&(cfg->beamformer));
        beamnet3->beam_caches =
            wtk_calloc(sizeof(OrtValue *), beamnet3->beamformer->num_in -
                                               cfg->beamformer.outer_in_num);
        if (beamnet3->beamformer->num_in - cfg->beamformer.outer_in_num !=
            beamnet3->beamformer->num_out - cfg->beamformer.outer_out_num) {
            wtk_debug("err inner_item\n");
            exit(0);
        }
        beamnet3->beamformer_out_len =
            (int *)wtk_malloc(sizeof(int) * (cfg->seperator.outer_out_num));
    }
#endif
    beamnet3->feature = wtk_float_new_p2(cfg->feature_len, beamnet3->nbin);
    beamnet3->x =
        (float *)wtk_malloc(sizeof(float) * cfg->feature_len * beamnet3->nbin);
    beamnet3->mix_spec = (float *)wtk_malloc(
        sizeof(float) * (cfg->nmicchannel) * 2 * beamnet3->nbin);
    beamnet3->time_delay = (float *)wtk_malloc(sizeof(float) * beamnet3->nbin);
    beamnet3->mic_aptd = (float *)wtk_malloc(sizeof(float) * beamnet3->nbin);
    beamnet3->echo_aptd = (float *)wtk_malloc(sizeof(float) * beamnet3->nbin);
    beamnet3->mic_covar = wtk_complex_new_p2(cfg->out_channels, beamnet3->nbin);
    beamnet3->ideal_phase_covar =
        wtk_complex_new_p2(cfg->out_channels, beamnet3->nbin);
    beamnet3->ideal_phase_shift =
        wtk_complex_new_p2(cfg->nmicchannel, beamnet3->nbin);
    beamnet3->freq_covar =
        wtk_complex_new_p2(cfg->out_channels, beamnet3->nbin);
    beamnet3->freq_covar_sum =
        (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * beamnet3->nbin);
    beamnet3->LPS = (float *)wtk_malloc(sizeof(float) * beamnet3->nbin);
    beamnet3->echo_LPS = (float *)wtk_malloc(sizeof(float) * beamnet3->nbin);
    beamnet3->IPDs = wtk_float_new_p2(cfg->out_channels, beamnet3->nbin);
    beamnet3->FDDF = (float *)wtk_malloc(sizeof(float) * beamnet3->nbin);
    beamnet3->mean_mat =
        (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * beamnet3->nbin);
    beamnet3->input_norm = wtk_complex_new_p2(cfg->nmicchannel, beamnet3->nbin);
    beamnet3->speech_covar =
        wtk_complex_new_p2(beamnet3->nbin, cfg->nmicchannel * cfg->nmicchannel);
    beamnet3->noise_covar =
        wtk_complex_new_p2(beamnet3->nbin, cfg->nmicchannel * cfg->nmicchannel);

    beamnet3->mix_aptd = NULL;
    beamnet3->speech_aptd = NULL;
    beamnet3->noise_aptd = NULL;
    if (cfg->bf_feature_type == 1) {
        beamnet3->mix_aptd =
            (float *)wtk_malloc(sizeof(float) * beamnet3->nbin);
        beamnet3->speech_aptd =
            (float *)wtk_malloc(sizeof(float) * beamnet3->nbin);
        beamnet3->noise_aptd =
            (float *)wtk_malloc(sizeof(float) * beamnet3->nbin);
    }

    beamnet3->qmmse = NULL;
    if (cfg->use_qmmse) {
        beamnet3->qmmse = wtk_qmmse_new(&(cfg->qmmse));
    }

    beamnet3->entropy_E = NULL;
    beamnet3->entropy_Eb = NULL;
    beamnet3->last_fftx = NULL;
    if (cfg->entropy_thresh > 0) {
        beamnet3->entropy_E =
            (float *)wtk_malloc(sizeof(float) * beamnet3->nbin);
        beamnet3->entropy_Eb = (float *)wtk_malloc(sizeof(float) * cfg->wins);
        if (cfg->delay_nf > 0) {
            beamnet3->last_fftx = (wtk_complex_t *)wtk_malloc(
                sizeof(wtk_complex_t) * beamnet3->nbin * (cfg->delay_nf + 1));
        }
    }

    beamnet3->raw_mask = NULL;
    if (cfg->use_raw_mask) {
        beamnet3->raw_mask = (float *)wtk_malloc(sizeof(float) * beamnet3->nbin);
    }

    wtk_beamnet3_reset2(beamnet3);

    return beamnet3;
}

void wtk_beamnet3_delete2(wtk_beamnet3_t *beamnet3) {
    wtk_complex_delete_p2(beamnet3->fft_sp, beamnet3->cfg->nspchannel);

    wtk_free(beamnet3->fftx);
#ifdef ONNX_DEC
    if(beamnet3->cfg->use_onnx){
        {
            int n = beamnet3->seperator->num_in - beamnet3->seperator->cfg->outer_in_num;
            if (beamnet3->sep_caches[0]) {
                int i;
                for (i = 0; i < n; i++) {
                    beamnet3->seperator->api->ReleaseValue(beamnet3->sep_caches[i]);
                }
            }
        }
        if (beamnet3->seperator) {
            qtk_onnxruntime_delete(beamnet3->seperator);
        }
        wtk_free(beamnet3->seperator_out_len);
        wtk_free(beamnet3->sep_caches);
        {
            int n = beamnet3->beamformer->num_in - beamnet3->beamformer->cfg->outer_in_num;
            if (beamnet3->beam_caches[0]) {
                int i;
                for (i = 0; i < n; i++) {
                    beamnet3->beamformer->api->ReleaseValue(beamnet3->beam_caches[i]);
                }
            }
        }
        if (beamnet3->beamformer) {
            qtk_onnxruntime_delete(beamnet3->beamformer);
        }
        wtk_free(beamnet3->beamformer_out_len);
        wtk_free(beamnet3->beam_caches);
    }
#endif
    wtk_float_delete_p2(beamnet3->feature, beamnet3->cfg->feature_len);
    wtk_free(beamnet3->x);
    wtk_free(beamnet3->mix_spec);
    wtk_free(beamnet3->time_delay);
    wtk_free(beamnet3->mic_aptd);
    wtk_free(beamnet3->echo_aptd);
    wtk_complex_delete_p2(beamnet3->mic_covar, beamnet3->cfg->out_channels);
    wtk_complex_delete_p2(beamnet3->ideal_phase_covar, beamnet3->cfg->out_channels);
    wtk_complex_delete_p2(beamnet3->ideal_phase_shift, beamnet3->cfg->nmicchannel);
    wtk_complex_delete_p2(beamnet3->freq_covar, beamnet3->cfg->out_channels);
    wtk_free(beamnet3->freq_covar_sum);
    wtk_free(beamnet3->LPS);
    wtk_free(beamnet3->echo_LPS);
    wtk_float_delete_p2(beamnet3->IPDs, beamnet3->cfg->out_channels);
    wtk_free(beamnet3->FDDF);
    wtk_free(beamnet3->mean_mat);
    wtk_complex_delete_p2(beamnet3->input_norm, beamnet3->cfg->nmicchannel);
    wtk_complex_delete_p2(beamnet3->speech_covar, beamnet3->nbin);
    wtk_complex_delete_p2(beamnet3->noise_covar, beamnet3->nbin);
    if(beamnet3->mix_aptd){
        wtk_free(beamnet3->mix_aptd);
    }
    if(beamnet3->speech_aptd){
        wtk_free(beamnet3->speech_aptd);
    }
    if(beamnet3->noise_aptd){
        wtk_free(beamnet3->noise_aptd);
    }

    if(beamnet3->speech_est_real){
        wtk_free(beamnet3->speech_est_real);
    }
    if(beamnet3->speech_est_imag){
        wtk_free(beamnet3->speech_est_imag);
    }
    if(beamnet3->noise_est_real){
        wtk_free(beamnet3->noise_est_real);
    }
    if(beamnet3->noise_est_imag){
        wtk_free(beamnet3->noise_est_imag);
    }
    if(beamnet3->beamformer_x){
        wtk_free(beamnet3->beamformer_x);
    }
    if(beamnet3->src_est_real){
        wtk_free(beamnet3->src_est_real);
    }
    if(beamnet3->src_est_imag){
        wtk_free(beamnet3->src_est_imag);
    }

    if(beamnet3->qmmse){
        wtk_qmmse_delete(beamnet3->qmmse);
    }
    if(beamnet3->entropy_E){
        wtk_free(beamnet3->entropy_E);
    }
    if(beamnet3->entropy_Eb){
        wtk_free(beamnet3->entropy_Eb);
    }
    if(beamnet3->last_fftx){
        wtk_free(beamnet3->last_fftx);
    }
    if(beamnet3->gf){
        wtk_free(beamnet3->gf);
    }
    if(beamnet3->last_gf){
        wtk_free(beamnet3->last_gf);
    }
    if(beamnet3->bf){
        wtk_bf_delete(beamnet3->bf);
    }
    if(beamnet3->covm){
        wtk_covm_delete(beamnet3->covm);
    }
    if(beamnet3->sim_scov)
	{
        wtk_free(beamnet3->sim_scov);
        wtk_free(beamnet3->sim_ncov);
        wtk_free(beamnet3->sim_cnt_sum);
    }
    if(beamnet3->raw_mask)
    {
        wtk_free(beamnet3->raw_mask);
    }

    wtk_free(beamnet3);
}

void wtk_beamnet3_start2(wtk_beamnet3_t *beamnet3, float theta, float phi) {
    if(beamnet3->cfg->sep_feature_type == 5){
        int i;
        float theta1,theta2;
        if(beamnet3->cfg->query_region[0] > beamnet3->cfg->query_region[1]){
            theta1 = beamnet3->cfg->query_region[0];
            theta2 = beamnet3->cfg->query_region[1];
        }else{
            theta1 = beamnet3->cfg->query_region[1];
            theta2 = beamnet3->cfg->query_region[0];
        }
        for(i = 0; i < beamnet3->cfg->total_regions; ++i){
            wtk_beamnet3_compute_multi_channel_phase_shift2(beamnet3, beamnet3->ideal_phase_shifts[i], beamnet3->cfg->target_theta[i], phi);
            wtk_beamnet3_compute_channel_covariance(beamnet3, beamnet3->ideal_phase_shifts[i], beamnet3->ideal_phase_covars[i], beamnet3->cfg->norm_channel);
            //complex_dump(beamnet3->ideal_phase_covars[i][0], beamnet3->nbin);
            float theta = beamnet3->cfg->target_theta[i];
            if(theta > theta2 && theta < theta1){
                beamnet3->region_mask[i] = 1;
            }else{
                beamnet3->region_mask[i] = 0;
            }
        }
    }else{
        wtk_complex_t **ideal_phase_shift = beamnet3->ideal_phase_shift;
        wtk_complex_t **ideal_phase_covar = beamnet3->ideal_phase_covar;
        beamnet3->theta = theta;
        wtk_beamnet3_compute_multi_channel_phase_shift(beamnet3, ideal_phase_shift, theta, phi);
        wtk_beamnet3_compute_channel_covariance(beamnet3, ideal_phase_shift, ideal_phase_covar, beamnet3->cfg->norm_channel);
        if(beamnet3->cfg->use_bf){
            wtk_bf_update_ovec(beamnet3->bf,theta,phi);
            wtk_bf_init_w(beamnet3->bf);
        }
    }
}

void wtk_beamnet3_reset2(wtk_beamnet3_t *beamnet3) {

    wtk_complex_zero_p2(beamnet3->fft_sp, beamnet3->cfg->nspchannel,
                        (beamnet3->nbin));
    memset(beamnet3->fftx, 0, sizeof(wtk_complex_t) * (beamnet3->nbin));
#ifdef ONNX_DEC
    if (beamnet3->cfg->use_onnx) {
        qtk_onnxruntime_reset(beamnet3->seperator);
        {
            int n = beamnet3->seperator->num_in -
                    beamnet3->seperator->cfg->outer_in_num;
            if (beamnet3->sep_caches[0]) {
                int i;
                for (i = 0; i < n; i++) {
                    beamnet3->seperator->api->ReleaseValue(
                        beamnet3->sep_caches[i]);
                }
                memset(beamnet3->sep_caches, 0, sizeof(OrtValue *) * n);
            }
        }
        memset(beamnet3->seperator_out_len, 0,
               sizeof(int) * (beamnet3->seperator->cfg->outer_out_num));
        qtk_onnxruntime_reset(beamnet3->beamformer);
        {
            int n = beamnet3->beamformer->num_in -
                    beamnet3->beamformer->cfg->outer_in_num;
            if (beamnet3->beam_caches[0]) {
                int i;
                for (i = 0; i < n; i++) {
                    beamnet3->beamformer->api->ReleaseValue(
                        beamnet3->beam_caches[i]);
                }
                memset(beamnet3->beam_caches, 0, sizeof(OrtValue *) * n);
            }
        }
        memset(beamnet3->beamformer_out_len, 0,
               sizeof(int) * (beamnet3->beamformer->cfg->outer_out_num));
    }
#endif
    wtk_float_zero_p2(beamnet3->feature, beamnet3->cfg->feature_len,
                      (beamnet3->nbin));
    memset(beamnet3->x, 0,
           sizeof(float) * beamnet3->cfg->feature_len * beamnet3->nbin);
    memset(beamnet3->mix_spec, 0,
           sizeof(float) * (beamnet3->cfg->nmicchannel) * 2 * beamnet3->nbin);
    memset(beamnet3->time_delay, 0, sizeof(float) * beamnet3->nbin);
    memset(beamnet3->mic_aptd, 0, sizeof(float) * beamnet3->nbin);
    memset(beamnet3->echo_aptd, 0, sizeof(float) * beamnet3->nbin);
    wtk_complex_zero_p2(beamnet3->mic_covar, beamnet3->cfg->out_channels,
                        (beamnet3->nbin));
    wtk_complex_zero_p2(beamnet3->ideal_phase_covar,
                        beamnet3->cfg->out_channels, (beamnet3->nbin));
    wtk_complex_zero_p2(beamnet3->ideal_phase_shift, beamnet3->cfg->nmicchannel,
                        (beamnet3->nbin));
    wtk_complex_zero_p2(beamnet3->freq_covar, beamnet3->cfg->out_channels,
                        (beamnet3->nbin));
    memset(beamnet3->freq_covar_sum, 0, sizeof(float) * beamnet3->nbin);
    memset(beamnet3->LPS, 0, sizeof(float) * beamnet3->nbin);
    memset(beamnet3->echo_LPS, 0, sizeof(float) * beamnet3->nbin);
    wtk_float_zero_p2(beamnet3->IPDs, beamnet3->cfg->out_channels,
                      (beamnet3->nbin));
    memset(beamnet3->FDDF, 0, sizeof(float) * beamnet3->nbin);
    memset(beamnet3->mean_mat, 0, sizeof(wtk_complex_t) * beamnet3->nbin);
    wtk_complex_zero_p2(beamnet3->input_norm, beamnet3->cfg->nmicchannel,
                        (beamnet3->nbin));
    wtk_complex_zero_p2(beamnet3->speech_covar, beamnet3->nbin,
                        beamnet3->cfg->nmicchannel *
                            beamnet3->cfg->nmicchannel);
    wtk_complex_zero_p2(beamnet3->noise_covar, beamnet3->nbin,
                        beamnet3->cfg->nmicchannel *
                            beamnet3->cfg->nmicchannel);

    if (beamnet3->mix_aptd) {
        memset(beamnet3->mix_aptd, 0, sizeof(float) * beamnet3->nbin);
    }
    if (beamnet3->speech_aptd) {
        memset(beamnet3->speech_aptd, 0, sizeof(float) * beamnet3->nbin);
    }
    if (beamnet3->noise_aptd) {
        memset(beamnet3->noise_aptd, 0, sizeof(float) * beamnet3->nbin);
    }

    if (beamnet3->qmmse) {
        wtk_qmmse_reset(beamnet3->qmmse);
    }
    if (beamnet3->entropy_E) {
        memset(beamnet3->entropy_E, 0, sizeof(float) * beamnet3->nbin);
    }
    if (beamnet3->entropy_Eb) {
        memset(beamnet3->entropy_Eb, 0, sizeof(float) * beamnet3->nbin);
    }
    if (beamnet3->last_fftx) {
        memset(beamnet3->last_fftx, 0,
               sizeof(wtk_complex_t) * (beamnet3->nbin) *
                   (beamnet3->cfg->delay_nf + 1));
    }
    if (beamnet3->raw_mask) {
        memset(beamnet3->raw_mask, 0, sizeof(float) * beamnet3->nbin);
    }

    beamnet3->entropy_in_cnt = 0;
    beamnet3->entropy_silcnt = 0;
    beamnet3->entropy_sil = 1;

    beamnet3->delay_nf = beamnet3->cfg->delay_nf;

    beamnet3->theta = 90.0;
    beamnet3->nframe = 0;
	beamnet3->bfflushnf=beamnet3->cfg->bfflush_cnt;

	beamnet3->bs_scale=1.0;
	beamnet3->bs_last_scale=1.0;
	beamnet3->bs_real_scale=1.0;
	beamnet3->bs_max_cnt=0;
	beamnet3->gc_cnt=0;
	beamnet3->gc_cnt2=0;
}

void wtk_beamnet3_set_notify(wtk_beamnet3_t *beamnet3, void *ths,
                             wtk_beamnet3_notify_f notify) {
    beamnet3->notify = notify;
    beamnet3->ths = ths;
}

void wtk_beamnet3_set_notify2(wtk_beamnet3_t *beamnet3, void *ths2,
                             wtk_beamnet3_notify_f2 notify2) {
    beamnet3->notify2 = notify2;
    beamnet3->ths2 = ths2;
}

void wtk_beamnet3_compute_feature(wtk_beamnet3_t * beamnet3, wtk_complex_t **fft, wtk_complex_t **fft_sp, float **feature, int theta, int feature_type)
{
    int nbin = beamnet3->nbin;
    int nspchannel = beamnet3->cfg->nspchannel;
    float *mic_aptd = beamnet3->mic_aptd;
    float *echo_aptd = beamnet3->echo_aptd;
    wtk_complex_t **mic_covar = beamnet3->mic_covar;
    wtk_complex_t **ideal_phase_covar = beamnet3->ideal_phase_covar;
    wtk_complex_t **freq_covar = beamnet3->freq_covar;
    wtk_complex_t *freq_covar_sum = beamnet3->freq_covar_sum;
    float *LPS = beamnet3->LPS;
    float *echo_LPS = beamnet3->echo_LPS;
    float **IPDs = beamnet3->IPDs;
    float *FDDF = beamnet3->FDDF;
    int out_channels = beamnet3->cfg->out_channels;
    float scaler = beamnet3->cfg->scaler;
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

    if(feature_type == 5){
        wtk_beamnet3_compute_channel_covariance(beamnet3, fft, mic_covar, beamnet3->cfg->norm_channel);
        for(k=0;k<nbin;++k){
            for(i=0;i<out_channels;++i){
                tmp = sqrtf(mic_covar[i][k].a * mic_covar[i][k].a + mic_covar[i][k].b * mic_covar[i][k].b) + eps;
                mic_covar[i][k].a /= tmp;
                mic_covar[i][k].b /= tmp;
            }
        }

        for(i = 0; i < beamnet3->nbin * 4; i++) {
            beamnet3->direct_features[i] = -FLT_MAX;
        }
        float *zone_in_features_real = beamnet3->direct_features;
        float *zone_in_features_imag = zone_in_features_real + beamnet3->nbin;
        float *zone_out_features_real = zone_in_features_imag + beamnet3->nbin;
        float *zone_out_features_imag = zone_out_features_real + beamnet3->nbin;
        wtk_complex_t cpx;
        wtk_complex_t ***ideal_phase_covars = beamnet3->ideal_phase_covars;
        float *p1,*p2;
        for(i = 0; i < beamnet3->cfg->total_regions; i++) {
            if(beamnet3->region_mask[i] == 1){
                p1 = zone_out_features_real;
                p2 = zone_out_features_imag;
            }else{
                p1 = zone_in_features_real;
                p2 = zone_in_features_imag;
            }
            for(k = 0; k < beamnet3->nbin; k++) {
                cpx.a = mic_covar[0][k].a * ideal_phase_covars[i][0][k].a + mic_covar[0][k].b * -ideal_phase_covars[i][0][k].b;
                cpx.b = mic_covar[0][k].a * -ideal_phase_covars[i][0][k].b - mic_covar[0][k].b * ideal_phase_covars[i][0][k].a;
                p1[k] = p1[k] > cpx.a ? p1[k] : cpx.a;
                p2[k] = p2[k] > cpx.b ? p2[k] : cpx.b;
            }
        }
        memcpy(feature[0],mic_aptd,sizeof(float)*nbin);
        memcpy(feature[1],zone_in_features_real,sizeof(float)*nbin);
        memcpy(feature[2],zone_in_features_imag,sizeof(float)*nbin);
        memcpy(feature[3],zone_out_features_real,sizeof(float)*nbin);
        memcpy(feature[4],zone_out_features_imag,sizeof(float)*nbin);
        return;
    }

    if(feature_type==0 || feature_type==1 || feature_type==2 || feature_type==3){
        for(k=0;k<nbin;++k){
            LPS[k] = 20.0 * log10f(mic_aptd[k]);
        }
        wtk_beamnet3_compute_channel_covariance(beamnet3, fft, mic_covar, beamnet3->cfg->norm_channel);
        if(scaler!=1.0){
            for(k=0;k<nbin;++k){
                for(i=0;i<out_channels;++i){
                    mic_covar[i][k].a *= scaler;
                    mic_covar[i][k].b *= scaler;
                }
            }
        }
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
        }else if(feature_type==1 || feature_type==2 || feature_type==3){
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
            }else if(feature_type==2){
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
            }else if (feature_type==3){
                for(k=0;k<nbin;++k){
                    mic_aptd[k] = sqrtf(mic_aptd[k]);
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

        }else if(feature_type==4){
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

void wtk_beamnet3_feed_seperator(wtk_beamnet3_t *beamnet3, float *x, float *mix_spec)
{
#ifdef ONNX_DEC
    int i, j;
    const OrtApi *api = beamnet3->seperator->api;
    OrtMemoryInfo *meminfo = beamnet3->seperator->meminfo;
    qtk_onnxruntime_t *seperator = beamnet3->seperator;
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
        if(beamnet3->seperator_out_len[j]==0){
            int64_t size = 0, *out_shape;
            int d_len;
            d_len = 1;
            out_shape = qtk_onnxruntime_get_outshape(seperator, j, &size);
            for(int k=0;k<size;++k){
                d_len *= out_shape[k];
            }
            wtk_free(out_shape);
            beamnet3->seperator_out_len[j] = d_len;
        }
        if(!beamnet3->speech_est_real){
            if(j==0){
                beamnet3->speech_est_real = (float *)wtk_malloc(sizeof(float)*beamnet3->seperator_out_len[j]);
            }
        }
        if(!beamnet3->speech_est_imag){
            if(j==1){
                beamnet3->speech_est_imag = (float *)wtk_malloc(sizeof(float)*beamnet3->seperator_out_len[j]);
            }
        }
        if(!beamnet3->noise_est_real){
            if(j==2){
                beamnet3->noise_est_real = (float *)wtk_malloc(sizeof(float)*beamnet3->seperator_out_len[j]);
            }
        }
        if(!beamnet3->noise_est_imag){
            if(j==3){
                beamnet3->noise_est_imag = (float *)wtk_malloc(sizeof(float)*beamnet3->seperator_out_len[j]);
            }
        }
        onnx_out = qtk_onnxruntime_getout(seperator, j);
        if(j==0){
            memcpy(beamnet3->speech_est_real, onnx_out, beamnet3->seperator_out_len[j]*sizeof(float));
        }else if(j==1){
            memcpy(beamnet3->speech_est_imag, onnx_out, beamnet3->seperator_out_len[j]*sizeof(float));
        }else if(j==2){
            memcpy(beamnet3->noise_est_real, onnx_out, beamnet3->seperator_out_len[j]*sizeof(float));
        }else if(j==3){
            memcpy(beamnet3->noise_est_imag, onnx_out, beamnet3->seperator_out_len[j]*sizeof(float));
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

void wtk_beamnet3_feature_extract(wtk_beamnet3_t *beamnet3, float *mix_spec)
{
    float *speech_est_real=beamnet3->speech_est_real;
    float *speech_est_imag=beamnet3->speech_est_imag;
    float *noise_est_real=beamnet3->noise_est_real;
    float *noise_est_imag=beamnet3->noise_est_imag;
    wtk_complex_t **speech_covar = beamnet3->speech_covar;
    wtk_complex_t **noise_covar = beamnet3->noise_covar;
    wtk_complex_t *cov, *cov1;
    wtk_complex_t *a, *b, *a1, *b1;
    float *mix_aptd = beamnet3->mix_aptd;
    float *speech_aptd = beamnet3->speech_aptd;
    float *noise_aptd = beamnet3->noise_aptd;
    float eps = 1.1920928955078125e-07;
    float feat_scaler = beamnet3->cfg->feat_scaler;

    int k;
    int i,j;
    int nmicchannel = beamnet3->cfg->nmicchannel;
    int nbin = beamnet3->nbin;

    if(feat_scaler!=1.0){
        for(i=0;i<nmicchannel*nbin;++i){
            speech_est_real[i] *= feat_scaler;
            speech_est_imag[i] *= feat_scaler;
            noise_est_real[i] *= feat_scaler;
            noise_est_imag[i] *= feat_scaler;
        }
    }

    if(beamnet3->cfg->use_mvdr){
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
        if(beamnet3->cfg->bf_feature_type==0){
            if(!beamnet3->beamformer_x){
                beamnet3->beamformer_x = (float *)wtk_malloc(sizeof(float)*beamnet3->nbin*nmicchannel*nmicchannel*4);
                memset(beamnet3->beamformer_x, 0, sizeof(float)*beamnet3->nbin*nmicchannel*nmicchannel*4);
            }
            for(k=0;k<nbin;++k){
                cov=speech_covar[k];
                cov1=noise_covar[k];
                for(i=0;i<nmicchannel*nmicchannel;++i){
                    beamnet3->beamformer_x[i*nbin+k] = cov[i].a;
                    beamnet3->beamformer_x[(i+nmicchannel*nmicchannel)*nbin+k] = cov[i].b;
                    beamnet3->beamformer_x[(i+nmicchannel*nmicchannel*2)*nbin+k] = cov1[i].a;
                    beamnet3->beamformer_x[(i+nmicchannel*nmicchannel*3)*nbin+k] = cov1[i].b;
                }
            }
        }else if(beamnet3->cfg->bf_feature_type==1){
            if(!beamnet3->beamformer_x){
                beamnet3->beamformer_x = (float *)wtk_malloc(sizeof(float)*beamnet3->nbin*(nmicchannel*nmicchannel*4+2));
                memset(beamnet3->beamformer_x, 0, sizeof(float)*beamnet3->nbin*(nmicchannel*nmicchannel*4+2));
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
                beamnet3->beamformer_x[k] = speech_aptd[k];
                beamnet3->beamformer_x[nbin+k] = noise_aptd[k];
                for(i=0;i<nmicchannel*nmicchannel;++i){
                    cov[i].a/=mix_aptd[k];
                    cov[i].b/=mix_aptd[k];
                    cov1[i].a/=mix_aptd[k];
                    cov1[i].b/=mix_aptd[k];
                    beamnet3->beamformer_x[(i+2)*nbin+k] = cov[i].a;
                    beamnet3->beamformer_x[(i+2+nmicchannel*nmicchannel)*nbin+k] = cov[i].b;
                    beamnet3->beamformer_x[(i+2+nmicchannel*nmicchannel*2)*nbin+k] = cov1[i].a;
                    beamnet3->beamformer_x[(i+2+nmicchannel*nmicchannel*3)*nbin+k] = cov1[i].b;
                }
            }
        }
    }else{

    }
}

void wtk_beamnet3_feed_beamformer(wtk_beamnet3_t *beamnet3, float *mix_spec)
{
#ifdef ONNX_DEC
    int i, j;
    float *beamformer_x=beamnet3->beamformer_x;
    const OrtApi *api = beamnet3->beamformer->api;
    OrtMemoryInfo *meminfo = beamnet3->beamformer->meminfo;
    qtk_onnxruntime_t *beamformer = beamnet3->beamformer;
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
        if(beamnet3->beamformer_out_len[j]==0){
            int64_t size = 0, *out_shape;
            int d_len;
            d_len = 1;
            out_shape = qtk_onnxruntime_get_outshape(beamformer, j, &size);
            for(int k=0;k<size;++k){
                d_len *= out_shape[k];
            }
            wtk_free(out_shape);
            beamnet3->beamformer_out_len[j] = d_len;
        }
        if(!beamnet3->src_est_real){
            if(j==0){
                beamnet3->src_est_real = (float *)wtk_malloc(sizeof(float)*beamnet3->beamformer_out_len[j]);
            }
        }
        if(!beamnet3->src_est_imag){
            if(j==1){
                beamnet3->src_est_imag = (float *)wtk_malloc(sizeof(float)*beamnet3->beamformer_out_len[j]);
            }
        }
        onnx_out = qtk_onnxruntime_getout(beamformer, j);
        if(j==0){
            memcpy(beamnet3->src_est_real, onnx_out, beamnet3->beamformer_out_len[j]*sizeof(float));
        }else if(j==1){
            memcpy(beamnet3->src_est_imag, onnx_out, beamnet3->beamformer_out_len[j]*sizeof(float));
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
void wtk_beamnet3_feed_model(wtk_beamnet3_t *beamnet3, wtk_complex_t **fft, wtk_complex_t **fft_sp, float **feature)
{
    float *x = beamnet3->x;
    float *mix_spec = beamnet3->mix_spec;
    wtk_complex_t *fftx = beamnet3->fftx;
    int i, j;
    int nbin = beamnet3->nbin;
    int nmicchannel = beamnet3->cfg->nmicchannel;
    int feature_len = beamnet3->cfg->feature_len;

    for(i=0;i<feature_len;++i){
        memcpy(x+i*nbin, feature[i], nbin * sizeof(float));
    }

    for(i=0;i<nmicchannel;++i){
        for(j=0;j<nbin;++j){
            mix_spec[i*nbin+j] = fft[i][j].a;
            mix_spec[(nmicchannel+i)*nbin+j] = fft[i][j].b;
        }
    }
    if(beamnet3->cfg->use_onnx){
        wtk_beamnet3_feed_seperator(beamnet3, x, mix_spec);
        if(beamnet3->cfg->bf_feature_type > 1){
#ifdef ONNX_DEC
            memcpy(beamnet3->beamformer_x, beamnet3->speech_est_real,beamnet3->seperator_out_len[0]*sizeof(float));
            memcpy(beamnet3->beamformer_x+2*beamnet3->nbin, beamnet3->speech_est_imag,beamnet3->seperator_out_len[1]*sizeof(float));
            memcpy(beamnet3->beamformer_x+4*beamnet3->nbin, beamnet3->noise_est_real,beamnet3->seperator_out_len[2]*sizeof(float));
            memcpy(beamnet3->beamformer_x+6*beamnet3->nbin, beamnet3->noise_est_imag,beamnet3->seperator_out_len[3]*sizeof(float));
#endif
        }else{
            wtk_beamnet3_feature_extract(beamnet3, mix_spec);
        }
        wtk_beamnet3_feed_beamformer(beamnet3, mix_spec);
        for(i=0;i<nbin;++i){
            fftx[i].a = beamnet3->src_est_real[i];
            fftx[i].b = beamnet3->src_est_imag[i];
        }

    }else{
        memcpy(fftx, fft[0], nbin*sizeof(wtk_complex_t));
    }
}

void wtk_beamnet3_feed_decode(wtk_beamnet3_t *beamnet3, wtk_complex_t **fft, wtk_complex_t **fft_sp)
{
    float **feature = beamnet3->feature;
    wtk_beamnet3_compute_feature(beamnet3, fft, fft_sp, feature, beamnet3->theta, beamnet3->cfg->sep_feature_type);
    wtk_beamnet3_feed_model(beamnet3, fft, fft_sp, feature);
}


float wtk_beamnet3_entropy(wtk_beamnet3_t *beamnet3, wtk_complex_t *fftx, float *raw_mask)
{
    int rate = beamnet3->cfg->rate;
    int wins = beamnet3->cfg->wins;
    int nbin = beamnet3->nbin;
    int i;
    int fx1 = (250 * 1.0 * wins) / rate;
    int fx2 = (3500 * 1.0 * wins) / rate;
    int km = floor(wins * 1.0 / 8);
    float K = 0.5;
    float *E = beamnet3->entropy_E;
    float P1;
    float *Eb = beamnet3->entropy_Eb;
    float sum;
    float prob;
    float Hb;

    memset(E, 0, sizeof(float) * nbin);
    memset(Eb, 0, sizeof(float) * wins);
    qtk_vector_cpx_mag_squared(fftx + fx1, E + fx1, fx2 - fx1);
    if (raw_mask) {
        for (i = fx1; i < fx2; ++i) {
            E[i] *= raw_mask[i];
        }
    }
    sum = 1e-10;
    for (i = fx1; i < fx2; ++i) {
        sum += E[i];
    }
    for (i = fx1; i < fx2; ++i) {
        P1 = E[i] / sum;
        if (P1 >= 0.9) {
            E[i] = 0;
        }
    }
    sum = 0;
    for (i = 0; i < km; ++i) {
        Eb[i] = K;
        Eb[i] += E[i * 4 + 1] + E[i * 4 + 2] + E[i * 4 + 3] + E[i * 4 + 4];
        sum += Eb[i];
    }
    Hb = 0;
    for (i = 0; i < nbin; ++i) {
        prob = (E[i] + K) / sum;
        Hb += -prob * logf(prob + 1e-12);
    }
    // printf("%f\n", Hb);

    return Hb;
}

void wtk_beamnet3_feed_bf(wtk_beamnet3_t *beamnet3, wtk_complex_t **fft)
{
	wtk_complex_t *fftx=beamnet3->fftx;
	int k,nbin=beamnet3->nbin;
    int i;
    // int nmicchannel=beamnet3->cfg->nmicchannel;
	int nbfchannel=beamnet3->cfg->nbfchannel;
	wtk_bf_t *bf=beamnet3->bf;
    wtk_covm_t *covm;
    int b;
    float gf;
    wtk_complex_t fft2[64];
    wtk_complex_t ffts[64];
    wtk_complex_t ffty[64];
	int clip_s=beamnet3->cfg->clip_s;
	int clip_e=beamnet3->cfg->clip_e;
	int bf_clip_s=beamnet3->cfg->bf_clip_s;
	int bf_clip_e=beamnet3->cfg->bf_clip_e;
	int bfflush_cnt=beamnet3->cfg->bfflush_cnt;

	fftx[0].a=fftx[0].b=0;
	fftx[nbin-1].a=fftx[nbin-1].b=0;
	if(beamnet3->cfg->use_fixtheta)
	{
		for(k=1; k<nbin-1; ++k)
		{
			gf=beamnet3->gf[k];
			for(i=0; i<nbfchannel; ++i)
			{
				ffts[i].a=fft[i][k].a*gf;
				ffts[i].b=fft[i][k].b*gf;
            }
			wtk_bf_output_fft_k(bf, ffts, fftx+k, k);
        }
	}else if(beamnet3->cfg->use_sim_bf){
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
        scov = beamnet3->sim_scov;
        ncov = beamnet3->sim_ncov;
        cnt_sum = beamnet3->sim_cnt_sum;
        scov_alpha = beamnet3->covm->cfg->scov_alpha;
        ncov_alpha = beamnet3->covm->cfg->ncov_alpha;
        init_covnf = beamnet3->covm->cfg->init_scovnf;
        scov_alpha_1 = 1.0 - scov_alpha;
        ncov_alpha_1 = 1.0 - ncov_alpha;
		for(k=clip_s+1; k<clip_e; ++k)
		{
			gf=beamnet3->gf[k];
            if(k>=bf_clip_s && k<bf_clip_e){
                mu=beamnet3->cfg->bfmu;
            }else{
                mu=beamnet3->cfg->bfmu2;
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
			gf=beamnet3->gf[k];
            if(k>=bf_clip_s && k<bf_clip_e){
                bf->cfg->mu=beamnet3->cfg->bfmu;
            }else{
                bf->cfg->mu=beamnet3->cfg->bfmu2;
            }
            covm = beamnet3->covm;
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
			if(b==1 && beamnet3->bfflushnf==bfflush_cnt)
			{
                wtk_bf_update_w(bf, k);
            }
			wtk_bf_output_fft_k(bf, fft2, fftx+k, k);
            // fftx[k]=fft[0][k];
        }
        --beamnet3->bfflushnf;
		if(beamnet3->bfflushnf==0)
		{
			beamnet3->bfflushnf=bfflush_cnt;
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

void wtk_beamnet3_control_bs(wtk_beamnet3_t *beamnet3, float *out, int len)
{
	float *bs_win=beamnet3->bs_win;
	float max_bs_out = beamnet3->cfg->max_bs_out;
	float out_max;
	int i;

	if(beamnet3->entropy_sil==0)
	{
		out_max=wtk_float_abs_max(out, len);
		if(out_max>max_bs_out)
		{
			beamnet3->bs_scale=max_bs_out/out_max;
			if(beamnet3->bs_scale<beamnet3->bs_last_scale)
			{
				beamnet3->bs_last_scale=beamnet3->bs_scale;
			}else
			{
				beamnet3->bs_scale=beamnet3->bs_last_scale;
			}
			beamnet3->bs_max_cnt=5;
		}
		if(bs_win){
			for(i=0; i<len/2; ++i)
			{
				out[i]*=beamnet3->bs_scale * bs_win[i] + beamnet3->bs_real_scale * (1.0-bs_win[i]);
			}
			for(i=len/2; i<len; ++i){
				out[i]*=beamnet3->bs_scale;
			}
			beamnet3->bs_real_scale = beamnet3->bs_scale;
		}else{
			for(i=0; i<len; ++i){
				out[i]*=beamnet3->bs_scale;
			}
		}
		if(beamnet3->bs_max_cnt>0)
		{
			--beamnet3->bs_max_cnt;
		}
		if(beamnet3->bs_max_cnt<=0 && beamnet3->bs_scale<1.0)
		{
			beamnet3->bs_scale*=1.1f;
			beamnet3->bs_last_scale=beamnet3->bs_scale;
			if(beamnet3->bs_scale>1.0)
			{
				beamnet3->bs_scale=1.0;
				beamnet3->bs_last_scale=1.0;
			}
		}
	}else
	{
		beamnet3->bs_scale=1.0;
		beamnet3->bs_last_scale=1.0;
		beamnet3->bs_max_cnt=0;
	}
} 

void wtk_beamnet3_feed(wtk_beamnet3_t *beamnet3, short *data, int len,
                       int is_end) {
    int i, j, n, k;
    int nmicchannel = beamnet3->cfg->nmicchannel;
    int nspchannel = beamnet3->cfg->nspchannel;
    int channel = beamnet3->cfg->channel;
	int *mic_channel=beamnet3->cfg->mic_channel;
	int *sp_channel=beamnet3->cfg->sp_channel;
    int wins = beamnet3->cfg->wins;
    int fsize = wins / 2;
    int length;
    wtk_drft_t *rfft = beamnet3->rfft;
    float *rfft_in = beamnet3->rfft_in;
    wtk_complex_t **fft = beamnet3->fft;
    wtk_complex_t **fft_sp = beamnet3->fft_sp;
    wtk_complex_t *fftx = beamnet3->fftx;
    float **analysis_mem = beamnet3->analysis_mem,
          **analysis_mem_sp = beamnet3->analysis_mem_sp;
	float *synthesis_mem=beamnet3->synthesis_mem;
    float *synthesis_window=beamnet3->synthesis_window;
    float *analysis_window = beamnet3->analysis_window;
    float *out = beamnet3->out;
    short *pv = (short *)out;
    wtk_strbuf_t **mic=beamnet3->mic;
    wtk_strbuf_t **sp=beamnet3->sp;
    float fv;
    float entropy=0;
    float entropy_thresh = beamnet3->cfg->entropy_thresh;
    int entropy_cnt = beamnet3->cfg->entropy_cnt;
    float entropy_scale;
    int nbin = beamnet3->nbin;
    wtk_complex_t *last_fftx = beamnet3->last_fftx;
    float *last_gf = beamnet3->last_gf;
    int delay_nf = beamnet3->cfg->delay_nf;
	int clip_s=beamnet3->cfg->clip_s;
	int clip_e=beamnet3->cfg->clip_e;
    float *mic_scale = beamnet3->cfg->mic_scale;
    float out_scale = beamnet3->cfg->out_scale;
    float mean_gf_thresh = beamnet3->cfg->mean_gf_thresh;
    int gf_clip_s = beamnet3->cfg->gf_clip_s;
    int gf_clip_e = beamnet3->cfg->gf_clip_e;
    float *raw_mask = beamnet3->raw_mask;
    float sum_raw_mask = 0;

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
        ++beamnet3->nframe;
        for(i=0;i<nmicchannel;++i){
            wtk_drft_stft(rfft, rfft_in, analysis_mem[i], fft[i], (float *)(mic[i]->data), wins, analysis_window);
        }
        for(i=0;i<nspchannel;++i){
            wtk_drft_stft(rfft, rfft_in, analysis_mem_sp[i], fft_sp[i], (float *)(sp[i]->data), wins, analysis_window);
        }
        if(beamnet3->cfg->use_raw_mask){
            sum_raw_mask = 0;
            for(k=0;k<nbin;++k){
                raw_mask[k] = fft[0][k].a * fft[0][k].a + fft[0][k].b * fft[0][k].b;
                sum_raw_mask += raw_mask[k];
            }
            sum_raw_mask = 0;
            for(k=0;k<nbin;++k){
                raw_mask[k] *= 1.0/sum_raw_mask * nbin;
                raw_mask[k] = min(max(raw_mask[k], 1e-10), 1.0);
                sum_raw_mask += raw_mask[k];
            }
            for(k=0;k<nbin;++k){
                raw_mask[k] /= sum_raw_mask;
            }
        }

        if(beamnet3->cfg->comp_filter){
            wtk_complex_t tmp_a;
            wtk_complex_t **comp = beamnet3->cfg->comp_filter;
            for(i=0;i<nmicchannel;++i){
                for(k=0;k<nbin;++k){
                    tmp_a.a = fft[i][k].a;
                    tmp_a.b = fft[i][k].b;
                    fft[i][k].a  = tmp_a.a * comp[i][k].a - tmp_a.b * comp[i][k].b;
                    fft[i][k].b  = tmp_a.a * comp[i][k].b + tmp_a.b * comp[i][k].a;
                }
            }
        }

        wtk_beamnet3_feed_decode(beamnet3, fft, fft_sp);


        if(beamnet3->gf){
            n = beamnet3->cfg->gf_channel;
            for(i=0;i<nbin;++i){
                beamnet3->gf[i] = (fftx[i].a*fftx[i].a+fftx[i].b*fftx[i].b)/(fft[n][i].a*fft[n][i].a+fft[n][i].b*fft[n][i].b+1e-12);
                beamnet3->gf[i] = max(min(beamnet3->gf[i],1.0),0.0);
                // printf("%f\n", beamnet3->gf[i]);
            }
        }
        if(raw_mask && beamnet3->gf){
            for(k=0;k<nbin;++k){
                raw_mask[k] *= beamnet3->gf[k];
            }
        }

        float gc_mask = wtk_float_abs_mean(raw_mask+gf_clip_s, gf_clip_e-gf_clip_s);
        // printf("%f\n", gc_mask);
        if(gc_mask > beamnet3->cfg->gc_min_thresh){
			beamnet3->gc_cnt = beamnet3->cfg->gc_cnt;
		}else{
			--beamnet3->gc_cnt;
		}

        if(gc_mask > beamnet3->cfg->gc_min_thresh2){
			beamnet3->gc_cnt2 = beamnet3->cfg->gc_cnt2;
		}else{
			--beamnet3->gc_cnt2;
		}

        if(beamnet3->cfg->use_bf){
            if((beamnet3->cfg->gc_min_thresh2 > 0 && beamnet3->gc_cnt2<=0) || beamnet3->cfg->gc_min_thresh2 == 0){
                wtk_beamnet3_feed_bf(beamnet3, fft);
            }
        }

        if(entropy_thresh>0){
            entropy=wtk_beamnet3_entropy(beamnet3, fftx, raw_mask);
            // entropy=wtk_beamnet3_entropy(beamnet3, fftx, NULL);
            if(entropy<entropy_thresh && gc_mask > mean_gf_thresh){
                ++beamnet3->entropy_in_cnt;
            }else{
                beamnet3->entropy_in_cnt = 0;
            }
            if(beamnet3->entropy_in_cnt>=beamnet3->cfg->entropy_in_cnt){
                beamnet3->entropy_sil = 0;
                beamnet3->entropy_silcnt = entropy_cnt;
            }else if(beamnet3->entropy_sil==0){
                beamnet3->entropy_silcnt -= 1;
                if(beamnet3->entropy_silcnt<=0){
                    beamnet3->entropy_sil = 1;
                }
            }
        }

        if(beamnet3->denoise_enable){
            if(beamnet3->qmmse){
                // wtk_qmmse_denoise(beamnet3->qmmse, fftx);
                wtk_qmmse_feed_mask(beamnet3->qmmse, fftx, beamnet3->gf);
            }
        }

        // printf("%f\n", entropy);
        if(beamnet3->cfg->delay_nf>0){
            memcpy(last_fftx, last_fftx+nbin, delay_nf*nbin*sizeof(wtk_complex_t));
            memcpy(last_fftx+delay_nf*nbin, fftx, nbin*sizeof(wtk_complex_t));
            if(beamnet3->last_gf){
                memcpy(last_gf, last_gf+nbin, delay_nf*nbin*sizeof(float));
                memcpy(last_gf+delay_nf*nbin, beamnet3->gf, nbin*sizeof(float));
            }

            if(beamnet3->delay_nf>0){
                --beamnet3->delay_nf;
                wtk_strbufs_pop(mic, nmicchannel, fsize*sizeof(float));
                wtk_strbufs_pop(sp, nspchannel, fsize*sizeof(float));
                length = mic[0]->pos/sizeof(float);
                continue;
            }else{
                memcpy(fftx, last_fftx, nbin*sizeof(wtk_complex_t));
            }
        }
        if(entropy>entropy_thresh && beamnet3->entropy_sil==1){
            if(beamnet3->entropy_silcnt > 0){
                entropy_scale = powf(beamnet3->entropy_silcnt * 1.0/entropy_cnt, beamnet3->cfg->entropy_ratio)+beamnet3->cfg->entropy_min_scale;
            }else{
                entropy_scale = powf(1.0/entropy_cnt, beamnet3->cfg->entropy_ratio)+beamnet3->cfg->entropy_min_scale;
            }
            entropy_scale = min(entropy_scale, 1.0);
            for(i=0;i<nbin;++i){
                fftx[i].a*=entropy_scale;
                fftx[i].b*=entropy_scale;
            }
        }

        if(beamnet3->gc_cnt>=0){
            if(entropy_thresh>0){
                if(beamnet3->entropy_sil==1){
                    if(beamnet3->gf){
                        for(i=0;i<nbin;++i){
                            beamnet3->gf[i] = 0;
                        }
                    }else{
                        memset(fftx, 0, nbin*sizeof(wtk_complex_t));
                    }
                }else if(beamnet3->agc_enable){
                    if(beamnet3->gc){
                        qtk_gain_controller_run(beamnet3->gc, fftx, fsize, NULL,gc_mask);
                    }
                }
            }
        }else{
            memset(fftx, 0, sizeof(wtk_complex_t)*nbin);
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

        if(out_scale != 1.0){
            for(i=0;i<fsize;++i){
                out[i] *= out_scale;
            }
        }

		wtk_beamnet3_control_bs(beamnet3, out, fsize);

        for(i=0;i<fsize;++i){
            // pv[i] = WTK_WAV_FLOAT_TO_SHORT(out[i]);
            pv[i] = floorf(out[i]+0.5);
        }
        if(beamnet3->notify){
            beamnet3->notify(beamnet3->ths, pv, fsize);
        }

        wtk_strbufs_pop(mic, nmicchannel, fsize*sizeof(float));
        wtk_strbufs_pop(sp, nspchannel, fsize*sizeof(float));
        length = mic[0]->pos/sizeof(float);
    }
    if(is_end){
        if(beamnet3->cfg->delay_nf>0){
            memcpy(last_fftx, last_fftx+nbin, delay_nf*nbin*sizeof(wtk_complex_t));
            memcpy(last_fftx+delay_nf*nbin, fftx, nbin*sizeof(wtk_complex_t));
            if(beamnet3->last_gf){
                memcpy(last_gf, last_gf+nbin, delay_nf*nbin*sizeof(float));
                memcpy(last_gf+delay_nf*nbin, beamnet3->gf, nbin*sizeof(float));
            }
            while(beamnet3->delay_nf<beamnet3->cfg->delay_nf){
                memcpy(fftx, last_fftx, nbin*sizeof(wtk_complex_t));
                if(beamnet3->last_gf){
                    memcpy(beamnet3->gf, last_gf, nbin*sizeof(float));
                }
                if(entropy>entropy_thresh && beamnet3->entropy_sil==1){
                    if(beamnet3->entropy_silcnt > 0){
                        entropy_scale = powf(beamnet3->entropy_silcnt * 1.0/entropy_cnt, beamnet3->cfg->entropy_ratio)+beamnet3->cfg->entropy_min_scale;
                    }else{
                        entropy_scale = powf(1.0/entropy_cnt, beamnet3->cfg->entropy_ratio)+beamnet3->cfg->entropy_min_scale;
                    }
                    entropy_scale = min(entropy_scale, 1.0);
                    for(i=0;i<nbin;++i){
                        fftx[i].a*=entropy_scale;
                        fftx[i].b*=entropy_scale;
                    }
                }
                if(beamnet3->qmmse){
                    wtk_qmmse_feed_mask(beamnet3->qmmse, fftx, beamnet3->gf);
                }

                // memcpy(fftx, fft[0], nbin*sizeof(wtk_complex_t));
                wtk_drft_istft(rfft, rfft_in, synthesis_mem, fftx, out, wins, synthesis_window);

                for(i=0;i<fsize;++i){
                    pv[i] = WTK_WAV_FLOAT_TO_SHORT(out[i]);
                }
                if(beamnet3->notify){
                    beamnet3->notify(beamnet3->ths, pv, fsize);
                }
                ++beamnet3->delay_nf;
            }
        }
        if(length>0){
            if(beamnet3->notify)
            {
                out = (float *)mic[0]->data;
                pv = (short *)out;
                for(i=0;i<length;++i){
                    pv[i] = WTK_WAV_FLOAT_TO_SHORT(out[i]/mic_scale[0]);
                }
                beamnet3->notify(beamnet3->ths,pv,length);
            }
        }
    }
}

/*
fft:输入的fft数据，nbin*channel,129*4
len、is_end：暂时不用，保留接口
匹配接口：new1、reset1、notify1、start、delete
每次只输入一个固定长度为129*4的fft，不考虑len、is_end，降低控制复杂度
*/
void wtk_beamnet3_feed_fft(wtk_beamnet3_t *beamnet3, wtk_complex_t **fft,
                           int len, int is_end) {
    int i;
    wtk_complex_t **fft_sp = beamnet3->fft_sp;
    wtk_complex_t *fftx = beamnet3->fftx;
    float entropy = 0;
    float entropy_thresh = beamnet3->cfg->entropy_thresh;
    int entropy_cnt = beamnet3->cfg->entropy_cnt;
    float entropy_scale;
    int nbin = beamnet3->nbin;
    wtk_complex_t *last_fftx = beamnet3->last_fftx;
    int delay_nf = beamnet3->cfg->delay_nf;

    // ++beamnet3->nframe;

    // start
    //  save_fft(fft[1], nbin, "decode_in_fft2.txt", 1000);
    wtk_beamnet3_feed_decode(beamnet3, fft, fft_sp);

    // save_fft(beamnet3->fftx, nbin, "decode_out_fft2.txt", 1000);

    if (entropy_thresh > 0) {
        entropy = wtk_beamnet3_entropy(beamnet3, fftx, NULL);
        if (entropy < entropy_thresh) {
            ++beamnet3->entropy_in_cnt;
        } else {
            beamnet3->entropy_in_cnt = 0;
        }
        if (beamnet3->entropy_in_cnt >= beamnet3->cfg->entropy_in_cnt) {
            beamnet3->entropy_sil = 0;
            beamnet3->entropy_silcnt = entropy_cnt;
        } else if (beamnet3->entropy_sil == 0) {
            beamnet3->entropy_silcnt -= 1;
            if (beamnet3->entropy_silcnt <= 0) {
                beamnet3->entropy_sil = 1;
            }
        }
    }
    if (beamnet3->cfg->delay_nf > 0) {
        memcpy(last_fftx, last_fftx + nbin,
               delay_nf * nbin * sizeof(wtk_complex_t));
        memcpy(last_fftx + delay_nf * nbin, fftx, nbin * sizeof(wtk_complex_t));
    }
    if (entropy > entropy_thresh) {
        if (beamnet3->entropy_silcnt > 0) {
            entropy_scale = powf(beamnet3->entropy_silcnt * 1.0 / entropy_cnt,
                                 beamnet3->cfg->entropy_ratio) +
                            beamnet3->cfg->entropy_min_scale;
        } else {
            entropy_scale =
                powf(1.0 / entropy_cnt, beamnet3->cfg->entropy_ratio) +
                beamnet3->cfg->entropy_min_scale;
        }
        entropy_scale = min(entropy_scale, 1.0);
        for (i = 0; i < nbin; ++i) {
            fftx[i].a *= entropy_scale;
            fftx[i].b *= entropy_scale;
        }
    }
    if (beamnet3->qmmse) {
        wtk_qmmse_denoise(beamnet3->qmmse, fftx);
    }

    // save_fft(fftx,129,"fftx.txt",1000);
    // for(int i=0;i<nbin;++i){
    //     fftx[i].a*=4;
    //     fftx[i].b*=4;
    // }
    // save_fft(fftx,129,"model_out_fft2.txt",1000);

    // end
    if (beamnet3->notify2) {
        beamnet3->notify2(beamnet3->ths2, fftx, nbin);
    }
}


void wtk_beamnet3_set_denoiseenable(wtk_beamnet3_t *beamnet3,int enable)
{
    beamnet3->denoise_enable = enable;
}
void wtk_beamnet3_set_noise_suppress(wtk_beamnet3_t *beamnet3,float noise_suppress)
{
    if(beamnet3->qmmse){
        wtk_qmmse_cfg_set_noise_suppress(beamnet3->qmmse->cfg, noise_suppress);
    }
}
void wtk_beamnet3_set_out_scale(wtk_beamnet3_t *beamnet3,float scale)
{
    beamnet3->cfg->out_scale = scale;
}
void wtk_beamnet3_set_agcenable(wtk_beamnet3_t *beamnet3,int enable)
{
    beamnet3->agc_enable = enable;
}

float theta_map[19][2] = {
    {0,-10.0},{10,0.0},{20.0,7.5},{30.0,20.0},{40,27.5},{50,40},
    {60,47.5},{70,57.5},{80,65},{90,70},{-90,-75},{-80,-75},
    {-70,-70},{-60,-62.5},{-50,-55},{-40,-45},{-30,-37.5},{-20,-27.5},{-10,-17.5}
};

int wtk_beamnet3_set_query_region(wtk_beamnet3_t *beamnet3,float theta1, float theta2){
    if(beamnet3->cfg->sep_feature_type == 5 && beamnet3->cfg->query_region){
        int i;
        for(i = 0; i < 19; i++){
            if(fabs(theta1 - theta_map[i][0]) < 1e-4){
                theta1 = theta_map[i][1];
            }
            if(fabs(theta2 - theta_map[i][0]) < 1e-4){
                theta2 = theta_map[i][1];
            }
        }
        float theta = PI * theta1/ 180.0;
        beamnet3->cfg->query_region[0] = theta;
        theta = PI * theta2/ 180.0;
        beamnet3->cfg->query_region[1] = theta;
        wtk_beamnet3_start(beamnet3, 0, 0);
        return 0;
    }else{
        return -1;
    }
}