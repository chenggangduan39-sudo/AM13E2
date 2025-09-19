#include "wtk_kparm.h"

void wtk_kparm_on_cmvn(wtk_kparm_t *parm,wtk_kfeat_t *feat);
void wtk_kparm_on_kcmn(wtk_kparm_t *parm,wtk_kfeat_t *feat);
void wtk_kparm_on_kcmn2(wtk_kparm_t *parm,wtk_kfeat_t *feat);
void wtk_kparm_on_ivector(wtk_kparm_t *parm,float *feat,int dim);
int wtk_kparm_first_sample_off_frame(wtk_kparm_t *parm);

void wtk_kparm_fix_fft(wtk_kparm_t *parm,wtk_fix_t *x)
{
	int i,j,k;
	int n=parm->win;
	int m=parm->fft_order;
    int n1, n2, n4,n5;
	wtk_fix_t xt;
	wtk_fix_t cc, ss, t1, t2;
    int i1, i2, i3, i4;
    wtk_fix_t *ccc=parm->ccc;
    wtk_fix_t *sss=parm->sss;

    /* Bit-reverse the input. */
    j = 0;
    for (i = 0; i < n - 1; ++i)
    {
        if (i < j)
        {
            xt = x[j];
            x[j] = x[i];
            x[i] = xt;
        }
        k = n / 2;
        while (k <= j)
        {
            j -= k;
            k /= 2;
        }
        j += k;
    }
    /* Basic butterflies (2-point FFT, real twiddle factors):
     * x[i]   = x[i] +  1 * x[i+1]
     * x[i+1] = x[i] + -1 * x[i+1]
     */
    for (i = 0; i < n; i += 2)
    {
        xt = x[i];
        x[i] = (xt + x[i + 1]);
        x[i + 1] = (xt - x[i + 1]);
    }

    /* The rest of the butterflies, in stages from 1..m */
    for (k = 1; k < m; ++k)
    {
        n4 = k - 1;
        n2 = k;
        n1 = k + 1;
        /* Stride over each (1 << (k+1)) points */
        for (i = 0; i < n; i += (1 << n1))
        {
            /* Basic butterfly with real twiddle factors:
             * x[i]          = x[i] +  1 * x[i + (1<<k)]
             * x[i + (1<<k)] = x[i] + -1 * x[i + (1<<k)]
             */
            xt = x[i];
            x[i] = (xt + x[i + (1 << n2)]);
            x[i + (1 << n2)] = (xt - x[i + (1 << n2)]);

            /* The other ones with real twiddle factors:
             * x[i + (1<<k) + (1<<(k-1))]
             *   = 0 * x[i + (1<<k-1)] + -1 * x[i + (1<<k) + (1<<k-1)]
             * x[i + (1<<(k-1))]
             *   = 1 * x[i + (1<<k-1)] +  0 * x[i + (1<<k) + (1<<k-1)]
             */
            x[i + (1 << n2) + (1 << n4)] = -x[i + (1 << n2) + (1 << n4)];
            //x[i + (1 << n4)] = x[i + (1 << n4)];

            n5=1<<n4;
            /* Butterflies with complex twiddle factors.
             * There are (1<<k-1) of them.
             */
            for (j = 1; j < n5; ++j)
            {
                i1 = i + j;
                i2 = i + (1 << n2) - j;
                i3 = i + (1 << n2) + j;
                i4 = i + (1 << n2) + (1 << n2) - j;

                /*
                 * cc = real(W[j * n / (1<<(k+1))])
                 * ss = imag(W[j * n / (1<<(k+1))])
                 */
                cc = ccc[j << (m - n1)];
                ss = sss[j << (m - n1)];

                /* There are some symmetry properties which allow us
                 * to get away with only four multiplications here. */
                t1 = COSMUL(x[i3], cc) + COSMUL(x[i4], ss);
                t2 = COSMUL(x[i3], ss) - COSMUL(x[i4], cc);

                x[i4] = (x[i2] - t2);
                x[i3] = (-x[i2] - t2);
                x[i2] = (x[i1] - t1);
                x[i1] = (x[i1] + t1);
            }
        }
    }
    //print_int(x,n);
}



float wtk_mel_scale(float fs)
{
	return 1127.0f * logf (1.0f + fs / 700.0f);
}

void wtk_fix_melbank_delete(wtk_fix_melbank_t *bank)
{
	int i;

	for(i=0;i<bank->cfg->num_bins;++i)
	{
		wtk_free(bank->bins[i].w);
	}
	wtk_free(bank->mel_energy);
	wtk_free(bank->bins);
	wtk_free(bank);
}

int wtk_fix_melbank_bytes(wtk_fix_melbank_t *bank,wtk_kparm_cfg_t *parm,int len)
{
	int bytes;
	float low_freq,high_freq,fft_bin_width;
	float mel_low_freq,mel_high_freq,mel_freq_delta;
	float left,right;
	int i,j;
	int nbin=len/2+1;
	float freq,mel;
	int first_index,last_index;
	wtk_melbank_cfg_t *cfg=bank->cfg;

	bytes=sizeof(wtk_fix_melbank_t);
	bytes+=bank->cfg->num_bins*sizeof(wtk_fix_t);
	bytes+=bank->cfg->num_bins*sizeof(wtk_fix_melbin_t);

	low_freq=bank->cfg->low_freq;
	if(cfg->high_freq>0)
	{
		high_freq=cfg->high_freq;
	}else
	{
		high_freq=0.5*parm->rate+cfg->high_freq;
	}
	fft_bin_width=parm->rate*1.0/len;
	//wtk_debug("width=%f [%f,%f]\n",fft_bin_width,low_freq,high_freq);
	mel_low_freq=wtk_mel_scale(low_freq);
	mel_high_freq=wtk_mel_scale(high_freq);
	mel_freq_delta=(mel_high_freq-mel_low_freq)/(cfg->num_bins+1);
	for(i=0;i<cfg->num_bins;++i)
	{
		left=mel_low_freq+i*mel_freq_delta;
		right=mel_low_freq+(i+2)*mel_freq_delta;
		//wtk_debug("v[%d]=%f/%f/%f\n",i,left,center,right);
		first_index=last_index=-1;
		for(j=0,freq=0;j<nbin;++j,freq+=fft_bin_width)
		{
			mel=wtk_mel_scale(freq);
			if(mel>left && mel<right)
			{
				if(first_index==-1)
				{
					first_index=j;
				}
				last_index=j;
			}else if(mel>=right)
			{
				break;
			}
		}
		bytes+=(last_index-first_index+1)*sizeof(wtk_fix_t);
	}
	return bytes;
}

wtk_fix_melbank_t* wtk_fix_melbank_new(wtk_melbank_cfg_t *cfg,wtk_kparm_cfg_t *parm,int len)
{
	wtk_fix_melbank_t *bank;
	float low_freq,high_freq,fft_bin_width;
	float mel_low_freq,mel_high_freq,mel_freq_delta;
	float left,center,right;
	int i,j,k;
	int nbin=len/2+1;
	float freq,mel,weight;
	int first_index,last_index;

	bank=(wtk_fix_melbank_t*)wtk_malloc(sizeof(wtk_fix_melbank_t));
	bank->cfg=cfg;
	low_freq=cfg->low_freq;
	if(cfg->high_freq>0)
	{
		high_freq=cfg->high_freq;
	}else
	{
		high_freq=0.5*parm->rate+cfg->high_freq;
	}
	bank->mel_energy=(wtk_fix_t*)wtk_calloc(cfg->num_bins,sizeof(wtk_fix_t));
	fft_bin_width=parm->rate*1.0/len;
	//wtk_debug("width=%f [%f,%f]\n",fft_bin_width,low_freq,high_freq);
	mel_low_freq=wtk_mel_scale(low_freq);
	mel_high_freq=wtk_mel_scale(high_freq);
	mel_freq_delta=(mel_high_freq-mel_low_freq)/(cfg->num_bins+1);
	//wtk_debug("delta=%f %f/%f bin=%f\n",mel_freq_delta,mel_high_freq,mel_low_freq,fft_bin_width);
	bank->bins=(wtk_fix_melbin_t*)wtk_calloc(cfg->num_bins,sizeof(wtk_fix_melbin_t));
	for(i=0;i<cfg->num_bins;++i)
	{
		left=mel_low_freq+i*mel_freq_delta;
		center=mel_low_freq+(i+1)*mel_freq_delta;
		right=mel_low_freq+(i+2)*mel_freq_delta;
		//wtk_debug("v[%d]=%f/%f/%f\n",i,left,center,right);
		first_index=last_index=-1;
		for(j=0,freq=0;j<nbin;++j,freq+=fft_bin_width)
		{
			mel=wtk_mel_scale(freq);
			if(mel>left && mel<right)
			{
				if(first_index==-1)
				{
					first_index=j;
				}
				last_index=j;
			}else if(mel>=right)
			{
				break;
			}
		}
		bank->bins[i].s=first_index;
		bank->bins[i].e=last_index;
		bank->bins[i].w=(wtk_fix_t*)wtk_calloc(last_index-first_index+1,sizeof(wtk_fix_t));
		//wtk_debug("v[%d]=%d/%d\n",i,first_index,last_index);
		for(j=first_index,freq=first_index*fft_bin_width,k=0;j<=last_index;++j,++k,freq+=fft_bin_width)
		{
			mel=wtk_mel_scale(freq);
			if(mel<=center)
			{
				weight=(mel-left)/(center-left);
			}else
			{
				weight=(right-mel)/(right-center);
			}
			bank->bins[i].w[k]=FLOAT2FIX(log(weight));
			//wtk_debug("v[%d]=%f w=%f/%d/%f\n",j,freq,weight,bank->bins[i].w[k],FIX2FLOAT(bank->bins[i].w[k]));
		}
		if(i==0 && cfg->htk_mode && mel_low_freq!=0)
		{
			bank->bins[i].w[0]=0;
		}
		//wtk_debug("v[%d]=[%d,%d]\n",i,first_index,last_index);
	}
	return bank;
}

int wtk_melbank_bytes(wtk_melbank_t *bank,wtk_kparm_cfg_t *parm,int len)
{
	int bytes;
	wtk_melbank_cfg_t *cfg=bank->cfg;
	float low_freq,high_freq,fft_bin_width;
	float mel_low_freq,mel_high_freq,mel_freq_delta;
	float left,right;
	int i,j;
	int nbin=len/2+1;
	float freq,mel;
	int first_index,last_index;

	bytes=sizeof(wtk_melbank_t);
	bytes+=cfg->num_bins*sizeof(float);
	bytes+=cfg->num_bins*sizeof(wtk_melbin_t);
	low_freq=cfg->low_freq;
	if(cfg->high_freq>0)
	{
		high_freq=cfg->high_freq;
	}else
	{
		high_freq=0.5*parm->rate+cfg->high_freq;
	}
	fft_bin_width=parm->rate*1.0/len;
	//wtk_debug("width=%f [%f,%f]\n",fft_bin_width,low_freq,high_freq);
	mel_low_freq=wtk_mel_scale(low_freq);
	mel_high_freq=wtk_mel_scale(high_freq);
	mel_freq_delta=(mel_high_freq-mel_low_freq)/(cfg->num_bins+1);
	for(i=0;i<cfg->num_bins;++i)
	{
		left=mel_low_freq+i*mel_freq_delta;
		right=mel_low_freq+(i+2)*mel_freq_delta;
		//wtk_debug("v[%d]=%f/%f/%f\n",i,left,center,right);
		first_index=last_index=-1;
		for(j=0,freq=0;j<nbin;++j,freq+=fft_bin_width)
		{
			mel=wtk_mel_scale(freq);
			if(mel>left && mel<right)
			{
				if(first_index==-1)
				{
					first_index=j;
				}
				last_index=j;
			}else if(mel>=right)
			{
				break;
			}
		}
		bytes+=(last_index-first_index+1)*sizeof(float);
	}
	//exit(0);
	return bytes;
}

wtk_hmelbin_t* wtk_hmelbin_new(int win)
{
	wtk_hmelbin_t *bin;

	bin=(wtk_hmelbin_t*)wtk_malloc(sizeof(wtk_hmelbin_t));
	bin->klo=1;
	bin->khi=win;
	bin->loChan=(short*)wtk_calloc(win,sizeof(short));
	bin->loWt=(float*)wtk_calloc(win,sizeof(float));
	return bin;
}

void wtk_hmelbin_delete(wtk_hmelbin_t *bin)
{
	wtk_free(bin->loChan);
	wtk_free(bin->loWt);
	wtk_free(bin);
}

static float mel2hz(float mels)
{
    float f_min = 0.0;
    float f_sp = 200.0 / 3;
    float freqs = f_min + f_sp * mels;

    float min_log_hz = 1000.0;
    float min_log_mel = (min_log_hz - f_min) / f_sp;
    float logstep = log(6.4) / 27.0;


    if(mels >= min_log_mel)
    {
    	freqs = min_log_hz * exp(logstep * (mels - min_log_mel));
    }
    return freqs;
}

