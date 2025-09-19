#include "wtk_gainnet_bf2.h"

void wtk_gainnet_bf2_denoise_on_gainnet(wtk_gainnet_bf2_denoise_t *qdenoise, float *gain, int len, int is_end);
void wtk_gainnet_bf2_on_qenvl(wtk_gainnet_bf2_t *qdenoise,wtk_qenvelope_msg_t *msg,wtk_qenvelope_state_t state,int is_end);
void wtk_gainnet_bf2_on_ssl2(wtk_gainnet_bf2_t *qdenoise, wtk_ssl2_extp_t *nbest_extp,int nbest, int ts,int te);

void wtk_gainnet_bf2_denoise_init(wtk_gainnet_bf2_denoise_t *qdenoise, wtk_gainnet_bf2_cfg_t *cfg)
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
    wtk_gainnet_set_notify(qdenoise->gainnet, qdenoise, (wtk_gainnet_notify_f)wtk_gainnet_bf2_denoise_on_gainnet);

	qdenoise->qmmse=NULL;
	if(cfg->use_qmmse)
	{
		qdenoise->qmmse=wtk_qmmse_new(&(cfg->qmmse));
	}
}

void wtk_gainnet_bf2_denoise_clean(wtk_gainnet_bf2_denoise_t *qdenoise)
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

void wtk_gainnet_bf2_denoise_reset(wtk_gainnet_bf2_denoise_t *qdenoise)
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

wtk_gainnet_bf2_t* wtk_gainnet_bf2_new(wtk_gainnet_bf2_cfg_t *cfg)
{
    wtk_gainnet_bf2_t *gainnet_bf2;
    int i;

    gainnet_bf2=(wtk_gainnet_bf2_t *)wtk_malloc(sizeof(wtk_gainnet_bf2_t));
    gainnet_bf2->cfg=cfg;
    gainnet_bf2->ths=NULL;
    gainnet_bf2->notify=NULL;
    gainnet_bf2->ssl_ths=NULL;
    gainnet_bf2->notify_ssl=NULL;

    gainnet_bf2->mic=wtk_strbufs_new(gainnet_bf2->cfg->channel);

    gainnet_bf2->mem_hp_x=wtk_float_new_p2(cfg->channel, 2);
    gainnet_bf2->a_hp=wtk_float_new_p2(cfg->channel, 2);
    gainnet_bf2->b_hp=wtk_float_new_p2(cfg->channel, 2);

    gainnet_bf2->nbin=cfg->wins/2+1;
    gainnet_bf2->half_window=wtk_malloc(sizeof(float)*cfg->wins/2);
    gainnet_bf2->analysis_mem=wtk_float_new_p2(cfg->channel, gainnet_bf2->nbin-1);
    gainnet_bf2->synthesis_mem=wtk_malloc(sizeof(float)*(gainnet_bf2->nbin-1));
    gainnet_bf2->rfft=wtk_drft_new(cfg->wins);
    gainnet_bf2->rfft_in=(float*)wtk_malloc(sizeof(float)*(cfg->wins));

    gainnet_bf2->fft=wtk_complex_new_p2(cfg->channel, gainnet_bf2->nbin);
    gainnet_bf2->ffts=wtk_complex_new_p2(cfg->channel, gainnet_bf2->nbin);
    gainnet_bf2->ffty=wtk_complex_new_p2(cfg->channel, gainnet_bf2->nbin);

    if(cfg->use_denoise_single)
    {
        gainnet_bf2->qdenoise=(wtk_gainnet_bf2_denoise_t *)wtk_malloc(sizeof(wtk_gainnet_bf2_denoise_t));
        wtk_gainnet_bf2_denoise_init(gainnet_bf2->qdenoise, cfg);
    }else
    {
        gainnet_bf2->qdenoise=(wtk_gainnet_bf2_denoise_t *)wtk_malloc(sizeof(wtk_gainnet_bf2_denoise_t)*cfg->channel);
        for(i=0; i<cfg->channel; ++i)
        {
            wtk_gainnet_bf2_denoise_init(gainnet_bf2->qdenoise+i, cfg);
        }
    }

    gainnet_bf2->out=wtk_malloc(sizeof(float)*(gainnet_bf2->nbin-1));

    gainnet_bf2->qenvl=NULL;
    gainnet_bf2->ssl2=NULL;
    if(cfg->use_ssl)
    {
        gainnet_bf2->qenvl=wtk_qenvelope_new(&(cfg->qenvl));
        wtk_qenvelope_set_notify(gainnet_bf2->qenvl, gainnet_bf2, (wtk_qenvelope_notify_f)wtk_gainnet_bf2_on_qenvl);

        gainnet_bf2->ssl2=wtk_ssl2_new(&(cfg->ssl2));
        wtk_ssl2_set_notify(gainnet_bf2->ssl2, gainnet_bf2, (wtk_ssl2_notify_f)wtk_gainnet_bf2_on_ssl2);
    }

    gainnet_bf2->wpd=wtk_wpd_new(&(cfg->wpd),gainnet_bf2->nbin);

    wtk_gainnet_bf2_reset(gainnet_bf2);

    return gainnet_bf2;
}

