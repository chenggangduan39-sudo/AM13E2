#include "wtk_gainnet_aec5.h"
void wtk_gainnet_aec5_aec_on_gainnet(wtk_gainnet_single_aec5_t *gsaec, float *gain, int len, int is_end);
void wtk_gainnet_aec5_aec_on_gainnet2(wtk_gainnet_single_aec5_t *gsaec, float *gain, int len, int is_end);


void wtk_gainnet_aec5_aec_init(wtk_gainnet_single_aec5_t *gsaec, wtk_gainnet_aec5_cfg_t *cfg)
{
	gsaec->cfg=cfg;
	gsaec->nbin=cfg->wins/2+1;

	gsaec->bank_mic=wtk_bankfeat_new(&(cfg->bankfeat));
	gsaec->bank_sp=wtk_bankfeat_new(&(cfg->bankfeat));

	gsaec->lastg=wtk_malloc(sizeof(float)*cfg->bankfeat.nb_bands);
	gsaec->g=wtk_malloc(sizeof(float)*cfg->bankfeat.nb_bands);
	gsaec->gf=wtk_malloc(sizeof(float)*gsaec->nbin);

	gsaec->g2=wtk_malloc(sizeof(float)*cfg->bankfeat.nb_bands);
	gsaec->gf2=wtk_malloc(sizeof(float)*gsaec->nbin);

	gsaec->gainnet=NULL;
	gsaec->gainnet2=NULL;
	gsaec->gainnet6=NULL;
	gsaec->agc_on=0;
	if(cfg->use_gainnet2)
	{
		gsaec->gainnet2=wtk_gainnet2_new(cfg->gainnet2);
		wtk_gainnet2_set_notify(gsaec->gainnet2, gsaec, (wtk_gainnet2_notify_f)wtk_gainnet_aec5_aec_on_gainnet);
		wtk_gainnet2_set_notify2(gsaec->gainnet2, gsaec, (wtk_gainnet2_notify_f2)wtk_gainnet_aec5_aec_on_gainnet2);
		if(cfg->gainnet2->use_agc)
		{
			gsaec->agc_on=1;
		}
	}else if(cfg->use_gainnet6)
	{
		gsaec->gainnet6=wtk_gainnet6_new(cfg->gainnet6);
		wtk_gainnet6_set_notify(gsaec->gainnet6, gsaec, (wtk_gainnet6_notify_f)wtk_gainnet_aec5_aec_on_gainnet);
		wtk_gainnet6_set_notify2(gsaec->gainnet6, gsaec, (wtk_gainnet6_notify_f2)wtk_gainnet_aec5_aec_on_gainnet2);
		if(cfg->gainnet6->use_agc)
		{
			gsaec->agc_on=1;
		}
	}else
	{
		gsaec->gainnet=wtk_gainnet5_new(cfg->gainnet);
		wtk_gainnet5_set_notify(gsaec->gainnet, gsaec, (wtk_gainnet5_notify_f)wtk_gainnet_aec5_aec_on_gainnet);
		wtk_gainnet5_set_notify2(gsaec->gainnet, gsaec, (wtk_gainnet5_notify_f2)wtk_gainnet_aec5_aec_on_gainnet2);
		if(cfg->gainnet->use_agc)
		{
			gsaec->agc_on=1;
		}
	}

	gsaec->qmmse=NULL;
	if(cfg->use_qmmse)
	{
		gsaec->qmmse=wtk_qmmse_new(&(cfg->qmmse));
	}
}

void wtk_gainnet_aec5_aec_clean(wtk_gainnet_single_aec5_t *gsaec)
{
	wtk_bankfeat_delete(gsaec->bank_mic);
	wtk_bankfeat_delete(gsaec->bank_sp);

	wtk_free(gsaec->lastg);
	wtk_free(gsaec->g);
	wtk_free(gsaec->gf);
	wtk_free(gsaec->g2);
	wtk_free(gsaec->gf2);

	if(gsaec->qmmse)
	{
		wtk_qmmse_delete(gsaec->qmmse);
	}
	if(gsaec->gainnet)
	{
		wtk_gainnet5_delete(gsaec->gainnet);
	}else if(gsaec->gainnet2)
	{
		wtk_gainnet2_delete(gsaec->gainnet2);
	}else if(gsaec->gainnet6)
	{
		wtk_gainnet6_delete(gsaec->gainnet6);
	}
}

void wtk_gainnet_aec5_aec_reset(wtk_gainnet_single_aec5_t *gsaec)
{
	wtk_bankfeat_reset(gsaec->bank_mic);
	wtk_bankfeat_reset(gsaec->bank_sp);

	memset(gsaec->lastg, 0, sizeof(float)*gsaec->cfg->bankfeat.nb_bands);
	memset(gsaec->g, 0, sizeof(float)*gsaec->cfg->bankfeat.nb_bands);
	memset(gsaec->gf, 0, sizeof(float)*gsaec->nbin);
	memset(gsaec->g2, 0, sizeof(float)*gsaec->cfg->bankfeat.nb_bands);
	memset(gsaec->gf2, 0, sizeof(float)*gsaec->nbin);
	gsaec->gff2=0;

	if(gsaec->gainnet)
	{
		wtk_gainnet5_reset(gsaec->gainnet);
	}else if(gsaec->gainnet2)
	{
		wtk_gainnet2_reset(gsaec->gainnet2);
	}else if(gsaec->gainnet6)
	{
		wtk_gainnet6_reset(gsaec->gainnet6);
	}
	if(gsaec->qmmse)
	{
		wtk_qmmse_reset(gsaec->qmmse);
	}
}

wtk_gainnet_aec5_nlmsdelay_t* wtk_gainnet_aec5_nlmsdelay_new(wtk_gainnet_aec5_cfg_t *cfg)
{
	wtk_gainnet_aec5_nlmsdelay_t *gnlmsd;

	gnlmsd=(wtk_gainnet_aec5_nlmsdelay_t *)wtk_malloc(sizeof(wtk_gainnet_aec5_nlmsdelay_t));
	gnlmsd->cfg=cfg;
	gnlmsd->nbin=cfg->wins/2+1;
    gnlmsd->W=(wtk_complex_t *)wtk_malloc(gnlmsd->nbin*cfg->M*sizeof(wtk_complex_t));
    gnlmsd->X=wtk_complex_new_p2(cfg->nspchannel,gnlmsd->nbin*cfg->M);
    gnlmsd->Y=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*gnlmsd->nbin);
	gnlmsd->E=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*gnlmsd->nbin);
	gnlmsd->power_x=(float *)wtk_malloc(sizeof(float)*gnlmsd->nbin);

	return gnlmsd;
}

void wtk_gainnet_aec5_nlmsdelay_delete(wtk_gainnet_aec5_nlmsdelay_t *gnlmsd)
{
	wtk_free(gnlmsd->W);
	wtk_complex_delete_p2(gnlmsd->X, gnlmsd->cfg->nspchannel);
	wtk_free(gnlmsd->Y);
	wtk_free(gnlmsd->E);
	wtk_free(gnlmsd->power_x);

	wtk_free(gnlmsd);
}

