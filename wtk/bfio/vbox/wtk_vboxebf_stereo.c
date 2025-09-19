#include "wtk_vboxebf_stereo.h"
void wtk_vboxebf_stereo_aec_on_gainnet(wtk_vboxebf_stereo_edra_t *vdr, float *gain, int len, int is_end);
void wtk_vboxebf_stereo_denoise_on_gainnet(wtk_vboxebf_stereo_edra_t *vdr, float *gain, int len, int is_end);
void wtk_vboxebf_stereo_agc_on_gainnet(wtk_vboxebf_stereo_edra_t *vdr, float *gain, int len, int is_end);

void wtk_vboxebf_stereo_edra_init(wtk_vboxebf_stereo_edra_t *vdr, wtk_vboxebf_stereo_cfg_t *cfg)
{
	vdr->cfg=cfg;
	vdr->nbin=cfg->wins/2+1;

	vdr->bank_mic=wtk_bankfeat_new(&(cfg->bankfeat));
	vdr->bank_sp=wtk_bankfeat_new(&(cfg->bankfeat));

	vdr->g=wtk_malloc(sizeof(float)*cfg->bankfeat.nb_bands);
	vdr->lastg=wtk_malloc(sizeof(float)*cfg->bankfeat.nb_bands);
	vdr->gf=wtk_malloc(sizeof(float)*vdr->nbin);
	vdr->g2=wtk_malloc(sizeof(float)*cfg->bankfeat.nb_bands);
	vdr->lastg2=wtk_malloc(sizeof(float)*cfg->bankfeat.nb_bands);
	vdr->gf2=wtk_malloc(sizeof(float)*vdr->nbin);

	vdr->feature_sp=NULL;
	if(cfg->featm_lm+cfg->featsp_lm>2)
	{
		vdr->feature_sp=wtk_malloc(sizeof(float)*cfg->bankfeat.nb_features*(cfg->featm_lm+cfg->featsp_lm-1));
	}

	vdr->gainnet2=wtk_gainnet2_new(cfg->gainnet2);
	wtk_gainnet2_set_notify(vdr->gainnet2, vdr, (wtk_gainnet2_notify_f)wtk_vboxebf_stereo_aec_on_gainnet);

	vdr->agc_gainnet=NULL;
	if(cfg->agc_gainnet)
	{
		vdr->agc_gainnet=wtk_gainnet_new(cfg->agc_gainnet);
		wtk_gainnet_set_notify(vdr->agc_gainnet, vdr, (wtk_gainnet_notify_f)wtk_vboxebf_stereo_agc_on_gainnet);
	}

	vdr->qmmse=NULL;
	if(cfg->use_qmmse)
	{
		vdr->qmmse=wtk_qmmse_new(&(cfg->qmmse));
	}
}

void wtk_vboxebf_stereo_edra_clean(wtk_vboxebf_stereo_edra_t *vdr)
{
	wtk_bankfeat_delete(vdr->bank_mic);
	wtk_bankfeat_delete(vdr->bank_sp);

	if(vdr->feature_sp)
	{
		wtk_free(vdr->feature_sp);
	}

	wtk_free(vdr->g);
	wtk_free(vdr->lastg);
	wtk_free(vdr->gf);
	wtk_free(vdr->g2);
	wtk_free(vdr->lastg2);
	wtk_free(vdr->gf2);

	if(vdr->qmmse)
	{
		wtk_qmmse_delete(vdr->qmmse);
	}

	wtk_gainnet2_delete(vdr->gainnet2);

	if(vdr->agc_gainnet)
	{
		wtk_gainnet_delete(vdr->agc_gainnet);
	}
}

void wtk_vboxebf_stereo_edra_reset(wtk_vboxebf_stereo_edra_t *vdr)
{
	wtk_bankfeat_reset(vdr->bank_mic);
	wtk_bankfeat_reset(vdr->bank_sp);

	if(vdr->feature_sp)
	{
		memset(vdr->feature_sp, 0, sizeof(float)*vdr->bank_sp->cfg->nb_features*(vdr->cfg->featm_lm+vdr->cfg->featsp_lm-1));
	}

	memset(vdr->g, 0, sizeof(float)*vdr->cfg->bankfeat.nb_bands);
	memset(vdr->lastg, 0, sizeof(float)*vdr->cfg->bankfeat.nb_bands);
	memset(vdr->gf, 0, sizeof(float)*vdr->nbin);

	memset(vdr->g2, 0, sizeof(float)*vdr->cfg->bankfeat.nb_bands);
	memset(vdr->lastg2, 0, sizeof(float)*vdr->cfg->bankfeat.nb_bands);
	memset(vdr->gf2, 0, sizeof(float)*vdr->nbin);

	wtk_gainnet2_reset(vdr->gainnet2);

	if(vdr->agc_gainnet)
	{
		wtk_gainnet_reset(vdr->agc_gainnet);
	}

	if(vdr->qmmse)
	{
		wtk_qmmse_reset(vdr->qmmse);
	}
}

