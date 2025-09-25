#include "wtk_qform_pickup5.h" 


#define qform_pickup5_mmse_tobank(n)   (13.1f*atan(.00074f*(n))+2.24f*atan((n)*(n)*1.85e-8f)+1e-4f*(n))

wtk_qform_pickup5_mmse_xfilterbank_t* wtk_qform_pickup5_mmse_xfilterbank_new()
{
	wtk_qform_pickup5_mmse_xfilterbank_t *bank;
	float df;
	float max_mel, mel_interval;
   int i;
   int id1;
   int id2;
   float curr_freq;
   float mel;
   float val;

   bank = (wtk_qform_pickup5_mmse_xfilterbank_t*)wtk_malloc(sizeof(wtk_qform_pickup5_mmse_xfilterbank_t));
   df =RATE*1.0f/(2*MMSE_STEP);
   max_mel = qform_pickup5_mmse_tobank(RATE/2);
   mel_interval =max_mel/(NBANDS-1);
   bank->bank_left = (int*)wtk_malloc(MMSE_STEP*sizeof(int));
   bank->bank_right = (int*)wtk_malloc(MMSE_STEP*sizeof(int));
   bank->filter_left = (float*)wtk_malloc(MMSE_STEP*sizeof(float));
   bank->filter_right = (float*)wtk_malloc(MMSE_STEP*sizeof(float));
   for (i=0;i<MMSE_STEP;i++)
   {
      curr_freq = i*df;
      mel = qform_pickup5_mmse_tobank(curr_freq);
      if (mel > max_mel)
      {
         break;
      }
      id1 = (int)(floor(mel/mel_interval));
      if (id1>NBANDS-2)
      {
         id1 = NBANDS-2;
         val = 1;
      }else
      {
    	  val=(mel - id1*mel_interval)/mel_interval;
      }
      id2 = id1+1;
      bank->bank_left[i] = id1;
      bank->filter_left[i] = 1-val;
      bank->bank_right[i] = id2;
      bank->filter_right[i] = val;
     //wtk_debug("v[%d]=%d/%d %f/%f mel=%f/%f %f\n",i,bank->bank_left[i],bank->bank_right[i],bank->filter_left[i],bank->filter_right[i],mel,curr_freq,df);
   }
   return bank;

}

void wtk_qform_pickup5_mmse_xfilterbank_delete(wtk_qform_pickup5_mmse_xfilterbank_t *bank)
{
	   wtk_free(bank->bank_left);
	   wtk_free(bank->bank_right);
	   wtk_free(bank->filter_left);
	   wtk_free(bank->filter_right);
	   wtk_free(bank);
}


void wtk_qform_pickup5_mmse_xfilterbank_compute_bank32(wtk_qform_pickup5_mmse_xfilterbank_t *bank, float *ps, float *mel)
{
   int i;
   int *left=bank->bank_left;
   int *right=bank->bank_right;
   float *fleft=bank->filter_left;
   float *fright=bank->filter_right;

   memset(mel,0,NBANDS*sizeof(float));
   for (i=0;i<MMSE_STEP;i++)
   {
      mel[left[i]] += fleft[i]*ps[i];
      mel[right[i]] += fright[i]*ps[i];
    // wtk_debug("v[%d]=%f/%f %f %f/%f\n",i,mel[left[i]],mel[right[i]],ps[i],fleft[i],fright[i]);
   }
}


void wtk_qform_pickup5_mmse_xfilterbank_compute_psd16(wtk_qform_pickup5_mmse_xfilterbank_t *bank,float *mel,float *ps)
{
   int i;
   int *left=bank->bank_left;
   int *right=bank->bank_right;
   float *fleft=bank->filter_left;
   float *fright=bank->filter_right;

   for (i=0;i<MMSE_STEP;i++)
   {
      ps[i] = mel[left[i]]*fleft[i]+mel[right[i]]*fright[i];
   }
}



float *wtk_qform_pickup5_analysiswin_new(int win)
{
	float *f;
	int i;

	f=(float*)wtk_malloc(win*sizeof(float));
	for(i=0;i<win;++i)
	{
		f[i]=sin((0.5+i)*PI/(win));
	}

	return f;
}

float *wtk_qform_pickup5_synthesiswin_new(float *win, int len, int step)
{
    int nstep;
    int i, j, n;
    float *w;

    w = wtk_calloc(sizeof(float), len);
    nstep = len / step;

    for (i = 0; i < step; ++i)
    {
        for (j = 0; j <= nstep; ++j)
        {
            n = i + j * step;
            if (n < len)
            {
                w[i] += win[n] * win[n];
            }
        }
    }

    for (i = 1; i < nstep; ++i) 
    {
        for (j = 0; j < step; ++j) 
        {
            w[i * step + j] = w[j];
        }
    }

    for (i = 0; i < len; ++i) 
    {
        w[i] = win[i] / w[i];
    }

    return w;
}


