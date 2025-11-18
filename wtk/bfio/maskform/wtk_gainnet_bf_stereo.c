#include "wtk_gainnet_bf_stereo.h"
void wtk_gainnet_bf_stereo_denoise_on_gainnet(wtk_gainnet_bf_stereo_dra_t *gbfsdr, float *gain, int len, int is_end);
void wtk_gainnet_bf_stereo_agc_on_gainnet(wtk_gainnet_bf_stereo_dra_t *gbfsdr, float *gain, int len, int is_end);

void wtk_gainnet_bf_stereo_dra_init(wtk_gainnet_bf_stereo_dra_t *gbfsdr, wtk_gainnet_bf_stereo_cfg_t *cfg)
{
	gbfsdr->cfg=cfg;
	gbfsdr->nbin=cfg->wins/2+1;

	gbfsdr->bank_mic=wtk_bankfeat_new(&(cfg->bankfeat));

	gbfsdr->g=wtk_malloc(sizeof(float)*cfg->bankfeat.nb_bands);
	gbfsdr->lastg=wtk_malloc(sizeof(float)*cfg->bankfeat.nb_bands);
	gbfsdr->gf=wtk_malloc(sizeof(float)*gbfsdr->nbin);
	gbfsdr->g2=wtk_malloc(sizeof(float)*cfg->bankfeat.nb_bands);
	gbfsdr->lastg2=wtk_malloc(sizeof(float)*cfg->bankfeat.nb_bands);
	gbfsdr->gf2=wtk_malloc(sizeof(float)*gbfsdr->nbin);

	gbfsdr->feature_lm=NULL;
	if(cfg->featm_lm>2)
	{
		gbfsdr->feature_lm=wtk_malloc(sizeof(float)*cfg->bankfeat.nb_features*(cfg->featm_lm-1));
	}

	gbfsdr->gainnet2=wtk_gainnet2_new(cfg->gainnet2);
	wtk_gainnet2_set_notify(gbfsdr->gainnet2, gbfsdr, (wtk_gainnet2_notify_f)wtk_gainnet_bf_stereo_denoise_on_gainnet);

	gbfsdr->agc_gainnet=NULL;
	if(cfg->agc_gainnet)
	{
		gbfsdr->agc_gainnet=wtk_gainnet_new(cfg->agc_gainnet);
		wtk_gainnet_set_notify(gbfsdr->agc_gainnet, gbfsdr, (wtk_gainnet_notify_f)wtk_gainnet_bf_stereo_agc_on_gainnet);
	}

	gbfsdr->qmmse=NULL;
	if(cfg->use_qmmse)
	{
		gbfsdr->qmmse=wtk_qmmse_new(&(cfg->qmmse));
	}
}

void wtk_gainnet_bf_stereo_dra_clean(wtk_gainnet_bf_stereo_dra_t *gbfsdr)
{
	wtk_bankfeat_delete(gbfsdr->bank_mic);

	wtk_free(gbfsdr->g);
	wtk_free(gbfsdr->lastg);
	wtk_free(gbfsdr->gf);
	wtk_free(gbfsdr->g2);
	wtk_free(gbfsdr->lastg2);
	wtk_free(gbfsdr->gf2);

	if(gbfsdr->qmmse)
	{
		wtk_qmmse_delete(gbfsdr->qmmse);
	}

	wtk_gainnet2_delete(gbfsdr->gainnet2);

	if(gbfsdr->agc_gainnet)
	{
		wtk_gainnet_delete(gbfsdr->agc_gainnet);
	}

	if(gbfsdr->feature_lm)
	{
		wtk_free(gbfsdr->feature_lm);
	}
}

