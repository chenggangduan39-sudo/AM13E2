#include "wtk_cmask_bfse.h"
#include "qtk/math/qtk_vector.h"
#ifndef WTK_WAV_SHORT_TO_FLOAT
#define WTK_WAV_SHORT_TO_FLOAT(f) ((f) > 0? (f/32767.0) : (f/32768.0))
#endif
#ifndef WTK_WAV_FLOAT_TO_SHORT
#define WTK_WAV_FLOAT_TO_SHORT(f) ((f) > 0? floorf(f*32767.0+0.5) : floorf(f*32768.0+0.5))
#endif

static void _compute_channel_covariance(wtk_cmask_bfse_t *cmask_bfse, wtk_complex_t **fft, wtk_complex_t **covar_mat, int norm){
    int nmicchannel = cmask_bfse->cfg->nmicchannel;
    int nbin = cmask_bfse->nbin;
    int i, j, k;
    wtk_complex_t *mean_mat = cmask_bfse->mean_mat;
    wtk_complex_t **input_norm = cmask_bfse->input_norm;
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

static void _compute_multi_channel_phase_shift(wtk_cmask_bfse_t *cmask_bfse){
    int wins = cmask_bfse->cfg->wins;
    int nmicchannel = cmask_bfse->cfg->nmicchannel;
    int nbin = cmask_bfse->nbin;
    int rate = cmask_bfse->cfg->rate;
    float **mic_pos = cmask_bfse->cfg->mic_pos;
    float sv = cmask_bfse->cfg->sv;
    float **time_delay = cmask_bfse->time_delay;
    wtk_complex_t ***phase_shift = cmask_bfse->phase_shift;
    float *theta = cmask_bfse->cfg->theta;
    float th;
    float ntheta = cmask_bfse->cfg->ntheta;
    float *mic;
    int i, j, k;
    float x, y, z;
    float t;
    float phi=0;

    phi = phi * PI / 180.0;
    for(i=0;i<ntheta;++i){
        th = theta[i] * PI / 180.0;
        x = cos(phi) * cos(th);
        y = cos(phi) * sin(th);
        z = sin(phi);
        for(j=0;j<nmicchannel;++j){
            mic = mic_pos[j];
            time_delay[i][j] = (mic[0] * x + mic[1] * y + mic[2] * z)/sv;
        }
        for(k=0;k<nbin;++k){
            t=2*PI*rate*1.0/wins*k;
            for(j=0;j<nmicchannel;++j){
                phase_shift[i][j][k].a = cosf(t*time_delay[i][j]);
                phase_shift[i][j][k].b = -sinf(t*time_delay[i][j]);
            }
        }
    }
}

static void _compute_feature(wtk_cmask_bfse_t *cmask_bfse, wtk_complex_t **fft){
    float **mic_mag = cmask_bfse->mic_mag;
    float **LPS = cmask_bfse->LPS;
    wtk_complex_t **mic_covar = cmask_bfse->mic_covar;
    wtk_complex_t ***ideal_phase_covar = cmask_bfse->ideal_phase_covar;
    wtk_complex_t ***freq_covar = cmask_bfse->freq_covar;
    wtk_complex_t **freq_covar_sum = cmask_bfse->freq_covar_sum;
    float *feat = cmask_bfse->feat;
    int nmicchannel = cmask_bfse->cfg->nmicchannel;
    int out_channels = cmask_bfse->cfg->out_channels;
    int ntheta = cmask_bfse->cfg->ntheta;
    int nbin = cmask_bfse->nbin;
    float tmp;
    int i, j, k;

    for(i=0;i<nmicchannel;++i){
        for(k=0;k<nbin;++k){
            mic_mag[i][k] = sqrtf(fft[i][k].a*fft[i][k].a + fft[i][k].b*fft[i][k].b)+1e-8;
            LPS[i][k] = 20.0*log10f(mic_mag[i][k]);
        }
    }
    _compute_channel_covariance(cmask_bfse, fft, mic_covar, cmask_bfse->cfg->use_norm);
    for(i=0;i<ntheta;++i){
        for(j=0;j<out_channels;++j){
            for(k=0;k<nbin;++k){
                freq_covar[i][j][k].a = mic_covar[j][k].a * ideal_phase_covar[i][j][k].a + mic_covar[j][k].b * ideal_phase_covar[i][j][k].b;
                freq_covar[i][j][k].b = mic_covar[j][k].b * ideal_phase_covar[i][j][k].a - mic_covar[j][k].a * ideal_phase_covar[i][j][k].b;
            }
        }
    }
    for(i=0;i<out_channels;++i){
        for(k=0;k<nbin;++k){
            tmp = sqrtf(mic_covar[i][k].a*mic_covar[i][k].a + mic_covar[i][k].b*mic_covar[i][k].b)+1e-8;
            mic_covar[i][k].a = mic_covar[i][k].a / tmp;
            mic_covar[i][k].b = mic_covar[i][k].b / tmp;
        }
    }
    for(i=0;i<ntheta;++i){
        memset(freq_covar_sum[i], 0, nbin * sizeof(wtk_complex_t));
        for(k=0;k<nbin;++k){
            for(j=0;j<out_channels;++j){
                freq_covar_sum[i][k].a += freq_covar[i][j][k].a;
                freq_covar_sum[i][k].b += freq_covar[i][j][k].b;
            }
            tmp = sqrtf(freq_covar_sum[i][k].a*freq_covar_sum[i][k].a + freq_covar_sum[i][k].b*freq_covar_sum[i][k].b)+1e-8;
            freq_covar_sum[i][k].a = freq_covar_sum[i][k].a / tmp;
            freq_covar_sum[i][k].b = freq_covar_sum[i][k].b / tmp;
        }
    }

    for(i=0;i<nmicchannel;++i){
        memcpy(feat+i*nbin, LPS[i], nbin * sizeof(float));
    }
    for(k=0;k<nbin;++k){
        for(i=0;i<out_channels;++i){
            feat[(i+nmicchannel)*nbin+k] = mic_covar[i][k].a;
            feat[(i+nmicchannel+out_channels)*nbin+k] = mic_covar[i][k].b;
        }
        for(i=0;i<ntheta;++i){
            feat[(nmicchannel+out_channels*2+i*2)*nbin+k] = freq_covar_sum[i][k].a;
            feat[(nmicchannel+out_channels*2+1+i*2)*nbin+k] = freq_covar_sum[i][k].b;
        }
    }
}

wtk_cmask_bfse_t *wtk_cmask_bfse_new(wtk_cmask_bfse_cfg_t *cfg) {
    wtk_cmask_bfse_t *cmask_bfse;
    int i;

    cmask_bfse = (wtk_cmask_bfse_t *)wtk_malloc(sizeof(wtk_cmask_bfse_t));
    cmask_bfse->cfg = cfg;
    cmask_bfse->ths = NULL;
    cmask_bfse->notify = NULL;
    cmask_bfse->mic = wtk_strbufs_new(cmask_bfse->cfg->nmicchannel);
    cmask_bfse->sp = wtk_strbufs_new(cmask_bfse->cfg->nspchannel);

    cmask_bfse->nbin = cfg->wins / 2 + 1;
    cmask_bfse->analysis_window = wtk_malloc(sizeof(float) * cfg->wins);  /// 2);
    cmask_bfse->synthesis_window = wtk_malloc(sizeof(float) * cfg->wins); /// 2);
    cmask_bfse->analysis_mem = wtk_float_new_p2(cfg->nmicchannel, cmask_bfse->nbin - 1);
    cmask_bfse->analysis_mem_sp = wtk_float_new_p2(cfg->nspchannel, cmask_bfse->nbin - 1);
    cmask_bfse->synthesis_mem = wtk_float_new_p2(cfg->ntheta, cmask_bfse->nbin - 1);
    cmask_bfse->rfft = wtk_drft_new(cfg->wins);
    cmask_bfse->rfft_in = (float *)wtk_malloc(sizeof(float) * (cfg->wins));

    cmask_bfse->fft = wtk_complex_new_p2(cfg->nmicchannel, cmask_bfse->nbin);
    cmask_bfse->fft_sp = wtk_complex_new_p2(cfg->nspchannel, cmask_bfse->nbin);
    cmask_bfse->fftx = wtk_complex_new_p2(cfg->ntheta, cmask_bfse->nbin);

    cmask_bfse->ffttmp=NULL;
    if(cfg->use_trick)
    {
        cmask_bfse->ffttmp=wtk_complex_new_p2(cfg->ntheta, cmask_bfse->nbin);
    }

    cmask_bfse->n_out = wtk_float_new_p2(cfg->ntheta, cmask_bfse->nbin - 1);
    cmask_bfse->out = wtk_malloc(sizeof(float) * (cmask_bfse->nbin - 1) * cfg->ntheta);

#ifdef ONNX_DEC
    cmask_bfse->sep = NULL;
    cmask_bfse->sep_caches = NULL;
    cmask_bfse->sep_out_len = NULL;
    if(cfg->use_onnx){
        cmask_bfse->sep = qtk_onnxruntime_new(&(cfg->sep));
        cmask_bfse->sep_caches = wtk_calloc(sizeof(OrtValue *), cmask_bfse->sep->num_in - cfg->sep.outer_in_num);
        if (cmask_bfse->sep->num_in - cfg->sep.outer_in_num != cmask_bfse->sep->num_out - cfg->sep.outer_out_num) {
            wtk_debug("err inner_item num_in:%ld outer_in_num:%d num_out:%ld outer_out_num:%d\n",cmask_bfse->sep->num_in,cmask_bfse->sep->cfg->outer_in_num,cmask_bfse->sep->num_out,cmask_bfse->sep->cfg->outer_out_num);
            exit(0);
        }
        cmask_bfse->sep_out_len = (int *)wtk_malloc(sizeof(int) * (cfg->sep.outer_out_num));
    }
    cmask_bfse->bfse = NULL;
    cmask_bfse->bfse_caches = NULL;
    cmask_bfse->bfse_out_len = NULL;
    if(cfg->use_onnx){
        cmask_bfse->bfse = qtk_onnxruntime_new(&(cfg->bfse));
        cmask_bfse->bfse_caches = wtk_calloc(sizeof(OrtValue *), cmask_bfse->bfse->num_in - cfg->bfse.outer_in_num);
        if (cmask_bfse->bfse->num_in - cfg->bfse.outer_in_num != cmask_bfse->bfse->num_out - cfg->bfse.outer_out_num) {
            wtk_debug("err inner_item num_in:%ld outer_in_num:%d num_out:%ld outer_out_num:%d\n",cmask_bfse->bfse->num_in,cmask_bfse->bfse->cfg->outer_in_num,cmask_bfse->bfse->num_out,cmask_bfse->bfse->cfg->outer_out_num);
            exit(0);
        }
        cmask_bfse->bfse_out_len = (int *)wtk_malloc(sizeof(int) * (cfg->bfse.outer_out_num));
    }
#endif
    cmask_bfse->covm=NULL;
    cmask_bfse->bf=NULL;
    if(cfg->use_bf){
        cmask_bfse->covm = (wtk_covm_t **)wtk_malloc(sizeof(wtk_covm_t *) * cfg->ntheta);
        cmask_bfse->bf = (wtk_bf_t **)wtk_malloc(sizeof(wtk_bf_t *) * cfg->ntheta);
        for(i=0;i<cfg->ntheta;++i){
            cmask_bfse->covm[i]=wtk_covm_new(&(cfg->covm),cmask_bfse->nbin,cfg->nbfchannel);
            cmask_bfse->bf[i]=wtk_bf_new(&(cfg->bf),cfg->wins);
        }
    }
	cmask_bfse->qmmse=NULL;
	if(cfg->use_qmmse)
	{
        cmask_bfse->qmmse = (wtk_qmmse_t **)wtk_malloc(sizeof(wtk_qmmse_t *) * cfg->ntheta);
        for(i=0;i<cfg->ntheta;++i){
            cmask_bfse->qmmse[i]=wtk_qmmse_new(&(cfg->qmmse));
        }
	}

    cmask_bfse->eq = NULL;
    if (cfg->use_eq) {
        cmask_bfse->eq = (wtk_equalizer_t **)wtk_malloc(sizeof(wtk_equalizer_t *) * cfg->ntheta);
        for(i=0;i<cfg->ntheta;++i){
            cmask_bfse->eq[i] = wtk_equalizer_new(&(cfg->eq));
        }
    }
    cmask_bfse->entropy_E=(float *)wtk_malloc(sizeof(float)*cmask_bfse->nbin);
    cmask_bfse->entropy_Eb=(float *)wtk_malloc(sizeof(float)*cfg->wins);
    cmask_bfse->time_delay = wtk_float_new_p2(cfg->ntheta, cfg->nmicchannel);
    cmask_bfse->phase_shift = wtk_complex_new_p3(cfg->ntheta, cfg->nmicchannel, cmask_bfse->nbin);
    cmask_bfse->mic_mag = wtk_float_new_p2(cfg->nmicchannel, cmask_bfse->nbin);
    cmask_bfse->LPS = wtk_float_new_p2(cfg->nmicchannel, cmask_bfse->nbin);
    cmask_bfse->mic_covar = wtk_complex_new_p2(cfg->out_channels, cmask_bfse->nbin);
    cmask_bfse->ideal_phase_covar = wtk_complex_new_p3(cfg->ntheta, cfg->out_channels, cmask_bfse->nbin);
    cmask_bfse->freq_covar = wtk_complex_new_p3(cfg->ntheta, cfg->out_channels, cmask_bfse->nbin);
    cmask_bfse->freq_covar_sum = wtk_complex_new_p2(cfg->ntheta, cmask_bfse->nbin);

    cmask_bfse->mean_mat = (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * cmask_bfse->nbin);
    cmask_bfse->input_norm = wtk_complex_new_p2(cfg->nmicchannel, cmask_bfse->nbin);
    cmask_bfse->feat = (float *)wtk_malloc(sizeof(float)*cfg->feat_len*cmask_bfse->nbin);
    cmask_bfse->s_real = (float *)wtk_malloc(sizeof(float) * cmask_bfse->nbin * cfg->nmicchannel);
    cmask_bfse->s_imag = (float *)wtk_malloc(sizeof(float) * cmask_bfse->nbin * cfg->nmicchannel);
    cmask_bfse->n_real = (float *)wtk_malloc(sizeof(float) * cmask_bfse->nbin * cfg->nmicchannel);
    cmask_bfse->n_imag = (float *)wtk_malloc(sizeof(float) * cmask_bfse->nbin * cfg->nmicchannel);
    cmask_bfse->s_cov = (float *)wtk_malloc(sizeof(float) * cmask_bfse->nbin * cfg->nmicchannel * cfg->nmicchannel * 2);
    cmask_bfse->n_cov = (float *)wtk_malloc(sizeof(float) * cmask_bfse->nbin * cfg->nmicchannel * cfg->nmicchannel * 2);

    cmask_bfse->bs_scale = (float *)wtk_malloc(sizeof(float) * cfg->ntheta);
    cmask_bfse->bs_last_scale = (float *)wtk_malloc(sizeof(float) * cfg->ntheta);
    cmask_bfse->bs_max_cnt = (int *)wtk_malloc(sizeof(int) * cfg->ntheta);
    cmask_bfse->mic_silcnt = (int *)wtk_malloc(sizeof(int) * cfg->ntheta);
    cmask_bfse->entropy_silcnt = (int *)wtk_malloc(sizeof(int) * cfg->ntheta);
    cmask_bfse->entropy_in_cnt = (int *)wtk_malloc(sizeof(int) * cfg->ntheta);
    cmask_bfse->mic_sil = (int *)wtk_malloc(sizeof(int) * cfg->ntheta);
    cmask_bfse->entropy_sil = (int *)wtk_malloc(sizeof(int) * cfg->ntheta);

    cmask_bfse->aspec=NULL;
    cmask_bfse->cov=NULL;
    cmask_bfse->inv_cov=NULL;
    cmask_bfse->tmp=NULL;
    cmask_bfse->cohv=NULL;
    cmask_bfse->gcc_fft=NULL;
    cmask_bfse->q_fring=NULL;
	cmask_bfse->qmmse2=NULL;
    if (cfg->use_aspec){
        cmask_bfse->aspec = (wtk_aspec_t **)wtk_malloc(sizeof(wtk_aspec_t *) * cfg->ntheta);
        cmask_bfse->q_fring = (wtk_fring_t **)wtk_malloc(sizeof(wtk_fring_t *) * cfg->ntheta);
        cmask_bfse->qmmse2 = (wtk_qmmse_t **)wtk_malloc(sizeof(wtk_qmmse_t *) * cfg->ntheta);
        for(i=0;i<cfg->ntheta;++i){
            cmask_bfse->aspec[i]=wtk_aspec_new(&(cfg->aspec), cmask_bfse->nbin, 3);
            cmask_bfse->q_fring[i]=wtk_fring_new(cfg->q_nf);
            cmask_bfse->qmmse2[i]=wtk_qmmse_new(&(cfg->qmmse2));
        }
        cmask_bfse->cohv=wtk_float_new_p2(cfg->ntheta,cmask_bfse->nbin);
        cmask_bfse->cov=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cmask_bfse->cfg->nmicchannel*cmask_bfse->cfg->nmicchannel);
        if(cmask_bfse->aspec[0]->need_inv_cov){
            cmask_bfse->inv_cov=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cmask_bfse->cfg->nmicchannel*cmask_bfse->cfg->nmicchannel);
            cmask_bfse->tmp=(wtk_dcomplex_t *)wtk_malloc(sizeof(wtk_dcomplex_t)*cmask_bfse->cfg->nmicchannel*cmask_bfse->cfg->nmicchannel*2);
        }
        if(cmask_bfse->aspec[0]->gccspec || cmask_bfse->aspec[0]->gccspec2){
            cmask_bfse->gcc_fft = wtk_complex_new_p2(cmask_bfse->nbin, cfg->nmicchannel);
        }
        cmask_bfse->specsum = (float *)wtk_malloc(sizeof(float) * cfg->ntheta);
        cmask_bfse->right_nf = (int *)wtk_malloc(sizeof(int) * cfg->ntheta);
        cmask_bfse->q_spec = (float *)wtk_malloc(sizeof(float) * cfg->ntheta);
    }

    wtk_cmask_bfse_reset(cmask_bfse);

    return cmask_bfse;
}

