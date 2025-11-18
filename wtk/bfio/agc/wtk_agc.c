#include "wtk_agc.h"

wtk_agc_t* wtk_agc_new(wtk_agc_cfg_t *cfg)
{
	wtk_agc_t *agc;

	agc=(wtk_agc_t *)wtk_malloc(sizeof(wtk_agc_t));
	agc->cfg=cfg;
	agc->ths=NULL;
	agc->notify=NULL;

	agc->mic=wtk_strbufs_new(agc->cfg->nmicchannel);
	agc->nbin=cfg->wins/2+1;
	agc->analysis_window=wtk_malloc(sizeof(float)*cfg->wins);///2);
	agc->synthesis_window=wtk_malloc(sizeof(float)*cfg->wins);///2);
	agc->analysis_mem=wtk_float_new_p2(cfg->nmicchannel, agc->nbin-1);
	agc->synthesis_mem=wtk_malloc(sizeof(float)*(agc->nbin-1));
	agc->rfft=wtk_drft_new2(cfg->wins);
	agc->rfft_in=(float*)wtk_malloc(sizeof(float)*(cfg->wins));

	agc->fft=wtk_complex_new_p2(cfg->nmicchannel, agc->nbin);

	agc->fftx=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*agc->nbin);

	agc->qmmse=NULL;
	if(cfg->use_qmmse){
		agc->qmmse=wtk_qmmse_new(&(cfg->qmmse));
	}

	agc->eq=NULL;
	if(cfg->use_eq)
	{
		agc->eq=wtk_equalizer_new(&(cfg->eq));
	}
	agc->bs_win=NULL;
	if(cfg->use_bs_win)
	{
		agc->bs_win=wtk_math_create_hanning_window2(cfg->wins/2);
	}

	agc->out=wtk_malloc(sizeof(float)*(agc->nbin-1));

	wtk_agc_reset(agc);

	return agc;
}

void wtk_agc_delete(wtk_agc_t *agc)
{
	wtk_strbufs_delete(agc->mic,agc->cfg->nmicchannel);

	wtk_free(agc->analysis_window);
	wtk_free(agc->synthesis_window);
	wtk_float_delete_p2(agc->analysis_mem, agc->cfg->nmicchannel);
	wtk_free(agc->synthesis_mem);
	wtk_free(agc->rfft_in);
	wtk_drft_delete2(agc->rfft);
	wtk_complex_delete_p2(agc->fft, agc->cfg->nmicchannel);

	if(agc->qmmse)
	{
		wtk_qmmse_delete(agc->qmmse);
	}

	if(agc->eq)
	{
		wtk_equalizer_delete(agc->eq);
	}
	if(agc->bs_win)
	{
		wtk_free(agc->bs_win);
	}

	wtk_free(agc->fftx);

	wtk_free(agc->out);

	wtk_free(agc);
}


void wtk_agc_start(wtk_agc_t *agc)
{
}

void wtk_agc_reset(wtk_agc_t *agc)
{
	int wins=agc->cfg->wins;
	int i;

	wtk_strbufs_reset(agc->mic,agc->cfg->nmicchannel);

	for (i=0;i<wins;++i)
	{
		agc->analysis_window[i] = sin((0.5+i)*PI/(wins));
	}
	wtk_drft_init_synthesis_window(agc->synthesis_window, agc->analysis_window, wins);

	wtk_float_zero_p2(agc->analysis_mem, agc->cfg->nmicchannel, (agc->nbin-1));
	memset(agc->synthesis_mem, 0, sizeof(float)*(agc->nbin-1));

	wtk_complex_zero_p2(agc->fft, agc->cfg->nmicchannel, agc->nbin);

	memset(agc->fftx, 0, sizeof(wtk_complex_t)*(agc->nbin));

	if(agc->qmmse)
	{
		wtk_qmmse_reset(agc->qmmse);
	}

	if(agc->eq)
	{
		wtk_equalizer_reset(agc->eq);
	}

	agc->mic_silcnt=0;
	agc->mic_sil=1;

	agc->bs_scale=1.0;
	agc->bs_last_scale=1.0;
	agc->bs_real_scale=1.0;
	agc->bs_max_cnt=0;

	agc->denoise_enable=1;
}


void wtk_agc_set_notify(wtk_agc_t *agc,void *ths,wtk_agc_notify_f notify)
{
	agc->notify=notify;
	agc->ths=ths;
}

static float wtk_agc_fft_energy(wtk_complex_t *fftx,int nbin)
{
	float f;
	int i;

	f=0;
	for(i=1; i<nbin-1; ++i)
	{
		f+=fftx[i].a*fftx[i].a+fftx[i].b*fftx[i].b;
	}

	return f;
}

