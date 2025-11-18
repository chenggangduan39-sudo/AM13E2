#include "wtk_gainnet_ssl.h"
void wtk_gainnet_ssl_aec_on_gainnet(wtk_gainnet_ssl_edra_t *vdr, float *gain, int len, int is_end);
void wtk_gainnet_ssl_denoise_on_gainnet(wtk_gainnet_ssl_edra_t *vdr, float *gain, int len, int is_end);
void wtk_gainnet_ssl_agc_on_gainnet(wtk_gainnet_ssl_edra_t *vdr, float *gain, int len, int is_end);
void wtk_gainnet_ssl_on_ssl2(wtk_gainnet_ssl_t *gainnet_ssl, wtk_ssl2_extp_t *nbest_extp,int nbest, int ts,int te);

void wtk_gainnet_ssl_edra_init(wtk_gainnet_ssl_edra_t *vdr, wtk_gainnet_ssl_cfg_t *cfg)
{
	vdr->cfg=cfg;
	vdr->nbin=cfg->wins/2+1;

	vdr->bank_mic=wtk_bankfeat_new(&(cfg->bankfeat));
	vdr->bank_sp=wtk_bankfeat_new(&(cfg->bankfeat));

	vdr->g=wtk_malloc(sizeof(float)*cfg->bankfeat.nb_bands);
	vdr->lastg=wtk_malloc(sizeof(float)*cfg->bankfeat.nb_bands);
	vdr->gf=wtk_malloc(sizeof(float)*vdr->nbin);

	vdr->gainnet2=wtk_gainnet2_new(cfg->gainnet2);
	wtk_gainnet2_set_notify(vdr->gainnet2, vdr, (wtk_gainnet2_notify_f)wtk_gainnet_ssl_aec_on_gainnet);

	vdr->gainnet7=wtk_gainnet7_new(cfg->gainnet7);
	wtk_gainnet7_set_notify(vdr->gainnet7, vdr, (wtk_gainnet7_notify_f)wtk_gainnet_ssl_denoise_on_gainnet);

	vdr->qmmse=NULL;
	if(cfg->use_qmmse)
	{
		vdr->qmmse=wtk_qmmse_new(&(cfg->qmmse));
	}
}

void wtk_gainnet_ssl_edra_clean(wtk_gainnet_ssl_edra_t *vdr)
{
	wtk_bankfeat_delete(vdr->bank_mic);
	wtk_bankfeat_delete(vdr->bank_sp);

	wtk_free(vdr->g);
	wtk_free(vdr->lastg);
	wtk_free(vdr->gf);

	if(vdr->qmmse)
	{
		wtk_qmmse_delete(vdr->qmmse);
	}

	wtk_gainnet2_delete(vdr->gainnet2);
	wtk_gainnet7_delete(vdr->gainnet7);

}

void wtk_gainnet_ssl_edra_reset(wtk_gainnet_ssl_edra_t *vdr)
{
	wtk_bankfeat_reset(vdr->bank_mic);
	wtk_bankfeat_reset(vdr->bank_sp);

	memset(vdr->g, 0, sizeof(float)*vdr->cfg->bankfeat.nb_bands);
	memset(vdr->lastg, 0, sizeof(float)*vdr->cfg->bankfeat.nb_bands);
	memset(vdr->gf, 0, sizeof(float)*vdr->nbin);

	wtk_gainnet2_reset(vdr->gainnet2);
	wtk_gainnet7_reset(vdr->gainnet7);

	if(vdr->qmmse)
	{
		wtk_qmmse_reset(vdr->qmmse);
	}
}