void wtk_cmask_bfse_reset(wtk_cmask_bfse_t *cmask_bfse) {
    int wins = cmask_bfse->cfg->wins;
    int i;

    wtk_strbufs_reset(cmask_bfse->mic, cmask_bfse->cfg->nmicchannel);
    wtk_strbufs_reset(cmask_bfse->sp, cmask_bfse->cfg->nspchannel);

    for (i = 0; i < wins; ++i) {
        cmask_bfse->analysis_window[i] = sin((0.5 + i) * PI / (wins));
    }
    wtk_drft_init_synthesis_window(cmask_bfse->synthesis_window,
                                   cmask_bfse->analysis_window, wins);

    wtk_float_zero_p2(cmask_bfse->analysis_mem, cmask_bfse->cfg->nmicchannel,
                      (cmask_bfse->nbin - 1));
    wtk_float_zero_p2(cmask_bfse->analysis_mem_sp, cmask_bfse->cfg->nspchannel,
                      (cmask_bfse->nbin - 1));
    wtk_float_zero_p2(cmask_bfse->synthesis_mem, cmask_bfse->cfg->ntheta,
                      (cmask_bfse->nbin - 1));

    wtk_complex_zero_p2(cmask_bfse->fft, cmask_bfse->cfg->nmicchannel, cmask_bfse->nbin);
    wtk_complex_zero_p2(cmask_bfse->fft_sp, cmask_bfse->cfg->nspchannel, cmask_bfse->nbin);
    wtk_complex_zero_p2(cmask_bfse->fftx, cmask_bfse->cfg->ntheta, cmask_bfse->nbin);
    if(cmask_bfse->ffttmp)
    {
        wtk_complex_zero_p2(cmask_bfse->ffttmp, cmask_bfse->cfg->ntheta, cmask_bfse->nbin);
    }
#ifdef ONNX_DEC
    if(cmask_bfse->cfg->use_onnx){
        qtk_onnxruntime_reset(cmask_bfse->sep);
        {
            int n = cmask_bfse->sep->num_in - cmask_bfse->sep->cfg->outer_in_num;
            if (cmask_bfse->sep_caches[0]) {
                int i;
                for (i = 0; i < n; i++) {
                    cmask_bfse->sep->api->ReleaseValue(cmask_bfse->sep_caches[i]);
                }
                memset(cmask_bfse->sep_caches, 0, sizeof(OrtValue *) * n);
            }
        }
        memset(cmask_bfse->sep_out_len, 0, sizeof(int) * (cmask_bfse->sep->cfg->outer_out_num));
        qtk_onnxruntime_reset(cmask_bfse->bfse);
        {
            int n = cmask_bfse->bfse->num_in - cmask_bfse->bfse->cfg->outer_in_num;
            if (cmask_bfse->bfse_caches[0]) {
                int i;
                for (i = 0; i < n; i++) {
                    cmask_bfse->bfse->api->ReleaseValue(cmask_bfse->bfse_caches[i]);
                }
                memset(cmask_bfse->bfse_caches, 0, sizeof(OrtValue *) * n);
            }
        }
        memset(cmask_bfse->bfse_out_len, 0, sizeof(int) * (cmask_bfse->bfse->cfg->outer_out_num));
    }
#endif
    if(cmask_bfse->covm){
        for(i=0;i<cmask_bfse->cfg->ntheta;++i){
            wtk_covm_reset(cmask_bfse->covm[i]);
        }
    }
    if(cmask_bfse->bf){
        for(i=0;i<cmask_bfse->cfg->ntheta;++i){
            wtk_bf_reset(cmask_bfse->bf[i]);
        }
    }
    if(cmask_bfse->qmmse){
        for(i=0;i<cmask_bfse->cfg->ntheta;++i){
            wtk_qmmse_reset(cmask_bfse->qmmse[i]);
        }
    }
    if (cmask_bfse->eq) {
        for(i=0;i<cmask_bfse->cfg->ntheta;++i){
            wtk_equalizer_reset(cmask_bfse->eq[i]);
        }
    }
    memset(cmask_bfse->entropy_E, 0, sizeof(float) * cmask_bfse->nbin);
    memset(cmask_bfse->entropy_Eb, 0, sizeof(float) * cmask_bfse->cfg->wins);
    wtk_float_zero_p2(cmask_bfse->time_delay, cmask_bfse->cfg->ntheta, cmask_bfse->cfg->nmicchannel);
    wtk_complex_zero_p3(cmask_bfse->phase_shift, cmask_bfse->cfg->ntheta, cmask_bfse->cfg->nmicchannel, cmask_bfse->nbin);
    wtk_float_zero_p2(cmask_bfse->mic_mag, cmask_bfse->cfg->nmicchannel, cmask_bfse->nbin);
    wtk_float_zero_p2(cmask_bfse->LPS, cmask_bfse->cfg->nmicchannel, cmask_bfse->nbin);
    wtk_complex_zero_p2(cmask_bfse->mic_covar, cmask_bfse->cfg->out_channels, cmask_bfse->nbin);
    wtk_complex_zero_p3(cmask_bfse->ideal_phase_covar, cmask_bfse->cfg->ntheta, cmask_bfse->cfg->out_channels, cmask_bfse->nbin);
    wtk_complex_zero_p3(cmask_bfse->freq_covar, cmask_bfse->cfg->ntheta, cmask_bfse->cfg->out_channels, cmask_bfse->nbin);
    wtk_complex_zero_p2(cmask_bfse->freq_covar_sum, cmask_bfse->cfg->ntheta, cmask_bfse->nbin);
    memset(cmask_bfse->mean_mat, 0, sizeof(wtk_complex_t) * cmask_bfse->nbin);
    wtk_complex_zero_p2(cmask_bfse->input_norm, cmask_bfse->cfg->nmicchannel, cmask_bfse->nbin);
    memset(cmask_bfse->feat, 0, sizeof(float)*cmask_bfse->cfg->feat_len*cmask_bfse->nbin);
    memset(cmask_bfse->s_real, 0, sizeof(float) * cmask_bfse->nbin * cmask_bfse->cfg->nmicchannel);
    memset(cmask_bfse->s_imag, 0, sizeof(float) * cmask_bfse->nbin * cmask_bfse->cfg->nmicchannel);
    memset(cmask_bfse->n_real, 0, sizeof(float) * cmask_bfse->nbin * cmask_bfse->cfg->nmicchannel);
    memset(cmask_bfse->n_imag, 0, sizeof(float) * cmask_bfse->nbin * cmask_bfse->cfg->nmicchannel);
    memset(cmask_bfse->s_cov, 0, sizeof(float) * cmask_bfse->nbin * cmask_bfse->cfg->nmicchannel * cmask_bfse->cfg->nmicchannel * 2);
    memset(cmask_bfse->n_cov, 0, sizeof(float) * cmask_bfse->nbin * cmask_bfse->cfg->nmicchannel * cmask_bfse->cfg->nmicchannel * 2);

    for(i=0;i<cmask_bfse->cfg->ntheta;++i){
        cmask_bfse->bs_scale[i] = 1.0;
        cmask_bfse->bs_last_scale[i] = 1.0;
        cmask_bfse->bs_max_cnt[i] = 0;

        cmask_bfse->entropy_in_cnt[i] = 0;
        cmask_bfse->entropy_silcnt[i] = 0;
        cmask_bfse->entropy_sil[i] = 1;
        cmask_bfse->mic_silcnt[i] = 0;
        cmask_bfse->mic_sil[i] = 1;
    }

    if(cmask_bfse->aspec){
        for(i=0;i<cmask_bfse->cfg->ntheta;++i){
            wtk_aspec_reset(cmask_bfse->aspec[i]);
            wtk_fring_reset(cmask_bfse->q_fring[i]);
            wtk_qmmse_reset(cmask_bfse->qmmse2[i]);
        }
        wtk_float_zero_p2(cmask_bfse->cohv, cmask_bfse->cfg->ntheta, cmask_bfse->nbin);
        memset(cmask_bfse->cov, 0, sizeof(wtk_complex_t)*cmask_bfse->cfg->nmicchannel*cmask_bfse->cfg->nmicchannel);
        if(cmask_bfse->inv_cov){
            memset(cmask_bfse->inv_cov, 0, sizeof(wtk_complex_t)*cmask_bfse->cfg->nmicchannel*cmask_bfse->cfg->nmicchannel);
            memset(cmask_bfse->tmp, 0, sizeof(wtk_dcomplex_t)*cmask_bfse->cfg->nmicchannel*cmask_bfse->cfg->nmicchannel*2);
        }
        if(cmask_bfse->gcc_fft){
            wtk_complex_zero_p2(cmask_bfse->gcc_fft, cmask_bfse->nbin, cmask_bfse->cfg->nmicchannel);
        }
        memset(cmask_bfse->specsum, 0, sizeof(float)*cmask_bfse->cfg->ntheta);
        memset(cmask_bfse->right_nf, 0, sizeof(int)*cmask_bfse->cfg->ntheta);
        memset(cmask_bfse->q_spec, 0, sizeof(float)*cmask_bfse->cfg->ntheta);
    }
}