void wtk_gainnet_bf_stereo_dra_reset(wtk_gainnet_bf_stereo_dra_t *gbfsdr)
{
	wtk_bankfeat_reset(gbfsdr->bank_mic);

	memset(gbfsdr->g, 0, sizeof(float)*gbfsdr->cfg->bankfeat.nb_bands);
	memset(gbfsdr->lastg, 0, sizeof(float)*gbfsdr->cfg->bankfeat.nb_bands);
	memset(gbfsdr->gf, 0, sizeof(float)*gbfsdr->nbin);

	memset(gbfsdr->g2, 0, sizeof(float)*gbfsdr->cfg->bankfeat.nb_bands);
	memset(gbfsdr->lastg2, 0, sizeof(float)*gbfsdr->cfg->bankfeat.nb_bands);
	memset(gbfsdr->gf2, 0, sizeof(float)*gbfsdr->nbin);

	wtk_gainnet2_reset(gbfsdr->gainnet2);

	if(gbfsdr->agc_gainnet)
	{
		wtk_gainnet_reset(gbfsdr->agc_gainnet);
	}

	if(gbfsdr->qmmse)
	{
		wtk_qmmse_reset(gbfsdr->qmmse);
	}

	if(gbfsdr->feature_lm)
	{
		memset(gbfsdr->feature_lm, 0, sizeof(float)*gbfsdr->cfg->bankfeat.nb_features*(gbfsdr->cfg->featm_lm-1));
	}
}

wtk_gainnet_bf_stereo_t* wtk_gainnet_bf_stereo_new(wtk_gainnet_bf_stereo_cfg_t *cfg)
{
	wtk_gainnet_bf_stereo_t *gbf_stereo;
	int i;
	int noutchannel=cfg->repate_outchannel>1?1:cfg->noutchannel;

	gbf_stereo=(wtk_gainnet_bf_stereo_t *)wtk_malloc(sizeof(wtk_gainnet_bf_stereo_t));
	gbf_stereo->cfg=cfg;
	gbf_stereo->ths=NULL;
	gbf_stereo->notify=NULL;

	gbf_stereo->mic=wtk_strbufs_new(gbf_stereo->cfg->nmicchannel);

	gbf_stereo->nbin=cfg->wins/2+1;
	gbf_stereo->analysis_window=wtk_malloc(sizeof(float)*cfg->wins);///2);
	gbf_stereo->synthesis_window=wtk_malloc(sizeof(float)*cfg->wins);///2);
	gbf_stereo->analysis_mem=wtk_float_new_p2(cfg->nmicchannel, gbf_stereo->nbin-1);
	gbf_stereo->synthesis_mem=wtk_float_new_p2(noutchannel, gbf_stereo->nbin-1);
	gbf_stereo->rfft=wtk_drft_new(cfg->wins);
	gbf_stereo->rfft_in=(float*)wtk_malloc(sizeof(float)*(cfg->wins));

	gbf_stereo->fft=wtk_complex_new_p2(cfg->nmicchannel, gbf_stereo->nbin);

	gbf_stereo->fftx=wtk_complex_new_p2(noutchannel,gbf_stereo->nbin);

	gbf_stereo->gbfsdr=(wtk_gainnet_bf_stereo_dra_t *)wtk_malloc(sizeof(wtk_gainnet_bf_stereo_dra_t)*noutchannel);
	for(i=0; i<noutchannel; ++i)
	{
		wtk_gainnet_bf_stereo_dra_init(gbf_stereo->gbfsdr+i, cfg);
	}

	gbf_stereo->covm=(wtk_covm_t **)wtk_malloc(sizeof(wtk_covm_t*)*noutchannel);
	for(i=0; i<noutchannel; ++i)
	{
		gbf_stereo->covm[i]=wtk_covm_new(&(cfg->covm), gbf_stereo->nbin, cfg->nmicchannel);
	}
	gbf_stereo->bf=wtk_malloc(sizeof(wtk_bf_t *)*noutchannel);
	for(i=0; i<noutchannel; ++i)
	{
		gbf_stereo->bf[i]=wtk_bf_new(&(cfg->bf), gbf_stereo->cfg->wins);
	}

	gbf_stereo->scale=(float *)wtk_malloc(sizeof(float)*noutchannel);
	gbf_stereo->last_scale=(float *)wtk_malloc(sizeof(float)*noutchannel);
	gbf_stereo->max_cnt=(int *)wtk_malloc(sizeof(int)*noutchannel);
	
	gbf_stereo->eq=NULL;
	if(cfg->use_eq)
	{
		gbf_stereo->eq=wtk_malloc(sizeof(wtk_equalizer_t)*noutchannel);
		for(i=0; i<noutchannel; ++i)
		{
			gbf_stereo->eq[i]=wtk_equalizer_new(&(cfg->eq));
		}
	}

	gbf_stereo->out=wtk_float_new_p2(cfg->noutchannel,(gbf_stereo->nbin-1));

	wtk_gainnet_bf_stereo_reset(gbf_stereo);

	return gbf_stereo;
}