wtk_qform_pickup5_t* wtk_qform_pickup5_new()
{
    wtk_qform_pickup5_t *qform_pickup5;
    int i;

    qform_pickup5=(wtk_qform_pickup5_t *)wtk_malloc(sizeof(wtk_qform_pickup5_t));
    qform_pickup5->ths=NULL;
    qform_pickup5->notify=NULL;
    

    qform_pickup5->mic_class[0][0]=2;
    qform_pickup5->mic_class[0][1]=1;
    qform_pickup5->mic_class[1][0]=1;
    qform_pickup5->mic_class[1][1]=0;

    qform_pickup5->micclass_pos[0][0]=0.01f;
    qform_pickup5->micclass_pos[0][1]=0.0f;
    qform_pickup5->micclass_pos[0][2]=0.0f;
    qform_pickup5->micclass_pos[1][0]=-0.01f;
    qform_pickup5->micclass_pos[1][1]=0.0f;
    qform_pickup5->micclass_pos[1][2]=0.0f;

    qform_pickup5->mic_pos[0][0]=-0.01f;
    qform_pickup5->mic_pos[0][1]=-0.01f;
    qform_pickup5->mic_pos[0][2]=0.0f;
    qform_pickup5->mic_pos[1][0]=-0.01f;
    qform_pickup5->mic_pos[1][1]=0.01f;
    qform_pickup5->mic_pos[1][2]=0.0f;
    qform_pickup5->mic_pos[2][0]=0.01f;
    qform_pickup5->mic_pos[2][1]=0.01f;
    qform_pickup5->mic_pos[2][2]=0.0f;

    qform_pickup5->input=wtk_float_new_p2(CHANNEL, LWINDOW);
    qform_pickup5->analysis_win=wtk_qform_pickup5_analysiswin_new(LWINDOW);
    qform_pickup5->synthesis_win=wtk_qform_pickup5_synthesiswin_new(qform_pickup5->analysis_win, LWINDOW, FRAME_STEP);

	qform_pickup5->rfft=wtk_rfft_new(LWINDOW/2);

    qform_pickup5->ang_num=12;
    qform_pickup5->gcc_ovec=wtk_complex_new_p3(qform_pickup5->ang_num,NBIN,CHANNEL2);
    for(i=0;i<OUT_CHANNEL;++i){
        qform_pickup5->ncov[i]=wtk_complex_new_p2(NBIN,CHANNELX2);
        qform_pickup5->ovec[i]=wtk_complex_new_p2(NBIN,CHANNEL);
        qform_pickup5->w[i]=wtk_complex_new_p2(NBIN,CHANNEL);
        qform_pickup5->pad[i]=(float*)wtk_malloc(LWINDOW*sizeof(float));
        qform_pickup5->zeta[i]=(float*)wtk_calloc(NBANDS,sizeof(float));
        qform_pickup5->noise_frame[i]=(int*)wtk_calloc(MMSE_STEP,sizeof(int));
        qform_pickup5->noise[i]=(float*)wtk_calloc(NM,sizeof(float));
        qform_pickup5->old_ps[i]=(float*)wtk_calloc(NM,sizeof(float));
        qform_pickup5->ncnt_sum[i]=(float *)wtk_malloc(sizeof(float)*NBIN);

    }


    qform_pickup5->bank=wtk_qform_pickup5_mmse_xfilterbank_new();
    qform_pickup5->min_noise=1e-9;

    wtk_qform_pickup5_reset(qform_pickup5);

    return qform_pickup5;
}

void wtk_qform_pickup5_delete(wtk_qform_pickup5_t *qform_pickup5)
{
    int i;
    wtk_float_delete_p2(qform_pickup5->input, CHANNEL);
    wtk_free(qform_pickup5->analysis_win);
    wtk_free(qform_pickup5->synthesis_win);
    wtk_rfft_delete(qform_pickup5->rfft);

    wtk_complex_delete_p3(qform_pickup5->gcc_ovec, qform_pickup5->ang_num, NBIN);
    for(i=0;i<OUT_CHANNEL;++i){
        wtk_complex_delete_p2(qform_pickup5->ncov[i], NBIN);
        wtk_complex_delete_p2(qform_pickup5->ovec[i], NBIN);
        wtk_complex_delete_p2(qform_pickup5->w[i], NBIN);
        wtk_free(qform_pickup5->pad[i]);
        wtk_free(qform_pickup5->zeta[i]);
        wtk_free(qform_pickup5->noise_frame[i]);
        wtk_free(qform_pickup5->noise[i]);
        wtk_free(qform_pickup5->old_ps[i]);
        wtk_free(qform_pickup5->ncnt_sum[i]);
    }

    wtk_qform_pickup5_mmse_xfilterbank_delete(qform_pickup5->bank);

    wtk_free(qform_pickup5);
}

void wtk_qform_pickup5_reset(wtk_qform_pickup5_t *qform_pickup5)
{
    int i,j;

    for(i=0;i<CHANNEL;++i)
    {
        memset(qform_pickup5->notch_mem[i],0,2*sizeof(float));
    }
	memset(qform_pickup5->memD,0,CHANNEL*sizeof(float));
    wtk_float_zero_p2(qform_pickup5->input, CHANNEL, LWINDOW);

    qform_pickup5->start_pos=0;
    qform_pickup5->pos=0;
    qform_pickup5->nframe=0;
    wtk_complex_zero_p3(qform_pickup5->gcc_ovec,qform_pickup5->ang_num, NBIN, 2);
    for(i=0;i<OUT_CHANNEL;++i){
        qform_pickup5->memX[i]=0;
        wtk_complex_zero_p2(qform_pickup5->ncov[i],NBIN,CHANNELX2);
        wtk_complex_zero_p2(qform_pickup5->ovec[i],NBIN,CHANNEL);
        wtk_complex_zero_p2(qform_pickup5->w[i],NBIN,CHANNEL);
        memset(qform_pickup5->pad[i], 0, sizeof(float)*LWINDOW);
        for(j=0;j<MMSE_STEP;++j)
        {
            qform_pickup5->noise_frame[i][j]=0;
            qform_pickup5->noise[i][j]=qform_pickup5->min_noise;
        }
        memset(qform_pickup5->zeta[i],0,NBANDS*sizeof(float));
        memset(qform_pickup5->old_ps[i],0,NM*sizeof(float));
        memset(qform_pickup5->ncnt_sum[i],0,sizeof(int)*NBIN);
    }
}


