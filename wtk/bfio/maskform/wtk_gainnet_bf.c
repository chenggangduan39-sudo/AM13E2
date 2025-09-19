#include "wtk_gainnet_bf.h"
#define gainnet_bf_tobank(n)   (13.1f*atan(.00074f*(n))+2.24f*atan((n)*(n)*1.85e-8f)+1e-4f*(n))


void wtk_gainnet_bf_denoise_on_gainnet(wtk_gainnet_bf_denoise_t *qdenoise, float *gain, int len, int is_end);
void wtk_gainnet_bf_on_qenvl(wtk_gainnet_bf_t *qdenoise,wtk_qenvelope_msg_t *msg,wtk_qenvelope_state_t state,int is_end);
void wtk_gainnet_bf_on_ssl2(wtk_gainnet_bf_t *qdenoise, wtk_ssl2_extp_t *nbest_extp,int nbest, int ts,int te);
void wtk_gainnet_bf_agc_on_gainnet(wtk_gainnet_bf_agc_t *gagc, float *gain, int len, int is_end);

void wtk_gainnet_bf_xfilterbank_init(int *eband, int bands,int rate,int len)
{
	float df;
	float max_mel, mel_interval;
	int i;
	int id;
	float curr_freq;
	float mel;

	df =rate*1.0/(2*(len-1));
	max_mel = gainnet_bf_tobank(rate/2);
	mel_interval =max_mel/(bands-1);
	for(i=0; i<bands; ++i)
   {
	   eband[i]=-1;
   }
   for (i=0;i<len;++i)
   {
		curr_freq = i*df;
		// printf("%f\n",curr_freq);
		mel = gainnet_bf_tobank(curr_freq);
		if (mel > max_mel)
		{
			break;
		}
		id = (int)(floor(mel/mel_interval));
		if(eband[id]==-1)
		{
			eband[id]=i;
		}
   }
   eband[bands-1]=len-1;
//    for(i=0; i<bands; ++i)
//    {
// 	   printf("%d ",eband[i]);
//    }
//    printf("\n");
}


void wtk_gainnet_bf_denoise_init(wtk_gainnet_bf_denoise_t *qdenoise, wtk_gainnet_bf_cfg_t *cfg)
{
    qdenoise->cfg=cfg;
    qdenoise->nbin=cfg->wins/2+1;

    qdenoise->Ex=wtk_malloc(sizeof(float)*cfg->nb_bands);
    qdenoise->Ep=wtk_malloc(sizeof(float)*cfg->nb_bands);
    qdenoise->Exp=wtk_malloc(sizeof(float)*cfg->nb_bands);
    qdenoise->Exp_dct=wtk_malloc(sizeof(float)*cfg->nb_bands);
    qdenoise->fft_p=(wtk_complex_t*)wtk_malloc(qdenoise->nbin*sizeof(wtk_complex_t)); 
    qdenoise->pitch_buf=wtk_malloc(sizeof(float)*cfg->pitch_buf_size);
    qdenoise->pitch_buf_tmp=wtk_malloc(sizeof(float)*(cfg->pitch_buf_size>>1));

    qdenoise->r=NULL;
    if(cfg->use_pitchpost)
    {
        qdenoise->r=wtk_malloc(sizeof(float)*cfg->nb_bands);
        qdenoise->rf=wtk_malloc(sizeof(float)*qdenoise->nbin);
        qdenoise->newE=wtk_malloc(sizeof(float)*cfg->nb_bands);
        qdenoise->norm=wtk_malloc(sizeof(float)*cfg->nb_bands);
        qdenoise->normf=wtk_malloc(sizeof(float)*qdenoise->nbin);
    }

    qdenoise->dct_table=wtk_malloc(sizeof(float)*cfg->nb_bands*cfg->nb_bands);
    qdenoise->cepstral_mem=wtk_float_new_p2(cfg->ceps_mem, cfg->nb_bands);

    qdenoise->lastg=wtk_malloc(sizeof(float)*cfg->nb_bands);
    qdenoise->g=wtk_malloc(sizeof(float)*cfg->nb_bands);
    qdenoise->gf=wtk_malloc(sizeof(float)*qdenoise->nbin);

    qdenoise->Ly=wtk_malloc(sizeof(float)*cfg->nb_bands);
    qdenoise->features=wtk_malloc(sizeof(float)*cfg->nb_features);

    qdenoise->gainnet=wtk_gainnet_new(cfg->gainnet);
    wtk_gainnet_set_notify(qdenoise->gainnet, qdenoise, (wtk_gainnet_notify_f)wtk_gainnet_bf_denoise_on_gainnet);

	qdenoise->qmmse=NULL;
	if(cfg->use_qmmse)
	{
		qdenoise->qmmse=wtk_qmmse_new(&(cfg->qmmse));
	}
}

void wtk_gainnet_bf_denoise_clean(wtk_gainnet_bf_denoise_t *qdenoise)
{
    wtk_free(qdenoise->Ex);
    wtk_free(qdenoise->fft_p);
    wtk_free(qdenoise->Ep);
    wtk_free(qdenoise->Exp);
    wtk_free(qdenoise->Exp_dct);
    wtk_free(qdenoise->pitch_buf);
    wtk_free(qdenoise->pitch_buf_tmp);

    if(qdenoise->r)
    {
        wtk_free(qdenoise->r);
        wtk_free(qdenoise->rf);
        wtk_free(qdenoise->newE);
        wtk_free(qdenoise->norm);
        wtk_free(qdenoise->normf);
    }

    wtk_free(qdenoise->dct_table);
    wtk_float_delete_p2(qdenoise->cepstral_mem, qdenoise->cfg->ceps_mem);

    wtk_free(qdenoise->lastg);
    wtk_free(qdenoise->g);
    wtk_free(qdenoise->gf);

    wtk_free(qdenoise->Ly);
    wtk_free(qdenoise->features);

    wtk_gainnet_delete(qdenoise->gainnet);
    if(qdenoise->qmmse)
	{
		wtk_qmmse_delete(qdenoise->qmmse);
	}

}