void wtk_gainnet_bf_stereo_delete(wtk_gainnet_bf_stereo_t *gbf_stereo)
{
	int i;
	int noutchannel=gbf_stereo->cfg->repate_outchannel>1?1:gbf_stereo->cfg->noutchannel;

	wtk_strbufs_delete(gbf_stereo->mic,gbf_stereo->cfg->nmicchannel);

	wtk_free(gbf_stereo->analysis_window);
	wtk_free(gbf_stereo->synthesis_window);
	wtk_float_delete_p2(gbf_stereo->analysis_mem, gbf_stereo->cfg->nmicchannel);
	wtk_float_delete_p2(gbf_stereo->synthesis_mem, noutchannel);
	wtk_free(gbf_stereo->rfft_in);
	wtk_drft_delete(gbf_stereo->rfft);
	wtk_complex_delete_p2(gbf_stereo->fft, gbf_stereo->cfg->nmicchannel);

	for(i=0;i<noutchannel;++i)
	{
		wtk_bf_delete(gbf_stereo->bf[i]);
		wtk_covm_delete(gbf_stereo->covm[i]);
		if(gbf_stereo->eq)
		{
			wtk_equalizer_delete(gbf_stereo->eq[i]);
		}
		wtk_gainnet_bf_stereo_dra_clean(gbf_stereo->gbfsdr+i);
	}
	wtk_free(gbf_stereo->bf);
	wtk_free(gbf_stereo->covm);
	if(gbf_stereo->eq)
	{
		wtk_free(gbf_stereo->eq);
	}
	wtk_free(gbf_stereo->gbfsdr);

	wtk_free(gbf_stereo->scale);
	wtk_free(gbf_stereo->last_scale);
	wtk_free(gbf_stereo->max_cnt);

	wtk_complex_delete_p2(gbf_stereo->fftx, noutchannel);
	wtk_float_delete_p2(gbf_stereo->out, gbf_stereo->cfg->noutchannel);

	wtk_free(gbf_stereo);
}


void wtk_gainnet_bf_stereo_start(wtk_gainnet_bf_stereo_t *gbf_stereo)
{
	int i;
	int noutchannel=gbf_stereo->cfg->repate_outchannel>1?1:gbf_stereo->cfg->noutchannel;

	for(i=0; i<noutchannel; ++i)
	{
		wtk_bf_update_ovec(gbf_stereo->bf[i],gbf_stereo->cfg->theta,gbf_stereo->cfg->phi);
		wtk_bf_init_w(gbf_stereo->bf[i]);
	}
}