static float hz2mel(float v)
{
	float f_min = 0.0;
	float f_sp = 200.0/3;
	float mels = (v - f_min)/f_sp;
	float min_log_hz = 1000.0;
    float min_log_mel = (min_log_hz - f_min) / f_sp;
    float logstep = log(6.4) / 27.0;

    if(v > 1000)//TODO
    {
    	mels = min_log_mel + log(v / min_log_hz) / logstep;
    }
    //wtk_debug("%f\n",mels);
    return mels;
}

wtk_melbank_t* wtk_melbank_new(wtk_melbank_cfg_t *cfg,wtk_kparm_cfg_t *parm,int len)
{
	wtk_melbank_t *bank;
	float low_freq,high_freq,fft_bin_width;
	float mel_low_freq,mel_high_freq,mel_freq_delta;
	float left,center,right;
	int i,j,k;
	int nbin=len/2+1;
	float freq,mel,weight;
        int first_index = -1, last_index = -1;

        bank = (wtk_melbank_t *)wtk_malloc(sizeof(wtk_melbank_t));
        bank->cfg=cfg;
	bank->mel_energy=(float*)wtk_calloc(cfg->num_bins,sizeof(float));
	bank->mel_energy_double = (double*)wtk_calloc(cfg->num_bins,sizeof(double));
	bank->bins=NULL;
	bank->hbin=NULL;
	if(cfg->use_htk)
	{
		float mlo,mhi;
		wtk_hmelbin_t *hbin;
		int max_chan,chan;
		float *cf,ms,melk;
		int win=nbin-1;

		hbin=bank->hbin=wtk_hmelbin_new(win);
		mlo=0;
		mhi=wtk_mel_scale(parm->rate/2);
		if(cfg->low_freq>0)
		{
			mlo=wtk_mel_scale(cfg->low_freq);
			hbin->klo=cfg->low_freq*win*2/parm->rate+1.5;
			if(hbin->klo<1)
			{
				hbin->klo=1;
			}
		}
		if(cfg->high_freq>0)
		{
			mhi=wtk_mel_scale(cfg->high_freq);
			hbin->khi=cfg->high_freq*win*2/parm->rate-0.5;
			if(hbin->khi>win)
			{
				hbin->khi=win;
			}
		}
		//wtk_debug("[%f,%f] [%d,%d]\n",mlo,mhi,hbin->klo,hbin->khi);
		max_chan=cfg->num_bins+1;
		cf=(float*)wtk_calloc(max_chan,sizeof(float));
		ms=mhi-mlo;
		for(i=0;i<max_chan;++i)
		{
			cf[i]=((float)(i+1)/(float)max_chan)*ms+mlo;
		}
		for(i=0,chan=0;i<win;++i)
		{
			if(i<hbin->klo || i>hbin->khi)
			{
				hbin->loChan[i]=-1;
			}else
			{
				melk=wtk_mel_scale(i*parm->rate/len);
				while(cf[chan]<melk && chan<max_chan)
				{
					++chan;
				}
				hbin->loChan[i]=chan;
			}
			//wtk_debug("v[%d]=%d\n",i,hbin->loChan[i]);
		}
		for(i=0;i<win;++i)
		{
			chan=hbin->loChan[i];
			if(i<hbin->klo || i>hbin->khi)
			{
				hbin->loWt[i]=0;
			}else
			{
				if(chan>0)
				{
					hbin->loWt[i]=(cf[chan]-wtk_mel_scale(i*parm->rate*1.0/len))/(cf[chan]-cf[chan-1]);
					//wtk_debug("%f/%f/%f %f\n",cf[chan],cf[chan-1],wtk_mel_scale(i*parm->rate*1.0/len),hbin->loWt[i]);
					//exit(0);
				}else
				{
					hbin->loWt[i]=(cf[0]-wtk_mel_scale((i)*parm->rate*1.0/len))/(cf[0]-mlo);
					//wtk_debug("v[%d]: %f/%f %f\n",i,cf[0],mlo,hbin->loWt[i]);
					//exit(0);
				}
			}
			//wtk_debug("v[%d]=%f\n",i,hbin->loWt[i]);
		}
		wtk_free(cf);
		//exit(0);
	}else if(cfg->use_torch)
	{
		float *a,*b,*c,d;

		bank->bins=(wtk_melbin_t*)wtk_calloc(cfg->num_bins,sizeof(wtk_melbin_t));
		high_freq=cfg->high_freq;
		low_freq=cfg->low_freq;

		//wtk_debug("%d %d\n",cfg->num_bins+2,len+1);
		b=(float*)wtk_calloc(cfg->num_bins+2,sizeof(float));
		c=(float*)wtk_calloc(cfg->num_bins+2,sizeof(float));
		a=(float*)wtk_calloc(len+1,sizeof(float));

		float interval = (parm->rate/2.0)/(len);
		a[0] = 0;//fftfreqs
		a[len] = parm->rate/2;
		for(i = 1;i < len; i++)
		{
			a[i] = a[i-1] + interval;
		}
		//print_float(a,len+1);

		float s = hz2mel(low_freq);
		float e = hz2mel(high_freq);
		//wtk_debug("%f %f %f %f\n",low_freq,s,high_freq,e);
		b[0] = s;//mels
		b[cfg->num_bins+1] = e;
		interval = 1.0*(e-s)/(cfg->num_bins+1);
		for(i = 1;i < cfg->num_bins+1; i++)
		{
			b[i] = b[i-1] + interval;
			//wtk_debug("%f\n",b[i-1]);
			b[i-1] = mel2hz(b[i-1]);
			//wtk_debug("%f\n",b[i-1]);
		}
		b[cfg->num_bins] = mel2hz(b[cfg->num_bins]);
		b[cfg->num_bins+1] = high_freq;//mel2hz(b[cfg->num_bins+1]);
		//print_float(b,cfg->num_bins+2);

		for(i=cfg->num_bins+1;i>0;--i)
		{
			//b[i]-=b[i-1];
			c[i] = b[i] - b[i-1];
		}
		c[0] = 0.0;
		//print_float(a,len+1);
		//print_float(b,cfg->num_bins+2);
		//print_float(c,cfg->num_bins+2);

		qtk_blas_matrix_t *m = qtk_blas_matrix_new(cfg->num_bins+2,len+1);
		for(i=0;i<m->row;i++)
		{
			for(j=0;j<m->col;j++)
			{
				m->m[i*m->col+j] = b[i] - a[j];
			}
		}

		float *lower = (float*)wtk_calloc(len+1,sizeof(float));
		float *upper = (float*)wtk_calloc(len+1,sizeof(float));
		int st=0;
		//print_float(c,42);
		for(i=0;i<cfg->num_bins;++i)
		{
			d = 2.0/(b[2+i] - b[i]);
			for(j=0;j<len+1;j++)
			{
				lower[j] = -(m->m[i*m->col+j])/c[i+1];
				upper[j] = (m->m[(i+2)*m->col+j])/c[i+2];
				if(lower[j] > 0 && upper[j] > 0)
				{
					if(!st)
					{
						first_index = j;
						st = 1;
					}else
					{
						last_index = j;
					}

					if(lower[j] > upper[j])
					{
						lower[j] = upper[j]*d;
					}else
					{
						lower[j] = lower[j]*d;
					}
				}
			}
			st = 0;
			//wtk_debug("%d %d\n",first_index,last_index);
			bank->bins[i].s=first_index;
			bank->bins[i].e=last_index;
			bank->bins[i].w=(float*)wtk_calloc(last_index-first_index+1,sizeof(float));
			memcpy(bank->bins[i].w,lower+first_index,sizeof(float)*(last_index-first_index+1));
			//print_float(bank->bins[i].w,last_index-first_index+1);
			//print_float(lower,513);
			//print_float(upper,513);
			//exit(0);
		}
		wtk_free(a);
		wtk_free(b);
		wtk_free(c);
		wtk_free(lower);
		wtk_free(upper);
		qtk_blas_matrix_delete(m);
		//qtk_blas_matrix_print(m);
		//exit(0);

	}else
	{
		bank->bins=(wtk_melbin_t*)wtk_calloc(cfg->num_bins,sizeof(wtk_melbin_t));
		low_freq=cfg->low_freq;
		if(cfg->high_freq>0)
		{
			high_freq=cfg->high_freq;
		}else
		{
			high_freq=0.5*parm->rate+cfg->high_freq;
		}
		fft_bin_width=parm->rate*1.0/len;
		//wtk_debug("width=%f [%f,%f]\n",fft_bin_width,low_freq,high_freq);
		mel_low_freq=wtk_mel_scale(low_freq);
		mel_high_freq=wtk_mel_scale(high_freq);
		mel_freq_delta=(mel_high_freq-mel_low_freq)/(cfg->num_bins+1);
		//wtk_debug("delta=%f %f/%f bin=%f\n",mel_freq_delta,mel_high_freq,mel_low_freq,fft_bin_width);
		for(i=0;i<cfg->num_bins;++i)
		{
			left=mel_low_freq+i*mel_freq_delta;
			center=mel_low_freq+(i+1)*mel_freq_delta;
			right=mel_low_freq+(i+2)*mel_freq_delta;
			//wtk_debug("v[%d]=%f/%f/%f\n",i,left,center,right);
			first_index=last_index=-1;
			for(j=0,freq=0;j<nbin;++j,freq+=fft_bin_width)
			{
				mel=wtk_mel_scale(freq);
				if(mel>left && mel<right)
				{
					if(first_index==-1)
					{
						first_index=j;
					}
					last_index=j;
				}else if(mel>=right)
				{
					break;
				}
			}
			bank->bins[i].s=first_index;
			bank->bins[i].e=last_index;
			bank->bins[i].w=(float*)wtk_calloc(last_index-first_index+1,sizeof(float));
			//wtk_debug("v[%d]=%d/%d\n",i,first_index,last_index);
			for(j=first_index,freq=first_index*fft_bin_width,k=0;j<=last_index;++j,++k,freq+=fft_bin_width)
			{
				mel=wtk_mel_scale(freq);
				if(mel<=center)
				{
					weight=(mel-left)/(center-left);
				}else
				{
					weight=(right-mel)/(right-center);
				}
				bank->bins[i].w[k]=weight;
				//wtk_debug("v[%d]=%f w=%f\n",j,freq,weight);
			}
			if(i==0 && cfg->htk_mode && mel_low_freq!=0)
			{
				bank->bins[i].w[0]=0;
			}
			//wtk_debug("v[%d]=[%d,%d]\n",i,first_index,last_index);
		}
	}
	//exit(0);
	return bank;
}

void wtk_melbank_delete(wtk_melbank_t *bank)
{
	int i;

	if(bank->bins)
	{
		for(i=0;i<bank->cfg->num_bins;++i)
		{
			wtk_free(bank->bins[i].w);
		}
		wtk_free(bank->bins);
	}
	if(bank->hbin)
	{
		wtk_hmelbin_delete(bank->hbin);
	}
	wtk_free(bank->mel_energy);
	wtk_free(bank->mel_energy_double);
	wtk_free(bank);
}


void wtk_fix_mfcc_post_delete(wtk_fix_mfcc_post_t *mfcc)
{
	wtk_mati_delete(mfcc->dct);
	if(mfcc->lifter_coeffs)
	{
		wtk_free(mfcc->lifter_coeffs);
	}
	wtk_free(mfcc);
}

int wtk_fix_mfcc_post_bytes(wtk_fix_mfcc_post_t *post,wtk_kparm_cfg_t *cfg)
{
	int bytes;

	bytes=sizeof(wtk_fix_mfcc_post_t);
	bytes+=wtk_mati_bytes(post->dct);
	if(post->lifter_coeffs)
	{
		bytes+=cfg->melbank.num_bins*sizeof(wtk_fix_t);
	}
	return bytes;
}

wtk_fix_mfcc_post_t* wtk_fix_mfcc_post_new(wtk_kparm_cfg_t *cfg)
{
	wtk_fix_mfcc_post_t *mfcc;
	int i,j;
	wtk_mati_t *m;
	float mfnorm,pi_factor;
	int nchan;
	float x,f;
	int *pf;

	mfcc=(wtk_fix_mfcc_post_t*)wtk_malloc(sizeof(wtk_fix_mfcc_post_t));
	mfcc->dct=m=wtk_mati_new(cfg->NUMCEPS,cfg->melbank.num_bins);
	//wtk_debug("[%d/%d]\n",cfg->NUMCEPS,cfg->melbank.num_bins);
	nchan=cfg->melbank.num_bins;
	mfnorm = sqrt(1.0/(float)nchan);
	pi_factor = PI/(float)nchan;
	pf=mfcc->dct->p;
	for(j=0;j<m->col;++j)
	{
		*(pf++)=FLOAT2FIX(mfnorm);
		//wtk_debug("v[%d/%d]=%d/%f/%d\n",0,j,pf[-1],mfnorm,FLOAT2FIX(mfnorm));
	}
	mfnorm = sqrt(2.0/(float)nchan);
	for(i=1;i<m->row;++i)
	{
		x=i*pi_factor;
		for(j=0;j<m->col;++j)
		{
			f=cos(x*(j+0.5))*mfnorm;
			*(pf++)=FLOAT2FIX(f);
			//wtk_debug("v[%d/%d]=%d %f/%f\n",i,j,pf[-1],FIX2FLOAT(pf[-1]),f);
			//exit(0);
		}
	}
	//exit(0);
	if(cfg->CEPLIFTER!=0.0)
	{
		mfcc->lifter_coeffs=(wtk_fix_t*)wtk_calloc(nchan,sizeof(wtk_fix_t));
		for(i=0;i<cfg->NUMCEPS;++i)
		{
			f=1.0+0.5*cfg->CEPLIFTER*sin(M_PI*i/cfg->CEPLIFTER);
			mfcc->lifter_coeffs[i]=FLOAT2FIX(f);
			//wtk_debug("v[%d]=%f\n",i,mfcc->lifter_coeffs[i]);
		}
		//exit(0);
	}else
	{
		mfcc->lifter_coeffs=NULL;
	}
	//exit(0);
	return mfcc;
}