void wtk_gainnet_bf_denoise_reset(wtk_gainnet_bf_denoise_t *qdenoise)
{
    int i,j;
    int nb_bands=qdenoise->cfg->nb_bands;

    for (i=0;i<nb_bands;++i) 
    {
        for (j=0;j<nb_bands;++j)
        {
            qdenoise->dct_table[i*nb_bands + j] = cos((i+.5)*j*PI/nb_bands);
            if (j==0)
            {
                qdenoise->dct_table[i*nb_bands + j] *= sqrt(.5);
            }
        }
    }

    memset(qdenoise->Ex, 0, sizeof(float)*(qdenoise->cfg->nb_bands));
    memset(qdenoise->Ep, 0, sizeof(float)*(qdenoise->cfg->nb_bands));
    memset(qdenoise->Exp, 0, sizeof(float)*(qdenoise->cfg->nb_bands));
    memset(qdenoise->Exp_dct, 0, sizeof(float)*(qdenoise->cfg->nb_bands));

    memset(qdenoise->pitch_buf, 0, sizeof(float)*qdenoise->cfg->pitch_buf_size);
    memset(qdenoise->pitch_buf_tmp, 0, sizeof(float)*(qdenoise->cfg->pitch_buf_size>>1));

    memset(qdenoise->fft_p, 0, sizeof(wtk_complex_t)*(qdenoise->nbin));

    if(qdenoise->r)
    {
        memset(qdenoise->r, 0, sizeof(float)*qdenoise->cfg->nb_bands);
        memset(qdenoise->rf, 0, sizeof(float)*qdenoise->nbin);

        memset(qdenoise->newE, 0, sizeof(float)*qdenoise->cfg->nb_bands);
        memset(qdenoise->norm, 0, sizeof(float)*qdenoise->cfg->nb_bands);
        memset(qdenoise->normf, 0, sizeof(float)*qdenoise->nbin);
    }

    qdenoise->memid=0;
    qdenoise->last_gain=0;
    qdenoise->last_period=0;
    wtk_float_zero_p2(qdenoise->cepstral_mem, qdenoise->cfg->ceps_mem, qdenoise->cfg->nb_bands);

    memset(qdenoise->lastg, 0, sizeof(float)*qdenoise->cfg->nb_bands);
    memset(qdenoise->g, 0, sizeof(float)*qdenoise->cfg->nb_bands);
    memset(qdenoise->gf, 0, sizeof(float)*qdenoise->nbin);

    memset(qdenoise->Ly, 0, sizeof(float)*qdenoise->cfg->nb_bands);
    memset(qdenoise->features, 0, sizeof(float)*qdenoise->cfg->nb_features);

    wtk_gainnet_reset(qdenoise->gainnet);
    if(qdenoise->qmmse)
	{
		wtk_qmmse_reset(qdenoise->qmmse);
	}
}


wtk_gainnet_bf_agc_t *wtk_gainnet_bf_agc_new(wtk_gainnet_bf_cfg_t *cfg)
{
	wtk_gainnet_bf_agc_t *gagc;

	gagc=(wtk_gainnet_bf_agc_t *)wtk_malloc(sizeof(wtk_gainnet_bf_agc_t));
	gagc->cfg=cfg;
	gagc->nbin=cfg->wins/2+1;

	gagc->eband=wtk_malloc(sizeof(int)*cfg->nb_bands);
	wtk_gainnet_bf_xfilterbank_init(gagc->eband, cfg->nb_bands, cfg->rate, gagc->nbin);

	gagc->Ex=wtk_malloc(sizeof(float)*cfg->nb_bands);

	gagc->dct_table=wtk_malloc(sizeof(float)*cfg->nb_bands*cfg->nb_bands);

    gagc->cepstral_mem=wtk_float_new_p2(cfg->ceps_mem, cfg->nb_bands);

	gagc->g=wtk_malloc(sizeof(float)*cfg->nb_bands);
	gagc->gf=wtk_malloc(sizeof(float)*gagc->nbin);

	gagc->Ly=wtk_malloc(sizeof(float)*cfg->nb_bands);
	gagc->features=wtk_malloc(sizeof(float)*cfg->nb_features);

    gagc->gainnet=wtk_gainnet_new(cfg->agc_gainnet);
    wtk_gainnet_set_notify(gagc->gainnet, gagc, (wtk_gainnet_notify_f)wtk_gainnet_bf_agc_on_gainnet);

	return gagc;
}

void wtk_gainnet_bf_agc_delete(wtk_gainnet_bf_agc_t *gagc)
{
	wtk_free(gagc->eband);
	wtk_free(gagc->Ex);

	wtk_free(gagc->dct_table);

	if(gagc->cepstral_mem)
	{
		wtk_float_delete_p2(gagc->cepstral_mem, gagc->cfg->ceps_mem);
	}

	wtk_free(gagc->g);
	wtk_free(gagc->gf);

	wtk_free(gagc->Ly);
	wtk_free(gagc->features);

	wtk_gainnet_delete(gagc->gainnet);

	wtk_free(gagc);
}

void wtk_gainnet_bf_agc_reset(wtk_gainnet_bf_agc_t *gagc)
{
	int i,j;
	int nb_bands=gagc->cfg->nb_bands;

	for (i=0;i<nb_bands;++i) 
	{
		for (j=0;j<nb_bands;++j)
		{
			gagc->dct_table[i*nb_bands + j] = cos((i+.5)*j*PI/nb_bands);
			if (j==0)
			{
			gagc->dct_table[i*nb_bands + j] *= sqrt(.5);
			}
		}
	}

	memset(gagc->Ex, 0, sizeof(float)*(gagc->cfg->nb_bands));

	gagc->memid=0;

	if(gagc->cepstral_mem)
	{
		wtk_float_zero_p2(gagc->cepstral_mem, gagc->cfg->ceps_mem, gagc->cfg->nb_bands);
	}

	memset(gagc->g, 0, sizeof(float)*gagc->cfg->nb_bands);
	memset(gagc->gf, 0, sizeof(float)*gagc->nbin);

	memset(gagc->Ly, 0, sizeof(float)*gagc->cfg->nb_bands);
	memset(gagc->features, 0, sizeof(float)*gagc->cfg->nb_features);

	wtk_gainnet_reset(gagc->gainnet);
}



