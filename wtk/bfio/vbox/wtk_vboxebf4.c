#include "wtk_vboxebf4.h"

void wtk_vboxebf4_aec_on_gainnet(wtk_vboxebf4_aec_t *vaec, float *gain, int len, int is_end);
void wtk_vboxebf4_aec_on_gainnet2(wtk_vboxebf4_aec_t *vaec, float *gain, int len, int is_end);
void wtk_vboxebf4_on_ssl2(wtk_vboxebf4_t *vboxebf4, wtk_ssl2_extp_t *nbest_extp,int nbest, int ts,int te);

void wtk_vboxebf4_aec_init(wtk_vboxebf4_aec_t *vaec, wtk_vboxebf4_cfg_t *cfg)
{
	vaec->cfg=cfg;
	vaec->nbin=cfg->wins/2+1;

	vaec->bank_mic=wtk_bankfeat_new(&(cfg->bankfeat));
	vaec->bank_sp=wtk_bankfeat_new(&(cfg->bankfeat));

	vaec->lastg=wtk_malloc(sizeof(float)*cfg->bankfeat.nb_bands);
	vaec->g=wtk_malloc(sizeof(float)*cfg->bankfeat.nb_bands);
	vaec->gf=wtk_malloc(sizeof(float)*vaec->nbin);
	vaec->g2=wtk_malloc(sizeof(float)*cfg->bankfeat.nb_bands);
	vaec->gf2=wtk_malloc(sizeof(float)*vaec->nbin);

	vaec->gainnet6=NULL;
	vaec->gainnet=NULL;
	if(cfg->use_gainnet6)
	{
		vaec->gainnet6=wtk_gainnet6_new(cfg->gainnet6);
		wtk_gainnet6_set_notify(vaec->gainnet6, vaec, (wtk_gainnet6_notify_f)wtk_vboxebf4_aec_on_gainnet);
		wtk_gainnet6_set_notify2(vaec->gainnet6, vaec, (wtk_gainnet6_notify_f2)wtk_vboxebf4_aec_on_gainnet2);
	}else
	{
		vaec->gainnet=wtk_gainnet5_new(cfg->gainnet);
		wtk_gainnet5_set_notify(vaec->gainnet, vaec, (wtk_gainnet5_notify_f)wtk_vboxebf4_aec_on_gainnet);
		wtk_gainnet5_set_notify2(vaec->gainnet, vaec, (wtk_gainnet5_notify_f2)wtk_vboxebf4_aec_on_gainnet2);
	}

	vaec->qmmse=NULL;
	if(cfg->use_qmmse)
	{
		vaec->qmmse=wtk_qmmse_new(&(cfg->qmmse));
	}
}

void wtk_vboxebf4_aec_clean(wtk_vboxebf4_aec_t *vaec)
{
	wtk_bankfeat_delete(vaec->bank_mic);
	wtk_bankfeat_delete(vaec->bank_sp);

	wtk_free(vaec->lastg);
	wtk_free(vaec->g);
	wtk_free(vaec->gf);
	wtk_free(vaec->g2);
	wtk_free(vaec->gf2);

	if(vaec->qmmse)
	{
		wtk_qmmse_delete(vaec->qmmse);
	}

	if(vaec->gainnet)
	{
		wtk_gainnet5_delete(vaec->gainnet);
	}else if(vaec->gainnet6)
	{
		wtk_gainnet6_delete(vaec->gainnet6);
	}
}

void wtk_vboxebf4_aec_reset(wtk_vboxebf4_aec_t *vaec)
{
	wtk_bankfeat_reset(vaec->bank_mic);
	wtk_bankfeat_reset(vaec->bank_sp);

	memset(vaec->lastg, 0, sizeof(float)*vaec->cfg->bankfeat.nb_bands);
	memset(vaec->g, 0, sizeof(float)*vaec->cfg->bankfeat.nb_bands);
	memset(vaec->gf, 0, sizeof(float)*vaec->nbin);

	memset(vaec->g2, 0, sizeof(float)*vaec->cfg->bankfeat.nb_bands);
	memset(vaec->gf2, 0, sizeof(float)*vaec->nbin);

	if(vaec->gainnet)
	{
		wtk_gainnet5_reset(vaec->gainnet);
	}else if(vaec->gainnet6)
	{
		wtk_gainnet6_reset(vaec->gainnet6);
	}

	if(vaec->qmmse)
	{
		wtk_qmmse_reset(vaec->qmmse);
	}
}


