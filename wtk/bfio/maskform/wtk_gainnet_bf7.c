#include "wtk_gainnet_bf7.h"
#define gainnet_bf7_tobank(n)   (13.1f*atan(.00074f*(n))+2.24f*atan((n)*(n)*1.85e-8f)+1e-4f*(n))

void wtk_gainnet_bf7_mask_on_masknet(float *mask, float *gain, int len, int is_end);

wtk_gainnet_bf7_t* wtk_gainnet_bf7_new(wtk_gainnet_bf7_cfg_t *cfg)
{
	wtk_gainnet_bf7_t *gainnet_bf7;

	gainnet_bf7=(wtk_gainnet_bf7_t *)wtk_malloc(sizeof(wtk_gainnet_bf7_t));
	gainnet_bf7->cfg=cfg;
	gainnet_bf7->ths=NULL;
	gainnet_bf7->notify=NULL;

	gainnet_bf7->mic=wtk_strbufs_new(gainnet_bf7->cfg->channel);

	gainnet_bf7->notch_mem=NULL;
	gainnet_bf7->memD=NULL;
	if(cfg->use_preemph)
	{
		gainnet_bf7->notch_mem=wtk_float_new_p2(cfg->channel,2);
		gainnet_bf7->memD=(float *)wtk_malloc(sizeof(float)*cfg->channel);
		gainnet_bf7->memX=0;
	}

	gainnet_bf7->nbin=cfg->wins/2+1;
	gainnet_bf7->window=wtk_malloc(sizeof(float)*cfg->wins);///2);
	gainnet_bf7->synthesis_window=wtk_malloc(sizeof(float)*cfg->wins);///2);
	gainnet_bf7->analysis_mem=wtk_float_new_p2(cfg->channel, gainnet_bf7->nbin-1);
	gainnet_bf7->synthesis_mem=wtk_malloc(sizeof(float)*(gainnet_bf7->nbin-1));
	gainnet_bf7->rfft=wtk_drft_new(cfg->wins);
	gainnet_bf7->rfft_in=(float*)wtk_malloc(sizeof(float)*(cfg->wins));

	gainnet_bf7->fft=wtk_complex_new_p2(cfg->channel, gainnet_bf7->nbin);
	gainnet_bf7->ovec=wtk_complex_new_p3(3,gainnet_bf7->nbin, cfg->channel);

	gainnet_bf7->fftx=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*gainnet_bf7->nbin);

	gainnet_bf7->feature=(float*)wtk_malloc(sizeof(float)*(cfg->features_len));

	gainnet_bf7->g=(float *)wtk_malloc(sizeof(float)*(gainnet_bf7->nbin-7));

	gainnet_bf7->masknet=NULL;
	if(cfg->masknet)
	{
		gainnet_bf7->masknet=wtk_masknet_new(cfg->masknet);
		wtk_masknet_set_notify(gainnet_bf7->masknet, gainnet_bf7->g, (wtk_masknet_notify_f)wtk_gainnet_bf7_mask_on_masknet);
	}

	gainnet_bf7->covm=NULL;
	gainnet_bf7->bf=NULL;
	if(cfg->use_bf)
	{
		gainnet_bf7->covm=wtk_covm_new(&(cfg->covm), gainnet_bf7->nbin, cfg->channel);
		gainnet_bf7->bf=wtk_bf_new(&(cfg->bf), gainnet_bf7->cfg->wins);
	}

	gainnet_bf7->out=wtk_malloc(sizeof(float)*(gainnet_bf7->nbin-1));

	wtk_gainnet_bf7_reset(gainnet_bf7);

	return gainnet_bf7;
}

