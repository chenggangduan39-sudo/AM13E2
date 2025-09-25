#include "wtk_gainnet_bf6.h"
#define gainnet_bf6_tobank(n)   (13.1f*atan(.00074f*(n))+2.24f*atan((n)*(n)*1.85e-8f)+1e-4f*(n))

void wtk_gainnet_bf6_mask_on_gainnet(wtk_gainnet_bf6_mask_t *gbfmask, float *gain, int len, int is_end);
void wtk_gainnet_bf6_mask2_on_gainnet(wtk_gainnet_bf6_mask2_t *gbfmask2, float *gain, int len, int is_end);

void wtk_gainnet_bf6_xfilterbank_init(int *eband, int bands,int rate,int len)
{
	float df;
	float max_mel, mel_interval;
	int i;
	int id;
	float curr_freq;
	float mel;

	df =rate*1.0/(2*(len-1));
	max_mel = gainnet_bf6_tobank(rate/2);
	mel_interval =max_mel/(bands-1);
	for(i=0; i<bands; ++i)
   {
	   eband[i]=-1;
   }
   for (i=0;i<len;++i)
   {
		curr_freq = i*df;
		// printf("%f\n",curr_freq);
		mel = gainnet_bf6_tobank(curr_freq);
		if (mel > max_mel)
		{
			break;
		}
		id = (int)(floor(mel/mel_interval));
		if(eband[id]==-1)
		{
			eband[id]=i;
		}
				// printf("%d %d %f \n",id,eband[id],curr_freq);
   }
   eband[bands-1]=len-1;
//    for(i=0; i<bands; ++i)
//    {
// 	   printf("%d ",eband[i]);
//    }
//    printf("\n");
}

void wtk_gainnet_bf6_mask_init(wtk_gainnet_bf6_mask_t *gbfmask, wtk_gainnet_bf6_cfg_t *cfg, wtk_gainnet2_cfg_t *gainnet, wtk_masknet_cfg_t *masknet)
{
	gbfmask->cfg=cfg;
	gbfmask->nbin=cfg->wins/2+1;

	gbfmask->eband=wtk_malloc(sizeof(int)*cfg->nb_bands);
	wtk_gainnet_bf6_xfilterbank_init(gbfmask->eband, cfg->nb_bands, cfg->rate, gbfmask->nbin);

	gbfmask->Ex=wtk_malloc(sizeof(float)*cfg->nb_bands);
	gbfmask->Ex2=wtk_malloc(sizeof(float)*cfg->nb_bands);

	gbfmask->dct_table=wtk_malloc(sizeof(float)*cfg->nb_bands*cfg->nb_bands);

	gbfmask->cepstral_mem=gbfmask->cepstral_mem2=NULL;
	if(cfg->use_ceps)
	{
		gbfmask->cepstral_mem=wtk_float_new_p2(cfg->ceps_mem, cfg->nb_bands);
		gbfmask->cepstral_mem2=wtk_float_new_p2(cfg->ceps_mem, cfg->nb_bands);
	}

	gbfmask->g=wtk_malloc(sizeof(float)*cfg->nb_bands);
	gbfmask->gf=wtk_malloc(sizeof(float)*gbfmask->nbin);
	gbfmask->g2=wtk_malloc(sizeof(float)*cfg->nb_bands);
	gbfmask->gf2=wtk_malloc(sizeof(float)*gbfmask->nbin);

	gbfmask->Ly=wtk_malloc(sizeof(float)*cfg->nb_bands);
	gbfmask->features=wtk_malloc(sizeof(float)*cfg->nb_features);

	gbfmask->masknet=NULL;
	gbfmask->gainnet=NULL;
	if(gainnet)
	{
		gbfmask->gainnet=wtk_gainnet2_new(gainnet);
		wtk_gainnet2_set_notify(gbfmask->gainnet, gbfmask, (wtk_gainnet2_notify_f)wtk_gainnet_bf6_mask_on_gainnet);
	}else
	{
		gbfmask->masknet=wtk_masknet_new(masknet);
		wtk_masknet_set_notify(gbfmask->masknet, gbfmask, (wtk_masknet_notify_f)wtk_gainnet_bf6_mask_on_gainnet);
	}
}

void wtk_gainnet_bf6_mask_clean(wtk_gainnet_bf6_mask_t *gbfmask)
{
	wtk_free(gbfmask->eband);
	wtk_free(gbfmask->Ex);
	wtk_free(gbfmask->Ex2);

	wtk_free(gbfmask->dct_table);

	if(gbfmask->cepstral_mem)
	{
		wtk_float_delete_p2(gbfmask->cepstral_mem, gbfmask->cfg->ceps_mem);
		wtk_float_delete_p2(gbfmask->cepstral_mem2, gbfmask->cfg->ceps_mem);
	}

	wtk_free(gbfmask->g);
	wtk_free(gbfmask->gf);
	wtk_free(gbfmask->g2);
	wtk_free(gbfmask->gf2);

	wtk_free(gbfmask->Ly);
	wtk_free(gbfmask->features);

	if(gbfmask->gainnet)
	{
		wtk_gainnet2_delete(gbfmask->gainnet);
	}
	if(gbfmask->masknet)
	{
		wtk_masknet_delete(gbfmask->masknet);
	}

}

void wtk_gainnet_bf6_mask_reset(wtk_gainnet_bf6_mask_t *gbfmask)
{
	int i,j;
	int nb_bands=gbfmask->cfg->nb_bands;

	for (i=0;i<nb_bands;++i) 
	{
		for (j=0;j<nb_bands;++j)
		{
			gbfmask->dct_table[i*nb_bands + j] = cos((i+.5)*j*PI/nb_bands);
			if (j==0)
			{
			gbfmask->dct_table[i*nb_bands + j] *= sqrt(.5);
			}
		}
	}

	memset(gbfmask->Ex, 0, sizeof(float)*(gbfmask->cfg->nb_bands));
	memset(gbfmask->Ex2, 0, sizeof(float)*(gbfmask->cfg->nb_bands));

	gbfmask->memid=0;
	gbfmask->memid2=0;

	if(gbfmask->cepstral_mem)
	{
		wtk_float_zero_p2(gbfmask->cepstral_mem, gbfmask->cfg->ceps_mem, gbfmask->cfg->nb_bands);
		wtk_float_zero_p2(gbfmask->cepstral_mem2, gbfmask->cfg->ceps_mem, gbfmask->cfg->nb_bands);
	}

	memset(gbfmask->g, 0, sizeof(float)*gbfmask->cfg->nb_bands);
	memset(gbfmask->gf, 0, sizeof(float)*gbfmask->nbin);
	memset(gbfmask->g2, 0, sizeof(float)*gbfmask->cfg->nb_bands);
	memset(gbfmask->gf2, 0, sizeof(float)*gbfmask->nbin);

	memset(gbfmask->Ly, 0, sizeof(float)*gbfmask->cfg->nb_bands);
	memset(gbfmask->features, 0, sizeof(float)*gbfmask->cfg->nb_features);

	if(gbfmask->gainnet)
	{
		wtk_gainnet2_reset(gbfmask->gainnet);
	}
	if(gbfmask->masknet)
	{
		wtk_masknet_reset(gbfmask->masknet);
	}

	gbfmask->silence=1;
}

void wtk_gainnet_bf6_mask2_init(wtk_gainnet_bf6_mask2_t *gbfmask2, wtk_gainnet_bf6_cfg_t *cfg, wtk_gainnet2_cfg_t *gainnet, wtk_masknet_cfg_t *masknet)
{
	gbfmask2->cfg=cfg;
	gbfmask2->nbin=cfg->wins/2+1;

	gbfmask2->g=wtk_malloc(sizeof(float)*(gbfmask2->nbin-2));
	// gbfmask2->features_len=(gbfmask2->nbin-2)*(2+cfg->channel-1+1);
	gbfmask2->features_len=(gbfmask2->nbin-2)*(2+1);
	gbfmask2->features=wtk_malloc(sizeof(float)*gbfmask2->features_len);

	gbfmask2->gainnet=NULL;
	gbfmask2->masknet=NULL;
	if(gainnet)
	{
		gbfmask2->gainnet=wtk_gainnet2_new(gainnet);
		wtk_gainnet2_set_notify(gbfmask2->gainnet, gbfmask2, (wtk_gainnet2_notify_f)wtk_gainnet_bf6_mask2_on_gainnet);
	}else
	{
		gbfmask2->masknet=wtk_masknet_new(masknet);
		wtk_masknet_set_notify(gbfmask2->masknet, gbfmask2, (wtk_masknet_notify_f)wtk_gainnet_bf6_mask2_on_gainnet);
	}
}

void wtk_gainnet_bf6_mask2_clean(wtk_gainnet_bf6_mask2_t *gbfmask2)
{
	wtk_free(gbfmask2->g);
	wtk_free(gbfmask2->features);

	if(gbfmask2->gainnet)
	{
		wtk_gainnet2_delete(gbfmask2->gainnet);
	}
	if(gbfmask2->masknet)
	{
		wtk_masknet_delete(gbfmask2->masknet);
	}

}

void wtk_gainnet_bf6_mask2_reset(wtk_gainnet_bf6_mask2_t *gbfmask2)
{
	memset(gbfmask2->g, 0, sizeof(float)*(gbfmask2->nbin-2));

	memset(gbfmask2->features, 0, sizeof(float)*gbfmask2->features_len);

	if(gbfmask2->gainnet)
	{
		wtk_gainnet2_reset(gbfmask2->gainnet);
	}
	if(gbfmask2->masknet)
	{
		wtk_masknet_reset(gbfmask2->masknet);
	}
}