wtk_vboxebf4_t* wtk_vboxebf4_new(wtk_vboxebf4_cfg_t *cfg)
{
	wtk_vboxebf4_t *vboxebf4;
	int i;

	vboxebf4=(wtk_vboxebf4_t *)wtk_malloc(sizeof(wtk_vboxebf4_t));
	vboxebf4->cfg=cfg;
	vboxebf4->ths=NULL;
	vboxebf4->notify=NULL;
	vboxebf4->ssl_ths=NULL;
    vboxebf4->notify_ssl=NULL;
	vboxebf4->eng_ths=NULL;
    vboxebf4->notify_eng=NULL;

	vboxebf4->mic=wtk_strbufs_new(vboxebf4->cfg->nmicchannel);
	vboxebf4->sp=wtk_strbufs_new(vboxebf4->cfg->nspchannel);

	vboxebf4->nbin=cfg->wins/2+1;
	vboxebf4->analysis_window=wtk_malloc(sizeof(float)*cfg->wins);///2);
	vboxebf4->synthesis_window=wtk_malloc(sizeof(float)*cfg->wins);///2);
	vboxebf4->analysis_mem=wtk_float_new_p2(cfg->nmicchannel, vboxebf4->nbin-1);
	vboxebf4->analysis_mem_sp=wtk_float_new_p2(cfg->nspchannel, vboxebf4->nbin-1);
	vboxebf4->synthesis_mem=wtk_malloc(sizeof(float)*(vboxebf4->nbin-1));
	vboxebf4->rfft=wtk_drft_new(cfg->wins);
	vboxebf4->rfft_in=(float*)wtk_malloc(sizeof(float)*(cfg->wins));

	vboxebf4->fft=wtk_complex_new_p2(cfg->nmicchannel, vboxebf4->nbin);
	vboxebf4->fft2=wtk_complex_new_p2(vboxebf4->nbin, cfg->nmicchannel);
	vboxebf4->fft_sp=wtk_complex_new_p2(cfg->nspchannel, vboxebf4->nbin);

    vboxebf4->erls=wtk_malloc(sizeof(wtk_rls_t)*(vboxebf4->nbin));
    for(i=0;i<vboxebf4->nbin;++i)
    {
      wtk_rls_init(vboxebf4->erls+i, &(cfg->echo_rls));
    }
	vboxebf4->fftx=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*vboxebf4->nbin);
	vboxebf4->ffty=wtk_complex_new_p2(cfg->nmicchannel, vboxebf4->nbin);
	vboxebf4->ffts=wtk_complex_new_p2(vboxebf4->nbin, cfg->nmicchannel);
	vboxebf4->ffty2=wtk_complex_new_p2(vboxebf4->nbin, cfg->nmicchannel);

	if(cfg->use_epostsingle)
	{
		vboxebf4->vaec=(wtk_vboxebf4_aec_t *)wtk_malloc(sizeof(wtk_vboxebf4_aec_t));
		wtk_vboxebf4_aec_init(vboxebf4->vaec, cfg);
	}else
	{
		vboxebf4->vaec=(wtk_vboxebf4_aec_t *)wtk_malloc(sizeof(wtk_vboxebf4_aec_t)*cfg->nmicchannel);
		for(i=0; i<cfg->nmicchannel; ++i)
		{
			wtk_vboxebf4_aec_init(vboxebf4->vaec+i, cfg);
		}
	}

	vboxebf4->covm=NULL;
	vboxebf4->echo_covm=NULL;
	if(!cfg->use_fixtheta)
	{
		vboxebf4->covm=wtk_covm_new(&(cfg->covm), vboxebf4->nbin, cfg->nbfchannel);
		if(cfg->use_echocovm)
		{
			vboxebf4->echo_covm=wtk_covm_new(&(cfg->echo_covm), vboxebf4->nbin, cfg->nbfchannel);
		}
	}
	vboxebf4->bf=wtk_bf_new(&(cfg->bf), vboxebf4->cfg->wins);
	
	vboxebf4->eq=NULL;
	if(cfg->use_eq)
	{
		vboxebf4->eq=wtk_equalizer_new(&(cfg->eq));
	}

    vboxebf4->ssl2=NULL;
	vboxebf4->maskssl=NULL;
	vboxebf4->maskssl2=NULL;
    if(cfg->use_ssl && !cfg->use_ssl_delay)
    {
        vboxebf4->ssl2=wtk_ssl2_new(&(cfg->ssl2));
        wtk_ssl2_set_notify(vboxebf4->ssl2, vboxebf4, (wtk_ssl2_notify_f)wtk_vboxebf4_on_ssl2);
    }else if(cfg->use_maskssl && !cfg->use_ssl_delay)
	{
		vboxebf4->maskssl=wtk_maskssl_new(&(cfg->maskssl));
        wtk_maskssl_set_notify(vboxebf4->maskssl, vboxebf4, (wtk_maskssl_notify_f)wtk_vboxebf4_on_ssl2);
	}else if(cfg->use_maskssl2 && !cfg->use_ssl_delay)
	{
		vboxebf4->maskssl2=wtk_maskssl2_new(&(cfg->maskssl2));
        wtk_maskssl2_set_notify(vboxebf4->maskssl2, vboxebf4, (wtk_maskssl2_notify_f)wtk_vboxebf4_on_ssl2);
	}

	vboxebf4->out=wtk_malloc(sizeof(float)*(vboxebf4->nbin-1));

	wtk_vboxebf4_reset(vboxebf4);

	return vboxebf4;
}