void wtk_qform_pickup5_set_notify(wtk_qform_pickup5_t *qform_pickup5,void *ths,wtk_qform_pickup5_notify_f notify)
{
    qform_pickup5->ths=ths;
    qform_pickup5->notify=notify;
}


void wtk_qform_pickup5_update_rnn1(wtk_qform_pickup5_t *qform_pickup5,wtk_complex_t *fft,int k, int chn)
{
    int i,j;
    wtk_complex_t *fft1,*fft2,*a,*b;
    wtk_complex_t **ncov=qform_pickup5->ncov[chn];
    // wtk_complex_t **ncovtmp=qform_pickup5->ncovtmp;

    if(qform_pickup5->ncnt_sum[chn][k]>=INIT_NCOVNF)
    {
        ++qform_pickup5->ncnt_sum[chn][k];
        fft1=fft2=fft;
        for(i=0;i<CHANNEL;++i,++fft1)
        {
            fft2=fft1;
            for(j=i;j<CHANNEL;++j,++fft2)
            {
                a=ncov[k]+i*CHANNEL+j;
                a->a=(1-NCOV_ALPHA)*a->a+NCOV_ALPHA*(fft1->a*fft2->a+fft1->b*fft2->b);
                a->b=(1-NCOV_ALPHA)*a->b+NCOV_ALPHA*(-fft1->a*fft2->b+fft1->b*fft2->a);

                if(i!=j)
                {
                    b=ncov[k]+j*CHANNEL+i;
                    b->a=a->a;
                    b->b=-a->b;
                }
            }
        }
    }else
    {
        ++qform_pickup5->ncnt_sum[chn][k];
        fft1=fft2=fft;
        for(i=0;i<CHANNEL;++i,++fft1)
        {
            fft2=fft1;
            for(j=i;j<CHANNEL;++j,++fft2)
            {
                a=ncov[k]+i*CHANNEL+j;
                a->a+=fft1->a*fft2->a+fft1->b*fft2->b;
                a->b+=-fft1->a*fft2->b+fft1->b*fft2->a;

                if(i!=j)
                {
                    b=ncov[k]+j*CHANNEL+i;
                    b->a=a->a;
                    b->b=-a->b;
                }
            }
        }
        if(qform_pickup5->ncnt_sum[chn][k]==INIT_NCOVNF)
        {
            for(i=0;i<CHANNEL;++i,++fft1)
            {
                fft2=fft1;
                for(j=i;j<CHANNEL;++j,++fft2)
                {
                    a=ncov[k]+i*CHANNEL+j;
                    a->a/=INIT_NCOVNF;
                    a->b/=INIT_NCOVNF;

                    if(i!=j)
                    {
                        b=ncov[k]+j*CHANNEL+i;
                        b->a=a->a;
                        b->b=-a->b;
                    }
                }
            }
        }
    }
}

void wtk_qform_pickup5_flush_gccspec_k(wtk_qform_pickup5_t *qform_pickup5, float cov_travg, wtk_complex_t *covclass,
                                                                                                                 float cov_travg2, wtk_complex_t *covclass2, int k, float *spec_k, int ang_idx)
{
    wtk_complex_t *a,*b,*c,*a2;
    float fa,fb,ff,fa2,fb2,ff2,f;
    wtk_complex_t *ovec=qform_pickup5->gcc_ovec[ang_idx][k];
    int i,j;

    a=covclass;
    a2=covclass2;
    ff=ff2=0;
    for(i=0;i<CHANNEL2;++i)
    {
        fa=fb=0;
        fa2=fb2=0;
        for(j=0;j<CHANNEL2;++j)
        {
            b=ovec+j;
            fa+=a->a*b->a-a->b*b->b;
            fb+=a->a*b->b+a->b*b->a;
            ++a;

            fa2+=a2->a*b->a-a2->b*b->b;
            fb2+=a2->a*b->b+a2->b*b->a;
            ++a2;
        }
        c=ovec+i;
        ff+=fa*c->a+fb*c->b;
        ff2+=fa2*c->a+fb2*c->b;
    }
    f=ff/(CHANNEL2*CHANNEL2);
    spec_k[0]=f/(cov_travg-f);
    f=ff2/(CHANNEL2*CHANNEL2);
    spec_k[1]=f/(cov_travg2-f);
}