wtk_gainnet_bf6_t* wtk_gainnet_bf6_new(wtk_gainnet_bf6_cfg_t *cfg)
{
	wtk_gainnet_bf6_t *gainnet_bf6;
	int ang_num;

	gainnet_bf6=(wtk_gainnet_bf6_t *)wtk_malloc(sizeof(wtk_gainnet_bf6_t));
	gainnet_bf6->cfg=cfg;
	gainnet_bf6->ths=NULL;
	gainnet_bf6->notify=NULL;

	gainnet_bf6->mic=wtk_strbufs_new(gainnet_bf6->cfg->channel);

	gainnet_bf6->notch_mem=NULL;
	gainnet_bf6->memD=NULL;
	if(cfg->use_preemph)
	{
		gainnet_bf6->notch_mem=wtk_float_new_p2(cfg->channel,2);
		gainnet_bf6->memD=(float *)wtk_malloc(sizeof(float)*cfg->channel);
		gainnet_bf6->memX=0;
	}

	gainnet_bf6->nbin=cfg->wins/2+1;
	gainnet_bf6->window=wtk_malloc(sizeof(float)*cfg->wins);///2);
	gainnet_bf6->synthesis_window=wtk_malloc(sizeof(float)*cfg->wins);///2);
	gainnet_bf6->analysis_mem=wtk_float_new_p2(cfg->channel, gainnet_bf6->nbin-1);
	gainnet_bf6->synthesis_mem=wtk_malloc(sizeof(float)*(gainnet_bf6->nbin-1));
	gainnet_bf6->rfft=wtk_drft_new(cfg->wins);
	gainnet_bf6->rfft_in=(float*)wtk_malloc(sizeof(float)*(cfg->wins));

	gainnet_bf6->fft=wtk_complex_new_p2(cfg->channel, gainnet_bf6->nbin);
	gainnet_bf6->fft2=wtk_complex_new_p2(gainnet_bf6->nbin,cfg->channel);
	gainnet_bf6->ovec=wtk_complex_new_p2(gainnet_bf6->nbin, cfg->channel);

	gainnet_bf6->aspec_mask=(float*)wtk_malloc(sizeof(float)*gainnet_bf6->nbin);

	gainnet_bf6->fftx=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*gainnet_bf6->nbin);

	gainnet_bf6->gmask=NULL;
	gainnet_bf6->gmask2=NULL;
	if(cfg->use_premask && cfg->use_nbands)
	{
		gainnet_bf6->gmask=(wtk_gainnet_bf6_mask_t *)wtk_malloc(sizeof(wtk_gainnet_bf6_mask_t));
		wtk_gainnet_bf6_mask_init(gainnet_bf6->gmask, cfg, cfg->gainnet, cfg->masknet);
	}else if(cfg->use_premask)
	{
		gainnet_bf6->gmask2=(wtk_gainnet_bf6_mask2_t *)wtk_malloc(sizeof(wtk_gainnet_bf6_mask2_t));
		wtk_gainnet_bf6_mask2_init(gainnet_bf6->gmask2, cfg, cfg->gainnet, cfg->masknet);
	}
	gainnet_bf6->covm=wtk_covm_new(&(cfg->covm), gainnet_bf6->nbin, cfg->channel);
	gainnet_bf6->bf=wtk_bf_new(&(cfg->bf), gainnet_bf6->cfg->wins);

	ang_num=cfg->use_line?(floor(180/cfg->thstep)+1):(floor(359/cfg->thstep)+1);
    gainnet_bf6->aspec=wtk_aspec_new(&(cfg->aspec), gainnet_bf6->nbin, ang_num);        

	gainnet_bf6->angle_spec=(float *)wtk_malloc(sizeof(float)*ang_num);
	
    gainnet_bf6->cov=NULL;
    if(gainnet_bf6->aspec && gainnet_bf6->aspec->need_cov)
    {
        gainnet_bf6->cov=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->channel*cfg->channel);
        if(cfg->lf<=0)
        {
            gainnet_bf6->winf=wtk_malloc(sizeof(float));
            gainnet_bf6->winf[0]=1;
        }else
        {
            gainnet_bf6->winf=wtk_math_create_hanning_window(2*cfg->lf+1);
        }
    }

    gainnet_bf6->inv_cov=NULL;
    gainnet_bf6->tmp=NULL;
    if(gainnet_bf6->aspec && gainnet_bf6->aspec->need_inv_cov)
    {
        gainnet_bf6->inv_cov=(wtk_complex_t *)wtk_malloc(cfg->channel*cfg->channel*sizeof(wtk_complex_t));
        gainnet_bf6->tmp=(wtk_dcomplex_t *)wtk_malloc(cfg->channel*cfg->channel*2*sizeof(wtk_dcomplex_t));
    }

	gainnet_bf6->qmmse=NULL;
	if(cfg->use_qmmse)
	{
		gainnet_bf6->qmmse=wtk_qmmse_new(&(cfg->qmmse));
	}

	gainnet_bf6->out=wtk_malloc(sizeof(float)*(gainnet_bf6->nbin-1));

	wtk_gainnet_bf6_reset(gainnet_bf6);
	
	return gainnet_bf6;
}

void wtk_gainnet_bf6_delete(wtk_gainnet_bf6_t *gainnet_bf6)
{
	wtk_strbufs_delete(gainnet_bf6->mic,gainnet_bf6->cfg->channel);
	if(gainnet_bf6->notch_mem)
	{
		wtk_float_delete_p2(gainnet_bf6->notch_mem, gainnet_bf6->cfg->channel);
		wtk_free(gainnet_bf6->memD);
	}
	wtk_free(gainnet_bf6->window);
	wtk_free(gainnet_bf6->synthesis_window);
	wtk_float_delete_p2(gainnet_bf6->analysis_mem, gainnet_bf6->cfg->channel);
	wtk_free(gainnet_bf6->synthesis_mem);
	wtk_free(gainnet_bf6->rfft_in);
	wtk_drft_delete(gainnet_bf6->rfft);
	wtk_complex_delete_p2(gainnet_bf6->fft, gainnet_bf6->cfg->channel);
	wtk_complex_delete_p2(gainnet_bf6->fft2, gainnet_bf6->nbin);

	wtk_complex_delete_p2(gainnet_bf6->ovec, gainnet_bf6->nbin);

	wtk_free(gainnet_bf6->aspec_mask);
	wtk_free(gainnet_bf6->angle_spec);

	if(gainnet_bf6->covm)
	{
		wtk_covm_delete(gainnet_bf6->covm);
	}
	wtk_bf_delete(gainnet_bf6->bf);

	wtk_free(gainnet_bf6->fftx);

	if(gainnet_bf6->gmask)
	{
		wtk_gainnet_bf6_mask_clean(gainnet_bf6->gmask);
		wtk_free(gainnet_bf6->gmask);
	}
	if(gainnet_bf6->gmask2)
	{
		wtk_gainnet_bf6_mask2_clean(gainnet_bf6->gmask2);
		wtk_free(gainnet_bf6->gmask2);
	}
  	if(gainnet_bf6->cov)
    {
        wtk_free(gainnet_bf6->cov);
        wtk_free(gainnet_bf6->winf);
    }
    if(gainnet_bf6->inv_cov)
    {
        wtk_free(gainnet_bf6->inv_cov);
    }
    if(gainnet_bf6->tmp)
    {
        wtk_free(gainnet_bf6->tmp);
    }
	wtk_aspec_delete(gainnet_bf6->aspec);
	wtk_free(gainnet_bf6->out);

	if(gainnet_bf6->qmmse)
	{
		wtk_qmmse_delete(gainnet_bf6->qmmse);
	}

	wtk_free(gainnet_bf6);
}


void wtk_gainnet_bf6_flush_ovec(wtk_gainnet_bf6_t *gainnet_bf6,  wtk_complex_t *ovec, float **mic_pos, float sv, int rate, 
                                                        int theta2, int phi2, int k, float *tdoa)
{
	float x,y,z;
	float t;
	float *mic;
	int j;
	int channel=gainnet_bf6->cfg->channel;
	int win=(gainnet_bf6->nbin-1)*2;
	wtk_complex_t *ovec1;
    float theta,phi;

    phi=phi2*PI/180;
    theta=theta2*PI/180;
    x=cos(phi)*cos(theta);
    y=cos(phi)*sin(theta);
    z=sin(phi);

	for(j=0;j<channel;++j)
	{
		mic=mic_pos[j];
		tdoa[j]=(mic[0]*x+mic[1]*y+mic[2]*z)/sv;
	}
    x=1.0/sqrt(channel);
    ovec1=ovec;
    t=2*PI*rate*1.0/win*k;
    for(j=0;j<channel;++j)
    {
        ovec1[j].a=cos(t*tdoa[j])*x;
        ovec1[j].b=sin(t*tdoa[j])*x;
    }
}