int wtk_mfcc_post_bytes(wtk_mfcc_post_t *post,wtk_kparm_cfg_t *cfg)
{
	int bytes;

	bytes=sizeof(wtk_mfcc_post_t);
	bytes+=wtk_matf_bytes(post->dct);
	if(post->lifter_coeffs)
	{
		bytes+=cfg->melbank.num_bins*sizeof(float);
	}
	return bytes;
}


wtk_mfcc_post_t* wtk_mfcc_post_new(wtk_kparm_cfg_t *cfg)
{
	wtk_mfcc_post_t *mfcc;
	int i,j;
	wtk_matf_t *m;
	float mfnorm,pi_factor;
	int nchan;
	float x;
	float *pf;

	mfcc=(wtk_mfcc_post_t*)wtk_malloc(sizeof(wtk_mfcc_post_t));
	mfcc->dct=m=wtk_matf_new(cfg->NUMCEPS,cfg->melbank.num_bins);
	//wtk_debug("[%d/%d]\n",cfg->NUMCEPS,cfg->melbank.num_bins);
	nchan=cfg->melbank.num_bins;
	mfnorm = sqrt(1.0/(float)nchan);
	pi_factor = PI/(float)nchan;
	pf=mfcc->dct->p;
	for(j=0;j<m->col;++j)
	{
		*(pf++)=mfnorm;
		//wtk_debug("v[%d/%d]=%f\n",0,j,pf[-1]);
	}
	mfnorm = sqrt(2.0/(float)nchan);
	for(i=1;i<m->row;++i)
	{
		x=i*pi_factor;
		for(j=0;j<m->col;++j)
		{
			*(pf++)=cos(x*(j+0.5))*mfnorm;
			//wtk_debug("v[%d/%d]=%f\n",i,j,pf[-1]);
			//exit(0);
		}
	}
	if(cfg->CEPLIFTER!=0.0)
	{
		mfcc->lifter_coeffs=(float*)wtk_calloc(nchan,sizeof(float));
		for(i=0;i<cfg->NUMCEPS;++i)
		{
			mfcc->lifter_coeffs[i]=1.0+0.5*cfg->CEPLIFTER*sin(M_PI*i/cfg->CEPLIFTER);
			//wtk_debug("v[%d]=%f\n",i,mfcc->lifter_coeffs[i]);
		}
		//exit(0);
	}else
	{
		mfcc->lifter_coeffs=NULL;
	}
	//exit(0);
	return mfcc;
}

void wtk_mfcc_post_delete(wtk_mfcc_post_t *mfcc)
{
	if(mfcc->lifter_coeffs)
	{
		wtk_free(mfcc->lifter_coeffs);
	}
	wtk_matf_delete(mfcc->dct);
	wtk_free(mfcc);
}


wtk_kfeat_t* wtk_kparm_new_feat(wtk_kparm_t *parm)
{
	//wtk_debug("vec2=%d\n",parm->cfg->vec_size3);
	return wtk_kfeat_new(parm->cfg->vec_size2);
}

void wtk_kparm_push_feat(wtk_kparm_t *p,wtk_kfeat_t *f)
{
	//wtk_debug("push feat[%d] used=%d\n",f->index,f->used);
	if(f->used==0)
	{
		//wtk_debug("push feat[%d] used=%d\n",f->index,f->used);
		wtk_hoard_push(&(p->hoard),f);
	}
}

wtk_kfeat_t* wtk_kparm_pop_feat(wtk_kparm_t *p)
{
	wtk_kfeat_t* f;

	f=(wtk_kfeat_t*)wtk_hoard_pop(&(p->hoard));
	f->index=p->nframe++;
	f->used=1;
	//wtk_debug("pop feat=%d\n",f->index);
	return f;
}



void wtk_kparm_create_twiddle(wtk_kparm_t *parm)
{
# define M_PI		3.14159265358979323846	/* pi */
	int i,n;
	double a;

	n=parm->win/4;
	parm->ccc=(wtk_fix_t*)wtk_calloc(n,sizeof(wtk_fix_t));
	parm->sss=(wtk_fix_t*)wtk_calloc(n,sizeof(wtk_fix_t));
	for(i=0;i<n;++i)
	{
		a = 2 * M_PI * i/parm->win;
        parm->ccc[i] = FLOAT2COS(cos(a));
        parm->sss[i] = FLOAT2COS(sin(a));
	}
}

int wtk_kparm_bytes(wtk_kparm_t *parm)
{
	int bytes;

	bytes=sizeof(wtk_kparm_t);
	if(parm->cfg->use_fixpoint)
	{
		bytes+=sizeof(wtk_fix_t)*parm->win*2;
		bytes+=sizeof(wtk_fix_t)*(parm->win/2+1)*2;
		bytes+=(parm->win/4)*sizeof(wtk_fix_t)*2;
		bytes+=wtk_fix_melbank_bytes(parm->fix_melbank,parm->cfg,parm->win);
	}else
	{
		bytes+=wtk_rfft_bytes(parm->rfft);
		bytes+=parm->rfft->len*sizeof(float)*3;
		bytes+=wtk_melbank_bytes(parm->melbank,parm->cfg,parm->rfft->len);
	}
	bytes+=(parm->hoard.cur_free+parm->hoard.use_length)*wtk_kfeat_bytes(parm->cfg->vec_size2);
	if(parm->cmvn)
	{
		bytes+=wtk_cmvn_bytes(parm->cmvn);
	}
	if(parm->pcen)
	{
		bytes+=wtk_pcen_bytes(parm->pcen);
	}
	if(parm->delta)
	{
		bytes+=wtk_delta_bytes(parm->delta);
	}
	if(parm->lda)
	{
		bytes+=wtk_lda_bytes(parm->lda,wtk_kparm_cfg_feature_base_size(parm->cfg));
	}
	switch(parm->cfg->kfind.bkind)
	{
	case WTK_FBANK:
		break;
	case WTK_MFCC:
		if(parm->cfg->use_fixpoint)
		{
			bytes+=wtk_fix_mfcc_post_bytes(parm->fix_mfcc,parm->cfg);
		}else
		{
			bytes+=wtk_mfcc_post_bytes(parm->mfcc,parm->cfg);
		}
		break;
	case WTK_PLP:
		exit(0);
		break;
	}
	return bytes;
}

wtk_kparm_t* wtk_kparm_new(wtk_kparm_cfg_t *cfg)
{
	wtk_kparm_t *parm;
	int v,win,st;

	parm=(wtk_kparm_t*)wtk_malloc(sizeof(wtk_kparm_t));
	parm->cfg=cfg;
	parm->input=NULL;
	parm->input2=NULL;
	parm->input3=NULL;
	parm->rfft=NULL;
	parm->melbank=NULL;
	parm->fix_melbank=NULL;
	parm->ccc=NULL;
	parm->sss=NULL;
	parm->spec=NULL;
	parm->mfspec=NULL;
	parm->idle_hint=0;
	parm->win_torch=NULL;
	parm->feed_idx = 0;
	if(cfg->use_fixpoint)
	{
		parm->fft_order=(int)ceil(log(cfg->frame_size)/log(2.0));
		win=pow(2.0,parm->fft_order);
		parm->win=win;
		parm->fix_input=(wtk_fix_t*)wtk_calloc(win,sizeof(wtk_fix_t));
		parm->fix_input2=(wtk_fix_t*)wtk_calloc(win,sizeof(wtk_fix_t));
		parm->spec=(wtk_fix_t*)wtk_calloc(parm->win/2+1,sizeof(wtk_fix_t));
		parm->mfspec=(wtk_fix_t*)wtk_calloc(parm->win/2+1,sizeof(wtk_fix_t));
		wtk_kparm_create_twiddle(parm);
		parm->fix_melbank=wtk_fix_melbank_new(&(cfg->melbank),cfg,win);
		//wtk_kparm_build_melfilers(parm);
	}else
	{
		win=cfg->frame_size/2;
		if(parm->cfg->melbank.use_torch)
		{
			win = cfg->frame_size;
			parm->rfft=wtk_rfft_new(win);
			parm->input=(float*)wtk_calloc(parm->rfft->len,sizeof(float));
			//wtk_debug("%d\n",parm->rfft->len);
			parm->input2=(float*)wtk_calloc(parm->rfft->len,sizeof(float));
			parm->input3=(float*)wtk_calloc(parm->cfg->frame_step+1,sizeof(float));
			parm->fft=(float*)wtk_calloc(parm->rfft->len,sizeof(float));
			parm->melbank=wtk_melbank_new(&(cfg->melbank),cfg,parm->rfft->len/2);
			parm->win_torch=(float*)wtk_calloc(parm->rfft->len,sizeof(float));
			st = (parm->rfft->len - parm->cfg->frame_size)/2;
			memcpy(parm->win_torch+st,parm->cfg->window,sizeof(float)*parm->cfg->frame_size);
		}else
		{
			parm->rfft=wtk_rfft_new(win);
			parm->input=(float*)wtk_calloc(parm->rfft->len,sizeof(float));
			parm->input2=(float*)wtk_calloc(parm->rfft->len,sizeof(float));
			parm->fft=(float*)wtk_calloc(parm->rfft->len,sizeof(float));
			parm->melbank=wtk_melbank_new(&(cfg->melbank),cfg,parm->rfft->len);
		}
	}
	wtk_hoard_init(&(parm->hoard),offsetof(wtk_kfeat_t,hoard_n),cfg->cache,
			(wtk_new_handler_t)wtk_kparm_new_feat,(wtk_delete_handler_t)wtk_kfeat_delete,parm);
	v=wtk_kparm_cfg_feature_base_size(cfg);
	//wtk_debug("online=%d\n",cfg->cmvn.use_online);
	if(cfg->use_cmvn)
	{
		parm->cmvn=wtk_cmvn_new(&(cfg->cmvn),v);
		wtk_cmvn_set_notify(parm->cmvn,parm,(wtk_kfeat_notify_f)wtk_kparm_on_cmvn);
	}else
	{
		parm->cmvn=NULL;
	}
	if(cfg->use_pcen)
	{
		parm->pcen=wtk_pcen_new(&(cfg->pcen),v);
	}else
	{
		parm->pcen=NULL;
	}
	if(cfg->use_kcmvn)
	{
		parm->kcmvn=wtk_kcmn_new(&(cfg->kcmvn),v);
		wtk_kcmn_set_notify(parm->kcmvn,parm,(wtk_kfeat_notify_f)wtk_kparm_on_cmvn);
	}else
	{
		parm->kcmvn=NULL;
	}
	if(cfg->cmvn_stats_fn)
	{
		parm->cur_cmvn_stats=wtk_vector_new(v+1);
	}else
	{
		parm->cur_cmvn_stats=NULL;
	}
	if(cfg->use_kcmn)
	{
		parm->kcmn=qtk_kcmn_new(&(cfg->kcmn),v);
		if(cfg->use_ivector)
		{
			qtk_kcmn_set_notify(parm->kcmn,parm,(wtk_kfeat_notify_f)wtk_kparm_on_kcmn);
			parm->ivector=qtk_ivector_new(&(cfg->ivector),parm->kcmn);
			parm->feat = (wtk_kfeat_t**)wtk_calloc(parm->ivector->cfg->right_context+1,sizeof(wtk_kfeat_t*));
			parm->cache_index=0;
			parm->ivector_dim = parm->ivector->extractor->m[0]->col;
			parm->ivector_feat = (float*)wtk_malloc(sizeof(float)*parm->ivector_dim);
			memset(parm->ivector_feat,0,sizeof(float)*parm->ivector_dim);
			parm->ivector_index=0;
		}else
		{
			qtk_kcmn_set_notify(parm->kcmn,parm,(wtk_kfeat_notify_f)wtk_kparm_on_kcmn2);
			parm->ivector=NULL;
			parm->feat=NULL;
			parm->cache_index=-1;
			parm->ivector_dim=0;
			parm->ivector_feat=NULL;
			parm->ivector_index=-2;
		}
	}else
	{
		parm->kcmn=NULL;
		parm->ivector=NULL;
		parm->feat=NULL;
		parm->cache_index=-1;
		parm->ivector_dim=0;
		parm->ivector_feat=NULL;
		parm->ivector_index=-2;
	}

	if(cfg->use_delta)
	{
		parm->delta=wtk_delta_new(&(cfg->delta),v);
		parm->delta->parm=parm;
	}else
	{
		parm->delta=NULL;
	}
	if(cfg->use_lda)
	{
		parm->lda=wtk_lda_new(&(cfg->lda),v);
		parm->lda->parm=parm;
	}else
	{
		parm->lda=NULL;
	}
	parm->mfcc=NULL;
	parm->fix_mfcc=NULL;
	switch(cfg->kfind.bkind)
	{
	case WTK_FBANK:
		break;
	case WTK_MFCC:
		if(cfg->use_fixpoint)
		{
			parm->fix_mfcc=wtk_fix_mfcc_post_new(cfg);
		}else
		{
			parm->mfcc=wtk_mfcc_post_new(cfg);
		}
		break;
	case WTK_PLP:
#ifndef USE_RTOS_OF_5215
		exit(0);
#endif
		break;
	}

	// printf("parm %p\n",parm);
	parm->cmvn_raise=NULL;
	parm->cmvn_raise_ths=NULL;

	parm->kind_raise=NULL;
	parm->kind_raise_ths=NULL;

	parm->notify=NULL;
	parm->notify_ths=NULL;
	parm->want_reset=0;
	parm->is_end=0;
	wtk_kparm_reset(parm);
	return parm;
}