wtk_gainnet_bf_t* wtk_gainnet_bf_new(wtk_gainnet_bf_cfg_t *cfg)
{
    wtk_gainnet_bf_t *gainnet_bf;
    int i;

    gainnet_bf=(wtk_gainnet_bf_t *)wtk_malloc(sizeof(wtk_gainnet_bf_t));
    gainnet_bf->cfg=cfg;
    gainnet_bf->ths=NULL;
    gainnet_bf->notify=NULL;
    gainnet_bf->ssl_ths=NULL;
    gainnet_bf->notify_ssl=NULL;

    gainnet_bf->mic=wtk_strbufs_new(gainnet_bf->cfg->channel);

    gainnet_bf->mem_hp_x=wtk_float_new_p2(cfg->channel, 2);
    gainnet_bf->a_hp=wtk_float_new_p2(cfg->channel, 2);
    gainnet_bf->b_hp=wtk_float_new_p2(cfg->channel, 2);

    gainnet_bf->nbin=cfg->wins/2+1;
    gainnet_bf->half_window=wtk_malloc(sizeof(float)*cfg->wins/2);
    gainnet_bf->analysis_mem=wtk_float_new_p2(cfg->channel, gainnet_bf->nbin-1);
    gainnet_bf->synthesis_mem=wtk_malloc(sizeof(float)*(gainnet_bf->nbin-1));
    gainnet_bf->rfft=wtk_drft_new(cfg->wins);
    gainnet_bf->rfft_in=(float*)wtk_malloc(sizeof(float)*(cfg->wins));

    gainnet_bf->fft=wtk_complex_new_p2(cfg->channel, gainnet_bf->nbin);
    gainnet_bf->ffts=wtk_complex_new_p2(cfg->channel, gainnet_bf->nbin);
    gainnet_bf->ffty=wtk_complex_new_p2(cfg->channel, gainnet_bf->nbin);

    if(cfg->use_denoise_single)
    {
        gainnet_bf->qdenoise=(wtk_gainnet_bf_denoise_t *)wtk_malloc(sizeof(wtk_gainnet_bf_denoise_t));
        wtk_gainnet_bf_denoise_init(gainnet_bf->qdenoise, cfg);
    }else
    {
        gainnet_bf->qdenoise=(wtk_gainnet_bf_denoise_t *)wtk_malloc(sizeof(wtk_gainnet_bf_denoise_t)*cfg->channel);
        for(i=0; i<cfg->channel; ++i)
        {
            wtk_gainnet_bf_denoise_init(gainnet_bf->qdenoise+i, cfg);
        }
    }

    gainnet_bf->out=wtk_malloc(sizeof(float)*(gainnet_bf->nbin-1));

    gainnet_bf->qenvl=NULL;
    gainnet_bf->ssl2=NULL;
    if(cfg->use_ssl)
    {
        gainnet_bf->qenvl=wtk_qenvelope_new(&(cfg->qenvl));
        wtk_qenvelope_set_notify(gainnet_bf->qenvl, gainnet_bf, (wtk_qenvelope_notify_f)wtk_gainnet_bf_on_qenvl);

        gainnet_bf->ssl2=wtk_ssl2_new(&(cfg->ssl2));
        wtk_ssl2_set_notify(gainnet_bf->ssl2, gainnet_bf, (wtk_ssl2_notify_f)wtk_gainnet_bf_on_ssl2);
    }

    gainnet_bf->covm=wtk_covm_new(&(cfg->covm),gainnet_bf->nbin,cfg->channel);
    gainnet_bf->bf=wtk_bf_new(&(cfg->bf),cfg->wins);

	gainnet_bf->agc=NULL;
	if(cfg->use_agc)
	{
		gainnet_bf->agc = wtk_gainnet_bf_agc_new(cfg);
	}

    wtk_gainnet_bf_reset(gainnet_bf);

    return gainnet_bf;
}

void wtk_gainnet_bf_delete(wtk_gainnet_bf_t *gainnet_bf)
{
    int i,channel=gainnet_bf->cfg->channel;

    wtk_float_delete_p2(gainnet_bf->mem_hp_x, gainnet_bf->cfg->channel);
    wtk_float_delete_p2(gainnet_bf->a_hp, gainnet_bf->cfg->channel);
    wtk_float_delete_p2(gainnet_bf->b_hp, gainnet_bf->cfg->channel);

    wtk_strbufs_delete(gainnet_bf->mic,gainnet_bf->cfg->channel);
    wtk_free(gainnet_bf->half_window);
    wtk_float_delete_p2(gainnet_bf->analysis_mem, gainnet_bf->cfg->channel);
    wtk_free(gainnet_bf->synthesis_mem);

    wtk_free(gainnet_bf->rfft_in);
    wtk_drft_delete(gainnet_bf->rfft);
    wtk_complex_delete_p2(gainnet_bf->fft, gainnet_bf->cfg->channel);
    wtk_complex_delete_p2(gainnet_bf->ffts, gainnet_bf->cfg->channel);
    wtk_complex_delete_p2(gainnet_bf->ffty, gainnet_bf->cfg->channel);

    wtk_free(gainnet_bf->out);

    if(gainnet_bf->cfg->use_denoise_single)
    {
        wtk_gainnet_bf_denoise_clean(gainnet_bf->qdenoise);
    }else
    {
        for(i=0; i<channel; ++i)
        {
            wtk_gainnet_bf_denoise_clean(gainnet_bf->qdenoise+i);
        }
    }
    wtk_free(gainnet_bf->qdenoise);

    if(gainnet_bf->ssl2)
    {
        wtk_qenvelope_delete(gainnet_bf->qenvl);
        wtk_ssl2_delete(gainnet_bf->ssl2);
    }

    wtk_covm_delete(gainnet_bf->covm);
    wtk_bf_delete(gainnet_bf->bf);

	if(gainnet_bf->agc)
	{
		wtk_gainnet_bf_agc_delete(gainnet_bf->agc);
	}

    wtk_free(gainnet_bf);
}