void wtk_gainnet_bf6_start(wtk_gainnet_bf6_t *gainnet_bf6, float theta, float phi)
{
	int n,th;
	wtk_aspec_t *aspec=gainnet_bf6->aspec;
	int theta_range=gainnet_bf6->cfg->theta_range;
	int theta2, theta3;
	int thstep=gainnet_bf6->cfg->thstep;
	wtk_complex_t **ovec=gainnet_bf6->ovec;
    int channel=gainnet_bf6->cfg->channel;
    int nbin=gainnet_bf6->nbin;
    wtk_complex_t *a,*b;//,*b2;
    wtk_complex_t *cov, *cov1, *cov2;
    int i,j,k,bb;
    wtk_dcomplex_t *tmp_inv;
	wtk_complex_t *tmp;
    float *tdoa;//,fa;
	float ls_eye=gainnet_bf6->cfg->ls_eye;
	int max_th=gainnet_bf6->cfg->use_line?180:359;

	wtk_bf_update_ovec(gainnet_bf6->bf,theta,phi);
	wtk_bf_init_w(gainnet_bf6->bf);

	if(gainnet_bf6->cfg->use_line)
	{
		for(n=0, th=0; th<=180; th+=thstep,++n)
		{
			wtk_aspec_start(aspec, th, 0, n);
		}
		aspec->start_ang_num=n;
	}else
	{
		for(n=0, th=0; th<360; th+=thstep,++n)
		{
			wtk_aspec_start(aspec, th, 0, n);
		}
		aspec->start_ang_num=n;
	}


	if(gainnet_bf6->cfg->use_line)
	{
		if(theta==0 || theta==180)
		{
			theta2=theta+theta_range;
        	if(theta2>180)
        	{
            	theta2=360-theta2;
        	}
			if(theta2<theta)
			{
				gainnet_bf6->thetas=theta2;
				gainnet_bf6->thetad=theta;
			}else
			{
				gainnet_bf6->thetas=theta;
				gainnet_bf6->thetad=theta2;
			}
		}else
		{
			theta2=theta-theta_range;
			if(theta2<0)
			{
				theta2=0;
			}
			theta3=theta+theta_range;
			if(theta3>180)
        	{
            	theta3=180;
        	}
			gainnet_bf6->thetas=theta2;
			gainnet_bf6->thetad=theta3;
		}
	}else
	{
		theta2=theta-theta_range;
		if(theta2<0)
		{
			theta2+=360;
		}
		theta3=theta+theta_range;
		if(theta3>360)
		{
			theta3=360-theta3;
		}
		gainnet_bf6->thetas=theta2;
		gainnet_bf6->thetad=theta3;
	}

	tdoa=(float *)wtk_malloc(channel*sizeof(float));
	if(gainnet_bf6->cfg->use_lsbf)
	{
		tmp=(wtk_complex_t *)wtk_calloc(channel,sizeof(wtk_complex_t));
		cov=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*channel*channel);
		tmp_inv=(wtk_dcomplex_t *)wtk_malloc(channel*(channel+1)*sizeof(wtk_dcomplex_t));

		for(k=6;k<nbin-1;++k)
		{
			memset(cov,0,sizeof(wtk_complex_t)*channel*channel);
			memset(ovec[k],0,sizeof(wtk_complex_t)*channel);

			for(th=0; th<=max_th; th+=1)
			{
				bb=0;
				if(gainnet_bf6->thetas>gainnet_bf6->thetad)
				{
					if((th >= gainnet_bf6->thetas &&  th < 360) || ( th >= 0 && th <=  gainnet_bf6->thetad))
					{
						bb=1;
					}
				}else
				{
					if(th <=  gainnet_bf6->thetad && th >=  gainnet_bf6->thetas)
					{
						bb=1;
					}
				}

				wtk_gainnet_bf6_flush_ovec(gainnet_bf6, tmp, gainnet_bf6->cfg->bf.mic_pos,gainnet_bf6->cfg->bf.speed,gainnet_bf6->cfg->rate, th,0,k,tdoa);
				a=tmp;
				for(i=0;i<channel;++i,++a)
				{
					b=a;
					for(j=i;j<channel;++j,++b)
					{
						cov1=cov+i*channel+j;
						cov1->a+=a->a*b->a+a->b*b->b;
						if(i!=j)
						{
							cov1->b+=-a->a*b->b+a->b*b->a;
							cov2=cov+j*channel+i;
							cov2->a=cov1->a;
							cov2->b=-cov1->b;
						}else
						{
							cov1->b=0;
						}
					}
					if(bb==1)
					{
						ovec[k][i].a+=a->a;
						ovec[k][i].b+=a->b;
					}
				}
			}

			for(i=0,j=0;i<channel;++i)
			{
				cov[j].a+=ls_eye;
				j+=channel+1;
			}
			wtk_complex_guass_elimination_p1(cov, ovec[k], tmp_inv, channel, tmp);
			memcpy(ovec[k], tmp, sizeof(wtk_complex_t)*channel);
		}

		wtk_free(tmp);
		wtk_free(cov);
		wtk_free(tmp_inv);
	}else
	{
		th=theta;
		for(k=6;k<nbin-1;++k)
		{
			wtk_gainnet_bf6_flush_ovec(gainnet_bf6, ovec[k], gainnet_bf6->cfg->bf.mic_pos,gainnet_bf6->cfg->bf.speed,gainnet_bf6->cfg->rate, th,0,k,tdoa);
		}
	}
	wtk_free(tdoa);
}

void wtk_gainnet_bf6_reset(wtk_gainnet_bf6_t *gainnet_bf6)
{
	int wins=gainnet_bf6->cfg->wins;
	int frame_size=gainnet_bf6->cfg->wins/2;
	int i;
	int shift, nshift, j, n;

	wtk_strbufs_reset(gainnet_bf6->mic,gainnet_bf6->cfg->channel);
	if(gainnet_bf6->notch_mem)
	{
		for(i=0;i<gainnet_bf6->cfg->channel;++i)
		{
			memset(gainnet_bf6->notch_mem[i],0,2*sizeof(float));
		}
		memset(gainnet_bf6->memD,0,gainnet_bf6->cfg->channel*sizeof(float));
		gainnet_bf6->memX=0;
	}
	for (i=0;i<wins;++i)
	{
		gainnet_bf6->window[i] = sin((0.5+i)*PI/(wins));//sin(.5*PI*sin(.5*PI*(i+.5)/frame_size) * sin(.5*PI*(i+.5)/frame_size));
		gainnet_bf6->synthesis_window[i]=0;
	}
	shift=wins-frame_size;
	nshift=wins / shift;
	for(i=0;i<shift;++i)
	{
		for(j=0;j<nshift+1;++j)
		{
			n = i+j*shift;
			if(n < wins)
			{
				gainnet_bf6->synthesis_window[i] += gainnet_bf6->window[n]*gainnet_bf6->window[n];
			}
		}
	}
	for(i=1;i<nshift;++i)
	{
		for(j=0;j<shift;++j)
		{
			gainnet_bf6->synthesis_window[i*shift+j] = gainnet_bf6->synthesis_window[j];
		}
	}
	for(i=0;i<wins;++i)
	{
		gainnet_bf6->synthesis_window[i]=gainnet_bf6->window[i]/gainnet_bf6->synthesis_window[i];
	}

	wtk_float_zero_p2(gainnet_bf6->analysis_mem, gainnet_bf6->cfg->channel, (gainnet_bf6->nbin-1));
	memset(gainnet_bf6->synthesis_mem, 0, sizeof(float)*(gainnet_bf6->nbin-1));
	wtk_complex_zero_p2(gainnet_bf6->fft, gainnet_bf6->cfg->channel, gainnet_bf6->nbin);
	wtk_complex_zero_p2(gainnet_bf6->fft2, gainnet_bf6->nbin, gainnet_bf6->cfg->channel);
	wtk_complex_zero_p2(gainnet_bf6->ovec, gainnet_bf6->nbin, gainnet_bf6->cfg->channel);

	memset(gainnet_bf6->aspec_mask, 0, sizeof(float)*gainnet_bf6->nbin);

    if(gainnet_bf6->covm)
	{
		wtk_covm_reset(gainnet_bf6->covm);
	}
	wtk_bf_reset(gainnet_bf6->bf);

	wtk_aspec_reset(gainnet_bf6->aspec);

	memset(gainnet_bf6->fftx, 0, sizeof(wtk_complex_t)*(gainnet_bf6->nbin));

	if(gainnet_bf6->gmask)
	{
		wtk_gainnet_bf6_mask_reset(gainnet_bf6->gmask);
	}
	if(gainnet_bf6->gmask2)
	{
		wtk_gainnet_bf6_mask2_reset(gainnet_bf6->gmask2);
	}
	if(gainnet_bf6->qmmse)	
	{
		wtk_qmmse_reset(gainnet_bf6->qmmse);
	}
	gainnet_bf6->training=0;
}


void wtk_gainnet_bf6_set_notify(wtk_gainnet_bf6_t *gainnet_bf6,void *ths,wtk_gainnet_bf6_notify_f notify)
{
	gainnet_bf6->notify=notify;
	gainnet_bf6->ths=ths;
}

void wtk_gainnet_bf6_set_tr_notify(wtk_gainnet_bf6_t *gainnet_bf6,void *ths,wtk_gainnet_bf6_notify_trfeat_f notify)
{
	gainnet_bf6->notify_tr=notify;
	gainnet_bf6->ths_tr=ths;
}

static void compute_band_energy(float *bandE, int *eband, int nb_bands, wtk_complex_t *fft) 
{
    int i;
    int j;
    int band_size;
    float tmp;
    float frac;

    memset(bandE, 0, sizeof(float)*nb_bands);
    for (i=0;i<nb_bands-1;++i)
    {
        band_size = eband[i+1]-eband[i];
        for (j=0;j<band_size;j++) 
        {
            frac = (float)j/band_size;
            tmp = fft[eband[i] + j].a* fft[eband[i] + j].a+ fft[eband[i] + j].b* fft[eband[i] + j].b;
            bandE[i] += (1-frac)*tmp;
            bandE[i+1] += frac*tmp;
        }
    }
    bandE[0] *= 2;
    bandE[nb_bands-1] *= 2;
}

static void compute_band_energy2(float *bandE, int *eband, int nb_bands, wtk_complex_t *fft, float *mask) 
{
    int i;
    int j;
    int band_size;
    float tmp;
    float frac;

    memset(bandE, 0, sizeof(float)*nb_bands);
    for (i=0;i<nb_bands-1;++i)
    {
        band_size = eband[i+1]-eband[i];
        for (j=0;j<band_size;j++) 
        {
            frac = (float)j/band_size;
            tmp = (fft[eband[i] + j].a* fft[eband[i] + j].a+ fft[eband[i] + j].b* fft[eband[i] + j].b)*mask[eband[i] + j]*mask[eband[i] + j];
            bandE[i] += (1-frac)*tmp;
            bandE[i+1] += frac*tmp;
        }
    }
    bandE[0] *= 2;
    bandE[nb_bands-1] *= 2;
}