void wtk_kparm_cmvn_stats_dump(wtk_kparm_t *parm)
{
	FILE *f=NULL;
	int n;

	n=wtk_kparm_cfg_feature_base_size(parm->cfg);
	f=fopen(parm->cfg->cmvn_stats_fn, "wb");

	fwrite("<MEAN> ",sizeof("<MEAN> ")-1,1,f);
	fwrite(&n,sizeof(int),1,f);
//	print_hex((char*)&n,4);
    fwrite(parm->cur_cmvn_stats+1,(n+1)*sizeof(float),1,f);
//    int i;
//    for(i=0;i<=n+1;++i)
//    {
//    	printf("aaa %f\n",parm->cur_cmvn_stats[i]);
//    }
//    exit(0);

	fclose(f);
}

void wtk_kparm_delete(wtk_kparm_t *parm)
{
	int i;
	if(parm->sss)
	{
		wtk_fix_melbank_delete(parm->fix_melbank);
		wtk_free(parm->spec);
		wtk_free(parm->mfspec);
		wtk_free(parm->ccc);
		wtk_free(parm->sss);
		wtk_free(parm->fix_input);
		wtk_free(parm->fix_input2);
		if(parm->fix_mfcc)
		{
			wtk_fix_mfcc_post_delete(parm->fix_mfcc);
		}
	}
	wtk_hoard_clean(&(parm->hoard));
	if(parm->mfcc)
	{
		wtk_mfcc_post_delete(parm->mfcc);
	}
	if(parm->cmvn)
	{
		wtk_cmvn_delete(parm->cmvn);
	}
	if(parm->pcen)
	{
		wtk_pcen_delete(parm->pcen);
	}
	if(parm->kcmvn)
	{
		wtk_kcmn_delete(parm->kcmvn);
	}
	if(parm->cur_cmvn_stats)
	{
		if(parm->cfg->cmvn_stats)
		{
			for(i=1;i<=wtk_kparm_cfg_feature_base_size(parm->cfg);++i)
			{
				parm->cur_cmvn_stats[i]+=parm->cfg->cmvn_stats[i];
			}
			parm->cur_cmvn_stats[i]+=parm->cfg->cmvn_stats[i];
		}

		wtk_kparm_cmvn_stats_dump(parm);

		wtk_vector_delete(parm->cur_cmvn_stats);
	}
	if(parm->kcmn)
	{
		qtk_kcmn_delete(parm->kcmn);
		if(parm->ivector)
		{
			qtk_ivector_delete(parm->ivector);
			wtk_free(parm->feat);
			wtk_free(parm->ivector_feat);
		}
	}
	if(parm->delta)
	{
		wtk_delta_delete(parm->delta);
	}
	if(parm->lda)
	{
		wtk_lda_delete(parm->lda);
	}
	if(parm->melbank)
	{
		wtk_melbank_delete(parm->melbank);
	}
	if(parm->rfft)
	{
		wtk_rfft_delete(parm->rfft);
		wtk_free(parm->input);
		wtk_free(parm->input2);
		if(parm->input3)
		{
			wtk_free(parm->input3);
		}
		wtk_free(parm->fft);
	}
	if(parm->win_torch)
	{
		wtk_free(parm->win_torch);
	}
	wtk_free(parm);
}

void wtk_kparm_start(wtk_kparm_t *parm)
{
	parm->start=1;
}

void wtk_kparm_reset(wtk_kparm_t *parm)
{
	parm->want_reset=1;
	parm->start = 0;
	parm->wav_bytes = 0;
	if(parm->cmvn)
	{
		if(parm->cfg->use_fixpoint)
		{
			wtk_cmvn_flush_fix(parm->cmvn);
		}else
		{
			wtk_cmvn_flush(parm->cmvn);
		}
		wtk_cmvn_reset(parm->cmvn);
	}
	if(parm->pcen)
	{
		wtk_pcen_reset(parm->pcen);
	}
	if(parm->kcmvn)
	{
		wtk_kcmn_reset(parm->kcmvn);
	}

	if(parm->kcmn)
	{
		qtk_kcmn_reset(parm->kcmn);
		if(parm->ivector)
		{
			parm->ivector_index = 0;
			qtk_ivector_reset(parm->ivector);
			memset(parm->ivector_feat,0,sizeof(float)*parm->ivector_dim);
		}
		parm->cache_index=0;
	}
	if(parm->delta)
	{
		if(parm->cfg->use_fixpoint)
		{
			wtk_delta_flush_fix(parm->delta);
		}else
		{
			wtk_delta_flush(parm->delta);
		}
		wtk_delta_reset(parm->delta);
	}
	parm->want_reset=0;
	parm->stop_flag=0;
	parm->pre_emphasis_prior=0;
	parm->pos=0;
	parm->start_pos=wtk_kparm_first_sample_off_frame(parm);
	parm->nframe=0;
	parm->is_end=0;
	parm->feed_idx = 0;
	parm->last_point = 0.0;

	memset(parm->input,0,sizeof(float)*parm->rfft->len);
	memset(parm->input2,0,sizeof(float)*parm->rfft->len);
	if(parm->input3)
	{
		memset(parm->input3,0,sizeof(float)*(parm->cfg->frame_step+1));
	}
	//wtk_debug("use=%d free=%d\n",parm->hoard.use_length,parm->hoard.cur_free);
	if(parm->hoard.use_length>0)
	{
		wtk_queue_node_t *qn,*nxt;
		wtk_kfeat_t *feat;

		//wtk_debug("use=%d free=%d\n",parm->hoard.use_length,parm->hoard.cur_free);
		for(qn=parm->hoard.use;qn;qn=nxt)
		{
			nxt=qn->prev;
			feat=data_offset2(qn,wtk_kfeat_t,hoard_n);
			feat->used=0;
			wtk_kparm_push_feat(parm,feat);
		}
		if(parm->hoard.use_length>0)
		{
			wtk_debug("use=%d free=%d\n",parm->hoard.use_length,parm->hoard.cur_free);
			exit(0);
		}
	}
}

void wtk_kparm_set_kind_raise(wtk_kparm_t *parm,void *ths,wtk_kfeat_notify_f notify)
{
	parm->kind_raise_ths=ths;
	parm->kind_raise=notify;
}

void wtk_kparm_set_cmvn_raise(wtk_kparm_t *parm,void *ths,wtk_kfeat_notify_f notify)
{
	parm->cmvn_raise_ths=ths;
	parm->cmvn_raise=notify;
}

void wtk_kparm_set_notify(wtk_kparm_t *parm,void *ths,wtk_kfeat_notify_f notify)
{
	parm->notify=notify;
	parm->notify_ths=ths;
}

//void wtk_kparm_set_ivector_notify(wtk_kparm_t *parm,void *ths,wtk_ivector_notify_f notify)
//{
//	//use same notify_ths with "wtk_kparm_set_notify" for now
//	parm->ivector_notify=ths;
//}

void wtk_kparm_add_dither(wtk_kparm_t *parm,float *data,int len)
{
	unsigned seed;
	int i;
	float dither=parm->cfg->dither;

	seed=rand()+27437;
	//seed=172696877;
	//wtk_debug("seed=%d\n",seed-27437);
	for(i=0;i<len;++i)
	{
		data[i]+=dither*sqrt(-2*log((wtk_math_rand_r(&seed)+1.0)/(RAND_MAX+2.0)))*cos(2*M_PI*(wtk_math_rand_r(&seed)+1.0)/(RAND_MAX+2.0));
		//wtk_debug("v[%d]=%f\n",i,data[i]);
	}
}

void wtk_kparm_remove_dc(wtk_kparm_t *parm,float *data,int len)
{
	float f;
	int i;

	f=data[0];
	for(i=1;i<len;++i)
	{
		f+=data[i];
	}
	f/=len;
	//wtk_debug("============= remove dc= %f\n",f);
	for(i=0;i<len;++i)
	{
		data[i]-=f;
	}
}

void wtk_kparm_preemphasize(wtk_kparm_t *parm,float *data,int len)
{
#if 0
	int i;
	float preemph_coeff=parm->cfg->preemph_coeff;

	for(i=len-1;i>0;--i)
	{
		data[i]-=preemph_coeff*data[i-1];
	}
	data[0]-=preemph_coeff*data[0];
#else
	int i;
	float preemph_coeff = parm->cfg->preemph_coeff;
	for(i = 0; i < len-1; ++i){
		data[i] = data[i+1] - preemph_coeff*data[i];
	}
#endif
}

void wtk_kparm_preemphasize2(wtk_kparm_t *parm,float *data,int len)
{
	int i;
	float preemph_coeff=parm->cfg->preemph_coeff;

	for(i=len-1;i>0;--i)
	{
		data[i]-=preemph_coeff*data[i-1];
	}
	data[0]-=preemph_coeff*data[0];
}

void wtk_xfft(float* s, int n,int invert)
{
   int ii,jj,nn,limit,m,j,inc,i;
   double wx,wr,wpr,wpi,wi,theta;
   double xre,xri,x;

   nn=n / 2; j = 1;
   for (ii=1;ii<=nn;ii++) {
      i = 2 * ii - 1;
      if (j>i) {
         xre = s[j]; xri = s[j + 1];
         s[j] = s[i];  s[j + 1] = s[i + 1];
         s[i] = xre; s[i + 1] = xri;
      }
      m = n / 2;
      while (m >= 2  && j > m) {
         j -= m; m /= 2;
      }
      j += m;
   };
   limit = 2;
   while (limit < n) {
      inc = 2 * limit; theta = WTK_TPI / limit;
      if (invert) theta = -theta;
      x = sin(0.5 * theta);
      wpr = -2.0 * x * x; wpi = sin(theta);
      wr = 1.0; wi = 0.0;
      for (ii=1; ii<=limit/2; ii++) {
         m = 2 * ii - 1;
         for (jj = 0; jj<=(n - m) / inc;jj++) {
            i = m + jj * inc;
            j = i + limit;
            xre = wr * s[j] - wi * s[j + 1];
            xri = wr * s[j + 1] + wi * s[j];
            s[j] = s[i] - xre; s[j + 1] = s[i + 1] - xri;
            s[i] = s[i] + xre; s[i + 1] = s[i + 1] + xri;
         }
         wx = wr;
         wr = wr * wpr - wi * wpi + wr;
         wi = wi * wpr + wx * wpi + wi;
      }
      limit = inc;
   }
   if (invert)
      for (i = 1;i<=n;i++)
         s[i] = s[i] / nn;

}


void wtk_float_fft(float* s,int n)
{
   int n2, i, i1, i2, i3, i4;
   double xr1, xi1, xr2, xi2, wrs, wis;
   double yr, yi, yr2, yi2, yr0, theta, x;

   wtk_xfft(s,n,0);
   n=n/ 2; n2 = n/2;
   theta = PI / n;
   //print_float(s+1,10);
   //exit(0);
   x = sin(0.5 * theta);
   yr2 = -2.0 * x * x;
   yi2 = sin(theta); yr = 1.0 + yr2; yi = yi2;
   for (i=2; i<=n2; i++) {
      i1 = i + i - 1;      i2 = i1 + 1;
      i3 = n + n + 3 - i2; i4 = i3 + 1;
      wrs = yr; wis = yi;
      xr1 = (s[i1] + s[i3])/2.0; xi1 = (s[i2] - s[i4])/2.0;
      xr2 = (s[i2] + s[i4])/2.0; xi2 = (s[i3] - s[i1])/2.0;
      s[i1] = xr1 + wrs * xr2 - wis * xi2;
      s[i2] = xi1 + wrs * xi2 + wis * xr2;
      s[i3] = xr1 - wrs * xr2 + wis * xi2;
      s[i4] = -xi1 + wrs * xi2 + wis * xr2;
      yr0 = yr;
      yr = yr * yr2 - yi  * yi2 + yr;
      yi = yi * yr2 + yr0 * yi2 + yi;
   }
   xr1 = s[1];
   s[1] = xr1 + s[2];
   s[2] = 0.0;
}