wtk_vboxebf_stereo_t* wtk_vboxebf_stereo_new(wtk_vboxebf_stereo_cfg_t *cfg)
{
	wtk_vboxebf_stereo_t *vboxebf_stereo;
	int i;

	vboxebf_stereo=(wtk_vboxebf_stereo_t *)wtk_malloc(sizeof(wtk_vboxebf_stereo_t));
	vboxebf_stereo->cfg=cfg;
	vboxebf_stereo->ths=NULL;
	vboxebf_stereo->notify=NULL;

	vboxebf_stereo->mic=wtk_strbufs_new(vboxebf_stereo->cfg->nmicchannel);
	vboxebf_stereo->sp=wtk_strbufs_new(vboxebf_stereo->cfg->nspchannel);

	vboxebf_stereo->nbin=cfg->wins/2+1;
	vboxebf_stereo->analysis_window=wtk_malloc(sizeof(float)*cfg->wins);///2);
	vboxebf_stereo->synthesis_window=wtk_malloc(sizeof(float)*cfg->wins);///2);
	vboxebf_stereo->analysis_mem=wtk_float_new_p2(cfg->nmicchannel, vboxebf_stereo->nbin-1);
	vboxebf_stereo->analysis_mem_sp=wtk_float_new_p2(cfg->nspchannel, vboxebf_stereo->nbin-1);
	vboxebf_stereo->synthesis_mem=wtk_float_new_p2(cfg->noutchannel, vboxebf_stereo->nbin-1);
	vboxebf_stereo->rfft=wtk_drft_new(cfg->wins);
	vboxebf_stereo->rfft_in=(float*)wtk_malloc(sizeof(float)*(cfg->wins));

	vboxebf_stereo->fft=wtk_complex_new_p2(cfg->nmicchannel, vboxebf_stereo->nbin);
	vboxebf_stereo->fft_sp=wtk_complex_new_p2(cfg->nspchannel, vboxebf_stereo->nbin);

    vboxebf_stereo->erls=wtk_malloc(sizeof(wtk_rls_t)*(vboxebf_stereo->nbin));
    for(i=0;i<vboxebf_stereo->nbin;++i)
    {
      wtk_rls_init(vboxebf_stereo->erls+i, &(cfg->echo_rls));
    }
	vboxebf_stereo->fftx=wtk_complex_new_p2(cfg->noutchannel,vboxebf_stereo->nbin);
	vboxebf_stereo->ffty=wtk_complex_new_p2(cfg->noutchannel,vboxebf_stereo->nbin);

	vboxebf_stereo->agc_on=0;
	if(cfg->agc_gainnet)
	{
		vboxebf_stereo->agc_on=1;
	}

	vboxebf_stereo->vdr=(wtk_vboxebf_stereo_edra_t *)wtk_malloc(sizeof(wtk_vboxebf_stereo_edra_t)*cfg->noutchannel);
	for(i=0; i<cfg->noutchannel; ++i)
	{
		wtk_vboxebf_stereo_edra_init(vboxebf_stereo->vdr+i, cfg);
	}

    vboxebf_stereo->ovec = wtk_complex_new_p3(cfg->noutchannel, vboxebf_stereo->nbin, cfg->nomicchannel);

	vboxebf_stereo->scale=(float *)wtk_malloc(sizeof(float)*cfg->noutchannel);
	vboxebf_stereo->last_scale=(float *)wtk_malloc(sizeof(float)*cfg->noutchannel);
	vboxebf_stereo->max_cnt=(int *)wtk_malloc(sizeof(int)*cfg->noutchannel);
	
	vboxebf_stereo->eq=NULL;
	if(cfg->use_eq)
	{
		vboxebf_stereo->eq=wtk_malloc(sizeof(wtk_equalizer_t)*cfg->noutchannel);
		for(i=0; i<cfg->noutchannel; ++i)
		{
			vboxebf_stereo->eq[i]=wtk_equalizer_new(&(cfg->eq));
		}
	}

	vboxebf_stereo->out=wtk_float_new_p2(cfg->noutchannel,(vboxebf_stereo->nbin-1));

	wtk_vboxebf_stereo_reset(vboxebf_stereo);

	return vboxebf_stereo;
}