void wtk_vboxebf4_delete(wtk_vboxebf4_t *vboxebf4)
{
	int i;
	int nmicchannel=vboxebf4->cfg->nmicchannel;

	wtk_strbufs_delete(vboxebf4->mic,vboxebf4->cfg->nmicchannel);
	wtk_strbufs_delete(vboxebf4->sp,vboxebf4->cfg->nspchannel);

	wtk_free(vboxebf4->analysis_window);
	wtk_free(vboxebf4->synthesis_window);
	wtk_float_delete_p2(vboxebf4->analysis_mem, vboxebf4->cfg->nmicchannel);
	wtk_float_delete_p2(vboxebf4->analysis_mem_sp, vboxebf4->cfg->nspchannel);
	wtk_free(vboxebf4->synthesis_mem);
	wtk_free(vboxebf4->rfft_in);
	wtk_drft_delete(vboxebf4->rfft);
	wtk_complex_delete_p2(vboxebf4->fft, vboxebf4->cfg->nmicchannel);
	wtk_complex_delete_p2(vboxebf4->fft2, vboxebf4->nbin);
	wtk_complex_delete_p2(vboxebf4->fft_sp, vboxebf4->cfg->nspchannel);

	if(vboxebf4->covm)
	{
		wtk_covm_delete(vboxebf4->covm);
	}
	if(vboxebf4->echo_covm)
	{
		wtk_covm_delete(vboxebf4->echo_covm);
	}
	wtk_bf_delete(vboxebf4->bf);

	for(i=0;i<vboxebf4->nbin;++i)
	{
		wtk_rls_clean(vboxebf4->erls+i);
	}
	wtk_free(vboxebf4->erls);

	if(vboxebf4->eq)
	{
		wtk_equalizer_delete(vboxebf4->eq);
	}

	wtk_free(vboxebf4->fftx);

	wtk_complex_delete_p2(vboxebf4->ffty, vboxebf4->cfg->nmicchannel);
	wtk_complex_delete_p2(vboxebf4->ffts, vboxebf4->nbin);
	wtk_complex_delete_p2(vboxebf4->ffty2, vboxebf4->nbin);

	if(vboxebf4->cfg->use_epostsingle)
	{
		wtk_vboxebf4_aec_clean(vboxebf4->vaec);
		wtk_free(vboxebf4->vaec);
	}else
	{
		for(i=0; i<nmicchannel; ++i)
		{
			wtk_vboxebf4_aec_clean(vboxebf4->vaec+i);
		}
		wtk_free(vboxebf4->vaec);
	}

    if(vboxebf4->ssl2)
    {
        wtk_ssl2_delete(vboxebf4->ssl2);
    }
    if(vboxebf4->maskssl)
    {
        wtk_maskssl_delete(vboxebf4->maskssl);
    }    
	if(vboxebf4->maskssl2)
    {
        wtk_maskssl2_delete(vboxebf4->maskssl2);
    }
	wtk_free(vboxebf4->out);

	wtk_free(vboxebf4);
}


void wtk_vboxebf4_start(wtk_vboxebf4_t *vboxebf4)
{
	wtk_bf_update_ovec(vboxebf4->bf,vboxebf4->cfg->theta,vboxebf4->cfg->phi);
	wtk_bf_init_w(vboxebf4->bf);
}

void wtk_vboxebf4_reset(wtk_vboxebf4_t *vboxebf4)
{
	int wins=vboxebf4->cfg->wins;
	int i,nbin=vboxebf4->nbin;
	int nmicchannel=vboxebf4->cfg->nmicchannel;

	wtk_strbufs_reset(vboxebf4->mic,vboxebf4->cfg->nmicchannel);
	wtk_strbufs_reset(vboxebf4->sp,vboxebf4->cfg->nspchannel);

	for (i=0;i<wins;++i)
	{
		vboxebf4->analysis_window[i] = sin((0.5+i)*PI/(wins));
	}
	wtk_drft_init_synthesis_window(vboxebf4->synthesis_window, vboxebf4->analysis_window, wins);

	wtk_float_zero_p2(vboxebf4->analysis_mem, vboxebf4->cfg->nmicchannel, (vboxebf4->nbin-1));
	wtk_float_zero_p2(vboxebf4->analysis_mem_sp, vboxebf4->cfg->nspchannel, (vboxebf4->nbin-1));
	memset(vboxebf4->synthesis_mem, 0, sizeof(float)*(vboxebf4->nbin-1));

	wtk_complex_zero_p2(vboxebf4->fft, vboxebf4->cfg->nmicchannel, vboxebf4->nbin);
	wtk_complex_zero_p2(vboxebf4->fft2, vboxebf4->nbin, vboxebf4->cfg->nmicchannel);
	wtk_complex_zero_p2(vboxebf4->fft_sp, vboxebf4->cfg->nspchannel, vboxebf4->nbin);

	if(vboxebf4->covm)
	{
		wtk_covm_reset(vboxebf4->covm);
	}
	if(vboxebf4->echo_covm)
	{
		wtk_covm_reset(vboxebf4->echo_covm);
	}

	wtk_bf_reset(vboxebf4->bf);

	for(i=0;i<nbin;++i)
    {
		wtk_rls_reset(vboxebf4->erls+i);
	}
	wtk_complex_zero_p2(vboxebf4->ffty, vboxebf4->cfg->nmicchannel, vboxebf4->nbin);

	wtk_complex_zero_p2(vboxebf4->ffts, vboxebf4->nbin, vboxebf4->cfg->nmicchannel);
	wtk_complex_zero_p2(vboxebf4->ffty2, vboxebf4->nbin, vboxebf4->cfg->nmicchannel);

	memset(vboxebf4->fftx, 0, sizeof(wtk_complex_t)*(vboxebf4->nbin));

	if(vboxebf4->cfg->use_epostsingle)
	{
		wtk_vboxebf4_aec_reset(vboxebf4->vaec);
	}else
	{
		for(i=0; i<nmicchannel; ++i)
		{
			wtk_vboxebf4_aec_reset(vboxebf4->vaec+i);
		}
	}

	vboxebf4->sp_silcnt=0;
	vboxebf4->sp_sil=1;

	vboxebf4->mic_silcnt=0;
	vboxebf4->mic_sil=1;

	vboxebf4->pframe=0;

	vboxebf4->ssl_enable=0;
    if(vboxebf4->ssl2)
    {
        wtk_ssl2_reset(vboxebf4->ssl2);
		vboxebf4->ssl_enable=1;
    }
	if(vboxebf4->maskssl)
    {
        wtk_maskssl_reset(vboxebf4->maskssl);
		vboxebf4->ssl_enable=1;
    }	
	if(vboxebf4->maskssl2)
    {
        wtk_maskssl2_reset(vboxebf4->maskssl2);
		vboxebf4->ssl_enable=1;
    }

	vboxebf4->inmic_scale=1.0;
	vboxebf4->agc_enable=1;
	vboxebf4->echo_enable=1;
	vboxebf4->denoise_enable=1;
	vboxebf4->bs_scale=1.0;
	vboxebf4->bs_last_scale=1.0;
	vboxebf4->bs_max_cnt=0;
}