void wtk_qform_pickup5_update_bf_mvdr_w(wtk_qform_pickup5_t *qform_pickup5,int k, int chn)
{
	int i;
	wtk_complex_t *ncov=qform_pickup5->ncov[chn][k];
	wtk_complex_t tmp2[12];
	wtk_complex_t *w=qform_pickup5->w[chn][k];
    wtk_complex_t *ovec=qform_pickup5->ovec[chn][k];
	float ff,fa;
	wtk_complex_t *a;
    int ret;
    wtk_complex_t wtmp[CHANNEL];

	ret=wtk_complex_guass_elimination_p1_f(ncov, ovec, tmp2, CHANNEL, wtmp);
    if(ret!=0)
    {
        return;
    }
    memcpy(w,wtmp,sizeof(wtk_complex_t)*CHANNEL);   

	fa=0;
	a=w;
	for(i=0;i<CHANNEL;++i,++ovec,++a)
	{
		fa+=ovec->a*a->a+ovec->b*a->b;
	}
	ff=1.0/fa;
	for(i=0;i<CHANNEL;++i,++w)
	{
		w->a*=ff;
		w->b*=ff;
	}
}

void wtk_qform_pickup5_notify_data(wtk_qform_pickup5_t *qform_pickup5,float data[][LWINDOW],int len, int is_end)
{
    short *out_data=qform_pickup5->out_data;
    int i,k;

    for(k=0;k<OUT_CHANNEL;++k){
        qform_pickup5->memX[k]=wtk_preemph_asis2(data[k],len,qform_pickup5->memX[k]);
        for(i=0;i<len;++i)
        {
            if(k==0){
                data[k][i]*=WAVSCALE*3;
            }else{
                data[k][i]*=WAVSCALE;
            }
            if(fabs(data[k][i])<1.0f)
            {
                out_data[i*2+k]=data[k][i]*32767.f;
            }else
            {
                if(data[k][i]>0.f)
                {
                    out_data[i*2+k]=32767.f;
                }else
                {
                    out_data[i*2+k]=-32767.f;
                }
            }
        }
    }

    if(qform_pickup5->notify)
    {
        qform_pickup5->notify(qform_pickup5->ths,out_data,len*OUT_CHANNEL,is_end);
    }
}


void wtk_qform_pickup5_start_gccovec(wtk_qform_pickup5_t *qform_pickup5, float theta, int ang_idx)
{
    wtk_complex_t **ovec=qform_pickup5->gcc_ovec[ang_idx];
	float x,y,z;
	float t;
	float *mic;
	int i,j;
	float *tdoa;
	wtk_complex_t *ovec1;
    float ff;

    theta*=PI/180;
    x=cos(theta);
    y=sin(theta);
    z=0;

	tdoa=(float *)wtk_malloc(CHANNEL2*sizeof(float));
	for(i=0;i<CHANNEL2;++i)
	{
		mic=qform_pickup5->micclass_pos[i];
		tdoa[i]=(mic[0]*x+mic[1]*y+mic[2]*z)/SPEED;
	}
    ff=1.0/sqrtf(CHANNEL2);
    for(i=1;i<(NBIN-1);++i)
    {
        ovec1=ovec[i];
        t=2*PI*RATE*1.0f/LWINDOW*i;
        for(j=0;j<CHANNEL2;++j)
        {
            ovec1[j].a=cos(t*tdoa[j])*ff;
            ovec1[j].b=sin(t*tdoa[j])*ff;
        }
    }
	wtk_free(tdoa);
}


void wtk_qform_pickup5_start_ovec(wtk_qform_pickup5_t *qform_pickup5, float theta, float phi, int chn)
{
    wtk_complex_t **ovec=qform_pickup5->ovec[chn];
	float x,y,z;
	float t;
	float *mic;
	int i,j;
	float *tdoa;
	wtk_complex_t *ovec1;
    float ff;

    phi*=PI/180;
    theta*=PI/180;
    x=cos(phi)*cos(theta);
    y=cos(phi)*sin(theta);
    z=sin(phi);

	tdoa=(float *)wtk_malloc(CHANNEL*sizeof(float));
	for(i=0;i<CHANNEL;++i)
	{
		mic=qform_pickup5->mic_pos[i];
		tdoa[i]=(mic[0]*x+mic[1]*y+mic[2]*z)/SPEED;
	}
	// print_float(tdoa,CHANNEL);
    ff=1.0/sqrtf(CHANNEL);
	for(i=1;i<(NBIN-1);++i)
	{
		ovec1=ovec[i];
		t=2*PI*RATE*1.0/LWINDOW*i;
		for(j=0;j<CHANNEL;++j)
		{
			ovec1[j].a=cos(t*tdoa[j])*ff;
			ovec1[j].b=sin(t*tdoa[j])*ff;
		}
	}
	wtk_free(tdoa);
}

void wtk_qform_pickup5_start(wtk_qform_pickup5_t *qform_pickup5)
{
    int i;
    wtk_qform_pickup5_start_ovec(qform_pickup5, 100, 80, 0);
    wtk_qform_pickup5_start_ovec(qform_pickup5, 100, 30, 1);
    for(i=0;i<OUT_CHANNEL;++i){
        wtk_complex_cpy_p2(qform_pickup5->w[i], qform_pickup5->ovec[i], NBIN, CHANNEL);
    }
    wtk_qform_pickup5_start_gccovec(qform_pickup5, 100, 0);
    wtk_qform_pickup5_start_gccovec(qform_pickup5, 100-2*THETA_RANGE, 1);
    wtk_qform_pickup5_start_gccovec(qform_pickup5, 100+2*THETA_RANGE, 2);
    wtk_qform_pickup5_start_gccovec(qform_pickup5, 110, 3);
    wtk_qform_pickup5_start_gccovec(qform_pickup5, 110-2*THETA_RANGE, 4);
    wtk_qform_pickup5_start_gccovec(qform_pickup5, 110+2*THETA_RANGE, 5);

    wtk_qform_pickup5_start_gccovec(qform_pickup5, 100, 6);
    wtk_qform_pickup5_start_gccovec(qform_pickup5, 100-2*THETA_RANGE, 7);
    wtk_qform_pickup5_start_gccovec(qform_pickup5, 100+2*THETA_RANGE, 8);
    wtk_qform_pickup5_start_gccovec(qform_pickup5, 30, 9);
    wtk_qform_pickup5_start_gccovec(qform_pickup5, 30+2*THETA_RANGE, 10);
}