void wtk_gainnet_bf_reset(wtk_gainnet_bf_t *gainnet_bf)
{
    int frame_size=gainnet_bf->cfg->wins/2;
    int i;
    int channel=gainnet_bf->cfg->channel;

    for(i=0; i<channel; ++i)
    {
        gainnet_bf->mem_hp_x[i][0]=gainnet_bf->mem_hp_x[i][1]=0;
        gainnet_bf->a_hp[i][0] = -1.99599;
        gainnet_bf->a_hp[i][1] = 0.99600;
        gainnet_bf->b_hp[i][0] = -2;
        gainnet_bf->b_hp[i][1] = 1;
    }

    wtk_strbufs_reset(gainnet_bf->mic,channel);
    for (i=0;i<frame_size;++i)
    {
        gainnet_bf->half_window[i] = sin(.5*PI*sin(.5*PI*(i+.5)/frame_size) * sin(.5*PI*(i+.5)/frame_size));
    }
    wtk_float_zero_p2(gainnet_bf->analysis_mem, gainnet_bf->cfg->channel, (gainnet_bf->nbin-1));
    memset(gainnet_bf->synthesis_mem, 0, sizeof(float)*(gainnet_bf->nbin-1));

    wtk_complex_zero_p2(gainnet_bf->fft, gainnet_bf->cfg->channel, gainnet_bf->nbin);
    wtk_complex_zero_p2(gainnet_bf->ffts, gainnet_bf->cfg->channel, gainnet_bf->nbin);
    wtk_complex_zero_p2(gainnet_bf->ffty, gainnet_bf->cfg->channel, gainnet_bf->nbin);

    if(gainnet_bf->cfg->use_denoise_single)
    {
        wtk_gainnet_bf_denoise_reset(gainnet_bf->qdenoise);
    }else
    {
        for(i=0; i<channel; ++i)
        {
            wtk_gainnet_bf_denoise_reset(gainnet_bf->qdenoise+i);
        }
    }

    gainnet_bf->sil=1;
    if(gainnet_bf->ssl2)
    {
        wtk_qenvelope_reset(gainnet_bf->qenvl);
        wtk_ssl2_reset(gainnet_bf->ssl2);
    }

    wtk_covm_reset(gainnet_bf->covm);
    wtk_bf_reset(gainnet_bf->bf);

	gainnet_bf->pframe=0;

	if(gainnet_bf->agc)
	{
		wtk_gainnet_bf_agc_reset(gainnet_bf->agc);
	}

}

void wtk_gainnet_bf_start(wtk_gainnet_bf_t *gainnet_bf)
{
    wtk_bf_update_ovec(gainnet_bf->bf,gainnet_bf->cfg->theta,gainnet_bf->cfg->phi);
    wtk_bf_init_w(gainnet_bf->bf);
}

void wtk_gainnet_bf_set_notify(wtk_gainnet_bf_t *gainnet_bf,void *ths,wtk_gainnet_bf_notify_f notify)
{
    gainnet_bf->notify=notify;
    gainnet_bf->ths=ths;
}

void wtk_gainnet_bf_set_ssl_notify(wtk_gainnet_bf_t *gainnet_bf,void *ths,wtk_gainnet_bf_notify_ssl_f notify)
{
    gainnet_bf->ssl_ths=ths;
    gainnet_bf->notify_ssl=notify;
}