void wtk_vboxebf4_set_notify(wtk_vboxebf4_t *vboxebf4,void *ths,wtk_vboxebf4_notify_f notify)
{
	vboxebf4->notify=notify;
	vboxebf4->ths=ths;
}

void wtk_vboxebf4_set_ssl_notify(wtk_vboxebf4_t *vboxebf4,void *ths,wtk_vboxebf4_notify_ssl_f notify)
{
	vboxebf4->notify_ssl=notify;
	vboxebf4->ssl_ths=ths;
}

void wtk_vboxebf4_set_eng_notify(wtk_vboxebf4_t *vboxebf4,void *ths,wtk_vboxebf4_notify_eng_f notify)
{
	vboxebf4->notify_eng=notify;
	vboxebf4->eng_ths=ths;
}

static float wtk_vboxebf4_sp_energy(float *p,int n)
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

static float wtk_vboxebf4_fft_energy(wtk_complex_t *fftx,int nbin)
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

void wtk_vboxebf4_aec_on_gainnet(wtk_vboxebf4_aec_t *vaec, float *gain, int len, int is_end)
{
	memcpy(vaec->g, gain, sizeof(float)*vaec->cfg->bankfeat.nb_bands);
}


void wtk_vboxebf4_aec_on_gainnet2(wtk_vboxebf4_aec_t *vaec, float *gain, int len, int is_end)
{
	int i;
	int nb_bands=vaec->cfg->bankfeat.nb_bands;
	float *g2=vaec->g2;
	float agc_a=vaec->cfg->agc_a;
	float agc_a2=vaec->cfg->agc_a2;
	float agc_b=vaec->cfg->agc_b;

	for(i=0; i<nb_bands; ++i)
	{
		g2[i]=-1/agc_a*agc_a2*(logf(1/gain[i]-1)-agc_b);
	}
}

void wtk_vboxebf4_aec_feed(wtk_vboxebf4_aec_t *vaec, wtk_complex_t *fftx, wtk_complex_t *ffty, wtk_complex_t **ffts, wtk_complex_t **ffty2, int nch, float gbias, int sp_sil)
{
	int i;
	int nbin=vaec->nbin;
	float *g=vaec->g, *gf=vaec->gf, *lastg=vaec->lastg, *g2=vaec->g2, *gf2=vaec->gf2;
	wtk_gainnet5_t *gainnet=vaec->gainnet;
	wtk_gainnet6_t *gainnet6=vaec->gainnet6;
	wtk_bankfeat_t *bank_mic=vaec->bank_mic;
	wtk_bankfeat_t *bank_sp=vaec->bank_sp;
	int nb_bands=bank_mic->cfg->nb_bands;
	int nb_features=bank_mic->cfg->nb_features;
	wtk_qmmse_t *qmmse=vaec->qmmse;
	float *qmmse_gain;
	wtk_complex_t *fftytmp, sed, *fftxtmp;
	float ef,yf;
	float leak;
	float ralpha=vaec->cfg->ralpha;
	float gftmp;

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
	if(qmmse)
	{
		wtk_qmmse_flush_mask(qmmse, fftx, gf);
	}

	if(gainnet)
	{
		wtk_gainnet5_feed(gainnet, bank_mic->features, nb_features, bank_sp->features, nb_features, 0);   
	}else
	{
		wtk_gainnet6_feed(gainnet6, bank_mic->features, nb_features, bank_sp->features, nb_features, 0); 
	}
	for (i=0;i<nb_bands;++i)
	{
		if(sp_sil==1)
		{
			g[i] = max(g[i], ralpha*lastg[i]);
		}
		lastg[i] = g[i];
	}
	wtk_bankfeat_interp_band_gain(bank_mic, nbin, gf, g);
	if((gainnet && gainnet->cfg->use_agc) || (gainnet6 && gainnet6->cfg->use_agc))
	{
		wtk_bankfeat_interp_band_gain(bank_mic, nbin, gf2, g2);
	}
	if(qmmse)
	{
		qmmse_gain=qmmse->gain;
		for (i=1; i<nbin-1; ++i)
		{
			if(gf[i]>qmmse_gain[i] )
			{
				if(sp_sil==1 && vaec->cfg->use_demmse==1)
				{
					gftmp=(qmmse_gain[i]+gf[i])/2;
					if((gainnet && gainnet->cfg->use_agc) || (gainnet6 && gainnet6->cfg->use_agc))
					{
						gf2[i]*=gftmp/gf[i];
					}
					gf[i]=gftmp;
				}else if(sp_sil==0 && vaec->cfg->use_aecmmse==1)
				{
					if((gainnet && gainnet->cfg->use_agc) || (gainnet6 && gainnet6->cfg->use_agc))
					{
						gf2[i]*=qmmse_gain[i]/gf[i];
					}
					gf[i]=qmmse_gain[i];
				}
			}
		}
	}
	if(sp_sil==1 && gbias>0)
	{
		for (i=1; i<nbin-1; ++i)
		{
			gf[i]=min(gf[i]+gbias,1);
		}
	}

	for (i=1; i<nbin-1; ++i)
	{
		ffts[i][nch].a *= gf[i];
		ffts[i][nch].b *= gf[i];

		ffty2[i][nch].a=fftx[i].a-ffts[i][nch].a;
		ffty2[i][nch].b=fftx[i].b-ffts[i][nch].b;
	}
}