void wtk_vboxebf_stereo_delete(wtk_vboxebf_stereo_t *vboxebf_stereo)
{
	int i;

	wtk_strbufs_delete(vboxebf_stereo->mic,vboxebf_stereo->cfg->nmicchannel);
	wtk_strbufs_delete(vboxebf_stereo->sp,vboxebf_stereo->cfg->nspchannel);

	wtk_free(vboxebf_stereo->analysis_window);
	wtk_free(vboxebf_stereo->synthesis_window);
	wtk_float_delete_p2(vboxebf_stereo->analysis_mem, vboxebf_stereo->cfg->nmicchannel);
	wtk_float_delete_p2(vboxebf_stereo->analysis_mem_sp, vboxebf_stereo->cfg->nspchannel);
	wtk_float_delete_p2(vboxebf_stereo->synthesis_mem, vboxebf_stereo->cfg->noutchannel);
	wtk_free(vboxebf_stereo->rfft_in);
	wtk_drft_delete(vboxebf_stereo->rfft);
	wtk_complex_delete_p2(vboxebf_stereo->fft, vboxebf_stereo->cfg->nmicchannel);
	wtk_complex_delete_p2(vboxebf_stereo->fft_sp, vboxebf_stereo->cfg->nspchannel);

	for(i=0;i<vboxebf_stereo->nbin;++i)
	{
		wtk_rls_clean(vboxebf_stereo->erls+i);
	}
	wtk_free(vboxebf_stereo->erls);

	for(i=0;i<vboxebf_stereo->cfg->noutchannel;++i)
	{
		if(vboxebf_stereo->eq)
		{
			wtk_equalizer_delete(vboxebf_stereo->eq[i]);
		}
		wtk_vboxebf_stereo_edra_clean(vboxebf_stereo->vdr+i);
	}

	if(vboxebf_stereo->eq)
	{
		wtk_free(vboxebf_stereo->eq);
	}
	wtk_free(vboxebf_stereo->vdr);

	wtk_complex_delete_p3(vboxebf_stereo->ovec,vboxebf_stereo->cfg->noutchannel,vboxebf_stereo->nbin);

	wtk_free(vboxebf_stereo->scale);
	wtk_free(vboxebf_stereo->last_scale);
	wtk_free(vboxebf_stereo->max_cnt);

	wtk_complex_delete_p2(vboxebf_stereo->fftx, vboxebf_stereo->cfg->noutchannel);
	wtk_complex_delete_p2(vboxebf_stereo->ffty, vboxebf_stereo->cfg->noutchannel);
	wtk_float_delete_p2(vboxebf_stereo->out, vboxebf_stereo->cfg->noutchannel);

	wtk_free(vboxebf_stereo);
}


void wtk_vboxebf_stereo_flush_ovec(wtk_vboxebf_stereo_t *vboxebf_stereo, wtk_complex_t *ovec,
                           float **mic_pos, float sv, int rate, float theta2,float phi2, int k)
{
	float x,y,z;
	float t;
	float *mic;
	int i;
	int nomicchannel=vboxebf_stereo->cfg->nomicchannel;
	float tdoa[64];
	int nbin=vboxebf_stereo->nbin;
	int win=(nbin-1)*2;

	phi2*=PI/180;
	theta2*=PI/180;
	x=cosf(phi2)*cosf(theta2);
	y=cosf(phi2)*sinf(theta2);
	z=sinf(phi2);

	for(i=0;i<nomicchannel;++i)
	{
		mic=mic_pos[i];
		tdoa[i]=(mic[0]*x+mic[1]*y+mic[2]*z)/sv;
	}

	t=2*PI*rate*1.0/win*k;
	for(i=0; i<nomicchannel; ++i, ++ovec)
	{
		ovec->a=cosf(t*tdoa[i]);
		ovec->b=sinf(t*tdoa[i]);
	}
}