static void compute_band_gain(float *bandE, int *eband, int nb_bands, float *g) 
{
    int i;
    int j;
    int band_size;
    float tmp;
    float frac;

    memset(bandE, 0, sizeof(float)*nb_bands);
    for (i=0;i<nb_bands-1;++i)
    {
        band_size = eband[i+1]-eband[i];
        for (j=0;j<band_size;j++) 
        {
            frac = (float)j/band_size;
            tmp = g[eband[i] + j];
            bandE[i] += (1-frac)*tmp;
            bandE[i+1] += frac*tmp;
        }
    }
    bandE[0] *= 2;
    bandE[nb_bands-1] *= 2;
}


static void interp_band_gain(int *eband, int nb_bands, int nbin, float *g, const float *bandE)
{
  int i,j;
  int band_size;
  float frac;

  memset(g, 0, nbin*sizeof(float));
  for (i=0;i<nb_bands-1;++i)
  {
    band_size = eband[i+1]-eband[i];
    for (j=0;j<band_size;j++)
    {
      frac = (float)j/band_size;
      g[eband[i] + j] = (1-frac)*bandE[i] + frac*bandE[i+1];
    }
  }
}

static void dct(float *dct_table, int nb_bands, float *out, const float *in)
{
  int i,j;
  float sum;

  for (i=0;i<nb_bands;++i)
  {
    sum = 0;
    for (j=0;j<nb_bands;++j)
    {
      sum += in[j] * dct_table[j*nb_bands + i];
    }
    out[i] = sum*sqrt(2./nb_bands);
  }
}

static void inverse_transform(wtk_gainnet_bf6_t *gainnet_bf6, wtk_complex_t *fft, float *out)
{
	wtk_drft_ifft2(gainnet_bf6->rfft,fft,out);
}

static void forward_transform(wtk_gainnet_bf6_t *gainnet_bf6, wtk_complex_t *fft, float *in)
{
	wtk_drft_fft2(gainnet_bf6->rfft,in,fft);
}

static void frame_analysis(wtk_gainnet_bf6_t *gainnet_bf6, float *rfft_in, float *analysis_mem, wtk_complex_t *fft, const float *in)
{
  int i;
  int wins=gainnet_bf6->cfg->wins;
  int fsize=wins/2;

  memmove(rfft_in, analysis_mem, fsize*sizeof(float));
  for(i=0;i<fsize;++i)
  {
    rfft_in[i+fsize]=in[i];
  }
  memcpy(analysis_mem, in, fsize*sizeof(float));
	for (i=0;i<wins;++i)
	{
		rfft_in[i] *= gainnet_bf6->window[i];
		//rfft_in[wins - 1 - i] *= gainnet_bf6->window[i];
	}
  forward_transform(gainnet_bf6, fft, rfft_in);
}

static void frame_synthesis(wtk_gainnet_bf6_t *gainnet_bf6, float *out, wtk_complex_t *fft)
{
  float *rfft_in=gainnet_bf6->rfft_in;
  int i;
  int wins=gainnet_bf6->cfg->wins;
  int fsize=wins/2;
  float *synthesis_mem=gainnet_bf6->synthesis_mem;

  inverse_transform(gainnet_bf6, fft, rfft_in);
  for (i=0;i<wins;++i)
  {
		rfft_in[i] *= gainnet_bf6->synthesis_window[i];
    // rfft_in[i] *= gainnet_bf6->window[i];
    // rfft_in[wins - 1 - i] *= gainnet_bf6->window[i];
  }
  for (i=0;i<fsize;i++) out[i] = rfft_in[i] + synthesis_mem[i];
  memcpy(synthesis_mem, &rfft_in[fsize], fsize*sizeof(float));
}

static int compute_frame_mask_features(wtk_gainnet_bf6_mask_t *gbfmask, float *features, wtk_complex_t *fftx, float *aspec_mask)
{
	float *Ex=gbfmask->Ex;
	int *eband=gbfmask->eband;
	int i,j,k;
	float E = 0;
	float *ceps_0, *ceps_1, *ceps_2;
	float spec_variability = 0;
	float *Ly=gbfmask->Ly;
	int nb_bands=gbfmask->cfg->nb_bands;
	// int nb_features=gbfmask->cfg->nb_features;
	int nb_features_x=gbfmask->cfg->nb_features_x;
	int nb_delta_ceps=gbfmask->cfg->nb_delta_ceps;
	int ceps_mem=gbfmask->cfg->ceps_mem;
	float follow, logMax;
	float mindist,dist,tmp;
	float *gpre;

	compute_band_energy(Ex,eband, nb_bands, fftx);
	logMax = -2;
	follow = -2;
	for (i=0;i<nb_bands;i++)
	{
		Ly[i] = log10(1e-2+Ex[i]);
		Ly[i] = max(logMax-7, max(follow-1.5, Ly[i]));
		logMax = max(logMax, Ly[i]);
		follow = max(follow-1.5, Ly[i]);
		E += Ex[i];
	}
	dct(gbfmask->dct_table, nb_bands, features, Ly);

	if(gbfmask->cepstral_mem)
	{
		features[0] -= 12;
		features[1] -= 4;
		ceps_0 = gbfmask->cepstral_mem[gbfmask->memid];
		ceps_1 = (gbfmask->memid < 1) ? gbfmask->cepstral_mem[ceps_mem+gbfmask->memid-1] : gbfmask->cepstral_mem[gbfmask->memid-1];
		ceps_2 = (gbfmask->memid < 2) ? gbfmask->cepstral_mem[ceps_mem+gbfmask->memid-2] : gbfmask->cepstral_mem[gbfmask->memid-2];
		for (i=0;i<nb_bands;i++)
		{
			ceps_0[i] = features[i];
		}
		gbfmask->memid++;
		for (i=0;i<nb_delta_ceps;i++)
		{
			features[i] = ceps_0[i] + ceps_1[i] + ceps_2[i];
			features[nb_bands+i] = ceps_0[i] - ceps_2[i];
			features[nb_bands+nb_delta_ceps+i] =  ceps_0[i] - 2*ceps_1[i] + ceps_2[i];
		}
		/* Spectral variability features. */
		if (gbfmask->memid == ceps_mem)
		{
			gbfmask->memid = 0;
		}
		for (i=0;i<ceps_mem;++i)
		{
			mindist = 1e15f;
			for (j=0;j<ceps_mem;++j)
			{
					dist=0;
				for (k=0;k<nb_bands;++k)
				{
					tmp = gbfmask->cepstral_mem[i][k] - gbfmask->cepstral_mem[j][k];
					dist += tmp*tmp;
				}
				if (j!=i)
				{
					mindist = min(mindist, dist);
				}
			}
			spec_variability += mindist;
		}
		features[nb_bands+2*nb_delta_ceps] = spec_variability/ceps_mem-2.1;
	}
	
	// features[nb_features-nb_bands-1]=wtk_float_mean(aspec_mask+1,gbfmask->nbin-2);

	gpre=features+nb_features_x;
	if(gbfmask->cfg->use_spec2)
	{
		compute_band_energy2(gpre,eband, nb_bands, fftx, aspec_mask);
		for (i=0;i<nb_bands;++i)
		{
			if(E < 0.1 || Ex[i] < 5e-2)// || gpre[i] < 5e-2)
			{
				gpre[i]=0;
			}else
			{
				gpre[i] = sqrt(gpre[i]/Ex[i]);
			}
			if (gpre[i] > 1) gpre[i] = 1;
		}
	}else
	{
		compute_band_gain(gpre, eband, nb_bands, aspec_mask);
	}

	return E < 0.1;
}

static int compute_frame_mask_features2(wtk_gainnet_bf6_mask_t *gbfmask, float *features, wtk_complex_t *fftx)
{
	float *Ex2=gbfmask->Ex2;
	int *eband=gbfmask->eband;
	int i,j,k;
	float E = 0;
	float *ceps_0, *ceps_1, *ceps_2;
	float spec_variability = 0;
	float *Ly=gbfmask->Ly;
	int nb_bands=gbfmask->cfg->nb_bands;
	int nb_delta_ceps=gbfmask->cfg->nb_delta_ceps;
	int ceps_mem=gbfmask->cfg->ceps_mem;
	float follow, logMax;
	float mindist,dist,tmp;

	compute_band_energy(Ex2,eband, nb_bands, fftx);
	logMax = -2;
	follow = -2;
	for (i=0;i<nb_bands;i++)
	{
		Ly[i] = log10(1e-2+Ex2[i]);
		Ly[i] = max(logMax-7, max(follow-1.5, Ly[i]));
		logMax = max(logMax, Ly[i]);
		follow = max(follow-1.5, Ly[i]);
		E += Ex2[i];
	}
	dct(gbfmask->dct_table, nb_bands, features, Ly);

	if(gbfmask->cepstral_mem2)
	{
		features[0] -= 12;
		features[1] -= 4;
		ceps_0 = gbfmask->cepstral_mem2[gbfmask->memid2];
		ceps_1 = (gbfmask->memid2 < 1) ? gbfmask->cepstral_mem2[ceps_mem+gbfmask->memid2-1] : gbfmask->cepstral_mem2[gbfmask->memid2-1];
		ceps_2 = (gbfmask->memid2 < 2) ? gbfmask->cepstral_mem2[ceps_mem+gbfmask->memid2-2] : gbfmask->cepstral_mem2[gbfmask->memid2-2];
		for (i=0;i<nb_bands;i++)
		{
			ceps_0[i] = features[i];
		}
		gbfmask->memid2++;
		for (i=0;i<nb_delta_ceps;i++)
		{
			features[i] = ceps_0[i] + ceps_1[i] + ceps_2[i];
			features[nb_bands+i] = ceps_0[i] - ceps_2[i];
			features[nb_bands+nb_delta_ceps+i] =  ceps_0[i] - 2*ceps_1[i] + ceps_2[i];
		}
		/* Spectral variability features. */
		if (gbfmask->memid2 == ceps_mem)
		{
			gbfmask->memid2 = 0;
		}
		for (i=0;i<ceps_mem;++i)
		{
			mindist = 1e15f;
			for (j=0;j<ceps_mem;++j)
			{
					dist=0;
				for (k=0;k<nb_bands;++k)
				{
					tmp = gbfmask->cepstral_mem2[i][k] - gbfmask->cepstral_mem2[j][k];
					dist += tmp*tmp;
				}
				if (j!=i)
				{
					mindist = min(mindist, dist);
				}
			}
			spec_variability += mindist;
		}
		features[nb_bands+2*nb_delta_ceps] = spec_variability/ceps_mem-2.1;
	}

	return E < 0.1;
}