void wtk_gainnet_bf_stereo_reset(wtk_gainnet_bf_stereo_t *gbf_stereo)
{
	int wins=gbf_stereo->cfg->wins;
	int i;
	int noutchannel=gbf_stereo->cfg->repate_outchannel>1?1:gbf_stereo->cfg->noutchannel;

	wtk_strbufs_reset(gbf_stereo->mic,gbf_stereo->cfg->nmicchannel);

	for (i=0;i<wins;++i)
	{
		gbf_stereo->analysis_window[i] = sin((0.5+i)*PI/(wins));
	}
	wtk_drft_init_synthesis_window(gbf_stereo->synthesis_window, gbf_stereo->analysis_window, wins);

	wtk_float_zero_p2(gbf_stereo->analysis_mem, gbf_stereo->cfg->nmicchannel, (gbf_stereo->nbin-1));
	wtk_float_zero_p2(gbf_stereo->synthesis_mem, noutchannel, (gbf_stereo->nbin-1));

	wtk_complex_zero_p2(gbf_stereo->fft, gbf_stereo->cfg->nmicchannel, gbf_stereo->nbin);

	for(i=0;i<noutchannel;++i)
	{
		wtk_bf_reset(gbf_stereo->bf[i]);
		wtk_covm_reset(gbf_stereo->covm[i]);
		wtk_gainnet_bf_stereo_dra_reset(gbf_stereo->gbfsdr+i);

		gbf_stereo->scale[i]=1.0;
		gbf_stereo->last_scale[i]=1.0;
		gbf_stereo->max_cnt[i]=0;
	}

	wtk_complex_zero_p2(gbf_stereo->fftx,noutchannel, gbf_stereo->nbin);

	gbf_stereo->mic_silcnt=0;
	gbf_stereo->mic_sil=1;
}


void wtk_gainnet_bf_stereo_set_notify(wtk_gainnet_bf_stereo_t *gbf_stereo,void *ths,wtk_gainnet_bf_stereo_notify_f notify)
{
	gbf_stereo->notify=notify;
	gbf_stereo->ths=ths;
}

static float wtk_gainnet_bf_stereo_fft_energy(wtk_complex_t *fftx,int nbin)
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

void wtk_gainnet_bf_stereo_denoise_on_gainnet(wtk_gainnet_bf_stereo_dra_t *gbfsdr, float *gain, int len, int is_end)
{
	memcpy(gbfsdr->g, gain, sizeof(float)*gbfsdr->cfg->bankfeat.nb_bands);
}

void wtk_gainnet_bf_stereo_aec_on_gainnet(wtk_gainnet_bf_stereo_dra_t *gbfsdr, float *gain, int len, int is_end)
{
	memcpy(gbfsdr->g, gain, sizeof(float)*gbfsdr->cfg->bankfeat.nb_bands);
}

void wtk_gainnet_bf_stereo_agc_on_gainnet(wtk_gainnet_bf_stereo_dra_t *gbfsdr, float *gain, int len, int is_end)
{
	memcpy(gbfsdr->g2, gain, sizeof(float)*gbfsdr->cfg->bankfeat.nb_bands);
}

