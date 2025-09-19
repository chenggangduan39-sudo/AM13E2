#include "wtk_agc2_cfg.h"

void _mel2hz(float *z, float *f, int htk, int len)
{
	float f_0 = 0;
	float f_sp = 200.0 / 3.0;
	float brkfrq = 1000.0;
	float brkpt  = (brkfrq - f_0) / f_sp;
	float logstep = logf(6.4) / 27.0;
	int i;

	if(htk==1){
		f[0] = 700*(powf(10.0, z[0]/2595.0)-1.0);
	}

	for(i=0;i<len;++i){
		if(z[i] < brkpt){
			f[i] = f_0 + f_sp * z[i];
		}else{
			f[i] = brkfrq * expf(logstep * (z[i] - brkpt));
		}
	}
}
void _hz2mel(float *f, float *z, int htk, int len)
{
	float f_0 = 0;
	float f_sp = 200.0 / 3.0;
	float brkfrq = 1000.0;
	float brkpt  = (brkfrq - f_0) / f_sp;
	float logstep = logf(6.4) / 27.0;
	int i;

	if(htk==1){
		z[0] = 2595 * log10f(1+f[0]*1.0/700);
	}

	for(i=0;i<len;++i){
		if(f[i] < brkfrq){
			z[i] = (f[i] - f_0) / f_sp;
		}else{
			z[i] = brkpt + logf(f[i] / brkfrq) / logstep;
		}
	}
}

void _fft2melmx(wtk_agc2_cfg_t *cfg, int nfft, int sr, int nfilts, int width, float minfrq, float maxfrq, int htkmel, int constamp, int num_params)
{
	float **f2a;
	float **f2a_diag;
	float *sf2a=cfg->sf2a;
	float **wts=cfg->wts;
	float *fftfrqs=cfg->fftfrqs;
	float *binfrqs=cfg->binfrqs;
	float *binfrqs_tmp;
	float loslope;
	float hislope;
	int i, j;
	float minmel;
	float maxmel;
	float fs[3]={0};
	float tmp;
	if(num_params < 2){
		sr = 8000;
	}
	if(num_params < 3){
		nfilts = 0;
	}
	if(num_params < 4){
		width = 1.0;
	}
	if(num_params < 5){
		minfrq = 0;
	}

	if(num_params < 6){
		maxfrq = sr/2;
	}
	if(num_params < 7){
		htkmel = 0;
	}
	if(num_params < 8){
		constamp = 0;
	}

	if(nfilts == 0){
		_hz2mel(&minfrq, &tmp, htkmel, 1);
		nfilts = (int)(tmp*0.5);
	}

	f2a = wtk_float_new_p2(nfilts, nfft/2+1);
	sf2a = (float *)wtk_malloc((nfft/2+1)*sizeof(float));
	f2a_diag = wtk_float_new_p2(nfilts, nfft/2+1);
	wts = wtk_float_new_p2(nfilts, nfft);
	fftfrqs = (float *)wtk_malloc((nfft/2+1)*sizeof(float));
	binfrqs = (float *)wtk_malloc((nfilts+2)*sizeof(float));
	binfrqs_tmp = (float *)wtk_malloc((nfilts+2)*sizeof(float));


	for(i=0;i<nfft/2+1;++i){
		fftfrqs[i] = i * 1.0/nfft*sr;
		// printf("%f\n",fftfrqs[i]);
	}
	_hz2mel(&minfrq, &minmel, htkmel, 1);
	_hz2mel(&maxfrq, &maxmel, htkmel, 1);
	// printf("%f %f %f %f\n", minfrq, maxfrq, minmel, maxmel);
	for(i=0;i<nfilts+2;++i){
		binfrqs_tmp[i] = minmel + i*1.0/(nfilts+1)*(maxmel-minmel);
		// printf("%f\n",binfrqs_tmp[i]);
	}
	_mel2hz(binfrqs_tmp, binfrqs, htkmel, nfilts+2);
	// for(i=0;i<nfilts+2;++i){
	// 	printf("%f\n",binfrqs[i]);
	// }

	for(i=0;i<nfilts;++i){
		fs[0] = binfrqs[i+0];
		fs[1] = binfrqs[i+1];
		fs[2] = binfrqs[i+2];
		fs[0] = fs[1] + width*(fs[0] - fs[1]);
		fs[2] = fs[1] + width*(fs[2] - fs[1]);
		for(j=0;j<nfft/2;++j){
			loslope = (fftfrqs[j] - fs[0])/(fs[1] - fs[0]);
			hislope = (fs[2] - fftfrqs[j])/(fs[2] - fs[1]);
			wts[i][j] = max(0, min(loslope, hislope));
		}
	}
	if(constamp == 0){
		for(i=0;i<nfft/2;++i){
			for(j=0;j<nfilts;++j){
				tmp = 2.0/(binfrqs[2+j] - binfrqs[j]);
				wts[j][i] *= tmp;
			}
		}
	}
	memset(sf2a, 0, (nfft/2+1)*sizeof(float));
	for(i=0;i<nfilts;++i){
		for(j=0;j<nfft/2;++j){
			f2a[i][j] = wts[i][j];
			sf2a[j] += wts[i][j];
		}
	}
	for(i=0;i<nfft/2+1;++i){
		for(j=0;j<nfilts;++j){
			tmp = 1.0/(sf2a[i]+(sf2a[i]==0?1:0));
			f2a_diag[j][i] = f2a[j][i] * tmp;
		}
	}

	wtk_float_delete_p2(wts, nfilts);
	wtk_free(sf2a);
	wtk_free(fftfrqs);
	wtk_free(binfrqs);
	wtk_free(binfrqs_tmp);
	cfg->f2a = f2a;
	cfg->f2a_diag = f2a_diag;
	cfg->nfilts = nfilts;
}