void wtk_gainnet_aec5_nlmsdelay_reset(wtk_gainnet_aec5_nlmsdelay_t *gnlmsd)
{
	memset(gnlmsd->W, 0,  sizeof(wtk_complex_t)*gnlmsd->nbin*gnlmsd->cfg->M);
	wtk_complex_zero_p2(gnlmsd->X, gnlmsd->cfg->nspchannel, gnlmsd->nbin*gnlmsd->cfg->M);
	memset(gnlmsd->power_x, 0,  sizeof(float)*gnlmsd->nbin);
}

void wtk_gainnet_aec5_nlmsdelay_feed(wtk_gainnet_aec5_nlmsdelay_t *gnlmsd, wtk_complex_t *fft, wtk_complex_t **fft_sp, int sp_sil)
{
	float *power_x=gnlmsd->power_x;
	float power_alpha=gnlmsd->cfg->power_alpha;
	int nspchannel=gnlmsd->cfg->nspchannel;
	wtk_complex_t **X=gnlmsd->X;
	wtk_complex_t *W=gnlmsd->W;
	wtk_complex_t  *fft1, *Xtmp, *Wtmp, *Ytmp, *Etmp;
	wtk_complex_t *E=gnlmsd->E;
    wtk_complex_t *Y=gnlmsd->Y;
    int nbin=gnlmsd->nbin;
    int M=gnlmsd->cfg->M;
    int i,k;
	float r,f,max_w;
	int idx;
	static int old_idx=0;
	static int stable_nf=0;
	int nlms_nframe=gnlmsd->cfg->nlms_nframe;
	float mufb=gnlmsd->cfg->mufb;

	for(i=0;i<nspchannel;++i)
	{
		memmove(X[i]+nbin,X[i],sizeof(wtk_complex_t)*(M-1)*nbin);
		memcpy(X[i], fft_sp[i], sizeof(wtk_complex_t)*nbin);
	}
	
	if(stable_nf<nlms_nframe && sp_sil==0)
	{
		++stable_nf;
		for(k=0, fft1=fft_sp[0]; k<nbin; ++k,++fft1)
		{
			power_x[k]=(1-power_alpha)*power_x[k]+power_alpha*(fft1->a*fft1->a+fft1->b*fft1->b);
		}

		memset(Y, 0, sizeof(wtk_complex_t)*gnlmsd->nbin);
		for(i=0, Xtmp=X[0], Wtmp=W; i<M; ++i)
		{
			for(k=0, Ytmp=Y; k<nbin; ++k,++Xtmp,++Wtmp,++Ytmp)
			{
				Ytmp->a+=Xtmp->a*Wtmp->a-Xtmp->b*Wtmp->b;
				Ytmp->b+=Xtmp->a*Wtmp->b+Xtmp->b*Wtmp->a;
			}
		}

		for(k=0, Etmp=E, Ytmp=Y, fft1=fft; k<nbin; ++k, ++Etmp, ++Ytmp,++fft1)
		{
			Etmp->a=fft1->a-Ytmp->a;
			Etmp->b=fft1->b-Ytmp->b;
		}

		Wtmp=W;
		Xtmp=X[0];
		max_w=0;
		idx=0;
		for(i=0; i<M; ++i)
		{
			Etmp=E;
			f=1e-9;
			for(k=0; k<nbin; ++k, ++Wtmp, ++Etmp, ++Xtmp)
			{
				r=mufb/(M*power_x[k]+1e-9);
				Wtmp->a+=r*(Xtmp->a*Etmp->a+Xtmp->b*Etmp->b);
				Wtmp->b+=r*(-Xtmp->b*Etmp->a+Xtmp->a*Etmp->b);
				f+=Wtmp->a*Wtmp->a+Wtmp->b*Wtmp->b;
			}
			if(f>max_w)
			{
				max_w=f;
				idx=i;
			}
		}
		if(idx>=1)
		{
			idx-=1;
		}
		if(stable_nf == nlms_nframe && old_idx!=idx)
		{
			stable_nf-= nlms_nframe/4;
		}
		old_idx=idx;
	}else if(stable_nf>=nlms_nframe)
	{
		// printf("%d\n",old_idx);
		for(i=0;i<nspchannel;++i)
		{
			memcpy(fft_sp[i], X[i]+nbin*old_idx, sizeof(wtk_complex_t)*nbin);
		}
	}
}


wtk_gainnet_aec5_t* wtk_gainnet_aec5_new(wtk_gainnet_aec5_cfg_t *cfg)
{
	wtk_gainnet_aec5_t *gainnet_aec5;
	int i;

	gainnet_aec5=(wtk_gainnet_aec5_t *)wtk_malloc(sizeof(wtk_gainnet_aec5_t));
	gainnet_aec5->cfg=cfg;
	gainnet_aec5->ths=NULL;
	gainnet_aec5->notify=NULL;
	gainnet_aec5->ths_tr=NULL;
	gainnet_aec5->notify_tr=NULL;

	gainnet_aec5->channel=cfg->nmicchannel+cfg->nspchannel;

	gainnet_aec5->mic=wtk_strbufs_new(gainnet_aec5->cfg->nmicchannel);
	gainnet_aec5->sp=wtk_strbufs_new(gainnet_aec5->cfg->nspchannel);

	gainnet_aec5->notch_mem=NULL;
	gainnet_aec5->memD=NULL;
	if(cfg->use_preemph)
	{
		gainnet_aec5->notch_mem=wtk_float_new_p2(gainnet_aec5->channel,2);
		gainnet_aec5->memD=(float *)wtk_malloc(sizeof(float)*gainnet_aec5->channel);
		gainnet_aec5->memX=(float *)wtk_malloc(sizeof(float)*gainnet_aec5->cfg->nmicchannel);
	}

	gainnet_aec5->nbin=cfg->wins/2+1;
	gainnet_aec5->analysis_window=wtk_malloc(sizeof(float)*cfg->wins);
	gainnet_aec5->synthesis_window=wtk_malloc(sizeof(float)*cfg->wins);
	gainnet_aec5->analysis_mem=wtk_float_new_p2(cfg->nmicchannel, gainnet_aec5->nbin-1);
	gainnet_aec5->analysis_mem_sp=wtk_float_new_p2(cfg->nspchannel, gainnet_aec5->nbin-1);
	gainnet_aec5->synthesis_mem=wtk_float_new_p2(cfg->nmicchannel, gainnet_aec5->nbin-1);
	gainnet_aec5->rfft=wtk_drft_new(cfg->wins);
	gainnet_aec5->rfft_in=(float*)wtk_malloc(sizeof(float)*(cfg->wins));

	gainnet_aec5->fft=wtk_complex_new_p2(cfg->nmicchannel, gainnet_aec5->nbin);
	gainnet_aec5->fft_sp=wtk_complex_new_p2(cfg->nspchannel, gainnet_aec5->nbin);

	gainnet_aec5->gnlmsd=NULL;
	if(cfg->use_nlmsdelay)
	{
		gainnet_aec5->gnlmsd=wtk_gainnet_aec5_nlmsdelay_new(cfg);
	}
	gainnet_aec5->erls=NULL;
	gainnet_aec5->enlms=NULL;
	gainnet_aec5->ekalman=NULL;
	if(cfg->use_nlms)//nlms
	{
		gainnet_aec5->enlms=wtk_malloc(sizeof(wtk_nlms_t)*(gainnet_aec5->nbin));
		for(i=0;i<gainnet_aec5->nbin;++i)
		{
			wtk_nlms_init(gainnet_aec5->enlms+i, &(cfg->echo_nlms));
		}
	}else if(cfg->use_rls)//rls
	{
		gainnet_aec5->erls=wtk_malloc(sizeof(wtk_rls_t)*(gainnet_aec5->nbin));
		for(i=0;i<gainnet_aec5->nbin;++i)
		{
			wtk_rls_init(gainnet_aec5->erls+i, &(cfg->echo_rls));
		}
	}else if(cfg->use_kalman)//kalman
	{
		int batchsize=1;
		gainnet_aec5->ekalman=qtk_kalman_new(&(cfg->echo_kalman), batchsize, gainnet_aec5->nbin);
	}
	gainnet_aec5->ffty=wtk_complex_new_p2(cfg->nmicchannel, gainnet_aec5->nbin);

	if(cfg->use_epostsingle)
	{
		gainnet_aec5->gsaec=(wtk_gainnet_single_aec5_t *)wtk_malloc(sizeof(wtk_gainnet_single_aec5_t));
		wtk_gainnet_aec5_aec_init(gainnet_aec5->gsaec, cfg);
	}else
	{
		gainnet_aec5->gsaec=(wtk_gainnet_single_aec5_t *)wtk_malloc(sizeof(wtk_gainnet_single_aec5_t)*cfg->nmicchannel);
		for(i=0; i<cfg->nmicchannel; ++i)
		{
			wtk_gainnet_aec5_aec_init(gainnet_aec5->gsaec+i, cfg);
		}
	}

	gainnet_aec5->out=wtk_float_new_p2(cfg->nmicchannel, gainnet_aec5->nbin-1);

	gainnet_aec5->train_echo=1;

	wtk_gainnet_aec5_reset(gainnet_aec5);

	return gainnet_aec5;
}