void wtk_gainnet_bf_stereo_dra_feed(wtk_gainnet_bf_stereo_dra_t *gbfsdr, wtk_complex_t *fftx)
{
	int i;
	int nbin=gbfsdr->nbin;
	float *g=gbfsdr->g, *gf=gbfsdr->gf, *g2=gbfsdr->g2, *gf2=gbfsdr->gf2;
	float *lastg=gbfsdr->lastg, *lastg2=gbfsdr->lastg2;
	float agc_a=gbfsdr->cfg->agc_a;
	float agc_b=gbfsdr->cfg->agc_b;
	float g_a=gbfsdr->cfg->g_a;
	float g_b=gbfsdr->cfg->g_b;
	float g2_min=gbfsdr->cfg->g2_min;
	float g2_max=gbfsdr->cfg->g2_max;
	float g_min=gbfsdr->cfg->g_min;
	float g_max=gbfsdr->cfg->g_max;
	float g_minthresh=gbfsdr->cfg->g_minthresh;
	float ralpha=gbfsdr->cfg->ralpha;
	float ralpha2=gbfsdr->cfg->ralpha2;
	wtk_gainnet2_t *gainnet2=gbfsdr->gainnet2;
	wtk_gainnet_t *agc_gainnet=gbfsdr->agc_gainnet;
	wtk_bankfeat_t *bank_mic=gbfsdr->bank_mic;
	int nb_bands=bank_mic->cfg->nb_bands;
	int nb_features=bank_mic->cfg->nb_features;
	wtk_qmmse_t *qmmse=gbfsdr->qmmse;
	float *qmmse_gain;
	int featm_lm=gbfsdr->cfg->featm_lm;
	float *feature_lm=gbfsdr->feature_lm;
	float gbias=gbfsdr->cfg->gbias;

	wtk_bankfeat_flush_frame_features(bank_mic, fftx);
	if(qmmse)
	{
		wtk_qmmse_flush_denoise_mask(qmmse, fftx);
	}
	if(0)
	{
		for (i=1; i<nbin-1; ++i)
		{
			gf[i]=0;
			gf2[i]=0;
		}
	}else
	{
		 if(gainnet2 && feature_lm)
		{
			wtk_gainnet2_feed2(gainnet2, bank_mic->features ,nb_features, feature_lm, nb_features*(featm_lm-1), 0);
		}   
		if(gbfsdr->cfg->use_gsigmoid)
		{
			for(i=0; i<nb_bands; ++i)
			{
				g[i]=max(g_min,g[i]);
				g[i]=min(g_max,g[i]);
				g[i]=-1/g_a*(logf(1/g[i]-1)-g_b);
				if(g[i]<0){g[i]=0;};
				if(g[i]>1){g[i]=1;};
			}
		}
		if(agc_gainnet)
		{
			wtk_gainnet_feed2(agc_gainnet, bank_mic->features, nb_features, g, nb_bands, 0);  
			for(i=0; i<nb_bands; ++i)
			{
				g2[i]=max(g2_min,g2[i]);
				g2[i]=min(g2_max,g2[i]);
				g2[i]=-1/agc_a*(logf(1/g2[i]-1)-agc_b);
				if(g2[i]<0){g2[i]=g[i];};
			}
			for(i=0; i<nb_bands; ++i)
			{
				g2[i]=max(g2[i],lastg2[i]*ralpha2);
				lastg2[i]=g2[i];
			}
			wtk_bankfeat_interp_band_gain(bank_mic, nbin, gf2, g2);
		}
		for (i=0;i<nb_bands;++i)
		{
			g[i] = max(g[i], ralpha*lastg[i]);
			lastg[i] = g[i];
		}
		wtk_bankfeat_interp_band_gain(bank_mic, nbin, gf, g);
		if(gbias>0)
		{
			for (i=1; i<nbin-1; ++i)
			{
				gf[i]=min(gf[i]+gbias,1);
			}
			if(agc_gainnet)
			{
				for (i=1; i<nbin-1; ++i)
				{
					gf2[i]=gf2[i]/gf[i];
					gf2[i]=max(1,gf2[i]);
				}
			}
		}else if(qmmse && agc_gainnet)
		{
			qmmse_gain=qmmse->gain;
			for (i=1; i<nbin-1; ++i)
			{
				if(gf[i]>g_minthresh)
				{
					gf2[i]=gf2[i]/gf[i];
				}else
				{
					gf2[i]=1;
				}
				if(gf[i]>qmmse_gain[i])
				{
					gf2[i]*=qmmse_gain[i]/gf[i];
					gf[i]=qmmse_gain[i];
				}
				gf2[i]=max(1,gf2[i]);
			}
		}else if(qmmse)
		{	
			qmmse_gain=qmmse->gain;
			for (i=1; i<nbin-1; ++i)
			{
				if(gf[i]>qmmse_gain[i] )
				{
					gf[i]=qmmse_gain[i];
				}
			}
		}else if(agc_gainnet)
		{
			for (i=1; i<nbin-1; ++i)
			{
				if(gf[i]>g_minthresh)
				{
					gf2[i]=gf2[i]/gf[i];
				}else
				{
					gf2[i]=1;
				}
				gf2[i]=max(1,gf2[i]);
			}
		}
	}
	if(feature_lm && featm_lm>1)
	{
		memmove(feature_lm+nb_features,feature_lm,sizeof(float)*nb_features*(featm_lm-2));
		memcpy(feature_lm,bank_mic->features,sizeof(float)*nb_features);
	}
}


