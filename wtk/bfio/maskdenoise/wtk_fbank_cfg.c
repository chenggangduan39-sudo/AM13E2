#include "wtk_fbank_cfg.h" 

float _inverse_mel_scale_scalar(float freq)
{
	return 700.0 * (exp(freq / 1127.0) - 1.0);
}

void _inverse_mel_scale(float *freq, float *out_freq, int n)
{
	int i;
	for(i=0;i<n;++i){
		out_freq[i] = _inverse_mel_scale_scalar(freq[i]);
	}
}

float _mel_scale_scalar(float freq)
{
	return 1127.0 * log(1.0 + freq / 700.0);
}

void _mel_scale(float *freq, float *out_freq, int n)
{
	int i;
	for(i=0;i<n;++i){
		out_freq[i] = _mel_scale_scalar(freq[i]);
	}
}

void _vtln_warp_freq(wtk_kaldi_fbank_cfg_t *cfg, float *mel)
{
	wtk_debug("Not yet implemented\n");
	exit(0);
}

void _vtln_warp_mel_freq(wtk_kaldi_fbank_cfg_t *cfg, float *mel)
{
	// vtln_low, vtln_high, low_freq, high_freq, vtln_warp_factor, left_mel
	_inverse_mel_scale(mel, mel, cfg->num_mel_bins);
	_vtln_warp_freq(cfg, mel);
	_mel_scale(mel, mel, cfg->num_mel_bins);
}

void _get_mel_banks(wtk_kaldi_fbank_cfg_t *cfg)
{
	int i, j;
	// num_mel_bins, padded_window_size, sample_frequency, low_freq, high_freq, vtln_low, vtln_high, vtln_warp
	cfg->num_fft_bins = cfg->padded_window_size / 2;
	cfg->nyquist = 0.5 * cfg->sample_frequency;
	if(cfg->high_freq <= 0.0){
		cfg->high_freq += cfg->nyquist;
	}
	cfg->fft_bin_width = cfg->sample_frequency / cfg->padded_window_size;
	cfg->mel_low_freq = _mel_scale_scalar(cfg->low_freq);
	cfg->mel_high_freq = _mel_scale_scalar(cfg->high_freq);
	cfg->mel_freq_delta = (cfg->mel_high_freq - cfg->mel_low_freq) / (cfg->num_mel_bins + 1);
	if(cfg->vtln_high < 0.0){
		cfg->vtln_high += cfg->nyquist;
	}
	cfg->bin = (float*)wtk_malloc(sizeof(float) * cfg->num_mel_bins);
	cfg->left_mel = (float*)wtk_malloc(sizeof(float) * cfg->num_mel_bins);
	cfg->center_mel = (float*)wtk_malloc(sizeof(float) * cfg->num_mel_bins);
	cfg->right_mel = (float*)wtk_malloc(sizeof(float) * cfg->num_mel_bins);
	for(i=0;i<cfg->num_mel_bins;++i){
		cfg->bin[i] = i;
		cfg->left_mel[i] = cfg->mel_low_freq + i * cfg->mel_freq_delta;
		cfg->center_mel[i] = cfg->mel_low_freq + (i + 1) * cfg->mel_freq_delta;
		cfg->right_mel[i] = cfg->mel_low_freq + (i + 2) * cfg->mel_freq_delta;
	}

	if(cfg->vtln_warp != 1.0){
		_vtln_warp_mel_freq(cfg, cfg->left_mel);
		_vtln_warp_mel_freq(cfg, cfg->center_mel);
		_vtln_warp_mel_freq(cfg, cfg->right_mel);
	}

	cfg->center_freqs = (float*)wtk_malloc(sizeof(float) * cfg->num_mel_bins);
	_inverse_mel_scale(cfg->center_mel, cfg->center_freqs, cfg->num_mel_bins);
	cfg->mel = (float*)wtk_malloc(sizeof(float) * cfg->num_fft_bins);
	for(i=0;i<cfg->num_fft_bins;++i){
		cfg->mel[i] = cfg->fft_bin_width * i;
	}
	_mel_scale(cfg->mel, cfg->mel, cfg->num_fft_bins);

	cfg->up_slope = (float **)wtk_malloc(sizeof(float*) * cfg->num_mel_bins);
	cfg->down_slope = (float **)wtk_malloc(sizeof(float*) * cfg->num_mel_bins);
        cfg->bins = (float **)wtk_malloc(sizeof(float *) * cfg->num_mel_bins);
        for(i=0;i<cfg->num_mel_bins;++i){
		cfg->up_slope[i] = (float*)wtk_malloc(sizeof(float) * cfg->num_fft_bins);
		cfg->down_slope[i] = (float*)wtk_malloc(sizeof(float) * cfg->num_fft_bins);
                cfg->bins[i] = (float *)wtk_malloc(sizeof(float) *
                                                   (cfg->num_fft_bins + 1));
                memset(cfg->bins[i], 0,
                       sizeof(float) *
                           (cfg->num_fft_bins + 1)); // padding with 0
        }

	//// up_slope = (mel - left_mel) / (center_mel - left_mel)
	//// down_slope = (right_mel - mel) / (right_mel - center_mel)

	for(i=0;i<cfg->num_mel_bins;++i){
		for(j=0;j<cfg->num_fft_bins;++j){
			cfg->up_slope[i][j] = (cfg->mel[j] - cfg->left_mel[i]) / (cfg->center_mel[i] - cfg->left_mel[i]);
			cfg->down_slope[i][j] = (cfg->right_mel[i] - cfg->mel[j]) / (cfg->right_mel[i] - cfg->center_mel[i]);
		}
	}


	if(cfg->vtln_warp == 1.0){
		for(i=0;i<cfg->num_mel_bins;++i){
			for(j=0;j<cfg->num_fft_bins;++j){
				cfg->bins[i][j] = max(min(cfg->up_slope[i][j], cfg->down_slope[i][j]), 0.0);
			}
		}
	}else{
		wtk_debug("Not yet implemented\n");
		exit(0);
	}

}