void wtk_gainnet_bf7_delete(wtk_gainnet_bf7_t *gainnet_bf7)
{
	wtk_strbufs_delete(gainnet_bf7->mic,gainnet_bf7->cfg->channel);
	if(gainnet_bf7->notch_mem)
	{
		wtk_float_delete_p2(gainnet_bf7->notch_mem, gainnet_bf7->cfg->channel);
		wtk_free(gainnet_bf7->memD);
	}
	wtk_free(gainnet_bf7->window);
	wtk_free(gainnet_bf7->synthesis_window);
	wtk_float_delete_p2(gainnet_bf7->analysis_mem, gainnet_bf7->cfg->channel);
	wtk_free(gainnet_bf7->synthesis_mem);
	wtk_free(gainnet_bf7->rfft_in);
	wtk_drft_delete(gainnet_bf7->rfft);
	wtk_complex_delete_p2(gainnet_bf7->fft, gainnet_bf7->cfg->channel);
	wtk_complex_delete_p3(gainnet_bf7->ovec, 3, gainnet_bf7->nbin);

	wtk_free(gainnet_bf7->fftx);

	wtk_free(gainnet_bf7->feature);

	wtk_masknet_delete(gainnet_bf7->masknet);

	wtk_free(gainnet_bf7->g);

	if(gainnet_bf7->bf)
	{
		wtk_covm_delete(gainnet_bf7->covm);
		wtk_bf_delete(gainnet_bf7->bf);
	}

	wtk_free(gainnet_bf7->out);

	wtk_free(gainnet_bf7);
}