void wtk_gainnet_bf_stereo_feed_dra(wtk_gainnet_bf_stereo_t *gbf_stereo, wtk_complex_t **fft)
{
	int noutchannel=gbf_stereo->cfg->repate_outchannel>1?1:gbf_stereo->cfg->noutchannel;
	int *out_channel=gbf_stereo->cfg->out_channel;
	int i,k;
	int nbin=gbf_stereo->nbin;
	wtk_complex_t **fftx=gbf_stereo->fftx;
	wtk_gainnet_bf_stereo_dra_t *gbfsdr=gbf_stereo->gbfsdr;

	for(k=0; k<nbin; ++k)
	{
		for(i=0; i<noutchannel; ++i)
		{
			fftx[i][k]=fft[out_channel[i]][k];
		}
	}
	for(i=0; i<noutchannel; ++i)
	{
		wtk_gainnet_bf_stereo_dra_feed(gbfsdr+i, fftx[i]);
	}
}


void wtk_gainnet_bf_stereo_feed_bf(wtk_gainnet_bf_stereo_t *gbf_stereo, wtk_complex_t **fft)
{
	wtk_complex_t **fftx=gbf_stereo->fftx;
	int k,nbin=gbf_stereo->nbin;
	int i,j;
	int nmicchannel=gbf_stereo->cfg->nmicchannel;
	int noutchannel=gbf_stereo->cfg->repate_outchannel>1?1:gbf_stereo->cfg->noutchannel;
	int *out_channel=gbf_stereo->cfg->out_channel;
	wtk_bf_t **bf=gbf_stereo->bf;
	wtk_covm_t *covm;
	int b;
	wtk_gainnet_bf_stereo_dra_t *gbfsdr;
	float gf;
	wtk_complex_t fft2[64];
	wtk_complex_t ffts[64];
	wtk_complex_t ffty[64];
	int clip_s=gbf_stereo->cfg->clip_s;
	int clip_e=gbf_stereo->cfg->clip_e;

	for(i=0;i<noutchannel;++i)
	{
		fftx[i][0].a=fftx[i][0].b=0;
		fftx[i][nbin-1].a=fftx[i][nbin-1].b=0;
		covm = gbf_stereo->covm[i];
		for(k=0; k<=clip_s; ++k)
		{
			fftx[i][k].a=fftx[i][k].b=0;
		}
		gbfsdr=gbf_stereo->gbfsdr+i;
		for(k=clip_s+1; k<clip_e; ++k)
		{
			gf=gbfsdr->gf[k];
			for(j=0; j<nmicchannel; ++j)
			{
				ffts[j].a=fft[j][k].a*gf;
				ffts[j].b=fft[j][k].b*gf;

				ffty[j].a=fft[j][k].a*(1-gf);
				ffty[j].b=fft[j][k].b*(1-gf);
			}
			if(gbf_stereo->cfg->use_fftsbf)
			{
				for(j=0; j<nmicchannel; ++j)
				{
					fft2[j]=ffts[j];
				}
			}else
			{
				for(j=0; j<nmicchannel; ++j)
				{
					fft2[j]=fft[j][k];
				}
			}

			b=0;
			b=wtk_covm_feed_fft3(covm, ffty, k, 1);
			if(b==1)
			{
				wtk_bf_update_ncov(bf[i], covm->ncov, k);
			}
			if(covm->scov)
			{
				b=wtk_covm_feed_fft3(covm, ffts, k, 0);
				if(b==1)
				{
					wtk_bf_update_scov(bf[i], covm->scov, k);
				}
			}
			if(b==1)
			{
				wtk_bf_update_w2(bf[i], k, out_channel[i]);
			}
			wtk_bf_output_fft_k(bf[i], fft2, fftx[i]+k, k);
		}
		for(k=clip_e; k<nbin; ++k)
		{
			fftx[i][k].a=fftx[i][k].b=0;
		}
	}
}