void wtk_cmask_bfse_delete(wtk_cmask_bfse_t *cmask_bfse) {
    int i;
    wtk_strbufs_delete(cmask_bfse->mic, cmask_bfse->cfg->nmicchannel);
    wtk_strbufs_delete(cmask_bfse->sp, cmask_bfse->cfg->nspchannel);

    wtk_free(cmask_bfse->analysis_window);
    wtk_free(cmask_bfse->synthesis_window);
    wtk_float_delete_p2(cmask_bfse->analysis_mem, cmask_bfse->cfg->nmicchannel);
    wtk_float_delete_p2(cmask_bfse->analysis_mem_sp, cmask_bfse->cfg->nspchannel);
    wtk_float_delete_p2(cmask_bfse->synthesis_mem, cmask_bfse->cfg->ntheta);
    wtk_free(cmask_bfse->rfft_in);
    wtk_drft_delete(cmask_bfse->rfft);
    wtk_complex_delete_p2(cmask_bfse->fft, cmask_bfse->cfg->nmicchannel);
    wtk_complex_delete_p2(cmask_bfse->fft_sp, cmask_bfse->cfg->nspchannel);
    wtk_complex_delete_p2(cmask_bfse->fftx, cmask_bfse->cfg->ntheta);
    if(cmask_bfse->ffttmp)
    {
        wtk_complex_delete_p2(cmask_bfse->ffttmp, cmask_bfse->cfg->ntheta);
    }

    wtk_float_delete_p2(cmask_bfse->n_out, cmask_bfse->cfg->ntheta);
    wtk_free(cmask_bfse->out);
#ifdef ONNX_DEC
    if(cmask_bfse->cfg->use_onnx){
        {
            int n = cmask_bfse->sep->num_in - cmask_bfse->sep->cfg->outer_in_num;
            if (cmask_bfse->sep_caches[0]) {
                int i;
                for (i = 0; i < n; i++) {
                    cmask_bfse->sep->api->ReleaseValue(cmask_bfse->sep_caches[i]);
                }
            }
        }
        if (cmask_bfse->sep) {
            qtk_onnxruntime_delete(cmask_bfse->sep);
        }
        wtk_free(cmask_bfse->sep_out_len);
        wtk_free(cmask_bfse->sep_caches);
        {
            int n = cmask_bfse->bfse->num_in - cmask_bfse->bfse->cfg->outer_in_num;
            if (cmask_bfse->bfse_caches[0]) {
                int i;
                for (i = 0; i < n; i++) {
                    cmask_bfse->bfse->api->ReleaseValue(cmask_bfse->bfse_caches[i]);
                }
            }
        }
        if (cmask_bfse->bfse) {
            qtk_onnxruntime_delete(cmask_bfse->bfse);
        }
        wtk_free(cmask_bfse->bfse_out_len);
        wtk_free(cmask_bfse->bfse_caches);
    }
#endif
    if(cmask_bfse->covm){
        for(i=0;i<cmask_bfse->cfg->ntheta;++i){
            wtk_covm_delete(cmask_bfse->covm[i]);
        }
        wtk_free(cmask_bfse->covm);
    }
    if(cmask_bfse->bf){
        for(i=0;i<cmask_bfse->cfg->ntheta;++i){
            wtk_bf_delete(cmask_bfse->bf[i]);
        }
        wtk_free(cmask_bfse->bf);
    }
    if(cmask_bfse->qmmse){
        for(i=0;i<cmask_bfse->cfg->ntheta;++i){
            wtk_qmmse_delete(cmask_bfse->qmmse[i]);
        }
        wtk_free(cmask_bfse->qmmse);
    }
    if (cmask_bfse->eq) {
        for(i=0;i<cmask_bfse->cfg->ntheta;++i){
            wtk_equalizer_delete(cmask_bfse->eq[i]);
        }
        wtk_free(cmask_bfse->eq);
    }
    wtk_free(cmask_bfse->entropy_E);
    wtk_free(cmask_bfse->entropy_Eb);
    wtk_float_delete_p2(cmask_bfse->time_delay, cmask_bfse->cfg->ntheta);
    wtk_complex_delete_p3(cmask_bfse->phase_shift, cmask_bfse->cfg->ntheta, cmask_bfse->cfg->nmicchannel);
    wtk_float_delete_p2(cmask_bfse->mic_mag, cmask_bfse->cfg->nmicchannel);
    wtk_float_delete_p2(cmask_bfse->LPS, cmask_bfse->cfg->nmicchannel);
    wtk_complex_delete_p2(cmask_bfse->mic_covar, cmask_bfse->cfg->out_channels);
    wtk_complex_delete_p3(cmask_bfse->ideal_phase_covar, cmask_bfse->cfg->ntheta, cmask_bfse->cfg->out_channels);
    wtk_complex_delete_p3(cmask_bfse->freq_covar, cmask_bfse->cfg->ntheta, cmask_bfse->cfg->out_channels);
    wtk_complex_delete_p2(cmask_bfse->freq_covar_sum, cmask_bfse->cfg->ntheta);
    wtk_free(cmask_bfse->mean_mat);
    wtk_complex_delete_p2(cmask_bfse->input_norm, cmask_bfse->cfg->nmicchannel);
    wtk_free(cmask_bfse->feat);
    wtk_free(cmask_bfse->s_real);
    wtk_free(cmask_bfse->s_imag);
    wtk_free(cmask_bfse->n_real);
    wtk_free(cmask_bfse->n_imag);
    wtk_free(cmask_bfse->s_cov);
    wtk_free(cmask_bfse->n_cov);

    wtk_free(cmask_bfse->bs_scale);
    wtk_free(cmask_bfse->bs_last_scale);
    wtk_free(cmask_bfse->bs_max_cnt);
    wtk_free(cmask_bfse->entropy_in_cnt);
    wtk_free(cmask_bfse->entropy_silcnt);
    wtk_free(cmask_bfse->entropy_sil);
    wtk_free(cmask_bfse->mic_silcnt);
    wtk_free(cmask_bfse->mic_sil);

    if(cmask_bfse->aspec){

        for(i=0;i<cmask_bfse->cfg->ntheta;++i){
            wtk_aspec_delete(cmask_bfse->aspec[i]);
            wtk_fring_delete(cmask_bfse->q_fring[i]);
            wtk_qmmse_delete(cmask_bfse->qmmse2[i]);
            wtk_free(cmask_bfse->cohv[i]);
        }
        wtk_free(cmask_bfse->aspec);
        wtk_free(cmask_bfse->q_fring);
        wtk_free(cmask_bfse->qmmse2);
        wtk_free(cmask_bfse->cohv);
        wtk_free(cmask_bfse->cov);
        if(cmask_bfse->inv_cov){
            wtk_free(cmask_bfse->inv_cov);
            wtk_free(cmask_bfse->tmp);
        }
        if(cmask_bfse->gcc_fft){
            wtk_complex_delete_p2(cmask_bfse->gcc_fft, cmask_bfse->nbin);
        }
        wtk_free(cmask_bfse->specsum);
        wtk_free(cmask_bfse->right_nf);
        wtk_free(cmask_bfse->q_spec);
    }

    wtk_free(cmask_bfse);
}