void wtk_kparm_calc_torch_mel(wtk_kparm_t *parm,float *fft)
{
	wtk_kparm_cfg_t *cfg=parm->cfg;
	int num_bins=cfg->melbank.num_bins;
	wtk_melbin_t *bin;
	int i,j,k,e;
	float energy;
	wtk_melbank_t *bank=parm->melbank;
	float *me=bank->mel_energy;
	float *w;

	//print_float(fft,512);
	//memcpy(fft,testf,sizeof(float)*512);
	for(i=0;i<num_bins;++i)
	{
		bin=bank->bins+i;
		w=bin->w;
		e=bin->e;
		energy=0;
		for(j=bin->s,k=0;j<=e;++j,++k)
		{
			//wtk_debug("v[%d/%d]=%f\n",k,j,bin->w[k]);
			energy+=(fft[j])*w[k];
			//wtk_debug("v[%d/%d]=%f/%f/%f\n",i,j,fft[j],w[k],energy);
		}
		me[i]=energy;
		//wtk_debug("energy[%d]=%f/%f\n",i,energy,log(energy));
	}

	//if(cfg->use_log_fbank2)
	{
		for(i=0;i<num_bins;++i)
		{
			if(me[i]<0.00001)
			{
				//me[i]=log(1.192093e-07);
				me[i]=0.00001;
				//wtk_debug("%f\n",me[i]);
			}
			//wtk_debug("%f\n",me[i]);

			if(cfg->melbank.use_normal)
			{
				me[i] = ((20*log10(me[i]) - 20));
				me[i] = (2 * 4) * (me[i] + 120) / 120 - 4;
			}
			else
			{
				me[i] = ((20*log10(me[i]) - 20)+100)/100;
				if(me[i]<1e-8)
				{
					//me[i]=log(1.192093e-07);
					me[i]=1e-8;
					//wtk_debug("%f\n",me[i]);
				}else if(me[i]>1.0)
				{
					me[i] = 1.0;
				}
			}

		}
	}
//max_db=100
	//print_float(me,num_bins);
	//exit(0);
}

void wtk_kparm_calc_torch_mel_d(wtk_kparm_t *parm,double *fft)
{
	wtk_kparm_cfg_t *cfg=parm->cfg;
	int num_bins=cfg->melbank.num_bins;
	wtk_melbin_t *bin;
	int i,j,k,e;
	double energy;
	wtk_melbank_t *bank=parm->melbank;
	// float *me=bank->mel_energy;
	double *me = bank->mel_energy_double;
	float *w;

	//print_float(fft,512);
	//memcpy(fft,testf,sizeof(float)*512);
	for(i=0;i<num_bins;++i)
	{
		bin=bank->bins+i;
		w=bin->w;
		e=bin->e;
		energy=0;
		for(j=bin->s,k=0;j<=e;++j,++k)
		{
			//wtk_debug("v[%d/%d]=%f\n",k,j,bin->w[k]);
			energy+=(fft[j])*w[k];
			//wtk_debug("v[%d/%d]=%f/%f/%f\n",i,j,fft[j],w[k],energy);
		}
		me[i]=energy;
		//wtk_debug("energy[%d]=%f/%f\n",i,energy,log(energy));
	}

	//if(cfg->use_log_fbank2)
	{
		for(i=0;i<num_bins;++i)
		{
			//sys charge
			// if(me[i]<0.00001)
			// {
			// 	//me[i]=log(1.192093e-07);
			// 	me[i]=0.00001;
			// 	//wtk_debug("%f\n",me[i]);
			// }
			//wtk_debug("%f\n",me[i]);			

			if(cfg->melbank.use_normal)
			{
				me[i] = ((20*log10(me[i]) - 20));
				me[i] = (2 * 4) * (me[i] + 120) / 120 - 4;
			}
			else
			{
				me[i] = ((20*log10(me[i]) - 20)+100)/100;
				if(me[i]<1e-8)
				{
					//me[i]=log(1.192093e-07);
					me[i]=1e-8;
					//wtk_debug("%f\n",me[i]);
				}else if(me[i]>1.0)
				{
					me[i] = 1.0;
				}
			}

		}
	}
//max_db=100
	//print_float(me,num_bins);
	//exit(0);
}

void wtk_kparm_calc_kaldi_mel(wtk_kparm_t *parm,float *fft)
{
	wtk_kparm_cfg_t *cfg=parm->cfg;
	int num_bins=cfg->melbank.num_bins;
	int htk_mode=cfg->melbank.htk_mode;
	wtk_melbin_t *bin;
	int i,j,k,e;
	float energy;
	wtk_melbank_t *bank=parm->melbank;
	float *me=bank->mel_energy;
	float *w;

	for(i=0;i<num_bins;++i)
	{
		bin=bank->bins+i;
		w=bin->w;
		e=bin->e;
		energy=0;
		for(j=bin->s,k=0;j<=e;++j,++k)
		{
			//wtk_debug("v[%d/%d]=%f\n",k,j,bin->w[k]);
			energy+=fft[j]*w[k];
			//wtk_debug("v[%d/%d]=%f/%f/%f\n",i,j,fft[j],w[k],energy);
		}
		if(htk_mode && energy<1.0)
		{
			energy=1.0;
		}
		me[i]=energy;
		//wtk_debug("energy[%d]=%f/%f\n",i,energy,log(energy));
	}

	if(cfg->use_log_fbank)
	{
		for(i=0;i<num_bins;++i)
		{
			if(me[i]<1.192093e-07)
			{
				//me[i]=log(1.192093e-07);
				me[i]=-15.94239;
			}else
			{
#if ((defined USE_RTOS) || (defined USE_LINUX_RTOS))
				//me[i]=wtk_fast_log(me[i]);
				me[i]=wtk_fast_log2(me[i]);
				//me[i]=log(me[i]);
#else
				me[i]=log(me[i]);
#endif
			}
			//wtk_debug("v[%d]=%f\n",i,me[i]);
		}
		//exit(0);
	}
}

void wtk_kparm_calc_htk_mel(wtk_kparm_t *parm,float *fft)
{
	wtk_melbank_t *bank=parm->melbank;
	wtk_hmelbin_t *hbin=bank->hbin;
	int k;
	float f;
	int bin;
	float *me=bank->mel_energy;
	int nbin=bank->cfg->num_bins;
	int i;

	memset(me,0,sizeof(float)*bank->cfg->num_bins);
	for(k=hbin->klo;k<=hbin->khi;++k)
	{
		f=hbin->loWt[k]*fft[k];
		bin=hbin->loChan[k];
		//wtk_debug("v[%d]=%f/%f %f bin=%d\n",k,hbin->loWt[k],fft[k],f,bin);
		if(bin>0)
		{
			me[bin-1]+=f;
		}
		if(bin<nbin)
		{
			me[bin]+=fft[k]-f;
		}
		//wtk_debug("v[%d]=%f/%f %f bin[%d]=%f\n",k,hbin->loWt[k],fft[k],f,bin,me[bin]);
	}
	//print_float(me,nbin);

	if(parm->cfg->use_log_fbank)
	{
		for(i=0;i<nbin;++i)
		{
			if(me[i]<1)
			{
				//me[i]=log(1.192093e-07);
				me[i]=0;
			}else
			{
#if ((defined USE_RTOS) || (defined USE_LINUX_RTOS))
				//me[i]=wtk_fast_log(me[i]);
				me[i]=wtk_fast_log2(me[i]);
				//me[i]=log(me[i]);
#else
				me[i]=log(me[i]);
#endif
			}
			//wtk_debug("v[%d]=%f\n",i,me[i]);
		}
		//exit(0);
	}
}


void wtk_kparm_calc_mel(wtk_kparm_t *parm,float *fft)
{
	if(parm->melbank->cfg->use_htk)
	{
		wtk_kparm_calc_htk_mel(parm,fft);
	}else if(parm->melbank->cfg->use_torch)
	{
		wtk_kparm_calc_torch_mel(parm,fft);
	}else
	{
		wtk_kparm_calc_kaldi_mel(parm,fft);
	}
}

void wtk_kparm_calc_mel_d(wtk_kparm_t *parm,double *fft)
{
	if(parm->melbank->cfg->use_htk)
	{
		// wtk_kparm_calc_htk_mel(parm,fft);
		wtk_debug("use htk\n");
		exit(1);
	}else if(parm->melbank->cfg->use_torch)
	{
		wtk_kparm_calc_torch_mel_d(parm,fft);
	}else
	{
		// wtk_kparm_calc_kaldi_mel(parm,fft);
		wtk_debug("use kaldi\n");
		exit(1);
	}
}

void wtk_kparm_feed_mfcc(wtk_kparm_t *parm,float *mel_energy,float *feat)
{
	int nbin=parm->cfg->melbank.num_bins;
	int nceps=parm->cfg->NUMCEPS;
	int i,j;
	float *dct=parm->mfcc->dct->p;
	float *lifter_coeffs=parm->mfcc->lifter_coeffs;
	float f;

	if(lifter_coeffs)
	{
		for(i=0;i<nceps;++i)
		{
			f=0;
			for(j=0;j<nbin;++j)
			{
				f+=mel_energy[j]*dct[j];
				//wtk_debug("v[%d]=%f/%f/%f\n",j,mel_energy[j],dct[j],f);
			}
			dct+=nbin;
			feat[i]=f*lifter_coeffs[i];
		}
	}else
	{
		for(i=0;i<nceps;++i)
		{
			f=0;
			for(j=0;j<nbin;++j)
			{
				f+=mel_energy[j]*(*(dct++));
			}
			feat[i]=f;
			//wtk_debug("v[%d]=%f\n",i,f);
		}
	}
	//print_float(feat,nceps);
}

void wtk_kparm_raise(wtk_kparm_t *parm,wtk_kfeat_t *feat)
{
//	static int last_ki=0;
//
//wtk_debug("raise feat[%d] used=%d want_reset=%d\n",feat->index,feat->used,parm->want_reset);
//	if(feat->index==0)
//	{
//		last_ki=0;
//	}else if(feat->index!=last_ki)
//	{
//		exit(0);
//	}
//	last_ki++;
	if(parm->stop_flag)
	{
		--feat->used;
		wtk_kparm_push_feat(parm,feat);
		return;
	}

	if(parm->want_reset)
	{
		--feat->used;
		//wtk_debug("push feat=%d used=%d\n",feat->index,feat->used);
		wtk_kparm_push_feat(parm,feat);
	}else
	{
		parm->notify(parm->notify_ths,feat);
	}
}

/**
 * feature-mfcc.cc:27
 * void MfccComputer::Compute(BaseFloat signal_log_energy,
                           BaseFloat vtln_warp,
                           VectorBase<BaseFloat> *signal_frame,
                           VectorBase<BaseFloat> *feature)
 */