void wtk_agc_control_bs(wtk_agc_t *agc, float *out, int len)
{
	float *bs_win=agc->bs_win;
	float max_out = agc->cfg->max_out;
	float out_max;
	int i;

	if(agc->mic_sil==0)
	{
		out_max=wtk_float_abs_max(out, len);
		if(out_max>max_out)
		{
			agc->bs_scale=max_out/out_max;
			if(agc->bs_scale<agc->bs_last_scale)
			{
				agc->bs_last_scale=agc->bs_scale;
			}else
			{
				agc->bs_scale=agc->bs_last_scale;
			}
			agc->bs_max_cnt=5;
		}
		if(bs_win){
			for(i=0; i<len/2; ++i)
			{
				out[i]*=agc->bs_scale * bs_win[i] + agc->bs_real_scale * (1.0-bs_win[i]);
			}
			for(i=len/2; i<len; ++i){
				out[i]*=agc->bs_scale;
			}
			agc->bs_real_scale = agc->bs_scale;
		}else{
			for(i=0; i<len; ++i){
				out[i]*=agc->bs_scale;
			}
		}
		if(agc->bs_max_cnt>0)
		{
			--agc->bs_max_cnt;
		}
		if(agc->bs_max_cnt<=0 && agc->bs_scale<1.0)
		{
			agc->bs_scale*=1.1f;
			agc->bs_last_scale=agc->bs_scale;
			if(agc->bs_scale>1.0)
			{
				agc->bs_scale=1.0;
				agc->bs_last_scale=1.0;
			}
		}
	}else
	{
		agc->bs_scale=1.0;
		agc->bs_last_scale=1.0;
		agc->bs_max_cnt=0;
	}
} 

void wtk_agc_feed(wtk_agc_t *agc,short *data,int len,int is_end)
{
	int i,j;
	int nbin=agc->nbin;
	int nmicchannel=agc->cfg->nmicchannel;
	int *mic_channel=agc->cfg->mic_channel;
	int channel=agc->cfg->channel;
	wtk_strbuf_t **mic=agc->mic;
	int wins=agc->cfg->wins;
	int fsize=wins/2;
	int length;
	float micenr;
	float micenr_thresh=agc->cfg->micenr_thresh;
	int micenr_cnt=agc->cfg->micenr_cnt;
	wtk_drft_t *rfft=agc->rfft;
	float *rfft_in=agc->rfft_in;
	wtk_complex_t **fft=agc->fft;
	float **analysis_mem=agc->analysis_mem;
	float *synthesis_mem=agc->synthesis_mem;
	float *analysis_window=agc->analysis_window, *synthesis_window=agc->synthesis_window;
	wtk_complex_t *fftx=agc->fftx;
	int clip_s = agc->cfg->clip_s;
	int clip_e = agc->cfg->clip_e;
	float *out=agc->out;
	short *pv=(short *)out;

	for(i=0;i<len;++i)
	{
		for(j=0; j<nmicchannel; ++j)
		{
			wtk_strbuf_push(mic[j],(char *)(data+mic_channel[j]),sizeof(short));
		}
		data+=channel;
	}
	length=mic[0]->pos/sizeof(short);
	while(length>=fsize)
	{
		for(i=0; i<nmicchannel; ++i)
		{
			wtk_drft_frame_analysis22(rfft, rfft_in, analysis_mem[i], fft[i], (short *)(mic[i]->data), wins, analysis_window);
		}

		memcpy(fftx, fft[0], sizeof(wtk_complex_t)*nbin);
		if(agc->qmmse && agc->denoise_enable)
		{
			wtk_qmmse_denoise(agc->qmmse, fftx);	
		}
		for(i=0;i<clip_s;++i){
			fftx[i].a = fftx[i].b = 0;
		}
		for(i=clip_e;i<nbin;++i){
			fftx[i].a = fftx[i].b = 0;
		}
		// static int cnt=0;
		// cnt++;
		micenr=wtk_agc_fft_energy(fftx, nbin);
		if(micenr>micenr_thresh)
		{
			// if(agc->mic_sil==1)
			// {
			// 	printf("sp start %f %f %f\n", 1.0/16000*cnt*(nbin-1),micenr,micenr_thresh);
			// }
			agc->mic_sil=0;
			agc->mic_silcnt=micenr_cnt;
		}else if(agc->mic_sil==0)
		{
			agc->mic_silcnt-=1;
			if(agc->mic_silcnt<=0)
			{
				// printf("sp end %f\n", 1.0/16000*cnt*(nbin-1));
				agc->mic_sil=1;
			}
		}

		wtk_strbufs_pop(mic, nmicchannel, fsize*sizeof(short));
		length=mic[0]->pos/sizeof(short);

	    wtk_drft_frame_synthesis22(rfft, rfft_in, synthesis_mem, fftx, out, wins, synthesis_window);
		if(agc->eq)
		{
			wtk_equalizer_feed_float(agc->eq, out, fsize);
		}
		wtk_agc_control_bs(agc, out, fsize);
		for(i=0; i<fsize; ++i)
		{
			pv[i]=floorf(out[i]+0.5);
		}
		if(agc->notify)
		{
			agc->notify(agc->ths,pv,fsize);
		}
	}
	if(is_end && length>0)
	{
		if(agc->notify)
		{
			pv=(short *)mic[0]->data;
			agc->notify(agc->ths,pv,length);
		}
	}
}

void wtk_agc_set_denoiseenable(wtk_agc_t *agc,int enable)
{
    agc->denoise_enable = enable;
}
void wtk_agc_set_noise_suppress(wtk_agc_t *agc,float noise_suppress)
{
    if(agc->qmmse){
        wtk_qmmse_cfg_set_noise_suppress(agc->qmmse->cfg, noise_suppress);
    }
}