void wtk_cmask_bfse_start_aspec1(wtk_cmask_bfse_t *cmask_bfse, int idx) {
    float theta2, theta3;
    float theta = cmask_bfse->cfg->theta[idx];
    float theta_range = cmask_bfse->cfg->theta_range;
    wtk_aspec_t *aspec = cmask_bfse->aspec[idx];

    if(theta==0 || theta==180)
    {
        theta2=theta+2*theta_range;
        if(theta2>180)
        {
            theta2=360-theta2;
        }

        aspec->start_ang_num=2;
        wtk_aspec_start(aspec, theta, 0, 0);
        wtk_aspec_start(aspec, theta2, 0, 1);
    }else
    {
        theta2=theta+2*theta_range;
        if(theta2>180)
        {
            if(theta+theta_range>=180)
            {
                theta2=-1;
            }else
            {
                theta2=180;   
            }
        }
        theta3=theta-2*theta_range;
        if(theta3<0)
        {
            if(theta-theta_range<=0)
            {
                theta3=-1;
            }else
            {
                theta3=0;
            }
        }
        if(theta2==-1 || theta3==-1)
        {
            aspec->start_ang_num=2;
            wtk_aspec_start(aspec, theta, 0, 0);
            wtk_aspec_start(aspec, theta2==-1?theta3:theta2, 0, 1);
        }else
        {
            aspec->start_ang_num=3;
            wtk_aspec_start(aspec, theta, 0, 0);
            wtk_aspec_start(aspec, theta2, 0, 1);
            wtk_aspec_start(aspec, theta3, 0, 2);
        }
    }
}