wtk_gainnet_ssl_t* wtk_gainnet_ssl_new(wtk_gainnet_ssl_cfg_t *cfg)
{
	wtk_gainnet_ssl_t *gainnet_ssl;
	int i;

	gainnet_ssl=(wtk_gainnet_ssl_t *)wtk_malloc(sizeof(wtk_gainnet_ssl_t));
	gainnet_ssl->cfg=cfg;
	gainnet_ssl->ssl_ths=NULL;
    gainnet_ssl->notify_ssl=NULL;

	gainnet_ssl->mic=wtk_strbufs_new(gainnet_ssl->cfg->nmicchannel);
	gainnet_ssl->sp=wtk_strbufs_new(gainnet_ssl->cfg->nspchannel);

	gainnet_ssl->nbin=cfg->wins/2+1;
	gainnet_ssl->analysis_window=wtk_malloc(sizeof(float)*cfg->wins);///2);
	gainnet_ssl->analysis_mem=wtk_float_new_p2(cfg->nmicchannel, gainnet_ssl->nbin-1);
	gainnet_ssl->analysis_mem_sp=wtk_float_new_p2(cfg->nspchannel, gainnet_ssl->nbin-1);
	gainnet_ssl->rfft=wtk_drft_new(cfg->wins);
	gainnet_ssl->rfft_in=(float*)wtk_malloc(sizeof(float)*(cfg->wins));

	gainnet_ssl->fft=wtk_complex_new_p2(cfg->nmicchannel, gainnet_ssl->nbin);
	gainnet_ssl->fft_sp=wtk_complex_new_p2(cfg->nspchannel, gainnet_ssl->nbin);

    gainnet_ssl->erls=wtk_malloc(sizeof(wtk_rls_t)*(gainnet_ssl->nbin));
    for(i=0;i<gainnet_ssl->nbin;++i)
    {
      wtk_rls_init(gainnet_ssl->erls+i, &(cfg->echo_rls));
    }
	gainnet_ssl->fftx=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*gainnet_ssl->nbin);
	gainnet_ssl->ffts=wtk_complex_new_p2(gainnet_ssl->nbin, cfg->nmicchannel);
	gainnet_ssl->ffty=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*gainnet_ssl->nbin);

	gainnet_ssl->vdr=(wtk_gainnet_ssl_edra_t *)wtk_malloc(sizeof(wtk_gainnet_ssl_edra_t));
	wtk_gainnet_ssl_edra_init(gainnet_ssl->vdr, cfg);

	gainnet_ssl->maskssl=NULL;
	gainnet_ssl->maskssl2=NULL;
	if(cfg->use_maskssl)
	{
		gainnet_ssl->maskssl=wtk_maskssl_new(&(cfg->maskssl));
        wtk_maskssl_set_notify(gainnet_ssl->maskssl, gainnet_ssl, (wtk_maskssl_notify_f)wtk_gainnet_ssl_on_ssl2);
	}else if(cfg->use_maskssl2)
	{
		gainnet_ssl->maskssl2=wtk_maskssl2_new(&(cfg->maskssl2));
        wtk_maskssl2_set_notify(gainnet_ssl->maskssl2, gainnet_ssl, (wtk_maskssl2_notify_f)wtk_gainnet_ssl_on_ssl2);
	}

	wtk_gainnet_ssl_reset(gainnet_ssl);

	return gainnet_ssl;
}

void wtk_gainnet_ssl_delete(wtk_gainnet_ssl_t *gainnet_ssl)
{
	int i;

	wtk_strbufs_delete(gainnet_ssl->mic,gainnet_ssl->cfg->nmicchannel);
	wtk_strbufs_delete(gainnet_ssl->sp,gainnet_ssl->cfg->nspchannel);

	wtk_free(gainnet_ssl->analysis_window);
	wtk_float_delete_p2(gainnet_ssl->analysis_mem, gainnet_ssl->cfg->nmicchannel);
	wtk_float_delete_p2(gainnet_ssl->analysis_mem_sp, gainnet_ssl->cfg->nspchannel);
	wtk_free(gainnet_ssl->rfft_in);
	wtk_drft_delete(gainnet_ssl->rfft);
	wtk_complex_delete_p2(gainnet_ssl->fft, gainnet_ssl->cfg->nmicchannel);
	wtk_complex_delete_p2(gainnet_ssl->fft_sp, gainnet_ssl->cfg->nspchannel);

	for(i=0;i<gainnet_ssl->nbin;++i)
	{
		wtk_rls_clean(gainnet_ssl->erls+i);
	}
	wtk_free(gainnet_ssl->erls);

	wtk_free(gainnet_ssl->fftx);
	wtk_free(gainnet_ssl->ffty);
	wtk_complex_delete_p2(gainnet_ssl->ffts, gainnet_ssl->nbin);

	wtk_gainnet_ssl_edra_clean(gainnet_ssl->vdr);
	wtk_free(gainnet_ssl->vdr);

    if(gainnet_ssl->maskssl)
    {
        wtk_maskssl_delete(gainnet_ssl->maskssl);
    }
	if(gainnet_ssl->maskssl2)
    {
        wtk_maskssl2_delete(gainnet_ssl->maskssl2);
    }

	wtk_free(gainnet_ssl);
}


void wtk_gainnet_ssl_start(wtk_gainnet_ssl_t *gainnet_ssl)
{
}

