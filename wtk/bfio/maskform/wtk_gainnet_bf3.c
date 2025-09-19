#include "wtk_gainnet_bf3.h"
void wtk_gainnet_bf3_denoise_on_gainnet(wtk_gainnet_bf3_denoise_t *gdenoise, float *gain, int len, int is_end);
void wtk_gainnet_bf3_agc_on_gainnet(wtk_gainnet_bf3_denoise_t *gdenoise, float *gain, int len, int is_end);
void wtk_gainnet_bf3_on_ssl2(wtk_gainnet_bf3_t *gainnet_bf3, wtk_ssl2_extp_t *nbest_extp,int nbest, int ts,int te);

void wtk_gainnet_bf3_denoise_init(wtk_gainnet_bf3_denoise_t *gdenoise, wtk_gainnet_bf3_cfg_t *cfg)
{
	gdenoise->cfg=cfg;
	gdenoise->nbin=cfg->wins/2+1;

	gdenoise->bank_mic=wtk_bankfeat_new(&(cfg->bankfeat));

	gdenoise->xvec=NULL;
	if(cfg->use_xvector)
	{
		gdenoise->xvec=(float *)wtk_malloc(sizeof(float)*cfg->xv_len);
	}

	gdenoise->g=wtk_malloc(sizeof(float)*cfg->bankfeat.nb_bands);
	gdenoise->lastg=wtk_malloc(sizeof(float)*cfg->bankfeat.nb_bands);
	gdenoise->gf=wtk_malloc(sizeof(float)*gdenoise->nbin);
	gdenoise->g2=wtk_malloc(sizeof(float)*cfg->bankfeat.nb_bands);
	gdenoise->lastg2=wtk_malloc(sizeof(float)*cfg->bankfeat.nb_bands);
	gdenoise->gf2=wtk_malloc(sizeof(float)*gdenoise->nbin);

	gdenoise->feature_lm=NULL;
	if(cfg->featm_lm>2)
	{
		gdenoise->feature_lm=wtk_malloc(sizeof(float)*cfg->bankfeat.nb_features*(cfg->featm_lm-1));
	}

	gdenoise->gainnet4=NULL;
	gdenoise->gainnet=NULL;
	if(cfg->use_gainnet2)
	{
		gdenoise->gainnet2=wtk_gainnet2_new(cfg->gainnet2);
		wtk_gainnet2_set_notify(gdenoise->gainnet2, gdenoise, (wtk_gainnet2_notify_f)wtk_gainnet_bf3_denoise_on_gainnet);
	}else if(cfg->use_gainnet4)
	{
		gdenoise->gainnet4=wtk_gainnet4_new(cfg->gainnet4);
		wtk_gainnet4_set_notify(gdenoise->gainnet4, gdenoise, (wtk_gainnet4_notify_f)wtk_gainnet_bf3_denoise_on_gainnet);
	}else
	{
		gdenoise->gainnet=wtk_gainnet7_new(cfg->gainnet);
		wtk_gainnet7_set_notify(gdenoise->gainnet, gdenoise, (wtk_gainnet7_notify_f)wtk_gainnet_bf3_denoise_on_gainnet);
	}

	gdenoise->agc_gainnet=NULL;
	if(cfg->agc_gainnet)
	{
		gdenoise->agc_gainnet=wtk_gainnet_new(cfg->agc_gainnet);
		wtk_gainnet_set_notify(gdenoise->agc_gainnet, gdenoise, (wtk_gainnet_notify_f)wtk_gainnet_bf3_agc_on_gainnet);
	}

	gdenoise->qmmse=NULL;
	if(cfg->use_qmmse)
	{
		gdenoise->qmmse=wtk_qmmse_new(&(cfg->qmmse));
	}
}

void wtk_gainnet_bf3_denoise_clean(wtk_gainnet_bf3_denoise_t *gdenoise)
{
	wtk_bankfeat_delete(gdenoise->bank_mic);
	if(gdenoise->xvec)
	{
		wtk_free(gdenoise->xvec);
	}

	wtk_free(gdenoise->g);
	wtk_free(gdenoise->lastg);
	wtk_free(gdenoise->gf);
	wtk_free(gdenoise->g2);
	wtk_free(gdenoise->lastg2);
	wtk_free(gdenoise->gf2);

	if(gdenoise->qmmse)
	{
		wtk_qmmse_delete(gdenoise->qmmse);
	}

	if(gdenoise->gainnet)
	{
		wtk_gainnet7_delete(gdenoise->gainnet);
	}else if(gdenoise->gainnet2)
	{
		wtk_gainnet2_delete(gdenoise->gainnet2);
	}else if(gdenoise->gainnet4)
	{
		wtk_gainnet4_delete(gdenoise->gainnet4);
	}

	if(gdenoise->agc_gainnet)
	{
		wtk_gainnet_delete(gdenoise->agc_gainnet);
	}

	if(gdenoise->feature_lm)
	{
		wtk_free(gdenoise->feature_lm);
	}
}

void wtk_gainnet_bf3_denoise_start(wtk_gainnet_bf3_denoise_t *gdenoise, char *xv_fn)
{
	FILE *xv;
	int i;
	float norm;
	int ret;

	if(gdenoise->xvec)
	{
		xv=fopen(xv_fn, "rb");
		ret = fread(gdenoise->xvec, sizeof(float), gdenoise->cfg->xv_len, xv);
		if(ret!=1){goto end;}
		norm=0;
		for(i=0;i<gdenoise->cfg->xv_len;++i)
		{
			norm+=gdenoise->xvec[i]*gdenoise->xvec[i];
		}
		norm=sqrtf(norm);
		for(i=0;i<gdenoise->cfg->xv_len;++i)
		{
			gdenoise->xvec[i]/=norm;
		}
end:
		fclose(xv);
	}
}

void wtk_gainnet_bf3_denoise_reset(wtk_gainnet_bf3_denoise_t *gdenoise)
{
	wtk_bankfeat_reset(gdenoise->bank_mic);

	memset(gdenoise->g, 0, sizeof(float)*gdenoise->cfg->bankfeat.nb_bands);
	memset(gdenoise->lastg, 0, sizeof(float)*gdenoise->cfg->bankfeat.nb_bands);
	memset(gdenoise->gf, 0, sizeof(float)*gdenoise->nbin);
	memset(gdenoise->g2, 0, sizeof(float)*gdenoise->cfg->bankfeat.nb_bands);
	memset(gdenoise->lastg2, 0, sizeof(float)*gdenoise->cfg->bankfeat.nb_bands);
	memset(gdenoise->gf2, 0, sizeof(float)*gdenoise->nbin);

	if(gdenoise->gainnet)
	{
		wtk_gainnet7_reset(gdenoise->gainnet);
	}else if(gdenoise->gainnet2)
	{
		wtk_gainnet2_reset(gdenoise->gainnet2);
	}else if(gdenoise->gainnet4)
	{
		wtk_gainnet4_reset(gdenoise->gainnet4);
	}

	if(gdenoise->agc_gainnet)
	{
		wtk_gainnet_reset(gdenoise->agc_gainnet);
	}

	if(gdenoise->qmmse)
	{
		wtk_qmmse_reset(gdenoise->qmmse);
	}

	if(gdenoise->feature_lm)
	{
		memset(gdenoise->feature_lm, 0, sizeof(float)*gdenoise->cfg->bankfeat.nb_features*(gdenoise->cfg->featm_lm-1));
	}
}