void wtk_cmask_bfse_start_aspec2(wtk_cmask_bfse_t *cmask_bfse, int idx) {
    float theta2, theta3;
    float theta = cmask_bfse->cfg->theta[idx];
    float theta_range = cmask_bfse->cfg->theta_range;
    wtk_aspec_t *aspec = cmask_bfse->aspec[idx];

    if(theta==0 || theta==180)
    {
        theta2=theta+2*theta_range;
        if(theta2>180)
        {
            theta2=360-theta2;
        }

        aspec->start_ang_num=2;
        wtk_aspec_start(aspec, theta, 0, 0);
        wtk_aspec_start(aspec, theta2, 0, 1);
    }else
    {
        theta2=theta+2*theta_range;
        if(theta2>180)
        {
            if(theta+theta_range>=180)
            {
                theta2=-1;
            }else
            {
                theta2=180;   
            }
        }
        theta3=theta-2*theta_range;
        if(theta3<0)
        {
            if(theta-theta_range<=0)
            {
                theta3=-1;
            }else
            {
                theta3=0;
            }
        }
        if(theta2==-1 || theta3==-1)
        {
            aspec->start_ang_num=2;
            wtk_aspec_start(aspec, theta, 0, 0);
            wtk_aspec_start(aspec, theta2==-1?theta3:theta2, 0, 1);
        }else
        {
            aspec->start_ang_num=3;
            wtk_aspec_start(aspec, theta, 0, 0);
            wtk_aspec_start(aspec, theta2, 0, 1);
            wtk_aspec_start(aspec, theta3, 0, 2);   
        }
    }
}

void wtk_cmask_bfse_start(wtk_cmask_bfse_t *cmask_bfse) {
    int i;
    int ntheta = cmask_bfse->cfg->ntheta;
    wtk_complex_t ***phase_shift = cmask_bfse->phase_shift;
    wtk_complex_t ***ideal_phase_covar = cmask_bfse->ideal_phase_covar;
    _compute_multi_channel_phase_shift(cmask_bfse);
    for(i=0;i<ntheta;++i){
        _compute_channel_covariance(cmask_bfse, phase_shift[i], ideal_phase_covar[i], cmask_bfse->cfg->use_norm);
    }
    if(cmask_bfse->cfg->use_bf){
        for(i=0;i<ntheta;++i){
            wtk_bf_update_ovec(cmask_bfse->bf[i],cmask_bfse->cfg->theta[i],0);
            wtk_bf_init_w(cmask_bfse->bf[i]);
        }
    }
    if(cmask_bfse->cfg->use_aspec){
        for(i=0;i<ntheta;++i){
            if(cmask_bfse->cfg->use_line){
                wtk_cmask_bfse_start_aspec1(cmask_bfse, i);
            }else{
                wtk_cmask_bfse_start_aspec2(cmask_bfse, i);
            }
        }
    }
}

void wtk_cmask_bfse_set_notify(wtk_cmask_bfse_t *cmask_bfse, void *ths, wtk_cmask_bfse_notify_f notify) {
    cmask_bfse->notify = notify;
    cmask_bfse->ths = ths;
}

void wtk_cmask_bfse_feed_bfmask(wtk_cmask_bfse_t *cmask_bfse, float *H1, float *H2)
{
    int nmicchannel = cmask_bfse->cfg->nmicchannel;
    int nbin = cmask_bfse->nbin;
    int ntheta = cmask_bfse->cfg->ntheta;
    int i, k;
    wtk_complex_t **fft = cmask_bfse->fft;
    wtk_complex_t **fftx = cmask_bfse->fftx;

    wtk_complex_zero_p2(fftx, ntheta, nbin);
    for(k=0;k<nbin;++k){
        for(i=0;i<nmicchannel;++i){
            fftx[0][k].a += H1[i*2*nbin+k] * fft[i][k].a - H1[(i*2+1)*nbin+k] * fft[i][k].b;
            fftx[0][k].b += H1[i*2*nbin+k] * fft[i][k].b + H1[(i*2+1)*nbin+k] * fft[i][k].a;
            fftx[1][k].a += H2[i*2*nbin+k] * fft[i][k].a - H2[(i*2+1)*nbin+k] * fft[i][k].b;
            fftx[1][k].b += H2[i*2*nbin+k] * fft[i][k].b + H2[(i*2+1)*nbin+k] * fft[i][k].a;
        }
    }
}

void wtk_cmask_bfse_feed_bfse(wtk_cmask_bfse_t *cmask_bfse)
{
#ifdef ONNX_DEC
    float *s_cov = cmask_bfse->s_cov;
    float *n_cov = cmask_bfse->n_cov;
    float *H1;
    float *H2;
    int i, j;
    const OrtApi *api = cmask_bfse->bfse->api;
    OrtMemoryInfo *meminfo = cmask_bfse->bfse->meminfo;
    qtk_onnxruntime_t *bfse = cmask_bfse->bfse;
    OrtStatus *status;
    int num_in = bfse->num_in;
    int outer_in_num = bfse->cfg->outer_in_num;
    int outer_out_num = bfse->cfg->outer_out_num;
    qtk_onnx_item_t *item;
    void *onnx_out;
    // int64_t size = 0, *out_shape;

    for (i = 0; i < outer_in_num; ++i) {
        item = bfse->in_items + i;
        if (i == 0) {
            memcpy(item->val, s_cov, item->bytes * item->in_dim);
        } else if (i == 1) {
            memcpy(item->val, n_cov, item->bytes * item->in_dim);
        }
    }

    // printf("num_in:\n");
    for (i = 0; i < num_in; ++i) {
        item = bfse->in_items + i;
        status = api->CreateTensorWithDataAsOrtValue(
            meminfo, item->val, item->bytes * item->in_dim, item->shape,
            item->shape_len, item->type, bfse->in + i);
        // printf("%d\n", i);
        // for(j=0;j<item->shape_len;++j){
        // 	printf("%d %ld\n", j, item->shape[j]);
        // }
        // printf("%ld\n", item->bytes*item->in_dim/sizeof(float));
    }

    status = api->Run(bfse->session, NULL,
                        cast(const char *const *, bfse->name_in),
                        cast(const OrtValue *const *, bfse->in), bfse->num_in,
                        cast(const char *const *, bfse->name_out),
                        bfse->num_out, bfse->out);

    H1 = qtk_onnxruntime_getout(bfse, 0);
    H2 = qtk_onnxruntime_getout(bfse, 1);
    wtk_cmask_bfse_feed_bfmask(cmask_bfse, H1, H2);

    for (i = outer_in_num, j = outer_out_num; i < num_in; ++i, ++j) {
        item = bfse->in_items + i;
        onnx_out = qtk_onnxruntime_getout(bfse, j);
        memcpy(item->val, onnx_out, item->bytes * item->in_dim);
    }
    qtk_onnxruntime_reset(bfse);
    (void)status;
#endif
}