void wtk_gainnet_bf7_flush_ovec(wtk_gainnet_bf7_t *gainnet_bf7,  wtk_complex_t *ovec, float **mic_pos, float sv, int rate, 
                                                        int theta2, int phi2, int k, float *tdoa)
{
	float x,y,z;
	float t;
	float *mic;
	int j;
	int channel=gainnet_bf7->cfg->channel;
	int win=(gainnet_bf7->nbin-1)*2;
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

void wtk_gainnet_bf7_start(wtk_gainnet_bf7_t *gainnet_bf7, float theta, float phi)
{
	wtk_complex_t ***ovec=gainnet_bf7->ovec, **ovec2, **ovec3, **ovec4;
    int channel=gainnet_bf7->cfg->channel;
    int nbin=gainnet_bf7->nbin;
    wtk_complex_t *a,*b,*b2;
    wtk_complex_t *cov;
    int i,j,k;
    wtk_dcomplex_t *tmp_inv;
	wtk_complex_t *tmp;
    float *tdoa;
	float fa;

    tmp=(wtk_complex_t *)wtk_calloc(channel,sizeof(wtk_complex_t));
    tdoa=(float *)wtk_malloc(channel*sizeof(float));
    cov=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*channel*channel);
    tmp_inv=(wtk_dcomplex_t *)wtk_malloc(channel*(channel+1)*sizeof(wtk_dcomplex_t));

    for(k=6;k<nbin-1;++k)
    {
		ovec2=ovec[0];
		ovec3=ovec[1];
		ovec4=ovec[2];
        memset(cov,0,sizeof(wtk_complex_t)*channel*channel);
		memset(ovec2[k],0,sizeof(wtk_complex_t)*channel);
		memset(ovec3[k],0,sizeof(wtk_complex_t)*channel);
		memset(ovec4[k],0,sizeof(wtk_complex_t)*channel);

		wtk_gainnet_bf7_flush_ovec(gainnet_bf7, ovec3[k], gainnet_bf7->cfg->mic_pos,gainnet_bf7->cfg->speed,gainnet_bf7->cfg->rate, 0,0,k,tdoa);
		wtk_gainnet_bf7_flush_ovec(gainnet_bf7, ovec4[k], gainnet_bf7->cfg->mic_pos,gainnet_bf7->cfg->speed,gainnet_bf7->cfg->rate, 180,0,k,tdoa);
		a=cov;
		b=ovec4[k];
		for(i=0; i<channel; ++i, ++b)
		{
			a+=i;
			b2=ovec4[k]+i;
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
		wtk_complex_guass_elimination_p1(cov, ovec3[k], tmp_inv, channel, tmp);
		fa=0;
		a=tmp;
		b=ovec3[k];
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

		wtk_gainnet_bf7_flush_ovec(gainnet_bf7, ovec4[k], gainnet_bf7->cfg->mic_pos,gainnet_bf7->cfg->speed,gainnet_bf7->cfg->rate, 90,0,k,tdoa);
	}

    wtk_free(tmp);
    wtk_free(tdoa);
    wtk_free(cov);
    wtk_free(tmp_inv);
}

void wtk_gainnet_bf7_reset(wtk_gainnet_bf7_t *gainnet_bf7)
{
	int wins=gainnet_bf7->cfg->wins;
	int frame_size=gainnet_bf7->cfg->wins/2;
	int i;
	int shift, nshift, j, n;

	wtk_strbufs_reset(gainnet_bf7->mic,gainnet_bf7->cfg->channel);
	if(gainnet_bf7->notch_mem)
	{
		for(i=0;i<gainnet_bf7->cfg->channel;++i)
		{
			memset(gainnet_bf7->notch_mem[i],0,2*sizeof(float));
		}
		memset(gainnet_bf7->memD,0,gainnet_bf7->cfg->channel*sizeof(float));
		gainnet_bf7->memX=0;
	}
	for (i=0;i<wins;++i)
	{
		gainnet_bf7->window[i] = sin((0.5+i)*PI/(wins));//sin(.5*PI*sin(.5*PI*(i+.5)/frame_size) * sin(.5*PI*(i+.5)/frame_size));
		gainnet_bf7->synthesis_window[i]=0;
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
				gainnet_bf7->synthesis_window[i] += gainnet_bf7->window[n]*gainnet_bf7->window[n];
			}
		}
	}
	for(i=1;i<nshift;++i)
	{
		for(j=0;j<shift;++j)
		{
			gainnet_bf7->synthesis_window[i*shift+j] = gainnet_bf7->synthesis_window[j];
		}
	}
	for(i=0;i<wins;++i)
	{
		gainnet_bf7->synthesis_window[i]=gainnet_bf7->window[i]/gainnet_bf7->synthesis_window[i];
	}

	wtk_float_zero_p2(gainnet_bf7->analysis_mem, gainnet_bf7->cfg->channel, (gainnet_bf7->nbin-1));
	memset(gainnet_bf7->synthesis_mem, 0, sizeof(float)*(gainnet_bf7->nbin-1));
	wtk_complex_zero_p2(gainnet_bf7->fft, gainnet_bf7->cfg->channel, gainnet_bf7->nbin);

	memset(gainnet_bf7->fftx, 0, sizeof(wtk_complex_t)*(gainnet_bf7->nbin));

	wtk_masknet_reset(gainnet_bf7->masknet);

	memset(gainnet_bf7->feature,0,sizeof(float)*gainnet_bf7->cfg->features_len);

	if(gainnet_bf7->bf)
	{
		wtk_covm_reset(gainnet_bf7->covm);
		wtk_bf_reset(gainnet_bf7->bf);
	}

	gainnet_bf7->training=0;
}


void wtk_gainnet_bf7_set_notify(wtk_gainnet_bf7_t *gainnet_bf7,void *ths,wtk_gainnet_bf7_notify_f notify)
{
	gainnet_bf7->notify=notify;
	gainnet_bf7->ths=ths;
}

void wtk_gainnet_bf7_set_tr_notify(wtk_gainnet_bf7_t *gainnet_bf7,void *ths,wtk_gainnet_bf7_notify_trfeat_f notify)
{
	gainnet_bf7->notify_tr=notify;
	gainnet_bf7->ths_tr=ths;
}

static void inverse_transform(wtk_gainnet_bf7_t *gainnet_bf7, wtk_complex_t *fft, float *out)
{
	wtk_drft_ifft2(gainnet_bf7->rfft,fft,out);
}

static void forward_transform(wtk_gainnet_bf7_t *gainnet_bf7, wtk_complex_t *fft, float *in)
{
	wtk_drft_fft2(gainnet_bf7->rfft,in,fft);
}