void wtk_vboxebf_stereo_start(wtk_vboxebf_stereo_t *vboxebf_stereo)
{
	int i,j,k;
	int nbin=vboxebf_stereo->nbin;
	float **omic_pos=vboxebf_stereo->cfg->omic_pos;
	float **otheta=vboxebf_stereo->cfg->otheta;
	int rate=vboxebf_stereo->cfg->rate;
	float eye=vboxebf_stereo->cfg->eye;
	float sv=vboxebf_stereo->cfg->speed;
	int nomicchannel=vboxebf_stereo->cfg->nomicchannel;
	int noutchannel=vboxebf_stereo->cfg->noutchannel;
	wtk_complex_t ***ovec = vboxebf_stereo->ovec;
	wtk_complex_t *ncov, *rk, *otmp;
	wtk_dcomplex_t *tmp2;
	float ff;

    ncov =(wtk_complex_t *)wtk_malloc(nomicchannel*nomicchannel*sizeof(wtk_complex_t));
    rk = (wtk_complex_t *)wtk_malloc(nomicchannel*sizeof(wtk_complex_t));
    tmp2 = (wtk_dcomplex_t *)wtk_malloc(nomicchannel*nomicchannel *2*sizeof(wtk_dcomplex_t));

	memset(rk, 0, sizeof(wtk_complex_t)*nomicchannel);
	rk[0].a=1;
	for(i=0; i<noutchannel; ++i)
	{
		for(k=1; k<nbin-1; ++k)
		{
			for(j=0; j<nomicchannel; ++j) 
			{
                wtk_vboxebf_stereo_flush_ovec(vboxebf_stereo, ncov+nomicchannel*j, omic_pos, sv, rate, otheta[i][j], 0, k);
            }
            for(j=0; j<nomicchannel; ++j)
			{
                ncov[j*nomicchannel+j].a += eye;
            }
            wtk_complex_guass_elimination_p1(ncov, rk, tmp2, nomicchannel,  ovec[i][k]);
            ff=0;
            otmp=ovec[i][k];
            for(j=0; j<nomicchannel; ++j, ++otmp)
			{
                ff += otmp->a * otmp->a + otmp->b * otmp->b;
            }
            ff=1.0/sqrtf(ff);
            otmp=ovec[i][k];
            for(j=0; j<nomicchannel; ++j, ++otmp)
			{
                otmp->a*=ff;
                otmp->b*=ff;
            }
		}
	}

	wtk_free(ncov);
	wtk_free(rk);
	wtk_free(tmp2);
}

void wtk_vboxebf_stereo_reset(wtk_vboxebf_stereo_t *vboxebf_stereo)
{
	int wins=vboxebf_stereo->cfg->wins;
	int i,nbin=vboxebf_stereo->nbin;

	wtk_strbufs_reset(vboxebf_stereo->mic,vboxebf_stereo->cfg->nmicchannel);
	wtk_strbufs_reset(vboxebf_stereo->sp,vboxebf_stereo->cfg->nspchannel);

	for (i=0;i<wins;++i)
	{
		vboxebf_stereo->analysis_window[i] = sinf((0.5+i)*PI/(wins));
	}
	wtk_drft_init_synthesis_window(vboxebf_stereo->synthesis_window, vboxebf_stereo->analysis_window, wins);

	wtk_float_zero_p2(vboxebf_stereo->analysis_mem, vboxebf_stereo->cfg->nmicchannel, (vboxebf_stereo->nbin-1));
	wtk_float_zero_p2(vboxebf_stereo->analysis_mem_sp, vboxebf_stereo->cfg->nspchannel, (vboxebf_stereo->nbin-1));
	wtk_float_zero_p2(vboxebf_stereo->synthesis_mem, vboxebf_stereo->cfg->noutchannel, (vboxebf_stereo->nbin-1));

	wtk_complex_zero_p2(vboxebf_stereo->fft, vboxebf_stereo->cfg->nmicchannel, vboxebf_stereo->nbin);
	wtk_complex_zero_p2(vboxebf_stereo->fft_sp, vboxebf_stereo->cfg->nspchannel, vboxebf_stereo->nbin);

	for(i=0;i<vboxebf_stereo->cfg->noutchannel;++i)
	{
		wtk_vboxebf_stereo_edra_reset(vboxebf_stereo->vdr+i);

		vboxebf_stereo->scale[i]=1.0;
		vboxebf_stereo->last_scale[i]=1.0;
		vboxebf_stereo->max_cnt[i]=0;
	}

	wtk_complex_zero_p3(vboxebf_stereo->ovec, vboxebf_stereo->cfg->noutchannel, vboxebf_stereo->nbin,vboxebf_stereo->cfg->nomicchannel);

	for(i=0;i<nbin;++i)
    {
		wtk_rls_reset(vboxebf_stereo->erls+i);
	}
	wtk_complex_zero_p2(vboxebf_stereo->fftx, vboxebf_stereo->cfg->noutchannel, vboxebf_stereo->nbin);
	wtk_complex_zero_p2(vboxebf_stereo->ffty, vboxebf_stereo->cfg->noutchannel, vboxebf_stereo->nbin);

	vboxebf_stereo->sp_silcnt=0;
	vboxebf_stereo->sp_sil=1;

	vboxebf_stereo->mic_silcnt=0;
	vboxebf_stereo->mic_sil=1;
}


void wtk_vboxebf_stereo_set_notify(wtk_vboxebf_stereo_t *vboxebf_stereo,void *ths,wtk_vboxebf_stereo_notify_f notify)
{
	vboxebf_stereo->notify=notify;
	vboxebf_stereo->ths=ths;
}

static float wtk_vboxebf_stereo_sp_energy(short *p,int n)
{
	float f,f2;
	int i;

	f=0;
	for(i=0;i<n;++i)
	{
		f+=p[i];
	}
    f/=n;

    f2=0;
	for(i=0;i<n;++i)
	{
		f2+=(p[i]-f)*(p[i]-f);
	}
	f2/=n;

	return f2;
}