float wtk_qform_pickup5_mmse_hypergeom_gain(float xx)
{
   int ind;
   float integer, frac;
   float x;
   static const float table[21] = {
      0.82157f, 1.02017f, 1.20461f, 1.37534f, 1.53363f, 1.68092f, 1.81865f,
      1.94811f, 2.07038f, 2.18638f, 2.29688f, 2.40255f, 2.50391f, 2.60144f,
      2.69551f, 2.78647f, 2.87458f, 2.96015f, 3.04333f, 3.12431f, 3.20326f};

      x = xx;
      integer = floor(2*x);
      ind = (int)integer;
     // wtk_debug("ind=%d\n",ind);
      if (ind<0)
      {
         return 1.f;
      }
      if (ind>19)
      {
         return (1.f+0.1296f/x);
      }
      frac = 2.f*x-integer;
      x=((1.f-frac)*table[ind] + frac*table[ind+1])/sqrtf(x+.0001f);
      return x;
}

void wtk_qform_pickup5_mmse_compute_gain_floor(float noise_suppress,float *noise,float *gain_floor,float min_noise,int len)
{
	float noise_floor;
	int i;

	noise_floor=expf(0.2302585f*noise_suppress);
	for(i=0;i<len;++i)
	{
		gain_floor[i]=sqrtf(noise_floor*noise[i])/sqrtf(min_noise+noise[i]);
	}
}

void wtk_qform_pickup5_mmse_flush(wtk_qform_pickup5_t *qform_pickup5, float *io, char *cohv, int chn)
{
	float ps[NM]={0};
    float *noise=qform_pickup5->noise[chn];
	float xpost[NM]={0};
    float prior[NM]={0};
	float *old_ps=qform_pickup5->old_ps[chn];
	float gain[NM]={0};
	float gain_floor[NM]={0};
	float gain2[NM]={0};
	float beta,beta_1,f,f2;
	int i;
    float min_noise=qform_pickup5->min_noise;
	float *pf;
	float zframe,pframe;

	for(i=1;i<MMSE_STEP;++i)
	{
		ps[i]=io[i]*io[i]+io[i+MMSE_STEP]*io[i+MMSE_STEP];
	}
    if(qform_pickup5->bank)
    {
    	wtk_qform_pickup5_mmse_xfilterbank_compute_bank32(qform_pickup5->bank,ps,ps+MMSE_STEP);
    }
    for(i=1;i<MMSE_STEP;++i)
    {
        if(cohv[i] <= 0)
        {
            ++qform_pickup5->noise_frame[chn][i];
            if(qform_pickup5->noise_frame[chn][i]>20000)
            {
                qform_pickup5->noise_frame[chn][i]=20000;
            }
            beta=max(BETA,1.0f/qform_pickup5->noise_frame[chn][i]);
	        beta_1=1.f-beta;
            f=beta_1 * noise[i] + beta*ps[i];
            if(f<0)
            {
                noise[i]=0;
            }else
            {
                noise[i]=f;
            }
        }
    }

    wtk_qform_pickup5_mmse_xfilterbank_compute_bank32(qform_pickup5->bank,noise,noise+MMSE_STEP);
	if(qform_pickup5->nframe==1)
	{
		memcpy(qform_pickup5->old_ps[chn],ps,NM*sizeof(float));
	}
	for(i=1;i<NM;++i)
	{
		f=min_noise+noise[i];
		xpost[i]=ps[i]/f - 1.f;
		if(xpost[i]>100)
		{
			xpost[i]=100;
		}
		f2=old_ps[i]/(old_ps[i]+f);
		beta=0.1f+0.89f*f2*f2;
		prior[i]=beta*max(0,xpost[i])+(1-beta)*old_ps[i]/f;
		if(prior[i]>100.f)
		{
			prior[i]=100.f;
		}
	}
    pf=qform_pickup5->zeta[chn];
    zframe=0;
    for(i=0;i<NBANDS;++i)
    {
        zframe+=pf[i]=0.7f*pf[i]+0.3f*prior[i+MMSE_STEP];
    }
    pframe=0.1f+0.899f*(1.f/(1.f+0.15f/(zframe/NBANDS)));
    for(i=MMSE_STEP;i<NM;++i)
    {
        f=prior[i]/(prior[i]+1);
        beta_1=f*(1.f+xpost[i]);
        beta=wtk_qform_pickup5_mmse_hypergeom_gain(beta_1);
        gain[i]=min(1.f,f*beta);   
        old_ps[i]=0.2f*old_ps[i]+0.8f*gain[i]*gain[i]*ps[i];
        f2=0.199f+0.8f/(1+0.15f/pf[i-MMSE_STEP]);
        f2=1.f-pframe*f2;
        gain2[i]=1.f/(1.f+(f2/(1.f-f2))*(1.f+prior[i])*expf(-beta_1));
    }
    wtk_qform_pickup5_mmse_xfilterbank_compute_psd16(qform_pickup5->bank,gain2+MMSE_STEP,gain2);
    wtk_qform_pickup5_mmse_xfilterbank_compute_psd16(qform_pickup5->bank,gain+MMSE_STEP,gain);

    wtk_qform_pickup5_mmse_compute_gain_floor(NOISE_SUPPRESS,noise+MMSE_STEP,gain_floor+MMSE_STEP,min_noise,NBANDS);
    wtk_qform_pickup5_mmse_xfilterbank_compute_psd16(qform_pickup5->bank,gain_floor+MMSE_STEP,gain_floor);

    for(i=1;i<MMSE_STEP;++i)
    {
        f=prior[i]/(prior[i]+1.f);
        beta_1=f*(1.f+xpost[i]);
        beta=wtk_qform_pickup5_mmse_hypergeom_gain(beta_1);
        f2=min(1.f,f*beta);
        if(gain[i]<0.333f*f2)
        {
            f2=3.f*gain[i];
        }
        gain[i]=f2;
        old_ps[i]=0.2f*old_ps[i]+0.8f*gain[i]*gain[i]*ps[i];

        gain[i]=max(gain_floor[i],gain[i]);
        f=gain2[i]*sqrtf(gain[i])+(1.f-gain2[i])*sqrtf(gain_floor[i]);
        gain[i]=f*f;
        // old_ps[i]=0.2f*old_ps[i]+0.8f*gain[i]*gain[i]*ps[i];

        io[i]*=gain[i];
        io[i+MMSE_STEP]*=gain[i];
    }
}