wtk_gainnet_bf3_t* wtk_gainnet_bf3_new(wtk_gainnet_bf3_cfg_t *cfg)
{
	wtk_gainnet_bf3_t *gainnet_bf3;

	gainnet_bf3=(wtk_gainnet_bf3_t *)wtk_malloc(sizeof(wtk_gainnet_bf3_t));
	gainnet_bf3->cfg=cfg;
	gainnet_bf3->ths=NULL;
	gainnet_bf3->notify=NULL;
	gainnet_bf3->ssl_ths=NULL;
    gainnet_bf3->notify_ssl=NULL;
	gainnet_bf3->ths_tr=NULL;
    gainnet_bf3->notify_tr=NULL;

	gainnet_bf3->mic=wtk_strbufs_new(gainnet_bf3->cfg->channel);


	gainnet_bf3->nbin=cfg->wins/2+1;
	gainnet_bf3->analysis_window=wtk_malloc(sizeof(float)*cfg->wins);
	gainnet_bf3->synthesis_window=wtk_malloc(sizeof(float)*cfg->wins);
	gainnet_bf3->analysis_mem=wtk_float_new_p2(cfg->channel, gainnet_bf3->nbin-1);
	gainnet_bf3->synthesis_mem=wtk_malloc(sizeof(float)*(gainnet_bf3->nbin-1));
	gainnet_bf3->rfft=wtk_drft_new(cfg->wins);
	gainnet_bf3->rfft_in=(float*)wtk_malloc(sizeof(float)*(cfg->wins));

	gainnet_bf3->howl_energy=NULL;
	gainnet_bf3->howl_scale=NULL;
	gainnet_bf3->howl_xld=NULL;
	gainnet_bf3->howl_power_x=NULL;
	gainnet_bf3->howl_W=NULL;
	gainnet_bf3->howl_cnt=NULL;;
	if(cfg->use_howl_detection)
	{
		gainnet_bf3->howl_energy=(float*)wtk_malloc(sizeof(float)*gainnet_bf3->nbin);
		gainnet_bf3->howl_scale=(float*)wtk_malloc(sizeof(float)*gainnet_bf3->nbin);
		gainnet_bf3->howl_power_x=(float*)wtk_malloc(sizeof(float)*gainnet_bf3->nbin);
		gainnet_bf3->howl_cnt=(int*)wtk_malloc(sizeof(int)*gainnet_bf3->nbin);
		gainnet_bf3->howl_xld=wtk_complex_new_p2(gainnet_bf3->nbin, cfg->LM+cfg->LD);
		gainnet_bf3->howl_W=wtk_complex_new_p2(gainnet_bf3->nbin, cfg->LM);
	}

	gainnet_bf3->fft=wtk_complex_new_p2(cfg->channel, gainnet_bf3->nbin);

	gainnet_bf3->fftx=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*gainnet_bf3->nbin);

	gainnet_bf3->gdenoise=(wtk_gainnet_bf3_denoise_t *)wtk_malloc(sizeof(wtk_gainnet_bf3_denoise_t));
	wtk_gainnet_bf3_denoise_init(gainnet_bf3->gdenoise, cfg);

	gainnet_bf3->covm=NULL;
	if(!cfg->use_fixtheta)
	{
		gainnet_bf3->covm=wtk_covm_new(&(cfg->covm), gainnet_bf3->nbin, cfg->channel);
	}
	gainnet_bf3->bf=wtk_bf_new(&(cfg->bf), gainnet_bf3->cfg->wins);
	
	gainnet_bf3->eq=NULL;
	if(cfg->use_eq)
	{
		gainnet_bf3->eq=wtk_equalizer_new(&(cfg->eq));
	}

	gainnet_bf3->maskssl=NULL;
	gainnet_bf3->maskssl2=NULL;
	if(cfg->use_maskssl)
	{
		gainnet_bf3->maskssl=wtk_maskssl_new(&(cfg->maskssl));
		wtk_maskssl_set_notify(gainnet_bf3->maskssl, gainnet_bf3, (wtk_maskssl_notify_f)wtk_gainnet_bf3_on_ssl2);
	}else if(cfg->use_maskssl2)
	{
		gainnet_bf3->maskssl2=wtk_maskssl2_new(&(cfg->maskssl2));
        wtk_maskssl2_set_notify(gainnet_bf3->maskssl2, gainnet_bf3, (wtk_maskssl2_notify_f)wtk_gainnet_bf3_on_ssl2);
	}

	gainnet_bf3->out=wtk_malloc(sizeof(float)*(gainnet_bf3->nbin-1));

	wtk_gainnet_bf3_reset(gainnet_bf3);

	return gainnet_bf3;
}

void wtk_gainnet_bf3_delete(wtk_gainnet_bf3_t *gainnet_bf3)
{
	wtk_strbufs_delete(gainnet_bf3->mic,gainnet_bf3->cfg->channel);

	wtk_free(gainnet_bf3->analysis_window);
	wtk_free(gainnet_bf3->synthesis_window);
	wtk_float_delete_p2(gainnet_bf3->analysis_mem, gainnet_bf3->cfg->channel);
	wtk_free(gainnet_bf3->synthesis_mem);
	wtk_free(gainnet_bf3->rfft_in);
	wtk_drft_delete(gainnet_bf3->rfft);
	wtk_complex_delete_p2(gainnet_bf3->fft, gainnet_bf3->cfg->channel);

	if(gainnet_bf3->howl_energy)
	{
		wtk_free(gainnet_bf3->howl_energy);
		wtk_free(gainnet_bf3->howl_scale);
		wtk_free(gainnet_bf3->howl_power_x);
		wtk_free(gainnet_bf3->howl_cnt);
		wtk_complex_delete_p2(gainnet_bf3->howl_xld, gainnet_bf3->nbin);
		wtk_complex_delete_p2(gainnet_bf3->howl_W, gainnet_bf3->nbin);
	}

	if(gainnet_bf3->covm)
	{
		wtk_covm_delete(gainnet_bf3->covm);
	}
	wtk_bf_delete(gainnet_bf3->bf);

	if(gainnet_bf3->eq)
	{
		wtk_equalizer_delete(gainnet_bf3->eq);
	}

	wtk_free(gainnet_bf3->fftx);

	wtk_gainnet_bf3_denoise_clean(gainnet_bf3->gdenoise);
	wtk_free(gainnet_bf3->gdenoise);

    if(gainnet_bf3->maskssl)
    {
        wtk_maskssl_delete(gainnet_bf3->maskssl);
    }
	if(gainnet_bf3->maskssl2)
    {
        wtk_maskssl2_delete(gainnet_bf3->maskssl2);
    }

	wtk_free(gainnet_bf3->out);

	wtk_free(gainnet_bf3);
}


void wtk_gainnet_bf3_start(wtk_gainnet_bf3_t *gainnet_bf3)
{
	wtk_bf_update_ovec(gainnet_bf3->bf,gainnet_bf3->cfg->theta,gainnet_bf3->cfg->phi);
	wtk_bf_init_w(gainnet_bf3->bf);
}

void wtk_gainnet_bf3_start_xv(wtk_gainnet_bf3_t *gainnet_bf3, char *xv_fn)
{
	wtk_bf_update_ovec(gainnet_bf3->bf,gainnet_bf3->cfg->theta,gainnet_bf3->cfg->phi);
	wtk_bf_init_w(gainnet_bf3->bf);

	wtk_gainnet_bf3_denoise_start(gainnet_bf3->gdenoise, xv_fn);
}



void wtk_gainnet_bf3_set_agcenable(wtk_gainnet_bf3_t *gainnet_bf3,int enable)
{
	gainnet_bf3->agc_enable=enable;
}

void wtk_gainnet_bf3_set_denoiseenable(wtk_gainnet_bf3_t *gainnet_bf3,int enable)
{
	gainnet_bf3->denoise_enable=enable;
	if(enable==0)
	{
		wtk_gainnet_bf3_set_denoisesuppress(gainnet_bf3, 0);
	}
}

void wtk_gainnet_bf3_set_denoisesuppress(wtk_gainnet_bf3_t *gainnet_bf3,float denoisesuppress)
{
	if(denoisesuppress<0.5)
	{
		gainnet_bf3->cfg->use_fftsbf=0;
	}else
	{
		gainnet_bf3->cfg->use_fftsbf=1;
	}
	// gainnet_bf3->cfg->bfmu=denoisesuppress;
	gainnet_bf3->cfg->gbias=1-denoisesuppress;
}