void wtk_gainnet_ssl_reset(wtk_gainnet_ssl_t *gainnet_ssl)
{
	int wins=gainnet_ssl->cfg->wins;
	int i,nbin=gainnet_ssl->nbin;

	wtk_strbufs_reset(gainnet_ssl->mic,gainnet_ssl->cfg->nmicchannel);
	wtk_strbufs_reset(gainnet_ssl->sp,gainnet_ssl->cfg->nspchannel);

	for (i=0;i<wins;++i)
	{
		gainnet_ssl->analysis_window[i] = sin((0.5+i)*PI/(wins));
	}

	wtk_float_zero_p2(gainnet_ssl->analysis_mem, gainnet_ssl->cfg->nmicchannel, (gainnet_ssl->nbin-1));
	wtk_float_zero_p2(gainnet_ssl->analysis_mem_sp, gainnet_ssl->cfg->nspchannel, (gainnet_ssl->nbin-1));

	wtk_complex_zero_p2(gainnet_ssl->fft, gainnet_ssl->cfg->nmicchannel, gainnet_ssl->nbin);
	wtk_complex_zero_p2(gainnet_ssl->fft_sp, gainnet_ssl->cfg->nspchannel, gainnet_ssl->nbin);

	for(i=0;i<nbin;++i)
    {
		wtk_rls_reset(gainnet_ssl->erls+i);
	}
	wtk_complex_zero_p2(gainnet_ssl->ffts, gainnet_ssl->nbin, gainnet_ssl->cfg->nmicchannel);
	memset(gainnet_ssl->ffty, 0, sizeof(wtk_complex_t)*(gainnet_ssl->nbin));
	memset(gainnet_ssl->fftx, 0, sizeof(wtk_complex_t)*(gainnet_ssl->nbin));

	wtk_gainnet_ssl_edra_reset(gainnet_ssl->vdr);

	gainnet_ssl->sp_silcnt=0;
	gainnet_ssl->sp_sil=1;

	gainnet_ssl->mic_silcnt=0;
	gainnet_ssl->mic_sil=1;

	gainnet_ssl->pframe=0;

	if(gainnet_ssl->maskssl)
    {
        wtk_maskssl_reset(gainnet_ssl->maskssl);
    }
	if(gainnet_ssl->maskssl2)
    {
        wtk_maskssl2_reset(gainnet_ssl->maskssl2);
    }

	gainnet_ssl->echo_enable=1;
}

void wtk_gainnet_ssl_set_ssl_notify(wtk_gainnet_ssl_t *gainnet_ssl,void *ths,wtk_gainnet_ssl_notify_ssl_f notify)
{
	gainnet_ssl->notify_ssl=notify;
	gainnet_ssl->ssl_ths=ths;
}

static float wtk_gainnet_ssl_sp_energy(short *p,int n)
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

static float wtk_gainnet_ssl_fft_energy(wtk_complex_t *fftx,int nbin)
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

void wtk_gainnet_ssl_denoise_on_gainnet(wtk_gainnet_ssl_edra_t *vdr, float *gain, int len, int is_end)
{
	memcpy(vdr->g, gain, sizeof(float)*vdr->cfg->bankfeat.nb_bands);
}

void wtk_gainnet_ssl_aec_on_gainnet(wtk_gainnet_ssl_edra_t *vdr, float *gain, int len, int is_end)
{
	memcpy(vdr->g, gain, sizeof(float)*vdr->cfg->bankfeat.nb_bands);
}

void wtk_gainnet_ssl_edra_feed(wtk_gainnet_ssl_edra_t *vdr, wtk_complex_t *fftx, wtk_complex_t *ffty, int sp_sil)
{
	int i;
	int nbin=vdr->nbin;
	float *g=vdr->g, *gf=vdr->gf;
	float *lastg=vdr->lastg;
	float ralpha=vdr->cfg->ralpha;
	wtk_gainnet2_t *gainnet2=vdr->gainnet2;
	wtk_gainnet7_t *gainnet7=vdr->gainnet7;
	wtk_bankfeat_t *bank_mic=vdr->bank_mic;
	wtk_bankfeat_t *bank_sp=vdr->bank_sp;
	int nb_bands=bank_mic->cfg->nb_bands;
	int nb_features=bank_mic->cfg->nb_features;
	wtk_qmmse_t *qmmse=vdr->qmmse;
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
	if(qmmse)
	{
		wtk_qmmse_flush_mask(qmmse, fftx, gf);
	}
	if(sp_sil)
	{
		wtk_gainnet7_feed(gainnet7, bank_mic->features, nb_features, 0); 
	}else
	{
		wtk_gainnet2_feed2(gainnet2, bank_mic->features, nb_features, bank_sp->features, nb_features, 0);  
	}
	for(i=0; i<nb_bands; ++i)
	{
		g[i]=max(g[i],lastg[i]*ralpha);
		lastg[i]=g[i];
	}
	wtk_bankfeat_interp_band_gain(bank_mic, nbin, gf, g);
	if(qmmse)
	{	
		qmmse_gain=qmmse->gain;
		for (i=1; i<nbin-1; ++i)
		{
			if(gf[i]>qmmse_gain[i] )
			{
				gf[i]=qmmse_gain[i];
			}
		}
	}
}


