#include "wtk_gainnet_bf5.h"
#define gainnet_bf5_tobank(n)   (13.1f*atan(.00074f*(n))+2.24f*atan((n)*(n)*1.85e-8f)+1e-4f*(n))

void wtk_gainnet_bf5_tfmask_on_gainnet(wtk_gainnet_bf5_tfmask_t *gtfmask, float *gain, int len, int is_end);
void wtk_gainnet_bf5_tfmask_on_gainnet2(wtk_gainnet_bf5_tfmask_t *gtfmask, float *gain, int len, int is_end);

void wtk_gainnet_bf5_xfilterbank_init(int *eband, int bands,int rate,int len)
{
	float df;
	float max_mel, mel_interval;
	int i;
	int id;
	float curr_freq;
	float mel;

	df =rate*1.0/(2*(len-1));
	max_mel = gainnet_bf5_tobank(rate/2);
	mel_interval =max_mel/(bands-1);
	for(i=0; i<bands; ++i)
   {
	   eband[i]=-1;
   }
   for (i=0;i<len;++i)
   {
		curr_freq = i*df;
		// printf("%f\n",curr_freq);
		mel = gainnet_bf5_tobank(curr_freq);
		if (mel > max_mel)
		{
			break;
		}
		id = (int)(floor(mel/mel_interval));
		// printf("%d %d %d %f %f\t", i, id, eband[id], mel, mel_interval);
		// printf("%f\n",curr_freq);
		if(eband[id]==-1)
		{
			eband[id]=i;
		}
						//  printf("%d  %d %f  %f  %f\n",id,eband[id],curr_freq, mel,mel_interval);
   }
   eband[bands-1]=len-1;
//    for(i=0; i<bands; ++i)
//    {
// 	   printf("%d ",eband[i]);
//    }
//    printf("\n");
}

void wtk_gainnet_bf5_tfmask_init(wtk_gainnet_bf5_tfmask_t *gtfmask, wtk_gainnet_bf5_cfg_t *cfg)
{
	gtfmask->cfg=cfg;
	gtfmask->nbin=cfg->wins/2+1;

	gtfmask->eband=wtk_malloc(sizeof(int)*cfg->nb_bands);
	wtk_gainnet_bf5_xfilterbank_init(gtfmask->eband, cfg->nb_bands, cfg->rate, gtfmask->nbin);

	gtfmask->Ex=wtk_malloc(sizeof(float)*cfg->nb_bands);

	gtfmask->dct_table=wtk_malloc(sizeof(float)*cfg->nb_bands*cfg->nb_bands);

	gtfmask->cepstral_mem=gtfmask->cepstral_mem_sp=NULL;
	if(cfg->use_ceps)
	{
		gtfmask->cepstral_mem=wtk_float_new_p2(cfg->ceps_mem, cfg->nb_bands);
		gtfmask->cepstral_mem_sp=wtk_float_new_p2(cfg->ceps_mem, cfg->nb_bands);
	}

	gtfmask->lastg=wtk_malloc(sizeof(float)*cfg->nb_bands);
	gtfmask->g=wtk_malloc(sizeof(float)*cfg->nb_bands);
	gtfmask->gf=wtk_malloc(sizeof(float)*gtfmask->nbin);
	gtfmask->lastg2=wtk_malloc(sizeof(float)*cfg->nb_bands);
	gtfmask->g2=wtk_malloc(sizeof(float)*cfg->nb_bands);
	gtfmask->gf2=wtk_malloc(sizeof(float)*gtfmask->nbin);


	gtfmask->Ly=wtk_malloc(sizeof(float)*cfg->nb_bands);
	gtfmask->features=wtk_malloc(sizeof(float)*cfg->nb_features);

	gtfmask->gainnet=wtk_gainnet_new(cfg->gainnet);
	wtk_gainnet_set_notify(gtfmask->gainnet, gtfmask, (wtk_gainnet_notify_f)wtk_gainnet_bf5_tfmask_on_gainnet);
	// wtk_gainnet_set_notify2(gtfmask->gainnet, gtfmask, (wtk_gainnet_notify_f2)wtk_gainnet_bf5_tfmask_on_gainnet2);

	gtfmask->qmmse=NULL;
	if(cfg->use_qmmse)
	{
		gtfmask->qmmse=wtk_qmmse_new(&(cfg->qmmse));
	}

	gtfmask->silence=1;
}

void wtk_gainnet_bf5_tfmask_clean(wtk_gainnet_bf5_tfmask_t *gtfmask)
{
	wtk_free(gtfmask->eband);
	wtk_free(gtfmask->Ex);

	wtk_free(gtfmask->dct_table);

	if(gtfmask->cepstral_mem)
	{
		wtk_float_delete_p2(gtfmask->cepstral_mem, gtfmask->cfg->ceps_mem);
		wtk_float_delete_p2(gtfmask->cepstral_mem_sp, gtfmask->cfg->ceps_mem);
	}

	wtk_free(gtfmask->lastg);
	wtk_free(gtfmask->g);
	wtk_free(gtfmask->gf);
	wtk_free(gtfmask->lastg2);
	wtk_free(gtfmask->g2);
	wtk_free(gtfmask->gf2);

	wtk_free(gtfmask->Ly);
	wtk_free(gtfmask->features);

	if(gtfmask->qmmse)
	{
		wtk_qmmse_delete(gtfmask->qmmse);
	}

	wtk_gainnet_delete(gtfmask->gainnet);
}

void wtk_gainnet_bf5_tfmask_reset(wtk_gainnet_bf5_tfmask_t *gtfmask)
{
	int i,j;
	int nb_bands=gtfmask->cfg->nb_bands;

	for (i=0;i<nb_bands;++i) 
	{
		for (j=0;j<nb_bands;++j)
		{
			gtfmask->dct_table[i*nb_bands + j] = cosf((i+.5)*j*PI/nb_bands);
			if (j==0)
			{
			gtfmask->dct_table[i*nb_bands + j] *= sqrtf(.5);
			}
		}
	}

	memset(gtfmask->Ex, 0, sizeof(float)*(gtfmask->cfg->nb_bands));

	gtfmask->memid=0;
	gtfmask->memid_sp=0;
	if(gtfmask->cepstral_mem)
	{
		wtk_float_zero_p2(gtfmask->cepstral_mem, gtfmask->cfg->ceps_mem, gtfmask->cfg->nb_bands);
		wtk_float_zero_p2(gtfmask->cepstral_mem_sp, gtfmask->cfg->ceps_mem, gtfmask->cfg->nb_bands);
	}

	memset(gtfmask->lastg, 0, sizeof(float)*gtfmask->cfg->nb_bands);
	memset(gtfmask->g, 0, sizeof(float)*gtfmask->cfg->nb_bands);
	memset(gtfmask->gf, 0, sizeof(float)*gtfmask->nbin);

	memset(gtfmask->lastg2, 0, sizeof(float)*gtfmask->cfg->nb_bands);
	memset(gtfmask->g2, 0, sizeof(float)*gtfmask->cfg->nb_bands);
	memset(gtfmask->gf2, 0, sizeof(float)*gtfmask->nbin);

	memset(gtfmask->Ly, 0, sizeof(float)*gtfmask->cfg->nb_bands);
	memset(gtfmask->features, 0, sizeof(float)*gtfmask->cfg->nb_features);

	wtk_gainnet_reset(gtfmask->gainnet);

	if(gtfmask->qmmse)
	{
		wtk_qmmse_reset(gtfmask->qmmse);
	}
}