void wtk_cmask_bfse_mask(wtk_cmask_bfse_t *cmask_bfse, float *m_s, float *m_n)
{
    wtk_complex_t **fft = cmask_bfse->fft;
    int nbin = cmask_bfse->nbin;
    int nmicchannel = cmask_bfse->cfg->nmicchannel;
    float *s_real = cmask_bfse->s_real;
    float *s_imag = cmask_bfse->s_imag;
    float *n_real = cmask_bfse->n_real;
    float *n_imag = cmask_bfse->n_imag;
    float *s_cov = cmask_bfse->s_cov;
    float *n_cov = cmask_bfse->n_cov;
    int i, j, k;

    for(k=0;k<nbin;++k){
        for(i=0;i<nmicchannel;++i){
            s_real[i*nbin+k] = m_s[k]*fft[i][k].a - m_s[k+nbin]*fft[i][k].b;
            s_imag[i*nbin+k] = m_s[k+nbin]*fft[i][k].a + m_s[k]*fft[i][k].b;
            n_real[i*nbin+k] = m_n[k]*fft[i][k].a - m_n[k+nbin]*fft[i][k].b;
            n_imag[i*nbin+k] = m_n[k+nbin]*fft[i][k].a + m_n[k]*fft[i][k].b;
        }
    }
    for(k=0;k<nbin;++k){
        for(i=0;i<nmicchannel;++i){
            for(j=i;j<nmicchannel;++j){
                if(i==j){
                    s_cov[(i*nmicchannel+j)*nbin+k] = s_real[i*nbin+k]*s_real[j*nbin+k] + s_imag[i*nbin+k]*s_imag[j*nbin+k];
                    s_cov[(i*nmicchannel+j+nmicchannel*nmicchannel)*nbin+k] = 0;
                    n_cov[(i*nmicchannel+j)*nbin+k] = n_real[i*nbin+k]*n_real[j*nbin+k] + n_imag[i*nbin+k]*n_imag[j*nbin+k];
                    n_cov[(i*nmicchannel+j+nmicchannel*nmicchannel)*nbin+k] = 0;
                }else{
                    s_cov[(i*nmicchannel+j)*nbin+k] = s_real[i*nbin+k]*s_real[j*nbin+k] + s_imag[i*nbin+k]*s_imag[j*nbin+k];
                    s_cov[(i*nmicchannel+j+nmicchannel*nmicchannel)*nbin+k] = s_imag[i*nbin+k]*s_real[j*nbin+k] - s_real[i*nbin+k]*s_imag[j*nbin+k];
                    n_cov[(i*nmicchannel+j)*nbin+k] = n_real[i*nbin+k]*n_real[j*nbin+k] + n_imag[i*nbin+k]*n_imag[j*nbin+k];
                    n_cov[(i*nmicchannel+j+nmicchannel*nmicchannel)*nbin+k] = n_imag[i*nbin+k]*n_real[j*nbin+k] - n_real[i*nbin+k]*n_imag[j*nbin+k];
                    s_cov[(j*nmicchannel+i)*nbin+k] = s_cov[(i*nmicchannel+j)*nbin+k];
                    s_cov[(j*nmicchannel+i+nmicchannel*nmicchannel)*nbin+k] = -s_cov[(i*nmicchannel+j+nmicchannel*nmicchannel)*nbin+k];
                    n_cov[(j*nmicchannel+i)*nbin+k] = n_cov[(i*nmicchannel+j)*nbin+k];
                    n_cov[(j*nmicchannel+i+nmicchannel*nmicchannel)*nbin+k] = -n_cov[(i*nmicchannel+j+nmicchannel*nmicchannel)*nbin+k];
                }
            }
        }
    }
}

void wtk_cmask_bfse_feed_sep(wtk_cmask_bfse_t *cmask_bfse, float *feat)
{
#ifdef ONNX_DEC
    float *m_s = NULL;
    float *m_n = NULL;
    int i, j;
    const OrtApi *api = cmask_bfse->sep->api;
    OrtMemoryInfo *meminfo = cmask_bfse->sep->meminfo;
    qtk_onnxruntime_t *sep = cmask_bfse->sep;
    OrtStatus *status;
    int num_in = sep->num_in;
    int outer_in_num = sep->cfg->outer_in_num;
    int outer_out_num = sep->cfg->outer_out_num;
    qtk_onnx_item_t *item;
    void *onnx_out;
    // int64_t size = 0, *out_shape;
    // int step=0;

    for (i = 0; i < outer_in_num; ++i) {
        item = sep->in_items + i;
        if (i == 0) {
            memcpy(item->val, feat, item->bytes * item->in_dim);
        }
    }

    // printf("num_in:\n");
    for (i = 0; i < num_in; ++i) {
        item = sep->in_items + i;
        status = api->CreateTensorWithDataAsOrtValue(
            meminfo, item->val, item->bytes * item->in_dim, item->shape,
            item->shape_len, item->type, sep->in + i);
        // printf("%d\n", i);
        // for(j=0;j<item->shape_len;++j){
        // 	printf("%d %ld\n", j, item->shape[j]);
        // }
        // printf("%ld\n", item->bytes*item->in_dim/sizeof(float));
    }

    status = api->Run(sep->session, NULL,
                        cast(const char *const *, sep->name_in),
                        cast(const OrtValue *const *, sep->in), sep->num_in,
                        cast(const char *const *, sep->name_out),
                        sep->num_out, sep->out);

    m_s = qtk_onnxruntime_getout(sep, 0);
    m_n = qtk_onnxruntime_getout(sep, 1);
    wtk_cmask_bfse_mask(cmask_bfse, m_s, m_n);

    for (i = outer_in_num, j = outer_out_num; i < num_in; ++i, ++j) {
        item = sep->in_items + i;
        onnx_out = qtk_onnxruntime_getout(sep, j);
        memcpy(item->val, onnx_out, item->bytes * item->in_dim);
    }
    qtk_onnxruntime_reset(sep);
    (void)status;
#endif
}

static float wtk_cmask_bfse_fft_energy(wtk_complex_t *fftx, int nbin) {
    return qtk_vector_cpx_mag_squared_sum(fftx + 1, nbin - 2);
}