void wtk_gainnet_ssl_feed_edra(wtk_gainnet_ssl_t *gainnet_ssl, wtk_complex_t **fft, wtk_complex_t **fft_sp)
{
	int nmicchannel=gainnet_ssl->cfg->nmicchannel;
	int nspchannel=gainnet_ssl->cfg->nspchannel;
	int i,j,k;
	int nbin=gainnet_ssl->nbin;
	wtk_rls_t *erls=gainnet_ssl->erls, *erlstmp;
	wtk_complex_t *fftx=gainnet_ssl->fftx;
	wtk_complex_t ffttmp[64]={0};
	wtk_complex_t fftsp2[10]={0};
	wtk_complex_t *ffty=gainnet_ssl->ffty;
	wtk_gainnet_ssl_edra_t *vdr=gainnet_ssl->vdr;
	float pframe_alpha=gainnet_ssl->cfg->pframe_alpha;
	int pfs=gainnet_ssl->cfg->pframe_fs;
	int fe_len=gainnet_ssl->cfg->pframe_fe-gainnet_ssl->cfg->pframe_fs;

	erlstmp=erls;
	for(k=0; k<nbin; ++k, ++erlstmp)
	{
		if(gainnet_ssl->cfg->use_erlssingle && gainnet_ssl->cfg->use_firstds)
		{
			ffttmp[0].a=ffttmp[0].b=0;
			for(i=0; i<nmicchannel; ++i)
			{
				ffttmp[0].a+=fft[i][k].a;
				ffttmp[0].b+=fft[i][k].b;	
			}
			ffttmp[0].a/=nmicchannel;
			ffttmp[0].b/=nmicchannel;
		}else
		{
			for(i=0; i<nmicchannel; ++i)
			{
				ffttmp[i]=fft[i][k];
			}
		}
		for(j=0; j<nspchannel; ++j)
		{
			fftsp2[j]=fft_sp[j][k];
		}
		wtk_rls_feed3(erlstmp, ffttmp, fftsp2, gainnet_ssl->sp_sil==0 && gainnet_ssl->echo_enable);
		if(gainnet_ssl->sp_sil==0 && gainnet_ssl->echo_enable)
		{
			ffty[k]=erlstmp->lsty[0];
			if(gainnet_ssl->cfg->use_erlssingle)
			{
				fftx[k]=erlstmp->out[0];
				for(i=0; i<nmicchannel; ++i)
				{
					fft[i][k].a-=erlstmp->lsty[0].a;
					fft[i][k].b-=erlstmp->lsty[0].b;
				}
			}else
			{
				for(i=0; i<nmicchannel; ++i)
				{
					fft[i][k]=erlstmp->out[i];
				}
			}
		}else
		{
			fftx[k]=ffttmp[0];
			ffty[k].a=ffty[k].b=0;
		}
	}

	wtk_gainnet_ssl_edra_feed(vdr, fftx, ffty, gainnet_ssl->echo_enable?gainnet_ssl->sp_sil:1);

	gainnet_ssl->pframe=(1-pframe_alpha)*gainnet_ssl->pframe+pframe_alpha*wtk_float_abs_mean(gainnet_ssl->vdr->gf+pfs, fe_len);
}

void wtk_gainnet_ssl_on_ssl2(wtk_gainnet_ssl_t *gainnet_ssl, wtk_ssl2_extp_t *nbest_extp,int nbest, int ts,int te)
{
	// printf("[%d %d] ==> %d %d %f\n", ts,te, nbest_extp[0].theta, nbest_extp[0].phi, nbest_extp[0].nspecsum);
    if(gainnet_ssl->notify_ssl)
    {
        gainnet_ssl->notify_ssl(gainnet_ssl->ssl_ths, ts, te, nbest_extp, nbest);
    }
}