static void frame_analysis(wtk_gainnet_bf7_t *gainnet_bf7, float *rfft_in, float *analysis_mem, wtk_complex_t *fft, const float *in)
{
  int i;
  int wins=gainnet_bf7->cfg->wins;
  int fsize=wins/2;

  memmove(rfft_in, analysis_mem, fsize*sizeof(float));
  for(i=0;i<fsize;++i)
  {
    rfft_in[i+fsize]=in[i];
  }
  memcpy(analysis_mem, in, fsize*sizeof(float));
	for (i=0;i<wins;++i)
	{
		rfft_in[i] *= gainnet_bf7->window[i];
		//rfft_in[wins - 1 - i] *= gainnet_bf7->window[i];
	}
  forward_transform(gainnet_bf7, fft, rfft_in);
}

static void frame_synthesis(wtk_gainnet_bf7_t *gainnet_bf7, float *out, wtk_complex_t *fft)
{
  float *rfft_in=gainnet_bf7->rfft_in;
  int i;
  int wins=gainnet_bf7->cfg->wins;
  int fsize=wins/2;
  float *synthesis_mem=gainnet_bf7->synthesis_mem;

  inverse_transform(gainnet_bf7, fft, rfft_in);
  for (i=0;i<wins;++i)
  {
		rfft_in[i] *= gainnet_bf7->synthesis_window[i];
    // rfft_in[i] *= gainnet_bf7->window[i];
    // rfft_in[wins - 1 - i] *= gainnet_bf7->window[i];
  }
  for (i=0;i<fsize;i++) out[i] = rfft_in[i] + synthesis_mem[i];
  memcpy(synthesis_mem, &rfft_in[fsize], fsize*sizeof(float));
}

void wtk_gainnet_bf7_mask_on_masknet(float *mask, float *gain, int len, int is_end)
{
	int i;

	for(i=0; i<len; ++i)
	{
		mask[i]=gain[i];
	}
	// print_float2(mask, len);
}