void wtk_gainnet_bf2_delete(wtk_gainnet_bf2_t *gainnet_bf2)
{
    int i,channel=gainnet_bf2->cfg->channel;

    wtk_float_delete_p2(gainnet_bf2->mem_hp_x, gainnet_bf2->cfg->channel);
    wtk_float_delete_p2(gainnet_bf2->a_hp, gainnet_bf2->cfg->channel);
    wtk_float_delete_p2(gainnet_bf2->b_hp, gainnet_bf2->cfg->channel);

    wtk_strbufs_delete(gainnet_bf2->mic,gainnet_bf2->cfg->channel);
    wtk_free(gainnet_bf2->half_window);
    wtk_float_delete_p2(gainnet_bf2->analysis_mem, gainnet_bf2->cfg->channel);
    wtk_free(gainnet_bf2->synthesis_mem);

    wtk_free(gainnet_bf2->rfft_in);
    wtk_drft_delete(gainnet_bf2->rfft);
    wtk_complex_delete_p2(gainnet_bf2->fft, gainnet_bf2->cfg->channel);
    wtk_complex_delete_p2(gainnet_bf2->ffts, gainnet_bf2->cfg->channel);
    wtk_complex_delete_p2(gainnet_bf2->ffty, gainnet_bf2->cfg->channel);

    wtk_free(gainnet_bf2->out);

    if(gainnet_bf2->cfg->use_denoise_single)
    {
        wtk_gainnet_bf2_denoise_clean(gainnet_bf2->qdenoise);
    }else
    {
        for(i=0; i<channel; ++i)
        {
            wtk_gainnet_bf2_denoise_clean(gainnet_bf2->qdenoise+i);
        }
    }
    wtk_free(gainnet_bf2->qdenoise);

    if(gainnet_bf2->ssl2)
    {
        wtk_qenvelope_delete(gainnet_bf2->qenvl);
        wtk_ssl2_delete(gainnet_bf2->ssl2);
    }

    wtk_wpd_delete(gainnet_bf2->wpd);

    wtk_free(gainnet_bf2);
}

void wtk_gainnet_bf2_reset(wtk_gainnet_bf2_t *gainnet_bf2)
{
    int frame_size=gainnet_bf2->cfg->wins/2;
    int i;
    int channel=gainnet_bf2->cfg->channel;

    for(i=0; i<channel; ++i)
    {
        gainnet_bf2->mem_hp_x[i][0]=gainnet_bf2->mem_hp_x[i][1]=0;
        gainnet_bf2->a_hp[i][0] = -1.99599;
        gainnet_bf2->a_hp[i][1] = 0.99600;
        gainnet_bf2->b_hp[i][0] = -2;
        gainnet_bf2->b_hp[i][1] = 1;
    }

    wtk_strbufs_reset(gainnet_bf2->mic,channel);
    for (i=0;i<frame_size;++i)
    {
        gainnet_bf2->half_window[i] = sin(.5*PI*sin(.5*PI*(i+.5)/frame_size) * sin(.5*PI*(i+.5)/frame_size));
    }
    wtk_float_zero_p2(gainnet_bf2->analysis_mem, gainnet_bf2->cfg->channel, (gainnet_bf2->nbin-1));
    memset(gainnet_bf2->synthesis_mem, 0, sizeof(float)*(gainnet_bf2->nbin-1));

    wtk_complex_zero_p2(gainnet_bf2->fft, gainnet_bf2->cfg->channel, gainnet_bf2->nbin);
    wtk_complex_zero_p2(gainnet_bf2->ffts, gainnet_bf2->cfg->channel, gainnet_bf2->nbin);
    wtk_complex_zero_p2(gainnet_bf2->ffty, gainnet_bf2->cfg->channel, gainnet_bf2->nbin);

    if(gainnet_bf2->cfg->use_denoise_single)
    {
        wtk_gainnet_bf2_denoise_reset(gainnet_bf2->qdenoise);
    }else
    {
        for(i=0; i<channel; ++i)
        {
            wtk_gainnet_bf2_denoise_reset(gainnet_bf2->qdenoise+i);
        }
    }

    gainnet_bf2->sil=1;
    if(gainnet_bf2->ssl2)
    {
        wtk_qenvelope_reset(gainnet_bf2->qenvl);
        wtk_ssl2_reset(gainnet_bf2->ssl2);
    }

    wtk_wpd_reset(gainnet_bf2->wpd);
}