void wtk_gainnet_aec5_delete(wtk_gainnet_aec5_t *gainnet_aec5)
{
	int i;
	int nmicchannel=gainnet_aec5->cfg->nmicchannel;

	wtk_strbufs_delete(gainnet_aec5->mic,gainnet_aec5->cfg->nmicchannel);
	wtk_strbufs_delete(gainnet_aec5->sp,gainnet_aec5->cfg->nspchannel);
	if(gainnet_aec5->notch_mem)
	{
		wtk_float_delete_p2(gainnet_aec5->notch_mem, gainnet_aec5->channel);
		wtk_free(gainnet_aec5->memD);
		wtk_free(gainnet_aec5->memX);
	}
	wtk_free(gainnet_aec5->analysis_window);
	wtk_free(gainnet_aec5->synthesis_window);
	wtk_float_delete_p2(gainnet_aec5->analysis_mem, gainnet_aec5->cfg->nmicchannel);
	wtk_float_delete_p2(gainnet_aec5->analysis_mem_sp, gainnet_aec5->cfg->nspchannel);
	wtk_float_delete_p2(gainnet_aec5->synthesis_mem, gainnet_aec5->cfg->nmicchannel);
	wtk_free(gainnet_aec5->rfft_in);
	wtk_drft_delete(gainnet_aec5->rfft);
	wtk_complex_delete_p2(gainnet_aec5->fft, gainnet_aec5->cfg->nmicchannel);
	wtk_complex_delete_p2(gainnet_aec5->fft_sp, gainnet_aec5->cfg->nspchannel);

	if(gainnet_aec5->gnlmsd)
	{
		wtk_gainnet_aec5_nlmsdelay_delete(gainnet_aec5->gnlmsd);
	}

	if(gainnet_aec5->erls)
	{
		for(i=0;i<gainnet_aec5->nbin;++i)
		{
			wtk_rls_clean(gainnet_aec5->erls+i);
		}
		wtk_free(gainnet_aec5->erls);
	}
	if(gainnet_aec5->enlms)
	{
		for(i=0;i<gainnet_aec5->nbin;++i)
		{
			wtk_nlms_clean(gainnet_aec5->enlms+i);
		}
		wtk_free(gainnet_aec5->enlms);
	}
	if(gainnet_aec5->ekalman)
	{
		qtk_kalman_delete(gainnet_aec5->ekalman);
	}
	wtk_complex_delete_p2(gainnet_aec5->ffty, gainnet_aec5->cfg->nmicchannel);

	if(gainnet_aec5->cfg->use_epostsingle)
	{
		wtk_gainnet_aec5_aec_clean(gainnet_aec5->gsaec);
		wtk_free(gainnet_aec5->gsaec);
	}else
	{
		for(i=0; i<nmicchannel; ++i)
		{
			wtk_gainnet_aec5_aec_clean(gainnet_aec5->gsaec+i);
		}
		wtk_free(gainnet_aec5->gsaec);
	}
	wtk_float_delete_p2(gainnet_aec5->out, gainnet_aec5->cfg->nmicchannel);

	wtk_free(gainnet_aec5);
}

void wtk_gainnet_aec5_reset(wtk_gainnet_aec5_t *gainnet_aec5)
{
	int i,nbin=gainnet_aec5->nbin;
	int nmicchannel=gainnet_aec5->cfg->nmicchannel;
	int wins=gainnet_aec5->cfg->wins;

	wtk_strbufs_reset(gainnet_aec5->mic,gainnet_aec5->cfg->nmicchannel);
	wtk_strbufs_reset(gainnet_aec5->sp,gainnet_aec5->cfg->nspchannel);
	if(gainnet_aec5->notch_mem)
	{
		for(i=0;i<gainnet_aec5->channel;++i)
		{
			memset(gainnet_aec5->notch_mem[i],0,2*sizeof(float));
		}
		memset(gainnet_aec5->memD,0,gainnet_aec5->channel*sizeof(float));
		memset(gainnet_aec5->memX,0,gainnet_aec5->cfg->nmicchannel*sizeof(float));
	}
	for (i=0;i<wins;++i)
	{
		gainnet_aec5->analysis_window[i] = sin((0.5+i)*PI/wins);
	}
	wtk_drft_init_synthesis_window(gainnet_aec5->synthesis_window, gainnet_aec5->analysis_window, wins);

	wtk_float_zero_p2(gainnet_aec5->analysis_mem, gainnet_aec5->cfg->nmicchannel, (gainnet_aec5->nbin-1));
	wtk_float_zero_p2(gainnet_aec5->analysis_mem_sp, gainnet_aec5->cfg->nspchannel, (gainnet_aec5->nbin-1));
	wtk_float_zero_p2(gainnet_aec5->synthesis_mem, gainnet_aec5->cfg->nmicchannel, (gainnet_aec5->nbin-1));

	if(gainnet_aec5->gnlmsd)
	{
		wtk_gainnet_aec5_nlmsdelay_reset(gainnet_aec5->gnlmsd);
	}

	wtk_complex_zero_p2(gainnet_aec5->fft, gainnet_aec5->cfg->nmicchannel, gainnet_aec5->nbin);
	wtk_complex_zero_p2(gainnet_aec5->fft_sp, gainnet_aec5->cfg->nspchannel, gainnet_aec5->nbin);

	if(gainnet_aec5->erls)
	{
		for(i=0;i<nbin;++i)
		{
			wtk_rls_reset(gainnet_aec5->erls+i);
		}
	}
	if(gainnet_aec5->enlms)
	{
		for(i=0;i<nbin;++i)
		{
			wtk_nlms_reset(gainnet_aec5->enlms+i);
		}
	}
	if(gainnet_aec5->ekalman){
		qtk_kalman_reset(gainnet_aec5->ekalman);
	}
	wtk_complex_zero_p2(gainnet_aec5->ffty, gainnet_aec5->cfg->nmicchannel, gainnet_aec5->nbin);

	if(gainnet_aec5->cfg->use_epostsingle)
	{
		wtk_gainnet_aec5_aec_reset(gainnet_aec5->gsaec);
	}else
	{
		for(i=0; i<nmicchannel; ++i)
		{
			wtk_gainnet_aec5_aec_reset(gainnet_aec5->gsaec+i);
		}
	}

	gainnet_aec5->sp_silcnt=0;
	gainnet_aec5->sp_sil=1;
}