void wtk_gainnet_bf7_flush(wtk_gainnet_bf7_t *gainnet_bf7)
{
	int k,n;
    int nbin=gainnet_bf7->bf->nbin;
    wtk_bf_t *bf=gainnet_bf7->bf;
    int i, channel=bf->channel;
    int b;
	wtk_complex_t **fft=gainnet_bf7->fft;
    wtk_covm_t *covm=gainnet_bf7->covm;
    wtk_complex_t *fftx=gainnet_bf7->fftx;
	wtk_complex_t ffty[64], ffts[64], fft1[64];
	float *g=gainnet_bf7->g;
	// float ff;

	fftx[0].a=fftx[0].b=0;
	fftx[nbin-1].a=fftx[nbin-1].b=0;
    for(k=6, n=0; k<nbin-1; ++k, ++n)
    {
		for(i=0; i<channel; ++i)
		{
			fft1[i]=fft[i][k];
			// ff=fft1[i].a*fft1[i].a+fft1[i].b*fft1[i].b;
			// if(ff>1e-2)
			// {
			// 	g[n]=sqrtf((fftx[k].a*fftx[k].a+fftx[k].b*fftx[k].b)/ff);
			// }
			ffts[i].a = fft1[i].a*g[n];
			ffts[i].b = fft1[i].b*g[n];

			ffty[i].a = fft1[i].a*(1-g[n]);
			ffty[i].b = fft1[i].b*(1-g[n]);
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
    }
}

void wtk_gainnet_bf7_feed_bf(wtk_gainnet_bf7_t *gainnet_bf7, wtk_complex_t **fft)
{
    wtk_complex_t ***ovec=gainnet_bf7->ovec, *ovec2;
	wtk_complex_t *fftx=gainnet_bf7->fftx,*ffts1;
    int channel=gainnet_bf7->cfg->channel;
    int nbin=gainnet_bf7->nbin;
    int i, k, n, m;
    float ta,tb;
	wtk_masknet_t *masknet=gainnet_bf7->masknet;
	float *feature=gainnet_bf7->feature;
	int features_len=gainnet_bf7->cfg->features_len;
	float ff2;
	float spec1,spec2;
	float *g=gainnet_bf7->g;

	ffts1=fftx+6;
	for(k=6, n=0; k<nbin-1; ++k, ++ffts1, ++n)
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

		ff2=sqrt(ta*ta+tb*tb);
		feature[n] = logf(1e-2+ff2);

		for(i=1, m=0; i<channel; ++i, ++m)
		{
			ta=fft[i][k].a*fft[0][k].a + fft[i][k].b*fft[0][k].b;
			tb=fft[i][k].b*fft[0][k].a - fft[i][k].a*fft[0][k].b;
			feature[nbin-7+n*(channel-1)+m]=atan2f(tb, ta);
		}

		ovec2=ovec[1][k];
		ta=tb=0;
		ff2=0;
		for(i=0; i<channel; ++i, ++ovec2)
		{
			ta+=ovec2->a*fft[i][k].a + ovec2->b*fft[i][k].b;
			tb+=ovec2->a*fft[i][k].b - ovec2->b*fft[i][k].a;
			ff2+=fft[i][k].a*fft[i][k].a+fft[i][k].b*fft[i][k].b;
		}
		spec1=sqrt((ta*ta+tb*tb)/(ff2*channel));

		ovec2=ovec[2][k];
		ta=tb=0;
		ff2=0;
		for(i=0; i<channel; ++i, ++ovec2)
		{
			ta+=ovec2->a*fft[i][k].a + ovec2->b*fft[i][k].b;
			tb+=ovec2->a*fft[i][k].b - ovec2->b*fft[i][k].a;
			ff2+=fft[i][k].a*fft[i][k].a+fft[i][k].b*fft[i][k].b;
		}
		spec2=sqrt((ta*ta+tb*tb)/(ff2*channel));

		if(spec1>spec2)
		{
			feature[(nbin-7)*channel+n] = spec1;
		}else
		{
			feature[(nbin-7)*channel+n] = 0;
		}
	}

	wtk_masknet_feed(masknet, feature, features_len, 0); 

	ffts1=fftx+6;
	for(k=6, n=0; k<nbin-1; ++k, ++ffts1, ++n)
	{
		ffts1->a*=g[n];
		ffts1->b*=g[n];
	}

	if(gainnet_bf7->bf)
	{
		wtk_gainnet_bf7_flush(gainnet_bf7);
	}
}

void wtk_gainnet_bf7_feed(wtk_gainnet_bf7_t *gainnet_bf7,short **data,int len,int is_end)
{
	int i,j;
	// int nbin=gainnet_bf7->nbin;
	int channel=gainnet_bf7->cfg->channel;
	wtk_strbuf_t **mic=gainnet_bf7->mic;
	float **notch_mem=gainnet_bf7->notch_mem;
	float *memD=gainnet_bf7->memD;
	float fv, *fp1;
	int wins=gainnet_bf7->cfg->wins;
	int fsize=wins/2;
	int length;
	float *rfft_in=gainnet_bf7->rfft_in;
	wtk_complex_t **fft=gainnet_bf7->fft;
	wtk_complex_t *fftx=gainnet_bf7->fftx;
	float *out=gainnet_bf7->out;
	short *pv=(short *)out;
	float **analysis_mem=gainnet_bf7->analysis_mem;

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
			frame_analysis(gainnet_bf7, rfft_in, analysis_mem[i], fft[i], fp1);
		}
		wtk_gainnet_bf7_feed_bf(gainnet_bf7, fft);

		wtk_strbufs_pop(mic, channel, fsize*sizeof(float));
		length=mic[0]->pos/sizeof(float);
	    frame_synthesis(gainnet_bf7, out, fftx);
		if(notch_mem)
		{
			gainnet_bf7->memX=wtk_preemph_asis2(out,fsize,gainnet_bf7->memX);
		}
		for(i=0; i<fsize; ++i)
		{
			pv[i]=floorf(out[i]+0.5);
		}
		if(gainnet_bf7->notify)
		{
			gainnet_bf7->notify(gainnet_bf7->ths,pv,fsize);
		}
	}
	if(is_end && length>0)
	{
		if(gainnet_bf7->notify)
		{
			pv=(short *)mic[0]->data;
			out=(float *)mic[0]->data;
			for(i=0; i<length; ++i)
			{
				pv[i]=floorf(out[i]+0.5);
			}
			gainnet_bf7->notify(gainnet_bf7->ths,pv,length);
		}
	}
}