void wtk_gainnet_ssl_feed(wtk_gainnet_ssl_t *gainnet_ssl,short *data,int len,int is_end)
{
	int i,j,k;
	int nbin=gainnet_ssl->nbin;
	int nmicchannel=gainnet_ssl->cfg->nmicchannel;
	int *mic_channel=gainnet_ssl->cfg->mic_channel;
	int nspchannel=gainnet_ssl->cfg->nspchannel;
	int *sp_channel=gainnet_ssl->cfg->sp_channel;
	int channel=gainnet_ssl->cfg->channel;
	wtk_strbuf_t **mic=gainnet_ssl->mic;
	wtk_strbuf_t **sp=gainnet_ssl->sp;
	int wins=gainnet_ssl->cfg->wins;
	int fsize=wins/2;
	int length;
	float spenr;
	float spenr_thresh=gainnet_ssl->cfg->spenr_thresh;
	int spenr_cnt=gainnet_ssl->cfg->spenr_cnt;
	float micenr;
	float micenr_thresh=gainnet_ssl->cfg->micenr_thresh;
	int micenr_cnt=gainnet_ssl->cfg->micenr_cnt;
	wtk_drft_t *rfft=gainnet_ssl->rfft;
	float *rfft_in=gainnet_ssl->rfft_in;
	wtk_complex_t **fft=gainnet_ssl->fft, **ffts=gainnet_ssl->ffts;
	wtk_complex_t **fft_sp=gainnet_ssl->fft_sp;
	float **analysis_mem=gainnet_ssl->analysis_mem, **analysis_mem_sp=gainnet_ssl->analysis_mem_sp;
	float *analysis_window=gainnet_ssl->analysis_window;

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
		for(i=0, j=nmicchannel; i<nspchannel; ++i, ++j)
		{
			wtk_drft_frame_analysis2(rfft, rfft_in, analysis_mem_sp[i], fft_sp[i], (short *)(sp[i]->data), wins, analysis_window);
		}
		if(nspchannel>0){
			spenr=wtk_gainnet_ssl_sp_energy((short *)sp[0]->data, fsize);
		}else{
			spenr=0;
		}
		// static int cnt=0;

		// cnt++;
		if(spenr>spenr_thresh)
		{
			// if(gainnet_ssl->sp_sil==1)
			// {
			// 	printf("sp start %f %f %f\n", 1.0/16000*cnt*(nbin-1),spenr,spenr_thresh);
			// }
			gainnet_ssl->sp_sil=0;
			gainnet_ssl->sp_silcnt=spenr_cnt;
		}else if(gainnet_ssl->sp_sil==0)
		{
			gainnet_ssl->sp_silcnt-=1;
			if(gainnet_ssl->sp_silcnt<=0)
			{
				// printf("sp end %f\n", 1.0/16000*cnt*(nbin-1));
				gainnet_ssl->sp_sil=1;
			}
		}
		wtk_gainnet_ssl_feed_edra(gainnet_ssl, fft, fft_sp);
		
		float gf;
		for(k=1; k<nbin-1; ++k)
		{
			gf=gainnet_ssl->vdr->gf[k];
			for(i=0; i<nmicchannel; ++i)
			{
				ffts[k][i].a=fft[i][k].a;
				ffts[k][i].b=fft[i][k].b;
				fft[i][k].a=fft[i][k].a*gf;
				fft[i][k].b=fft[i][k].b*gf;
			}
		}
		// static int cnt=0;
		// cnt++;
		micenr=wtk_gainnet_ssl_fft_energy(fft[0], nbin);
		if(micenr>micenr_thresh)
		{
			// if(gainnet_ssl->mic_sil==1)
			// {
			// 	printf("sp start %f %f %f\n", 1.0/16000*cnt*(nbin-1),micenr,micenr_thresh);
			// }
			gainnet_ssl->mic_sil=0;
			gainnet_ssl->mic_silcnt=micenr_cnt;
		}else if(gainnet_ssl->mic_sil==0)
		{
			gainnet_ssl->mic_silcnt-=1;
			if(gainnet_ssl->mic_silcnt<=0)
			{
				// printf("sp end %f\n", 1.0/16000*cnt*(nbin-1));
				gainnet_ssl->mic_sil=1;
			}
		}

		if(gainnet_ssl->maskssl2)
		{
			wtk_maskssl2_feed_fft(gainnet_ssl->maskssl2, ffts, gainnet_ssl->vdr->gf, gainnet_ssl->mic_sil);
		}else if(gainnet_ssl->maskssl)
		{
			wtk_maskssl_feed_fft(gainnet_ssl->maskssl, ffts, gainnet_ssl->vdr->gf, gainnet_ssl->mic_sil);
		}

		wtk_strbufs_pop(mic, nmicchannel, fsize*sizeof(short));
		wtk_strbufs_pop(sp, nspchannel, fsize*sizeof(short));
		length=mic[0]->pos/sizeof(short);
	}
}

void wtk_gainnet_ssl_set_echoenable(wtk_gainnet_ssl_t *gainnet_ssl,int enable)
{
	gainnet_ssl->echo_enable=enable;
}