wtk_gainnet_bf5_t* wtk_gainnet_bf5_new(wtk_gainnet_bf5_cfg_t *cfg)
{
	wtk_gainnet_bf5_t *gainnet_bf5;

	gainnet_bf5=(wtk_gainnet_bf5_t *)wtk_malloc(sizeof(wtk_gainnet_bf5_t));
	gainnet_bf5->cfg=cfg;
	gainnet_bf5->ths=NULL;
	gainnet_bf5->notify=NULL;

	gainnet_bf5->mic=wtk_strbufs_new(gainnet_bf5->cfg->channel);

	gainnet_bf5->notch_mem=NULL;
	gainnet_bf5->memD=NULL;
	if(cfg->use_preemph)
	{
		gainnet_bf5->notch_mem=wtk_float_new_p2(cfg->channel,2);
		gainnet_bf5->memD=(float *)wtk_malloc(sizeof(float)*cfg->channel);
		gainnet_bf5->memX=0;
	}

	gainnet_bf5->nbin=cfg->wins/2+1;
	gainnet_bf5->window=wtk_malloc(sizeof(float)*cfg->wins);///2);
	gainnet_bf5->synthesis_window=wtk_malloc(sizeof(float)*cfg->wins);///2);
	gainnet_bf5->analysis_mem=wtk_float_new_p2(cfg->channel, gainnet_bf5->nbin-1);
	gainnet_bf5->synthesis_mem=wtk_malloc(sizeof(float)*(gainnet_bf5->nbin-1));
	gainnet_bf5->rfft=wtk_drft_new(cfg->wins);
	gainnet_bf5->rfft_in=(float*)wtk_malloc(sizeof(float)*(cfg->wins));

    gainnet_bf5->ovec=wtk_complex_new_p3(2,gainnet_bf5->nbin, cfg->channel);
	gainnet_bf5->fft=wtk_complex_new_p2(cfg->channel, gainnet_bf5->nbin);

	gainnet_bf5->ffts=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*gainnet_bf5->nbin);
	gainnet_bf5->ffty=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*gainnet_bf5->nbin);

	gainnet_bf5->gtfmask=(wtk_gainnet_bf5_tfmask_t *)wtk_malloc(sizeof(wtk_gainnet_bf5_tfmask_t));
	wtk_gainnet_bf5_tfmask_init(gainnet_bf5->gtfmask, cfg);

	gainnet_bf5->out=wtk_malloc(sizeof(float)*(gainnet_bf5->nbin-1));

	wtk_gainnet_bf5_reset(gainnet_bf5);

	return gainnet_bf5;
}

void wtk_gainnet_bf5_delete(wtk_gainnet_bf5_t *gainnet_bf5)
{
	wtk_strbufs_delete(gainnet_bf5->mic,gainnet_bf5->cfg->channel);
	if(gainnet_bf5->notch_mem)
	{
		wtk_float_delete_p2(gainnet_bf5->notch_mem, gainnet_bf5->cfg->channel);
		wtk_free(gainnet_bf5->memD);
	}
	wtk_free(gainnet_bf5->window);
	wtk_free(gainnet_bf5->synthesis_window);
	wtk_float_delete_p2(gainnet_bf5->analysis_mem, gainnet_bf5->cfg->channel);
	wtk_free(gainnet_bf5->synthesis_mem);
	wtk_free(gainnet_bf5->rfft_in);
	wtk_drft_delete(gainnet_bf5->rfft);
	wtk_complex_delete_p2(gainnet_bf5->fft, gainnet_bf5->cfg->channel);

	wtk_free(gainnet_bf5->ffts);
	wtk_free(gainnet_bf5->ffty);

	wtk_complex_delete_p3(gainnet_bf5->ovec, 2, gainnet_bf5->nbin);

	wtk_gainnet_bf5_tfmask_clean(gainnet_bf5->gtfmask);
	wtk_free(gainnet_bf5->gtfmask);

	wtk_free(gainnet_bf5->out);

	wtk_free(gainnet_bf5);
}

void wtk_gainnet_bf5_flush_ovec(wtk_gainnet_bf5_t *gainnet_bf5,  wtk_complex_t *ovec, float **mic_pos, float sv, int rate, 
                                                        int theta2, int phi2, int k, float *tdoa)
{
	float x,y,z;
	float t;
	float *mic;
	int j;
	int channel=gainnet_bf5->cfg->channel;
	int win=(gainnet_bf5->nbin-1)*2;
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
    // x=1.0/sqrt(channel);
    ovec1=ovec;
    t=2*PI*rate*1.0/win*k;
    for(j=0;j<channel;++j)
    {
        ovec1[j].a=cos(t*tdoa[j]);
        ovec1[j].b=sin(t*tdoa[j]);
    }
}