void wtk_qform_pickup5_feed_frame(wtk_qform_pickup5_t *qform_pickup5, int pos, int is_end)
{
    wtk_rfft_t *rfft=qform_pickup5->rfft;
    int i, j;
    float *fp;
    float **input=qform_pickup5->input;
    float *analysis_win=qform_pickup5->analysis_win;
    float *synthesis_win=qform_pickup5->synthesis_win;
    float tm_tmp[OUT_CHANNEL][LWINDOW]={0};
    float fft_tmp[OUT_CHANNEL][LWINDOW]={0};
    wtk_complex_t fft[NBIN][CHANNEL], *fft1, *fft2, *fft3;
    int k;
    wtk_complex_t ***gcc_ovec=qform_pickup5->gcc_ovec;
    wtk_complex_t *gccovertmp,*gccovertmp2,*gccovertmp3,*gccovertmp4,*gccovertmp5;
    wtk_complex_t *gccovertmp6,*gccovertmp7,*gccovertmp8,*gccovertmp9,*gccovertmp10, *gccovertmp11;
    wtk_complex_t coh1,coh2,coh3,coh4,coh5,coh6,coh7,coh8;
    wtk_complex_t coh9,coh10,coh11;
    wtk_complex_t **w=qform_pickup5->w[0], *w1;
    wtk_complex_t **w2=qform_pickup5->w[1], *w3;
    char cohv[OUT_CHANNEL][NBIN];
    float spec_k1[CLASSCNT], spec_k2[CLASSCNT], spec_k3[CLASSCNT], spec_k4[CLASSCNT], spec_k5[CLASSCNT], spec_k6[CLASSCNT];
    float ta,tb,ta2,tb2;
    float output[OUT_CHANNEL][LWINDOW]={0};
    float *pad=qform_pickup5->pad[0];
    float *pad2=qform_pickup5->pad[1];

    ++qform_pickup5->nframe;

    for(i=0; i<CHANNEL; ++i)
    {
        fp=input[i];
        wtk_preemph_dc(fp+qform_pickup5->start_pos, qform_pickup5->notch_mem[i], pos-qform_pickup5->start_pos);
        qform_pickup5->memD[i]=wtk_preemph_asis(fp+qform_pickup5->start_pos, pos-qform_pickup5->start_pos, qform_pickup5->memD[i]);
        for(j=0; j<LWINDOW; ++j)
        {
            tm_tmp[0][j] = fp[j] * analysis_win[j];
            tm_tmp[1][j] = tm_tmp[0][j];
        }
        wtk_rfft_process_fft(rfft,fft_tmp[0],tm_tmp[0]);

        fft[0][i].a=fft_tmp[0][0];
        fft[0][i].b=0;
        for(j=1;j<(NBIN-1);++j)
        {
            fft[j][i].a=fft_tmp[0][j];
            fft[j][i].b=-fft_tmp[0][j+NBIN-1];
        }
        fft[NBIN-1][i].a=fft_tmp[0][NBIN-1];
        fft[NBIN-1][i].b=0;
    }

    for(k=1;k<(NBIN-1);++k)
    {
        fft1=fft[k];

        coh1.a=coh1.b=0;
        coh2.a=coh2.b=0;
        coh3.a=coh3.b=0;
        coh4.a=coh4.b=0;
        coh5.a=coh5.b=0;
        coh6.a=coh6.b=0;

        coh7.a=coh7.b=0;
        coh8.a=coh8.b=0;
        coh9.a=coh9.b=0;
        coh10.a=coh10.b=0;
        coh11.a=coh11.b=0;

        gccovertmp=gcc_ovec[0][k];
        gccovertmp2=gcc_ovec[1][k];
        gccovertmp3=gcc_ovec[2][k];
        gccovertmp4=gcc_ovec[3][k];
        gccovertmp5=gcc_ovec[4][k];
        gccovertmp6=gcc_ovec[5][k];

        gccovertmp7=gcc_ovec[6][k];
        gccovertmp8=gcc_ovec[7][k];
        gccovertmp9=gcc_ovec[8][k];
        gccovertmp10=gcc_ovec[9][k];
        gccovertmp11=gcc_ovec[10][k];

        for(i=0;i<CHANNEL2;++i)
        {
            fft2=fft1+qform_pickup5->mic_class[0][i];
            fft3=fft1+qform_pickup5->mic_class[1][i];

            coh1.a+=fft2->a*gccovertmp->a+fft2->b*gccovertmp->b;
            coh1.b+=-fft2->a*gccovertmp->b+fft2->b*gccovertmp->a;
            coh2.a+=fft2->a*gccovertmp2->a+fft2->b*gccovertmp2->b;
            coh2.b+=-fft2->a*gccovertmp2->b+fft2->b*gccovertmp2->a;
            coh3.a+=fft2->a*gccovertmp3->a+fft2->b*gccovertmp3->b;
            coh3.b+=-fft2->a*gccovertmp3->b+fft2->b*gccovertmp3->a;

            coh4.a+=fft3->a*gccovertmp4->a+fft3->b*gccovertmp4->b;
            coh4.b+=-fft3->a*gccovertmp4->b+fft3->b*gccovertmp4->a;
            coh5.a+=fft3->a*gccovertmp5->a+fft3->b*gccovertmp5->b;
            coh5.b+=-fft3->a*gccovertmp5->b+fft3->b*gccovertmp5->a;
            coh6.a+=fft3->a*gccovertmp6->a+fft3->b*gccovertmp6->b;
            coh6.b+=-fft3->a*gccovertmp6->b+fft3->b*gccovertmp6->a;

            coh7.a+=fft2->a*gccovertmp7->a+fft2->b*gccovertmp7->b;
            coh7.b+=-fft2->a*gccovertmp7->b+fft2->b*gccovertmp7->a;
            coh8.a+=fft2->a*gccovertmp8->a+fft2->b*gccovertmp8->b;
            coh8.b+=-fft2->a*gccovertmp8->b+fft2->b*gccovertmp8->a;
            coh9.a+=fft2->a*gccovertmp9->a+fft2->b*gccovertmp9->b;
            coh9.b+=-fft2->a*gccovertmp9->b+fft2->b*gccovertmp9->a;

            coh10.a+=fft3->a*gccovertmp10->a+fft3->b*gccovertmp10->b;
            coh10.b+=-fft3->a*gccovertmp10->b+fft3->b*gccovertmp10->a;
            coh11.a+=fft3->a*gccovertmp11->a+fft3->b*gccovertmp11->b;
            coh11.b+=-fft3->a*gccovertmp11->b+fft3->b*gccovertmp11->a;  

            ++gccovertmp;
            ++gccovertmp2;
            ++gccovertmp3;
            ++gccovertmp4;
            ++gccovertmp5;
            ++gccovertmp6;
            ++gccovertmp7;
            ++gccovertmp8;
            ++gccovertmp9;
            ++gccovertmp10;
            ++gccovertmp11;
        }
        // wtk_debug("%f+%fi, %f\n",coh.a,coh.b,sqrt(f*f2));

        spec_k1[0]=coh1.a*coh1.a+coh1.b*coh1.b;
        spec_k2[0]=coh2.a*coh2.a+coh2.b*coh2.b;
        spec_k3[0]=coh3.a*coh3.a+coh3.b*coh3.b;

        spec_k1[1]=coh4.a*coh4.a+coh4.b*coh4.b;
        spec_k2[1]=coh5.a*coh5.a+coh5.b*coh5.b;
        spec_k3[1]=coh6.a*coh6.a+coh6.b*coh6.b;

        spec_k4[0]=coh7.a*coh7.a+coh7.b*coh7.b;
        spec_k5[0]=coh8.a*coh8.a+coh8.b*coh8.b;
        spec_k6[0]=coh9.a*coh9.a+coh9.b*coh9.b;
        
        spec_k4[1]=coh10.a*coh10.a+coh10.b*coh10.b;
        spec_k5[1]=coh11.a*coh11.a+coh11.b*coh11.b;

        cohv[0][k]=-1;
        cohv[1][k]=-1;
        // if(spec_k1[0] - spec_k2[0] >= COHV_THRESH && spec_k1[0] - spec_k3[0] >= COHV_THRESH && spec_k1[1] - spec_k2[1] >= COHV_THRESH && spec_k1[1] - spec_k3[1]>= COHV_THRESH)
        if(spec_k1[0] - spec_k2[0] >= COHV_THRESH && spec_k1[0] - spec_k3[0] >= COHV_THRESH && spec_k1[1] - spec_k2[1] >= COHV_THRESH && spec_k1[1] - spec_k3[1]>= COHV_THRESH)
        {
            cohv[0][k]=1;
        }
        if(spec_k4[0] - spec_k5[0] >= COHV_THRESH && spec_k4[0] - spec_k6[0] >= COHV_THRESH && spec_k4[1] - spec_k5[1] >= COHV_THRESH)
        {
            cohv[1][k]=1;
        }
        if(cohv[0][k]<0)
        {
            wtk_qform_pickup5_update_rnn1(qform_pickup5,fft[k],k,0);
        }
        if(cohv[1][k]<0){
            wtk_qform_pickup5_update_rnn1(qform_pickup5,fft[k],k,1);
        }
        if(qform_pickup5->ncnt_sum[0][k]>5)
        {
            wtk_qform_pickup5_update_bf_mvdr_w(qform_pickup5, k, 0);
        }
        if(qform_pickup5->ncnt_sum[1][k]>5)
        {
            wtk_qform_pickup5_update_bf_mvdr_w(qform_pickup5, k, 1);
        }
        ta=tb=0;
        ta2=tb2=0;
        w1=w[k];
        w3=w2[k];
        fft1=fft[k];
        for(i=0;i<CHANNEL;++i,++fft1,++w1,++w3)
        {
            ta+=w1->a*fft1->a + w1->b*fft1->b;
            tb+=w1->a*fft1->b - w1->b*fft1->a;
            ta2+=w3->a*fft1->a + w3->b*fft1->b;
            tb2+=w3->a*fft1->b - w3->b*fft1->a;
        }
        fft_tmp[0][k]=ta;
        fft_tmp[0][k+NBIN-1]=-tb;
        fft_tmp[1][k]=ta2;
        fft_tmp[1][k+NBIN-1]=-tb2;
        // if(cohv[k]>0)
        // {
        //     fft_tmp[k]=fft[k][0].a;
        //     fft_tmp[k+NBIN-1]=-fft[k][0].b;
        // }else
        // {
        //     fft_tmp[k]=0;
        //     fft_tmp[k+NBIN-1]=0;
        // }
    }
    fft_tmp[0][0]=0;
	fft_tmp[0][NBIN-1]=0;
    fft_tmp[1][0]=0;
	fft_tmp[1][NBIN-1]=0;
    wtk_qform_pickup5_mmse_flush(qform_pickup5, fft_tmp[0], cohv[0], 0);
    wtk_qform_pickup5_mmse_flush(qform_pickup5, fft_tmp[1], cohv[1], 1);
	wtk_rfft_process_ifft(rfft,fft_tmp[0],tm_tmp[0]);
	wtk_rfft_process_ifft(rfft,fft_tmp[1],tm_tmp[1]);
    for (i=0; i<pos; ++i)
    {
        output[0][i]=pad[i]+tm_tmp[0][i] * synthesis_win[i];
        output[1][i]=pad2[i]+tm_tmp[1][i] * synthesis_win[i];
    }
    memcpy(pad,  output[0]+FRAME_STEP, sizeof(float) * (LWINDOW-FRAME_STEP));
    memcpy(pad2,  output[1]+FRAME_STEP, sizeof(float) * (LWINDOW-FRAME_STEP));

    if(qform_pickup5->notify)
    {
        wtk_qform_pickup5_notify_data(qform_pickup5,output,is_end?pos:FRAME_STEP,is_end);
    }
}