void wtk_gainnet_bf6_mask_on_gainnet(wtk_gainnet_bf6_mask_t *gbfmask, float *gain, int len, int is_end)
{
	memcpy(gbfmask->g, gain, sizeof(float)*gbfmask->cfg->nb_bands);
}


void wtk_gainnet_bf6_mask_feed(wtk_gainnet_bf6_t *gainnet_bf6, wtk_complex_t **ifft, float *aspec_mask)
{
	int i,k;
	wtk_gainnet_bf6_mask_t *gmask=gainnet_bf6->gmask;
	int nb_bands=gmask->cfg->nb_bands;
	int nbin=gmask->nbin;
	float *g=gmask->g, *gf=gmask->gf;
	// float *gf2=gmask->gf2;
	int *eband=gmask->eband;
	wtk_gainnet2_t *gainnet=gmask->gainnet;
	wtk_masknet_t *masknet=gmask->masknet;
	int nb_features=gmask->cfg->nb_features;
	int nb_features_x=gmask->cfg->nb_features_x;
	float *qmmse_gain;
	wtk_complex_t **ovec=gainnet_bf6->ovec, *ovec2;
	int channel=gainnet_bf6->cfg->channel;
	wtk_complex_t *fftx=gainnet_bf6->fftx;

	memset(fftx,0,sizeof(wtk_complex_t)*nbin);
	for(k=1;k<nbin-1;++k)
	{
		ovec2=ovec[k];
		for(i=0; i<channel; ++i, ++ovec2)
		{
			fftx[k].a+=ovec2->a*ifft[i][k].a + ovec2->b*ifft[i][k].b;
			fftx[k].b+=ovec2->a*ifft[i][k].b - ovec2->b*ifft[i][k].a;
		}
	}
	compute_frame_mask_features2(gmask, gmask->features, fftx);
	ifft[0][0].a=ifft[0][0].b=0;
	ifft[0][nbin-1].a=ifft[0][nbin-1].b=0;
    gmask->silence=compute_frame_mask_features(gmask, gmask->features+nb_features_x, ifft[0], aspec_mask);
	if(!gainnet_bf6->training)
	{
		if(gainnet_bf6->qmmse)
		{
			wtk_qmmse_update_mask(gainnet_bf6->qmmse, ifft[0], aspec_mask);
		}
		if(gmask->silence)
		{
			for (i=1; i<nbin-1; ++i)
			{
				gf[i]=0;
			}
		}else
		{
			if(gainnet)
			{
				wtk_gainnet2_feed(gainnet, gmask->features, 0, nb_features, 0); 
			}else if(masknet)
			{
				wtk_masknet_feed(masknet, gmask->features, nb_features, 0); 
			}
			interp_band_gain(eband, nb_bands, nbin, gf, g);
			if(gainnet_bf6->qmmse)
			{
				qmmse_gain=gainnet_bf6->qmmse->gain;
				for (i=1; i<nbin-1; ++i)
				{
					if(gf[i]>qmmse_gain[i])
					{
						gf[i]=qmmse_gain[i];
					}
				}
			}
		}
		for (i=1; i<nbin-1; ++i)
		{
			aspec_mask[i]=gf[i];
		}	
	}
}

void wtk_gainnet_bf6_mask2_on_gainnet(wtk_gainnet_bf6_mask2_t *gbfmask2, float *gain, int len, int is_end)
{
	memcpy(gbfmask2->g, gain, sizeof(float)*(gbfmask2->nbin-2));
}

void wtk_gainnet_bf6_mask2_feed(wtk_gainnet_bf6_t *gainnet_bf6, wtk_complex_t **fftx, float *aspec_mask2)
{
	int i,k;
	wtk_gainnet_bf6_mask2_t *gmask2=gainnet_bf6->gmask2;
	int nbin=gmask2->nbin;
	float *g=gmask2->g, ff2;
	wtk_complex_t **ovec=gainnet_bf6->ovec, *ovec2;
	wtk_gainnet2_t *gainnet=gmask2->gainnet;
	wtk_masknet_t *masknet=gmask2->masknet;
	int features_len=gmask2->features_len;
	float *features=gmask2->features;
	float *qmmse_gain;
	int channel=gainnet_bf6->cfg->channel;
	float ta,tb;

	for(k=1; k<nbin-1; ++k)
	{
		ff2=sqrtf(fftx[0][k].a*fftx[0][k].a+fftx[0][k].b*fftx[0][k].b);
		features[k-1] = logf(1e-2+ff2);

		ovec2=ovec[k];
		ta=tb=0;
		for(i=0; i<channel; ++i, ++ovec2)
		{
			ta+=ovec2->a*fftx[i][k].a + ovec2->b*fftx[i][k].b;
			tb+=ovec2->a*fftx[i][k].b - ovec2->b*fftx[i][k].a;
		}
		ff2=sqrt(ta*ta+tb*tb);
		features[nbin-2+k-1] = logf(1e-2+ff2);

		// for(i=1;i<channel;++i)
		// {
		// 	ta=fftx[i][k].a*fftx[0][k].a + fftx[i][k].b*fftx[0][k].b;
		// 	tb=fftx[i][k].b*fftx[0][k].a - fftx[i][k].a*fftx[0][k].b;
		// 	features[2*(nbin-2)+(i-1)*(nbin-2)+k-1] = atan2f(tb, ta);
		// }
	}
	// memcpy(features+(nbin-2)*(2+channel-1), aspec_mask2+1, sizeof(float)*(nbin-2));
	memcpy(features+(nbin-2)*2, aspec_mask2+1, sizeof(float)*(nbin-2));
	if(!gainnet_bf6->training)
	{
		if(gainnet_bf6->qmmse)
		{
			wtk_qmmse_flush_denoise_mask(gainnet_bf6->qmmse, fftx[0]);
		}
		if(gainnet)
		{
			wtk_gainnet2_feed(gainnet, gmask2->features, 0, features_len, 0); 
		}else if(masknet)
		{
			wtk_masknet_feed(masknet, gmask2->features, features_len, 0); 
		}
		if(gainnet_bf6->qmmse)
		{
			qmmse_gain=gainnet_bf6->qmmse->gain;
			for (k=1; k<nbin-1; ++k)
			{
				if(g[k-1]>qmmse_gain[k])
				{
					g[k-1]=qmmse_gain[k];
				}
			}
		}
		for (k=1; k<nbin-1; ++k)
		{
			aspec_mask2[k]=g[k-1];
		}	
	}
}

void wtk_gainnet_bf6_flush2(wtk_gainnet_bf6_t *gainnet_bf6)
{
	int i,k;
    int nbin=gainnet_bf6->bf->nbin;
    wtk_complex_t *fftx=gainnet_bf6->fftx;
	int wins=gainnet_bf6->cfg->wins;
	int fsize=wins/2;
	float *out=gainnet_bf6->out;
	short *pv=(short *)out;
	int clip_s=gainnet_bf6->cfg->clip_s;
	int clip_e=gainnet_bf6->cfg->clip_e;

	for(k=1; k<=clip_s; ++k)
	{
		fftx[k].a=fftx[k].b=0;
	}
	for(k=clip_e; k<nbin-1; ++k)
	{
		fftx[k].a=fftx[k].b=0;
	}

	frame_synthesis(gainnet_bf6, out, fftx);
	if(gainnet_bf6->notch_mem)
	{
		gainnet_bf6->memX=wtk_preemph_asis2(out,fsize,gainnet_bf6->memX);
	}
	for(i=0; i<fsize; ++i)
	{
		pv[i]=floorf(out[i]+0.5);
	}
	if(gainnet_bf6->notify)
	{
		gainnet_bf6->notify(gainnet_bf6->ths,pv,fsize);
	}
}

void wtk_gainnet_bf6_feed_agc(wtk_gainnet_bf6_mask_t *gmask, wtk_complex_t *fftx)
{
	float *Ex=gmask->Ex;
	int *eband=gmask->eband;
	int i;
	int nbin=gmask->nbin;
	int nb_bands=gmask->cfg->nb_bands;
	float *g2=gmask->g2;
	float *gf2=gmask->gf2;

	compute_band_energy(g2, eband, nb_bands, fftx);
	for (i=0; i<nb_bands; ++i)
	{
		if(gmask->silence || Ex[i] < 5e-2)
		{
			g2[i]=0;
		}else
		{
			g2[i] = sqrt((g2[i])/(Ex[i]));
		}
	}
	if(!gmask->silence)
	{
		// wtk_gainnet2_feed_agc(gmask->gainnet, g2, nb_bands, 0); 
		// gf2m=wtk_float_abs_mean(gf2+1, nbin-2);
		for (i=1;i<nbin-1;++i)
		{
			fftx[i].a *= gf2[i];
			fftx[i].b *= gf2[i];
		}
	}
}