void wtk_vboxebf4_feed_echo(wtk_vboxebf4_t *vboxebf4, wtk_complex_t **fft, wtk_complex_t **fft_sp)
{
	int nmicchannel=vboxebf4->cfg->nmicchannel;
	int nspchannel=vboxebf4->cfg->nspchannel;
	int i,j,k;
	int nbin=vboxebf4->nbin;
	wtk_rls_t *erls=vboxebf4->erls, *erlstmp;
	wtk_complex_t ffttmp[64]={0};
	wtk_complex_t fftsp2[10]={0};
	wtk_complex_t **ffty=vboxebf4->ffty;
	wtk_complex_t **ffty2=vboxebf4->ffty2;
	wtk_vboxebf4_aec_t *vaec=vboxebf4->vaec;
	wtk_complex_t **ffts=vboxebf4->ffts;
	wtk_complex_t **fft2=vboxebf4->fft2;
	float pframe_alpha=vboxebf4->cfg->pframe_alpha;
	int pfs=vboxebf4->cfg->pframe_fs;
	int fe_len=vboxebf4->cfg->pframe_fe-vboxebf4->cfg->pframe_fs;
	float *gf=vaec->gf;
	float gtmp;

	wtk_complex_zero_p2(vboxebf4->ffty, vboxebf4->cfg->nmicchannel, vboxebf4->nbin);

	erlstmp=erls;
	for(k=0; k<nbin; ++k, ++erlstmp)
	{
		for(i=0; i<nmicchannel; ++i)
		{
			ffttmp[i]=fft[i][k];
		}
		for(j=0; j<nspchannel; ++j)
		{
			fftsp2[j]=fft_sp[j][k];
		}
		wtk_rls_feed3(erlstmp, ffttmp, fftsp2, vboxebf4->sp_sil==0 && vboxebf4->echo_enable);
		if(vboxebf4->sp_sil==0 && vboxebf4->echo_enable)
		{
			for(i=0; i<nmicchannel; ++i)
			{
				fft[i][k]=erlstmp->out[i];
				ffts[k][i]=erlstmp->out[i];
				ffty[i][k]=erlstmp->lsty[i];
				fft2[k][i]=fft[i][k];
			}
		}else
		{
			for(i=0; i<nmicchannel; ++i)
			{
				ffts[k][i]=fft[i][k];
				ffty[i][k].a=ffty[i][k].b=0;
				fft2[k][i]=fft[i][k];
			}
		}
	}

	if(vboxebf4->cfg->use_epostsingle)
	{
		wtk_vboxebf4_aec_feed(vaec, fft[0], ffty[0], ffts, ffty2, 0, vboxebf4->denoise_enable?vboxebf4->cfg->gbias:1.0, vboxebf4->echo_enable?vboxebf4->sp_sil:1);
		for (k=1; k<nbin-1; ++k)
		{
			for(i=1; i<nmicchannel; ++i)
			{
				ffts[k][i].a *= gf[k];
				ffts[k][i].b *= gf[k];

				ffty2[k][i].a=fft[i][k].a-ffts[k][i].a;
				ffty2[k][i].b=fft[i][k].b-ffts[k][i].b;
			}
		}
		if(!vboxebf4->echo_enable || (vboxebf4->sp_sil==1 && vboxebf4->cfg->use_fftsbf)||(vboxebf4->sp_sil==0 && vboxebf4->cfg->use_efftsbf))
		{
			for (k=1; k<nbin-1; ++k)
			{
				gtmp=gf[k];
				for(i=0; i<nmicchannel; ++i)
				{
					fft[i][k].a *= gtmp;
					fft[i][k].b *= gtmp;
				}
			}
		}
	}else
	{
		for(i=0; i<nmicchannel; ++i, ++vaec)
		{
			wtk_vboxebf4_aec_feed(vaec, fft[i], ffty[i], ffts, ffty2, i, vboxebf4->denoise_enable?vboxebf4->cfg->gbias:1.0, vboxebf4->echo_enable?vboxebf4->sp_sil:1);
			if(!vboxebf4->echo_enable || (vboxebf4->sp_sil==1 && vboxebf4->cfg->use_fftsbf)||(vboxebf4->sp_sil==0 && vboxebf4->cfg->use_efftsbf))
			{
				gf=vaec->gf;
				for (k=1; k<nbin-1; ++k)
				{
					gtmp=gf[k];
					fft[i][k].a *= gtmp;
					fft[i][k].b *= gtmp;
				}
			}
		}
	}
	// printf("%d %d %d\n",pfs,vboxebf4->cfg->pframe_fe,fe_len);
	vboxebf4->pframe=(1-pframe_alpha)*vboxebf4->pframe+pframe_alpha*wtk_float_abs_mean(vboxebf4->vaec->gf+pfs, fe_len);

}

