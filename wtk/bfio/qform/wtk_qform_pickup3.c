#include "wtk_qform_pickup3.h" 


#define qform_pickup3_mmse_tobank(n)   (13.1f*atan(.00074f*(n))+2.24f*atan((n)*(n)*1.85e-8f)+1e-4f*(n))

wtk_qform_pickup3_mmse_xfilterbank_t* wtk_qform_pickup3_mmse_xfilterbank_new()
{
	wtk_qform_pickup3_mmse_xfilterbank_t *bank;
	float df;
	float max_mel, mel_interval;
   int i;
   int id1;
   int id2;
   float curr_freq;
   float mel;
   float val;

   bank = (wtk_qform_pickup3_mmse_xfilterbank_t*)wtk_malloc(sizeof(wtk_qform_pickup3_mmse_xfilterbank_t));
   df =RATE*1.0f/(2*MMSE_STEP);
   max_mel = qform_pickup3_mmse_tobank(RATE/2);
   mel_interval =max_mel/(NBANDS-1);
   bank->bank_left = (int*)wtk_malloc(MMSE_STEP*sizeof(int));
   bank->bank_right = (int*)wtk_malloc(MMSE_STEP*sizeof(int));
   bank->filter_left = (float*)wtk_malloc(MMSE_STEP*sizeof(float));
   bank->filter_right = (float*)wtk_malloc(MMSE_STEP*sizeof(float));
   for (i=0;i<MMSE_STEP;i++)
   {
      curr_freq = i*df;
      mel = qform_pickup3_mmse_tobank(curr_freq);
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

void wtk_qform_pickup3_mmse_xfilterbank_delete(wtk_qform_pickup3_mmse_xfilterbank_t *bank)
{
	   wtk_free(bank->bank_left);
	   wtk_free(bank->bank_right);
	   wtk_free(bank->filter_left);
	   wtk_free(bank->filter_right);
	   wtk_free(bank);
}


void wtk_qform_pickup3_mmse_xfilterbank_compute_bank32(wtk_qform_pickup3_mmse_xfilterbank_t *bank, float *ps, float *mel)
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


void wtk_qform_pickup3_mmse_xfilterbank_compute_psd16(wtk_qform_pickup3_mmse_xfilterbank_t *bank,float *mel,float *ps)
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



float *wtk_qform_pickup3_analysiswin_new(int win)
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

float *wtk_qform_pickup3_synthesiswin_new(float *win, int len, int step)
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


wtk_qform_pickup3_t* wtk_qform_pickup3_new()
{
    wtk_qform_pickup3_t *qform_pickup3;

    qform_pickup3=(wtk_qform_pickup3_t *)wtk_malloc(sizeof(wtk_qform_pickup3_t));
    qform_pickup3->ths=NULL;
    qform_pickup3->notify=NULL;
    

    qform_pickup3->mic_class[0][0]=0;
    qform_pickup3->mic_class[0][1]=1;
    qform_pickup3->mic_class[1][0]=2;
    qform_pickup3->mic_class[1][1]=1;

    qform_pickup3->micclass_pos[0][0]=0.021f;
    qform_pickup3->micclass_pos[0][1]=0.0f;
    qform_pickup3->micclass_pos[0][2]=0.0f;
    qform_pickup3->micclass_pos[1][0]=-0.021f;
    qform_pickup3->micclass_pos[1][1]=0.0f;
    qform_pickup3->micclass_pos[1][2]=0.0f;

    qform_pickup3->mic_pos[0][0]=0.021f;
    qform_pickup3->mic_pos[0][1]=0.021f;
    qform_pickup3->mic_pos[0][2]=0.0f;
    qform_pickup3->mic_pos[1][0]=-0.021f;
    qform_pickup3->mic_pos[1][1]=0.021f;
    qform_pickup3->mic_pos[1][2]=0.0f;
    qform_pickup3->mic_pos[2][0]=-0.021f;
    qform_pickup3->mic_pos[2][1]=-0.021f;
    qform_pickup3->mic_pos[2][2]=0.0f;


    qform_pickup3->input=wtk_float_new_p2(CHANNEL, LWINDOW);
    qform_pickup3->analysis_win=wtk_qform_pickup3_analysiswin_new(LWINDOW);
    qform_pickup3->synthesis_win=wtk_qform_pickup3_synthesiswin_new(qform_pickup3->analysis_win, LWINDOW, FRAME_STEP);

	qform_pickup3->rfft=wtk_rfft_new(LWINDOW/2);

    qform_pickup3->gcc_ovec=wtk_complex_new_p3(3,NBIN,CHANNEL2);
    qform_pickup3->ang_num=3;    

    qform_pickup3->ncov=wtk_complex_new_p2(NBIN,CHANNELX2);
    qform_pickup3->ncnt_sum=(float *)wtk_malloc(sizeof(float)*NBIN);

	qform_pickup3->ovec=wtk_complex_new_p2(NBIN,CHANNEL);
    qform_pickup3->w=wtk_complex_new_p2(NBIN,CHANNEL);

	qform_pickup3->pad=(float*)wtk_malloc(LWINDOW*sizeof(float));

    qform_pickup3->bank=wtk_qform_pickup3_mmse_xfilterbank_new();
    qform_pickup3->zeta=(float*)wtk_calloc(NBANDS,sizeof(float));
    qform_pickup3->min_noise=1e-9;
    qform_pickup3->noise_frame=(int*)wtk_calloc(MMSE_STEP,sizeof(int));
    qform_pickup3->noise=(float*)wtk_calloc(NM,sizeof(float));
    qform_pickup3->old_ps=(float*)wtk_calloc(NM,sizeof(float));


    wtk_qform_pickup3_reset(qform_pickup3);

    return qform_pickup3;
}

void wtk_qform_pickup3_delete(wtk_qform_pickup3_t *qform_pickup3)
{
    wtk_float_delete_p2(qform_pickup3->input, CHANNEL);
    wtk_free(qform_pickup3->analysis_win);
    wtk_free(qform_pickup3->synthesis_win);
    wtk_rfft_delete(qform_pickup3->rfft);

    wtk_complex_delete_p3(qform_pickup3->gcc_ovec, qform_pickup3->ang_num, NBIN);

    wtk_free(qform_pickup3->ncnt_sum);
    wtk_complex_delete_p2(qform_pickup3->ncov, NBIN);

    wtk_complex_delete_p2(qform_pickup3->ovec, NBIN);

    wtk_complex_delete_p2(qform_pickup3->w, NBIN);
    wtk_free(qform_pickup3->pad);

    wtk_qform_pickup3_mmse_xfilterbank_delete(qform_pickup3->bank);
    wtk_free(qform_pickup3->zeta);
    wtk_free(qform_pickup3->noise_frame);
    wtk_free(qform_pickup3->noise);
    wtk_free(qform_pickup3->old_ps);

    wtk_free(qform_pickup3);
}

void wtk_qform_pickup3_reset(wtk_qform_pickup3_t *qform_pickup3)
{
    int i;

    for(i=0;i<CHANNEL;++i)
    {
        memset(qform_pickup3->notch_mem[i],0,2*sizeof(float));
    }
	memset(qform_pickup3->memD,0,CHANNEL*sizeof(float));
    qform_pickup3->memX=0;
    wtk_float_zero_p2(qform_pickup3->input, CHANNEL, LWINDOW);

    qform_pickup3->start_pos=0;
    qform_pickup3->pos=0;
    qform_pickup3->nframe=0;

    memset(qform_pickup3->ncnt_sum,0,sizeof(int)*NBIN);
    wtk_complex_zero_p2(qform_pickup3->ncov,NBIN,CHANNELX2);

    wtk_complex_zero_p3(qform_pickup3->gcc_ovec,qform_pickup3->ang_num, NBIN,2);
    wtk_complex_zero_p2(qform_pickup3->ovec,NBIN,CHANNEL);

    wtk_complex_zero_p2(qform_pickup3->w,NBIN,CHANNEL);

    memset(qform_pickup3->pad, 0, sizeof(float)*LWINDOW);

    for(i=0;i<MMSE_STEP;++i)
    {
        qform_pickup3->noise_frame[i]=0;
        qform_pickup3->noise[i]=qform_pickup3->min_noise;
    }

    memset(qform_pickup3->zeta,0,NBANDS*sizeof(float));
    memset(qform_pickup3->old_ps,0,NM*sizeof(float));
}


void wtk_qform_pickup3_set_notify(wtk_qform_pickup3_t *qform_pickup3,void *ths,wtk_qform_pickup3_notify_f notify)
{
    qform_pickup3->ths=ths;
    qform_pickup3->notify=notify;
}


// void wtk_qform_pickup3_update_rnn2(wtk_qform_pickup3_t *qform_pickup3, wtk_complex_t **fft,int k)
// {
//     int CHANNEL=CHANNEL;
//     int i,j;
//     wtk_complex_t *fft1,*fft2,*a,*b;
//     wtk_complex_t **scov=qform_pickup3->scov;
//     // wtk_complex_t **scovtmp=qform_pickup3->scovtmp;
//     float alpha=qform_pickup3->qform_pickup3->scov_alpha;
//     float alpha_1=1-qform_pickup3->qform_pickup3->scov_alpha;

//     if(qform_pickup3->scnt_sum[k]>=qform_pickup3->qform_pickup3->init_scovnf)
//     {
//         ++qform_pickup3->scnt_sum[k];
//         fft1=fft2=fft[k];
//         for(i=0;i<CHANNEL;++i,++fft1)
//         {
//             fft2=fft1;
//             for(j=i;j<CHANNEL;++j,++fft2)
//             {
//                 a=scov[k]+i*CHANNEL+j;
//                 a->a=alpha_1*a->a+alpha*(fft1->a*fft2->a+fft1->b*fft2->b);
//                 a->b=alpha_1*a->b+alpha*(-fft1->a*fft2->b+fft1->b*fft2->a);

//                 if(i!=j)
//                 {
//                     b=scov[k]+j*CHANNEL+i;
//                     b->a=a->a;
//                     b->b=-a->b;
//                 }
//             }
//         }
//     }else
//     {
//         ++qform_pickup3->scnt_sum[k];
//         fft1=fft2=fft[k];
//         for(i=0;i<CHANNEL;++i,++fft1)
//         {
//             fft2=fft1;
//             for(j=i;j<CHANNEL;++j,++fft2)
//             {
//                 a=scov[k]+i*CHANNEL+j;
//                 a->a+=fft1->a*fft2->a+fft1->b*fft2->b;
//                 a->b+=-fft1->a*fft2->b+fft1->b*fft2->a;

//                 // if(qform_pickup3->scnt_sum[k] == qform_pickup3->qform_pickup3->init_scovnf)
//                 // {
//                 //     a->a/=qform_pickup3->qform_pickup3->init_scovnf;
//                 //     a->b/=qform_pickup3->qform_pickup3->init_scovnf;
//                 // }

//                 if(i!=j)
//                 {
//                     b=scov[k]+j*CHANNEL+i;
//                     b->a=a->a;
//                     b->b=-a->b;
//                 }

//             }
//         }
//     }
// }


void wtk_qform_pickup3_update_rnn1(wtk_qform_pickup3_t *qform_pickup3,wtk_complex_t *fft,int k)
{
    int i,j;
    wtk_complex_t *fft1,*fft2,*a,*b;
    wtk_complex_t **ncov=qform_pickup3->ncov;
    // wtk_complex_t **ncovtmp=qform_pickup3->ncovtmp;

    if(qform_pickup3->ncnt_sum[k]>=INIT_NCOVNF)
    {
        ++qform_pickup3->ncnt_sum[k];
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
        ++qform_pickup3->ncnt_sum[k];
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
        if(qform_pickup3->ncnt_sum[k]==INIT_NCOVNF)
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

void wtk_qform_pickup3_flush_gccspec_k(wtk_qform_pickup3_t *qform_pickup3, float cov_travg, wtk_complex_t *covclass,
                                                                                                                 float cov_travg2, wtk_complex_t *covclass2, int k, float *spec_k, int ang_idx)
{
    wtk_complex_t *a,*b,*c,*a2;
    float fa,fb,ff,fa2,fb2,ff2,f;
    wtk_complex_t *ovec=qform_pickup3->gcc_ovec[ang_idx][k];
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


void wtk_qform_pickup3_update_bf_mvdr_w(wtk_qform_pickup3_t *qform_pickup3,int k)
{
	int i;
	wtk_complex_t *ncov=qform_pickup3->ncov[k];
	wtk_complex_t tmp2[12];
	wtk_complex_t *w=qform_pickup3->w[k];
    wtk_complex_t *ovec=qform_pickup3->ovec[k];
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

void wtk_qform_pickup3_notify_data(wtk_qform_pickup3_t *qform_pickup3,float *data,int len, int is_end)
{
    short *pv=(short *)data;
    int i;

    qform_pickup3->memX=wtk_preemph_asis2(data,len,qform_pickup3->memX);
    for(i=0;i<len;++i)
    {
        data[i]*=WAVSCALE;
        if(fabs(data[i])<1.0f)
        {
            pv[i]=data[i]*32767.f;
        }else
        {
            if(data[i]>0.f)
            {
                pv[i]=32767.f;
            }else
            {
                pv[i]=-32767.f;
            }
        }
    }

    if(qform_pickup3->notify)
    {
        qform_pickup3->notify(qform_pickup3->ths,pv,len,is_end);
    }
}


void wtk_qform_pickup3_start_gccovec(wtk_qform_pickup3_t *qform_pickup3, float theta, int ang_idx)
{
    wtk_complex_t **ovec=qform_pickup3->gcc_ovec[ang_idx];
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
		mic=qform_pickup3->micclass_pos[i];
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


void wtk_qform_pickup3_start_ovec(wtk_qform_pickup3_t *qform_pickup3, float theta, float phi)
{
    wtk_complex_t **ovec=qform_pickup3->ovec;
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
		mic=qform_pickup3->mic_pos[i];
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

void wtk_qform_pickup3_start(wtk_qform_pickup3_t *qform_pickup3)
{
    wtk_qform_pickup3_start_ovec(qform_pickup3, 90, 90);
    wtk_complex_cpy_p2(qform_pickup3->w, qform_pickup3->ovec, NBIN, CHANNEL);

    wtk_qform_pickup3_start_gccovec(qform_pickup3, 90, 0);
    wtk_qform_pickup3_start_gccovec(qform_pickup3, 90-2*THETA_RANGE, 1);
    wtk_qform_pickup3_start_gccovec(qform_pickup3, 90+2*THETA_RANGE, 2);
}

float wtk_qform_pickup3_mmse_hypergeom_gain(float xx)
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

void wtk_qform_pickup3_mmse_compute_gain_floor(float noise_suppress,float *noise,float *gain_floor,float min_noise,int len)
{
	float noise_floor;
	int i;

	noise_floor=expf(0.2302585f*noise_suppress);
	for(i=0;i<len;++i)
	{
		gain_floor[i]=sqrtf(noise_floor*noise[i])/sqrtf(min_noise+noise[i]);
	}
}

void wtk_qform_pickup3_mmse_flush(wtk_qform_pickup3_t *qform_pickup3, float *io, char *cohv)
{
	float ps[NM]={0};
    float *noise=qform_pickup3->noise;
	float xpost[NM]={0};
    float prior[NM]={0};
	float *old_ps=qform_pickup3->old_ps;
	float gain[NM]={0};
	float gain_floor[NM]={0};
	float gain2[NM]={0};
	float beta,beta_1,f,f2;
	int i;
    float min_noise=qform_pickup3->min_noise;
	float *pf;
	float zframe,pframe;

	for(i=1;i<MMSE_STEP;++i)
	{
		ps[i]=io[i]*io[i]+io[i+MMSE_STEP]*io[i+MMSE_STEP];
	}
    if(qform_pickup3->bank)
    {
    	wtk_qform_pickup3_mmse_xfilterbank_compute_bank32(qform_pickup3->bank,ps,ps+MMSE_STEP);
    }
    for(i=1;i<MMSE_STEP;++i)
    {
        if(cohv[i] <= 0)
        {
            ++qform_pickup3->noise_frame[i];
            if(qform_pickup3->noise_frame[i]>20000)
            {
                qform_pickup3->noise_frame[i]=20000;
            }
            beta=max(BETA,1.0f/qform_pickup3->noise_frame[i]);
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

    wtk_qform_pickup3_mmse_xfilterbank_compute_bank32(qform_pickup3->bank,noise,noise+MMSE_STEP);
	if(qform_pickup3->nframe==1)
	{
		memcpy(qform_pickup3->old_ps,ps,NM*sizeof(float));
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
    pf=qform_pickup3->zeta;
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
        beta=wtk_qform_pickup3_mmse_hypergeom_gain(beta_1);
        gain[i]=min(1.f,f*beta);   
        old_ps[i]=0.2f*old_ps[i]+0.8f*gain[i]*gain[i]*ps[i];
        f2=0.199f+0.8f/(1+0.15f/pf[i-MMSE_STEP]);
        f2=1.f-pframe*f2;
        gain2[i]=1.f/(1.f+(f2/(1.f-f2))*(1.f+prior[i])*expf(-beta_1));
    }
    wtk_qform_pickup3_mmse_xfilterbank_compute_psd16(qform_pickup3->bank,gain2+MMSE_STEP,gain2);
    wtk_qform_pickup3_mmse_xfilterbank_compute_psd16(qform_pickup3->bank,gain+MMSE_STEP,gain);

    wtk_qform_pickup3_mmse_compute_gain_floor(NOISE_SUPPRESS,noise+MMSE_STEP,gain_floor+MMSE_STEP,min_noise,NBANDS);
    wtk_qform_pickup3_mmse_xfilterbank_compute_psd16(qform_pickup3->bank,gain_floor+MMSE_STEP,gain_floor);

    for(i=1;i<MMSE_STEP;++i)
    {
        f=prior[i]/(prior[i]+1.f);
        beta_1=f*(1.f+xpost[i]);
        beta=wtk_qform_pickup3_mmse_hypergeom_gain(beta_1);
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

void wtk_qform_pickup3_feed_frame(wtk_qform_pickup3_t *qform_pickup3, int pos, int is_end)
{
    wtk_rfft_t *rfft=qform_pickup3->rfft;
    int i, j;
    float *fp;
    float **input=qform_pickup3->input;
    float *analysis_win=qform_pickup3->analysis_win;
    float *synthesis_win=qform_pickup3->synthesis_win;
    float tm_tmp[LWINDOW]={0};
    float fft_tmp[LWINDOW]={0};
    wtk_complex_t fft[NBIN][CHANNEL], *fft1, *fft2, *fft3;
    int k;
    wtk_complex_t ***gcc_ovec=qform_pickup3->gcc_ovec;
    wtk_complex_t *gccovertmp,*gccovertmp2,*gccovertmp3;
    wtk_complex_t coh1,coh2,coh3,coh4,coh5,coh6;
    wtk_complex_t **w=qform_pickup3->w, *w1;
    char cohv[NBIN];
    float spec_k1[CLASSCNT], spec_k2[CLASSCNT], spec_k3[CLASSCNT];
    float ta,tb;
    float output[LWINDOW]={0};
    float *pad=qform_pickup3->pad;

    ++qform_pickup3->nframe;

    for(i=0; i<CHANNEL; ++i)
    {
        fp=input[i];
        wtk_preemph_dc(fp+qform_pickup3->start_pos, qform_pickup3->notch_mem[i], pos-qform_pickup3->start_pos);
        qform_pickup3->memD[i]=wtk_preemph_asis(fp+qform_pickup3->start_pos, pos-qform_pickup3->start_pos, qform_pickup3->memD[i]);
        for(j=0; j<LWINDOW; ++j)
        {
            tm_tmp[j] = fp[j] * analysis_win[j];
        }
        wtk_rfft_process_fft(rfft,fft_tmp,tm_tmp);

        fft[0][i].a=fft_tmp[0];
        fft[0][i].b=0;
        for(j=1;j<(NBIN-1);++j)
        {
            fft[j][i].a=fft_tmp[j];
            fft[j][i].b=-fft_tmp[j+NBIN-1];
        }
        fft[NBIN-1][i].a=fft_tmp[NBIN-1];
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
        gccovertmp=gcc_ovec[0][k];
        gccovertmp2=gcc_ovec[1][k];
        gccovertmp3=gcc_ovec[2][k];

        for(i=0;i<CHANNEL2;++i)
        {
            fft2=fft1+qform_pickup3->mic_class[0][i];
            fft3=fft1+qform_pickup3->mic_class[1][i];

            coh1.a+=fft2->a*gccovertmp->a+fft2->b*gccovertmp->b;
            coh1.b+=-fft2->a*gccovertmp->b+fft2->b*gccovertmp->a;
            coh2.a+=fft2->a*gccovertmp2->a+fft2->b*gccovertmp2->b;
            coh2.b+=-fft2->a*gccovertmp2->b+fft2->b*gccovertmp2->a;
            coh3.a+=fft2->a*gccovertmp3->a+fft2->b*gccovertmp3->b;
            coh3.b+=-fft2->a*gccovertmp3->b+fft2->b*gccovertmp3->a;

            coh4.a+=fft3->a*gccovertmp->a+fft3->b*gccovertmp->b;
            coh4.b+=-fft3->a*gccovertmp->b+fft3->b*gccovertmp->a;
            coh5.a+=fft3->a*gccovertmp2->a+fft3->b*gccovertmp2->b;
            coh5.b+=-fft3->a*gccovertmp2->b+fft3->b*gccovertmp2->a;
            coh6.a+=fft3->a*gccovertmp3->a+fft3->b*gccovertmp3->b;
            coh6.b+=-fft3->a*gccovertmp3->b+fft3->b*gccovertmp3->a;

            ++gccovertmp;
            ++gccovertmp2;
            ++gccovertmp3;
        }
        // wtk_debug("%f+%fi, %f\n",coh.a,coh.b,sqrt(f*f2));

        spec_k1[0]=coh1.a*coh1.a+coh1.b*coh1.b;
        spec_k2[0]=coh2.a*coh2.a+coh2.b*coh2.b;
        spec_k3[0]=coh3.a*coh3.a+coh3.b*coh3.b;

        spec_k1[1]=coh4.a*coh4.a+coh4.b*coh4.b;
        spec_k2[1]=coh5.a*coh5.a+coh5.b*coh5.b;
        spec_k3[1]=coh6.a*coh6.a+coh6.b*coh6.b;

        cohv[k]=-1;
        if(spec_k1[0] - spec_k2[0] >= COHV_THRESH && spec_k1[0] - spec_k3[0] >= COHV_THRESH && spec_k1[1] - spec_k2[1] >= COHV_THRESH && spec_k1[1] - spec_k3[1]>= COHV_THRESH )
        {
            cohv[k]=1;
        }
        if(cohv[k]<0)
        {
            wtk_qform_pickup3_update_rnn1(qform_pickup3,fft[k],k);
        }
        if(qform_pickup3->ncnt_sum[k]>5)
        {
            wtk_qform_pickup3_update_bf_mvdr_w(qform_pickup3, k);
        }
        ta=tb=0;
        w1=w[k];
        fft1=fft[k];
        for(i=0;i<CHANNEL;++i,++fft1,++w1)
        {
            ta+=w1->a*fft1->a + w1->b*fft1->b;
            tb+=w1->a*fft1->b - w1->b*fft1->a;
        }
        fft_tmp[k]=ta;
        fft_tmp[k+NBIN-1]=-tb;
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
    fft_tmp[0]=0;
	fft_tmp[NBIN-1]=0;
    wtk_qform_pickup3_mmse_flush(qform_pickup3, fft_tmp, cohv);
	wtk_rfft_process_ifft(rfft,fft_tmp,tm_tmp);
    for (i=0; i<pos; ++i)
    {
        output[i]=pad[i]+tm_tmp[i] * synthesis_win[i];
    }
    memcpy(pad,  output+FRAME_STEP, sizeof(float) * (LWINDOW-FRAME_STEP));

    if(qform_pickup3->notify)
    {
        wtk_qform_pickup3_notify_data(qform_pickup3,output,is_end?pos:FRAME_STEP,is_end);
    }
}

void wtk_qform_pickup3_feed(wtk_qform_pickup3_t *qform_pickup3,short **data,int len,int is_end)
{
#ifdef DEBUG_WAV
	static wtk_wavfile_t *mic_log=NULL;

	if(!mic_log)
	{
		mic_log=wtk_wavfile_new(16000);
		wtk_wavfile_set_channel(mic_log,CHANNEL);
		wtk_wavfile_open2(mic_log,"qform_pickup3");
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
    float **input=qform_pickup3->input;
    float *fp;
    short *mic;

    step2=0;
    while(len > 0)
    {
        step=min(len, LWINDOW-qform_pickup3->pos);
        for(i=0; i<CHANNEL; ++i)
        {
            fp=input[i];
            mic=data[i]+step2;
            for(j=0; j<step; ++j)
            {
                fp[qform_pickup3->pos+j]=mic[j]/32768.0;
            }
            mic+=step;
        }
        step2+=step;
        qform_pickup3->pos+=step;
        if(qform_pickup3->pos == LWINDOW)
        {
            wtk_qform_pickup3_feed_frame(qform_pickup3, LWINDOW, 0);
            for(i=0; i<CHANNEL; ++i)
            {
                memmove(input[i], input[i]+FRAME_STEP, sizeof(float) * (LWINDOW-FRAME_STEP));
            }
            qform_pickup3->pos-=FRAME_STEP;
            qform_pickup3->start_pos = qform_pickup3->pos;
        }
        len -= step;
    }

    if (is_end)
    {
        if(qform_pickup3->pos > 0)
        {
            step=LWINDOW-qform_pickup3->pos;
            for(i=0; i<CHANNEL; ++i)
            {
                memset(input[i]+qform_pickup3->pos, 0, sizeof(float)*step);
            }
            wtk_qform_pickup3_feed_frame(qform_pickup3, qform_pickup3->pos, 1);
        }else
        {
            if(qform_pickup3->notify)
            {
                qform_pickup3->notify(qform_pickup3->ths, NULL, 0, 1);
            }  
        }
    }
}