void wtk_gainnet_bf6_flush(wtk_gainnet_bf6_t *gainnet_bf6,wtk_complex_t **fft)
{
	int k;
    int nbin=gainnet_bf6->bf->nbin;
    wtk_bf_t *bf=gainnet_bf6->bf;
    int i, channel=bf->channel;
    int b;
    wtk_covm_t *covm=gainnet_bf6->covm;
    wtk_complex_t *fftx=gainnet_bf6->fftx;
    wtk_complex_t *fft1;
	wtk_complex_t ffty[64], ffts[64];
	float *aspec_mask=gainnet_bf6->aspec_mask;

	fftx[0].a=fftx[0].b=0;
	fftx[nbin-1].a=fftx[nbin-1].b=0;
    for(k=1; k<nbin-1; ++k)
    {
        fft1=fft[k];
		for(i=0; i<channel; ++i)
		{
			ffts[i].a = fft1[i].a*aspec_mask[k];
			ffts[i].b = fft1[i].b*aspec_mask[k];

			ffty[i].a = fft1[i].a*(1-aspec_mask[k]);
			ffty[i].b = fft1[i].b*(1-aspec_mask[k]);
		}
		b=wtk_covm_feed_fft3(covm, ffts, k, 0);
		if(b==1)
		{
			wtk_bf_update_scov(bf, covm->scov, k);
		}
		b=wtk_covm_feed_fft3(covm, ffty, k, 1);
		if(b==1)
		{
			wtk_bf_update_ncov(bf, covm->ncov, k);
		}
        if(covm->ncnt_sum[k]>0 && (covm->scnt_sum==NULL ||  covm->scnt_sum[k]>0) && b==1)
        {
            wtk_bf_update_w(bf, k);
        }
        wtk_bf_output_fft_k(bf, fft1, fftx+k, k);
		// fftx[k]=ffts[0];
    }
	wtk_gainnet_bf6_flush2(gainnet_bf6);
}

// void wtk_gainnet_bf6_update_aspec2(wtk_gainnet_bf6_t *gainnet_bf6, wtk_aspec_t *aspec, wtk_complex_t *cov, 
//                                                                                     wtk_complex_t *inv_cov, float cov_travg, int k, float *aspec_mask)
// {
//     int sang_num=aspec->start_ang_num;
//     int n,m,m2;
// 	int th,b;
// 	int thstep=gainnet_bf6->cfg->thstep;
// 	float sum,sum2;
// 	// float spec;
// 	int thetas=gainnet_bf6->thetas;
// 	int thetad=gainnet_bf6->thetad;
// 	float mask;
// 	float *angle_spec=gainnet_bf6->angle_spec;

//     for(n=0, th=0; n<sang_num; ++n, th+=thstep)
//     {
// 		angle_spec[n]=wtk_aspec_flush_spec_k(aspec, NULL, 0, cov_travg, cov, inv_cov, k ,n);
// 		// angle_spec[n]=max(angle_spec[n],0);
// 	}

// 	sum2=sum=0;
// 	m=m2=0;
//     if(gainnet_bf6->cfg->use_line)
//     {
//         for(n=0,th=0; n<sang_num; ++n,th+=thstep)
//         {
//             b=0;
//             if(n==0)
//             {
//                 if(angle_spec[0]>=angle_spec[1])
//                 {
//                     b=1;
//                 }
//             }else if(n==sang_num-1)
//             {
//                 if(angle_spec[sang_num-1]>=angle_spec[sang_num-2])
//                 {
//                     b=1;
//                 }
//             }else
//             {
//                 if(angle_spec[n]>=angle_spec[n-1] && angle_spec[n]>=angle_spec[n+1])
//                 {
//                     b=1;
//                 }
//             }
//             if(b)
//             {
// 				sum+=angle_spec[n];
// 				++m;
//             } 
// 			if(th <= thetad && th >= thetas)
// 			{
// 				sum2+=angle_spec[n];
// 				++m2;
// 			}
// 		}
// 		// printf("%d %d\n",m,m2);
//     }else
// 	{
// 		for(n=0,th=0; n<sang_num; ++n,th+=thstep)
//         {
//             b=0;
// 			if(n==0)
// 			{
// 				if(angle_spec[0]>=angle_spec[1] && angle_spec[0]>=angle_spec[sang_num-1])
// 				{
// 					b=1;
// 				}
// 			}else if(n==sang_num-1)
// 			{
// 				if(angle_spec[sang_num-1]>=angle_spec[sang_num-2] && angle_spec[sang_num-1]>=angle_spec[0])
// 				{
// 					b=1;
// 				}
// 			}else
// 			{
// 				if(angle_spec[n]>=angle_spec[n-1] && angle_spec[n]>=angle_spec[n+1])
// 				{
// 					b=1;
// 				}
// 			}
//             if(b)
//             {
// 				sum+=angle_spec[n];
// 				++m;
//             } 
// 			if(thetas>thetad)
// 			{
// 				if((th >= thetas &&  th < 360) || ( th >= 0 && th <= thetad))
// 				{
// 					sum2+=angle_spec[n];
// 					++m2;
// 				}
// 			}else
// 			{
// 				if(th <= thetad && th >= thetas)
// 				{
// 					sum2+=angle_spec[n];
// 					++m2;
// 				}
// 			}
// 		}
// 	}

// 	if(gainnet_bf6->cfg->use_specmean && m>0 && m2>0)
// 	{
// 		sum2/=m2;
// 		sum/=m;
// 	}
// 	if(sum>0)
// 	{
// 		mask=sum2/sum;
// 		mask=min(1, mask);
// 		mask=max(0, mask);
// 	}else
// 	{
// 		mask=0;
// 	}
// 	*aspec_mask=mask;
// }


void wtk_gainnet_bf6_update_aspec2(wtk_gainnet_bf6_t *gainnet_bf6, wtk_aspec_t *aspec, wtk_complex_t *cov, 
                                                                                    wtk_complex_t *inv_cov, float cov_travg, int k, float *aspec_mask)
{
    int sang_num=aspec->start_ang_num;
    int n,m;
	int th;
	int thstep=gainnet_bf6->cfg->thstep;
	float sum,sum2;
	float spec;
	int thetas=gainnet_bf6->thetas;
	int thetad=gainnet_bf6->thetad;
	float mask;

	sum2=sum=0;
	m=0;
    for(n=0, th=0; n<sang_num; ++n, th+=thstep)
    {
		spec=wtk_aspec_flush_spec_k(aspec, NULL, 0, cov_travg, cov, inv_cov, k ,n);
		spec=max(spec,0);
		if(thetas>thetad)
		{
			if((th >= thetas &&  th < 360) || ( th >= 0 && th <= thetad))
			{
				sum2+=spec;
				++m;
			}
		}else
		{
			if(th <= thetad && th >= thetas)
			{
				sum2+=spec;
				++m;
			}
		}
		sum+=spec;
	}
	if(gainnet_bf6->cfg->use_specmean)
	{
		sum2/=m;
		sum/=sang_num;
	}
	if(sum>0)
	{
		mask=sum2/sum;
		mask=min(1, mask);
		mask=max(0, mask);
	}else
	{
		mask=0;
	}
	*aspec_mask=mask;
}


void wtk_gainnet_bf6_flush_aspec_lt(wtk_gainnet_bf6_t *gainnet_bf6, wtk_complex_t **ffts)
{
    int lf=gainnet_bf6->cfg->lf;\
    int i,j,k,k2,ff;
    int nbin=gainnet_bf6->nbin;
    int channel=gainnet_bf6->bf->channel;
    wtk_complex_t *cov=gainnet_bf6->cov;
    wtk_complex_t *ffts1,*ffts2,*a,*b;
    float *winf=gainnet_bf6->winf;
    float wintf,winsum;
    wtk_complex_t *inv_cov=gainnet_bf6->inv_cov;
    wtk_dcomplex_t *tmp=gainnet_bf6->tmp;
    float cov_travg;
    int ret;
    float *aspec_mask=gainnet_bf6->aspec_mask;
    float specsum;
	int specsum_fs=gainnet_bf6->cfg->specsum_fs;
    int specsum_fe=gainnet_bf6->cfg->specsum_fe;
	wtk_complex_t **ifft=gainnet_bf6->fft;

    specsum=0;
	aspec_mask[0]=aspec_mask[nbin-1]=0;
	for(k=1;k<nbin-1;++k)
    {
        memset(cov,0,sizeof(wtk_complex_t)*channel*channel);
        winsum=0;
		for(k2=max(1,k-lf),ff=k2-(k-lf);k2<min(nbin-1,k+lf+1);++k2,++ff)
		{
			wintf=winf[ff];
			winsum+=wintf;

			ffts1=ffts2=ffts[k2];
			for(i=0;i<channel;++i,++ffts1)
			{
				ffts2=ffts1;
				for(j=i;j<channel;++j,++ffts2)
				{
					a=cov+i*channel+j;
					if(i!=j)
					{
						a->a+=(ffts1->a*ffts2->a+ffts1->b*ffts2->b)*wintf;
						a->b+=(-ffts1->a*ffts2->b+ffts1->b*ffts2->a)*wintf;
					}else
					{
						a->a+=(ffts1->a*ffts2->a+ffts1->b*ffts2->b)*wintf;
						a->b+=0;
					}
				}
			}
		}
        winsum=1.0/winsum;
        for(i=0;i<channel;++i)
        {
            for(j=i;j<channel;++j)
            {
                a=cov+i*channel+j;
                a->a*=winsum;
                a->b*=winsum;

                if(i!=j)
                {
                    b=cov+j*channel+i;
                    b->a=a->a;
                    b->b=-a->b;
                }
            }
        }
        if(inv_cov)
        {
            ret=wtk_complex_invx4(cov,tmp,channel,inv_cov,1);            
            if(ret!=0)
            {
                j=0;
                for(i=0;i<channel;++i)
                {
                    cov[j].a+=0.01;
                    j+=channel+1;
                }
                wtk_complex_invx4(cov,tmp,channel,inv_cov,1);
            }
        }

        cov_travg=0;
        if(gainnet_bf6->aspec->need_cov_travg) 
        {
            for(i=0;i<channel;++i)
            {
                cov_travg+=cov[i*channel+i].a;
            }
            cov_travg/=channel;
        }

        wtk_gainnet_bf6_update_aspec2(gainnet_bf6, gainnet_bf6->aspec, cov, inv_cov, cov_travg, k, aspec_mask+k);
		if(k>=specsum_fs && k<=specsum_fe)
		{
			specsum+=aspec_mask[k];
		}
    }
		// print_float2(aspec_mask, nbin);
	if(gainnet_bf6->gmask)
	{
		wtk_gainnet_bf6_mask_feed(gainnet_bf6, ifft, aspec_mask);
	}else if(gainnet_bf6->gmask2)
	{
		wtk_gainnet_bf6_mask2_feed(gainnet_bf6, ifft, aspec_mask);
	}
	if(!gainnet_bf6->training)
	{
		wtk_gainnet_bf6_flush(gainnet_bf6, ffts);
	}
}