void wtk_gainnet_bf5_start(wtk_gainnet_bf5_t *gainnet_bf5,float theta, float phi)
{
    wtk_complex_t ***ovec=gainnet_bf5->ovec, **ovec2, **ovec3;
    int channel=gainnet_bf5->cfg->channel;
    int nbin=gainnet_bf5->nbin;
    wtk_complex_t *novec,*novec2,*a,*b,*b2;
    wtk_complex_t *cov;//,*cov1,*cov2;
    int i,j,k;
    wtk_dcomplex_t *tmp_inv;
	wtk_complex_t *tmp;
    float *tdoa;
	float fa;
	// float eye=gainnet_bf5->cfg->lds_eye;

    tmp=(wtk_complex_t *)wtk_calloc(channel,sizeof(wtk_complex_t));
    tdoa=(float *)wtk_malloc(channel*sizeof(float));
    novec=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*channel);
	novec2=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*channel);
    cov=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*channel*channel);
    tmp_inv=(wtk_dcomplex_t *)wtk_malloc(channel*(channel+1)*sizeof(wtk_dcomplex_t));

    for(k=6;k<nbin-1;++k)
    {
		ovec2=ovec[0];
		ovec3=ovec[1];
        memset(cov,0,sizeof(wtk_complex_t)*channel*channel);
		memset(ovec2[k],0,sizeof(wtk_complex_t)*channel);
		memset(ovec3[k],0,sizeof(wtk_complex_t)*channel);

		wtk_gainnet_bf5_flush_ovec(gainnet_bf5, novec, gainnet_bf5->cfg->mic_pos,gainnet_bf5->cfg->speed,gainnet_bf5->cfg->rate, 0,0,k,tdoa);
		wtk_gainnet_bf5_flush_ovec(gainnet_bf5, novec2, gainnet_bf5->cfg->mic_pos,gainnet_bf5->cfg->speed,gainnet_bf5->cfg->rate, 180,0,k,tdoa);
		a=cov;
		b=novec2;
		for(i=0; i<channel; ++i, ++b)
		{
			a+=i;
			b2=novec2+i;
			for(j=i; j<channel; ++j, ++a,++b2)
			{
				a->a=b->a*b2->a+b->b*b2->b;
				if(j==i)
				{
					a->a+=1e-6;
					a->b=0;
				}else
				{
					a->b=-b->a*b2->b+b->b*b2->a;
					cov[j*channel+i].a=a->a;
					cov[j*channel+i].b=-a->b;
				}
			}
		}
		wtk_complex_guass_elimination_p1(cov, novec, tmp_inv, channel, tmp);
		fa=0;
		a=tmp;
		b=novec;
		for(i=0;i<channel;++i,++b,++a)
		{
			fa+=b->a*a->a+b->b*a->b;
		}
		fa=1.0/fa;
		a=tmp;
		for(i=0;i<channel;++i,++a)
		{
			a->a*=fa;
			a->b*=fa;
		}
        memcpy(ovec2[k], tmp, sizeof(wtk_complex_t)*channel);


		wtk_gainnet_bf5_flush_ovec(gainnet_bf5, novec, gainnet_bf5->cfg->mic_pos,gainnet_bf5->cfg->speed,gainnet_bf5->cfg->rate, 180,0,k,tdoa);
		wtk_gainnet_bf5_flush_ovec(gainnet_bf5, novec2, gainnet_bf5->cfg->mic_pos,gainnet_bf5->cfg->speed,gainnet_bf5->cfg->rate, 0,0,k,tdoa);
		a=cov;
		b=novec2;
		for(i=0; i<channel; ++i, ++b)
		{
			a+=i;
			b2=novec2+i;
			for(j=i; j<channel; ++j, ++a,++b2)
			{
				a->a=b->a*b2->a+b->b*b2->b;
				if(j==i)
				{
					a->a+=1e-6;
					a->b=0;
				}else
				{
					a->b=-b->a*b2->b+b->b*b2->a;
					cov[j*channel+i].a=a->a;
					cov[j*channel+i].b=-a->b;
				}
			}
		}
		wtk_complex_guass_elimination_p1(cov, novec, tmp_inv, channel, tmp);
		fa=0;
		a=tmp;
		b=novec;
		for(i=0;i<channel;++i,++b,++a)
		{
			fa+=b->a*a->a+b->b*a->b;
		}
		fa=1.0/fa;
		a=tmp;
		for(i=0;i<channel;++i,++a)
		{
			a->a*=fa;
			a->b*=fa;
		}
        memcpy(ovec3[k], tmp, sizeof(wtk_complex_t)*channel);
	}

    // for(k=6;k<nbin-1;++k)
    // {
	// 	ovec2=ovec[0];
	// 	ovec3=ovec[1];
    //     memset(cov,0,sizeof(wtk_complex_t)*channel*channel);
	// 	memset(ovec2[k],0,sizeof(wtk_complex_t)*channel);
	// 	memset(ovec3[k],0,sizeof(wtk_complex_t)*channel);
	// 	ovec2[k][0].a=1;
	// 	ovec2[k][1].a=0;

	// 	ovec3[k][0].a=0;
	// 	ovec3[k][1].a=1;

	// 	wtk_gainnet_bf5_flush_ovec(gainnet_bf5, novec, aspec->cfg->mic_pos,aspec->cfg->speed,gainnet_bf5->cfg->rate, 0,0,k,tdoa);
	// 	for(i=0;i<channel;++i)
	// 	{
	// 		cov[i]=novec[i];
	// 	}
	// 	wtk_gainnet_bf5_flush_ovec(gainnet_bf5, novec, aspec->cfg->mic_pos,aspec->cfg->speed,gainnet_bf5->cfg->rate,180,0,k,tdoa);
	// 	for(i=0;i<channel;++i)
	// 	{
	// 		cov[i+channel]=novec[i];
	// 	}
    // 	wtk_complex_guass_elimination_p1(cov, ovec2[k], tmp_inv, channel, tmp);
    //     memcpy(ovec2[k], tmp, sizeof(wtk_complex_t)*channel);
    // 	wtk_complex_guass_elimination_p1(cov, ovec3[k], tmp_inv, channel, tmp);
    //     memcpy(ovec3[k], tmp, sizeof(wtk_complex_t)*channel);
    // }
    wtk_free(tmp);
    wtk_free(tdoa);
    wtk_free(cov);
    wtk_free(tmp_inv);
    wtk_free(novec);
	wtk_free(novec2);
}

void wtk_gainnet_bf5_reset(wtk_gainnet_bf5_t *gainnet_bf5)
{
	int wins=gainnet_bf5->cfg->wins;
	int frame_size=gainnet_bf5->cfg->wins/2;
	int i;
	int shift, nshift, j, n;

	wtk_strbufs_reset(gainnet_bf5->mic,gainnet_bf5->cfg->channel);
	if(gainnet_bf5->notch_mem)
	{
		for(i=0;i<gainnet_bf5->cfg->channel;++i)
		{
			memset(gainnet_bf5->notch_mem[i],0,2*sizeof(float));
		}
		memset(gainnet_bf5->memD,0,gainnet_bf5->cfg->channel*sizeof(float));
		gainnet_bf5->memX=0;
	}
	for (i=0;i<wins;++i)
	{
		gainnet_bf5->window[i] = sin((0.5+i)*PI/(wins));//sin(.5*PI*sin(.5*PI*(i+.5)/frame_size) * sin(.5*PI*(i+.5)/frame_size));
		gainnet_bf5->synthesis_window[i]=0;
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
				gainnet_bf5->synthesis_window[i] += gainnet_bf5->window[n]*gainnet_bf5->window[n];
			}
		}
	}
	for(i=1;i<nshift;++i)
	{
		for(j=0;j<shift;++j)
		{
			gainnet_bf5->synthesis_window[i*shift+j] = gainnet_bf5->synthesis_window[j];
		}
	}
	for(i=0;i<wins;++i)
	{
		gainnet_bf5->synthesis_window[i]=gainnet_bf5->window[i]/gainnet_bf5->synthesis_window[i];
	}


	wtk_float_zero_p2(gainnet_bf5->analysis_mem, gainnet_bf5->cfg->channel, (gainnet_bf5->nbin-1));
	memset(gainnet_bf5->synthesis_mem, 0, sizeof(float)*(gainnet_bf5->nbin-1));

	wtk_complex_zero_p2(gainnet_bf5->fft, gainnet_bf5->cfg->channel, gainnet_bf5->nbin);
	
	memset(gainnet_bf5->ffts, 0, sizeof(wtk_complex_t)*(gainnet_bf5->nbin));
	memset(gainnet_bf5->ffty, 0, sizeof(wtk_complex_t)*(gainnet_bf5->nbin));

	wtk_gainnet_bf5_tfmask_reset(gainnet_bf5->gtfmask);

	gainnet_bf5->training=0;
}