void wtk_gainnet_bf3_reset(wtk_gainnet_bf3_t *gainnet_bf3)
{
	int wins=gainnet_bf3->cfg->wins;
	int i;

	wtk_strbufs_reset(gainnet_bf3->mic,gainnet_bf3->cfg->channel);

	for (i=0;i<wins;++i)
	{
		gainnet_bf3->analysis_window[i] = sin((0.5+i)*PI/(wins));//sin(.5*PI*sin(.5*PI*(i+.5)/frame_size) * sin(.5*PI*(i+.5)/frame_size));
	}
	wtk_drft_init_synthesis_window(gainnet_bf3->synthesis_window, gainnet_bf3->analysis_window, wins);

	wtk_float_zero_p2(gainnet_bf3->analysis_mem, gainnet_bf3->cfg->channel, (gainnet_bf3->nbin-1));
	memset(gainnet_bf3->synthesis_mem, 0, sizeof(float)*(gainnet_bf3->nbin-1));

	wtk_complex_zero_p2(gainnet_bf3->fft, gainnet_bf3->cfg->channel, gainnet_bf3->nbin);

	if(gainnet_bf3->howl_energy)
	{
		memset(gainnet_bf3->howl_energy, 0, sizeof(float)*gainnet_bf3->nbin);
		memset(gainnet_bf3->howl_scale, 0, sizeof(float)*gainnet_bf3->nbin);

		wtk_complex_zero_p2(gainnet_bf3->howl_xld, gainnet_bf3->nbin, gainnet_bf3->cfg->LM+gainnet_bf3->cfg->LD);
		wtk_complex_zero_p2(gainnet_bf3->howl_W, gainnet_bf3->nbin, gainnet_bf3->cfg->LM);
		memset(gainnet_bf3->howl_power_x, 0, sizeof(float)*gainnet_bf3->nbin);
		memset(gainnet_bf3->howl_cnt, 0, sizeof(int)*gainnet_bf3->nbin);
	}

	if(gainnet_bf3->covm)
	{
		wtk_covm_reset(gainnet_bf3->covm);
	}

	wtk_bf_reset(gainnet_bf3->bf);

	memset(gainnet_bf3->fftx, 0, sizeof(wtk_complex_t)*(gainnet_bf3->nbin));

	wtk_gainnet_bf3_denoise_reset(gainnet_bf3->gdenoise);

	gainnet_bf3->mic_silcnt=0;
	gainnet_bf3->mic_sil=1;

	gainnet_bf3->pframe=0;

	if(gainnet_bf3->maskssl)
    {
        wtk_maskssl_reset(gainnet_bf3->maskssl);
    }
	if(gainnet_bf3->maskssl2)
    {
        wtk_maskssl2_reset(gainnet_bf3->maskssl2);
    }
	
	gainnet_bf3->agc_enable=1;
	gainnet_bf3->denoise_enable=1;

	gainnet_bf3->nframe=1;
}


void wtk_gainnet_bf3_set_notify(wtk_gainnet_bf3_t *gainnet_bf3,void *ths,wtk_gainnet_bf3_notify_f notify)
{
	gainnet_bf3->notify=notify;
	gainnet_bf3->ths=ths;
}

void wtk_gainnet_bf3_set_tr_notify(wtk_gainnet_bf3_t *gainnet_bf3,void *ths,wtk_gainnet_bf3_notify_trfeat_f notify)
{
	gainnet_bf3->notify_tr=notify;
	gainnet_bf3->ths_tr=ths;
}

void wtk_gainnet_bf3_set_ssl_notify(wtk_gainnet_bf3_t *gainnet_bf3,void *ths,wtk_gainnet_bf3_notify_ssl_f notify)
{
	gainnet_bf3->notify_ssl=notify;
	gainnet_bf3->ssl_ths=ths;
}

static float wtk_gainnet_bf3_fft_energy(wtk_complex_t *fftx,int nbin)
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

void wtk_gainnet_bf3_denoise_on_gainnet(wtk_gainnet_bf3_denoise_t *gdenoise, float *gain, int len, int is_end)
{
	memcpy(gdenoise->g, gain, sizeof(float)*gdenoise->cfg->bankfeat.nb_bands);
}


void wtk_gainnet_bf3_agc_on_gainnet(wtk_gainnet_bf3_denoise_t *gdenoise, float *gain, int len, int is_end)
{
	memcpy(gdenoise->g2, gain, sizeof(float)*gdenoise->cfg->bankfeat.nb_bands);
}

void wtk_gainnet_bf3_denoise_feed(wtk_gainnet_bf3_denoise_t *gdenoise, wtk_complex_t *fftx)
{
	int i;
	int nbin=gdenoise->nbin;
	float *g=gdenoise->g, *lastg=gdenoise->lastg, *gf=gdenoise->gf;
	float *g2=gdenoise->g2, *gf2=gdenoise->gf2, *lastg2=gdenoise->lastg2;;
	float gbias=gdenoise->cfg->gbias;
	float agc_a=gdenoise->cfg->agc_a;
	float agc_b=gdenoise->cfg->agc_b;
	float g_a=gdenoise->cfg->g_a;
	float g_b=gdenoise->cfg->g_b;
	float g2_min=gdenoise->cfg->g2_min;
	float g2_max=gdenoise->cfg->g2_max;
	float g_min=gdenoise->cfg->g_min;
	float g_max=gdenoise->cfg->g_max;
	float g_minthresh=gdenoise->cfg->g_minthresh;
	float ralpha=gdenoise->cfg->ralpha;
	wtk_bankfeat_t *bank_mic=gdenoise->bank_mic;
	int nb_bands=bank_mic->cfg->nb_bands;
	int nb_features=bank_mic->cfg->nb_features;
	wtk_qmmse_t *qmmse=gdenoise->qmmse;
	wtk_gainnet7_t *gainnet=gdenoise->gainnet;
	wtk_gainnet2_t *gainnet2=gdenoise->gainnet2;
	wtk_gainnet4_t *gainnet4=gdenoise->gainnet4;
	wtk_gainnet_t *agc_gainnet=gdenoise->agc_gainnet;
	float *qmmse_gain;
	int featm_lm=gdenoise->cfg->featm_lm;
	float *feature_lm=gdenoise->feature_lm;
	// static FILE *g_fn=NULL;
	// static FILE *feat_fn=NULL;

	// if(g_fn==NULL)
	// {
	// 	g_fn=fopen("test.bin", "rb");
	// }
	// if(feat_fn==NULL)
	// {
	// 	feat_fn=fopen("feat.bin", "rb");
	// }

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
		if(gdenoise->xvec)
		{
			if(gainnet4)
			{
				// fread(bank_mic->features, sizeof(float), nb_features, feat_fn);
				// fread(gdenoise->xvec, sizeof(float), gdenoise->cfg->xv_len, feat_fn);
				wtk_gainnet4_feed(gainnet4, bank_mic->features, nb_features, gdenoise->xvec, gdenoise->cfg->xv_len,0);   
			}
		}else
		{
			if(gainnet)
			{
				wtk_gainnet7_feed(gainnet, bank_mic->features, nb_features, 0);   
			}else if(gainnet2 && feature_lm)
			{
				wtk_gainnet2_feed2(gainnet2, bank_mic->features ,nb_features, feature_lm, nb_features*(featm_lm-1), 0);   
			}
		}
		if(gdenoise->cfg->use_gsigmoid)
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
		// print_float2(g, nb_bands);
		// fread(g, sizeof(float), nb_bands, g_fn);
		// print_float2(g, nb_bands);
		// printf("===============================\n");
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
				g2[i]=max(g2[i],lastg2[i]*ralpha);
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
					gf2[i]=gf2[i]/(gf[i]+1e-6f);
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
		if(gbias>0)
		{
			for (i=1; i<nbin-1; ++i)
			{
				gf[i]=min(gf[i]+gbias,1);
			}
		}
	}
	if(feature_lm && featm_lm>1)
	{
		memmove(feature_lm+nb_features,feature_lm,sizeof(float)*nb_features*(featm_lm-2));
		memcpy(feature_lm,bank_mic->features,sizeof(float)*nb_features);
	}

}


void wtk_gainnet_bf3_feed_denoise(wtk_gainnet_bf3_t *gainnet_bf3, wtk_complex_t **fft)
{
	wtk_gainnet_bf3_denoise_t *gdenoise=gainnet_bf3->gdenoise;
	float pframe_alpha=gainnet_bf3->cfg->pframe_alpha;
	int pfs=gainnet_bf3->cfg->pframe_fs;
	int fe_len=gainnet_bf3->cfg->pframe_fe-gainnet_bf3->cfg->pframe_fs;

	wtk_gainnet_bf3_denoise_feed(gdenoise, fft[0]);

	gainnet_bf3->pframe=(1-pframe_alpha)*gainnet_bf3->pframe+pframe_alpha*wtk_float_abs_mean(gainnet_bf3->gdenoise->gf+pfs, fe_len);

}