void wtk_gainnet_bf6_update_aspec(wtk_gainnet_bf6_t *gainnet_bf6, wtk_aspec_t *aspec, wtk_complex_t **fft, float fftabs2, int k, float *aspec_mask)
{
    int sang_num=aspec->start_ang_num;
    int n,th,m;
	int thstep=gainnet_bf6->cfg->thstep;
	float sum,sum2;
	float spec,mask;
	int thetas=gainnet_bf6->thetas;
	int thetad=gainnet_bf6->thetad;

	sum=sum2=0;
	m=0;
    for(n=0, th=0; n<sang_num; ++n, th+=thstep)
    {
		spec=wtk_aspec_flush_spec_k(aspec, fft, fftabs2, 0, NULL, NULL, k ,n);
		spec=max(spec,0);
		if(thetas>thetad)
		{
			if((th >= thetas &&  th < 360) || ( th >= 0 && th <= thetad))
			{
				sum2+=spec;
				++m;
			}
		}else
		{
			if(th <= thetad && th >= thetas)
			{
				sum2+=spec;
				++m;
			}
		}
		sum+=spec;
	}
	if(gainnet_bf6->cfg->use_specmean)
	{
		sum2/=m;
		sum/=sang_num;
	}
	if(sum>0)
	{
		mask=sum2/sum;
		mask=min(1, mask);
		mask=max(0, mask);
	}else
	{
		mask=0;
	}
	*aspec_mask=mask;
}

void wtk_gainnet_bf6_flush_aspec(wtk_gainnet_bf6_t *gainnet_bf6, wtk_complex_t **ffts)
{
    int k,i;
    int nbin=gainnet_bf6->nbin;
    int channel=gainnet_bf6->bf->channel;
    wtk_complex_t *fft2;
    float fftabs2;
    float specsum;
	int specsum_fs=gainnet_bf6->cfg->specsum_fs;
    int specsum_fe=gainnet_bf6->cfg->specsum_fe;
	float *aspec_mask=gainnet_bf6->aspec_mask;
	wtk_complex_t **ifft=gainnet_bf6->fft;

	specsum=0;
	aspec_mask[0]=aspec_mask[nbin-1]=0;
    for(k=1; k<nbin-1; ++k)
    {
        fftabs2=0;
        fft2=ffts[k];
        for(i=0; i<channel; ++i,++fft2)
        {
            fftabs2+=fft2->a*fft2->a+fft2->b*fft2->b;
        }

        wtk_gainnet_bf6_update_aspec(gainnet_bf6,gainnet_bf6->aspec,ffts,fftabs2,k,aspec_mask+k);
		if(k>=specsum_fs && k<=specsum_fe)
		{
			specsum+=aspec_mask[k];
		}
    }
	if(gainnet_bf6->gmask)
	{
		wtk_gainnet_bf6_mask_feed(gainnet_bf6, ifft, aspec_mask);
	}else if(gainnet_bf6->gmask2)
	{
		wtk_gainnet_bf6_mask2_feed(gainnet_bf6, ifft, aspec_mask);
	}
	if(!gainnet_bf6->training)
	{
		wtk_gainnet_bf6_flush(gainnet_bf6, ffts);
	}
}


void wtk_gainnet_bf6_feed_bf(wtk_gainnet_bf6_t *gainnet_bf6, wtk_complex_t **fft)
{
    int nbin=gainnet_bf6->nbin;
    int channel=gainnet_bf6->cfg->channel;
    int i,k;
	wtk_complex_t **fft2=gainnet_bf6->fft2;//,*fft3;
	// wtk_complex_t **ovec=gainnet_bf6->ovec, *ovec2;
	// wtk_complex_t *beam_fft=gainnet_bf6->beam_fft;
	// float ta,tb;

	for(i=0; i<channel; ++i)
	{
		for(k=0; k<nbin; ++k)
		{
			fft2[k][i]=fft[i][k];
		}
	}
	// for(k=0; k<nbin; ++k, ++beam_fft)
	// {
	// 	ovec2=ovec[k];
	// 	fft3=fft2[k];
	// 	ta=tb=0;
	// 	for(i=0; i<channel; ++i, ++ovec2, ++fft3)
	// 	{
	// 		ta+=ovec2->a*fft3->a + ovec2->b*fft3->b;
	// 		tb+=ovec2->a*fft3->b - ovec2->b*fft3->a;
	// 	}
	// 	beam_fft->a=ta;
	// 	beam_fft->b=tb;
	// }
    if(gainnet_bf6->cov)
	{   
		wtk_gainnet_bf6_flush_aspec_lt(gainnet_bf6,fft2);
    }else
    {
        wtk_gainnet_bf6_flush_aspec(gainnet_bf6,fft2);
    }   
}

void wtk_gainnet_bf6_feed(wtk_gainnet_bf6_t *gainnet_bf6,short **data,int len,int is_end)
{
	int i,j;
	// int nbin=gainnet_bf6->nbin;
	int channel=gainnet_bf6->cfg->channel;
	wtk_strbuf_t **mic=gainnet_bf6->mic;
	float **notch_mem=gainnet_bf6->notch_mem;
	float *memD=gainnet_bf6->memD;
	float fv, *fp1;
	int wins=gainnet_bf6->cfg->wins;
	int fsize=wins/2;
	int length;
	float *rfft_in=gainnet_bf6->rfft_in;
	wtk_complex_t **fft=gainnet_bf6->fft;
	float **analysis_mem=gainnet_bf6->analysis_mem;

	for(j=0; j<channel; ++j)
	{	
		for(i=0;i<len;++i)
		{
			fv=data[j][i];
			wtk_strbuf_push(mic[j],(char *)&(fv),sizeof(float));
		}
	}
	length=mic[0]->pos/sizeof(float);
	while(length>=fsize)
	{
		for(i=0; i<channel; ++i)
		{
			fp1=(float *)mic[i]->data;
			if(notch_mem)
			{
				wtk_preemph_dc(fp1, notch_mem[i], fsize);
				memD[i]=wtk_preemph_asis(fp1, fsize, memD[i]);
			}
			frame_analysis(gainnet_bf6, rfft_in, analysis_mem[i], fft[i], fp1);
		}
		wtk_gainnet_bf6_feed_bf(gainnet_bf6, fft);

		wtk_strbufs_pop(mic, channel, fsize*sizeof(float));
		length=mic[0]->pos/sizeof(float);
	}
}

void wtk_gainnet_bf6_reset2(wtk_gainnet_bf6_t *gainnet_bf6)
{
	int i;

	wtk_strbufs_reset(gainnet_bf6->mic,gainnet_bf6->cfg->channel);
	if(gainnet_bf6->notch_mem)
	{
		for(i=0;i<gainnet_bf6->cfg->channel;++i)
		{
			memset(gainnet_bf6->notch_mem[i],0,2*sizeof(float));
		}
		memset(gainnet_bf6->memD,0,gainnet_bf6->cfg->channel*sizeof(float));
		gainnet_bf6->memX=0;
	}

	wtk_float_zero_p2(gainnet_bf6->analysis_mem, gainnet_bf6->cfg->channel, (gainnet_bf6->nbin-1));
	wtk_complex_zero_p2(gainnet_bf6->fft, gainnet_bf6->cfg->channel, gainnet_bf6->nbin);

	memset(gainnet_bf6->fftx, 0, sizeof(wtk_complex_t)*(gainnet_bf6->nbin));

	if(gainnet_bf6->qmmse)	
	{
		wtk_qmmse_reset(gainnet_bf6->qmmse);
	}
}