void wtk_gainnet_bf5_set_notify(wtk_gainnet_bf5_t *gainnet_bf5,void *ths,wtk_gainnet_bf5_notify_f notify)
{
	gainnet_bf5->notify=notify;
	gainnet_bf5->ths=ths;
}

void wtk_gainnet_bf5_set_tr_notify(wtk_gainnet_bf5_t *gainnet_bf5,void *ths,wtk_gainnet_bf5_notify_trfeat_f notify)
{
	gainnet_bf5->notify_tr=notify;
	gainnet_bf5->ths_tr=ths;
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

// static void compute_band_energy2(float *bandE, int *eband, int nb_bands, wtk_complex_t *fft, float *mask) 
// {
//     int i;
//     int j;
//     int band_size;
//     float tmp;
//     float frac;

//     memset(bandE, 0, sizeof(float)*nb_bands);
//     for (i=0;i<nb_bands-1;++i)
//     {
//         band_size = eband[i+1]-eband[i];
//         for (j=0;j<band_size;j++) 
//         {
//             frac = (float)j/band_size;
//             tmp = (fft[eband[i] + j].a* fft[eband[i] + j].a+ fft[eband[i] + j].b* fft[eband[i] + j].b)*mask[eband[i] + j]*mask[eband[i] + j];
//             bandE[i] += (1-frac)*tmp;
//             bandE[i+1] += frac*tmp;
//         }
//     }
//     bandE[0] *= 2;
//     bandE[nb_bands-1] *= 2;
// }

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
    out[i] = sum*sqrtf(2.f/nb_bands);
  }
}

static void inverse_transform(wtk_gainnet_bf5_t *gainnet_bf5, wtk_complex_t *fft, float *out)
{
	wtk_drft_ifft2(gainnet_bf5->rfft,fft,out);
}

static void forward_transform(wtk_gainnet_bf5_t *gainnet_bf5, wtk_complex_t *fft, float *in)
{
	wtk_drft_fft2(gainnet_bf5->rfft,in,fft);
}

static void frame_analysis(wtk_gainnet_bf5_t *gainnet_bf5, float *rfft_in, float *analysis_mem, wtk_complex_t *fft, const float *in)
{
  int i;
  int wins=gainnet_bf5->cfg->wins;
  int fsize=wins/2;

  memmove(rfft_in, analysis_mem, fsize*sizeof(float));
  for(i=0;i<fsize;++i)
  {
    rfft_in[i+fsize]=in[i];
  }
  memcpy(analysis_mem, in, fsize*sizeof(float));
	for (i=0;i<wins;++i)
	{
		rfft_in[i] *= gainnet_bf5->window[i];
		//rfft_in[wins - 1 - i] *= gainnet_bf5->window[i];
	}
  forward_transform(gainnet_bf5, fft, rfft_in);
}

static void frame_synthesis(wtk_gainnet_bf5_t *gainnet_bf5, float *out, wtk_complex_t *fft)
{
  float *rfft_in=gainnet_bf5->rfft_in;
  int i;
  int wins=gainnet_bf5->cfg->wins;
  int fsize=wins/2;
  float *synthesis_mem=gainnet_bf5->synthesis_mem;

  inverse_transform(gainnet_bf5, fft, rfft_in);
  for (i=0;i<wins;++i)
  {
		rfft_in[i] *= gainnet_bf5->synthesis_window[i];
    // rfft_in[i] *= gainnet_bf5->window[i];
    // rfft_in[wins - 1 - i] *= gainnet_bf5->window[i];
  }
  for (i=0;i<fsize;i++) out[i] = rfft_in[i] + synthesis_mem[i];
  memcpy(synthesis_mem, &rfft_in[fsize], fsize*sizeof(float));
}

