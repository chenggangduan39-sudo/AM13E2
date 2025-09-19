#include "wtk_fbank.h" 

void _subtract_column_mean(float *data, int len, int subtract_mean)
{
	wtk_debug("View global mean, not supported yet\n");
	exit(0);
}

float _get_log_energy(float *data, int len, float energy_floor)
{
	float log_energy=EPSILON;
	int i;

	for(i=0;i<len;++i){
		log_energy = logf(max(log_energy, data[i]*data[i]));
	}
	if(energy_floor == 0.0){
		return log_energy;
	}
	return max(log_energy, log(energy_floor));
}

void _get_strided(wtk_fbank_t *fbank, float *data, int len)
{
	if(fbank->cfg->kaldi_fbank.snip_edges){

	}else{
		wtk_debug("padding data\n");
		exit(0);
	}
}

void _get_window(wtk_fbank_t *fbank, float *data, int len)
{
	int i;
	float mean;
	float preemphasis_coefficient=fbank->cfg->kaldi_fbank.preemphasis_coefficient;
	float padded_window_size=fbank->cfg->kaldi_fbank.padded_window_size;
	float window_size=fbank->cfg->kaldi_fbank.window_size;
	float *window=fbank->cfg->kaldi_fbank.window;
	float *offset_strided_input=fbank->offset_strided_input;

	_get_strided(fbank, data, len);
	if(fbank->cfg->kaldi_fbank.dither != 0.0){
		wtk_float_add_dither(data, len);
	}

	if(fbank->cfg->kaldi_fbank.remove_dc_offset){
		mean = wtk_float_mean(data, len);
		for(i=0;i<len;++i){
			data[i] -= mean;
		}
	}
	
	if(fbank->cfg->kaldi_fbank.raw_energy){
		fbank->signal_log_energy = _get_log_energy(data, len, fbank->cfg->kaldi_fbank.energy_floor);
	}

	if(preemphasis_coefficient != 0.0){
		offset_strided_input[0] = data[0];
		memcpy(offset_strided_input+1, data, (len-1)*sizeof(float));
		for(i=0;i<len;++i){
			data[i] -= preemphasis_coefficient * offset_strided_input[i];
		}
	}

	for(i=0;i<len;++i){
		data[i] *= window[i];
	}

	if(padded_window_size != window_size){
		//// pass
	}

	if(!fbank->cfg->kaldi_fbank.raw_energy){
		fbank->signal_log_energy = _get_log_energy(data, padded_window_size, fbank->cfg->kaldi_fbank.energy_floor);
	}

}

wtk_fbank_t *wtk_fbank_new(wtk_fbank_cfg_t *cfg)
{
	wtk_fbank_t *fbank;

	fbank=(wtk_fbank_t *)wtk_malloc(sizeof(wtk_fbank_t));
	fbank->cfg=cfg;
	fbank->wins=cfg->kaldi_fbank.padded_window_size;
	fbank->nbin=fbank->wins/2+1;
	fbank->ths=NULL;
	fbank->notify=NULL;

	fbank->rfft=wtk_rfft_new(cfg->kaldi_fbank.padded_window_size/2);
	fbank->mic=wtk_strbuf_new(1024, 1);

	fbank->strided_input=(float *)wtk_malloc(cfg->kaldi_fbank.padded_window_size*sizeof(float));
	fbank->offset_strided_input=(float *)wtk_malloc(cfg->kaldi_fbank.padded_window_size*sizeof(float));
	fbank->fft_data=(float *)wtk_malloc(cfg->kaldi_fbank.padded_window_size*2*sizeof(float));
	fbank->spectrum=(float *)wtk_malloc(fbank->nbin*sizeof(float));
	fbank->mel_energies=(float *)wtk_malloc(fbank->cfg->num_fbank*sizeof(float));

	wtk_fbank_reset(fbank);

	return fbank;
}


void wtk_fbank_delete(wtk_fbank_t *fbank)
{
	wtk_rfft_delete(fbank->rfft);
	wtk_strbuf_delete(fbank->mic);
	wtk_free(fbank->strided_input);
	wtk_free(fbank->offset_strided_input);
	wtk_free(fbank->fft_data);
	wtk_free(fbank->spectrum);
	wtk_free(fbank->mel_energies);
	wtk_free(fbank);
}