void wtk_gainnet_bf_stereo_feed_agc2(wtk_gainnet_bf_stereo_t *gbf_stereo, wtk_complex_t **fft)
{
	wtk_gainnet_bf_stereo_dra_t *gbfsdr;
	int noutchannel=gbf_stereo->cfg->repate_outchannel>1?1:gbf_stereo->cfg->noutchannel;
	int nbin=gbf_stereo->nbin;
	float *gf2;
	int i,j;
	int nb_bands=gbf_stereo->cfg->bankfeat.nb_bands;
	float E;
	float *Ex;
	
	for(j=0; j<noutchannel; ++j)
	{
		gbfsdr=gbf_stereo->gbfsdr+j;
		gf2=gbfsdr->gf2;
		Ex=gbfsdr->bank_mic->Ex;
		wtk_bankfeat_compute_band_energy(gbfsdr->bank_mic, Ex, fft[j]);
		E=0;
		for (i=0;i<nb_bands;++i)
		{
			E += Ex[i];
		}
		if (!gbfsdr->bank_mic->silence && E>gbf_stereo->cfg->agce_thresh)
		{
			for (i=1;i<nbin-1;++i)
			{
				fft[j][i].a *= gf2[i];
				fft[j][i].b *= gf2[i];
			}
		}
	}
}


void wtk_gainnet_bf_stereo_control_bs(wtk_gainnet_bf_stereo_t *gbf_stereo, float *out, int len, int nch)
{
	float out_max;
	float *scale=gbf_stereo->scale;
	float *last_scale=gbf_stereo->last_scale;
	int *max_cnt=gbf_stereo->max_cnt;
	int i;

	if(gbf_stereo->mic_sil==0)
	{
		out_max=wtk_float_abs_max(out, len);
		if(out_max>32700.0)
		{
			scale[nch]=32700.0f/out_max;
			if(scale[nch]<last_scale[nch])
			{
				last_scale[nch]=scale[nch];
			}else
			{
				scale[nch]=last_scale[nch];
			}
			max_cnt[nch]=5;
		}
		for(i=0; i<len; ++i)
		{
			out[i]*=scale[nch];
		}
		if(max_cnt[nch]>0)
		{
			--max_cnt[nch];
		}
		if(max_cnt[nch]<=0 && scale[nch]<1.0)
		{
			scale[nch]*=1.1f;
			last_scale[nch]=scale[nch];
			if(scale[nch]>1.0)
			{
				scale[nch]=1.0;
				last_scale[nch]=1.0;
			}
		}
	}else
	{
		scale[nch]=1.0;
		last_scale[nch]=1.0;
		max_cnt[nch]=0;
	}
} 