static int compute_frame_gtfmask_features(wtk_gainnet_bf5_tfmask_t *gtfmask, float *features, wtk_complex_t *fftx)
{
	float *Ex=gtfmask->Ex;
	int *eband=gtfmask->eband;
	int i,j,k;
	float E = 0;
	float *ceps_0, *ceps_1, *ceps_2;
	float spec_variability = 0;
	float *Ly=gtfmask->Ly;
	int nb_bands=gtfmask->cfg->nb_bands;
	int nb_delta_ceps=gtfmask->cfg->nb_delta_ceps;
	int ceps_mem=gtfmask->cfg->ceps_mem;
	float follow, logMax;
	float mindist,dist,tmp;
	
	if(gtfmask->qmmse)
	{
		wtk_qmmse_flush_denoise_mask(gtfmask->qmmse, fftx);
	}

	compute_band_energy(Ex,eband, nb_bands, fftx);
	logMax = -2;
	follow = -2;
	for (i=0;i<nb_bands;i++)
	{
		Ly[i] = log10f(1e-2f+Ex[i]);
		Ly[i] = max(logMax-7, max(follow-1.5, Ly[i]));
		logMax = max(logMax, Ly[i]);
		follow = max(follow-1.5, Ly[i]);
		E += Ex[i];
	}
	dct(gtfmask->dct_table, nb_bands, features, Ly);
	
	if(gtfmask->cepstral_mem)
	{
		features[0] -= 12;
		features[1] -= 4;
		ceps_0 = gtfmask->cepstral_mem[gtfmask->memid];
		ceps_1 = (gtfmask->memid < 1) ? gtfmask->cepstral_mem[ceps_mem+gtfmask->memid-1] : gtfmask->cepstral_mem[gtfmask->memid-1];
		ceps_2 = (gtfmask->memid < 2) ? gtfmask->cepstral_mem[ceps_mem+gtfmask->memid-2] : gtfmask->cepstral_mem[gtfmask->memid-2];
		for (i=0;i<nb_bands;i++)
		{
			ceps_0[i] = features[i];
		}
		gtfmask->memid++;
		for (i=0;i<nb_delta_ceps;i++)
		{
			features[i] = ceps_0[i] + ceps_1[i] + ceps_2[i];
			features[nb_bands+i] = ceps_0[i] - ceps_2[i];
			features[nb_bands+nb_delta_ceps+i] =  ceps_0[i] - 2*ceps_1[i] + ceps_2[i];
		}
		/* Spectral variability features. */
		if (gtfmask->memid == ceps_mem)
		{
			gtfmask->memid = 0;
		}
		for (i=0;i<ceps_mem;++i)
		{
			mindist = 1e15f;
			for (j=0;j<ceps_mem;++j)
			{
					dist=0;
				for (k=0;k<nb_bands;++k)
				{
					tmp = gtfmask->cepstral_mem[i][k] - gtfmask->cepstral_mem[j][k];
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



static void compute_frame_gtfmask_features2(wtk_gainnet_bf5_tfmask_t *gtfmask, float *features, wtk_complex_t *fftsp)
{
	float *Ex=gtfmask->Ex;
	int *eband=gtfmask->eband;
	int i,j,k;
	float E = 0;
	float *ceps_0, *ceps_1, *ceps_2;
	float spec_variability = 0;
	float *Ly=gtfmask->Ly;
	int nb_bands=gtfmask->cfg->nb_bands;
	int nb_delta_ceps=gtfmask->cfg->nb_delta_ceps;
	int ceps_mem=gtfmask->cfg->ceps_mem;
	float follow, logMax;
	float mindist,dist,tmp;

	compute_band_energy(Ex,eband, nb_bands, fftsp);
	logMax = -2;
	follow = -2;
	for (i=0;i<nb_bands;i++)
	{
		Ly[i] = log10f(1e-2f+Ex[i]);
		Ly[i] = max(logMax-7, max(follow-1.5, Ly[i]));
		logMax = max(logMax, Ly[i]);
		follow = max(follow-1.5, Ly[i]);
		E += Ex[i];
	}
	dct(gtfmask->dct_table, nb_bands, features, Ly);
	if(gtfmask->cepstral_mem)
	{
		features[0] -= 12;
		features[1] -= 4;

		ceps_0 = gtfmask->cepstral_mem_sp[gtfmask->memid_sp];
		ceps_1 = (gtfmask->memid_sp < 1) ? gtfmask->cepstral_mem_sp[ceps_mem+gtfmask->memid_sp-1] : gtfmask->cepstral_mem_sp[gtfmask->memid_sp-1];
		ceps_2 = (gtfmask->memid_sp < 2) ? gtfmask->cepstral_mem_sp[ceps_mem+gtfmask->memid_sp-2] : gtfmask->cepstral_mem_sp[gtfmask->memid_sp-2];
		for (i=0;i<nb_bands;i++)
		{
			ceps_0[i] = features[i];
		}
		gtfmask->memid_sp++;
		for (i=0;i<nb_delta_ceps;i++)
		{
			features[i] = ceps_0[i] + ceps_1[i] + ceps_2[i];
			features[nb_bands+i] = ceps_0[i] - ceps_2[i];
			features[nb_bands+nb_delta_ceps+i] =  ceps_0[i] - 2*ceps_1[i] + ceps_2[i];
		}
		/* Spectral variability features. */
		if (gtfmask->memid_sp == ceps_mem)
		{
			gtfmask->memid_sp = 0;
		}
		for (i=0;i<ceps_mem;++i)
		{
			mindist = 1e15f;
			for (j=0;j<ceps_mem;++j)
			{
					dist=0;
				for (k=0;k<nb_bands;++k)
				{
					tmp = gtfmask->cepstral_mem_sp[i][k] - gtfmask->cepstral_mem_sp[j][k];
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
}


void wtk_gainnet_bf5_tfmask_on_gainnet(wtk_gainnet_bf5_tfmask_t *gtfmask, float *gain, int len, int is_end)
{
	memcpy(gtfmask->g, gain, sizeof(float)*gtfmask->cfg->nb_bands);
}


void wtk_gainnet_bf5_tfmask_on_gainnet2(wtk_gainnet_bf5_tfmask_t *gtfmask, float *gain, int len, int is_end)
{
	int i;
	int nb_bands=gtfmask->cfg->nb_bands;
	float *g2=gtfmask->g2;
	float agc_a=gtfmask->cfg->agc_a;
	float agc_b=gtfmask->cfg->agc_b;

	for(i=0; i<nb_bands; ++i)
	{
		g2[i]=-1/agc_a*(logf(1/gain[i]-1)-agc_b);
	}
}

void wtk_gainnet_bf5_tfmask_feed(wtk_gainnet_bf5_t *gainnet_bf5, wtk_complex_t *fftx, wtk_complex_t *ffty)
{
	int i;
	wtk_gainnet_bf5_tfmask_t *gtfmask=gainnet_bf5->gtfmask;
	int nb_bands=gtfmask->cfg->nb_bands;
	int nbin=gtfmask->nbin;
	float *g=gtfmask->g, *gf=gtfmask->gf, *lastg=gtfmask->lastg;//, *g2=gtfmask->g2, *gf2=gtfmask->gf2, *lastg2=gtfmask->lastg2;
	int *eband=gtfmask->eband;
	wtk_gainnet_t *gainnet=gtfmask->gainnet;
	int nb_features=gtfmask->cfg->nb_features;
	int nb_features_x=gtfmask->cfg->nb_features_x;
	float *qmmse_gain;

    gtfmask->silence=compute_frame_gtfmask_features(gtfmask, gtfmask->features, fftx);
    compute_frame_gtfmask_features2(gtfmask, gtfmask->features+nb_features_x, ffty);

	if(!gainnet_bf5->training)
	{
		if(0)
		{
			for (i=1; i<nbin-1; ++i)
			{
				gf[i]=0;
				// gf2[i]=0;
			}
		}else
		{
			// wtk_gainnet5_feed(gainnet, gtfmask->features, nb_features_x, gtfmask->features+nb_features_x, nb_features-nb_features_x, 0);   
			wtk_gainnet_feed(gainnet, gtfmask->features, nb_features, 0);   
			for (i=0;i<nb_bands;++i)
			{
				g[i] = max(g[i], 0.6f*lastg[i]);
				// lastg[i] = g[i];
			}
			interp_band_gain(eband, nb_bands, nbin, gf, g);
			// if(gainnet && gainnet->cfg->use_agc)
			// {
			// 	for (i=0;i<nb_bands;++i)
			// 	{
			// 		// g2[i] = max(g2[i], 0.6*lastg2[i]);
			// 		lastg2[i] = g2[i];
			// 	}
			// 	interp_band_gain(eband, nb_bands, nbin, gf2, g2);
			// }
			if(gtfmask->qmmse)
			{
				qmmse_gain=gtfmask->qmmse->gain;
				for (i=1; i<nbin-1; ++i)
				{
					if(gf[i]>qmmse_gain[i])
					{
						// if(gainnet && gainnet->cfg->use_agc)
						// {
						// 	gf2[i]*=qmmse_gain[i]/gf[i];
						// }
						gf[i]=qmmse_gain[i];
					}
				}
			}
		}

		for (i=1; i<nbin-1; ++i)
		{
			fftx[i].a *= gf[i];
			fftx[i].b *= gf[i];
		}
	}
}

void wtk_gainnet_bf5_feed_bf(wtk_gainnet_bf5_t *gainnet_bf5)
{
    wtk_complex_t ***ovec=gainnet_bf5->ovec, *ovec2;
    wtk_complex_t **fft=gainnet_bf5->fft;
	wtk_complex_t *ffts=gainnet_bf5->ffts,*ffts1;
	wtk_complex_t *ffty=gainnet_bf5->ffty,*ffty1;
    int channel=gainnet_bf5->cfg->channel;
    int nbin=gainnet_bf5->nbin;
    int i, k;
    float ta,tb;
	// wtk_complex_t **fft2=gainnet_bf5->fft2;

	ffts1=ffts;
	ffty1=ffty;
	for(k=0; k<nbin; ++k, ++ffts1, ++ffty1)
	{
		ovec2=ovec[0][k];
		ta=tb=0;
		for(i=0; i<channel; ++i, ++ovec2)
		{
			ta+=ovec2->a*fft[i][k].a + ovec2->b*fft[i][k].b;
			tb+=ovec2->a*fft[i][k].b - ovec2->b*fft[i][k].a;
		}
		ffts1->a=ta;
		ffts1->b=tb;

		ovec2=ovec[1][k];
		ta=tb=0;
		for(i=0; i<channel; ++i, ++ovec2)
		{
			ta+=ovec2->a*fft[i][k].a + ovec2->b*fft[i][k].b;
			tb+=ovec2->a*fft[i][k].b - ovec2->b*fft[i][k].a;
		}
		ffty1->a=ta;
		ffty1->b=tb;
		// ffts1->a=ffty1->a;
		// ffts1->b=ffty1->b;
	}

	wtk_gainnet_bf5_tfmask_feed(gainnet_bf5, ffts, ffty);
}


void wtk_gainnet_bf5_feed_agc2(wtk_gainnet_bf5_t *gainnet_bf5, wtk_complex_t * fft)
{
	wtk_gainnet_bf5_tfmask_t *gtfmask=gainnet_bf5->gtfmask;
	int nbin=gtfmask->nbin;
	float *gf2=gtfmask->gf2;
	float gf;
	int i;

	if (!gtfmask->silence)
	{
		gf=wtk_float_abs_mean(gf2+1, nbin-2);
		for (i=1;i<nbin-1;++i)
		{
			fft[i].a *= gf;//gf2[i];
			fft[i].b *= gf;//gf2[i];
		}
	}
}

void wtk_gainnet_bf5_feed(wtk_gainnet_bf5_t *gainnet_bf5,short **data,int len,int is_end)
{
	int i,j;//,k;
	// int nbin=gainnet_bf5->nbin;
	int channel=gainnet_bf5->cfg->channel;
	wtk_strbuf_t **mic=gainnet_bf5->mic;
	float **notch_mem=gainnet_bf5->notch_mem;
	float *memD=gainnet_bf5->memD;
	float fv, *fp1;
	int wins=gainnet_bf5->cfg->wins;
	int fsize=wins/2;
	int length;
	float *rfft_in=gainnet_bf5->rfft_in;
	wtk_complex_t **fft=gainnet_bf5->fft;
	float **analysis_mem=gainnet_bf5->analysis_mem;
	wtk_complex_t *ffts=gainnet_bf5->ffts;
	float *out=gainnet_bf5->out;
	short *pv=(short *)out;

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
			frame_analysis(gainnet_bf5, rfft_in, analysis_mem[i], fft[i], fp1);
		}
		wtk_gainnet_bf5_feed_bf(gainnet_bf5);
		// if(gainnet_bf5->gtfmask->cfg->gainnet && gainnet_bf5->gtfmask->cfg->gainnet->use_agc)
		// {
        //     wtk_gainnet_bf5_feed_agc2(gainnet_bf5, ffts);
		// }
	
		wtk_strbufs_pop(mic, channel, fsize*sizeof(float));
		length=mic[0]->pos/sizeof(float);

	    frame_synthesis(gainnet_bf5, out, ffts);
		if(notch_mem)
		{
			gainnet_bf5->memX=wtk_preemph_asis2(out,fsize,gainnet_bf5->memX);
		}
		for(i=0; i<fsize; ++i)
		{
			pv[i]=floorf(out[i]+0.5);
		}
		if(gainnet_bf5->notify)
		{
			gainnet_bf5->notify(gainnet_bf5->ths,pv,fsize);
		}
	}
	if(is_end && length>0)
	{
		if(gainnet_bf5->notify)
		{
			pv=(short *)mic[0]->data;
			out=(float *)mic[0]->data;
			for(i=0; i<length; ++i)
			{
				pv[i]=floorf(out[i]+0.5);
			}
			gainnet_bf5->notify(gainnet_bf5->ths,pv,length);
		}
	}
}






void wtk_gainnet_bf5_feed_train(wtk_gainnet_bf5_t *gainnet_bf5,short **data,int len, int bb)
{
	int channel=gainnet_bf5->cfg->channel;
	int fsize=gainnet_bf5->cfg->wins/2;
	int nbin=gainnet_bf5->nbin;
	int i,j,k;
	float **xn;
	float **x;
	float **xr;
	float *g;
	float *Er;
	float *gn;
	float *En;
	wtk_complex_t **fftn,*fftn2;
	wtk_complex_t **fftr,*fftr2;
	wtk_gainnet_bf5_tfmask_t *gtfmask=gainnet_bf5->gtfmask;
	int *eband=gtfmask->eband;
	int nb_bands=gtfmask->cfg->nb_bands;
	int nb_features=gtfmask->cfg->nb_features;
	float *Ex=gtfmask->Ex;
	int wins=gainnet_bf5->cfg->wins;
	float **analysis_mem_tr;
	float **analysis_mem_tr2;
	float *rfft_in_tr;
	int pos;
	wtk_complex_t ***ovec=gainnet_bf5->ovec,*ovec2;
	float ta,tb;
	//   short *pv;
	//   wtk_strbuf_t *outbuf=wtk_strbuf_new(1024,2);
	//   wtk_complex_t *ffts=gainnet_bf5->ffts;
	float *rfft_in=gainnet_bf5->rfft_in;
	wtk_complex_t **fft=gainnet_bf5->fft;
	float **analysis_mem=gainnet_bf5->analysis_mem;

	xn=wtk_float_new_p2(channel,fsize);
	x=wtk_float_new_p2(channel,fsize);
	xr=wtk_float_new_p2(channel,fsize);

	gn=wtk_malloc(sizeof(float)*(nb_bands));
	En=wtk_malloc(sizeof(float)*(nb_bands));
	g=wtk_malloc(sizeof(float)*(nb_bands));
	Er=wtk_malloc(sizeof(float)*(nb_bands));

	fftn=wtk_complex_new_p2(channel,nbin);
	fftn2=wtk_malloc(sizeof(wtk_complex_t)*(nbin));
	fftr=wtk_complex_new_p2(channel,nbin);
	fftr2=wtk_malloc(sizeof(wtk_complex_t)*(nbin));

	analysis_mem_tr=wtk_float_new_p2(channel,fsize);
	analysis_mem_tr2=wtk_float_new_p2(channel,fsize);
	rfft_in_tr=wtk_calloc(sizeof(float),wins);

	if(bb==2)
	{
		for(j=0;j<channel;++j)
		{
			memset(data[channel+j], 0, sizeof(short)*len);
			memset(data[4*channel+j], 0, sizeof(short)*len);
		}
	}
	if(bb==3)
	{
		for(j=0;j<channel;++j)
		{
			memset(data[j], 0, sizeof(short)*len);
			memset(data[channel*3+j], 0, sizeof(short)*len);
		}
	}
	if(bb==4)
	{
		for(j=0;j<channel;++j)
		{
			memcpy(data[j], data[channel*3+j], sizeof(short)*len);
			memcpy(data[channel+j], data[channel*4+j], sizeof(short)*len);
		}
	}
	if(bb==5)
	{
		for(j=0;j<channel;++j)
		{
			memset(data[channel*2+j], 0, sizeof(short)*len);
		}
	}
	if(bb==6)
	{
		for(j=0;j<channel;++j)
		{
			memset(data[channel*2+j], 0, sizeof(short)*len);
			memset(data[channel+j], 0, sizeof(short)*len);
			memset(data[4*channel+j], 0, sizeof(short)*len);
		}
	}
	pos=0;
	while((len-pos)>=fsize)
	{
		for(i=0;i<fsize;++i)
		{
			for(j=0;j<channel;++j)
			{
				xn[j][i]=data[j][i+pos]+data[channel+j][i+pos];//+data[channel*2+j][i+pos];
				x[j][i]=data[j][i+pos];
				xr[j][i]=data[channel*3+j][i+pos];
			}
		}

		for(j=0;j<channel;++j)
		{
			memmove(rfft_in_tr, analysis_mem_tr[j], fsize*sizeof(float));
			for(i=0;i<fsize;++i)
			{
				rfft_in_tr[i+fsize]=x[j][i];
			}
			memcpy(analysis_mem_tr[j], x[j], fsize*sizeof(float));
			for (i=0;i<wins;++i)
			{
				rfft_in_tr[i] *= gainnet_bf5->window[i];
			}
			forward_transform(gainnet_bf5, fftn[j], rfft_in_tr);
		}

		for(j=0;j<channel;++j)
		{
			memmove(rfft_in_tr, analysis_mem_tr2[j], fsize*sizeof(float));
			for(i=0;i<fsize;++i)
			{
				rfft_in_tr[i+fsize]=xr[j][i];
			}
			memcpy(analysis_mem_tr2[j], xr[j], fsize*sizeof(float));
			for (i=0;i<wins;++i)
			{
				rfft_in_tr[i] *= gainnet_bf5->window[i];
			}
			forward_transform(gainnet_bf5, fftr[j], rfft_in_tr);
		}

		for(i=0; i<channel; ++i)
		{
			frame_analysis(gainnet_bf5, rfft_in, analysis_mem[i], fft[i], xn[i]);
		}
		gainnet_bf5->training=1;
		wtk_gainnet_bf5_feed_bf(gainnet_bf5);
        for(k=0; k<nbin; ++k)
        {
            ovec2=ovec[0][k];
            ta=tb=0;
            for(i=0; i<channel; ++i, ++ovec2)
            {
                ta+=ovec2->a*fftn[i][k].a + ovec2->b*fftn[i][k].b;
                tb+=ovec2->a*fftn[i][k].b - ovec2->b*fftn[i][k].a;
            }
            fftn2[k].a=ta;
            fftn2[k].b=tb;


            ovec2=ovec[0][k];
            ta=tb=0;
            for(i=0; i<channel; ++i, ++ovec2)
            {
                ta+=ovec2->a*fftr[i][k].a + ovec2->b*fftr[i][k].b;
                tb+=ovec2->a*fftr[i][k].b - ovec2->b*fftr[i][k].a;
            }
            fftr2[k].a=ta;
            fftr2[k].b=tb;
        }
		compute_band_energy(En, eband, nb_bands, fftn2);
		compute_band_energy(Er, eband, nb_bands, fftr2);

		for (i=0;i<nb_bands;++i)
		{
			if(gtfmask->silence || Ex[i] < 5e-2 || En[i] < 5e-2)
			{
				gn[i]=0;
			}else
			{
				gn[i] = sqrt((En[i])/(Ex[i]));
			}
			if (gn[i] > 1) gn[i] = 1;

			if(gtfmask->silence || Ex[i] < 5e-2 || Er[i] < 5e-2)
			{
				g[i]=0;
			}else
			{
				g[i] = sqrt((Er[i])/(Ex[i]));
			}
			if (g[i] > 1) g[i] = 1;
		}
		// {
		// 	interp_band_gain(eband, nb_bands, nbin, gtfmask->gf, gn);
		// 	for (i=1;i<nbin-1;i++)
		// 	{
		// 		ffts[i].a *= gtfmask->gf[i];
		// 		ffts[i].b *= gtfmask->gf[i];
		// 	}
		// 	frame_synthesis(gainnet_bf5, gainnet_bf5->out,ffts);
		// 	pv=(short *)gainnet_bf5->out;
		// 	for(i=0;i<fsize;++i)
		// 	{
		// 		pv[i]=gainnet_bf5->out[i];
		// 	}
		// 	wtk_strbuf_push(outbuf, (char *)pv, sizeof(short)*fsize);
		// }

		if(gainnet_bf5->notify_tr)
		{
			gainnet_bf5->notify_tr(gainnet_bf5->ths_tr, gtfmask->features, nb_features,  g, nb_bands);;//, gn, nb_bands);
		}
		pos+=fsize;
	}
	// {
	// 	wtk_wavfile_t *wav;
	// 	wav=wtk_wavfile_new(gainnet_bf5->cfg->rate);
	// 	wav->max_pend=0;
	// 	wtk_wavfile_open(wav,"o.wav");
	// 	wtk_wavfile_write(wav,(char *)outbuf->data,outbuf->pos);
	// 	wtk_wavfile_delete(wav);
	// }

	wtk_float_delete_p2(xn,channel);
	wtk_float_delete_p2(x,channel);
	wtk_float_delete_p2(xr,channel);

	wtk_free(gn);
	wtk_free(En);
	wtk_free(g);
	wtk_free(Er);

	wtk_free(fftr2);
	wtk_free(fftn2);

	wtk_complex_delete_p2(fftr,channel);
	wtk_complex_delete_p2(fftn,channel);

	wtk_float_delete_p2(analysis_mem_tr2,channel);
	wtk_float_delete_p2(analysis_mem_tr,channel);
	wtk_free(rfft_in_tr);
}



void wtk_gainnet_bf5_feed_train2(wtk_gainnet_bf5_t *gainnet_bf5,short **data,short **data2,short **datar, int len, int enr)
{
	int channel=gainnet_bf5->cfg->channel;
	int fsize=gainnet_bf5->cfg->wins/2;
	int nbin=gainnet_bf5->nbin;
	int i,j,k;
	float **xn;
	float **xr;
	float *g;
	float *Er;
	wtk_complex_t **fftr,*fftr2;
	wtk_gainnet_bf5_tfmask_t *gtfmask=gainnet_bf5->gtfmask;
	int *eband=gtfmask->eband;
	int nb_bands=gtfmask->cfg->nb_bands;
	int nb_features=gtfmask->cfg->nb_features;
	float *Ex=gtfmask->Ex;
	int wins=gainnet_bf5->cfg->wins;
	float **analysis_mem_tr2;
	float *rfft_in_tr;
	int pos;
	wtk_complex_t ***ovec=gainnet_bf5->ovec,*ovec2;
	float ta,tb;
	//   short *pv;
	//   wtk_strbuf_t *outbuf=wtk_strbuf_new(1024,2);
	//   wtk_complex_t *ffts=gainnet_bf5->ffts;
	float *rfft_in=gainnet_bf5->rfft_in;
	wtk_complex_t **fft=gainnet_bf5->fft;
	float **analysis_mem=gainnet_bf5->analysis_mem;

	xn=wtk_float_new_p2(channel,fsize);
	xr=wtk_float_new_p2(channel,fsize);

	g=wtk_malloc(sizeof(float)*(nb_bands));
	Er=wtk_malloc(sizeof(float)*(nb_bands));

	fftr=wtk_complex_new_p2(channel,nbin);
	fftr2=wtk_malloc(sizeof(wtk_complex_t)*(nbin));

	analysis_mem_tr2=wtk_float_new_p2(channel,fsize);
	rfft_in_tr=wtk_calloc(sizeof(float),wins);

	if(enr==2)
	{
		for(j=0;j<channel;++j)
		{
			memset(data[j], 0, sizeof(short)*len);
			memset(datar[j], 0, sizeof(short)*len);
		}
	}
	if(enr==3)
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
			memmove(rfft_in_tr, analysis_mem_tr2[j], fsize*sizeof(float));
			for(i=0;i<fsize;++i)
			{
				rfft_in_tr[i+fsize]=xr[j][i];
			}
			memcpy(analysis_mem_tr2[j], xr[j], fsize*sizeof(float));
			for (i=0;i<wins;++i)
			{
				rfft_in_tr[i] *= gainnet_bf5->window[i];
			}
			forward_transform(gainnet_bf5, fftr[j], rfft_in_tr);
		}

		for(i=0; i<channel; ++i)
		{
			frame_analysis(gainnet_bf5, rfft_in, analysis_mem[i], fft[i], xn[i]);
		}
		gainnet_bf5->training=1;
		wtk_gainnet_bf5_feed_bf(gainnet_bf5);
        for(k=0; k<nbin; ++k)
        {
            ovec2=ovec[0][k];
            ta=tb=0;
            for(i=0; i<channel; ++i, ++ovec2)
            {
                ta+=ovec2->a*fftr[i][k].a + ovec2->b*fftr[i][k].b;
                tb+=ovec2->a*fftr[i][k].b - ovec2->b*fftr[i][k].a;
            }
            fftr2[k].a=ta;
            fftr2[k].b=tb;
        }
		compute_band_energy(Er, eband, nb_bands, fftr2);

		for (i=0;i<nb_bands;++i)
		{
			if(gtfmask->silence || Ex[i] < 5e-2 || Er[i] < 5e-2)
			{
				g[i]=0;
			}else
			{
				g[i] = sqrt((Er[i])/(Ex[i]));
			}
			if (g[i] > 1) g[i] = 1;
		}
		// {
		// 	interp_band_gain(eband, nb_bands, nbin, gtfmask->gf, g);
		// 	// for (i=1;i<nbin-1;i++)
		// 	// {
		// 	// 	ffts[i].a *= gtfmask->gf[i];
		// 	// 	ffts[i].b *= gtfmask->gf[i];
		// 	// }
		// 	frame_synthesis(gainnet_bf5, gainnet_bf5->out,ffts);
		// 	pv=(short *)gainnet_bf5->out;
		// 	for(i=0;i<fsize;++i)
		// 	{
		// 		pv[i]=gainnet_bf5->out[i];
		// 	}
		// 	wtk_strbuf_push(outbuf, (char *)pv, sizeof(short)*fsize);
		// }

		if(gainnet_bf5->notify_tr)
		{
			gainnet_bf5->notify_tr(gainnet_bf5->ths_tr, gtfmask->features, nb_features,  g, nb_bands);
		}
		pos+=fsize;
	}
	// {
	// 	wtk_wavfile_t *wav;
	// 	wav=wtk_wavfile_new(gainnet_bf5->cfg->rate);
	// 	wav->max_pend=0;
	// 	wtk_wavfile_open(wav,"o.wav");
	// 	wtk_wavfile_write(wav,(char *)outbuf->data,outbuf->pos);
	// 	wtk_wavfile_delete(wav);
	// }

	wtk_float_delete_p2(xn,channel);
	wtk_float_delete_p2(xr,channel);

	wtk_free(g);
	wtk_free(Er);

	wtk_free(fftr2);

	wtk_complex_delete_p2(fftr,channel);
	wtk_float_delete_p2(analysis_mem_tr2,channel);
	wtk_free(rfft_in_tr);
}