int _feature_window_function(wtk_kaldi_fbank_cfg_t *cfg)
{
	double a;
	int fs=cfg->window_size;
	int i;
	float *win;

	a= M_2PI/(fs-1);
	cfg->window=(float*)wtk_calloc(cfg->window_size,sizeof(float));
	win=cfg->window;
	if(strcmp(cfg->window_type,"hanning")==0)
	{
		for(i=0;i<fs;++i)
		{
			win[i]=0.5*(1-cos(a*i));
		}
	}else if(strcmp(cfg->window_type,"hamming")==0)
	{
		for(i=0;i<fs;++i)
		{
			win[i]=0.54-0.46*cos(a*i);
		}
	}else if(strcmp(cfg->window_type,"povey")==0)
	{
		for(i=0;i<fs;++i)
		{
			win[i]=pow(0.5 - 0.5*cos(a * i), 0.85);
			// wtk_debug("v[%d]=%f\n",i,win[i]);
		}
		// exit(0);
	}else if(strcmp(cfg->window_type,"rectangular")==0)
	{
		for(i=0;i<fs;++i)
		{
			win[i]=1.0;
		}
	}else if(strcmp(cfg->window_type,"blackman")==0)
	{
		for(i=0;i<fs;++i)
		{
			win[i]=cfg->blackman_coeff-0.5*cos(a*i)+(0.5-cfg->blackman_coeff)*cos(2*a*i);
		}
	}else
	{
		wtk_debug("unkown window type\n");
		return -1;
	}
	return 0;
}

int _next_power_of_2(int n)
{
	if(n == 0){
		return 1;
	}else{
		int i = 1;
		while(i < n){
			i <<= 1;
		}
		return i;
	}
}

void _get_waveform_and_window_properties(wtk_kaldi_fbank_cfg_t *cfg)
{
	cfg->channel = max(cfg->channel, 0);
	cfg->window_shift = (int)(cfg->sample_frequency * cfg->frame_shift * MILLISECONDS_TO_SECONDS);
	cfg->window_size = (int)(cfg->sample_frequency * cfg->frame_length * MILLISECONDS_TO_SECONDS);
	if(cfg->round_to_power_of_two){
		cfg->padded_window_size = _next_power_of_2(cfg->window_size);
	}else{
		cfg->padded_window_size = cfg->window_size;
	}
}

int wtk_kaldi_fbank_cfg_init(wtk_kaldi_fbank_cfg_t *cfg)
{
	cfg->blackman_coeff = 0.42;
	cfg->channel = -1;
	cfg->dither = 0.0;
	cfg->energy_floor = 1.0;
	cfg->frame_length = 25.0;
	cfg->frame_shift = 10.0;
	cfg->high_freq = 0.0;
	cfg->htk_compat = 0;
	cfg->low_freq = 20.0;
	cfg->min_duration = 0.0;
	cfg->num_mel_bins = 23;
	cfg->preemphasis_coefficient = 0.97;
	cfg->raw_energy = 1;
	cfg->remove_dc_offset = 1;
	cfg->round_to_power_of_two = 1;
	cfg->sample_frequency = 16000.0;
	cfg->snip_edges = 1;
	cfg->subtract_mean = 0;
	cfg->use_energy = 0;
	cfg->use_log_fbank = 1;
	cfg->use_power = 1;
	cfg->vtln_high = -500.0;
	cfg->vtln_low = 100.0;
	cfg->vtln_warp = 1.0;
	cfg->window_type = "povey";

	cfg->window = NULL;
	cfg->bin = NULL;
	cfg->left_mel = NULL;
	cfg->center_mel = NULL;
	cfg->right_mel = NULL;
	cfg->center_freqs = NULL;
	cfg->mel = NULL;
	cfg->up_slope = NULL;
	cfg->down_slope = NULL;
	cfg->bins = NULL;

	return 0;
}