static float wtk_vboxebf_stereo_fft_energy(wtk_complex_t *fftx,int nbin)
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

void wtk_vboxebf_stereo_denoise_on_gainnet(wtk_vboxebf_stereo_edra_t *vdr, float *gain, int len, int is_end)
{
	memcpy(vdr->g, gain, sizeof(float)*vdr->cfg->bankfeat.nb_bands);
}

void wtk_vboxebf_stereo_aec_on_gainnet(wtk_vboxebf_stereo_edra_t *vdr, float *gain, int len, int is_end)
{
	memcpy(vdr->g, gain, sizeof(float)*vdr->cfg->bankfeat.nb_bands);
}

void wtk_vboxebf_stereo_agc_on_gainnet(wtk_vboxebf_stereo_edra_t *vdr, float *gain, int len, int is_end)
{
	memcpy(vdr->g2, gain, sizeof(float)*vdr->cfg->bankfeat.nb_bands);
}

void wtk_vboxebf_stereo_edra_feed(wtk_vboxebf_stereo_edra_t *vdr, wtk_complex_t *fftx, wtk_complex_t *ffty, int sp_sil)
{
	int i;
	int nbin=vdr->nbin;
	float *g=vdr->g, *gf=vdr->gf, *g2=vdr->g2, *gf2=vdr->gf2;
	float *lastg=vdr->lastg, *lastg2=vdr->lastg2;
	float ralpha=vdr->cfg->ralpha, ralpha2=vdr->cfg->ralpha2;
	float echo_ralpha=vdr->cfg->ralpha, echo_ralpha2=vdr->cfg->echo_ralpha2;
	float agc_a,agc_b;
	float g2_min=vdr->cfg->g2_min;
	float g2_max=vdr->cfg->g2_max;
	float g_minthresh=vdr->cfg->g_minthresh;
	wtk_gainnet2_t *gainnet2=vdr->gainnet2;
	wtk_gainnet_t *agc_gainnet=vdr->agc_gainnet;
	wtk_bankfeat_t *bank_mic=vdr->bank_mic;
	wtk_bankfeat_t *bank_sp=vdr->bank_sp;
	int featsp_lm=vdr->cfg->featsp_lm;
	int featm_lm=vdr->cfg->featm_lm;
	float *feature_sp=vdr->feature_sp;
	int nb_bands=bank_mic->cfg->nb_bands;
	int nb_features=bank_mic->cfg->nb_features;
	wtk_qmmse_t *qmmse=vdr->qmmse;
	float gbias=vdr->cfg->gbias;
	float *qmmse_gain;
	wtk_complex_t *fftytmp, sed, *fftxtmp;
	float ef,yf;
	float leak;

	wtk_bankfeat_flush_frame_features(bank_mic, fftx);
	fftytmp=ffty;
	fftxtmp=fftx;
	for(i=0;i<nbin;++i,++fftxtmp,++fftytmp)
	{
		ef=fftxtmp->a*fftxtmp->a+fftxtmp->b*fftxtmp->b;
		yf=fftytmp->a*fftytmp->a+fftytmp->b*fftytmp->b;
		sed.a=fftytmp->a*fftxtmp->a+fftytmp->b*fftxtmp->b;
		sed.b=-fftytmp->a*fftxtmp->b+fftytmp->b*fftxtmp->a;
		leak=(sed.a*sed.a+sed.b*sed.b)/(max(ef,yf)*yf+1e-9);
		gf[i]=leak*yf;
		leak=sqrtf(leak);
		fftytmp->a*=leak;
		fftytmp->b*=leak;
	}
    wtk_bankfeat_flush_frame_features(bank_sp, ffty);
	if(feature_sp && featsp_lm>1)
	{
		memmove(feature_sp+nb_features*featm_lm,feature_sp+nb_features*(featm_lm-1),sizeof(float)*nb_features*(featsp_lm-1));
		memcpy(feature_sp+nb_features*(featm_lm-1),bank_sp->features,sizeof(float)*nb_features);
	}

	if(qmmse)
	{
		wtk_qmmse_flush_mask(qmmse, fftx, gf);
	}
	if(1)
	{
		if(feature_sp)
		{
			wtk_gainnet2_feed2(gainnet2, bank_mic->features, nb_features, feature_sp, nb_features*(featm_lm+featsp_lm-1), 0);  
		}else
		{
			wtk_gainnet2_feed2(gainnet2, bank_mic->features, nb_features, bank_sp->features, nb_features, 0);  
		}
		if(agc_gainnet)
		{
			wtk_gainnet_feed2(agc_gainnet, bank_mic->features, nb_features, g, nb_bands, 0);  
			if(sp_sil)
			{
				agc_a=vdr->cfg->agc_a;
				agc_b=vdr->cfg->agc_b;
			}else
			{
				agc_a=vdr->cfg->eagc_a;
				agc_b=vdr->cfg->eagc_b;
			}
			for(i=0; i<nb_bands; ++i)
			{
				g2[i]=max(g2_min,g2[i]);
				g2[i]=min(g2_max,g2[i]);
				g2[i]=-1/agc_a*(logf(1/g2[i]-1)-agc_b);
				if(g2[i]<0){g2[i]=g[i];};
			}
			if(sp_sil)
			{
				for(i=0; i<nb_bands; ++i)
				{
					g2[i]=max(g2[i],lastg2[i]*ralpha2);
					lastg2[i]=g2[i];
				}
			}else
			{
				for(i=0; i<nb_bands; ++i)
				{
					g2[i]=max(g2[i],lastg2[i]*echo_ralpha2);
					lastg2[i]=g2[i];
				}
			}
			wtk_bankfeat_interp_band_gain(bank_mic, nbin, gf2, g2);
		}
		if(sp_sil)
		{
			for(i=0; i<nb_bands; ++i)
			{
				g[i]=max(g[i],lastg[i]*ralpha);
				lastg[i]=g[i];
			}
		}else
		{
			for(i=0; i<nb_bands; ++i)
			{
				g[i]=max(g[i],lastg[i]*echo_ralpha);
				lastg[i]=g[i];
			}
		}
		wtk_bankfeat_interp_band_gain(bank_mic, nbin, gf, g);
		if(qmmse && agc_gainnet)
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
		if(sp_sil && gbias>0)
		{
			for (i=1; i<nbin-1; ++i)
			{
				gf[i]=min(gf[i]+gbias,1);
			}
		}
	}
	if(feature_sp && featm_lm>1)
	{
		memmove(feature_sp+nb_features,feature_sp,sizeof(float)*nb_features*(featm_lm-2));
		memcpy(feature_sp,bank_mic->features,sizeof(float)*nb_features);
	}
}