void wtk_gainnet_aec5_set_notify(wtk_gainnet_aec5_t *gainnet_aec5,void *ths,wtk_gainnet_aec5_notify_f notify)
{
	gainnet_aec5->notify=notify;
	gainnet_aec5->ths=ths;
}

void wtk_gainnet_aec5_set_notify_tr(wtk_gainnet_aec5_t *gaec,void *ths,wtk_gainnet_aec5_notify_trfeat_f notify)
{
	gaec->notify_tr=notify;
	gaec->ths_tr=ths;
}

static float wtk_gainnet_aec5_sp_energy(float *p,int n)
{
	float f;
	int i;

	f=0;
	for(i=0;i<n;++i)
	{
		f+=p[i]*p[i];
	}
	f/=n;

	return f;
}

void wtk_gainnet_aec5_aec_on_gainnet(wtk_gainnet_single_aec5_t *gsaec, float *gain, int len, int is_end)
{
	memcpy(gsaec->g, gain, sizeof(float)*gsaec->cfg->bankfeat.nb_bands);
}

void wtk_gainnet_aec5_aec_on_gainnet2(wtk_gainnet_single_aec5_t *vaec, float *gain, int len, int is_end)
{
	int i;
	int nb_bands=vaec->cfg->bankfeat.nb_bands;
	float *g2=vaec->g2;
	float agc_a=vaec->cfg->agc_a;
	float agc_b=vaec->cfg->agc_b;

	for(i=0; i<nb_bands; ++i)
	{
		g2[i]=-1/agc_a*(logf(1/gain[i]-1)-agc_b);
	}
}