void wtk_kparm_feed_frame(wtk_kparm_t *parm,float *data,int len)
{
	if (parm->cfg->use_trick)
	{
		parm->idle_hint++;
		//printf("%d\n",parm->idle_hint);
	}
	wtk_kparm_cfg_t *cfg=parm->cfg;
	float *win=parm->cfg->window;
	int i,j;
	float *fft=parm->fft;
	int nbin=parm->rfft->win;
	wtk_kfeat_t *feat;

	//++parm->nframe;
	feat=wtk_kparm_pop_feat(parm);
	//print_float(parm->input,parm->rfft->len);
	if(cfg->dither>0)
	{
		wtk_kparm_add_dither(parm,data,len);
	}
	if(cfg->remove_dc)
	{
		wtk_kparm_remove_dc(parm,data,len);
	}
	if(cfg->kfind.has_energy)
	{
		parm->log_energy_pre_window=0.0;
		for(i=0;i<len;++i)
		{
			parm->log_energy_pre_window+=data[i]*data[i];
		}
		parm->log_energy_pre_window=log(parm->log_energy_pre_window);

// wtk_debug("%f\n",parm->log_energy_pre_window);
// #ifndef USE_RTOS_OF_5215
// 		exit(0);
// #endif
	}

	if(cfg->preemph_coeff!=0)
	{
		//wtk_debug("coeff=%f\n",cfg->preemph_coeff);
		if(cfg->use_pad)
		{
			wtk_kparm_preemphasize2(parm,data,len);
		}else
		{
			wtk_kparm_preemphasize2(parm,data,len);
		}
	}
	for(i=0;i<len;++i)
	{
		data[i]*=win[i];
	}

/*	float sum=0.0;

	for(i=0;i<len;++i)
	{
		sum+=data[i]*data[i];
	}
	sum=logf(sum);*/
//	wtk_debug("%f\n",sum);
	//print_float(data,len);
	//exit(0);
	//wtk_float_fft(data,len);
	wtk_rfft_process_fft(parm->rfft,fft,data);
	//wtk_rfft_print_fft(fft,parm->rfft->len);
	//exit(0);
	//wtk_debug("fft=%f/%f\n",data[0],fft[0]);
	fft[0]*=fft[0];
	fft[nbin]*=fft[nbin];
	for(i=1,j=i+nbin;i<nbin;++i,++j)
	{
		fft[i]=fft[i]*fft[i]+fft[j]*fft[j];
		//wtk_debug("v[%d]=%f+%f\n",i,fft[i],fft[j]);
	}
	//print_float(fft,nbin);
	//exit(0);
	//print_float(fft,parm->rfft->win+1);
	if(!cfg->use_power)
	{
		for(i=0;i<=nbin;++i)
		{
			fft[i]=sqrt(fft[i]);
		}
	}
	wtk_kparm_calc_mel(parm,fft);
	//print_float(parm->melbank->mel_energy,parm->cfg->melbank.num_bins);
	//exit(0);
	switch(parm->cfg->kfind.bkind)
	{
	case WTK_FBANK:
		memcpy(feat->v,parm->melbank->mel_energy,parm->cfg->melbank.num_bins*sizeof(float));
		break;
	case WTK_MFCC:
		wtk_kparm_feed_mfcc(parm,parm->melbank->mel_energy,feat->v);
//		*(feat->v)=sum;
		//print_float(feat->v,parm->cfg->NUMCEPS);
		//exit(0);
		break;
	case WTK_PLP:
		exit(0);
		break;
	}
	if(cfg->kfind.has_energy)
	{
		if(cfg->energy_floor>0.0f && parm->log_energy_pre_window < cfg->log_energy_floor)
		{
			parm->log_energy_pre_window=cfg->log_energy_floor;
		}
		feat->v[0]=parm->log_energy_pre_window;
	}
	if(cfg->use_kind_notify)
	{
		// printf("%d %p %p %d\n",__LINE__,parm,feat,feat->index);
		if(parm->cur_cmvn_stats)
		{
			for(i=0;i<wtk_kparm_cfg_feature_base_size(cfg);++i)
			{
				parm->cur_cmvn_stats[i+1]+=feat->v[i];
			}
			parm->cur_cmvn_stats[i+1]+=1;
		}
		parm->kind_raise(parm->kind_raise_ths,feat);
		//wtk_cmvn_feed(parm->cmvn,feat);
	}else
	{
		if(parm->kcmn)
		{
//		print_float(feat->v,parm->cfg->NUMCEPS);
			qtk_kcmn_feed(parm->kcmn,feat);
		}else if(parm->cmvn)
		{
			wtk_cmvn_feed(parm->cmvn,feat);
		}else
		{
			if(parm->pcen)
			{
				wtk_pcen_feed(parm->pcen,feat);
			}
			wtk_kparm_on_cmvn(parm,feat);
			//wtk_kparm_raise(parm,feat);
		}
	}
}

void wtk_kparm_feed_frame_torch(wtk_kparm_t *parm,float *data,int len)
{
	wtk_kparm_cfg_t *cfg=parm->cfg;
	int i,j;
	float *fft=parm->fft;
	int nbin=parm->rfft->win;
	wtk_kfeat_t *feat;
	float *input = wtk_malloc(sizeof(float)*parm->rfft->len);
	// wtk_debug("cccccccc\n");
	// for(i = 0; i < len; ++i){
	// 	printf("%f\n",data[i]);
	// }
	//++parm->nframe;
	feat=wtk_kparm_pop_feat(parm);

	if(parm->cfg->melbank.use_torch)
	{
		i = (parm->rfft->len - parm->cfg->frame_size)/2;
		j=i;
		// for(i=0;i<parm->cfg->frame_size;++j,++i)
		for(i = 0; i < parm->rfft->len; ++i)
		{
			//wtk_debug("%f %f %d %d\n",data[j],parm->win_torch[j],j,len);
			// data[j]*=(parm->win_torch[j]);
			input[i] = data[i] * parm->win_torch[i];
		}
		// wtk_rfft_process_fft(parm->rfft,fft,data);
		wtk_rfft_process_fft(parm->rfft,fft,input);
		// print_float(fft,1024);
	}
	// printf("%f %f\n",fft[0],0.0f);
	// for(i = 1; i < 512; ++i){
	// 	printf("%f %f\n",fft[i],-fft[i+512]);
	// }
	// printf("%f %f\n",fft[512],0.0f);
	fft[0]*=fft[0];
	fft[nbin]*=fft[nbin];
	for(i=1,j=i+nbin;i<nbin;++i,++j)
	{
		fft[i]=fft[i]*fft[i]+fft[j]*fft[j];
		//wtk_debug("v[%d]=%f+%f\n",i,fft[i],fft[j]);
	}

	if(!cfg->use_power)
	{
		for(i=0;i<=nbin;++i)
		{
			fft[i]=sqrt(fft[i]+1e-9);
			// printf("%f\n",fft[i]);
		}
	}
	
	wtk_kparm_calc_mel(parm,fft);
	//print_float(parm->melbank->mel_energy,parm->cfg->melbank.num_bins);
	//exit(0);
	switch(parm->cfg->kfind.bkind)
	{
	case WTK_FBANK:
		memcpy(feat->v,parm->melbank->mel_energy,parm->cfg->melbank.num_bins*sizeof(float));
		break;
	case WTK_MFCC:
		wtk_kparm_feed_mfcc(parm,parm->melbank->mel_energy,feat->v);
//		*(feat->v)=sum;
		//print_float(feat->v,parm->cfg->NUMCEPS);
		//exit(0);
		break;
	case WTK_PLP:
		exit(0);
		break;
	}
	if(cfg->kfind.has_energy)
	{
		if(cfg->energy_floor>0.0f && parm->log_energy_pre_window < cfg->log_energy_floor)
		{
			parm->log_energy_pre_window=cfg->log_energy_floor;
		}
		feat->v[0]=parm->log_energy_pre_window;
	}
	//exit(0);

	 //int ii;
	 //for(ii=0;ii<30;++ii)
	 //{
	 //	printf("%d %f\n",ii,feat->v[ii]);
	 //}
	//print_float(feat->v,40);
	if(cfg->use_kind_notify)
	{
		// printf("%d %p %p %d\n",__LINE__,parm,feat,feat->index);
		//wtk_debug("xxxxxxxxxxxxxxxx?\n");
		if(parm->cur_cmvn_stats)
		{
			for(i=0;i<wtk_kparm_cfg_feature_base_size(cfg);++i)
			{
				parm->cur_cmvn_stats[i+1]+=feat->v[i];
			}
			parm->cur_cmvn_stats[i+1]+=1;
		}
		parm->kind_raise(parm->kind_raise_ths,feat);
		//wtk_cmvn_feed(parm->cmvn,feat);
	}else
	{
		if(parm->kcmn)
		{
//		print_float(feat->v,parm->cfg->NUMCEPS);
			qtk_kcmn_feed(parm->kcmn,feat);
		}else if(parm->cmvn)
		{
			wtk_cmvn_feed(parm->cmvn,feat);
		}else
		{
			if(parm->pcen)
			{
				wtk_pcen_feed(parm->pcen,feat);
			}
			wtk_kparm_on_cmvn(parm,feat);
			//wtk_kparm_raise(parm,feat);
		}
	}
	wtk_free(input);
	//exit(0);
}

void wtk_kparm_feed_frame_torch_double(wtk_kparm_t *parm,float *data,int len)
{
	wtk_kparm_cfg_t *cfg=parm->cfg;
	int i,j;
	int nbin=parm->rfft->win;
	wtk_kfeat_t *feat;
	float *input = wtk_malloc(sizeof(float)*parm->rfft->len);
	double *fftd = wtk_malloc(parm->rfft->len * sizeof(double));
	//++parm->nframe;
	feat=wtk_kparm_pop_feat(parm);
	//wtk_debug("parm=%d\n",feat->index);

	//memcpy(data,fff,sizeof(float)*len);
	//wtk_debug("----------------\n");

	if(parm->cfg->melbank.use_torch)
	{
		i = (parm->rfft->len - parm->cfg->frame_size)/2;
		j=i;
		//wtk_debug("%d %d\n",i,parm->cfg->frame_size);
		for(i = 0; i < parm->rfft->len; ++i)
		{
			//wtk_debug("%f %f %d %d\n",data[j],parm->win_torch[j],j,len);
			input[i] = data[i] * parm->win_torch[i];
			
		}
		// wtk_rfft_process_fft(parm->rfft,fft,data);
		wtk_rfft_process_fft_d(parm->rfft,fftd,input);
		//print_float(fft,1024);
	}

	fftd[0]*=fftd[0];
	fftd[nbin]*=fftd[nbin];
	for(i=1,j=i+nbin;i<nbin;++i,++j)
	{
		fftd[i]=fftd[i]*fftd[i]+fftd[j]*fftd[j];
		//wtk_debug("v[%d]=%f+%f\n",i,fft[i],fft[j]);
	}

	if(!cfg->use_power)
	{
		for(i=0;i<=nbin;++i)
		{
			fftd[i]=sqrt(fftd[i]+1e-9);
		}
	}
	//exit(0);
	wtk_kparm_calc_mel_d(parm,fftd);
	//print_float(parm->melbank->mel_energy,parm->cfg->melbank.num_bins);
	//exit(0);
	switch(parm->cfg->kfind.bkind)
	{
	case WTK_FBANK:
		// memcpy(feat->v,parm->melbank->mel_energy,parm->cfg->melbank.num_bins*sizeof(float));
		// memcpy(feat->v,parm->melbank->mel_energy,parm->cfg->melbank.num_bins*sizeof(float));
		// wtk_debug("---- > %d\n",parm->cfg->melbank.num_bins);
		for(i = 0; i < parm->cfg->melbank.num_bins; ++i){
			feat->v[i] = parm->melbank->mel_energy_double[i];
		}
		break;
	case WTK_MFCC:
		wtk_debug("22222\n");
		exit(1);
		wtk_kparm_feed_mfcc(parm,parm->melbank->mel_energy,feat->v);
//		*(feat->v)=sum;
		//print_float(feat->v,parm->cfg->NUMCEPS);
		//exit(0);
		break;
	case WTK_PLP:
		exit(0);
		break;
	}
	if(cfg->kfind.has_energy)
	{
		if(cfg->energy_floor>0.0f && parm->log_energy_pre_window < cfg->log_energy_floor)
		{
			parm->log_energy_pre_window=cfg->log_energy_floor;
		}
		feat->v[0]=parm->log_energy_pre_window;
	}
	//exit(0);

	 //int ii;
	 //for(ii=0;ii<30;++ii)
	 //{
	 //	printf("%d %f\n",ii,feat->v[ii]);
	 //}
	//print_float(feat->v,40);
	if(cfg->use_kind_notify)
	{
		// printf("%d %p %p %d\n",__LINE__,parm,feat,feat->index);
		wtk_debug("xxxxxxxxxxxxxxxx?\n");
		if(parm->cur_cmvn_stats)
		{
			for(i=0;i<wtk_kparm_cfg_feature_base_size(cfg);++i)
			{
				parm->cur_cmvn_stats[i+1]+=feat->v[i];
			}
			parm->cur_cmvn_stats[i+1]+=1;
		}
		parm->kind_raise(parm->kind_raise_ths,feat);
		//wtk_cmvn_feed(parm->cmvn,feat);
	}else
	{
		if(parm->kcmn)
		{
//		print_float(feat->v,parm->cfg->NUMCEPS);
			qtk_kcmn_feed(parm->kcmn,feat);
		}else if(parm->cmvn)
		{
			wtk_cmvn_feed(parm->cmvn,feat);
		}else
		{
			if(parm->pcen)
			{
				wtk_pcen_feed(parm->pcen,feat);
			}
			wtk_kparm_on_cmvn(parm,feat);
			//wtk_kparm_raise(parm,feat);
		}
	}
	wtk_free(input);
	wtk_free(fftd);
	//exit(0);
	return;
}

void wtk_kparm_on_cmvn(wtk_kparm_t *parm,wtk_kfeat_t *feat)
{
	//wtk_debug("feat=%d used=%d\n",feat->index,feat->used);
	//int ii;
	//for(ii=0;ii<30;++ii)
	//{
	// printf("%d %f\n",ii,feat->v[ii]);
    //}
	
	if(parm->delta)
	{
		if(parm->cfg->use_fixpoint)
		{
			wtk_delta_feed_fix(parm->delta,feat);
		}else
		{
			wtk_delta_feed(parm->delta,feat);
		}
	}else if(parm->lda)
	{
		wtk_lda_feed(parm->lda,feat,0);
	}else
	{
		if(parm->cmvn_raise)
		{
			parm->cmvn_raise(parm->cmvn_raise_ths,feat);
		}else
		{
			wtk_kparm_raise(parm,feat);
		}
	}
}

void wtk_kparm_on_kcmn2(wtk_kparm_t *parm,wtk_kfeat_t *feat)
{
//	print_float(feat->v,64);
	wtk_kparm_raise(parm,feat);
}