void wtk_vboxebf_stereo_feed_edra(wtk_vboxebf_stereo_t *vboxebf_stereo, wtk_complex_t **fft, wtk_complex_t **fft_sp)
{
	int noutchannel=vboxebf_stereo->cfg->noutchannel;
	int nomicchannel=vboxebf_stereo->cfg->nomicchannel;
	int *orls_channel=vboxebf_stereo->cfg->orls_channel;
	int **omic_channel=vboxebf_stereo->cfg->omic_channel;
	int nspchannel=vboxebf_stereo->cfg->nspchannel;
	int i,j,k,n;
	int nbin=vboxebf_stereo->nbin;
	wtk_rls_t *erls=vboxebf_stereo->erls, *erlstmp;
	wtk_complex_t **fftx=vboxebf_stereo->fftx;
	wtk_complex_t **ffty=vboxebf_stereo->ffty;
	wtk_complex_t ffttmp[64]={0};
	wtk_complex_t fftsp2[10]={0};
	wtk_vboxebf_stereo_edra_t *vdr=vboxebf_stereo->vdr;

	erlstmp=erls;
	for(k=0; k<nbin; ++k, ++erlstmp)
	{
		for(i=0; i<noutchannel; ++i)
		{
			ffttmp[i]=fft[orls_channel[i]][k];
		}
		for(j=0; j<nspchannel; ++j)
		{
			fftsp2[j]=fft_sp[j][k];
		}
		wtk_rls_feed3(erlstmp, ffttmp, fftsp2, vboxebf_stereo->sp_sil==0);
		if(vboxebf_stereo->sp_sil==0)
		{
			for(i=0; i<noutchannel; ++i)
			{
				for(j=0; j<nomicchannel; ++j)
				{
					n=omic_channel[i][j];
					fft[n][k].a-=erlstmp->lsty[i].a;
					fft[n][k].b-=erlstmp->lsty[i].b;
				}
				fft[orls_channel[i]][k]=erlstmp->out[i];
				ffty[i][k]=erlstmp->lsty[i];
				fftx[i][k]=erlstmp->out[i];
			}
		}else
		{
			for(i=0; i<noutchannel; ++i)
			{
				fftx[i][k]=ffttmp[i];
				ffty[i][k].a=ffty[i][k].b=0;
			}
		}
	}
	for(i=0; i<noutchannel; ++i)
	{
		wtk_vboxebf_stereo_edra_feed(vdr+i, fftx[i], ffty[i], vboxebf_stereo->sp_sil);
	}
}