void wtk_gainnet_aec5_single_aec_feed(wtk_gainnet_single_aec5_t *gsaec, wtk_complex_t *fftx, wtk_complex_t *ffty, int nch,int sp_sil)
{
	int i;
	int nbin=gsaec->nbin;
	float *g=gsaec->g, *gf=gsaec->gf, *lastg=gsaec->lastg, *g2=gsaec->g2,*gf2=gsaec->gf2;
	wtk_gainnet5_t *gainnet=gsaec->gainnet;
	wtk_gainnet2_t *gainnet2=gsaec->gainnet2;
	wtk_gainnet6_t *gainnet6=gsaec->gainnet6;
	wtk_bankfeat_t *bank_mic=gsaec->bank_mic;
	wtk_bankfeat_t *bank_sp=gsaec->bank_sp;
	int nb_bands=bank_mic->cfg->nb_bands;
	int nb_features=bank_mic->cfg->nb_features;
	wtk_qmmse_t *qmmse=gsaec->qmmse;
	float *qmmse_gain;
	wtk_complex_t *fftytmp, sed, *fftxtmp;
	float ef,yf;
	float leak;
	float ralpha=gsaec->cfg->ralpha;
	float gbias=gsaec->cfg->gbias, gftmp;
	// static FILE *g_fn=NULL;

	// if(g_fn==NULL)
	// {
	// 	g_fn=fopen("test.bin", "rb");
	// }

	wtk_bankfeat_flush_frame_features(bank_mic, fftx);
	fftytmp=ffty;
	fftxtmp=fftx;
	for(i=0;i<nbin;++i,++fftxtmp,++fftytmp)
	{
		ef=fftxtmp->a*fftxtmp->a+fftxtmp->b*fftxtmp->b;
		yf=fftytmp->a*fftytmp->a+fftytmp->b*fftytmp->b;
		sed.a=fftytmp->a*fftxtmp->a+fftytmp->b*fftxtmp->b;
		sed.b=-fftytmp->a*fftxtmp->b+fftytmp->b*fftxtmp->a;
		if(gsaec->cfg->use_maxleak)
		{
			leak=(sed.a*sed.a+sed.b*sed.b)/(ef*yf+1e-9);
		}else
		{
			leak=(sed.a*sed.a+sed.b*sed.b)/(max(ef,yf)*yf+1e-9);
		}
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
	}else if(gainnet2)
	{
		wtk_gainnet2_feed2(gainnet2, bank_mic->features, nb_features, bank_sp->features, nb_features, 0);   
	}else if(gainnet6)
	{
		wtk_gainnet6_feed(gainnet6, bank_mic->features, nb_features, bank_sp->features, nb_features, 0);   
	}
	// fread(g, sizeof(float), nb_bands, g_fn);
	for (i=0;i<nb_bands;++i)
	{
		g[i] = max(g[i], ralpha*lastg[i]);
		lastg[i] = g[i];
	}
	wtk_bankfeat_interp_band_gain(bank_mic, nbin, gf, g);
	if(gsaec->agc_on)
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
				if(sp_sil==1)
				{
					gftmp=(qmmse_gain[i]+gf[i])/2;
					if(gsaec->agc_on)			
					{
						gf2[i]*=gftmp/gf[i];
					}
					gf[i]=gftmp;
				}else if(sp_sil==0)
				{
					if(gsaec->agc_on)					
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
	fftx[0].a=fftx[0].b=0;
	fftx[nbin-1].a=fftx[nbin-1].b=0;
	for (i=1; i<nbin-1; ++i)
	{
		ffty[i].a += fftx[i].a*(1-gf[i]);
		ffty[i].b += fftx[i].b*(1-gf[i]);

		fftx[i].a *= gf[i];
		fftx[i].b *= gf[i];
	}

	if(gsaec->agc_on)
	{
		gsaec->gff2=wtk_float_abs_mean(gf2+1, nbin-2);
		for (i=1;i<nbin-1;++i)
		{
			fftx[i].a *= gsaec->gff2;
			fftx[i].b *= gsaec->gff2;
		}
	}
}


void wtk_gainnet_aec5_feed_echo(wtk_gainnet_aec5_t *gainnet_aec5, wtk_complex_t **fft, wtk_complex_t **fft_sp, float *spx)
{
	int nmicchannel=gainnet_aec5->cfg->nmicchannel;
	int nspchannel=gainnet_aec5->cfg->nspchannel;
	int fsize=gainnet_aec5->cfg->wins/2;
	int i,j,k;
	int nbin=gainnet_aec5->nbin;
	wtk_rls_t *erls=gainnet_aec5->erls, *erlstmp;
	wtk_nlms_t *enlms=gainnet_aec5->enlms, *enlmstmp;
	wtk_complex_t fft2[64];
	wtk_complex_t fftsp2[10];
	wtk_complex_t **ffty=gainnet_aec5->ffty;
	float spenr;
	float spenr_thresh=gainnet_aec5->cfg->spenr_thresh;
	int spenr_cnt=gainnet_aec5->cfg->spenr_cnt;
	wtk_gainnet_single_aec5_t *gsaec=gainnet_aec5->gsaec;
	float *gf=gsaec->gf;

	wtk_complex_zero_p2(gainnet_aec5->ffty, gainnet_aec5->cfg->nmicchannel, gainnet_aec5->nbin);
	spenr=wtk_gainnet_aec5_sp_energy(spx, fsize);
	if(spenr>spenr_thresh)
	{
		gainnet_aec5->sp_sil=0;
		gainnet_aec5->sp_silcnt=spenr_cnt;
	}else if(gainnet_aec5->sp_sil==0)
	{
		gainnet_aec5->sp_silcnt-=1;
		if(gainnet_aec5->sp_silcnt<=0)
		{
			gainnet_aec5->sp_sil=1;
		}
	}

	if(gainnet_aec5->gnlmsd)
	{
		wtk_gainnet_aec5_nlmsdelay_feed(gainnet_aec5->gnlmsd,  fft[0],  fft_sp, gainnet_aec5->sp_sil);
	}
	if(erls)
	{
		erlstmp=erls;
		for(k=0; k<nbin; ++k, ++erlstmp)
		{
			for(i=0; i<nmicchannel; ++i)
			{
				fft2[i]=fft[i][k];
			}
			for(j=0; j<nspchannel; ++j)
			{
				fftsp2[j]=fft_sp[j][k];
			}
			wtk_rls_feed3(erlstmp, fft2, fftsp2,gainnet_aec5->sp_sil==0);
			if(gainnet_aec5->sp_sil==0)
			{
				for(i=0; i<nmicchannel; ++i)
				{
					fft[i][k]=erlstmp->out[i];
					ffty[i][k]=erlstmp->lsty[i];
				}
			}else
			{
				for(i=0; i<nmicchannel; ++i)
				{
					ffty[i][k].a=ffty[i][k].b=0;
				}
			}
		}
	}else if(enlms)
	{
		enlmstmp=enlms;
		for(k=0; k<nbin; ++k, ++enlmstmp)
		{
			for(i=0; i<nmicchannel; ++i)
			{
				fft2[i]=fft[i][k];
			}
			for(j=0; j<nspchannel; ++j)
			{
				fftsp2[j]=fft_sp[j][k];
			}
			wtk_nlms_feed3(enlmstmp, fft2, fftsp2,gainnet_aec5->sp_sil==0);
			if(gainnet_aec5->sp_sil==0)
			{
				for(i=0; i<nmicchannel; ++i)
				{
					fft[i][k]=enlmstmp->out[i];
					ffty[i][k]=enlmstmp->lsty[i];
				}
			}else
			{
				for(i=0; i<nmicchannel; ++i)
				{
					ffty[i][k].a=ffty[i][k].b=0;
				}
			}
		}
	}
	if(gainnet_aec5->cfg->use_epostsingle)
	{
		wtk_gainnet_aec5_single_aec_feed(gsaec, fft[0], ffty[0], 0, gainnet_aec5->sp_sil);
		for(i=1; i<nmicchannel; ++i)
		{
			fft[i][0].a=fft[i][0].b=0;
			fft[i][nbin-1].a=fft[i][nbin-1].b=0;
			for (k=1; k<nbin-1; ++k)
			{	
				ffty[i][k].a += fft[i][k].a*(1-gf[k]);
				ffty[i][k].b += fft[i][k].b*(1-gf[k]);

				fft[i][k].a *= gf[k];
				fft[i][k].b *= gf[k];

				if(gsaec->agc_on)
				{
					fft[i][k].a *= gsaec->gff2;
					fft[i][k].b *= gsaec->gff2;
				}
			}
		}
	}else
	{
		for(i=0; i<nmicchannel; ++i, ++gsaec)
		{
			wtk_gainnet_aec5_single_aec_feed(gsaec, fft[i], ffty[i], i, gainnet_aec5->sp_sil);
		}
	}
}

void wtk_gainnet_aec5_feed(wtk_gainnet_aec5_t *gainnet_aec5,short **data,int len,int is_end)
{
#ifdef DEBUG_WAV
	static wtk_wavfile_t *mic_log=NULL;

	if(!mic_log)
	{
		mic_log=wtk_wavfile_new(16000);
		wtk_wavfile_set_channel(mic_log,gainnet_aec5->channel);
		wtk_wavfile_open2(mic_log,"gainnet_aec5");
	}
	if(len>0)
	{
		wtk_wavfile_write_mc(mic_log,data,len);
	}
	if(is_end && mic_log)
	{
		wtk_wavfile_close(mic_log);
		wtk_wavfile_delete(mic_log);
		mic_log=NULL;
	}
#endif

	int i,j,ii;
	int nmicchannel=gainnet_aec5->cfg->nmicchannel;
	int nspchannel=gainnet_aec5->cfg->nspchannel;
	int channel=gainnet_aec5->channel;
	wtk_strbuf_t **mic=gainnet_aec5->mic;
	wtk_strbuf_t **sp=gainnet_aec5->sp;
	float **notch_mem=gainnet_aec5->notch_mem;
	float *memD=gainnet_aec5->memD;
	float *memX=gainnet_aec5->memX;
	float fv, *fp1, *fp2;
	int wins=gainnet_aec5->cfg->wins;
	int fsize=wins/2;
	int length;
	wtk_drft_t *rfft=gainnet_aec5->rfft;
	float *rfft_in=gainnet_aec5->rfft_in;
	wtk_complex_t **fft=gainnet_aec5->fft, **fft_sp=gainnet_aec5->fft_sp;
	float **analysis_mem=gainnet_aec5->analysis_mem, **analysis_mem_sp=gainnet_aec5->analysis_mem_sp;
	float **synthesis_mem=gainnet_aec5->synthesis_mem;
	float *analysis_window=gainnet_aec5->analysis_window, *synthesis_window=gainnet_aec5->synthesis_window;	
	float **out=gainnet_aec5->out, *outtmp;
	short *pv[64];

	for(i=0; i<nmicchannel; ++i)
	{
		for(j=0; j<len; ++j)
		{
			fv=data[i][j];
			wtk_strbuf_push(mic[i],(char *)&(fv),sizeof(float));
		}
	}
	for(i=nmicchannel, ii=0; i<channel; ++i, ++ii)
	{
		for(j=0; j<len; ++j)
		{
			fv=data[i][j];
			wtk_strbuf_push(sp[ii],(char *)&(fv),sizeof(float));
		}
	}
	length=mic[0]->pos/sizeof(float);
	while(length>=fsize)
	{
		for(i=0; i<nmicchannel; ++i)
		{
			fp1=(float *)mic[i]->data;
			if(notch_mem)
			{
				wtk_preemph_dc(fp1, notch_mem[i], fsize);
				memD[i]=wtk_preemph_asis(fp1, fsize, memD[i]);
			}
			wtk_drft_frame_analysis(rfft, rfft_in, analysis_mem[i], fft[i], fp1, wins, analysis_window);
		}
		for(i=0, j=nmicchannel; i<nspchannel; ++i, ++j)
		{
			fp2=(float *)sp[i]->data;
			if(notch_mem)
			{
				wtk_preemph_dc(fp2, notch_mem[j], fsize);
				memD[j]=wtk_preemph_asis(fp2, fsize, memD[j]);
			}
			wtk_drft_frame_analysis(rfft, rfft_in, analysis_mem_sp[i], fft_sp[i], fp2, wins, analysis_window);
		}
		wtk_gainnet_aec5_feed_echo(gainnet_aec5, fft, fft_sp, (float *)sp[0]->data);

		wtk_strbufs_pop(mic, nmicchannel, fsize*sizeof(float));
		wtk_strbufs_pop(sp, nspchannel, fsize*sizeof(float));
		length=mic[0]->pos/sizeof(float);

		for(i=0; i<nmicchannel; ++i)
		{
			wtk_drft_frame_synthesis(rfft, rfft_in, synthesis_mem[i], fft[i], out[i], wins, synthesis_window);
			if(notch_mem)
			{
				memX[i]=wtk_preemph_asis2(out[i],fsize,memX[i]);
			}
			pv[i]=(short *)out[i];
			for(j=0; j<fsize; ++j)
			{
				if(fabs(out[i][j])<32767.0)
				{
					pv[i][j]=floorf(out[i][j]+0.5);
				}else
				{
					if(out[i][j]>0)
					{
						pv[i][j]=32767;
					}else
					{
						pv[i][j]=-32767;
					}
				}
			}
		}
		if(gainnet_aec5->notify)
		{
			gainnet_aec5->notify(gainnet_aec5->ths,pv,fsize,0);
		}
	}
	if(is_end)
	{
		length=mic[0]->pos/sizeof(float);
		if(length>0)
		{
			if(gainnet_aec5->notify)
			{
				for(i=0; i<nmicchannel; ++i)
				{
					pv[i]=(short *)mic[i]->data;
					outtmp=(float *)mic[i]->data;
					for(j=0; j<length; ++j)
					{
						pv[i][j]=floorf(outtmp[j]+0.5);
					}
				}
				gainnet_aec5->notify(gainnet_aec5->ths,pv,length,1);
			}
		}else
		{
			if(gainnet_aec5->notify)
			{
				gainnet_aec5->notify(gainnet_aec5->ths,NULL,0,1);
			}  
		}
	}
}

void wtk_gainnet_aec5_feed_train(wtk_gainnet_aec5_t *gaec,short **data,int len,int channel, int bb, int stabe)
{
	// static const float a_hp[2] = {-1.99599, 0.99600};
	// static const float b_hp[2] = {-2, 1};
	// static float mem_hp_x[2]={0,0};
	// static float mem_hp_xn[2]={0,0};
	// static float mem_hp_spx[2]={0,0};
	wtk_gainnet_single_aec5_t *gsaec=gaec->gsaec;
	short *sdata, *spdata, *ydata, *ndata, *sdatar, *sdataagc, *dataagc;
	int fsize=gaec->cfg->wins/2;
	int i;
	float *xn;
	float *spx;
	float *x;
	float *Es;
	float *xr;
	float *Er;
	float *xagc;
	float *Eagc;
	float *agc;
	float *g;
	float *ge;
	wtk_complex_t *ffts;
	wtk_complex_t *fftr;
	wtk_complex_t *fftagc;
	wtk_bankfeat_t *bank_mic=gsaec->bank_mic;
	wtk_bankfeat_t *bank_sp=gsaec->bank_sp;
	int nb_bands=gaec->cfg->bankfeat.nb_bands;
	int nb_features=gaec->cfg->bankfeat.nb_features;
	float *Ex=bank_mic->Ex;
	int wins=gaec->cfg->wins;
	float *analysis_mem_tr;
	float *analysis_mem_tr2;
	float *analysis_mem_tr3;
	int pos;
	//   short *pv;
	//   wtk_strbuf_t *outbuf=wtk_strbuf_new(1024,2);
	int nbin=gsaec->nbin;
	wtk_drft_t *rfft=gaec->rfft;
	float *rfft_in=gaec->rfft_in;
	wtk_complex_t *fft=gaec->fft[0], *fft_sp=gaec->fft_sp[0], *ffty=gaec->ffty[0];
	float *analysis_mem=gaec->analysis_mem[0], *analysis_mem_sp=gaec->analysis_mem_sp[0];//, *synthesis_mem=gaec->synthesis_mem[0];
	wtk_rls_t *erls=gaec->erls;
	wtk_nlms_t *enlms=gaec->enlms;
	qtk_ahs_kalman_t *ekalman=gaec->ekalman;
	float notch_tmp[3][2]={{0,0},{0,0},{0,0}};
	float memtmp[3]={0,0,0};
	float spenr;
	float spenr_thresh=gaec->cfg->spenr_thresh;
	int spenr_cnt=gaec->cfg->spenr_cnt;
	float data_max, fscale;
	wtk_complex_t *fftytmp, sed, *fftxtmp;
	float ef,yf;
	float leak;
	// time_t t;
	// srand((unsigned) time(&t));

	xn=wtk_malloc(sizeof(float)*fsize);
	spx=wtk_malloc(sizeof(float)*fsize);
	x=wtk_malloc(sizeof(float)*fsize);
	xr=wtk_malloc(sizeof(float)*fsize);
	xagc=wtk_malloc(sizeof(float)*fsize);

	Es=wtk_malloc(sizeof(float)*nb_bands);
	Er=wtk_malloc(sizeof(float)*nb_bands);
	Eagc=wtk_malloc(sizeof(float)*nb_bands);

	ge=wtk_malloc(sizeof(float)*nb_bands);
	g=wtk_malloc(sizeof(float)*nb_bands);
	agc=wtk_malloc(sizeof(float)*nb_bands);

	ffts=wtk_malloc(sizeof(wtk_complex_t)*nbin);
	fftr=wtk_malloc(sizeof(wtk_complex_t)*nbin);
	fftagc=wtk_malloc(sizeof(wtk_complex_t)*nbin);

	analysis_mem_tr=wtk_calloc(sizeof(float),fsize);
	analysis_mem_tr2=wtk_calloc(sizeof(float),fsize);
	analysis_mem_tr3=wtk_calloc(sizeof(float),fsize);

	dataagc=wtk_malloc(sizeof(short)*len);

	data_max=wtk_short_abs_max(data[5], len);
	fscale=22937.0/data_max;
	for(i=0; i<len; ++i)
	{
		dataagc[i]=floorf(data[5][i]*fscale+0.5);
	}
	if(gaec->train_echo==0)
	{
		if(bb==2)  // 纯语音，无回声，无噪声
		{
			memset(data[1], 0, sizeof(short)*len);
			memset(data[3], 0, sizeof(short)*len);
			memset(data[4], 0, sizeof(short)*len);
			stabe=0;
		}
		if(bb==3)  // 纯噪声，无语音，无回声
		{
			memset(data[1], 0, sizeof(short)*len);
			memset(data[3], 0, sizeof(short)*len);
			memset(data[2], 0, sizeof(short)*len);
			memset(data[5], 0, sizeof(short)*len);
			memset(dataagc, 0, sizeof(short)*len);
			stabe=0;
		}
		if(bb==4)  // 噪音和语音，无回声
		{
			memset(data[1], 0, sizeof(short)*len);
			memset(data[3], 0, sizeof(short)*len);
			stabe=0;

			for(i=0;i<fsize;++i)
			{
				data[0][i]=data[2][i]+data[4][i];
			}
			data_max=wtk_short_abs_max(data[0], len);
			fscale=rand()/(RAND_MAX+1.0)*0.8+0.1;
			fscale=(fscale*32767.0)/data_max;
			for(i=0; i<len; ++i)
			{
				data[2][i]=floorf(data[2][i]*fscale+0.5);
				data[4][i]=floorf(data[4][i]*fscale+0.5);
				data[5][i]=floorf(data[5][i]*fscale+0.5);
			}
		}
	}else
	{
		if(bb==2)  // 无语音和噪声，有回声
		{
			memset(data[2], 0, sizeof(short)*len);
			memset(data[5], 0, sizeof(short)*len);
			memset(dataagc, 0, sizeof(short)*len);
			memset(data[4], 0, sizeof(short)*len);
		}
		if(bb==3)  // 无噪声，有回声
		{
			memset(data[4], 0, sizeof(short)*len);
		}
		if(bb==4)  // 无语音，有回声和噪声
		{
			memset(data[2], 0, sizeof(short)*len);
			memset(data[5], 0, sizeof(short)*len);
			memset(dataagc, 0, sizeof(short)*len);
		}
		if(bb==5)  // 语音放大
		{
			data_max=wtk_short_abs_max(data[0], len);
			fscale=rand()/(RAND_MAX+1.0)*0.8+0.1;
			fscale=(fscale*32767.0)/data_max;
			for(i=0; i<len; ++i)
			{
				data[2][i]=floorf(data[2][i]*fscale+0.5);
				data[5][i]=floorf(data[5][i]*fscale+0.5);
				data[3][i]=floorf(data[3][i]*fscale+0.5);
				data[4][i]=floorf(data[4][i]*fscale+0.5);
			}
		}
	}
	pos=0;

	if(stabe==1)
	{
		while((len-pos)>=fsize)
		{
			spdata=data[1]+pos;
			sdata=data[2]+pos;
			ydata=data[3]+pos;
			ndata=data[4]+pos;
			for(i=0;i<fsize;++i)
			{
				xn[i]=sdata[i]+ydata[i]+ndata[i];
				spx[i]=spdata[i];
			}
			if(gaec->notch_mem)
			{
				wtk_preemph_dc(xn, gaec->notch_mem[0], fsize);
				gaec->memD[0]=wtk_preemph_asis(xn, fsize, gaec->memD[0]);
				wtk_preemph_dc(spx, gaec->notch_mem[1], fsize);
				gaec->memD[1]=wtk_preemph_asis(spx, fsize, gaec->memD[1]);
			}

			wtk_drft_frame_analysis(rfft, rfft_in, analysis_mem, fft, xn, wins, gaec->analysis_window);
			wtk_drft_frame_analysis(rfft, rfft_in, analysis_mem_sp, fft_sp, spx, wins, gaec->analysis_window);
			if(enlms)
			{
				for(i=0;i<nbin;++i)
				{
					wtk_nlms_feed(enlms+i, fft+i, fft_sp+i);
				}
			}else if(erls)
			{
				for(i=0;i<nbin;++i)
				{
					wtk_rls_feed(erls+i, fft+i, fft_sp+i);
				}
			}else if(ekalman){
				qtk_kalman_update(ekalman, fft, gaec->nbin, 1, fft_sp);
			}
			pos+=fsize;
		}
	}

	if(gaec->notch_mem)
	{
		for(i=0;i<gaec->channel;++i)
		{
			memset(gaec->notch_mem[i],0,2*sizeof(float));
		}
		memset(gaec->memD,0,gaec->channel*sizeof(float));

		for(i=0;i<3;++i)
		{
			memset(notch_tmp[i],0,2*sizeof(float));
		}
		memset(memtmp,0,3*sizeof(float));
	}
	if(erls)
	{
		for(i=0;i<nbin;++i)
		{
			memset((erls+i)->xld, 0, sizeof(wtk_complex_t)*(erls+i)->cfg->nl);
		}
	}else if(enlms)
	{
		for(i=0;i<nbin;++i)
		{
			memset((enlms+i)->far_x, 0, sizeof(wtk_complex_t)*(enlms+i)->cfg->L);
			(enlms+i)->power_x=0;
		}
	}else if(ekalman){
		memset(ekalman->x_cache, 0, sizeof(wtk_complex_t)*ekalman->nbin*ekalman->cfg->L);
		memset(ekalman->NonLinear_SPEC, 0, sizeof(wtk_complex_t)*ekalman->nbin*ekalman->cfg->L);
	}
	memset(analysis_mem,0,sizeof(float)*fsize);
	memset(analysis_mem_sp,0,sizeof(float)*fsize);
	pos=0;
	while((len-pos)>=fsize)
	{
		spdata=data[1]+pos;
		sdata=data[2]+pos;
		ydata=data[3]+pos;

		ndata=data[4]+pos;
		sdatar=data[5]+pos;
		sdataagc=dataagc+pos;
		for(i=0;i<fsize;++i)
		{
			xn[i]=sdata[i]+ydata[i]+ndata[i];
			spx[i]=spdata[i];
			x[i]=sdata[i];
			xr[i]=sdatar[i];
			xagc[i]=sdataagc[i];
		}
		// biquad(x, mem_hp_x, x, b_hp, a_hp, fsize);
		// biquad(xn, mem_hp_xn, xn, b_hp, a_hp, fsize);
		// biquad(spx, mem_hp_spx, spx, b_hp, a_hp, fsize);

		if(gaec->notch_mem)
		{
			wtk_preemph_dc(x, notch_tmp[0], fsize);
			memtmp[0]=wtk_preemph_asis(x, fsize, memtmp[0]);
			wtk_preemph_dc(xr, notch_tmp[1], fsize);
			memtmp[1]=wtk_preemph_asis(xr, fsize, memtmp[1]);
			// wtk_preemph_dc(xagc, notch_tmp[2], fsize);
			// memtmp[2]=wtk_preemph_asis(xagc, fsize, memtmp[2]);
		}

		wtk_drft_frame_analysis(rfft, rfft_in, analysis_mem_tr, ffts, x, wins, gaec->analysis_window);
		ffts[0].a=ffts[0].b=ffts[nbin-1].a=ffts[nbin-1].b=0;
		wtk_bankfeat_compute_band_energy(bank_mic, Es, ffts);

		wtk_drft_frame_analysis(rfft, rfft_in, analysis_mem_tr2, fftr, xr, wins, gaec->analysis_window);
		fftr[0].a=fftr[0].b=fftr[nbin-1].a=fftr[nbin-1].b=0;
		wtk_bankfeat_compute_band_energy(bank_mic, Er, fftr);

		wtk_drft_frame_analysis(rfft, rfft_in, analysis_mem_tr3, fftagc, xagc, wins, gaec->analysis_window);
		fftagc[0].a=fftagc[0].b=fftagc[nbin-1].a=fftagc[nbin-1].b=0;
		wtk_bankfeat_compute_band_energy(bank_mic, Eagc, fftagc);

		if(gaec->notch_mem)
		{
			wtk_preemph_dc(xn, gaec->notch_mem[0], fsize);
			gaec->memD[0]=wtk_preemph_asis(xn, fsize, gaec->memD[0]);
			wtk_preemph_dc(spx, gaec->notch_mem[1], fsize);
			gaec->memD[1]=wtk_preemph_asis(spx, fsize, gaec->memD[1]);
		}

		spenr=wtk_gainnet_aec5_sp_energy(spx, fsize);
		if(spenr>spenr_thresh)
		{
			gaec->sp_sil=0;
			gaec->sp_silcnt=spenr_cnt;
		}else if(gaec->sp_sil==0)
		{
			gaec->sp_silcnt-=1;
			if(gaec->sp_silcnt<=0)
			{
				gaec->sp_sil=1;
			}
		}
		wtk_drft_frame_analysis(rfft, rfft_in, analysis_mem, fft, xn, wins, gaec->analysis_window);
		wtk_drft_frame_analysis(rfft, rfft_in, analysis_mem_sp, fft_sp, spx, wins, gaec->analysis_window);
		if(erls)
		{
			for(i=0;i<nbin;++i)
			{
				wtk_rls_feed3(erls+i, fft+i, fft_sp+i, gaec->sp_sil==0);
				if(gaec->sp_sil==0)
				{
					fft[i]=erls[i].out[0];
					ffty[i]=erls[i].lsty[0];
				}else
				{
					ffty[i].a=ffty[i].b=0;
				}
			}
		}else if(enlms)
		{
			for(i=0;i<nbin;++i)
			{
				wtk_nlms_feed3(enlms+i, fft+i, fft_sp+i, gaec->sp_sil==0);
				if(gaec->sp_sil==0)
				{
					fft[i]=enlms[i].out[0];
					ffty[i]=enlms[i].lsty[0];
				}else
				{
					ffty[i].a=ffty[i].b=0;
				}
			}
		}else if(ekalman){
			qtk_kalman_update(ekalman, fft, gaec->nbin, 1, fft_sp);
			if(gaec->sp_sil==0)
			{
				for(int i=0;i<gaec->nbin;++i)
				{
					fft[i].a=ekalman->s_hat[i].a;
					fft[i].b=ekalman->s_hat[i].b;
					ffty[i].a=ekalman->lsty_hat[i].a;
					ffty[i].b=ekalman->lsty_hat[i].b;	
				}
			}else
			{
				ffty[i].a=ffty[i].b=0;
			}
		}
		
		wtk_bankfeat_flush_frame_features(bank_mic, fft);
		fftytmp=ffty;
		fftxtmp=fft;
		for(i=0;i<nbin;++i,++fftxtmp,++fftytmp)
		{
			ef=fftxtmp->a*fftxtmp->a+fftxtmp->b*fftxtmp->b;
			yf=fftytmp->a*fftytmp->a+fftytmp->b*fftytmp->b;
			sed.a=fftytmp->a*fftxtmp->a+fftytmp->b*fftxtmp->b;
			sed.b=-fftytmp->a*fftxtmp->b+fftytmp->b*fftxtmp->a;
			if(gsaec->cfg->use_maxleak)
			{
				leak=(sed.a*sed.a+sed.b*sed.b)/(ef*yf+1e-9);
			}else
			{
				leak=(sed.a*sed.a+sed.b*sed.b)/(max(ef,yf)*yf+1e-9);
			}
			leak=sqrtf(leak);
			fftytmp->a*=leak;
			fftytmp->b*=leak;
		}
		wtk_bankfeat_flush_frame_features(bank_sp, ffty);

		for (i=0;i<nb_bands;i++)
		{
			// printf("%d  %f %f   // ", silence, Ex[i], Er[i]);
			if(bank_mic->silence || Ex[i] < 5e-2 || Es[i] < 5e-2 || Er[i] < 5e-2)
			{
				g[i]=0;
				ge[i]=0;
				agc[i]=0;
			}else
			{
				g[i] = sqrt((Er[i])/(Ex[i]));
				ge[i] = sqrt((Es[i])/(Ex[i]));
				if (g[i] > 1) g[i] = 1;
				if (ge[i] > 1) ge[i] = 1;
				// agc[i] = sqrt((Eagc[i])/(Ex[i]))/g[i];
				agc[i] = sqrt((Eagc[i])/(Ex[i]));

				// ge[i] = Es[i];
				// g[i] = Er[i];
				// agc[i] = Eagc[i];
				// printf("%f ",agc[i]);
				// if(Ex[i]<1e-2)
				// {
				// 	g[i]=-1;
				// 	ge[i]=-1;
				// 	agc[i]=-1;
				// }
			}
		}
		// printf("\n");

		// {
		// 	float gf2[1024];
		// 	wtk_bankfeat_interp_band_gain(gsaec->bank_mic, nbin, gsaec->gf, g);
		// 	wtk_bankfeat_interp_band_gain(gsaec->bank_mic, nbin, gf2, agc);
		// 	for (i=1; i<nbin-1; ++i)
		// 	{
		// 		gf2[i]=gf2[i]/(gsaec->gf[i]+1e-6);
		// 		gf2[i]=max(1,gf2[i]);
		// 	}
		// 	for (i=0;i<gsaec->nbin;i++)
		// 	{
		// 		// gsaec->gf[i]=sqrt(  (fftr[i].a*fftr[i].a+fftr[i].b*fftr[i].b)   / (fft[i].a*fft[i].a+fft[i].b*fft[i].b)  );
		// 		fft[i].a *= gsaec->gf[i];//*gf2[i];
		// 		fft[i].b *= gsaec->gf[i];//*gf2[i];
		// 	}
		// 	wtk_drft_frame_synthesis(rfft, rfft_in, synthesis_mem, fft, gaec->out[0], wins, gaec->synthesis_window);
		// 	if(gaec->notch_mem)
		// 	{
		// 		gaec->memX[0]=wtk_preemph_asis2(gaec->out[0],fsize,gaec->memX[0]);
		// 	}
		// 	pv=(short *)gaec->out[0];
		// 	for(i=0;i<fsize;++i)
		// 	{
		// 		pv[i]=gaec->out[0][i];
		// 	}
		// 	wtk_strbuf_push(outbuf, (char *)pv, sizeof(short)*fsize);
		// }

		if(gaec->notify_tr)
		{
			gaec->notify_tr(gaec->ths_tr, bank_mic->features, nb_features,  bank_sp->features, nb_features, agc, g, ge, nb_bands);
		}
		pos+=fsize;
	}
	// {
	// 	wtk_wavfile_t *wav;
	// 	wav=wtk_wavfile_new(gsaec->cfg->rate);
	// 	wav->max_pend=0;
	// 	wtk_wavfile_open(wav,"o.wav");
	// 	wtk_wavfile_write(wav,(char *)outbuf->data,outbuf->pos);
	// 	wtk_wavfile_delete(wav);
	// }

	wtk_free(xn);
	wtk_free(spx);
	wtk_free(x);
	wtk_free(xr);
	wtk_free(xagc);

	wtk_free(Es);
	wtk_free(Er);
	wtk_free(Eagc);

	wtk_free(ge);
	wtk_free(g);
	wtk_free(agc);

	wtk_free(ffts);
	wtk_free(fftr);
	wtk_free(fftagc);

	wtk_free(analysis_mem_tr);
	wtk_free(analysis_mem_tr2);
	wtk_free(analysis_mem_tr3);

	wtk_free(dataagc);
}