float wtk_cmask_bfse_entropy(wtk_cmask_bfse_t *cmask_bfse, wtk_complex_t *fftx)
{
    int rate = cmask_bfse->cfg->rate;
    int wins = cmask_bfse->cfg->wins;
    int i;
    int fx1 = (250*1.0*wins)/rate;
    int fx2 = (3500*1.0*wins)/rate;
    int km = floor(wins*1.0/8);
    float K = 0.5;
    float *E=cmask_bfse->entropy_E;
    float P1;
    float *Eb=cmask_bfse->entropy_Eb;
    float sum;
    float prob;
    float Hb;

    memset(E, 0, sizeof(float) * cmask_bfse->nbin);
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

void wtk_cmask_bfse_feed_bf(wtk_cmask_bfse_t *cmask_bfse, wtk_complex_t *fft, wtk_complex_t *fft_n, int idx)
{
	int k;
	wtk_bf_t *bf=cmask_bfse->bf[idx];
	wtk_covm_t *covm=cmask_bfse->covm[idx];
	int b;
	wtk_complex_t fft2;
	wtk_complex_t ffts;
	wtk_complex_t ffty;
	int clip_s=cmask_bfse->cfg->clip_s;
	int clip_e=cmask_bfse->cfg->clip_e;
	int bf_clip_s=cmask_bfse->cfg->bf_clip_s;
	int bf_clip_e=cmask_bfse->cfg->bf_clip_e;

    for(k=clip_s+1; k<clip_e; ++k)
    {
        if(k>=bf_clip_s && k<bf_clip_e){
            bf->cfg->mu=cmask_bfse->cfg->bfmu;
        }else{
            bf->cfg->mu=cmask_bfse->cfg->bfmu2;
        }
        ffts.a=fft[k].a;
        ffts.b=fft[k].b;

        ffty.a=fft_n[k].a;
        ffty.b=fft_n[k].b;
        ffty.a=0;
        ffty.b=0;
        fft2.a=ffts.a;
        fft2.b=ffts.b;

        b=0;
        b=wtk_covm_feed_fft3(covm, &ffty, k, 1);
        if(b==1)
        {
            wtk_bf_update_ncov(bf, covm->ncov, k);
        }
        if(covm->scov)
        {
            b=wtk_covm_feed_fft3(covm, &ffts, k, 0);
            if(b==1)
            {
                wtk_bf_update_scov(bf, covm->scov, k);
            }
        }
        if(b==1)
        {
            wtk_bf_update_w(bf, k);
        }
        wtk_bf_output_fft_k(bf, &fft2, fft+k, k);
        // printf("%f %f %f %f\n", fft2.a, fft2.b, fft[k].a, fft[k].b);
	}
}


void wtk_cmask_bfse_notify_data(wtk_cmask_bfse_t *cmask_bfse, wtk_complex_t *fftx, wtk_complex_t *ffty, wtk_complex_t *fft, int idx)
{
    int i;
    int nbin = cmask_bfse->nbin;
    float entropy=0;
    float entropy_thresh = cmask_bfse->cfg->entropy_thresh;
    int entropy_cnt = cmask_bfse->cfg->entropy_cnt;
    
    float micenr;
    float micenr_thresh = cmask_bfse->cfg->micenr_thresh;
    int micenr_cnt = cmask_bfse->cfg->micenr_cnt;
    float de_eng_sum=0;
    int de_clip_s=cmask_bfse->cfg->de_clip_s;
    int de_clip_e=cmask_bfse->cfg->de_clip_e;
    float de_thresh=cmask_bfse->cfg->de_thresh;
    float de_alpha=cmask_bfse->cfg->de_alpha;
    float de_alpha2;
    float entropy_scale;

    if(entropy_thresh>0){
        entropy=wtk_cmask_bfse_entropy(cmask_bfse, fftx);
    }
    if(entropy_thresh>0){
        if(entropy<entropy_thresh){
            ++cmask_bfse->entropy_in_cnt[idx];
        }else{
            cmask_bfse->entropy_in_cnt[idx] = 0;
        }
        if(cmask_bfse->entropy_in_cnt[idx]>=cmask_bfse->cfg->entropy_in_cnt){
            cmask_bfse->entropy_sil[idx] = 0;
            cmask_bfse->entropy_silcnt[idx] = entropy_cnt;
        }else if(cmask_bfse->entropy_sil[idx]==0){
            cmask_bfse->entropy_silcnt[idx] -= 1;
            if(cmask_bfse->entropy_silcnt[idx]<=0){
                cmask_bfse->entropy_sil[idx] = 1;
            }
        }
    }
    if(cmask_bfse->cfg->use_bf){
        wtk_cmask_bfse_feed_bf(cmask_bfse, fftx, ffty, idx);
    }
    if(cmask_bfse->qmmse[idx]){
        wtk_qmmse_denoise(cmask_bfse->qmmse[idx], fftx);
        // wtk_qmmse_feed_echo_denoise3(cmask_bfse->qmmse[idx], fftx, ffty);
    }

    // static int cnt=0;
    // cnt++;
    micenr = wtk_cmask_bfse_fft_energy(fftx, nbin);
    if (micenr > micenr_thresh) {
        // if(cmask_bfse->mic_sil==1)
        // {
        // 	printf("sp start %f %f
        // %f\n", 1.0/16000*cnt*(nbin-1),micenr,micenr_thresh);
        // }
        cmask_bfse->mic_sil[idx] = 0;
        cmask_bfse->mic_silcnt[idx] = micenr_cnt;
    } else if (cmask_bfse->mic_sil[idx] == 0) {
        cmask_bfse->mic_silcnt[idx] -= 1;
        if (cmask_bfse->mic_silcnt[idx] <= 0) {
            // printf("sp end %f\n", 1.0/16000*cnt*(nbin-1));
            cmask_bfse->mic_sil[idx] = 1;
        }
    }

    if(de_alpha!=1.0){
        for(i=de_clip_s;i<de_clip_e;++i){
            de_eng_sum+=fftx[i].a*fftx[i].a+fftx[i].b*fftx[i].b;
        }
        // printf("%f\n", logf(de_eng_sum+1e-9));
        if(de_eng_sum>de_thresh){
            float de_pow_ratio = cmask_bfse->cfg->de_pow_ratio;
            float de_mul_ratio = cmask_bfse->cfg->de_mul_ratio;
            float alpha;
            de_alpha2 = de_alpha * min(max(0.1, 5.0/logf(de_eng_sum+1e-9)), 1.0);
            for(i=de_clip_s;i<de_clip_e;++i){
                alpha = (de_mul_ratio - de_alpha2) * (pow(de_clip_e-i, de_pow_ratio))/pow((nbin-de_clip_s), de_pow_ratio) + de_alpha2;
                // fftx[i].a*=de_alpha2;
                // fftx[i].b*=de_alpha2;
                fftx[i].a*=alpha;
                fftx[i].b*=alpha;
            }
        }
    }
    if(entropy_thresh>0){
        if(entropy>entropy_thresh){
            if(cmask_bfse->entropy_silcnt[idx] > 0){
                entropy_scale = powf(cmask_bfse->entropy_silcnt[idx] * 1.0/entropy_cnt, cmask_bfse->cfg->entropy_ratio)+cmask_bfse->cfg->entropy_min_scale;
            }else{
                entropy_scale = powf(1.0/entropy_cnt, cmask_bfse->cfg->entropy_ratio)+cmask_bfse->cfg->entropy_min_scale;
            }
            entropy_scale = min(entropy_scale, 1.0);
            for(i=0;i<nbin;++i){
                fftx[i].a*=entropy_scale;
                fftx[i].b*=entropy_scale;
            }
        }
    }
}

void wtk_cmask_bfse_feed_cnon(wtk_cmask_bfse_t *cmask_bfse, wtk_complex_t *fft) {
    int nbin = cmask_bfse->nbin;
    float sym = cmask_bfse->cfg->sym;
    static float fx = 2.0f * PI / RAND_MAX;
    float f, f2;
    int i;

    for (i = 1; i < nbin - 1; ++i) {
        f = rand() * fx;
        f2 = 1.f;
        if (f2 > 0) {
            // f2=sqrtf(f2);
            fft[i].a += sym * cosf(f) * f2;
            fft[i].b += sym * sinf(f) * f2;
        }
    }
}

void wtk_cmask_bfse_control_bs(wtk_cmask_bfse_t *cmask_bfse, float *out, int len, int idx) {
    float out_max;
    int i;

    if (cmask_bfse->mic_sil == 0) {
        out_max = wtk_float_abs_max(out, len);
        if (out_max > cmask_bfse->cfg->max_bs_out) {
            cmask_bfse->bs_scale[idx] = cmask_bfse->cfg->max_bs_out / out_max;
            if (cmask_bfse->bs_scale[idx] < cmask_bfse->bs_last_scale[idx]) {
                cmask_bfse->bs_last_scale[idx] = cmask_bfse->bs_scale[idx];
            } else {
                cmask_bfse->bs_scale[idx] = cmask_bfse->bs_last_scale[idx];
            }
            cmask_bfse->bs_max_cnt[idx] = 5;
        }
        for (i = 0; i < len; ++i) {
            out[i] *= cmask_bfse->bs_scale[idx];
        }
        if (cmask_bfse->bs_max_cnt[idx] > 0) {
            --cmask_bfse->bs_max_cnt[idx];
        }
        if (cmask_bfse->bs_max_cnt[idx] <= 0 && cmask_bfse->bs_scale[idx] < 1.0) {
            cmask_bfse->bs_scale[idx] *= 1.1f;
            cmask_bfse->bs_last_scale[idx] = cmask_bfse->bs_scale[idx];
            if (cmask_bfse->bs_scale[idx] > 1.0) {
                cmask_bfse->bs_scale[idx] = 1.0;
                cmask_bfse->bs_last_scale[idx] = 1.0;
            }
        }
    } else {
        cmask_bfse->bs_scale[idx] = 1.0;
        cmask_bfse->bs_last_scale[idx] = 1.0;
        cmask_bfse->bs_max_cnt[idx] = 0;
    }
}

void wtk_cmask_bfse_feed_decode(wtk_cmask_bfse_t *cmask_bfse, wtk_complex_t **fft){
    float *feat = cmask_bfse->feat;
    _compute_feature(cmask_bfse, fft);
    wtk_cmask_bfse_feed_sep(cmask_bfse, feat);
    wtk_cmask_bfse_feed_bfse(cmask_bfse);
}

void wtk_cmask_bfse_feed_aspec(wtk_cmask_bfse_t *cmask_bfse, wtk_complex_t **fft){
    int nmicchannel = cmask_bfse->cfg->nmicchannel;
    int specsum_ns=cmask_bfse->cfg->specsum_ns;
    int specsum_ne=cmask_bfse->cfg->specsum_ne;
    int nbin = cmask_bfse->nbin;
    int ntheta = cmask_bfse->cfg->ntheta;
    int i, j, k, n;
    int ret;
    wtk_complex_t *cov = cmask_bfse->cov;
    wtk_complex_t *inv_cov = cmask_bfse->inv_cov;
    wtk_complex_t **gcc_fft = cmask_bfse->gcc_fft;
    wtk_dcomplex_t *tmp = cmask_bfse->tmp;
    float **cohv = cmask_bfse->cohv;
    float cov_travg;
    float spec_k[3]={0};
    float fftabs2;
    float *specsum=cmask_bfse->specsum;
    float max_spec;
    float q_alpha = cmask_bfse->cfg->q_alpha;
    float q_alpha_1 = 1.0 - q_alpha;

    memset(specsum, 0, sizeof(float)*ntheta);
    if(cmask_bfse->aspec[0]->gccspec || cmask_bfse->aspec[0]->gccspec2){
        for(i=0;i<nmicchannel;++i){
            for(k=0;k<nbin;++k){
                gcc_fft[k][i].a = fft[i][k].a;
                gcc_fft[k][i].b = fft[i][k].b;
            }
        }
    }

    for(k=0;k<nbin;++k){
        memset(cov, 0, sizeof(wtk_complex_t)*nmicchannel*nmicchannel);
        for(i=0;i<nmicchannel;++i){
            for(j=i;j<nmicchannel;++j){
                if(i!=j){
                    cov[i*nmicchannel+j].a = fft[i][k].a * fft[j][k].a + fft[i][k].b * fft[j][k].b;
                    cov[i*nmicchannel+j].b = -fft[i][k].a * fft[j][k].b + fft[i][k].b * fft[j][k].a;
                    cov[j*nmicchannel+i].a = cov[i*nmicchannel+j].a;
                    cov[j*nmicchannel+i].b = -cov[i*nmicchannel+j].b;
                }else{
                    cov[i*nmicchannel+j].a = fft[i][k].a * fft[j][k].a + fft[i][k].b * fft[j][k].b;
                    cov[i*nmicchannel+j].b = 0;
                }
            }
        }
        if(inv_cov){
            ret=wtk_complex_invx4(cov, tmp, nmicchannel, inv_cov, 1);
            if(ret!=0)
            {
                j=0;
                for(i=0;i<nmicchannel;++i)
                {
                    cov[j].a+=0.01;
                    j+=nmicchannel+1;
                }
                wtk_complex_invx4(cov,tmp,nmicchannel,inv_cov,1);
            }
        }
        cov_travg = 0;
        if(cmask_bfse->aspec[0]->need_cov_travg){
            for(i=0;i<nmicchannel;++i){
                cov_travg += cov[i*nmicchannel+i].a;
            }
            cov_travg /= nmicchannel;
        }
        if(cmask_bfse->aspec[0]->gccspec || cmask_bfse->aspec[0]->gccspec2){
            fftabs2=0;
            for(i=0; i<nmicchannel; ++i)
            {
                fftabs2+=gcc_fft[k][i].a*gcc_fft[k][i].a+gcc_fft[k][i].b*gcc_fft[k][i].b;
            }
        }
        for(n=0;n<ntheta;++n){
            for(i=0;i<3;++i){
                spec_k[i] = wtk_aspec_flush_spec_k(cmask_bfse->aspec[n], gcc_fft, fftabs2, cov_travg, cov, inv_cov, k, i);
            }
            if(spec_k[0] > spec_k[1] && spec_k[0] > spec_k[2]){
                cohv[n][k] = 1;
                if(k>=specsum_ns && k<=specsum_ne){
                    specsum[n] += spec_k[0] * 2 - spec_k[1] - spec_k[2];
                }
            }else{
                cohv[n][k] = 0;
            }
        }
    }
    // printf("%f\n", specsum[1]);
    for(n=0;n<ntheta;++n){
        wtk_fring_push2(cmask_bfse->q_fring[n], specsum[n]);
        if(cmask_bfse->q_fring[n]->used == cmask_bfse->q_fring[n]->nslot - 1){
            max_spec = wtk_fring_max(cmask_bfse->q_fring[n]);
            if(max_spec < cmask_bfse->q_spec[n]){
                max_spec = q_alpha * cmask_bfse->q_spec[n] + q_alpha_1 * max_spec;
            }
            cmask_bfse->q_spec[n] = max_spec;
            if(max_spec > cmask_bfse->cfg->min_speccrest){
                cmask_bfse->right_nf[n] = cmask_bfse->cfg->right_nf;
            }else if(max_spec > cmask_bfse->cfg->envelope_thresh){

            }else if(max_spec > cmask_bfse->cfg->right_min_thresh){
                --cmask_bfse->right_nf[n];
            }else{
                cmask_bfse->right_nf[n] = 0;
            }
        }
        if(cmask_bfse->right_nf[n] <= 0){
            for(k=0;k<nbin;++k){
                cohv[n][k] = 0;
            }
        }else{
            if(cmask_bfse->cfg->use_sqenvelope){
                for(k=1; k<nbin-1; ++k)
                {
                    cohv[n][k]=1;
                }
            }
        }
    }
}

void wtk_cmask_bfse_feed(wtk_cmask_bfse_t *cmask_bfse, short *data, int len,
                        int is_end) {
    int i, j;
    int nmicchannel = cmask_bfse->cfg->nmicchannel;
    int nspchannel = cmask_bfse->cfg->nspchannel;
    int channel = cmask_bfse->cfg->channel;
	int *mic_channel=cmask_bfse->cfg->mic_channel;
	int *sp_channel=cmask_bfse->cfg->sp_channel;
    int ntheta = cmask_bfse->cfg->ntheta;
    int wins = cmask_bfse->cfg->wins;
    int fsize = wins / 2;
    int nbin = cmask_bfse->nbin;
    int length;
    wtk_drft_t *rfft = cmask_bfse->rfft;
    float *rfft_in = cmask_bfse->rfft_in;
    wtk_complex_t **fft = cmask_bfse->fft;
    wtk_complex_t **fft_sp = cmask_bfse->fft_sp;
    wtk_complex_t **fftx = cmask_bfse->fftx;
    wtk_complex_t **ffttmp = cmask_bfse->ffttmp;
    float **analysis_mem = cmask_bfse->analysis_mem,
          **analysis_mem_sp = cmask_bfse->analysis_mem_sp;
	float **synthesis_mem=cmask_bfse->synthesis_mem;
    float *synthesis_window=cmask_bfse->synthesis_window;
    float *analysis_window = cmask_bfse->analysis_window;
    float **n_out = cmask_bfse->n_out;
    float *out = cmask_bfse->out;
    short *pv = (short *)out;
    wtk_strbuf_t **mic=cmask_bfse->mic;
    wtk_strbuf_t **sp=cmask_bfse->sp;
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

        wtk_strbufs_pop(mic, nmicchannel, fsize*sizeof(float));
        wtk_strbufs_pop(sp, nspchannel, fsize*sizeof(float));
        length = mic[0]->pos/sizeof(float);

        if(cmask_bfse->cfg->use_freq_preemph){
            int pre_clip_s = cmask_bfse->cfg->pre_clip_s;
            int pre_clip_e = cmask_bfse->cfg->pre_clip_e;
            float pre_pow_ratio = cmask_bfse->cfg->pre_pow_ratio;
            float pre_mul_ratio = cmask_bfse->cfg->pre_mul_ratio;
            float alpha;
            for(i=0;i<nmicchannel;++i){
                for(j=pre_clip_s;j<pre_clip_e;++j){
                    alpha = (pre_mul_ratio - 1) * (pow(j-pre_clip_s, pre_pow_ratio))/pow((nbin-pre_clip_s), pre_pow_ratio) + 1;
                    fft[i][j].a *= alpha;
                    fft[i][j].b *= alpha;
                }
            }
        }

        if(cmask_bfse->cfg->use_onnx){
            wtk_cmask_bfse_feed_decode(cmask_bfse, fft);
        }

        if(cmask_bfse->cfg->use_aspec){
            wtk_cmask_bfse_feed_aspec(cmask_bfse, fft);
            for(i=0;i<ntheta;++i){
                if(cmask_bfse->right_nf[i] <= 0){
                    for(j=0;j<nbin;++j){
                        fftx[i][j].a = 0;
                        fftx[i][j].b = 0;
                    }
                }
                wtk_qmmse_feed_cohv(cmask_bfse->qmmse2[i], fftx[i], cmask_bfse->cohv[i]);
            }
        }
        if(cmask_bfse->cfg->use_trick){
            for(i=0;i<ntheta;++i){
                memcpy(ffttmp[i], fftx[i], sizeof(wtk_complex_t)*nbin);
            }
        }

        for(i=0;i<ntheta;++i){
            if(cmask_bfse->cfg->use_trick){
                wtk_cmask_bfse_notify_data(cmask_bfse, fftx[i], ffttmp[i==1?0:1], fft[0], i);
            }

            if (cmask_bfse->cfg->use_cnon) {
                wtk_cmask_bfse_feed_cnon(cmask_bfse, fftx[i]);
            }

            wtk_drft_istft(rfft, rfft_in, synthesis_mem[i], fftx[i], n_out[i], wins, synthesis_window);

            if (cmask_bfse->eq) {
                wtk_equalizer_feed_float(cmask_bfse->eq[i], n_out[i], fsize);
            }

            wtk_cmask_bfse_control_bs(cmask_bfse, n_out[i], fsize, i);

            for(j=0;j<fsize;++j){
                out[j*ntheta+i] = n_out[i][j];
            }
        }
        for(i=0;i<fsize*ntheta;++i){
            pv[i] = WTK_WAV_FLOAT_TO_SHORT(out[i]);
        }
        if(cmask_bfse->notify){
            cmask_bfse->notify(cmask_bfse->ths, pv, fsize*ntheta);
        }
    }
    if(is_end && length>0){
        short *pv2;
        if(cmask_bfse->notify)
        {
            pv=(short *)out;
            pv2=(short *)mic[0]->data;
            for(i=0;i<length;++i){
                for(j=0;j<ntheta;++j){
                    pv[i*ntheta+j] = pv2[i];
                }
            }
            cmask_bfse->notify(cmask_bfse->ths,pv,length*ntheta);
        }
    }
}