void wtk_kparm_on_kcmn(wtk_kparm_t *parm,wtk_kfeat_t *feat)
{
	//need cache feat because ivector should be updated
	int periord = parm->cfg->ivector.ivector_period;
	int index = 0,i;
	int right = parm->cfg->ivector.right_context+1;
	if(periord != 0 )
	{
		index = (feat->index+1)%periord;
	}
	if(index <= right && index!=0)
	{
//		wtk_debug("raise ----- index:%d\n",index);
		parm->feat[parm->cache_index] = feat;
//		wtk_debug("raise %d %d\n",parm->cache_index,feat->index);
		parm->cache_index = ((parm->cache_index+1) == right)?0:(parm->cache_index+1);
		if(index == right)
		{
			//called ivector get frame then notify kxparm
			memset(parm->ivector_feat,0,sizeof(float)*parm->ivector_dim);
			//wtk_debug("====================ivector get frame\n");
			qtk_ivector_get_frame(parm->ivector,periord*parm->ivector_index,parm->ivector_feat,parm->ivector_dim);
			//wtk_debug("fffffffffffffffffff:%f %f %d %d\n",*(parm->ivector_feat),*(parm->ivector_feat+1),index,feat->index);
			parm->ivector_index++;
			//parm->ivector_notify(parm->notify_ths,parm->ivector_feat,parm->ivector_dim);//update ivector
			for(i = 0; i < right; i++)
			{
				//wtk_debug("--------- raise: %d %d\n",parm->feat[i]->index,parm->is_end);
				wtk_kparm_raise(parm,parm->feat[i]);//send cache frame
			}
		}else
		{
			//wtk_debug("hhhhhh %d %d %d\n",feat->index,parm->cache_index,parm->is_end);
			if(parm->is_end == 1)
			{
				for(i = 0; i < parm->cache_index; i++)
				{
					//wtk_debug("--------- raise: %d %d\n",parm->feat[i]->index,parm->is_end);
					wtk_kparm_raise(parm,parm->feat[i]);
				}
			}
		}
	}else
	{
		//notify kxparm
		//wtk_debug("--------- raise: %d %d\n",feat->index,parm->is_end);
		wtk_kparm_raise(parm,feat);
	}
}

int wtk_kparm_first_sample_off_frame(wtk_kparm_t *parm) 
{
  int frame_shift = parm->cfg->frame_step;//opts.WindowShift();

  if(parm->cfg->use_snip_edges) //use_snip_edges
  {
    return 0;
  }else 
  {
    int midpoint_of_frame = frame_shift * 0  +  frame_shift / 2;
    int beginning_of_frame = midpoint_of_frame  -  parm->cfg->frame_size /2;

    return beginning_of_frame;
  }
}

void wtk_kparm_on_ivector(wtk_kparm_t *parm,float *feat,int dim)
{
	//get ivector result, set ivector to nnet3 through kxparm
}

void wtk_kparm_on_delta(wtk_kparm_t *parm,wtk_kfeat_t *feat)
{
	if(parm->lda)
	{
		wtk_lda_feed(parm->lda,feat,0);
	}else
	{
		wtk_kparm_raise(parm,feat);
	}
}

void wtk_kparm_feed_floatx(wtk_kparm_t *parm,short *data,int len,int is_end)
{
	wtk_kparm_cfg_t *cfg=parm->cfg;
	int fs=cfg->frame_size;
	int i,n,step,j;
	float *pv,*pv2;
	int nx;
	int s_in_wav;

	nx=cfg->frame_size-cfg->frame_step;
	n=0;
	//fs=frame_size=400 frame_step=160 
	while(n<len)
	{
		step=min(len-n,fs-parm->pos);
		pv=parm->input+parm->pos;

		if(parm->start_pos<0)
		{
			for(i=0;i<step;++i)
			{
				s_in_wav=i+parm->start_pos;
				if(s_in_wav<0)
				{
					s_in_wav=-s_in_wav-1;
				}
				pv[i]=data[s_in_wav];
			}
			data+=(step+parm->start_pos);
			n+=(step+parm->start_pos);
			parm->start_pos+=cfg->frame_step;
		}else
		{
			for(i=0;i<step;++i)
			{
				pv[i]=data[i];
			}
			data+=step;
			n+=step;
		}
		parm->pos+=step;

		if(parm->pos>=fs)
		{
			//will constuct input;
			// memcpy(parm->input_cache,parm->input,cfg->frame_step*sizeof(float));
			memcpy(parm->input2,parm->input+cfg->frame_step,nx*sizeof(float));
			wtk_kparm_feed_frame(parm,parm->input,parm->pos);
			parm->pos=nx;

			pv=parm->input;
			parm->input=parm->input2;
			parm->input2=pv;
		}
	}

	if(is_end)
	{
		parm->is_end=1;
		if(parm->pos>0)
		{
			// n=((int)((parm->pos+(cfg->frame_step/2))/(1.0f*cfg->frame_step)+0.5))%2;
			n=(int)((parm->pos-cfg->frame_step)/(1.0f*cfg->frame_size) + 0.5);
			// printf("aaaa %d %d %d %d\n",parm->pos,n,parm->nframe,cfg->frame_step);
			pv=parm->input;
			for(i=parm->pos,j=parm->pos-1;i<fs;++i,--j)
			{
				pv[i]=pv[j];
			}

			if(n==1)
			{
				pv2=parm->input2;
				pv=parm->input;
				for(i=0,j=cfg->frame_step;j<parm->pos;++i,++j)
				{
					pv2[i]=pv[j];
				}

				for(j=parm->pos-1;i<fs;++i,--j)
				{
					pv2[i]=pv[j];
				}
			}

			wtk_kparm_feed_frame(parm,parm->input,fs);	

			if(n==1)
			{
				wtk_kparm_feed_frame(parm,parm->input2,fs);
			}

		}

		if(cfg->use_kind_notify)
		{
			return;
		}
		
		//wtk_debug("pos=%d nframe=%d\n",parm->pos,parm->nframe);
		if(parm->cmvn)
		{
			wtk_cmvn_flush(parm->cmvn);
		}
		if(parm->delta)
		{
			wtk_delta_flush(parm->delta);
		}
		if(parm->lda)
		{
			wtk_lda_flush(parm->lda);
		}
	}
}

void wtk_kparm_feed_torch(wtk_kparm_t *parm,short *data,int len,int is_end)
{
	wtk_kparm_cfg_t *cfg=parm->cfg;
	int fs=cfg->frame_size;
	int i,j,n,step,tmp_len;
	float *pv,*pv2;
	float *tmp;
	short *d = data;

	tmp_len = parm->rfft->len - parm->cfg->frame_step;
	//nx=cfg->frame_size-cfg->frame_step;
	//wtk_debug("%d %d %d\n",nx,cfg->frame_size,cfg->frame_step);
	n=0;

	if(len > 0)
	{
		parm->feed_idx++;
	}

	if(parm->feed_idx == 1)
	{
		fs = parm->rfft->len/2 + 1;
		if(len + parm->pos < fs)
		{
			parm->feed_idx = 0;
			pv=parm->input2+parm->pos;
			for(i=0;i<len;++i)
			{
				pv[i]=data[i]/32768.0;
			}
			parm->pos+=len;
			len = 0;
			//exit(0);//TODO len must > 1024 when first feed
		}else
		{
			pv = parm->input2 + parm->pos;
			pv2 = parm->input;
			for(i=0;i<fs-parm->pos;++i)
			{
				pv[i] = data[i]/32768.0;
			}
			//print_float(pv,513);
			d += fs-parm->pos;
			len = len-fs+parm->pos;
			parm->last_point = parm->input2[fs-1];

			//wtk_debug("%f\n",parm->last_point);
			pv = parm->input2;

			wtk_kparm_preemphasize(parm,pv,fs);
			for(j=fs-2,i=0;j>=0;j--,i++)
			{
				pv2[j] = pv[i];
			}
			memcpy(pv2+fs-1,pv,sizeof(float)*(fs-1));
			memcpy(pv,pv2+parm->cfg->frame_step,sizeof(float)*tmp_len);
			parm->input3[0] = parm->last_point;
			parm->pos = 1;
			//parm->pos = 864;
			// print_float(parm->input,400);
			// wtk_kparm_feed_frame_torch(parm,parm->input,400);
			wtk_kparm_feed_frame_torch_double(parm,parm->input,parm->cfg->frame_size);
		}
	}
	fs=cfg->frame_step;
	//wtk_debug("%d %d\n",parm->pos,fs);
	while(n < len)
	{
		step=min(len-n,fs-parm->pos+1);
		//wtk_debug("%d %d %d\n",step,parm->pos,fs);
		pv=parm->input3+parm->pos;
		for(i=0;i<step;++i)
		{
			pv[i]=d[i]/32768.0;
			//wtk_debug("%d %d %f\n",parm->pos,i,pv[i]);
		}

		parm->pos+=step;
		if(parm->pos>fs)
		{
			parm->last_point = parm->input3[parm->cfg->frame_step];
			//print_float(parm->input3,161);
			//wtk_debug("%f\n",parm->last_point);
			wtk_kparm_preemphasize(parm,parm->input3,parm->cfg->frame_step+1);
			memcpy(parm->input2+tmp_len,parm->input3,sizeof(float)*parm->cfg->frame_step);
			tmp = parm->input;
			parm->input = parm->input2;
			parm->input2 = tmp;
			memcpy(parm->input2,parm->input+parm->cfg->frame_step,sizeof(float)*tmp_len);
			parm->input3[0] = parm->last_point;
			parm->pos = 1;
			//print_float(parm->input,1024);
			// wtk_kparm_feed_frame_torch(parm,parm->input,400);
			wtk_kparm_feed_frame_torch_double(parm,parm->input,parm->cfg->frame_size);
		}
		d+=step;
		n+=step;
//		if(wcc==2)
//		{
//			exit(0);
//		}
	}

//	while(n<len)
//	{
//		step=min(len-n,fs-parm->pos);
//		wtk_debug("%d %d %d\n",step,parm->pos,fs);
//		pv=parm->input+parm->pos;
//		for(i=0;i<step;++i)
//		{
//			pv[i]=data[i]/32768.0;
//		}
//		parm->pos+=step;
//		if(parm->pos>=fs)
//		{
//			//will constuct input;
//			memcpy(parm->input2,parm->input+cfg->frame_step,nx*sizeof(float));
//			wtk_kparm_feed_frame_torch(parm,parm->input,parm->pos);
//			parm->pos=nx;
//			pv=parm->input;
//			parm->input=parm->input2;
//			parm->input2=pv;
//		}
//		data+=step;
//		n+=step;
//	}
	if(is_end)
	{
		parm->is_end=1;
		if(parm->feed_idx == 0){
			wtk_debug("warning: the speech is short \n");
			return;
		}
		if(parm->pos>0)
		{
			// wtk_debug("%d %d %d %d\n",parm->pos,fs,fs-parm->pos,parm->rfft->len);
			// memcpy(parm->input,parm->input3);
			wtk_kparm_preemphasize(parm,parm->input3,parm->pos+1);
			// for(i = 0; i < parm->pos-1; ++i){
			// 	printf("%f\n",parm->input3[i]);
			// }
			fs = tmp_len+parm->pos;
			pv = wtk_malloc(sizeof(float)*(fs+parm->rfft->len/2));
			memcpy(pv,parm->input2,sizeof(float)*tmp_len);
			memcpy(pv+tmp_len,parm->input3,parm->pos*sizeof(float));
			//low
			for(i = 0; i < parm->rfft->len/2;++i){
				pv[fs+i] = pv[fs-i-1];
			}
			// memcpy(parm->input2+864,parm->input3,parm->pos*sizeof(float));
			// memset(parm->input+parm->pos,0,(parm->rfft->len-parm->pos)*sizeof(float));
			// wtk_kparm_feed_frame_torch(parm,parm->input,parm->pos);
			n = 0;
			do{
				// wtk_kparm_feed_frame_torch(parm,pv+n,400);
				wtk_kparm_feed_frame_torch_double(parm,pv+n,parm->cfg->frame_size);
				n+=parm->cfg->frame_step;
			}while(n < (fs-(parm->rfft->len/2)));
			parm->pos=0;
			wtk_free(pv);
		}
		//wtk_debug("pos=%d nframe=%d\n",parm->pos,parm->nframe);
		if(parm->cmvn)
		{
			wtk_cmvn_flush(parm->cmvn);
		}
//		if(parm->kcmn)
//		{
//			qtk_kcmn_flush(parm->kcmn);
//		}
		if(parm->delta)
		{
			wtk_delta_flush(parm->delta);
		}
		if(parm->lda)
		{
			wtk_lda_flush(parm->lda);
		}
	}
}