static void biquad(float *y, float mem[2], const float *x, const float *b, const float *a, int N) 
{
    int i;
    float xi, yi;
    
    for (i=0;i<N;++i) 
    {
        xi = x[i];
        yi = x[i] + mem[0];
        mem[0] = mem[1] + (b[0]*(double)xi - a[0]*(double)yi);
        mem[1] = (b[1]*(double)xi - a[1]*(double)yi);
        y[i] = yi;
    }
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


static void compute_band_corr(float *bandE, int *eband, int nb_bands, wtk_complex_t *fft, wtk_complex_t *fft2)
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
        for (j=0;j<band_size;++j)
        {
            frac = (float)j/band_size;
            tmp = fft[eband[i] + j].a* fft2[eband[i] + j].a+ fft[eband[i] + j].b* fft2[eband[i] + j].b;
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

static void inverse_transform(wtk_gainnet_bf_t *gainnet_bf, wtk_complex_t *fft, float *out)
{
    wtk_drft_ifft2(gainnet_bf->rfft,fft,out);
}

static void forward_transform(wtk_gainnet_bf_t *gainnet_bf, wtk_complex_t *fft, float *in)
{
    wtk_drft_fft2(gainnet_bf->rfft,in,fft);
}

static void frame_analysis(wtk_gainnet_bf_t *gainnet_bf, float *rfft_in, float *analysis_mem, wtk_complex_t *fft, const float *in)
{
    int i;
    int wins=gainnet_bf->cfg->wins;
    int fsize=wins/2;

    memmove(rfft_in, analysis_mem, fsize*sizeof(float));
    for(i=0;i<fsize;++i)
    {
        rfft_in[i+fsize]=in[i];
    }
    memcpy(analysis_mem, in, fsize*sizeof(float));
    for (i=0;i<fsize;++i)
    {
        rfft_in[i] *= gainnet_bf->half_window[i];
        rfft_in[wins - 1 - i] *= gainnet_bf->half_window[i];
    }
    forward_transform(gainnet_bf, fft, rfft_in);
}


static void frame_synthesis(wtk_gainnet_bf_t *gainnet_bf, float *rfft_in, float *synthesis_mem, float *out, wtk_complex_t *fft)
{
    int i;
    int wins=gainnet_bf->cfg->wins;
    int fsize=wins/2;

    inverse_transform(gainnet_bf, fft, rfft_in);
    for (i=0;i<fsize;++i)
    {
        rfft_in[i] *= gainnet_bf->half_window[i];
        rfft_in[wins - 1 - i] *= gainnet_bf->half_window[i];
    }
    for (i=0;i<fsize;i++) out[i] = rfft_in[i] + synthesis_mem[i];
    memcpy(synthesis_mem, &rfft_in[fsize], fsize*sizeof(float));
}


static void pitch_filter(wtk_gainnet_bf_denoise_t *qdenoise, wtk_complex_t *X, wtk_complex_t *P, const float *Ex, const float *Ep,
const float *Exp, const float *g)
{
    int i;
    int nb_bands=qdenoise->cfg->nb_bands;
    int nbin=qdenoise->nbin;
    float *r=qdenoise->r;
    float *rf=qdenoise->rf;
    float *newE=qdenoise->newE;
    float *norm=qdenoise->norm;
    float *normf=qdenoise->normf;

    for (i=0;i<nb_bands;i++)
    {
        #if 0
        if (Exp[i]>g[i]) r[i] = 1;
        else r[i] = Exp[i]*(1-g[i])/(.001 + g[i]*(1-Exp[i]));
        r[i] = min(1, max(0, r[i]));
        #else
        if (Exp[i]>g[i]) r[i] = 1;
        else r[i] = Exp[i]*Exp[i]*(1-g[i]*g[i])/(.001 + g[i]*g[i]*(1-Exp[i]*Exp[i]));
        r[i] = sqrt(min(1, max(0, r[i])));
        #endif
        r[i] *= sqrt(Ex[i]/(1e-8+Ep[i]));
    }
    interp_band_gain(qdenoise->cfg->eband, nb_bands, nbin, rf, r);
    for (i=0;i<nbin;++i)
    {
        X[i].a += rf[i]*P[i].a;
        X[i].b += rf[i]*P[i].b;
    }

    compute_band_energy(newE, qdenoise->cfg->eband, nb_bands, X);
    for (i=0;i<nb_bands;i++)
    {
        norm[i] = sqrt(Ex[i]/(1e-8+newE[i]));
    }
    interp_band_gain(qdenoise->cfg->eband, nb_bands, nbin, normf, norm);
    for (i=0;i<nbin;++i)
    {
        X[i].a *= normf[i];
        X[i].b *= normf[i];
    }
}

static int compute_frame_features(wtk_gainnet_bf_t *gainnet_bf, wtk_gainnet_bf_denoise_t *qdenoise, float *features, wtk_complex_t *fftx, float *data)
{
    wtk_complex_t *fft_p=qdenoise->fft_p;
    float *Ex=qdenoise->Ex, *Ep=qdenoise->Ep, *Exp=qdenoise->Exp, *Exp_dct=qdenoise->Exp_dct;
    int *eband=qdenoise->cfg->eband;
    int wins=qdenoise->cfg->wins;
    int fsize=qdenoise->cfg->wins/2;
    int pitch_buf_size=qdenoise->cfg->pitch_buf_size;
    int pitch_max_period=qdenoise->cfg->pitch_max_period;
    int pitch_min_period=qdenoise->cfg->pitch_min_period;
    int pitch_frame_size=qdenoise->cfg->pitch_frame_size;
    float *pitch_buf=qdenoise->pitch_buf;
    float *pitch_buf_tmp=qdenoise->pitch_buf_tmp;
    int i,j,k;
    float E = 0;
    float *ceps_0, *ceps_1, *ceps_2;
    float spec_variability = 0;
    float *Ly=qdenoise->Ly;
    int nb_bands=qdenoise->cfg->nb_bands;
    int nb_delta_ceps=qdenoise->cfg->nb_delta_ceps;
    int ceps_mem=qdenoise->cfg->ceps_mem;
    float *rfft_in=gainnet_bf->rfft_in;
    float *half_window=gainnet_bf->half_window;
    float *pre[1], gain;
    int pitch_index;
    float follow, logMax;
    float mindist,dist,tmp;
    int nb_features=qdenoise->cfg->nb_features;

    compute_band_energy(Ex, eband, nb_bands, fftx);
    memmove(pitch_buf, &pitch_buf[fsize],(pitch_buf_size-fsize)*sizeof(float));
    memcpy(&pitch_buf[pitch_buf_size-fsize], data ,fsize*sizeof(float));
    pre[0] = &pitch_buf[0];
    pitch_downsample(pre, pitch_buf_tmp, pitch_buf_size, 1);
    pitch_search(pitch_buf_tmp+(pitch_max_period>>1), pitch_buf_tmp, pitch_frame_size,
    pitch_max_period-3*pitch_min_period, &pitch_index);
    pitch_index = pitch_max_period-pitch_index;
    gain = remove_doubling(pitch_buf_tmp, pitch_max_period, pitch_min_period,
    pitch_frame_size, &pitch_index, qdenoise->last_period, qdenoise->last_gain);
    qdenoise->last_period = pitch_index;
    qdenoise->last_gain = gain;
    for (i=0;i<wins;++i)
    {
        rfft_in[i] = pitch_buf[pitch_buf_size-wins-pitch_index+i];
    }
    for (i=0;i<fsize;++i)
    {
        rfft_in[i] *= half_window[i];
        rfft_in[wins - 1 - i] *= half_window[i];
    }
    forward_transform(gainnet_bf, fft_p, rfft_in);
    compute_band_energy(Ep, qdenoise->cfg->eband,nb_bands,fft_p);
    compute_band_corr(Exp, qdenoise->cfg->eband,nb_bands, fftx, fft_p);
    for (i=0;i<nb_bands;i++)
    {
        Exp[i] = Exp[i]/sqrt(.001+Ex[i]*Ep[i]);
    }
    dct(qdenoise->dct_table, nb_bands, Exp_dct, Exp);
    for (i=0;i<nb_delta_ceps;i++)
    {
        features[nb_bands+2*nb_delta_ceps+i] = Exp_dct[i];
    }
    features[nb_bands+2*nb_delta_ceps] -= 1.3;
    features[nb_bands+2*nb_delta_ceps+1] -= 0.9;
    features[nb_bands+3*nb_delta_ceps] = .01*(pitch_index-300);

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
    if (E < 0.04)
    {
        memset(features, 0, sizeof(float)*nb_features);
        return 1;
    }
    dct(qdenoise->dct_table, nb_bands, features, Ly);
    features[0] -= 12;
    features[1] -= 4;
    ceps_0 = qdenoise->cepstral_mem[qdenoise->memid];
    ceps_1 = (qdenoise->memid < 1) ? qdenoise->cepstral_mem[ceps_mem+qdenoise->memid-1] : qdenoise->cepstral_mem[qdenoise->memid-1];
    ceps_2 = (qdenoise->memid < 2) ? qdenoise->cepstral_mem[ceps_mem+qdenoise->memid-2] : qdenoise->cepstral_mem[qdenoise->memid-2];
    for (i=0;i<nb_bands;i++)
    {
        ceps_0[i] = features[i];
    }
    qdenoise->memid++;
    for (i=0;i<nb_delta_ceps;i++)
    {
        features[i] = ceps_0[i] + ceps_1[i] + ceps_2[i];
        features[nb_bands+i] = ceps_0[i] - ceps_2[i];
        features[nb_bands+nb_delta_ceps+i] =  ceps_0[i] - 2*ceps_1[i] + ceps_2[i];
    }
    /* Spectral variability features. */
    if (qdenoise->memid == ceps_mem)
    {
        qdenoise->memid = 0;
    }
    for (i=0;i<ceps_mem;++i)
    {
        mindist = 1e15f;
        for (j=0;j<ceps_mem;++j)
        {
            dist=0;
            for (k=0;k<nb_bands;++k)
            {
                tmp = qdenoise->cepstral_mem[i][k] - qdenoise->cepstral_mem[j][k];
                dist += tmp*tmp;
            }
            if (j!=i)
            {
                mindist = min(mindist, dist);
            }
        }
        spec_variability += mindist;
    }
    features[nb_bands+3*nb_delta_ceps+1] = spec_variability/ceps_mem-2.1;

    return 0;
}

void wtk_gainnet_bf_denoise_on_gainnet(wtk_gainnet_bf_denoise_t *qdenoise, float *gain, int len, int is_end)
{
    memcpy(qdenoise->g, gain, sizeof(float)*qdenoise->cfg->nb_bands);
}

void wtk_gainnet_bf_denoise_feed(wtk_gainnet_bf_t *gainnet_bf, wtk_gainnet_bf_denoise_t *qdenoise, wtk_complex_t *fftx, float *data, wtk_complex_t *ffts, wtk_complex_t *ffty)
{
    int i;
    int nb_bands=qdenoise->cfg->nb_bands;
    int nbin=qdenoise->nbin;
    float *g=qdenoise->g, *gf=qdenoise->gf, *lastg=qdenoise->lastg;
    int *eband=qdenoise->cfg->eband;
    wtk_gainnet_t *gainnet=qdenoise->gainnet;
    int nb_features=qdenoise->cfg->nb_features;
    int silence;
    wtk_complex_t *fft_p=qdenoise->fft_p;
    float *Ex=qdenoise->Ex, *Ep=qdenoise->Ep, *Exp=qdenoise->Exp;
    float *qmmse_gain;
	
    if(qdenoise->qmmse)
	{
		wtk_qmmse_flush_denoise_mask(qdenoise->qmmse, fftx);
	}
    silence = compute_frame_features(gainnet_bf, qdenoise, qdenoise->features, fftx, data);
    if (!silence)
    {
        wtk_gainnet_feed(gainnet, qdenoise->features, nb_features, 0);   
        if(qdenoise->cfg->use_pitchpost)
        {
            pitch_filter(qdenoise, fftx, fft_p, Ex, Ep, Exp, g);
            for (i=0;i<nb_bands;++i)
            {
                // g[i] = max(g[i], 0.6*lastg[i]);
                lastg[i] = g[i];
            }
        }else
        {
            for (i=0;i<nb_bands;++i)
            {
                lastg[i] = g[i];
            }
        }
        interp_band_gain(eband, nb_bands, nbin, gf, g);
        if(qdenoise->qmmse)
        {
            qmmse_gain=qdenoise->qmmse->gain;
            for (i=1; i<nbin-1; ++i)
            {
                gf[i]=min(gf[i], qmmse_gain[i]);
            }
        }
    }else
    {
        for (i=1; i<nbin-1; ++i)
        {
            gf[i]=0;
        }
    }
    for (i=1; i<nbin-1; ++i)
    {
        ffty[i].a = fftx[i].a*(1-gf[i]);
        ffty[i].b = fftx[i].b*(1-gf[i]);

        ffts[i].a =  fftx[i].a*gf[i];
        ffts[i].b =  fftx[i].b*gf[i];
    }

}


void wtk_gainnet_bf_feed_denoise(wtk_gainnet_bf_t *gainnet_bf, wtk_complex_t **fft, float **data, wtk_complex_t **ffts, wtk_complex_t **ffty)
{
    int channel=gainnet_bf->cfg->channel;
    // int fsize=gainnet_bf->cfg->wins/2;
    int i,k;
    int nbin=gainnet_bf->nbin;
    wtk_gainnet_bf_denoise_t *qdenoise=gainnet_bf->qdenoise;
    float pframe_alpha=gainnet_bf->cfg->pframe_alpha;

    if(gainnet_bf->cfg->use_denoise_single)
    {
        wtk_gainnet_bf_denoise_feed(gainnet_bf, qdenoise, fft[0], data[0], ffts[0], ffty[0]);
        for(i=1; i<channel; ++i)
        {
            for (k=1; k<nbin-1; ++k)
            {	
                ffty[i][k].a = fft[i][k].a*(1-qdenoise->gf[k]);
                ffty[i][k].b = fft[i][k].b*(1-qdenoise->gf[k]);

                ffts[i][k].a = fft[i][k].a*qdenoise->gf[k];
                ffts[i][k].b = fft[i][k].b*qdenoise->gf[k];
            }
        }
    }else
    {
        for(i=0; i<channel; ++i, ++qdenoise)
        {
            wtk_gainnet_bf_denoise_feed(gainnet_bf, qdenoise, fft[i], data[i], ffts[i], ffty[i]);
        }
    }
	gainnet_bf->pframe=(1-pframe_alpha)*gainnet_bf->pframe+pframe_alpha*wtk_float_abs_mean(qdenoise->gf+2, nbin-3);
}

void wtk_gainnet_bf_on_qenvl(wtk_gainnet_bf_t *gainnet_bf,wtk_qenvelope_msg_t *msg,wtk_qenvelope_state_t state,int is_end)
{
    wtk_complex_t **fft;
    static float nframe=0;

    if(msg)
    {
        ++nframe;
        fft=(wtk_complex_t **)(msg->hook);
        if(state == WTK_QENVELOPE_CREST || state == WTK_QENVELOPE_FLAT)
        {
            if(gainnet_bf->sil==1)
            {
                gainnet_bf->sil=0;
                printf("speech start %f ms\n",((nframe-1)*gainnet_bf->cfg->wins/2)/(gainnet_bf->cfg->rate/1000));
            }
           wtk_ssl2_feed_fft(gainnet_bf->ssl2, fft, 0);
        }else
        {
            if(gainnet_bf->sil==0)
            {
                gainnet_bf->sil=1;
                printf("speech end %f ms\n",((nframe-1)*gainnet_bf->cfg->wins/2)/(gainnet_bf->cfg->rate/1000));
                wtk_ssl2_feed_fft(gainnet_bf->ssl2, NULL, 1);
                wtk_ssl2_reset(gainnet_bf->ssl2);
            }
        }
    }
}

void wtk_gainnet_bf_on_ssl2(wtk_gainnet_bf_t *gainnet_bf, wtk_ssl2_extp_t *nbest_extp,int nbest, int ts,int te)
{
    if(gainnet_bf->notify_ssl)
    {
        gainnet_bf->notify_ssl(gainnet_bf->ssl_ths, nbest_extp, nbest);
    }
}

void wtk_gainnet_bf_feed_ssl(wtk_gainnet_bf_t *gainnet_bf, wtk_complex_t **fft)
{
    int k;
    int nbin=gainnet_bf->nbin;
    wtk_gainnet_bf_denoise_t *qdenoise=gainnet_bf->qdenoise;
    float specsum=0;

    for (k=1; k<nbin-1; ++k)
    {
        specsum += qdenoise->gf[k];
    }
    wtk_qenvelope_feed(gainnet_bf->qenvl, specsum, fft, 0);
}

void wtk_gainnet_bf_feed_bf(wtk_gainnet_bf_t *gainnet_bf, wtk_complex_t **fft, wtk_complex_t **ffts, wtk_complex_t **ffty)
{
    wtk_covm_t *covm=gainnet_bf->covm;
    wtk_bf_t *bf=gainnet_bf->bf;
    wtk_complex_t ifft[64];
    int channel=bf->channel;
    int nbin=bf->nbin;
    int k,i;
    int b;

    for(k=1;k<nbin-1;++k)
    {
        b=wtk_covm_feed_fft(covm, ffty, k, 1);
        if(b)
        {
            wtk_bf_update_ncov(gainnet_bf->bf, covm->ncov, k);
        }
        if(covm->scov)
        {
            b=wtk_covm_feed_fft(covm, ffts, k, 0);
            if(b)
            {
                wtk_bf_update_scov(gainnet_bf->bf, covm->scov, k);
            }
        }
        if(b)
        {
            wtk_bf_update_w(gainnet_bf->bf,k);
        }
        for(i=0;i<channel;++i)
        {
            ifft[i]=ffts[i][k];
        }
        wtk_bf_output_fft_k(bf, ifft, fft[0]+k, k);
    }
    fft[0][0].a=fft[0][0].b=0;
    fft[0][nbin-1].a=fft[0][nbin-1].b=0;
}


static int compute_frame_gagc_features(wtk_gainnet_bf_agc_t *gagc, float *features, wtk_complex_t *fftx)
{
	float *Ex=gagc->Ex;
	int *eband=gagc->eband;
	int i,j,k;
	float E = 0;
	float *ceps_0, *ceps_1, *ceps_2;
	float spec_variability = 0;
	float *Ly=gagc->Ly;
	int nb_bands=gagc->cfg->nb_bands;
	int nb_delta_ceps=gagc->cfg->nb_delta_ceps;
	int ceps_mem=gagc->cfg->ceps_mem;
	float follow, logMax;
	float mindist,dist,tmp;

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
	dct(gagc->dct_table, nb_bands, features, Ly);
	
	if(gagc->cepstral_mem)
	{
		features[0] -= 12;
		features[1] -= 4;
		ceps_0 = gagc->cepstral_mem[gagc->memid];
		ceps_1 = (gagc->memid < 1) ? gagc->cepstral_mem[ceps_mem+gagc->memid-1] : gagc->cepstral_mem[gagc->memid-1];
		ceps_2 = (gagc->memid < 2) ? gagc->cepstral_mem[ceps_mem+gagc->memid-2] : gagc->cepstral_mem[gagc->memid-2];
		for (i=0;i<nb_bands;i++)
		{
			ceps_0[i] = features[i];
		}
		gagc->memid++;
		for (i=0;i<nb_delta_ceps;i++)
		{
			features[i] = ceps_0[i] + ceps_1[i] + ceps_2[i];
			features[nb_bands+i] = ceps_0[i] - ceps_2[i];
			features[nb_bands+nb_delta_ceps+i] =  ceps_0[i] - 2*ceps_1[i] + ceps_2[i];
		}
		/* Spectral variability features. */
		if (gagc->memid == ceps_mem)
		{
			gagc->memid = 0;
		}
		for (i=0;i<ceps_mem;++i)
		{
			mindist = 1e15f;
			for (j=0;j<ceps_mem;++j)
			{
					dist=0;
				for (k=0;k<nb_bands;++k)
				{
					tmp = gagc->cepstral_mem[i][k] - gagc->cepstral_mem[j][k];
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

void wtk_gainnet_bf_agc_on_gainnet(wtk_gainnet_bf_agc_t *gagc, float *gain, int len, int is_end)
{
	int i;
	int nb_bands=gagc->cfg->nb_bands;
	float *g=gagc->g;
	float agc_a=gagc->cfg->agc_a;
	float agc_b=gagc->cfg->agc_b;

	for(i=0; i<nb_bands; ++i)
	{
		g[i]=-1/agc_a*(logf(1/gain[i]-1)-agc_b);
	}
}

void wtk_gainnet_bf_feed_agc(wtk_gainnet_bf_t *gainnet_bf, wtk_complex_t * fft)
{
	wtk_gainnet_bf_agc_t *gagc=gainnet_bf->agc;
	int silence;
	wtk_gainnet_t *gainnet=gagc->gainnet;
	int *eband=gagc->eband;
	int nb_bands=gagc->cfg->nb_bands;
	int nbin=gagc->nbin;
	float *gf=gagc->gf;
	float *g=gagc->g;
	float pframe_thresh=gainnet_bf->cfg->pframe_thresh;
	int i;

	silence=compute_frame_gagc_features(gagc,  gagc->features, fft);
	wtk_gainnet_feed(gainnet, gagc->features, gagc->cfg->agc_nb_features, 0);
	if (!silence)
	{
		interp_band_gain(eband, nb_bands, nbin, gf, g);
		if(gainnet_bf->pframe>pframe_thresh)
		{
			for (i=1;i<nbin-1;++i)
			{
				fft[i].a *= gf[i];
				fft[i].b *= gf[i];
			}
		}
	}
}

void wtk_gainnet_bf_feed(wtk_gainnet_bf_t *gainnet_bf,short **data,int len,int is_end)
{
#ifdef DEBUG_WAV
	static wtk_wavfile_t *mic_log=NULL;

	if(!mic_log)
	{
		mic_log=wtk_wavfile_new(16000);
		wtk_wavfile_set_channel(mic_log,gainnet_bf->cfg->channel);
		wtk_wavfile_open2(mic_log,"gainnet_bf");
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

    int i,j;
    // int nbin=gainnet_bf->nbin;
    int channel=gainnet_bf->cfg->channel;
    wtk_strbuf_t **mic=gainnet_bf->mic;
    float **mem_hp_x=gainnet_bf->mem_hp_x;
    float **b_hp=gainnet_bf->b_hp;
    float **a_hp=gainnet_bf->a_hp;
    float fv, *fp1, *fp2[64];
    int wins=gainnet_bf->cfg->wins;
    int fsize=wins/2;
    int length;
    float *rfft_in=gainnet_bf->rfft_in;
    wtk_complex_t **fft=gainnet_bf->fft, **ffts=gainnet_bf->ffts, **ffty=gainnet_bf->ffty;
    float **analysis_mem=gainnet_bf->analysis_mem;
    float *synthesis_mem=gainnet_bf->synthesis_mem;
    float *out=gainnet_bf->out, *outtmp;
    short *pv;

    for(i=0; i<channel; ++i)
    {
        for(j=0; j<len; ++j)
        {
            fv=data[i][j];
            wtk_strbuf_push(mic[i],(char *)&(fv),sizeof(float));
        }
    }
    length=mic[0]->pos/sizeof(float);
    while(length>=fsize)
    {
        for(i=0; i<channel; ++i)
        {
            fp1=(float *)mic[i]->data;
            biquad(fp1, mem_hp_x[i], fp1, b_hp[i], a_hp[i], fsize);
            frame_analysis(gainnet_bf, rfft_in, analysis_mem[i], fft[i], fp1);
            // memcpy(fft2[i], fft[i], sizeof(wtk_complex_t)*nbin);
            fp2[i]=fp1;
        }
        wtk_gainnet_bf_feed_denoise(gainnet_bf, fft, fp2, ffts, ffty);
        if(gainnet_bf->ssl2)
        {
            wtk_gainnet_bf_feed_ssl(gainnet_bf, ffts);
        }
        wtk_gainnet_bf_feed_bf(gainnet_bf, fft, ffts, ffty);
        if(gainnet_bf->cfg->use_agc)
        {
            wtk_gainnet_bf_feed_agc(gainnet_bf, fft[0]);
        }

        wtk_strbufs_pop(mic, channel, fsize*sizeof(float));
        length=mic[0]->pos/sizeof(float);

        frame_synthesis(gainnet_bf, rfft_in, synthesis_mem, out, fft[0]);
        pv=(short *)out;
        for(j=0; j<fsize; ++j)
        {
            if(fabs(out[j])<32767.0)
            {
                pv[j]=floorf(out[j]+0.5);
            }else
            {
                if(out[j]>0)
                {
                    pv[j]=32767;
                }else
                {
                    pv[j]=-32767;
                }
            }
        }
        if(gainnet_bf->notify)
        {
            gainnet_bf->notify(gainnet_bf->ths,pv,fsize,0);
        }
    }
    if(is_end)
    {
        length=mic[0]->pos/sizeof(float);
        if(length>0)
        {
            if(gainnet_bf->notify)
            {
                pv=(short *)mic[0]->data;
                outtmp=(float *)mic[0]->data;
                for(j=0; j<length; ++j)
                {
                    pv[j]=floorf(outtmp[j]+0.5);
                }
                gainnet_bf->notify(gainnet_bf->ths,pv,length,1);
            }
        }else
        {
            if(gainnet_bf->notify)
            {
                gainnet_bf->notify(gainnet_bf->ths,NULL,0,1);
            }  
        }
    }
}