void wtk_gainnet_bf2_start(wtk_gainnet_bf2_t *gainnet_bf2)
{
    wtk_wpd_start(gainnet_bf2->wpd, gainnet_bf2->cfg->theta, gainnet_bf2->cfg->phi);
}

void wtk_gainnet_bf2_set_notify(wtk_gainnet_bf2_t *gainnet_bf2,void *ths,wtk_gainnet_bf2_notify_f notify)
{
    gainnet_bf2->notify=notify;
    gainnet_bf2->ths=ths;
}

void wtk_gainnet_bf2_set_ssl_notify(wtk_gainnet_bf2_t *gainnet_bf2,void *ths,wtk_gainnet_bf2_notify_ssl_f notify)
{
    gainnet_bf2->ssl_ths=ths;
    gainnet_bf2->notify_ssl=notify;
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

static void dct(wtk_gainnet_bf2_denoise_t *qdenoise, float *out, const float *in)
{
    int i,j;
    int nb_bands=qdenoise->cfg->nb_bands;
    float sum;

    for (i=0;i<nb_bands;++i)
    {
        sum = 0;
        for (j=0;j<nb_bands;++j)
        {
            sum += in[j] * qdenoise->dct_table[j*nb_bands + i];
        }
        out[i] = sum*sqrt(2./nb_bands);
    }
}

static void inverse_transform(wtk_gainnet_bf2_t *gainnet_bf2, wtk_complex_t *fft, float *out)
{
    wtk_drft_ifft2(gainnet_bf2->rfft,fft,out);
}

static void forward_transform(wtk_gainnet_bf2_t *gainnet_bf2, wtk_complex_t *fft, float *in)
{
    wtk_drft_fft2(gainnet_bf2->rfft,in,fft);
}

static void frame_analysis(wtk_gainnet_bf2_t *gainnet_bf2, float *rfft_in, float *analysis_mem, wtk_complex_t *fft, const float *in)
{
    int i;
    int wins=gainnet_bf2->cfg->wins;
    int fsize=wins/2;

    memmove(rfft_in, analysis_mem, fsize*sizeof(float));
    for(i=0;i<fsize;++i)
    {
        rfft_in[i+fsize]=in[i];
    }
    memcpy(analysis_mem, in, fsize*sizeof(float));
    for (i=0;i<fsize;++i)
    {
        rfft_in[i] *= gainnet_bf2->half_window[i];
        rfft_in[wins - 1 - i] *= gainnet_bf2->half_window[i];
    }
    forward_transform(gainnet_bf2, fft, rfft_in);
}


static void frame_synthesis(wtk_gainnet_bf2_t *gainnet_bf2, float *rfft_in, float *synthesis_mem, float *out, wtk_complex_t *fft)
{
    int i;
    int wins=gainnet_bf2->cfg->wins;
    int fsize=wins/2;

    inverse_transform(gainnet_bf2, fft, rfft_in);
    for (i=0;i<fsize;++i)
    {
        rfft_in[i] *= gainnet_bf2->half_window[i];
        rfft_in[wins - 1 - i] *= gainnet_bf2->half_window[i];
    }
    for (i=0;i<fsize;i++) out[i] = rfft_in[i] + synthesis_mem[i];
    memcpy(synthesis_mem, &rfft_in[fsize], fsize*sizeof(float));
}


static void pitch_filter(wtk_gainnet_bf2_denoise_t *qdenoise, wtk_complex_t *X, wtk_complex_t *P, const float *Ex, const float *Ep,
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

static int compute_frame_features(wtk_gainnet_bf2_t *gainnet_bf2, wtk_gainnet_bf2_denoise_t *qdenoise, float *features, wtk_complex_t *fftx, float *data)
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
    float *rfft_in=gainnet_bf2->rfft_in;
    float *half_window=gainnet_bf2->half_window;
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
    forward_transform(gainnet_bf2, fft_p, rfft_in);
    compute_band_energy(Ep, qdenoise->cfg->eband,nb_bands,fft_p);
    compute_band_corr(Exp, qdenoise->cfg->eband,nb_bands, fftx, fft_p);
    for (i=0;i<nb_bands;i++)
    {
        Exp[i] = Exp[i]/sqrt(.001+Ex[i]*Ep[i]);
    }
    dct(qdenoise, Exp_dct, Exp);
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
    dct(qdenoise, features, Ly);
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

void wtk_gainnet_bf2_denoise_on_gainnet(wtk_gainnet_bf2_denoise_t *qdenoise, float *gain, int len, int is_end)
{
    memcpy(qdenoise->g, gain, sizeof(float)*qdenoise->cfg->nb_bands);
}

void wtk_gainnet_bf2_denoise_feed(wtk_gainnet_bf2_t *gainnet_bf2, wtk_gainnet_bf2_denoise_t *qdenoise, wtk_complex_t *fftx, float *data, wtk_complex_t *ffts, wtk_complex_t *ffty)
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
    silence = compute_frame_features(gainnet_bf2, qdenoise, qdenoise->features, fftx, data);
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


void wtk_gainnet_bf2_feed_bf(wtk_gainnet_bf2_t *gainnet_bf2, wtk_complex_t **fft, float **data, wtk_complex_t **ffts, wtk_complex_t **ffty)
{
    int channel=gainnet_bf2->cfg->channel;
    // int fsize=gainnet_bf2->cfg->wins/2;
    int i,k;
    int nbin=gainnet_bf2->nbin;
    wtk_gainnet_bf2_denoise_t *qdenoise=gainnet_bf2->qdenoise;
    wtk_complex_t fft2[64], ffts2[64];
    float gain2[64];

    if(gainnet_bf2->cfg->use_denoise_single)
    {
        wtk_gainnet_bf2_denoise_feed(gainnet_bf2, qdenoise, fft[0], data[0], ffts[0], ffty[0]);
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
            wtk_gainnet_bf2_denoise_feed(gainnet_bf2, qdenoise, fft[i], data[i], ffts[i], ffty[i]);
        }
    }

    for(k=1; k<nbin-1; ++k)
    {
        qdenoise=gainnet_bf2->qdenoise;
        if(gainnet_bf2->cfg->use_denoise_single)
        {   
            for(i=0; i<channel; ++i)
            { 
                gain2[i]=qdenoise->gf[k]*qdenoise->gf[k];
            }
        }else
        {
            for(i=0; i<channel; ++i, ++qdenoise)
            { 
                gain2[i]=qdenoise->gf[k]*qdenoise->gf[k];
            }
        }

        for(i=0;i<channel;++i)
        {
            fft2[i]=fft[i][k];
            ffts2[i]=ffts[i][k];
        }

        wtk_wpd_feed_k(gainnet_bf2->wpd, fft2, ffts2, gain2, fft[0]+k, k);
    }
    fft[0][0].a=fft[0][0].b=0;
    fft[0][nbin-1].a=fft[0][nbin-1].b=0;

}

void wtk_gainnet_bf2_on_qenvl(wtk_gainnet_bf2_t *gainnet_bf2,wtk_qenvelope_msg_t *msg,wtk_qenvelope_state_t state,int is_end)
{
    wtk_complex_t **fft;
    static float nframe=0;

    if(msg)
    {
        ++nframe;
        fft=(wtk_complex_t **)(msg->hook);
        if(state == WTK_QENVELOPE_CREST || state == WTK_QENVELOPE_FLAT)
        {
            if(gainnet_bf2->sil==1)
            {
                gainnet_bf2->sil=0;
                printf("speech start %f ms\n",((nframe-1)*gainnet_bf2->cfg->wins/2)/(gainnet_bf2->cfg->rate/1000));
            }
           wtk_ssl2_feed_fft(gainnet_bf2->ssl2, fft, 0);
        }else
        {
            if(gainnet_bf2->sil==0)
            {
                gainnet_bf2->sil=1;
                printf("speech end %f ms\n",((nframe-1)*gainnet_bf2->cfg->wins/2)/(gainnet_bf2->cfg->rate/1000));
                wtk_ssl2_feed_fft(gainnet_bf2->ssl2, NULL, 1);
                wtk_ssl2_reset(gainnet_bf2->ssl2);
            }
        }
    }
}

void wtk_gainnet_bf2_on_ssl2(wtk_gainnet_bf2_t *gainnet_bf2, wtk_ssl2_extp_t *nbest_extp,int nbest, int ts,int te)
{
    if(gainnet_bf2->notify_ssl)
    {
        gainnet_bf2->notify_ssl(gainnet_bf2->ssl_ths, nbest_extp, nbest);
    }
}

void wtk_gainnet_bf2_feed_ssl(wtk_gainnet_bf2_t *gainnet_bf2, wtk_complex_t **fft)
{
    int k;
    int nbin=gainnet_bf2->nbin;
    wtk_gainnet_bf2_denoise_t *qdenoise=gainnet_bf2->qdenoise;
    float specsum=0;

    for (k=1; k<nbin-1; ++k)
    {
        specsum += qdenoise->gf[k];
    }
    wtk_qenvelope_feed(gainnet_bf2->qenvl, specsum, fft, 0);
}

void wtk_gainnet_bf2_feed(wtk_gainnet_bf2_t *gainnet_bf2,short **data,int len,int is_end)
{
    int i,j;
    // int nbin=gainnet_bf2->nbin;
    int channel=gainnet_bf2->cfg->channel;
    wtk_strbuf_t **mic=gainnet_bf2->mic;
    float **mem_hp_x=gainnet_bf2->mem_hp_x;
    float **b_hp=gainnet_bf2->b_hp;
    float **a_hp=gainnet_bf2->a_hp;
    float fv, *fp1, *fp2[64];
    int wins=gainnet_bf2->cfg->wins;
    int fsize=wins/2;
    int length;
    float *rfft_in=gainnet_bf2->rfft_in;
    wtk_complex_t **fft=gainnet_bf2->fft, **ffts=gainnet_bf2->ffts, **ffty=gainnet_bf2->ffty;
    float **analysis_mem=gainnet_bf2->analysis_mem;
    float *synthesis_mem=gainnet_bf2->synthesis_mem;
    float *out=gainnet_bf2->out, *outtmp;
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
            frame_analysis(gainnet_bf2, rfft_in, analysis_mem[i], fft[i], fp1);
            // memcpy(fft2[i], fft[i], sizeof(wtk_complex_t)*nbin);
            fp2[i]=fp1;
        }
        wtk_gainnet_bf2_feed_bf(gainnet_bf2, fft, fp2, ffts, ffty);
        if(gainnet_bf2->ssl2)
        {
            wtk_gainnet_bf2_feed_ssl(gainnet_bf2, ffts);
        }

        wtk_strbufs_pop(mic, channel, fsize*sizeof(float));
        length=mic[0]->pos/sizeof(float);

        frame_synthesis(gainnet_bf2, rfft_in, synthesis_mem, out, fft[0]);
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
        if(gainnet_bf2->notify)
        {
            gainnet_bf2->notify(gainnet_bf2->ths,pv,fsize,0);
        }
    }
    if(is_end)
    {
        length=mic[0]->pos/sizeof(float);
        if(length>0)
        {
            if(gainnet_bf2->notify)
            {
                pv=(short *)mic[0]->data;
                outtmp=(float *)mic[0]->data;
                for(j=0; j<length; ++j)
                {
                    pv[j]=floorf(outtmp[j]+0.5);
                }
                gainnet_bf2->notify(gainnet_bf2->ths,pv,length,1);
            }
        }else
        {
            if(gainnet_bf2->notify)
            {
                gainnet_bf2->notify(gainnet_bf2->ths,NULL,0,1);
            }  
        }
    }
}