int wtk_kaldi_fbank_cfg_clean(wtk_kaldi_fbank_cfg_t *cfg)
{
	int i;
	if(cfg->window){
		wtk_free(cfg->window);
	}
	if(cfg->bin){
		wtk_free(cfg->bin);
	}
	if(cfg->left_mel){
		wtk_free(cfg->left_mel);
	}
	if(cfg->center_mel){
		wtk_free(cfg->center_mel);
	}
	if(cfg->right_mel){
		wtk_free(cfg->right_mel);
	}
	if(cfg->center_freqs){
		wtk_free(cfg->center_freqs);
	}
	if(cfg->mel){
		wtk_free(cfg->mel);
	}
	if(cfg->up_slope){
		for(i=0;i<cfg->num_mel_bins;++i){
			wtk_free(cfg->up_slope[i]);
		}
		wtk_free(cfg->up_slope);
	}
	if(cfg->down_slope){
		for(i=0;i<cfg->num_mel_bins;++i){
			wtk_free(cfg->down_slope[i]);
		}
		wtk_free(cfg->down_slope);
	}
	if(cfg->bins){
		for(i=0;i<cfg->num_mel_bins;++i){
			wtk_free(cfg->bins[i]);
		}
		wtk_free(cfg->bins);
	}
	return 0;
}

int wtk_kaldi_fbank_cfg_update_local(wtk_kaldi_fbank_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;
	int ret=0;

	wtk_local_cfg_update_cfg_f(lc,cfg,blackman_coeff,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,channel,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,dither,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,energy_floor,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,frame_length,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,frame_shift,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,high_freq,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,htk_compat,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,low_freq,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,min_duration,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,num_mel_bins,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,preemphasis_coefficient,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,raw_energy,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,remove_dc_offset,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,round_to_power_of_two,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,sample_frequency,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,snip_edges,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,subtract_mean,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_energy,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_log_fbank,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_power,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,vtln_high,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,vtln_low,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,vtln_warp,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,window_type,v);

	_get_waveform_and_window_properties(cfg);

	ret = _feature_window_function(cfg);
	if(ret != 0){
		wtk_debug("Error in feature window function\n");
		return ret;
	}
	_get_mel_banks(cfg);

	// for(int i=0;i<cfg->num_mel_bins;++i){
	// 	for(int j=0;j<cfg->num_fft_bins;++j){
	// 		printf("cfg->bins[%d][%d]=%f\n",i,j,cfg->bins[i][j]);
	// 	}
	// }
	return ret;
}

int wtk_kaldi_fbank_cfg_update(wtk_kaldi_fbank_cfg_t *cfg)
{

	return 0;
}



int wtk_fbank_cfg_init(wtk_fbank_cfg_t *cfg)
{
	wtk_kaldi_fbank_cfg_init(&cfg->kaldi_fbank);

	return 0;
}

int wtk_fbank_cfg_clean(wtk_fbank_cfg_t *cfg)
{
	wtk_kaldi_fbank_cfg_clean(&cfg->kaldi_fbank);
	return 0;
}

int wtk_fbank_cfg_update_local(wtk_fbank_cfg_t *cfg,wtk_local_cfg_t *m)
{
	wtk_local_cfg_t *lc;
	int ret = 0;

	lc=m;

	lc=wtk_local_cfg_find_lc_s(m,"kaldi_fbank");
	if(lc){
		ret = wtk_kaldi_fbank_cfg_update_local(&(cfg->kaldi_fbank),lc);
		if(ret != 0){
			wtk_debug("Error in kaldi_fbank update\n");
			return ret;
		}
	}

	return ret;
}

int wtk_fbank_cfg_update(wtk_fbank_cfg_t *cfg)
{
	if(cfg->kaldi_fbank.use_energy){
		cfg->num_fbank=cfg->kaldi_fbank.num_mel_bins+1;
	}else{
		cfg->num_fbank=cfg->kaldi_fbank.num_mel_bins;
	}
	wtk_kaldi_fbank_cfg_update(&cfg->kaldi_fbank);
	return 0;
}

int wtk_fbank_cfg_update2(wtk_fbank_cfg_t *cfg,wtk_source_loader_t *sl)
{
	wtk_fbank_cfg_update(cfg);
	return 0;
}

wtk_fbank_cfg_t* wtk_fbank_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_fbank_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_fbank_cfg,fn);
	if(!main_cfg)
	{
		return NULL;
	}
	cfg=(wtk_fbank_cfg_t*)main_cfg->cfg;
	cfg->main_cfg = main_cfg;
	return cfg;
}

void wtk_fbank_cfg_delete(wtk_fbank_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_fbank_cfg_t* wtk_fbank_cfg_new_bin(char *fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_fbank_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_fbank_cfg,fn,"./cfg");
	if(!mbin_cfg)
	{
		return NULL;
	}
	cfg=(wtk_fbank_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_fbank_cfg_delete_bin(wtk_fbank_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