void wtk_gainnet_bf6_feed_train3(wtk_gainnet_bf6_t *gainnet_bf6,short **data,short **data2,short **datar,int len, int bb)
{
	int channel=gainnet_bf6->cfg->channel;
	int fsize=gainnet_bf6->cfg->wins/2;
	int nbin=gainnet_bf6->nbin;
	int i,j;
	float **xn;
	float **xr;
	float *g;
	float **Er;
	wtk_complex_t **fftr;
	wtk_gainnet_bf6_mask_t *gbfmask=gainnet_bf6->gmask;
	int *eband=gbfmask->eband;
	int nb_bands=gbfmask->cfg->nb_bands;
	int nb_features=gbfmask->cfg->nb_features;
	float *Ex=gbfmask->Ex;
	int wins=gainnet_bf6->cfg->wins;
	float **analysis_mem_tr;
	float *rfft_in_tr;
	int pos;
	//   short *pv;
	//   wtk_strbuf_t *outbuf=wtk_strbuf_new(1024,2);
	//   wtk_complex_t *fftx=gainnet_bf6->fftx;
	float *rfft_in=gainnet_bf6->rfft_in;
	wtk_complex_t **fft=gainnet_bf6->fft;
	float **analysis_mem=gainnet_bf6->analysis_mem;

	xn=wtk_float_new_p2(channel,fsize);
	xr=wtk_float_new_p2(channel, fsize);

	g=wtk_malloc(sizeof(float)*(nb_bands));
	Er=wtk_float_new_p2(channel, nb_bands);

	fftr=wtk_complex_new_p2(channel, nbin);

	analysis_mem_tr=wtk_float_new_p2(channel, fsize);
	rfft_in_tr=wtk_calloc(sizeof(float),wins);

	if(bb==2)
	{
		for(j=0;j<channel;++j)
		{
			memset(data[j], 0, sizeof(short)*len);
			memset(datar[j], 0, sizeof(short)*len);
		}
	}
	if(bb==3)
	{
		for(j=0;j<channel;++j)
		{
			memset(data2[j], 0, sizeof(short)*len);
		}
	}
	pos=0;
	while((len-pos)>=fsize)
	{
		for(i=0;i<fsize;++i)
		{
			for(j=0;j<channel;++j)
			{
				xn[j][i]=data[j][i+pos]+data2[j][i+pos];
				xr[j][i]=datar[j][i+pos];
			}
		}
		for(j=0;j<channel;++j)
		{
			memmove(rfft_in_tr, analysis_mem_tr[j], fsize*sizeof(float));
			for(i=0;i<fsize;++i)
			{
				rfft_in_tr[i+fsize]=xr[j][i];
			}
			memcpy(analysis_mem_tr[j], xr[j], fsize*sizeof(float));
			for (i=0;i<wins;++i)
			{
				rfft_in_tr[i] *= gainnet_bf6->window[i];
			}
			forward_transform(gainnet_bf6, fftr[j], rfft_in_tr);

			fftr[j][0].a=fftr[j][0].b=fftr[j][nbin-1].a=fftr[j][nbin-1].b=0;
		}

		for(i=0; i<channel; ++i)
		{
			frame_analysis(gainnet_bf6, rfft_in, analysis_mem[i], fft[i], xn[i]);
		}
		gainnet_bf6->training=1;
		wtk_gainnet_bf6_feed_bf(gainnet_bf6, fft);
		for(j=0;j<channel;++j)
		{
			compute_band_energy(Er[j], eband, nb_bands, fftr[j]);
		}

		for (i=0;i<nb_bands;++i)
		{
			if(gbfmask->silence || Ex[i] < 5e-2 || Er[0][i] < 5e-2)
			{
				g[i]=0;
			}else
			{
				g[i]=0;
				for(j=0;j<channel;++j)
				{
					g[i] += sqrt((Er[j][i])/(Ex[i]));
				}
				g[i]/=channel;
			}
			if (g[i] > 1) g[i] = 1;
		}
		// printf("%f [%f %f] %f ms speech\n",vad,pos/16.0,(pos+fsize)/16.0,E);

		// {
		// 	interp_band_gain(eband, nb_bands, nbin, gbfmask->gf, g);
		// 	for (i=1;i<nbin-1;i++)
		// 	{
		// 		fftx[i].a *= gbfmask->gf[i];
		// 		fftx[i].b *= gbfmask->gf[i];
		// 	}
		// 	frame_synthesis(gainnet_bf6, gainnet_bf6->out,fftx);
		// 	pv=(short *)gainnet_bf6->out;
		// 	for(i=0;i<fsize;++i)
		// 	{
		// 		pv[i]=gainnet_bf6->out[i];
		// 	}
		// 	wtk_strbuf_push(outbuf, (char *)pv, sizeof(short)*fsize);
		// }

		if(gainnet_bf6->notify_tr)
		{
			gainnet_bf6->notify_tr(gainnet_bf6->ths_tr, gbfmask->features, nb_features,  g, nb_bands, NULL, 0, NULL, 0);
		}
		pos+=fsize;
	}
	// {
	// 	wtk_wavfile_t *wav;
	// 	wav=wtk_wavfile_new(gainnet_bf6->cfg->rate);
	// 	wav->max_pend=0;
	// 	wtk_wavfile_open(wav,"o.wav");
	// 	wtk_wavfile_write(wav,(char *)outbuf->data,outbuf->pos);
	// 	wtk_wavfile_delete(wav);
	// }

	wtk_float_delete_p2(xn,channel);
	wtk_free(xr);
	wtk_free(g);
	wtk_free(Er);
	wtk_free(fftr);
	wtk_free(analysis_mem_tr);
	wtk_free(rfft_in_tr);
}

void wtk_gainnet_bf6_feed_train4(wtk_gainnet_bf6_t *gainnet_bf6,short **data,short **data2,short **datar,int len, int bb)
{
	int channel=gainnet_bf6->cfg->channel;
	int fsize=gainnet_bf6->cfg->wins/2;
	int nbin=gainnet_bf6->nbin;
	int i,j;
	float **xn;
	float *xr;
	float *g;
	wtk_complex_t *fftr;
	wtk_gainnet_bf6_mask2_t *gbfmask2=gainnet_bf6->gmask2;
	int features_len=gbfmask2->features_len;
	int wins=gainnet_bf6->cfg->wins;
	float *analysis_mem_tr;
	float *rfft_in_tr;
	int pos;
	//   short *pv;
	//   wtk_strbuf_t *outbuf=wtk_strbuf_new(1024,2);
	//   wtk_complex_t *fftx=gainnet_bf6->fftx;
	float *rfft_in=gainnet_bf6->rfft_in;
	wtk_complex_t **fft=gainnet_bf6->fft;
	float **analysis_mem=gainnet_bf6->analysis_mem;
	float ff;

	xn=wtk_float_new_p2(channel,fsize);
	xr=wtk_malloc(sizeof(float)*(fsize));
	g=wtk_malloc(sizeof(float)*(nbin-2));
	fftr=wtk_calloc(sizeof(wtk_complex_t), nbin);

	analysis_mem_tr=wtk_calloc(sizeof(float), fsize);
	rfft_in_tr=wtk_calloc(sizeof(float),wins);

	if(bb==2)
	{
		for(j=0;j<channel;++j)
		{
			memset(data[j], 0, sizeof(short)*len);
			memset(datar[j], 0, sizeof(short)*len);
		}
	}
	if(bb==3)
	{
		for(j=0;j<channel;++j)
		{
			memset(data2[j], 0, sizeof(short)*len);
		}
	}
	pos=0;
	while((len-pos)>=fsize)
	{
		for(i=0;i<fsize;++i)
		{
			for(j=0;j<channel;++j)
			{
				xn[j][i]=data[j][i+pos]+data2[j][i+pos];
			}
			xr[i]=datar[0][i+pos];
		}

		memmove(rfft_in_tr, analysis_mem_tr, fsize*sizeof(float));
		for(i=0;i<fsize;++i)
		{
			rfft_in_tr[i+fsize]=xr[i];
		}
		memcpy(analysis_mem_tr, xr, fsize*sizeof(float));
		for (i=0;i<wins;++i)
		{
			rfft_in_tr[i] *= gainnet_bf6->window[i];
		}
		forward_transform(gainnet_bf6, fftr, rfft_in_tr);

		for(i=0; i<channel; ++i)
		{
			frame_analysis(gainnet_bf6, rfft_in, analysis_mem[i], fft[i], xn[i]);
		}
		gainnet_bf6->training=1;
		wtk_gainnet_bf6_feed_bf(gainnet_bf6, fft);

		for (i=1;i<nbin-1;++i)
		{
			ff=fft[0][i].a*fft[0][i].a+fft[0][i].b*fft[0][i].b;
			if(ff>=1e-4)
			{
				ff = sqrtf((fftr[i].a*fftr[i].a+fftr[i].b*fftr[i].b)/(ff));
				g[i-1]=min(ff,1.0);
			}else
			{
				g[i-1]=0;
			}
		}

		// {
		// 	for (i=1;i<nbin-1;i++)
		// 	{
		// 		fftx[i].a = fft[0][i].a;//*g[i-1];
		// 		fftx[i].b = fft[0][i].b;//*g[i-1];
		// 	}
		// 	frame_synthesis(gainnet_bf6, gainnet_bf6->out,fftx);
		// 	pv=(short *)gainnet_bf6->out;
		// 	for(i=0;i<fsize;++i)
		// 	{
		// 		pv[i]=gainnet_bf6->out[i];
		// 	}
		// 	wtk_strbuf_push(outbuf, (char *)pv, sizeof(short)*fsize);
		// }

		if(gainnet_bf6->notify_tr)
		{
			gainnet_bf6->notify_tr(gainnet_bf6->ths_tr, gbfmask2->features, features_len,  g, nbin-2, NULL, 0, NULL, 0);
		}
		pos+=fsize;
	}
	// {
	// 	wtk_wavfile_t *wav;
	// 	wav=wtk_wavfile_new(gainnet_bf6->cfg->rate);
	// 	wav->max_pend=0;
	// 	wtk_wavfile_open(wav,"o.wav");
	// 	wtk_wavfile_write(wav,(char *)outbuf->data,outbuf->pos);
	// 	wtk_wavfile_delete(wav);
	// }

	wtk_float_delete_p2(xn,channel);
	wtk_free(xr);
	wtk_free(g);
	wtk_free(fftr);
	wtk_free(analysis_mem_tr);
	wtk_free(rfft_in_tr);
}