void wtk_kparm_feed_floatx2(wtk_kparm_t *parm,short *data,int len,int is_end)
{
	wtk_kparm_cfg_t *cfg=parm->cfg;
	int fs=cfg->frame_size;
	int i,n,step;
	float *pv;
	int nx;
	int k2_num_frames = 0;
	int k2_wav_samples = 0;
	int padding_samples = 0;
	short *tmp = NULL;

	parm->wav_bytes += len;
	if(cfg->use_k2_offline && is_end){
		k2_num_frames = (parm->wav_bytes +160/2)/160;
		k2_wav_samples = ((k2_num_frames - 1) * 0.01 + 0.025 ) * 16000;
		padding_samples = k2_wav_samples - parm->wav_bytes;
		//wtk_debug("%d %d %d\n",k2_num_frames,k2_wav_samples,padding_samples);
		tmp = (short*)wtk_malloc((padding_samples+len)*sizeof(short));
		memset(tmp,0,(padding_samples+len)*sizeof(short));
		memcpy(tmp,data,len*sizeof(short));
		data = tmp;
		len += padding_samples;
		parm->wav_bytes += padding_samples;
	}


	nx=cfg->frame_size-cfg->frame_step;
	n=0;
	while(n<len)
	{
		//wtk_debug("%d %d\n",parm->start,cfg->use_pad);
		if(cfg->use_pad && parm->start)
		{
			pv=parm->input+parm->pos;
			parm->start=0;
			if(len < 120)
			{
				exit(0);
			}
			for(i=0;i<120;++i)
			{
				pv[i]=data[119-i]/32768.0;
			}
			parm->pos+=120;
		}

		step=min(len-n,fs-parm->pos);
		pv=parm->input+parm->pos;
		for(i=0;i<step;++i)
		{
			if(cfg->use_pad)
			{
				pv[i]=data[i]/32768.0;
			}else
			{
				pv[i]=data[i];
			}
		}
		parm->pos+=step;
		if(parm->pos>=fs)
		{
			//will constuct input;
			memcpy(parm->input2,parm->input+cfg->frame_step,nx*sizeof(float));
			wtk_kparm_feed_frame(parm,parm->input,parm->pos);
			parm->pos=nx;
			pv=parm->input;
			parm->input=parm->input2;
			parm->input2=pv;
		}
		data+=step;
		n+=step;
	}
	if(is_end)
	{
		parm->is_end=1;
		if(!cfg->use_k2_offline && parm->pos>0)
		{
			memset(parm->input+parm->pos,0,(fs-parm->pos)*sizeof(float));
			wtk_kparm_feed_frame(parm,parm->input,parm->pos);
			parm->pos=0;
		}

		if(cfg->use_k2_offline)
		{
			wtk_free(tmp);
		}

		//wtk_debug("pos=%d nframe=%d\n",parm->pos,parm->nframe);
		if(parm->cmvn)
		{
			wtk_cmvn_flush(parm->cmvn);
		}
//		if(parm->kcmn)
//		{
//			qtk_kcmn_flush(parm->kcmn);
//		}
		if(parm->delta)
		{
			wtk_delta_flush(parm->delta);
		}
		if(parm->lda)
		{
			wtk_lda_flush(parm->lda);
		}
	}
}



void wtk_kparm_print_int(int *v,int len)
{
	int i;

	for(i=0;i<len;++i)
	{
		wtk_debug("v[%d]=%d/%f\n",i,v[i],FIX2FLOAT(v[i]));
	}
}

void wtk_kparm_calc_fix_mel(wtk_kparm_t *parm,wtk_fix_t *fft)
{
	static int MEL_MIN_I=FLOAT2FIX(-15.94239);
	wtk_kparm_cfg_t *cfg=parm->cfg;
	int num_bins=cfg->melbank.num_bins;
	wtk_fix_melbin_t *bin;
	int i,j,k,e;
	wtk_fix_t energy;
	wtk_fix_melbank_t *bank=parm->fix_melbank;
	wtk_fix_t *me=bank->mel_energy;
	wtk_fix_t *w;

	for(i=0;i<num_bins;++i)
	{
		bin=bank->bins+i;
		w=bin->w;
		e=bin->e;
		energy=0;
		for(j=bin->s,k=0;j<=e;++j,++k)
		{
			//wtk_debug("v[%d/%d]=%f\n",k,j,bin->w[k]);
			if(j==bin->s)
			{
				energy=fft[j]+w[k];
				//wtk_debug("%d/%f %d/%f %d/%f\n",fft[j],FIX2FLOAT(fft[j]),w[k],FIX2FLOAT(w[k]),energy,FIX2FLOAT(energy));
			}else
			{
				energy=fe_log_add(energy,fft[j]+w[k]);
			}
		}
		if(energy<MEL_MIN_I)
		{
			me[i]=MEL_MIN_I;
		}else
		{
			me[i]=energy;
		}
		//wtk_debug("energy[%d]=%d/%f\n",i,energy,FIX2FLOAT(energy));
	}
	//wtk_fix_print_float(me,num_bins);
	//exit(0);
	//exit(0);
}



void wtk_kparm_feed_fix_mfcc(wtk_kparm_t *parm,int *mel_energy,int *feat)
{
	int nbin=parm->cfg->melbank.num_bins;
	int nceps=parm->cfg->NUMCEPS;
	int i,j;
	wtk_fix_t *dct=parm->fix_mfcc->dct->p;
	wtk_fix_t *lifter_coeffs=parm->fix_mfcc->lifter_coeffs;
	int f;

	if(lifter_coeffs)
	{
		for(i=0;i<nceps;++i)
		{
			f=0;
			for(j=0;j<nbin;++j)
			{
				f+=FIXMUL(mel_energy[j],dct[j]);
				//f+=mel_energy[j]*dct[j];
			}
			dct+=nbin;
			feat[i]=FIXMUL(f,lifter_coeffs[i]);
			//feat[i]=FIX2FLOAT(f);
		}
	}else
	{
		for(i=0;i<nceps;++i)
		{
			f=0;
			for(j=0;j<nbin;++j)
			{
				f+=FIXMUL(mel_energy[j],dct[j]);
				//f+=mel_energy[j]*dct[j];
			}
			dct+=nbin;
			feat[i]=f;
			//feat[i]=FLOAT2FIX(f);
		}
	}
}


void wtk_kparm_feed_fix_frame(wtk_kparm_t *parm,wtk_fix_t *data,int len)
{
	wtk_kparm_cfg_t *cfg=parm->cfg;
	int i,n,fft_size;
	int alpha,v;
	wtk_fix_t mean;
	wtk_fix_t *win=parm->cfg->fix_win;
	wtk_fix_t *spec=parm->spec;
	wtk_fix_t rr,ii;
	wtk_kfeat_t *feat;

	//++parm->nframe;
	feat=wtk_kparm_pop_feat(parm);
	//wtk_debug("============== feat[%d] ==============\n",feat->index);
	if(cfg->remove_dc)
	{
		mean=0;
		for(i=0;i<len;++i)
		{
			mean+=data[i];
		}
		mean/=len;
		//wtk_debug("mean=%d\n",mean);
		for(i=0;i<len;++i)
		{
			data[i]-=mean;
		}
		//print_int(data,len);
	}
	if(cfg->fix_preemph_coeff!=0)
	{
		alpha=cfg->fix_preemph_coeff;
		v=data[len-1];
		for(i=len-1;i>0;--i)
		{
			data[i]=(data[i]<<DEFAULT_RADIX) - data[i-1]*alpha;
		}
		//keep like kaldi
		parm->pre_emphasis_prior=data[0];
		data[0]=(data[0]<<DEFAULT_RADIX)-parm->pre_emphasis_prior*alpha;
		parm->pre_emphasis_prior=v;
	}

	//window
	n=len;
	for(i=0;i<n;++i)
	{
		data[i]=COSMUL(data[i],win[i]);
	}
	memset(data+n,0,(parm->win-n)*sizeof(wtk_fix_t));
	wtk_kparm_fix_fft(parm,data);
	//wtk_debug("fft=%d/%f\n",data[0],FIX2FLOAT(data[0]));
	spec[0] = FIXLN(abs(data[0]))*2;
	fft_size=parm->win;
	n=fft_size/2;
	//wtk_fix_print_float(data,n+1);
	for(i=1;i<=n;++i)
	{
		rr = FIXLN(abs(data[i])) * 2;
        ii = FIXLN(abs(data[fft_size-i])) * 2;
     //   wtk_debug("v[%d]=%d\n",i,fe_log_add(rr,ii));
        spec[i]=fe_log_add(rr,ii);
        //wtk_debug("v[%d]=%d/%d %d/%d/%d\n",i,data[i],data[len-i],rr,ii,spec[i]);
	}
	//wtk_kparm_print_int(spec,n+1);
	wtk_kparm_calc_fix_mel(parm,spec);
	//wtk_kparm_mel_spec(parm);
	switch(parm->cfg->kfind.bkind)
	{
	case WTK_FBANK:
		memcpy(feat->v,parm->fix_melbank->mel_energy,parm->cfg->melbank.num_bins*sizeof(int));
		//wtk_fix_print((int*)(feat->v),parm->cfg->vec_size2,12);
		//exit(0);
		//memcpy(feat->v,parm->melbank->mel_energy,parm->cfg->melbank.num_bins*sizeof(float));
		break;
	case WTK_MFCC:
		wtk_kparm_feed_fix_mfcc(parm,parm->fix_melbank->mel_energy,(int*)(feat->v));
		//print_float(feat->v,parm->cfg->NUMCEPS);
		//exit(0);
		break;
	case WTK_PLP:
#ifndef USE_RTOS_OF_5215
		exit(0);
#endif
		break;
	}

	if(parm->cmvn)
	{
		wtk_cmvn_feed_fix(parm->cmvn,feat);
	}else
	{
		//wtk_kparm_raise(parm,feat);
		wtk_kparm_on_cmvn(parm,feat);
	}
}

void wtk_kparm_feed_fix(wtk_kparm_t *parm,short *data,int len,int is_end)
{
	wtk_kparm_cfg_t *cfg=parm->cfg;
	int fs=cfg->frame_size;
	int i,n,step;
	wtk_fix_t *pv;
	int nx;

	nx=cfg->frame_size-cfg->frame_step;
	n=0;
	while(n<len)
	{
		step=min(len-n,fs-parm->pos);
		pv=parm->fix_input+parm->pos;
		for(i=0;i<step;++i)
		{
			pv[i]=data[i];
		}
		parm->pos+=step;
		if(parm->pos>=fs)
		{
			//will constuct input;
			memcpy(parm->fix_input2,parm->fix_input+cfg->frame_step,nx*sizeof(wtk_fix_t));
			wtk_kparm_feed_fix_frame(parm,parm->fix_input,parm->pos);
			parm->pos=nx;
			pv=parm->fix_input;
			parm->fix_input=parm->fix_input2;
			parm->fix_input2=pv;
		}
		data+=step;
		n+=step;
	}
	if(is_end)
	{
		if(parm->pos>0)
		{
			memset(parm->fix_input+parm->pos,0,(fs-parm->pos)*sizeof(wtk_fix_t));
			wtk_kparm_feed_fix_frame(parm,parm->fix_input,parm->pos);
			parm->pos=0;
		}
		//wtk_debug("pos=%d nframe=%d\n",parm->pos,parm->nframe);
		//wtk_debug("======== start flush ========\n");
		if(parm->cmvn)
		{
			wtk_cmvn_flush_fix(parm->cmvn);
		}
		//wtk_debug("======== start flush delta ========\n");
		if(parm->delta)
		{
			wtk_delta_flush_fix(parm->delta);
		}
	}
}

void wtk_kparm_feed(wtk_kparm_t *parm,short *data,int len,int is_end)
{
	if(parm->cfg->use_fixpoint)
	{
		wtk_kparm_feed_fix(parm,data,len,is_end);
	}else
	{
		if(parm->cfg->use_snip_edges==0)
		{
			wtk_kparm_feed_floatx(parm,data,len,is_end);
		}else
		{
			if(parm->cfg->melbank.use_torch == 1)
			{
				wtk_kparm_feed_torch(parm,data,len,is_end);
			}else
			{
				wtk_kparm_feed_floatx2(parm,data,len,is_end);
			}
		}
		
	}
}

void wtk_kparm_feed_float(wtk_kparm_t *parm,float *data,int len,int is_end)
{
	wtk_kparm_cfg_t *cfg=parm->cfg;
	int fs=cfg->frame_size;
	int i,n,step;
	float *pv;
	int nx;
	float f=32767;

	nx=cfg->frame_size-cfg->frame_step;
	n=0;
	while(n<len)
	{
		step=min(len-n,fs-parm->pos);
		pv=parm->input+parm->pos;
		for(i=0;i<step;++i)
		{
			pv[i]=data[i]*f;
		}
		parm->pos+=step;
		if(parm->pos>=fs)
		{
			//will constuct input;
			memcpy(parm->input2,parm->input+cfg->frame_step,nx*sizeof(float));
			wtk_kparm_feed_frame(parm,parm->input,parm->pos);
			parm->pos=nx;
			pv=parm->input;
			parm->input=parm->input2;
			parm->input2=pv;
		}
		data+=step;
		n+=step;
	}
	if(is_end)
	{
		if(parm->pos>0)
		{
			memset(parm->input+parm->pos,0,(fs-parm->pos)*sizeof(float));
			wtk_kparm_feed_frame(parm,parm->input,parm->pos);
			parm->pos=0;
		}
		//wtk_debug("pos=%d nframe=%d\n",parm->pos,parm->nframe);
		if(parm->cmvn)
		{
			wtk_cmvn_flush(parm->cmvn);
		}
		if(parm->delta)
		{
			wtk_delta_flush(parm->delta);
		}
		if(parm->lda)
		{
			wtk_lda_flush(parm->lda);
		}
	}
}