void wtk_qform_pickup5_feed(wtk_qform_pickup5_t *qform_pickup5,short **data,int len,int is_end)
{
#ifdef DEBUG_WAV
	static wtk_wavfile_t *mic_log=NULL;

	if(!mic_log)
	{
		mic_log=wtk_wavfile_new(16000);
		wtk_wavfile_set_channel(mic_log,CHANNEL);
		wtk_wavfile_open2(mic_log,"qform_pickup5");
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

    int step, step2;
    int i, j;
    float **input=qform_pickup5->input;
    float *fp;
    short *mic;

    step2=0;
    while(len > 0)
    {
        step=min(len, LWINDOW-qform_pickup5->pos);
        for(i=0; i<CHANNEL; ++i)
        {
            fp=input[i];
            mic=data[i]+step2;
            for(j=0; j<step; ++j)
            {
                fp[qform_pickup5->pos+j]=mic[j]/32768.0;
            }
            mic+=step;
        }
        step2+=step;
        qform_pickup5->pos+=step;
        if(qform_pickup5->pos == LWINDOW)
        {
            wtk_qform_pickup5_feed_frame(qform_pickup5, LWINDOW, 0);
            for(i=0; i<CHANNEL; ++i)
            {
                memmove(input[i], input[i]+FRAME_STEP, sizeof(float) * (LWINDOW-FRAME_STEP));
            }
            qform_pickup5->pos-=FRAME_STEP;
            qform_pickup5->start_pos = qform_pickup5->pos;
        }
        len -= step;
    }

    if (is_end)
    {
        if(qform_pickup5->pos > 0)
        {
            step=LWINDOW-qform_pickup5->pos;
            for(i=0; i<CHANNEL; ++i)
            {
                memset(input[i]+qform_pickup5->pos, 0, sizeof(float)*step);
            }
            wtk_qform_pickup5_feed_frame(qform_pickup5, qform_pickup5->pos, 1);
        }else
        {
            if(qform_pickup5->notify)
            {
                qform_pickup5->notify(qform_pickup5->ths, NULL, 0, 1);
            }  
        }
    }
}