void wtk_gainnet_bf7_feed_bf_tr(wtk_gainnet_bf7_t *gainnet_bf7, wtk_complex_t **fft, float *feature)
{
    wtk_complex_t ***ovec=gainnet_bf7->ovec, *ovec2;
	wtk_complex_t *fftx=gainnet_bf7->fftx,*ffts1;
    int channel=gainnet_bf7->cfg->channel;
    int nbin=gainnet_bf7->nbin;
    int i, k, n, m;
    float ta,tb;
	float ff2;
	float spec1,spec2;

	ffts1=fftx+6;
	for(k=6, n=0; k<nbin-1; ++k, ++ffts1, ++n)
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

		ff2=sqrt(ta*ta+tb*tb);
		feature[n] = logf(1e-2+ff2);

		for(i=1, m=0; i<channel; ++i, ++m)
		{
			ta=fft[i][k].a*fft[0][k].a + fft[i][k].b*fft[0][k].b;
			tb=fft[i][k].b*fft[0][k].a - fft[i][k].a*fft[0][k].b;
			feature[nbin-7+n*(channel-1)+m]=atan2f(tb, ta);
		}

		ovec2=ovec[1][k];
		ta=tb=0;
		ff2=0;
		for(i=0; i<channel; ++i, ++ovec2)
		{
			ta+=ovec2->a*fft[i][k].a + ovec2->b*fft[i][k].b;
			tb+=ovec2->a*fft[i][k].b - ovec2->b*fft[i][k].a;
			ff2+=fft[i][k].a*fft[i][k].a+fft[i][k].b*fft[i][k].b;
		}
		spec1=sqrt((ta*ta+tb*tb)/(ff2*channel));

		ovec2=ovec[2][k];
		ta=tb=0;
		ff2=0;
		for(i=0; i<channel; ++i, ++ovec2)
		{
			ta+=ovec2->a*fft[i][k].a + ovec2->b*fft[i][k].b;
			tb+=ovec2->a*fft[i][k].b - ovec2->b*fft[i][k].a;
			ff2+=fft[i][k].a*fft[i][k].a+fft[i][k].b*fft[i][k].b;
		}
		spec2=sqrt((ta*ta+tb*tb)/(ff2*channel));

		if(spec1>spec2)
		{
			feature[(nbin-7)*channel+n] = spec1;
		}else
		{
			feature[(nbin-7)*channel+n] = 0;
		}
	}
}