void wtk_fbank_reset(wtk_fbank_t *fbank)
{
	memset(fbank->strided_input, 0, fbank->cfg->kaldi_fbank.padded_window_size*sizeof(float));
	memset(fbank->offset_strided_input, 0, fbank->cfg->kaldi_fbank.padded_window_size*sizeof(float));
	memset(fbank->fft_data, 0, fbank->cfg->kaldi_fbank.padded_window_size*2*sizeof(float));
	memset(fbank->spectrum, 0, fbank->nbin*sizeof(float));
	memset(fbank->mel_energies, 0, fbank->cfg->num_fbank*sizeof(float));
	fbank->signal_log_energy=0.0;
}

void wtk_fbank_set_notify(wtk_fbank_t *fbank,void *ths,wtk_fbank_notify_f notify)
{
	fbank->ths=ths;
	fbank->notify=notify;
}

void wtk_fbank_feed(wtk_fbank_t *fbank, short *data, int len, int is_end)
{
	wtk_strbuf_t *mic=fbank->mic;
	int i,j;
	int length;
	int nbin=fbank->nbin;
	int window_size=fbank->cfg->kaldi_fbank.window_size;
	int window_shift=fbank->cfg->kaldi_fbank.window_shift;
	int num_mel_bins=fbank->cfg->kaldi_fbank.num_mel_bins;
	float *strided_input=fbank->strided_input;
	float *mel_energies=fbank->mel_energies;
	float*fft_data=fbank->fft_data;
	float *specsum=fbank->spectrum;
	float fv1;
	float *fv;

	for(i=0;i<len;++i){
		fv1=data[i]*1.0/32768.0;
		wtk_strbuf_push(mic,(char *)&fv1, sizeof(float));
	}
	length = mic->pos/sizeof(float);
	while(length>=window_size){
		fv=(float *)mic->data;
		memcpy(strided_input, fv, window_size*sizeof(float));
		_get_window(fbank, strided_input, window_size);
		wtk_rfft_process_fft(fbank->rfft, fft_data, strided_input);

		//// specsum = fft_data.abs ** 2
		if(fbank->cfg->kaldi_fbank.use_power){
			specsum[0] = fft_data[0]*fft_data[0];
			specsum[nbin-1] = fft_data[nbin-1]*fft_data[nbin-1];
			for(i=1;i<nbin-1;++i){
				specsum[i] = fft_data[i]*fft_data[i] + fft_data[i+nbin-1]*fft_data[i+nbin-1];
			}
		}else{
			specsum[0] = fabs(fft_data[0]);
			specsum[nbin-1] = fabs(fft_data[nbin-1]);
			for(i=1;i<nbin-1;++i){
				specsum[i] = sqrtf(fft_data[i]*fft_data[i] + fft_data[i+nbin-1]*fft_data[i+nbin-1]);
			}
		}

		//// specsum * mel_energies
		memset(mel_energies, 0, (fbank->cfg->num_fbank)*sizeof(float));
		for(i=0;i<num_mel_bins;++i){
			for(j=0;j<nbin;++j){
				mel_energies[i] += specsum[j] * fbank->cfg->kaldi_fbank.bins[i][j];
			}
		}

		if(fbank->cfg->kaldi_fbank.use_log_fbank){
			for(i=0;i<num_mel_bins;++i){
				mel_energies[i] = logf(max(mel_energies[i], EPSILON));
			}
		}

		if(fbank->cfg->kaldi_fbank.use_energy){
			if(fbank->cfg->kaldi_fbank.htk_compat){
				mel_energies[num_mel_bins] = fbank->signal_log_energy;
			}else{
				memcpy(mel_energies+1, mel_energies, (num_mel_bins-1)*sizeof(float));
				mel_energies[0] = fbank->signal_log_energy;
			}
			if(fbank->cfg->kaldi_fbank.subtract_mean){
				_subtract_column_mean(mel_energies, num_mel_bins+1, fbank->cfg->kaldi_fbank.subtract_mean);
			}
		}else{
			if(fbank->cfg->kaldi_fbank.subtract_mean){
				_subtract_column_mean(mel_energies, num_mel_bins, fbank->cfg->kaldi_fbank.subtract_mean);
			}
		}
		// for(i=0;i<nbin;++i){
		// 	printf("%d %.12f\n", i, specsum[i]);
		// }
		// for(i=0;i<num_mel_bins;++i){
		// 	printf("%d %.12f\n", i, mel_energies[i]);
		// }
		// getchar();
		if(fbank->notify){
			fbank->notify(fbank->ths, mel_energies, fbank->cfg->num_fbank);
		}

		wtk_strbuf_pop(mic, NULL, window_shift*sizeof(float));
		length = mic->pos/sizeof(float);
	}
}