void wtk_agc2_tf_get_f2a(wtk_agc2_cfg_t *cfg)
{
	_fft2melmx(cfg, cfg->ftlen, cfg->rate, cfg->nbands, cfg->mwidth, 0, 0, 0, 0, 4);
}

void wtk_agc2_tf_param_update(wtk_agc2_cfg_t *cfg)
{
	cfg->ftlen = pow(2, round(logf(0.016*cfg->rate)/log(2)));
	cfg->winlen = cfg->ftlen;
	cfg->hoplen = cfg->winlen/2;
	cfg->ftsr = cfg->rate/cfg->hoplen;
	cfg->nbands = max(cfg->min_nbands, 20/cfg->f_scale);
	cfg->mwidth = cfg->f_scale * cfg->nbands*1.0/10;
	cfg->alpha = expf(-(1.0/cfg->ftsr)/cfg->t_scale);
	cfg->wins = cfg->ftlen;
	cfg->clip_s=(cfg->clip_s*1.0*cfg->wins)/cfg->rate;
	cfg->clip_e=(cfg->clip_e*1.0*cfg->wins)/cfg->rate;

	// printf("ftlen=%d, winlen=%d, hoplen=%d, ftsr=%f, nbands=%d, mwidth=%f\n",cfg->ftlen,cfg->winlen,cfg->hoplen,cfg->ftsr,cfg->nbands,cfg->mwidth);
	wtk_agc2_tf_get_f2a(cfg);
}

int wtk_agc2_cfg_init(wtk_agc2_cfg_t *cfg)
{
	cfg->channel=0;
	cfg->nmicchannel=0;
	cfg->mic_channel=NULL;

    cfg->wins=1024;

	wtk_qmmse_cfg_init(&(cfg->qmmse));

	cfg->micenr_thresh=300;
	cfg->micenr_cnt=10;

	cfg->main_cfg=NULL;
	cfg->mbin_cfg=NULL;

	cfg->rate=16000;

	wtk_equalizer_cfg_init(&(cfg->eq));
	cfg->use_eq=0;

	cfg->clip_s=0;
	cfg->clip_e=8000;

	cfg->use_qmmse=1;

	cfg->t_scale=0.5;
	cfg->f_scale=30.0;
	cfg->max_gain_back=0.2;
	cfg->min_nbands=10;
	cfg->use_tf_agc=0;

	cfg->f2a=NULL;
	cfg->f2a_diag=NULL;
	return 0;
}

int wtk_agc2_cfg_clean(wtk_agc2_cfg_t *cfg)
{
	if(cfg->mic_channel)
	{
		wtk_free(cfg->mic_channel);
	}
	if(cfg->f2a){
		wtk_float_delete_p2(cfg->f2a, cfg->nfilts);
	}
	if(cfg->f2a_diag){
		wtk_float_delete_p2(cfg->f2a_diag, cfg->nfilts);
	}
	wtk_qmmse_cfg_clean(&(cfg->qmmse));
	wtk_equalizer_cfg_clean(&(cfg->eq));

	return 0;
}