void wtk_gainnet_bf_stereo_feed(wtk_gainnet_bf_stereo_t *gbf_stereo,short *data,int len,int is_end)
{
	int i,j;
	int nbin=gbf_stereo->nbin;
	int channel=gbf_stereo->cfg->channel;
	int nmicchannel=gbf_stereo->cfg->nmicchannel;
	int noutchannel=gbf_stereo->cfg->repate_outchannel>1?1:gbf_stereo->cfg->noutchannel;
	int repate_outchannel=gbf_stereo->cfg->repate_outchannel;
	int *mic_channel=gbf_stereo->cfg->mic_channel;
	wtk_strbuf_t **mic=gbf_stereo->mic;
	int wins=gbf_stereo->cfg->wins;
	int fsize=wins/2;
	int length;
	float micenr;
	float micenr_thresh=gbf_stereo->cfg->micenr_thresh;
	int micenr_cnt=gbf_stereo->cfg->micenr_cnt;
	wtk_drft_t *rfft=gbf_stereo->rfft;
	float *rfft_in=gbf_stereo->rfft_in;
	wtk_complex_t **fft=gbf_stereo->fft;
	float **analysis_mem=gbf_stereo->analysis_mem;
	float **synthesis_mem=gbf_stereo->synthesis_mem;
	float *analysis_window=gbf_stereo->analysis_window, *synthesis_window=gbf_stereo->synthesis_window;
	wtk_complex_t **fftx=gbf_stereo->fftx;
	float **out=gbf_stereo->out;
	short *pv[64];

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
			wtk_drft_frame_analysis2(rfft, rfft_in, analysis_mem[i], fft[i], (short *)(mic[i]->data), wins, analysis_window);
		}

		wtk_gainnet_bf_stereo_feed_dra(gbf_stereo, fft);
		wtk_gainnet_bf_stereo_feed_bf(gbf_stereo, fft);
		
		if(gbf_stereo->gbfsdr->agc_gainnet)
		{
            wtk_gainnet_bf_stereo_feed_agc2(gbf_stereo, fftx);
		}

		// static int cnt=0;
		// cnt++;
		micenr=wtk_gainnet_bf_stereo_fft_energy(fftx[0], nbin);
		if(micenr>micenr_thresh)
		{
			// if(gbf_stereo->mic_sil==1)
			// {
			// 	printf("sp start %f %f %f\n", 1.0/16000*cnt*(nbin-1),micenr,micenr_thresh);
			// }
			gbf_stereo->mic_sil=0;
			gbf_stereo->mic_silcnt=micenr_cnt;
		}else if(gbf_stereo->mic_sil==0)
		{
			gbf_stereo->mic_silcnt-=1;
			if(gbf_stereo->mic_silcnt<=0)
			{
				// printf("sp end %f\n", 1.0/16000*cnt*(nbin-1));
				gbf_stereo->mic_sil=1;
			}
		}

		wtk_strbufs_pop(mic, nmicchannel, fsize*sizeof(short));
		length=mic[0]->pos/sizeof(short);

		for(i=0;i<noutchannel;++i)
		{
	    	wtk_drft_frame_synthesis(rfft, rfft_in, synthesis_mem[i], fftx[i], out[i], wins, synthesis_window);
		}
		if(gbf_stereo->eq)
		{
			for(i=0;i<noutchannel;++i)
			{
				wtk_equalizer_feed_float(gbf_stereo->eq[i], out[i], fsize);
			}
		}
		for(i=0;i<noutchannel;++i)
		{
			wtk_gainnet_bf_stereo_control_bs(gbf_stereo, out[i], fsize, i);
		}
		if(repate_outchannel>1)
		{
			pv[0]=(short *)out[0];
			for(j=0; j<fsize; ++j)
			{
				pv[0][j]=floorf(out[0][j]+0.5);
			}
			for(i=1;i<gbf_stereo->cfg->noutchannel;++i)
			{
				pv[i]=(short *)out[i];
				for(j=0; j<fsize; ++j)
				{
					pv[i][j]=pv[0][j];
				}
			}
		}else
		{
			for(i=0;i<noutchannel;++i)
			{
				pv[i]=(short *)out[i];
				for(j=0; j<fsize; ++j)
				{
					pv[i][j]=floorf(out[i][j]+0.5);
				}
			}
		}
		if(gbf_stereo->notify)
		{
			gbf_stereo->notify(gbf_stereo->ths,pv,fsize);
		}
	}
	if(is_end && length>0)
	{
		if(gbf_stereo->notify)
		{
			for(i=0;i<gbf_stereo->cfg->noutchannel;++i)
			{
				pv[i]=(short *)mic[0]->data;
			}
			gbf_stereo->notify(gbf_stereo->ths,pv,length);
		}
	}
}