void wtk_gainnet_bf3_feed_bf(wtk_gainnet_bf3_t *gainnet_bf3, wtk_complex_t **fft)
{
	wtk_complex_t *fftx=gainnet_bf3->fftx;
	int k,nbin=gainnet_bf3->nbin;
	int i;
	int channel=gainnet_bf3->cfg->channel;
	wtk_bf_t *bf=gainnet_bf3->bf;
	wtk_covm_t *covm=gainnet_bf3->covm;
	int b;
	wtk_gainnet_bf3_denoise_t *gdenoise=gainnet_bf3->gdenoise;
	float gf;
	wtk_complex_t fft2[64];
	wtk_complex_t ffts[64];
	wtk_complex_t ffty[64];
	int clip_s=gainnet_bf3->cfg->clip_s;
	int clip_e=gainnet_bf3->cfg->clip_e;

	fftx[0].a=fftx[0].b=0;
	fftx[nbin-1].a=fftx[nbin-1].b=0;
	if(gainnet_bf3->cfg->use_fixtheta)
	{
		for(k=1; k<nbin-1; ++k)
		{
			gf=gdenoise->gf[k];
			for(i=0; i<channel; ++i)
			{
				ffts[i].a=fft[i][k].a*gf;
				ffts[i].b=fft[i][k].b*gf;
			}
			wtk_bf_output_fft_k(bf, ffts, fftx+k, k);
		}
	}else
	{
		for(k=clip_s+1; k<nbin; ++k)
		{
			gf=gdenoise->gf[k];
			for(i=0; i<channel; ++i)
			{
				ffts[i].a=fft[i][k].a*gf;
				ffts[i].b=fft[i][k].b*gf;

				ffty[i].a=fft[i][k].a*(1-gf);
				ffty[i].b=fft[i][k].b*(1-gf);
			}
			if(gainnet_bf3->cfg->use_fftsbf)
			{
				for(i=0; i<channel; ++i)
				{
					fft2[i]=ffts[i];
				}
			}else
			{
				for(i=0; i<channel; ++i)
				{
					fft2[i]=fft[i][k];
				}
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
			if(b==1)
			{
				wtk_bf_update_w(bf, k);
			}
			wtk_bf_output_fft_k(bf, fft2, fftx+k, k);
			// fft[k]=ffts[0];
		}
	}
	for(k=1; k<=clip_s; ++k)
	{
		fftx[k].a=fftx[k].b=0;
	}
	for(k=clip_e; k<nbin; ++k)
	{
		fftx[k].a=fftx[k].b=0;
	}
}

void wtk_gainnet_bf3_feed_agc2(wtk_gainnet_bf3_t *gainnet_bf3, wtk_complex_t * fft)
{
	wtk_gainnet_bf3_denoise_t *gdenoise=gainnet_bf3->gdenoise;
	int nbin=gdenoise->nbin;
	float *gf2=gdenoise->gf2;
	float gf;
	wtk_bankfeat_t *bankfeat=gdenoise->bank_mic;
	int nb_bands=bankfeat->cfg->nb_bands;
	float E = 0;
	float *Ex=bankfeat->Ex;
	int i;

	wtk_bankfeat_compute_band_energy(bankfeat, Ex, fft);
	for (i=0;i<nb_bands;++i)
	{
		E += Ex[i];
	}

	if (gainnet_bf3->agc_enable && !gdenoise->bank_mic->silence && E>gainnet_bf3->cfg->agce_thresh)
	{
		if(gainnet_bf3->cfg->use_agcmean)
		{
			gf=wtk_float_abs_mean(gf2+1, nbin-2);
			for (i=1;i<nbin-1;++i)
			{
				fft[i].a *= gf;
				fft[i].b *= gf;
			}
		}else
		{
			for (i=1;i<nbin-1;++i)
			{
				fft[i].a *= gf2[i];
				fft[i].b *= gf2[i];
			}
		}
	}
}


void wtk_gainnet_bf3_on_ssl2(wtk_gainnet_bf3_t *gainnet_bf3, wtk_ssl2_extp_t *nbest_extp,int nbest, int ts,int te)
{
    if(gainnet_bf3->notify_ssl)
    {
        gainnet_bf3->notify_ssl(gainnet_bf3->ssl_ths, nbest_extp, nbest);
    }
}


void wtk_gainnet_bf3_control_bs(wtk_gainnet_bf3_t *gainnet_bf3, float *out, int len)
{
	float out_max;
	static float scale=1.0;
	static float last_scale=1.0;
	static int max_cnt=0;
	int i;

	if(gainnet_bf3->mic_sil==0)
	{
		out_max=wtk_float_abs_max(out, len);
		if(out_max>32700.0)
		{
			scale=32700.0/out_max;
			if(scale<last_scale)
			{
				last_scale=scale;
			}else
			{
				scale=last_scale;
			}
			max_cnt=10;
		}
		for(i=0; i<len; ++i)
		{
			out[i]*=scale;
		}
		if(max_cnt>0)
		{
			--max_cnt;
		}
		if(max_cnt<=0 && scale<1.0)
		{
			scale*=1.1;
			last_scale=scale;
			if(scale>1.0)
			{
				scale=1.0;
				last_scale=1.0;
			}
		}
	}else
	{
		scale=1.0;
		last_scale=1.0;
		max_cnt=0;
	}
} 

void wtk_gainnet_bf3_feed(wtk_gainnet_bf3_t *gainnet_bf3,short *data,int len,int is_end)
{
	int i,j;
	int nbin=gainnet_bf3->nbin;
	int channel=gainnet_bf3->cfg->channel;
	wtk_strbuf_t **mic=gainnet_bf3->mic;
	float fv, *fp1;
	int wins=gainnet_bf3->cfg->wins;
	int fsize=wins/2;
	int length;
	float micenr;
	float micenr_thresh=gainnet_bf3->cfg->micenr_thresh;
	int micenr_cnt=gainnet_bf3->cfg->micenr_cnt;
	wtk_drft_t *rfft=gainnet_bf3->rfft;
	float *rfft_in=gainnet_bf3->rfft_in;
	wtk_complex_t **fft=gainnet_bf3->fft;
	float **analysis_mem=gainnet_bf3->analysis_mem,	*synthesis_mem=gainnet_bf3->synthesis_mem;
	float *analysis_window=gainnet_bf3->analysis_window, *synthesis_window=gainnet_bf3->synthesis_window;
	wtk_complex_t *fftx=gainnet_bf3->fftx;
	int save_s=gainnet_bf3->cfg->save_s;
	float *out=gainnet_bf3->out;
	short *pv=(short *)out;

	for(i=0;i<len;++i)
	{
		for(j=0; j<channel; ++j)
		{
			fv=data[j];
			wtk_strbuf_push(mic[j],(char *)&(fv),sizeof(float));
		}
		data+=channel;
	}
	length=mic[0]->pos/sizeof(float);
	while(length>=fsize)
	{
		for(i=0; i<channel; ++i)
		{
			fp1=(float *)mic[i]->data;
			wtk_drft_frame_analysis(rfft, rfft_in, analysis_mem[i], fft[i], fp1, wins, analysis_window);
		}
		wtk_gainnet_bf3_feed_denoise(gainnet_bf3, fft);
		wtk_gainnet_bf3_feed_bf(gainnet_bf3, fft);

		if(gainnet_bf3->gdenoise->agc_gainnet)
		{
            wtk_gainnet_bf3_feed_agc2(gainnet_bf3, fftx);
		}

		// static int cnt=0;
		// cnt++;
		micenr=wtk_gainnet_bf3_fft_energy(fftx, nbin);
		if(micenr>micenr_thresh)
		{
			// if(gainnet_bf3->mic_sil==1)
			// {
			// 	printf("sp start %f %f %f\n", 1.0/16000*cnt*(nbin-1),micenr,micenr_thresh);
			// }
			gainnet_bf3->mic_sil=0;
			gainnet_bf3->mic_silcnt=micenr_cnt;
		}else if(gainnet_bf3->mic_sil==0)
		{
			gainnet_bf3->mic_silcnt-=1;
			if(gainnet_bf3->mic_silcnt<=0)
			{
				// printf("sp end %f\n", 1.0/16000*cnt*(nbin-1));
				gainnet_bf3->mic_sil=1;
			}
		}

		if(gainnet_bf3->maskssl2)
		{
			wtk_maskssl2_feed_fft2(gainnet_bf3->maskssl2, fft, gainnet_bf3->gdenoise->gf, gainnet_bf3->mic_sil);
		}else if(gainnet_bf3->maskssl)
		{
			wtk_maskssl_feed_fft2(gainnet_bf3->maskssl, fft, gainnet_bf3->gdenoise->gf, gainnet_bf3->mic_sil);
		}

		wtk_strbufs_pop(mic, channel, fsize*sizeof(float));
		length=mic[0]->pos/sizeof(float);

		for(i=save_s;i<nbin-1;++i)
		{
			fftx[i]=fft[0][i];
		}
	    wtk_drft_frame_synthesis(rfft, rfft_in, synthesis_mem, fftx, out, wins, synthesis_window);
		if(gainnet_bf3->eq)
		{
			wtk_equalizer_feed_float(gainnet_bf3->eq, out, fsize);
		}
		wtk_gainnet_bf3_control_bs(gainnet_bf3, out, fsize);
		for(i=0; i<fsize; ++i)
		{
			pv[i]=floorf(out[i]+0.5);
		}
		if(gainnet_bf3->notify)
		{
			gainnet_bf3->notify(gainnet_bf3->ths,pv,fsize);
		}
	}
	if(0 && is_end && length>0)
	{
		if(gainnet_bf3->notify)
		{
			pv=(short *)mic[0]->data;
			out=(float *)mic[0]->data;
			for(i=0; i<length; ++i)
			{
				pv[i]=floorf(out[i]+0.5);
			}
			gainnet_bf3->notify(gainnet_bf3->ths,pv,length);
		}
	}
}

void wtk_gainnet_bf3_feed_train2(wtk_gainnet_bf3_t *gainnet_bf3,short **data,int len,int channel, int bb)
{
	wtk_gainnet_bf3_denoise_t *gdenoise=gainnet_bf3->gdenoise;
	short *mdata, *sdata, *ndata, *ndata2, *sdatar, *sdataagc, *dataagc;
	int fsize=gdenoise->cfg->wins/2;
	int nbin=gdenoise->nbin;
	int i;
	float *xn;
	float *x;
	float *Es;
	float *xr;
	float *Er;
	float *xagc;
	float *Eagc;
	float *agc;
	float *g;
	float *ns;
	wtk_complex_t *ffts;
	wtk_complex_t *fftr;
	wtk_complex_t *fftagc;
	wtk_drft_t *rfft=gainnet_bf3->rfft;
	wtk_bankfeat_t *bank_mic=gdenoise->bank_mic;
	int nb_bands=gdenoise->cfg->bankfeat.nb_bands;
	int nb_features=gdenoise->cfg->bankfeat.nb_features;
	float *Ex=bank_mic->Ex;
	int wins=gdenoise->cfg->wins;
	float *analysis_mem_tr;
	float *analysis_mem_tr2;
	float *analysis_mem_tr3;
	int pos;
	//   short *pv;
	//   wtk_strbuf_t *outbuf=wtk_strbuf_new(1024,2);
	float *rfft_in=gainnet_bf3->rfft_in;
	wtk_complex_t *fft=gainnet_bf3->fft[0];
	float *analysis_mem=gainnet_bf3->analysis_mem[0];
	float data_max, fscale;
	// time_t t;
	// srand((unsigned) time(&t));

	xn=wtk_malloc(sizeof(float)*fsize);
	x=wtk_malloc(sizeof(float)*fsize);
	xr=wtk_malloc(sizeof(float)*fsize);
	xagc=wtk_malloc(sizeof(float)*fsize);

	Es=wtk_malloc(sizeof(float)*nb_bands);
	Er=wtk_malloc(sizeof(float)*nb_bands);
	Eagc=wtk_malloc(sizeof(float)*nb_bands);

	ns=wtk_malloc(sizeof(float)*nb_bands);
	g=wtk_malloc(sizeof(float)*nb_bands);
	agc=wtk_malloc(sizeof(float)*nb_bands);

	ffts=wtk_malloc(sizeof(wtk_complex_t)*nbin);
	fftr=wtk_malloc(sizeof(wtk_complex_t)*nbin);
	fftagc=wtk_malloc(sizeof(wtk_complex_t)*nbin);

	analysis_mem_tr=wtk_calloc(sizeof(float),fsize);
	analysis_mem_tr2=wtk_calloc(sizeof(float),fsize);
	analysis_mem_tr3=wtk_calloc(sizeof(float),fsize);

	mdata=wtk_malloc(sizeof(short)*len);
	dataagc=wtk_malloc(sizeof(short)*len);

	data_max=wtk_short_abs_max(data[1], len);
	fscale=19660.0/data_max;
	// fscale=22937.0/data_max;
	for(i=0; i<len; ++i)
	{
		dataagc[i]=floorf(data[1][i]*fscale+0.5);
	}
	if(bb==2)
	{
		memset(data[1], 0, sizeof(short)*len);
		memset(data[0], 0, sizeof(short)*len);
		memset(data[3], 0, sizeof(short)*len);
		memset(dataagc, 0, sizeof(short)*len);
	}
	if(bb==3)
	{
		memset(data[0], 0, sizeof(short)*len);
		memset(data[3], 0, sizeof(short)*len);
		memset(dataagc, 0, sizeof(short)*len);
	}
	if(bb==4)
	{
		memset(data[1], 0, sizeof(short)*len);
		memset(data[2], 0, sizeof(short)*len);
	}
	if(bb==5)
	{
		memset(data[2], 0, sizeof(short)*len);
	}
	if(bb==6)
	{
		memset(data[1], 0, sizeof(short)*len);
	}
	if(bb==7)
	{
		for(i=0;i<len;++i)
		{
			mdata[i]=data[0][i]+data[1][i]+data[2][i];
		}
		data_max=wtk_short_abs_max(mdata, len);
		fscale=rand()/(RAND_MAX+1.0)*0.8+0.1;
		fscale=(fscale*32767.0)/data_max;
		// printf("%f\n",fscale);
		for(i=0; i<len; ++i)
		{
			data[0][i]=floorf(data[0][i]*fscale+0.5);
			data[1][i]=floorf(data[1][i]*fscale+0.5);
			data[2][i]=floorf(data[2][i]*fscale+0.5);
			data[3][i]=floorf(data[3][i]*fscale+0.5);
		}
	}

	pos=0;
	while((len-pos)>=fsize)
	{
		sdata=data[0]+pos;
		ndata=data[1]+pos;
		ndata2=data[2]+pos;
		sdatar=data[3]+pos;
		sdataagc=dataagc+pos;
		for(i=0;i<fsize;++i)
		{
			xn[i]=sdata[i]+ndata[i]+ndata2[i];
			x[i]=sdata[i];
			xr[i]=sdatar[i];
			xagc[i]=sdataagc[i];
		}
		wtk_drft_frame_analysis(rfft, rfft_in, analysis_mem_tr, ffts, x, wins, gainnet_bf3->analysis_window);
		ffts[0].a=ffts[0].b=ffts[nbin-1].a=ffts[nbin-1].b=0;
		wtk_bankfeat_compute_band_energy(bank_mic, Es, ffts);

		wtk_drft_frame_analysis(rfft, rfft_in, analysis_mem_tr2, fftr, xr, wins, gainnet_bf3->analysis_window);
		fftr[0].a=fftr[0].b=fftr[nbin-1].a=fftr[nbin-1].b=0;
		wtk_bankfeat_compute_band_energy(bank_mic, Er, fftr);

		wtk_drft_frame_analysis(rfft, rfft_in, analysis_mem_tr3, fftagc, xagc, wins, gainnet_bf3->analysis_window);
		fftagc[0].a=fftagc[0].b=fftagc[nbin-1].a=fftagc[nbin-1].b=0;
		wtk_bankfeat_compute_band_energy(bank_mic, Eagc, fftagc);

		wtk_drft_frame_analysis(rfft, rfft_in, analysis_mem, fft, xn, wins, gainnet_bf3->analysis_window);
		fft[0].a=fft[0].b=fft[nbin-1].a=fft[nbin-1].b=0;
		wtk_bankfeat_flush_frame_features(bank_mic, fft);

		for (i=0;i<nb_bands;i++)
		{
			if(bank_mic->silence || Ex[i] < 5e-2 || Er[i] < 5e-2 || Es[i] < 5e-2)
			{
				g[i]=0;
				ns[i]=0;
				agc[i]=0;
			}else
			{
				// g[i] = sqrt((Er[i])/(Ex[i]));
				// ns[i] = sqrt((Es[i])/(Ex[i]));
				// if (g[i] > 1) g[i] = 1;
				// if (ns[i] > 1) ns[i] = 1;
				// agc[i] = sqrt((Eagc[i])/(Ex[i]));

				
				// if(Ex[i]<1e-2)
				// {
				// 	g[i]=-1;
				// 	ns[i]=-1;
				// 	agc[i]=-1;
				// }
				// if(Er[i]<1e-2)
				// {
				// 	agc[i]=-1;
				// }

				ns[i] = Es[i];
				g[i] = Er[i];
				agc[i] = Eagc[i];

			}
			// if (g[i] > 1) g[i] = 1;
			// if (ns[i] > 1) ns[i] = 1;
			// if (agc[i] > 35) agc[i] = 35;
		}

		// {
		// 	wtk_bankfeat_interp_band_gain(bank_mic, gdenoise->nbin, gdenoise->gf, ns);
		// 	// wtk_bankfeat_interp_band_gain(bank_mic, gdenoise->nbin, gdenoise->gf2, agc);
		// 	for (i=0;i<gdenoise->nbin;i++)
		// 	{
		// 		fft[i].a *= gdenoise->gf[i];//* gdenoise->gf2[i];
		// 		fft[i].b *= gdenoise->gf[i];//* gdenoise->gf2[i];
		// 	}
		// 	wtk_drft_frame_synthesis(rfft, rfft_in, gainnet_bf3->synthesis_mem, fft, gainnet_bf3->out, wins, gainnet_bf3->synthesis_window);
		// 	pv=(short *)gainnet_bf3->out;
		// 	for(i=0;i<fsize;++i)
		// 	{
		// 	pv[i]=gainnet_bf3->out[i];
		// 	}
		// 	wtk_strbuf_push(outbuf, (char *)pv, sizeof(short)*fsize);
		// }

		if(gainnet_bf3->notify_tr)
		{
			gainnet_bf3->notify_tr(gainnet_bf3->ths_tr, bank_mic->features, nb_features, Ex, g, ns, nb_bands);
		}
		pos+=fsize;
	}
	// {
	// 	wtk_wavfile_t *wav;
	// 	wav=wtk_wavfile_new(gainnet_bf3->cfg->rate);
	// 	wav->max_pend=0;
	// 	wtk_wavfile_open(wav,"o.wav");
	// 	wtk_wavfile_write(wav,(char *)outbuf->data,outbuf->pos);
	// 	wtk_wavfile_delete(wav);
	// }

	wtk_free(xn);
	wtk_free(x);
	wtk_free(xr);
	wtk_free(xagc);

	wtk_free(Es);
	wtk_free(Er);
	wtk_free(Eagc);

	wtk_free(ns);
	wtk_free(g);
	wtk_free(agc);

	wtk_free(ffts);
	wtk_free(fftr);
	wtk_free(fftagc);

	wtk_free(analysis_mem_tr);
	wtk_free(analysis_mem_tr2);
	wtk_free(analysis_mem_tr3);

	wtk_free(dataagc);
	wtk_free(mdata);
}

void wtk_gainnet_bf3_feed_train(wtk_gainnet_bf3_t *gainnet_bf3,short **data,int len,int channel, int bb)
{
	wtk_gainnet_bf3_denoise_t *gdenoise=gainnet_bf3->gdenoise;
	short *mdata, *sdata, *ndata, *sdatar, *sdataagc, *dataagc;
	int fsize=gdenoise->cfg->wins/2;
	int nbin=gdenoise->nbin;
	int i;
	float *xn;
	float *x;
	float *Es;
	float *xr;
	float *Er;
	float *xagc;
	float *Eagc;
	float *agc;
	float *g;
	float *ns;
	wtk_complex_t *ffts;
	wtk_complex_t *fftr;
	wtk_complex_t *fftagc;
	wtk_drft_t *rfft=gainnet_bf3->rfft;
	wtk_bankfeat_t *bank_mic=gdenoise->bank_mic;
	int nb_bands=gdenoise->cfg->bankfeat.nb_bands;
	int nb_features=gdenoise->cfg->bankfeat.nb_features;
	float *Ex=bank_mic->Ex;
	int wins=gdenoise->cfg->wins;
	float *analysis_mem_tr;
	float *analysis_mem_tr2;
	float *analysis_mem_tr3;
	int pos;
	//   short *pv;
	//   wtk_strbuf_t *outbuf=wtk_strbuf_new(1024,2);
	float *rfft_in=gainnet_bf3->rfft_in;
	wtk_complex_t *fft=gainnet_bf3->fft[0];
	float *analysis_mem=gainnet_bf3->analysis_mem[0];
	float data_max, fscale;
	// time_t t;
	// srand((unsigned) time(&t));

	xn=wtk_malloc(sizeof(float)*fsize);
	x=wtk_malloc(sizeof(float)*fsize);
	xr=wtk_malloc(sizeof(float)*fsize);
	xagc=wtk_malloc(sizeof(float)*fsize);

	Es=wtk_malloc(sizeof(float)*nb_bands);
	Er=wtk_malloc(sizeof(float)*nb_bands);
	Eagc=wtk_malloc(sizeof(float)*nb_bands);

	ns=wtk_malloc(sizeof(float)*nb_bands);
	g=wtk_malloc(sizeof(float)*nb_bands);
	agc=wtk_malloc(sizeof(float)*nb_bands);

	ffts=wtk_malloc(sizeof(wtk_complex_t)*nbin);
	fftr=wtk_malloc(sizeof(wtk_complex_t)*nbin);
	fftagc=wtk_malloc(sizeof(wtk_complex_t)*nbin);

	analysis_mem_tr=wtk_calloc(sizeof(float),fsize);
	analysis_mem_tr2=wtk_calloc(sizeof(float),fsize);
	analysis_mem_tr3=wtk_calloc(sizeof(float),fsize);

	mdata=wtk_malloc(sizeof(short)*len);
	dataagc=wtk_malloc(sizeof(short)*len);

	data_max=wtk_short_abs_max(data[1], len);
	fscale=19660.0/data_max;
	// fscale=22937.0/data_max;
	for(i=0; i<len; ++i)
	{
		dataagc[i]=floorf(data[1][i]*fscale+0.5);
	}
	if(bb==2)
	{
		memset(data[1], 0, sizeof(short)*len);
		memset(data[0], 0, sizeof(short)*len);
		memset(data[3], 0, sizeof(short)*len);
		memset(dataagc, 0, sizeof(short)*len);
	}
	if(bb==3)
	{
		memset(data[0], 0, sizeof(short)*len);
		memset(data[3], 0, sizeof(short)*len);
		memset(dataagc, 0, sizeof(short)*len);
	}
	// if(bb==4)
	// {
	// 	memset(data[1], 0, sizeof(short)*len);
	// }
	if(bb==4)
	{
		memset(data[2], 0, sizeof(short)*len);
	}
	if(bb==5)
	{
		for(i=0;i<len;++i)
		{
			mdata[i]=data[0][i]+data[1][i]+data[2][i];
		}
		data_max=wtk_short_abs_max(mdata, len);
		fscale=rand()/(RAND_MAX+1.0)*0.8+0.1;
		fscale=(fscale*32767.0)/data_max;
		// printf("%f\n",fscale);
		for(i=0; i<len; ++i)
		{
			data[0][i]=floorf(data[0][i]*fscale+0.5);
			data[1][i]=floorf(data[1][i]*fscale+0.5);
			data[2][i]=floorf(data[2][i]*fscale+0.5);
			data[3][i]=floorf(data[3][i]*fscale+0.5);
		}
	}

	pos=0;
	while((len-pos)>=fsize)
	{
		sdata=data[1]+pos;
		ndata=data[0]+pos;
		sdatar=data[1]+pos;
		sdataagc=dataagc+pos;
		for(i=0;i<fsize;++i)
		{
			xn[i]=ndata[i];//sdata[i]+ndata[i]+ndata2[i];
			x[i]=sdata[i];
			xr[i]=sdatar[i];
			xagc[i]=sdataagc[i];
		}
		wtk_drft_frame_analysis(rfft, rfft_in, analysis_mem_tr, ffts, x, wins, gainnet_bf3->analysis_window);
		ffts[0].a=ffts[0].b=ffts[nbin-1].a=ffts[nbin-1].b=0;
		wtk_bankfeat_compute_band_energy(bank_mic, Es, ffts);

		wtk_drft_frame_analysis(rfft, rfft_in, analysis_mem_tr2, fftr, xr, wins, gainnet_bf3->analysis_window);
		fftr[0].a=fftr[0].b=fftr[nbin-1].a=fftr[nbin-1].b=0;
		wtk_bankfeat_compute_band_energy(bank_mic, Er, fftr);

		wtk_drft_frame_analysis(rfft, rfft_in, analysis_mem_tr3, fftagc, xagc, wins, gainnet_bf3->analysis_window);
		fftagc[0].a=fftagc[0].b=fftagc[nbin-1].a=fftagc[nbin-1].b=0;
		wtk_bankfeat_compute_band_energy(bank_mic, Eagc, fftagc);

		wtk_drft_frame_analysis(rfft, rfft_in, analysis_mem, fft, xn, wins, gainnet_bf3->analysis_window);
		fft[0].a=fft[0].b=fft[nbin-1].a=fft[nbin-1].b=0;
		wtk_bankfeat_flush_frame_features(bank_mic, fft);

		for (i=0;i<nb_bands;i++)
		{
			if(bank_mic->silence || Ex[i] < 5e-2 || Er[i] < 5e-2 || Es[i] < 5e-2)
			{
				g[i]=0;
				ns[i]=0;
				agc[i]=0;
			}else
			{
				g[i] = sqrt((Er[i])/(Ex[i]));
				ns[i] = sqrt((Es[i])/(Ex[i]));
				if (g[i] > 1) g[i] = 1;
				if (ns[i] > 1) ns[i] = 1;
				agc[i] = sqrt((Eagc[i])/(Ex[i]));
				// if(Ex[i]<1e-2)
				// {
				// 	g[i]=-1;
				// 	ns[i]=-1;
				// 	agc[i]=-1;
				// }
				// if(Er[i]<1e-2)
				// {
				// 	agc[i]=-1;
				// }
			}
			// if (g[i] > 1) g[i] = 1;
			// if (ns[i] > 1) ns[i] = 1;
			// if (agc[i] > 35) agc[i] = 35;
		}

		// {
		// 	wtk_bankfeat_interp_band_gain(bank_mic, gdenoise->nbin, gdenoise->gf, ns);
		// 	// wtk_bankfeat_interp_band_gain(bank_mic, gdenoise->nbin, gdenoise->gf2, agc);
		// 	for (i=0;i<gdenoise->nbin;i++)
		// 	{
		// 		fft[i].a *= gdenoise->gf[i];//* gdenoise->gf2[i];
		// 		fft[i].b *= gdenoise->gf[i];//* gdenoise->gf2[i];
		// 	}
		// 	wtk_drft_frame_synthesis(rfft, rfft_in, gainnet_bf3->synthesis_mem, fft, gainnet_bf3->out, wins, gainnet_bf3->synthesis_window);
		// 	pv=(short *)gainnet_bf3->out;
		// 	for(i=0;i<fsize;++i)
		// 	{
		// 	pv[i]=gainnet_bf3->out[i];
		// 	}
		// 	wtk_strbuf_push(outbuf, (char *)pv, sizeof(short)*fsize);
		// }

		if(gainnet_bf3->notify_tr)
		{
			gainnet_bf3->notify_tr(gainnet_bf3->ths_tr, bank_mic->features, nb_features, agc, g, ns, nb_bands);
		}
		pos+=fsize;
	}
	// {
	// 	wtk_wavfile_t *wav;
	// 	wav=wtk_wavfile_new(gainnet_bf3->cfg->rate);
	// 	wav->max_pend=0;
	// 	wtk_wavfile_open(wav,"o.wav");
	// 	wtk_wavfile_write(wav,(char *)outbuf->data,outbuf->pos);
	// 	wtk_wavfile_delete(wav);
	// }

	wtk_free(xn);
	wtk_free(x);
	wtk_free(xr);
	wtk_free(xagc);

	wtk_free(Es);
	wtk_free(Er);
	wtk_free(Eagc);

	wtk_free(ns);
	wtk_free(g);
	wtk_free(agc);

	wtk_free(ffts);
	wtk_free(fftr);
	wtk_free(fftagc);

	wtk_free(analysis_mem_tr);
	wtk_free(analysis_mem_tr2);
	wtk_free(analysis_mem_tr3);

	wtk_free(dataagc);
	wtk_free(mdata);
}



void wtk_gainnet_bf3_ptpr(wtk_gainnet_bf3_t *gainnet_bf3, wtk_complex_t *fft, int *idx, float thresh)
{
	float power=0;
	int i;
	int nbin=gainnet_bf3->nbin;

	for(i=0;i<nbin;++i){
		power = 10*log10f(fft[i].a * fft[i].a + fft[i].b * fft[i].b);
		if(gainnet_bf3->nframe==1){
			gainnet_bf3->howl_energy[i] = power;
		}else if(gainnet_bf3->nframe<200){
			gainnet_bf3->howl_energy[i] = gainnet_bf3->howl_energy[i] * 0.1 + power * 0.9;
		}else{
			gainnet_bf3->howl_energy[i] = gainnet_bf3->howl_energy[i] * 0.9 + power * 0.1;
		}
		if(gainnet_bf3->howl_energy[i] < power){
			gainnet_bf3->howl_scale[i] = gainnet_bf3->howl_energy[i] / (power + 1e-6)*0.9 + 0.1;
		}else{
			gainnet_bf3->howl_scale[i] = 1.0;
		}
		// printf("%f ", power);
		if(power > thresh){
			idx[i] = 1;
		}
	}
	// printf("\n");
}

void wtk_gainnet_bf3_papr(wtk_gainnet_bf3_t *gainnet_bf3, wtk_complex_t *fft, int *idx, float thresh)
{
	float power[513]={0};
	float sum_power=0;
	float mean_power=0;
	int i;
	int nbin=gainnet_bf3->nbin;

	for(i=0;i<nbin;++i)
	{
		power[i] = fft[i].a * fft[i].a + fft[i].b * fft[i].b;
		sum_power += power[i];
	}
	mean_power = sum_power*1.0/nbin;
	for(i=0;i<nbin;++i)
	{
		power[i] = 10*log10f(power[i]/mean_power);
		if(power[i] > thresh)
		{
			idx[i] = 1;
		}
	}
}

void wtk_gainnet_bf3_pnpr(wtk_gainnet_bf3_t *gainnet_bf3, wtk_complex_t *fft, int *idx, float thresh)
{
	float power[513]={0};
	int i, r;
	int nbin=gainnet_bf3->nbin;
	int range[6]={2,3,4,5,6,7};
	float p1, p2, p3, p4;

	for(i=0;i<nbin;++i)
	{
		power[i] = fft[i].a * fft[i].a + fft[i].b * fft[i].b;
	}
	// for(i=0;i<range;++i){
	// 	p1 = 20*log10f(power[i]/power[i+(range-1)]);
	// 	p2 = 20*log10f(power[i]/power[i+range]);
	// 	if(p1 > thresh && p2 > thresh){
	// 		idx[i] = 1;
	// 	}
	// }
	for(r=0;r<6;++r)
	{
		for(i=range[r];i<nbin-range[r];++i)
		{
			p1 = 20*log10f(power[i]/power[i-(range[r]-1)]);
			p2 = 20*log10f(power[i]/power[i-range[r]]);
			p3 = 20*log10f(power[i]/power[i+(range[r]-1)]);
			p4 = 20*log10f(power[i]/power[i+range[r]]);
			if(p1 > thresh && p2 > thresh && p3 > thresh && p4 > thresh)
			{
				idx[i] = 1;
			}
		}
		for(i=nbin-range[r];i<nbin;++i)
		{
			p1 = 20*log10f(power[i]/power[i-(range[r]-1)]);
			p2 = 20*log10f(power[i]/power[i-range[r]]);
			if(p1 > thresh && p2 > thresh)
			{
				idx[i] = 1;
			}
		}
	}
}


void wtk_gainnet_bf3_feed_howlnlms(wtk_gainnet_bf3_t *gainnet_bf3, wtk_complex_t **fft, int k)
{
	wtk_complex_t *X=gainnet_bf3->howl_xld[k];
	wtk_complex_t *W=gainnet_bf3->howl_W[k];
	wtk_complex_t  *Xtmp, *wtmp, *fft1;
	float ya,yb,ea,eb,ffta,fftb;
	float *power_x=gainnet_bf3->howl_power_x;
    int LM=gainnet_bf3->cfg->LM;
	int LD=gainnet_bf3->cfg->LD;
    int i;
	float r;
	float mufb=gainnet_bf3->cfg->howllms_mufb;
	int channel=gainnet_bf3->cfg->channel;

	ffta=fft[0][k].a;
	fftb=fft[0][k].b;
	ya=yb=0;
	Xtmp=X+LD;
	for(i=0, wtmp=W; i<LM; ++i, ++wtmp)
	{
		ya+=Xtmp->a*wtmp->a-Xtmp->b*wtmp->b;
		yb+=Xtmp->a*wtmp->b+Xtmp->b*wtmp->a;
	}
	ea=ffta-ya;
	eb=fftb-yb;

	wtmp=W;
	Xtmp=X+LD;
	for(i=0; i<LM; ++i)
	{	
		r=mufb/(LM*power_x[k]+1e-9);
		wtmp->a+=r*(Xtmp->a*ea+Xtmp->b*eb);
		wtmp->b+=r*(-Xtmp->b*ea+Xtmp->a*eb);
		++wtmp; ++Xtmp;
	}

	ya=yb=0;
	Xtmp=X+LD;
	for(i=0, wtmp=W; i<LM; ++i, ++wtmp)
	{
		ya+=Xtmp->a*wtmp->a-Xtmp->b*wtmp->b;
		yb+=Xtmp->a*wtmp->b+Xtmp->b*wtmp->a;
	}
	for(i=0;i<channel;++i)
	{
		fft1=fft[i]+k;
		fft1->a-=ya;
		fft1->b-=yb;
		// fft1->b=fft1->a=0;
	}
}

void wtk_gainnet_bf3_howl_detect(wtk_gainnet_bf3_t *gainnet_bf3, wtk_complex_t **fft)
{
	int ptpr[513]={0};
	int papr[513]={0};
	int pnpr[513]={0};
	float ptpr_thresh=gainnet_bf3->cfg->ptpr_thresh;
	float papr_thresh=gainnet_bf3->cfg->papr_thresh;
	float pnpr_thresh=gainnet_bf3->cfg->pnpr_thresh;
	int k,k2;
	int howl_lf=gainnet_bf3->cfg->howl_lf;
	float ff;
	int nbin=gainnet_bf3->nbin;
	wtk_complex_t *fft1;
	float palpha=gainnet_bf3->cfg->howllms_palpha;
	float *howl_power_x=gainnet_bf3->howl_power_x;
	wtk_complex_t **howl_xld=gainnet_bf3->howl_xld, *howl_xld_tmp;
	int LD=gainnet_bf3->cfg->LD;
	int LM=gainnet_bf3->cfg->LM;
	int *howl_cnt=gainnet_bf3->howl_cnt;

	fft1=fft[0];
	for(k=0; k<nbin; ++k,++fft1)
	{
		howl_power_x[k]=(1-palpha)*howl_power_x[k]+palpha*(fft1->a*fft1->a+fft1->b*fft1->b);
	}
	fft1=fft[0];
    for(k=0; k<nbin; ++k)
    {
		howl_xld_tmp=howl_xld[k];
		memmove(howl_xld_tmp+1, howl_xld_tmp, sizeof(wtk_complex_t)*(LD+LM-1));
		howl_xld_tmp[0]=fft1[k];
    }
	wtk_gainnet_bf3_ptpr(gainnet_bf3, fft[0], ptpr, ptpr_thresh);
	wtk_gainnet_bf3_papr(gainnet_bf3, fft[0], papr, papr_thresh);
	wtk_gainnet_bf3_pnpr(gainnet_bf3, fft[0], pnpr, pnpr_thresh);

	for(k=1;k<nbin-1;++k)
	{
		if(ptpr[k] && papr[k] && pnpr[k])
		{
			howl_cnt[k]=gainnet_bf3->cfg->howl_cnt;
            for(k2=max(1,k-howl_lf);k2<min(nbin-1,k+howl_lf+1);++k2)
			{
				ff=(howl_lf-abs(k2-k)+1)*1.0/(howl_lf+1);
				howl_cnt[k2]=max(howl_cnt[k2], howl_cnt[k]*ff);
			}
		}
	}
	for(k=1;k<nbin-1;++k)
	{
		if(howl_cnt[k]>0)
		{
			wtk_gainnet_bf3_feed_howlnlms(gainnet_bf3, fft, k);
			--howl_cnt[k];
			// if(howl_cnt[k]==0)
			// {
			// 	memset(gainnet_bf3->howl_W[k],0,sizeof(wtk_complex_t)*LM);
			// }
		}
	}
}


void wtk_gainnet_bf3_feed_howl(wtk_gainnet_bf3_t *gainnet_bf3,short *data,int len,int is_end)
{
	int i,j;
	int nbin=gainnet_bf3->nbin;
	int channel=gainnet_bf3->cfg->channel;
	wtk_strbuf_t **mic=gainnet_bf3->mic;
	float fv, *fp1;
	int wins=gainnet_bf3->cfg->wins;
	int fsize=wins/2;
	int length;
	float micenr;
	float micenr_thresh=gainnet_bf3->cfg->micenr_thresh;
	int micenr_cnt=gainnet_bf3->cfg->micenr_cnt;
	wtk_drft_t *rfft=gainnet_bf3->rfft;
	float *rfft_in=gainnet_bf3->rfft_in;
	wtk_complex_t **fft=gainnet_bf3->fft;
	float **analysis_mem=gainnet_bf3->analysis_mem,	*synthesis_mem=gainnet_bf3->synthesis_mem;
	float *analysis_window=gainnet_bf3->analysis_window, *synthesis_window=gainnet_bf3->synthesis_window;
	wtk_complex_t *fftx=gainnet_bf3->fftx;
	int save_s=gainnet_bf3->cfg->save_s;
	float *out=gainnet_bf3->out;
	short *pv=(short *)out;

	for(i=0;i<len;++i)
	{
		for(j=0; j<channel; ++j)
		{
			fv=data[j];
			wtk_strbuf_push(mic[j],(char *)&(fv),sizeof(float));
		}
		data+=channel;
	}
	length=mic[0]->pos/sizeof(float);
	while(length>=fsize)
	{
		++gainnet_bf3->nframe;
		for(i=0; i<channel; ++i)
		{
			fp1=(float *)mic[i]->data;
			wtk_drft_frame_analysis(rfft, rfft_in, analysis_mem[i], fft[i], fp1, wins, analysis_window);
		}

		if(gainnet_bf3->cfg->use_howl_detection)
		{
			wtk_gainnet_bf3_howl_detect(gainnet_bf3, fft);
		}

		wtk_gainnet_bf3_feed_denoise(gainnet_bf3, fft);
		wtk_gainnet_bf3_feed_bf(gainnet_bf3, fft);

		if(gainnet_bf3->gdenoise->agc_gainnet)
		{
            wtk_gainnet_bf3_feed_agc2(gainnet_bf3, fftx);
		}

		// static int cnt=0;
		// cnt++;
		micenr=wtk_gainnet_bf3_fft_energy(fftx, nbin);
		if(micenr>micenr_thresh)
		{
			// if(gainnet_bf3->mic_sil==1)
			// {
			// 	printf("sp start %f %f %f\n", 1.0/16000*cnt*(nbin-1),micenr,micenr_thresh);
			// }
			gainnet_bf3->mic_sil=0;
			gainnet_bf3->mic_silcnt=micenr_cnt;
		}else if(gainnet_bf3->mic_sil==0)
		{
			gainnet_bf3->mic_silcnt-=1;
			if(gainnet_bf3->mic_silcnt<=0)
			{
				// printf("sp end %f\n", 1.0/16000*cnt*(nbin-1));
				gainnet_bf3->mic_sil=1;
			}
		}

		if(gainnet_bf3->maskssl2)
		{
			wtk_maskssl2_feed_fft2(gainnet_bf3->maskssl2, fft, gainnet_bf3->gdenoise->gf, gainnet_bf3->mic_sil);
		}else if(gainnet_bf3->maskssl)
		{
			wtk_maskssl_feed_fft2(gainnet_bf3->maskssl, fft, gainnet_bf3->gdenoise->gf, gainnet_bf3->mic_sil);
		}

		wtk_strbufs_pop(mic, channel, fsize*sizeof(float));
		length=mic[0]->pos/sizeof(float);

		for(i=save_s;i<nbin-1;++i)
		{
			fftx[i]=fft[0][i];
		}
	    wtk_drft_frame_synthesis(rfft, rfft_in, synthesis_mem, fftx, out, wins, synthesis_window);
		if(gainnet_bf3->eq)
		{
			wtk_equalizer_feed_float(gainnet_bf3->eq, out, fsize);
		}
		wtk_gainnet_bf3_control_bs(gainnet_bf3, out, fsize);
		for(i=0; i<fsize; ++i)
		{
			pv[i]=floorf(out[i]+0.5);
		}
		if(gainnet_bf3->notify)
		{
			gainnet_bf3->notify(gainnet_bf3->ths,pv,fsize);
		}
	}
	if(0 && is_end && length>0)
	{
		if(gainnet_bf3->notify)
		{
			pv=(short *)mic[0]->data;
			out=(float *)mic[0]->data;
			for(i=0; i<length; ++i)
			{
				pv[i]=floorf(out[i]+0.5);
			}
			gainnet_bf3->notify(gainnet_bf3->ths,pv,length);
		}
	}
}