int wtk_agc2_cfg_update_local(wtk_agc2_cfg_t *cfg,wtk_local_cfg_t *m)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	int ret;
	wtk_array_t *a;
	int i;

	lc=m;
	wtk_local_cfg_update_cfg_i(lc,cfg,wins,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,micenr_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,micenr_cnt,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_eq,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,clip_s,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,clip_e,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_qmmse,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,t_scale,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,f_scale,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,max_gain_back,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,min_nbands,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_tf_agc,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,channel,v);

	a=wtk_local_cfg_find_array_s(lc,"mic_channel");
	if(a)
	{
		cfg->mic_channel=(int*)wtk_malloc(sizeof(int)*a->nslot);
		cfg->nmicchannel=a->nslot;
		for(i=0;i<a->nslot;++i)
		{
			v=((wtk_string_t**)a->slot)[i];
			cfg->mic_channel[i]=wtk_str_atoi(v->data,v->len);
		}
	}
	lc=wtk_local_cfg_find_lc_s(m,"qmmse");
	if(lc)
	{
        ret=wtk_qmmse_cfg_update_local(&(cfg->qmmse),lc);
		cfg->qmmse.step=cfg->wins/2;
        if(ret!=0){goto end;}
    }

	lc=wtk_local_cfg_find_lc_s(m,"eq");
	if(lc)
	{
		ret=wtk_equalizer_cfg_update_local(&(cfg->eq),lc);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return ret;
}

int wtk_agc2_cfg_update(wtk_agc2_cfg_t *cfg)
{
	int ret;

	ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
	if(ret!=0){goto end;}
	ret=wtk_equalizer_cfg_update(&(cfg->eq));
	if(ret!=0){goto end;}
	if(cfg->channel<cfg->nmicchannel)
	{
		cfg->channel=cfg->nmicchannel;
	}
	if(cfg->use_tf_agc){
		wtk_agc2_tf_param_update(cfg);
	}else{
		cfg->clip_s=(cfg->clip_s*1.0*cfg->wins)/cfg->rate;
		cfg->clip_e=(cfg->clip_e*1.0*cfg->wins)/cfg->rate;
	}

	ret=0;
end:
	return ret;
}

int wtk_agc2_cfg_update2(wtk_agc2_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
	if(ret!=0){goto end;}
	ret=wtk_equalizer_cfg_update(&(cfg->eq));
	if(ret!=0){goto end;}

	if(cfg->channel<cfg->nmicchannel)
	{
		cfg->channel=cfg->nmicchannel;
	}

	if(cfg->use_tf_agc){
		wtk_agc2_tf_param_update(cfg);
	}else{
		cfg->clip_s=(cfg->clip_s*1.0*cfg->wins)/cfg->rate;
		cfg->clip_e=(cfg->clip_e*1.0*cfg->wins)/cfg->rate;
	}

	ret=0;
end:
	return ret;
}

wtk_agc2_cfg_t* wtk_agc2_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_agc2_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_agc2_cfg,fn);
	if(!main_cfg)
	{
		return NULL;
	}
	cfg=(wtk_agc2_cfg_t*)main_cfg->cfg;
	cfg->main_cfg = main_cfg;
	return cfg;
}

void wtk_agc2_cfg_delete(wtk_agc2_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_agc2_cfg_t* wtk_agc2_cfg_new_bin(char *fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_agc2_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_agc2_cfg,fn,"./cfg");
	if(!mbin_cfg)
	{
		return NULL;
	}
	cfg=(wtk_agc2_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_agc2_cfg_delete_bin(wtk_agc2_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}

wtk_agc2_cfg_t* wtk_agc2_cfg_new2(char *fn, char *fn2)
{
	wtk_main_cfg_t *main_cfg;
	wtk_agc2_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type5(wtk_agc2_cfg,fn,fn2);
	if(!main_cfg)
	{
		return NULL;
	}
	cfg=(wtk_agc2_cfg_t*)main_cfg->cfg;
	cfg->main_cfg = main_cfg;
	return cfg;
}

void wtk_agc2_cfg_delete2(wtk_agc2_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_agc2_cfg_t* wtk_agc2_cfg_new_bin2(char *fn, char *fn2)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_agc2_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type4(wtk_agc2_cfg,fn,"./cfg",fn2);
	if(!mbin_cfg)
	{
		return NULL;
	}
	cfg=(wtk_agc2_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_agc2_cfg_delete_bin2(wtk_agc2_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}