void wtk_gainnet_bf7_feed_train3(wtk_gainnet_bf7_t *gainnet_bf7,short **data,short **data2,short **datar, int len, int bb)
{
	int channel=gainnet_bf7->cfg->channel;
	int fsize=gainnet_bf7->cfg->wins/2;
	int nbin=gainnet_bf7->nbin;
	int i,j,k;
	float **xn;
	float **xr;
	float *g;
	float *feat;
	int features_len=gainnet_bf7->cfg->features_len;
	wtk_complex_t **fftr, *fftr2;
	int wins=gainnet_bf7->cfg->wins;
	float **analysis_mem_tr;
	float *rfft_in_tr;
	int pos;
	//   short *pv;
	//   wtk_strbuf_t *outbuf=wtk_strbuf_new(1024,2);
	float *rfft_in=gainnet_bf7->rfft_in;
	wtk_complex_t **fft=gainnet_bf7->fft;
	wtk_complex_t *fftx=gainnet_bf7->fftx;
	float **analysis_mem=gainnet_bf7->analysis_mem;
	float ff,ff2;
	wtk_complex_t ***ovec=gainnet_bf7->ovec,*ovec2;
	float ta,tb;

	xn=wtk_float_new_p2(channel,fsize);
	xr=wtk_float_new_p2(channel,fsize);

	feat=wtk_malloc(sizeof(float)*features_len);
	g=wtk_malloc(sizeof(float)*(nbin-7));

	fftr=wtk_complex_new_p2(channel, nbin);
	fftr2=wtk_malloc(sizeof(wtk_complex_t)*(nbin));

	analysis_mem_tr=wtk_float_new_p2(channel,fsize);

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

		for(j=0; j<channel; ++j)
		{
			memmove(rfft_in_tr, analysis_mem_tr[j], fsize*sizeof(float));
			for(i=0;i<fsize;++i)
			{
				rfft_in_tr[i+fsize]=xr[j][i];
			}
			memcpy(analysis_mem_tr[j], xr[j], fsize*sizeof(float));
			for (i=0;i<wins;++i)
			{
				rfft_in_tr[i] *= gainnet_bf7->window[i];
				// rfft_in_tr2[wins - 1 - i] *= gainnet_bf7->window[i];
			}
			forward_transform(gainnet_bf7, fftr[j], rfft_in_tr);
		}
		for(i=0; i<channel; ++i)
		{
			frame_analysis(gainnet_bf7, rfft_in, analysis_mem[i], fft[i], xn[i]);
		}
		wtk_gainnet_bf7_feed_bf_tr(gainnet_bf7, fft, feat);
        for(k=6, j=0; k<nbin-1; ++k, ++j)
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

			ff=fftx[k].a*fftx[k].a+fftx[k].b*fftx[k].b;
			ff2=0;
			if(ff>=1e-4)
			{
				ff2 = sqrtf((fftr2[k].a*fftr2[k].a+fftr2[k].b*fftr2[k].b)/ff);
				ff2 = min(ff2,1.0);
			}
			g[j]=ff2;
        }
		// printf("%f\n",wtk_float_abs_max(g,(nbin-7)*channel*2));
		// print_float2(g, (nbin-7)*channel*2);
		// {
		// 	// wtk_complex_t tmp;
		// 	for (i=6,j=0;i<nbin-1;++i,++j)
		// 	{
		// 		// tmp=fft[0][i];
		// 		fftx[i].a*=g[j];//tmp.a * g[(i-1)*channel*2] - tmp.b * g[(i-1)*channel*2+1];
		// 		fftx[i].b*=g[j];//tmp.b * g[(i-1)*channel*2] + tmp.a * g[(i-1)*channel*2+1];
		// 	}
		// 	frame_synthesis(gainnet_bf7, gainnet_bf7->out,fftx);
		// 	pv=(short *)gainnet_bf7->out;
		// 	for(i=0;i<fsize;++i)
		// 	{
		// 		pv[i]=gainnet_bf7->out[i];
		// 	}
		// 	wtk_strbuf_push(outbuf, (char *)pv, sizeof(short)*fsize);
		// }

		if(gainnet_bf7->notify_tr)
		{
			gainnet_bf7->notify_tr(gainnet_bf7->ths_tr, feat, features_len,  g, (nbin-7));
		}
		pos+=fsize;
	}
	// {
	// 	wtk_wavfile_t *wav;
	// 	wav=wtk_wavfile_new(gainnet_bf7->cfg->rate);
	// 	wav->max_pend=0;
	// 	wtk_wavfile_open(wav,"o.wav");
	// 	wtk_wavfile_write(wav,(char *)outbuf->data,outbuf->pos);
	// 	wtk_wavfile_delete(wav);
	// }

	wtk_float_delete_p2(xn,channel);
	// wtk_float_delete_p2(x,channel);
	wtk_float_delete_p2(xr,channel);

	wtk_free(g);

	wtk_complex_delete_p2(fftr, channel);
	wtk_free(fftr2);

	wtk_float_delete_p2(analysis_mem_tr,channel);

	wtk_free(rfft_in_tr);

	wtk_free(feat);
}