void wtk_vboxebf_stereo_feed_bf(wtk_vboxebf_stereo_t *vboxebf_stereo, wtk_complex_t **fft)
{
	wtk_complex_t **fftx=vboxebf_stereo->fftx;
	wtk_complex_t ***ovec=vboxebf_stereo->ovec, *ovectmp;
	int k,nbin=vboxebf_stereo->nbin;
	int i,j,n;
	int noutchannel=vboxebf_stereo->cfg->noutchannel;
	int **omic_channel=vboxebf_stereo->cfg->omic_channel;
	int nomicchannel=vboxebf_stereo->cfg->nomicchannel;
	wtk_vboxebf_stereo_edra_t *vdr;
	float gf;
	wtk_complex_t fft2[64];
	int clip_s=vboxebf_stereo->cfg->clip_s;
	int clip_e=vboxebf_stereo->cfg->clip_e;
	float ta,tb;

	for(i=0;i<noutchannel;++i)
	{
		fftx[i][0].a=fftx[i][0].b=0;
		fftx[i][nbin-1].a=fftx[i][nbin-1].b=0;
		for(k=0; k<=clip_s; ++k)
		{
			fftx[i][k].a=fftx[i][k].b=0;
		}
		vdr=vboxebf_stereo->vdr+i;
		for(k=clip_s+1; k<clip_e; ++k)
		{
			ovectmp=ovec[i][k];
			gf=vdr->gf[k];
			for(j=0; j<nomicchannel; ++j)
			{
				n=omic_channel[i][j];
				fft2[j]=fft[n][k];
			}
			ta=tb=0;
			for(j=0; j<nomicchannel; ++j, ++ovectmp)
			{
				ta+=ovectmp->a*fft2[j].a - ovectmp->b*fft2[j].b;
				tb+=ovectmp->a*fft2[j].b + ovectmp->b*fft2[j].a;
			}
			fftx[i][k].a=ta*gf;
			fftx[i][k].b=tb*gf;
		}
		for(k=clip_e; k<nbin; ++k)
		{
			fftx[i][k].a=fftx[i][k].b=0;
		}
	}
}

void wtk_vboxebf_stereo_feed_agc2(wtk_vboxebf_stereo_t *vboxebf_stereo, wtk_complex_t **fft)
{
	wtk_vboxebf_stereo_edra_t *vdr;
	int noutchannel=vboxebf_stereo->cfg->noutchannel;
	int nbin=vboxebf_stereo->nbin;
	float agcaddg=vboxebf_stereo->cfg->agcaddg;
	float *gf2;
	int i,j;

	if(agcaddg>1.0)
	{
		for(j=0; j<noutchannel; ++j)
		{
			vdr=vboxebf_stereo->vdr+j;
			if (!vdr->bank_mic->silence )
			{
				for (i=1;i<nbin-1;++i)
				{
					fft[j][i].a *= agcaddg;
					fft[j][i].b *= agcaddg;
				}
			}
		}
	}else
	{
		for(j=0; j<noutchannel; ++j)
		{
			vdr=vboxebf_stereo->vdr+j;
			gf2=vdr->gf2;
			if (!vdr->bank_mic->silence )
			{
				for (i=1;i<nbin-1;++i)
				{
					fft[j][i].a *= gf2[i];
					fft[j][i].b *= gf2[i];
				}
			}
		}
	}
}