void wtk_vboxebf4_feed_bf(wtk_vboxebf4_t *vboxebf4, wtk_complex_t **fft, wtk_complex_t **ffts, wtk_complex_t **ffty)
// void wtk_vboxebf4_feed_bf(wtk_vboxebf4_t *vboxebf4, wtk_complex_t **ffts, wtk_complex_t **ffty)
{
	wtk_complex_t *fftx=vboxebf4->fftx;
	int k,nbin=vboxebf4->nbin;
	int i;
	int nbfchannel=vboxebf4->cfg->nbfchannel;
	wtk_bf_t *bf=vboxebf4->bf;
	wtk_covm_t *covm;
	int b;
	wtk_complex_t fft2[64];
	int clip_s=vboxebf4->cfg->clip_s;
	int clip_e=vboxebf4->cfg->clip_e;

	fftx[0].a=fftx[0].b=0;
	fftx[nbin-1].a=fftx[nbin-1].b=0;
	for(k=0; k<=clip_s; ++k)
	{
		fftx[k].a=fftx[k].b=0;
	}
	for(k=clip_e; k<nbin; ++k)
	{
		fftx[k].a=fftx[k].b=0;
	}
	if(vboxebf4->cfg->use_fixtheta)
	{
		for(k=clip_s+1; k<clip_e; ++k)
		{
			wtk_bf_output_fft_k(bf, ffts[k], fftx+k, k);
		}
	}else
	{
		if(vboxebf4->sp_sil==0 && vboxebf4->echo_enable)
		{
			bf->cfg->mu=vboxebf4->cfg->echo_bfmu;
			covm = vboxebf4->echo_covm==NULL? vboxebf4->covm: vboxebf4->echo_covm;
		}else
		{
			bf->cfg->mu=vboxebf4->cfg->bfmu;
			covm=vboxebf4->covm;
		}
		for(k=clip_s+1; k<clip_e; ++k)
		{
			for(i=0; i<nbfchannel; ++i)
			{
				fft2[i].a=fft[i][k].a;
				fft2[i].b=fft[i][k].b;
			}
			b=0;
			b=wtk_covm_feed_fft2(covm, ffty, k, 1);
			if(b==1)
			{
				wtk_bf_update_ncov(bf, covm->ncov, k);
			}
			if(covm->scov)
			{
				b=wtk_covm_feed_fft2(covm, ffts, k, 0);
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
		}
	}
}

void wtk_vboxebf4_feed_agc2(wtk_vboxebf4_t *vboxebf4, wtk_complex_t * fft)
{
	wtk_vboxebf4_aec_t *vaec=vboxebf4->vaec;
	int nbin=vaec->nbin;
	float *gf2=vaec->gf2;
	float gf;
	int i;
	float pframe_thresh=vboxebf4->cfg->pframe_thresh;

	if (vboxebf4->agc_enable &&(!vaec->bank_mic->silence && (vboxebf4->sp_sil==0 || vboxebf4->pframe>pframe_thresh)) )
	{
		gf=wtk_float_abs_mean(gf2+1, nbin-2);
		for (i=1;i<nbin-1;++i)
		{
			fft[i].a *= gf;//gf2[i];
			fft[i].b *= gf;//gf2[i];
		}
	}
}

void wtk_vboxebf4_feed_cnon(wtk_vboxebf4_t *vboxebf4, wtk_complex_t * fft)
{
	wtk_vboxebf4_aec_t *vaec=vboxebf4->vaec;
	int nbin=vaec->nbin;
	float *gf=vaec->gf;
	float sym=vboxebf4->cfg->sym;
	static float fx=2.0f*PI/RAND_MAX;
	float f,f2;
	int i;

	for(i=1;i<nbin;++i)
	{
		f=rand()*fx;
		f2=1.f-gf[i]*gf[i];
		if(f2>0)
		{
			// f2=sqrtf(f2);
			fft[i].a+=sym*cosf(f)*f2;
			fft[i].b+=sym*sinf(f)*f2;
		}
	}
}


void wtk_vboxebf4_on_ssl2(wtk_vboxebf4_t *vboxebf4, wtk_ssl2_extp_t *nbest_extp,int nbest, int ts,int te)
{
    if(vboxebf4->notify_ssl)
    {
        vboxebf4->notify_ssl(vboxebf4->ssl_ths, ts,te,nbest_extp, nbest);
    }
}


void wtk_vboxebf4_phaseo(wtk_vboxebf4_t *vboxebf4, wtk_complex_t *fftx)
{
	int nbin=vboxebf4->nbin;
	int k;

	for(k=0; k<nbin; ++k)
	{
		fftx[k].a=-fftx[k].a;
		fftx[k].b=-fftx[k].b;
	}
}

void wtk_vboxebf4_control_bs(wtk_vboxebf4_t *vboxebf4, float *out, int len)
{
	float out_max;
	int i;

	if(vboxebf4->mic_sil==0)
	{
		out_max=wtk_float_abs_max(out, len);
		if(out_max>32700.0)
		{
			vboxebf4->bs_scale=32700.0f/out_max;
			if(vboxebf4->bs_scale<vboxebf4->bs_last_scale)
			{
				vboxebf4->bs_last_scale=vboxebf4->bs_scale;
			}else
			{
				vboxebf4->bs_scale=vboxebf4->bs_last_scale;
			}
			vboxebf4->bs_max_cnt=5;
		}
		for(i=0; i<len; ++i)
		{
			out[i]*=vboxebf4->bs_scale;
		}
		if(vboxebf4->bs_max_cnt>0)
		{
			--vboxebf4->bs_max_cnt;
		}
		if(vboxebf4->bs_max_cnt<=0 && vboxebf4->bs_scale<1.0)
		{
			vboxebf4->bs_scale*=1.1f;
			vboxebf4->bs_last_scale=vboxebf4->bs_scale;
			if(vboxebf4->bs_scale>1.0)
			{
				vboxebf4->bs_scale=1.0;
				vboxebf4->bs_last_scale=1.0;
			}
		}
	}else
	{
		vboxebf4->bs_scale=1.0;
		vboxebf4->bs_last_scale=1.0;
		vboxebf4->bs_max_cnt=0;
	}
} 

void wtk_vboxebf4_feed(wtk_vboxebf4_t *vboxebf4,short *data,int len,int is_end)
{
	int i,j;
	int nbin=vboxebf4->nbin;
	int nmicchannel=vboxebf4->cfg->nmicchannel;
	int *mic_channel=vboxebf4->cfg->mic_channel;
	int nspchannel=vboxebf4->cfg->nspchannel;
	int *sp_channel=vboxebf4->cfg->sp_channel;
	int channel=vboxebf4->cfg->channel;
	wtk_strbuf_t **mic=vboxebf4->mic;
	wtk_strbuf_t **sp=vboxebf4->sp;
	float fv, *fp1, *fp2;
	int wins=vboxebf4->cfg->wins;
	int fsize=wins/2;
	int length;
	float spenr;
	float spenr_thresh=vboxebf4->cfg->spenr_thresh;
	int spenr_cnt=vboxebf4->cfg->spenr_cnt;
	float micenr;
	float micenr_thresh=vboxebf4->cfg->micenr_thresh;
	int micenr_cnt=vboxebf4->cfg->micenr_cnt;
	wtk_drft_t *rfft=vboxebf4->rfft;
	float *rfft_in=vboxebf4->rfft_in;
	wtk_complex_t **fft=vboxebf4->fft, **ffts=vboxebf4->ffts, **fft2=vboxebf4->fft2;
	wtk_complex_t **fft_sp=vboxebf4->fft_sp, **ffty2=vboxebf4->ffty2;
	float **analysis_mem=vboxebf4->analysis_mem, **analysis_mem_sp=vboxebf4->analysis_mem_sp;
	float *synthesis_mem=vboxebf4->synthesis_mem;
	float *analysis_window=vboxebf4->analysis_window, *synthesis_window=vboxebf4->synthesis_window;
	wtk_complex_t *fftx=vboxebf4->fftx;
	float *out=vboxebf4->out;
	short *pv=(short *)out;
	float mic_energy=0;
	float energy=0;
	float snr=0;

    float de_eng_sum=0;
    int de_clip_s=vboxebf4->cfg->de_clip_s;
    int de_clip_e=vboxebf4->cfg->de_clip_e;
    float de_thresh=vboxebf4->cfg->de_thresh;
    float de_alpha=vboxebf4->cfg->de_alpha;
    float de_alpha2;

	for(i=0;i<len;++i)
	{
		for(j=0; j<nmicchannel; ++j)
		{
			fv=data[mic_channel[j]];
			wtk_strbuf_push(mic[j],(char *)&(fv),sizeof(float));
		}
		for(j=0; j<nspchannel; ++j)
		{
			fv=data[sp_channel[j]];
			wtk_strbuf_push(sp[j],(char *)&(fv),sizeof(float));
		}
		data+=channel;
	}
	length=mic[0]->pos/sizeof(float);
	while(length>=fsize)
	{
		if(vboxebf4->notify_eng){
			mic_energy = sqrtf(wtk_vboxebf4_fft_energy(fft[0], nbin)*2);
		}
		for(i=0; i<nmicchannel; ++i)
		{
			fp1=(float *)mic[i]->data;
			wtk_drft_frame_analysis(rfft, rfft_in, analysis_mem[i], fft[i], fp1, wins, analysis_window);
			if(vboxebf4->agc_enable==0 && vboxebf4->inmic_scale>1.0)
			{
				for(j=0; j<nbin; ++j)
				{
					fft[i][j].a*= vboxebf4->inmic_scale;
					fft[i][j].b*= vboxebf4->inmic_scale;
				}
			}
		}
		for(i=0, j=nmicchannel; i<nspchannel; ++i, ++j)
		{
			fp2=(float *)sp[i]->data;
			wtk_drft_frame_analysis(rfft, rfft_in, analysis_mem_sp[i], fft_sp[i], fp2, wins, analysis_window);
		}
		if(nspchannel>0){
			spenr=wtk_vboxebf4_sp_energy((float *)sp[0]->data, fsize);
		}else{
			spenr=0;
		}
		// static int cnt=0;

		// cnt++;
		if(spenr>spenr_thresh)
		{
			// if(vboxebf4->sp_sil==1)
			// {
			// 	printf("sp start %f %f %f\n", 1.0/16000*cnt*(nbin-1),spenr,spenr_thresh);
			// }
			vboxebf4->sp_sil=0;
			vboxebf4->sp_silcnt=spenr_cnt;
		}else if(vboxebf4->sp_sil==0)
		{
			vboxebf4->sp_silcnt-=1;
			if(vboxebf4->sp_silcnt<=0)
			{
				// printf("sp end %f\n", 1.0/16000*cnt*(nbin-1));
				vboxebf4->sp_sil=1;
			}
		}
		wtk_vboxebf4_feed_echo(vboxebf4, fft, fft_sp);
		wtk_vboxebf4_feed_bf(vboxebf4, fft, ffts, ffty2);
		
		if(vboxebf4->notify_eng){
			energy = sqrtf(wtk_vboxebf4_fft_energy(fftx, nbin)*2);
			snr = 20 * log10(max(floor(energy / max(abs(mic_energy - energy), 1e-6)+0.5), 1.0));
			energy = 20 * log10(max(floor(energy+0.5), 1.0)) - 20 * log10(32768.0);

			// wtk_debug("%f %f\n", energy, snr);
			vboxebf4->notify_eng(vboxebf4->eng_ths, energy, snr);
		}

		if((vboxebf4->vaec->cfg->gainnet && vboxebf4->vaec->cfg->gainnet->use_agc) || (vboxebf4->vaec->cfg->gainnet6 && vboxebf4->vaec->cfg->gainnet6->use_agc))
		{
            wtk_vboxebf4_feed_agc2(vboxebf4, fftx);
		}
		// static int cnt=0;
		// cnt++;
		micenr=wtk_vboxebf4_fft_energy(fftx, nbin);
		if(micenr>micenr_thresh)
		{
			// if(vboxebf4->mic_sil==1)
			// {
			// 	printf("sp start %f %f %f\n", 1.0/16000*cnt*(nbin-1),micenr,micenr_thresh);
			// }
			vboxebf4->mic_sil=0;
			vboxebf4->mic_silcnt=micenr_cnt;

			if(vboxebf4->ssl2 && vboxebf4->ssl_enable)
			{
				wtk_ssl2_feed_fft2(vboxebf4->ssl2, ffts, 0);
			}
		}else if(vboxebf4->mic_sil==0)
		{
			vboxebf4->mic_silcnt-=1;
			if(vboxebf4->mic_silcnt<=0)
			{
				// printf("sp end %f\n", 1.0/16000*cnt*(nbin-1));
				vboxebf4->mic_sil=1;
				if(vboxebf4->ssl2 && vboxebf4->ssl_enable)
				{
					wtk_ssl2_feed_fft2(vboxebf4->ssl2, NULL, 1);
				}
			}else
			{
				if(vboxebf4->ssl2 && vboxebf4->ssl_enable)
				{
					wtk_ssl2_feed_fft2(vboxebf4->ssl2, ffts, 0);
				}
			}
		}
		if(vboxebf4->maskssl2 && vboxebf4->ssl_enable)
		{
			wtk_maskssl2_feed_fft(vboxebf4->maskssl2, fft2, vboxebf4->vaec->gf, vboxebf4->mic_sil);
		}else if(vboxebf4->maskssl && vboxebf4->ssl_enable)
		{
			wtk_maskssl_feed_fft(vboxebf4->maskssl, fft2, vboxebf4->vaec->gf, vboxebf4->mic_sil);
		}

		if(vboxebf4->cfg->use_cnon)
		{
			wtk_vboxebf4_feed_cnon(vboxebf4,fftx);
		}

		wtk_strbufs_pop(mic, nmicchannel, fsize*sizeof(float));
		wtk_strbufs_pop(sp, nspchannel, fsize*sizeof(float));
		length=mic[0]->pos/sizeof(float);

		if(vboxebf4->cfg->use_phaseo)
		{
			wtk_vboxebf4_phaseo(vboxebf4, fftx);
		}
		// for(k=1;k<nbin-1;++k)
		// {
		// 	fftx[k]=ffts[k][0];
		// }
		if(de_alpha!=1.0){
			for(i=de_clip_s;i<de_clip_e;++i){
				de_eng_sum+=fftx[i].a*fftx[i].a+fftx[i].b*fftx[i].b;
			}
			// printf("%f\n", logf(de_eng_sum+1e-9));
			// printf("%f\n", de_eng_sum);
			if(de_eng_sum>de_thresh){
				de_alpha2 = de_alpha * min(max(0.1, 5.0/logf(de_eng_sum+1e-9)), 1.0);
				for(i=de_clip_s;i<de_clip_e;++i){
					fftx[i].a*=de_alpha2;
					fftx[i].b*=de_alpha2;
				}
			}
		}

	    wtk_drft_frame_synthesis(rfft, rfft_in, synthesis_mem, fftx, out, wins, synthesis_window);
		if(vboxebf4->eq)
		{
			wtk_equalizer_feed_float(vboxebf4->eq, out, fsize);
		}
		wtk_vboxebf4_control_bs(vboxebf4, out, fsize);
		for(i=0; i<fsize; ++i)
		{
			pv[i]=floorf(out[i]+0.5);
		}
		if(vboxebf4->notify)
		{
			vboxebf4->notify(vboxebf4->ths,pv,fsize);
		}
	}
	if(is_end && length>0)
	{
		if(vboxebf4->notify)
		{
			pv=(short *)mic[0]->data;
			out=(float *)mic[0]->data;
			for(i=0; i<length; ++i)
			{
				pv[i]=floorf(out[i]+0.5);
			}
			vboxebf4->notify(vboxebf4->ths,pv,length);
		}
	}
}


void wtk_vboxebf4_set_micvolume(wtk_vboxebf4_t *vboxebf4,float fscale)
{
	vboxebf4->inmic_scale=fscale;
}

void wtk_vboxebf4_set_agcenable(wtk_vboxebf4_t *vboxebf4,int enable)
{
	vboxebf4->agc_enable=enable;
}

void wtk_vboxebf4_set_echoenable(wtk_vboxebf4_t *vboxebf4,int enable)
{
	vboxebf4->echo_enable=enable;
}

void wtk_vboxebf4_set_denoiseenable(wtk_vboxebf4_t *vboxebf4,int enable)
{
	vboxebf4->denoise_enable=enable;
}

void wtk_vboxebf4_ssl_delay_new(wtk_vboxebf4_t *vboxebf4)
{
	if(vboxebf4->cfg->use_ssl)
    {
        vboxebf4->ssl2=wtk_ssl2_new(&(vboxebf4->cfg->ssl2));
        wtk_ssl2_set_notify(vboxebf4->ssl2, vboxebf4, (wtk_ssl2_notify_f)wtk_vboxebf4_on_ssl2);
    }else if(vboxebf4->cfg->use_maskssl)
	{
		vboxebf4->maskssl=wtk_maskssl_new(&(vboxebf4->cfg->maskssl));
		wtk_maskssl_set_notify(vboxebf4->maskssl, vboxebf4, (wtk_maskssl_notify_f)wtk_vboxebf4_on_ssl2);

	}else if(vboxebf4->cfg->use_maskssl2)
	{
		vboxebf4->maskssl2=wtk_maskssl2_new(&(vboxebf4->cfg->maskssl2));
        wtk_maskssl2_set_notify(vboxebf4->maskssl2, vboxebf4, (wtk_maskssl2_notify_f)wtk_vboxebf4_on_ssl2);
	}
    if(vboxebf4->ssl2)
    {
        wtk_ssl2_reset(vboxebf4->ssl2);
    }
	if(vboxebf4->maskssl)
    {
        wtk_maskssl_reset(vboxebf4->maskssl);
    }
	if(vboxebf4->maskssl2)
    {
        wtk_maskssl2_reset(vboxebf4->maskssl2);
    }
	vboxebf4->ssl_enable = 1;
}