void wtk_vboxebf_stereo_control_bs(wtk_vboxebf_stereo_t *vboxebf_stereo, float *out, int len, int nch)
{
	float out_max;
	float *scale=vboxebf_stereo->scale;
	float *last_scale=vboxebf_stereo->last_scale;
	int *max_cnt=vboxebf_stereo->max_cnt;
	int i;

	if(vboxebf_stereo->mic_sil==0)
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


void wtk_vboxebf_stereo_feed(wtk_vboxebf_stereo_t *vboxebf_stereo,short *data,int len,int is_end)
{
	int i,j;
	int nbin=vboxebf_stereo->nbin;
	int nmicchannel=vboxebf_stereo->cfg->nmicchannel;
	int noutchannel=vboxebf_stereo->cfg->noutchannel;
	int *mic_channel=vboxebf_stereo->cfg->mic_channel;
	int nspchannel=vboxebf_stereo->cfg->nspchannel;
	int *sp_channel=vboxebf_stereo->cfg->sp_channel;
	int channel=vboxebf_stereo->cfg->channel;
	wtk_strbuf_t **mic=vboxebf_stereo->mic;
	wtk_strbuf_t **sp=vboxebf_stereo->sp;
	int wins=vboxebf_stereo->cfg->wins;
	int fsize=wins/2;
	int length;
	float spenr;
	float spenr_thresh=vboxebf_stereo->cfg->spenr_thresh;
	int spenr_cnt=vboxebf_stereo->cfg->spenr_cnt;
	float micenr;
	float micenr_thresh=vboxebf_stereo->cfg->micenr_thresh;
	int micenr_cnt=vboxebf_stereo->cfg->micenr_cnt;
	wtk_drft_t *rfft=vboxebf_stereo->rfft;
	float *rfft_in=vboxebf_stereo->rfft_in;
	wtk_complex_t **fft=vboxebf_stereo->fft;
	wtk_complex_t **fft_sp=vboxebf_stereo->fft_sp;
	float **analysis_mem=vboxebf_stereo->analysis_mem, **analysis_mem_sp=vboxebf_stereo->analysis_mem_sp;
	float **synthesis_mem=vboxebf_stereo->synthesis_mem;
	float *analysis_window=vboxebf_stereo->analysis_window, *synthesis_window=vboxebf_stereo->synthesis_window;
	wtk_complex_t **fftx=vboxebf_stereo->fftx;
	float **out=vboxebf_stereo->out;
	short *pv[64];

	for(i=0;i<len;++i)
	{
		for(j=0; j<nmicchannel; ++j)
		{
			wtk_strbuf_push(mic[j],(char *)(data+mic_channel[j]),sizeof(short));
		}
		for(j=0; j<nspchannel; ++j)
		{
			wtk_strbuf_push(sp[j],(char *)(data+sp_channel[j]),sizeof(short));
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
		for(i=0; i<nspchannel; ++i)
		{
			wtk_drft_frame_analysis2(rfft, rfft_in, analysis_mem_sp[i], fft_sp[i], (short *)(sp[i]->data), wins, analysis_window);
		}
		if(nspchannel>0){
			spenr=wtk_vboxebf_stereo_sp_energy((short *)sp[0]->data, fsize);
		}else{
			spenr=0;
		}
		// static int cnt=0;
		// cnt++;
		if(spenr>spenr_thresh)
		{
			// if(vboxebf_stereo->sp_sil==1)
			// {
			// 	printf("sp start %f %f %f\n", 1.0/16000*cnt*(nbin-1),spenr,spenr_thresh);
			// }
			vboxebf_stereo->sp_sil=0;
			vboxebf_stereo->sp_silcnt=spenr_cnt;
		}else if(vboxebf_stereo->sp_sil==0)
		{
			vboxebf_stereo->sp_silcnt-=1;
			if(vboxebf_stereo->sp_silcnt<=0)
			{
				// printf("sp end %f\n", 1.0/16000*cnt*(nbin-1));
				vboxebf_stereo->sp_sil=1;
			}
		}

		wtk_vboxebf_stereo_feed_edra(vboxebf_stereo, fft, fft_sp);
		wtk_vboxebf_stereo_feed_bf(vboxebf_stereo, fft);
		
		if(vboxebf_stereo->agc_on)
		{
            wtk_vboxebf_stereo_feed_agc2(vboxebf_stereo, fftx);
		}

		// static int cnt=0;
		// cnt++;
		micenr=wtk_vboxebf_stereo_fft_energy(fftx[0], nbin);
		if(micenr>micenr_thresh)
		{
			// if(vboxebf_stereo->mic_sil==1)
			// {
			// 	printf("sp start %f %f %f\n", 1.0/16000*cnt*(nbin-1),micenr,micenr_thresh);
			// }
			vboxebf_stereo->mic_sil=0;
			vboxebf_stereo->mic_silcnt=micenr_cnt;
		}else if(vboxebf_stereo->mic_sil==0)
		{
			vboxebf_stereo->mic_silcnt-=1;
			if(vboxebf_stereo->mic_silcnt<=0)
			{
				// printf("sp end %f\n", 1.0/16000*cnt*(nbin-1));
				vboxebf_stereo->mic_sil=1;
			}
		}

		wtk_strbufs_pop(mic, nmicchannel, fsize*sizeof(short));
		wtk_strbufs_pop(sp, nspchannel, fsize*sizeof(short));
		length=mic[0]->pos/sizeof(short);

		for(i=0;i<noutchannel;++i)
		{
	    	wtk_drft_frame_synthesis(rfft, rfft_in, synthesis_mem[i], fftx[i], out[i], wins, synthesis_window);
		}
		if(vboxebf_stereo->eq)
		{
			for(i=0;i<noutchannel;++i)
			{
				wtk_equalizer_feed_float(vboxebf_stereo->eq[i], out[i], fsize);
			}
		}
		for(i=0;i<noutchannel;++i)
		{
			wtk_vboxebf_stereo_control_bs(vboxebf_stereo, out[i], fsize, i);
		}
		for(i=0;i<noutchannel;++i)
		{
			pv[i]=(short *)out[i];
			for(j=0; j<fsize; ++j)
			{
				pv[i][j]=floorf(out[i][j]+0.5);
			}
		}
		if(vboxebf_stereo->notify)
		{
			vboxebf_stereo->notify(vboxebf_stereo->ths,pv,fsize);
		}
	}
}