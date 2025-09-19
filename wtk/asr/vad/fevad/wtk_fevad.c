#include "wtk_fevad.h" 
void wtk_fevad_on_stft(wtk_fevad_t *vad,wtk_stft_msg_t *msg,int pos,int is_end);


wtk_vframe_t* wtk_fevad_new_vframe(wtk_fevad_t *vad)
{
	wtk_vframe_t *vf;

	vf=wtk_vframe_new3(vad->frame_size,vad->frame_size);
	vf->sample_data=(float*)wtk_calloc(vad->frame_size,sizeof(float));
	vf->wav_data=(short*)wtk_calloc(vad->frame_size,sizeof(short));
	return vf;
}

#define LOGNX 10

wtk_fevad_t* wtk_fevad_new(wtk_fevad_cfg_t *cfg,void *raise_ths,wtk_vframe_raise_f raise)
{
	wtk_fevad_t *vad;

	vad=(wtk_fevad_t*)wtk_malloc(sizeof(wtk_fevad_t));
	vad->cfg=cfg;
	vad->frame_size=cfg->stft.win;
	vad->stft=wtk_stft_new(&(cfg->stft));
	wtk_stft_set_notify(vad->stft,vad,(wtk_stft_notify_f)wtk_fevad_on_stft);
	vad->raise_ths=raise_ths;
	vad->raise=raise;
	vad->sfms=10*log10(2.718281828);
	wtk_hoard_init(&vad->frame_hoard,offsetof(wtk_vframe_t,hoard_n),100,
			(wtk_new_handler_t)wtk_fevad_new_vframe,(wtk_delete_handler_t)wtk_vframe_delete,vad);
	vad->ths=NULL;
	vad->check_speech=NULL;
	vad->lx=log(LOGNX)*vad->stft->nbin;
	vad->fx=10*log10(2.718281828);
	wtk_fevad_reset(vad);
	return vad;
}

void wtk_fevad_delete(wtk_fevad_t *v)
{
	wtk_hoard_clean(&(v->frame_hoard));
	wtk_stft_delete(v->stft);
	wtk_free(v);
}

void wtk_fevad_reset(wtk_fevad_t *v)
{
	v->notch_mem[0]=0;
	v->notch_mem[1]=0;
	wtk_queue_init(&(v->bak_q));
	v->n_frame_index=0;
	v->state=WTK_FEVAD_INIT;
	v->speech_state=WTK_FEVAD_SIL;
	v->Min_E=1e10;
	v->Min_F=1e10;
	v->Min_SFM=1e10;
	v->idx=0;
	v->sil_cnt=0;
	v->mean_start_sil_e=0;
	v->Thresh_E=0;
	v->Thresh_F=0;
	v->Thresh_SF=0;
	v->Min_Fv=0;
}

void wtk_fevad_set_notify(wtk_fevad_t *vad,void *ths,wtk_fevad_check_speech_f check_speech)
{
	vad->ths=ths;
	vad->check_speech=check_speech;
}

void wtk_fevad_push_vframe(wtk_fevad_t *vad,wtk_vframe_t *vf)
{
	wtk_hoard_push(&(vad->frame_hoard),vf);
}

wtk_vframe_t* wtk_fevad_pop_vframe(wtk_fevad_t *vad,float *p,int n)
{
	wtk_vframe_t *f;
	int i;

	f=wtk_hoard_pop(&(vad->frame_hoard));
	wtk_frame_reset(f);
	f->frame_step=n;
	memcpy(f->sample_data,p,n*sizeof(float));
	for(i=0;i<n;++i)
	{
		f->wav_data[i]=p[i]*32768;
	}
	f->index=++vad->n_frame_index;
	return f;
}

void wtk_fevad_flush_frame_queue(wtk_fevad_t *vad,wtk_queue_t *q)
{
	wtk_queue_node_t *n;
	wtk_vframe_t *f;

	while(1)
	{
		n=wtk_queue_pop(q);
		if(!n){break;}
		f=data_offset(n,wtk_vframe_t,q_n);
		wtk_fevad_push_vframe(vad,f);
	}
}

//static int xinput=0;
//static int xoutput=0;

void wtk_fevad_flush_bak(wtk_fevad_t *vad,int sil)
{
	wtk_queue_node_t *qn;
	wtk_vframe_t* vf;
	wtk_queue_t *q=&(vad->bak_q);

	while(1)
	{
		qn=wtk_queue_pop(q);
		if(!qn){break;}
		vf=data_offset2(qn,wtk_vframe_t,q_n);
		if(sil)
		{
			vf->state=wtk_vframe_sil;
		}else
		{
			vf->state=wtk_vframe_speech;
		}
		vad->raise(vad->raise_ths,vf);
	}
}

#ifndef M_PI_4
#define M_PI_4 (3.1415926535897932384626433832795/4.0)
#endif

float fastatan(float x)
{
    return M_PI_4*x - x*(fabs(x) - 1)*(0.2447 + 0.0663*fabs(x));
}

double FastArcTan(double x) {
  return M_PI_4*x - x*(fabs(x) - 1)*(0.2447 + 0.0663*fabs(x));
}

#define A 0.0776509570923569
#define B -0.287434475393028
#define C (M_PI_4 - A - B)

double Fast2ArcTan(double x) {
  double xx = x * x;
  return ((A*xx + B)*xx + C)*x;
}


#define ATAN fastatan
//#define ATAN atan
//#define ATAN Fast2ArcTan

void wtk_fevad_on_stft2(wtk_fevad_t *vad,wtk_stft_msg_t *msg,int pos,int is_end)
{
	float e;
	float maxf,f,F;
	int maxi;
	int i;
	float *pf=vad->stft->input[0];
	wtk_complex_t *fft;
	int counter;
	int speech;
	wtk_vframe_t* vf;
	int nbin=vad->stft->nbin;

	//nbin=2000*nbin*2/(vad->cfg->rate);
	//wtk_debug("pos=%d is_end=%d\n",pos,is_end);
	if(is_end && pos==0)
	{
		if( vad->bak_q.length>0)
		{
			if(vad->speech_state==WTK_FEVAD_SIL)
			{
				wtk_fevad_flush_bak(vad,1);
			}else
			{
				wtk_fevad_flush_bak(vad,0);
			}
			//exit(0);
		}
		return;
	}
	//xoutput+=pos;
	if(is_end)
	{
		vf=wtk_fevad_pop_vframe(vad,vad->stft->input[0],pos);
	}else
	{
		vf=wtk_fevad_pop_vframe(vad,vad->stft->input[0],vad->frame_size);
	}
	//print_float(vad->stft->input[0],vad->frame_size);
	//exit(0);
	++vad->idx;
	e=0;
	for(i=0;i<vad->stft->pos;++i)
	{
		e+=pf[i]*pf[i];
	}
	fft=msg->fft[0];
	maxf=fft[0].a*fft[0].a+fft[0].b*fft[0].b;
	maxi=0;
	//wtk_debug("F=%f\n",F);
	//wtk_debug("e=%f f=%d/%f\n",e,maxi,f);
	for(i=1;i<nbin;++i)
	{
		//wtk_debug("v[%d]=%f+%fi\n",i,fft[i].a,fft[i].b);
		f=fft[i].a*fft[i].a+fft[i].b*fft[i].b;
		if(f>maxf)
		{
			maxf=f;
			maxi=i;
		}
	}
	//gm.a=(log(gm.a)-log(100)*nbin)/2;
	F=(maxi+1)*vad->cfg->rate/vad->stft->cfg->win;


//	wtk_debug("v[%d,%f]= e=%f/%f/%f f=%f/%f/%f sfm=%f/%f/%f\n",vf->index,vf->index*vf->frame_step*1.0/16000,
//					e,vad->Min_E,vad->Thresh_E,
//					F,vad->Min_F,vad->Thresh_F,
//					sfm,vad->Min_SFM,vad->Thresh_SF);
	switch(vad->state)
	{
	case WTK_FEVAD_INIT:
		///wtk_debug("v[%d,%f]: E=%f F=%f SFM=%f time=%f sil\n",ki,ki*vad->cfg->stft.win*1.0/16000,e,F,sfm,t);
		vad->mean_start_sil_e+=e;
		if(vf->index==1)
		{
			vad->Min_E=e;
			vad->Min_F=F;
		}else
		{
			if(e<vad->Min_E)
			{
				vad->Min_E=e;
			}
			if(F<vad->Min_F)
			{
				vad->Min_F=F;
			}
		}
		if(vad->cfg->debug)
		{
			wtk_debug("v[%d,%f]= e=%f/%f/%f f=%f/%f/%f\n",vf->index,vf->index*vf->frame_step*1.0/vad->cfg->rate,
							e,vad->Min_E,vad->Thresh_E,
							F,vad->Min_F,vad->Thresh_F);
		}
		vf->state=wtk_vframe_sil;
		vad->raise(vad->raise_ths,vf);
		if(vad->idx==vad->cfg->min_start_frame)
		{
			vad->mean_start_sil_e/=vad->cfg->min_start_frame;
			vad->Min_E=vad->mean_start_sil_e;
			//vad->Thresh_E=vad->cfg->e_thresh*log(vad->Min_E*32768*32768)/(32768*32768);
			vad->Thresh_E=vad->cfg->e_thresh*vad->Min_E;//log(vad->Min_E);//*fx1)*fx2;
			//wtk_debug("%f/%f %f/%f\n",vad->Min_E,vad->Thresh_E,vad->Min_E*32768*32768,log(vad->Min_E*32768*32768));
			vad->Thresh_F=vad->cfg->f_thresh;
			vad->Thresh_SF=vad->cfg->sf_thresh;
			vad->sil_cnt=1;
			//wtk_debug("Min_E=%f Min_F=%f Min_SFM=%f Thresh=%f/%f/%f\n",vad->Min_E,vad->Min_F,vad->Min_SFM,vad->Thresh_E,vad->Thresh_F,vad->Thresh_SF);
			vad->state=WTK_FEVAD_UPDATE;
			vad->speech_state=WTK_FEVAD_SIL;
		}
		break;
	case WTK_FEVAD_UPDATE:
		counter=0;
		if((e-vad->Min_E)>=vad->Thresh_E)
		{
			++counter;
		}
		//wtk_debug("%f/%f/%d\n",(F-vad->Min_F),vad->Thresh_F,(F-vad->Min_F)>=vad->Thresh_F);
		if((F-vad->Min_F)>=vad->Thresh_F)
		{
			++counter;
		}
		speech=counter>1?1:0;
		if(speech==0 && e/vad->Min_E<vad->cfg->e_ratio)
		{
			float alpha=vad->cfg->e_alpha;

			//wtk_debug("update min e\n");
			//vad->Min_E=((vad->sil_cnt*vad->Min_E)+e)/(vad->sil_cnt+1);
			vad->Min_E=vad->Min_E*alpha+(1-alpha)*e;
			//vad->mean_start_sil_e=((vad->sil_cnt*vad->mean_start_sil_e)+e)/(vad->sil_cnt+1);
			//vad->Thresh_E=vad->cfg->e_thresh*log(vad->Min_E*32768*32768)/(32768*32768);
			vad->Thresh_E=vad->cfg->e_thresh*vad->Min_E;//*log(vad->Min_E);////*fx1)*fx2;
			++vad->sil_cnt;
		}
		if(vad->cfg->debug)
		{
		wtk_debug("v[%d,%f]= e=%f/%f/%f f=%f/%f/%f  speech=%d/%d\n",vf->index,vf->index*vf->frame_step*1.0/vad->cfg->rate,
						e,vad->Min_E,vad->Thresh_E,
						F,vad->Min_F,vad->Thresh_F,
						speech,vad->cfg->min_speech_frame);
		}
		vf->raw_state=speech?wtk_vframe_speech:wtk_vframe_sil;
		//wtk_debug("v[%d,%f]: E=%f F=%f SFM=%f time=%f %s counter=%d\n",ki,ki*vad->cfg->stft.win*1.0/16000,e,F,sfm,t,speech?"speech":"sil",counter);
		//wtk_debug("v[%d,%f]: E=%f F=%f SFM=%f min=%f/%f/%f time=%f %s counter=%d\n",ki,ki*vad->cfg->stft.win*1.0/16000,e,F,sfm,vad->Min_E,vad->Min_F,vad->Min_SFM,t,speech?"speech":"sil",counter);
		switch(vad->speech_state)
		{
		case WTK_FEVAD_SIL:
			if(speech)
			{
				//wtk_debug("v[%d,%f]: E=%f F=%f SFM=%f min=%f/%f/%f time=%f %s counter=%d\n",ki,ki*vad->cfg->stft.win*1.0/16000,e,F,sfm,vad->Min_E,vad->Min_F,vad->Min_SFM,t,speech?"speech":"sil",counter);
				wtk_queue_push(&(vad->bak_q),&(vf->q_n));
				if(vad->bak_q.length>=vad->cfg->min_speech_frame)
				{
					//wtk_debug("%f(%d) got speech \n",(vf->index-vad->bak_q.length)*vad->cfg->stft.win*1.0/16000,vf->index-vad->bak_q.length);
					//exit(0);
					wtk_fevad_flush_bak(vad,0);
					vad->speech_state=WTK_FEVAD_SPEECH;
				}
			}else
			{
				if(vad->bak_q.length>0)
				{
					wtk_fevad_flush_bak(vad,1);
				}
//				vad->Min_E=((vad->sil_cnt*vad->Min_E)+e)/(vad->sil_cnt+1);
//				vad->Thresh_E=vad->cfg->e_thresh*log(vad->Min_E*fx1)*fx2;
//				++vad->sil_cnt;
				vf->state=wtk_vframe_sil;
				vad->raise(vad->raise_ths,vf);
			}
			break;
		case WTK_FEVAD_SPEECH:
			if(speech)
			{
				if(vad->bak_q.length>0)
				{
					wtk_fevad_flush_bak(vad,0);
				}
				vf->state=wtk_vframe_speech;
				vad->raise(vad->raise_ths,vf);
			}else
			{
				wtk_queue_push(&(vad->bak_q),&(vf->q_n));
				if(vad->bak_q.length>=vad->cfg->min_sil_frame)
				{
					//wtk_debug("%f got sil\n",(vf->index-vad->bak_q.length)*vad->cfg->stft.win*1.0/16000);
					wtk_fevad_flush_bak(vad,1);
					vad->speech_state=WTK_FEVAD_SIL;
				}
			}
			break;
		}
		//wtk_debug("v[%d,%f]: E=%f F=%f SFM=%f time=%f %s\n",ki,ki*vad->cfg->stft.win*1.0/16000,e,F,sfm,t,speech?"speech":"sil");
		//exit(0);
		break;
	}
	if(is_end && vad->bak_q.length>0)
	{
		if(vad->speech_state==WTK_FEVAD_SIL)
		{
			wtk_fevad_flush_bak(vad,1);
		}else
		{
			wtk_fevad_flush_bak(vad,0);
		}
		//exit(0);
	}
	wtk_stft_push_msg(vad->stft,msg);
}

void wtk_fevad_on_stft(wtk_fevad_t *vad,wtk_stft_msg_t *msg,int pos,int is_end)
{
	float e;
	float maxf,f,F,Fv;
	int maxi;
	int i;
	float *pf=vad->stft->input[0];
	wtk_complex_t *fft;
	wtk_dcomplex_t am,gm,c;
	float sfm;
	int counter;
	int speech;
	wtk_vframe_t* vf;
	int nbin=vad->stft->nbin;
	int tx=0;

	//nbin=2000*nbin*2/(vad->cfg->rate);
	//wtk_debug("pos=%d is_end=%d\n",pos,is_end);
	if(is_end && pos==0)
	{
		if( vad->bak_q.length>0)
		{
			if(vad->speech_state==WTK_FEVAD_SIL)
			{
				wtk_fevad_flush_bak(vad,1);
			}else
			{
				wtk_fevad_flush_bak(vad,0);
			}
			//exit(0);
		}
		return;
	}
	//xoutput+=pos;
	if(is_end)
	{
		vf=wtk_fevad_pop_vframe(vad,vad->stft->input[0],pos);
	}else
	{
		vf=wtk_fevad_pop_vframe(vad,vad->stft->input[0],vad->frame_size);
	}
	//print_float(vad->stft->input[0],vad->frame_size);
	//exit(0);
	++vad->idx;
	e=0;
	for(i=0;i<vad->stft->pos;++i)
	{
		if(pf[i]>0)
		{
			e+=pf[i];
		}else
		{
			e-=pf[i];
		}
		//e+=pf[i]*pf[i];
	}
	fft=msg->fft[0];
	maxf=fft[0].a*fft[0].a+fft[0].b*fft[0].b;
	maxi=0;
	//wtk_debug("F=%f\n",F);
	//wtk_debug("e=%f f=%d/%f\n",e,maxi,f);
	am.a=am.b=0;
	gm.a=gm.b=0;
	gm.a=1;
	for(i=0;i<nbin;++i)
	{
		//wtk_debug("v[%d]=%f+%fi\n",i,fft[i].a,fft[i].b);
		am.a+=fft[i].a;
		am.b+=fft[i].b;
		f=fft[i].a*fft[i].a+fft[i].b*fft[i].b;
		if(f>maxf)
		{
			maxf=f;
			maxi=i;
		}
		if(f!=0)
		{
			gm.a*=f;
			if(gm.a<1e-2)
			{
				gm.a*=1000;
				++tx;
			}
			//wtk_debug("gm.a=%e tx=%d f=%f\n",gm.a,tx,f);
		}
		if(fft[i].a!=0)
		{
			gm.b+=(fft[i].b/fft[i].a);
			//gm.b+=atan(fft[i].b/fft[i].a);
		}
		//wtk_debug(" %f+%fi %f+%f\n",fft[i].a,fft[i].b,gm.a,gm.b);
	}
	if(1)
	{
		gm.a=(log(gm.a)-3*tx*log(10))/2;
	}else
	{
		gm.a=(log(gm.a)-log(100)*nbin)/2;
	}
	//gm.a=(log(gm.a)-log(100)*nbin)/2;
	F=(maxi+1)*vad->cfg->rate/vad->stft->cfg->win;
	Fv=maxf;
	//wtk_debug("%f/%f\n",gm.a,gm.b);
//	exit(0);
//	//wtk_debug("%f+%f\n",am.a,am.b);
	gm.a*=2;
	gm.b*=2;
	am.a/=(vad->cfg->stft.win);
	am.b/=(vad->cfg->stft.win);
	c.a=log(am.a*am.a+am.b*am.b)/2;
	//c.b=ATAN(am.b/am.a);
	if(am.a==0)
	{
		c.b=1e10;
	}else
	{
		c.b=(am.b/am.a);
	}
	//wtk_debug("gm=%f+%f am=%f+%f\n",gm.a,gm.b,am.a,am.b);

	am=c;
	gm.a/=(vad->cfg->stft.win);
	gm.b/=(vad->cfg->stft.win);
	f=vad->fx;
	c.a=f*(gm.a-am.a);
	c.b=f*(gm.b-am.b);
	//wtk_debug("f=%f gm=%f+%f am=%f+%f\n",f,gm.a,gm.b,am.a,am.b);
	sfm=sqrt(c.a*c.a+c.b*c.b);

//	wtk_debug("v[%d,%f]= e=%f/%f/%f f=%f/%f/%f sfm=%f/%f/%f\n",vf->index,vf->index*vf->frame_step*1.0/16000,
//					e,vad->Min_E,vad->Thresh_E,
//					F,vad->Min_F,vad->Thresh_F,
//					sfm,vad->Min_SFM,vad->Thresh_SF);
	switch(vad->state)
	{
	case WTK_FEVAD_INIT:
		///wtk_debug("v[%d,%f]: E=%f F=%f SFM=%f time=%f sil\n",ki,ki*vad->cfg->stft.win*1.0/16000,e,F,sfm,t);
		vad->mean_start_sil_e+=e;
		if(vf->index==1)
		{
			vad->Min_E=e;
			vad->Min_F=F;
			vad->Min_SFM=sfm;
		}else
		{
			if(e<vad->Min_E)
			{
				vad->Min_E=e;
			}
			if(F<vad->Min_F)
			{
				vad->Min_F=F;
				vad->Min_Fv=Fv;
			}
			if(sfm<vad->Min_SFM)
			{
				vad->Min_SFM=sfm;
			}
		}
		if(vad->cfg->debug)
		{
			wtk_debug("v[%d,%f]= e=%f/%f/%f f=%f/%f/%f sfm=%f/%f/%f\n",vf->index,vf->index*vf->frame_step*1.0/vad->cfg->rate,
							e,vad->Min_E,vad->Thresh_E,
							F,vad->Min_F,vad->Thresh_F,
							sfm,vad->Min_SFM,vad->Thresh_SF);
		}
		vf->state=wtk_vframe_sil;
		vad->raise(vad->raise_ths,vf);
		if(vad->idx==vad->cfg->min_start_frame)
		{
			vad->mean_start_sil_e/=vad->cfg->min_start_frame;
			vad->Min_E=vad->mean_start_sil_e;
			//vad->Thresh_E=vad->cfg->e_thresh*log(vad->Min_E*32768*32768)/(32768*32768);
			vad->Thresh_E=vad->cfg->e_thresh*vad->Min_E;//log(vad->Min_E);//*fx1)*fx2;
			//wtk_debug("%f/%f %f/%f\n",vad->Min_E,vad->Thresh_E,vad->Min_E*32768*32768,log(vad->Min_E*32768*32768));
			vad->Thresh_F=vad->cfg->f_thresh;
			vad->Thresh_SF=vad->cfg->sf_thresh;
			vad->Thresh_Fv=vad->cfg->fv_thresh;
			vad->sil_cnt=1;
			//wtk_debug("Min_E=%f Min_F=%f Min_SFM=%f Thresh=%f/%f/%f\n",vad->Min_E,vad->Min_F,vad->Min_SFM,vad->Thresh_E,vad->Thresh_F,vad->Thresh_SF);
			vad->state=WTK_FEVAD_UPDATE;
			vad->speech_state=WTK_FEVAD_SIL;
		}
		break;
	case WTK_FEVAD_UPDATE:
		counter=0;
		if((e-vad->Min_E)>=vad->Thresh_E)
		{
			++counter;
		}
		//wtk_debug("%f/%f/%d\n",(F-vad->Min_F),vad->Thresh_F,(F-vad->Min_F)>=vad->Thresh_F);
		if((F-vad->Min_F)>=vad->Thresh_F || (Fv-vad->Min_Fv)>=vad->Thresh_Fv)
		{
			++counter;
		}
		if((sfm-vad->Min_SFM)>=vad->Thresh_SF)
		{
			++counter;
		}
		speech=counter>1?1:0;
		if(speech==0 && e/vad->Min_E<vad->cfg->e_ratio)
		{
			float alpha=vad->cfg->e_alpha;

			//wtk_debug("update min e\n");
			//vad->Min_E=((vad->sil_cnt*vad->Min_E)+e)/(vad->sil_cnt+1);
			vad->Min_E=vad->Min_E*alpha+(1-alpha)*e;
			//vad->mean_start_sil_e=((vad->sil_cnt*vad->mean_start_sil_e)+e)/(vad->sil_cnt+1);
			//vad->Thresh_E=vad->cfg->e_thresh*log(vad->Min_E*32768*32768)/(32768*32768);
			vad->Thresh_E=vad->cfg->e_thresh*vad->Min_E;//*log(vad->Min_E);////*fx1)*fx2;
			++vad->sil_cnt;
		}
		if(vad->cfg->debug)
		{
		wtk_debug("v[%d,%f]= e=%f/%f/%f f=%f/%f/%f %f/%f sfm=%f/%f/%f speech=%d/%d\n",vf->index,vf->index*vf->frame_step*1.0/vad->cfg->rate,
						e,vad->Min_E,vad->Thresh_E,
						F,vad->Min_F,vad->Thresh_F,Fv,vad->Min_Fv,
						sfm,vad->Min_SFM,vad->Thresh_SF,speech,vad->cfg->min_speech_frame);
		}
		if(vad->check_speech && speech)
		{
			speech=vad->check_speech(vad->ths,vf->index,e,F,sfm,vad->Min_E,vad->mean_start_sil_e);
		}
		vf->raw_state=speech?wtk_vframe_speech:wtk_vframe_sil;
		//wtk_debug("v[%d,%f]: E=%f F=%f SFM=%f time=%f %s counter=%d\n",ki,ki*vad->cfg->stft.win*1.0/16000,e,F,sfm,t,speech?"speech":"sil",counter);
		//wtk_debug("v[%d,%f]: E=%f F=%f SFM=%f min=%f/%f/%f time=%f %s counter=%d\n",ki,ki*vad->cfg->stft.win*1.0/16000,e,F,sfm,vad->Min_E,vad->Min_F,vad->Min_SFM,t,speech?"speech":"sil",counter);
		switch(vad->speech_state)
		{
		case WTK_FEVAD_SIL:
			if(speech)
			{
				//wtk_debug("v[%d,%f]: E=%f F=%f SFM=%f min=%f/%f/%f time=%f %s counter=%d\n",ki,ki*vad->cfg->stft.win*1.0/16000,e,F,sfm,vad->Min_E,vad->Min_F,vad->Min_SFM,t,speech?"speech":"sil",counter);
				wtk_queue_push(&(vad->bak_q),&(vf->q_n));
				if(vad->bak_q.length>=vad->cfg->min_speech_frame)
				{
					//wtk_debug("%f(%d) got speech \n",(vf->index-vad->bak_q.length)*vad->cfg->stft.win*1.0/16000,vf->index-vad->bak_q.length);
					//exit(0);
					wtk_fevad_flush_bak(vad,0);
					vad->speech_state=WTK_FEVAD_SPEECH;
				}
			}else
			{
				if(vad->bak_q.length>0)
				{
					wtk_fevad_flush_bak(vad,1);
				}
//				vad->Min_E=((vad->sil_cnt*vad->Min_E)+e)/(vad->sil_cnt+1);
//				vad->Thresh_E=vad->cfg->e_thresh*log(vad->Min_E*fx1)*fx2;
//				++vad->sil_cnt;
				vf->state=wtk_vframe_sil;
				vad->raise(vad->raise_ths,vf);
			}
			break;
		case WTK_FEVAD_SPEECH:
			if(speech)
			{
				if(vad->bak_q.length>0)
				{
					wtk_fevad_flush_bak(vad,0);
				}
				vf->state=wtk_vframe_speech;
				vad->raise(vad->raise_ths,vf);
			}else
			{
				wtk_queue_push(&(vad->bak_q),&(vf->q_n));
				if(vad->bak_q.length>=vad->cfg->min_sil_frame)
				{
					//wtk_debug("%f got sil\n",(vf->index-vad->bak_q.length)*vad->cfg->stft.win*1.0/16000);
					wtk_fevad_flush_bak(vad,1);
					vad->speech_state=WTK_FEVAD_SIL;
				}
			}
			break;
		}
		//wtk_debug("v[%d,%f]: E=%f F=%f SFM=%f time=%f %s\n",ki,ki*vad->cfg->stft.win*1.0/16000,e,F,sfm,t,speech?"speech":"sil");
		//exit(0);
		break;
	}
//	if(vf->index*vf->frame_step*1.0/vad->cfg->rate>49.0)
//	{
//		exit(0);
//	}
	if(is_end && vad->bak_q.length>0)
	{
		if(vad->speech_state==WTK_FEVAD_SIL)
		{
			wtk_fevad_flush_bak(vad,1);
		}else
		{
			wtk_fevad_flush_bak(vad,0);
		}
		//exit(0);
	}
	wtk_stft_push_msg(vad->stft,msg);
}

void wtk_fevad_on_stft1(wtk_fevad_t *vad,wtk_stft_msg_t *msg,int pos,int is_end)
{
	float e;
	float maxf,f,F;
	int maxi;
	int i,j;
	float *pf=vad->stft->input[0];
	wtk_complex_t *fft;
	wtk_dcomplex_t am,gm,c;
	float sfm;
	int counter;
	int speech;
	wtk_vframe_t* vf;
	int nbin=vad->stft->nbin;

	//wtk_debug("pos=%d is_end=%d\n",pos,is_end);
	if(is_end && pos==0)
	{
		if( vad->bak_q.length>0)
		{
			if(vad->speech_state==WTK_FEVAD_SIL)
			{
				wtk_fevad_flush_bak(vad,1);
			}else
			{
				wtk_fevad_flush_bak(vad,0);
			}
			//exit(0);
		}
		return;
	}
	//xoutput+=pos;
	if(is_end)
	{
		vf=wtk_fevad_pop_vframe(vad,vad->stft->input[0],pos);
	}else
	{
		vf=wtk_fevad_pop_vframe(vad,vad->stft->input[0],vad->frame_size);
	}
	//print_float(vad->stft->input[0],vad->frame_size);
	//exit(0);
	++vad->idx;
	e=0;
	for(i=0;i<vad->stft->pos;++i)
	{
		e+=pf[i]*pf[i];
	}
//	wtk_debug("v[%d,%f]=%f/%f\n",vad->idx,vad->idx*vad->cfg->stft.win*1.0/16000,e,vad->mean_start_sil_e);
//	if(vad->idx*vad->cfg->stft.win*1.0/16000>4.8)
//	{
//		exit(0);
//	}
	fft=msg->fft[0];
	maxf=fft[0].a*fft[0].a+fft[0].b*fft[0].b;
	maxi=0;
	//wtk_debug("v[0]=%f+%f maxi=%d maxf=%f\n",fft[0].a,fft[0].b,maxi,maxf);
	for(i=1;i<nbin;++i)
	{
		f=fft[i].a*fft[i].a+fft[i].b*fft[i].b;
		//wtk_debug("v[%d]=%f+%f maxi=%d maxf=%f\n",i,fft[i].a,fft[i].b,maxi,maxf);
		if(f>maxf)
		{
			maxf=f;
			maxi=i;
		}
	}
	F=(maxi+1)*vad->cfg->rate/vad->stft->cfg->win;
	//wtk_debug("F=%f\n",F);
	//wtk_debug("e=%f f=%d/%f\n",e,maxi,f);
	am.a=am.b=0;
	gm.a=gm.b=0;
	for(i=0;i<nbin;++i)
	{
		//wtk_debug("v[%d]=%f+%fi\n",i,fft[i].a,fft[i].b);
		am.a+=fft[i].a;
		am.b+=fft[i].b;
		f=fft[i].a*fft[i].a+fft[i].b*fft[i].b;
		if(f!=0)
		{
			gm.a+=log(f)/2;
		}
		if(fft[i].a!=0)
		{
			gm.b+=atan(fft[i].b/fft[i].a);
		}
		//wtk_debug(" %f+%fi %f+%f\n",fft[i].a,fft[i].b,gm.a,gm.b);
	}
	//wtk_debug("%f+%f\n",am.a,am.b);
	for(i=nbin;i<vad->cfg->stft.win;++i)
	{
		//j=i-vad->stft->nbin+1;
		j=nbin-(i-nbin+2);
		//wtk_debug("v[%d/%d]=%f+%f\n",i,j,fft[j].a,-fft[j].b);
		am.a+=fft[j].a;
		am.b+=-fft[j].b;
		//wtk_debug("v[%d]=%f+%fi j=%d\n",i,fft[j].a,-fft[j].b,j);
		//exit(0);
		f=fft[j].a*fft[j].a+fft[j].b*fft[j].b;
		if(f!=0)
		{
			gm.a+=log(f)/2;
		}
		if(fft[j].a!=0)
		{
			gm.b+=atan(-fft[j].b/fft[j].a);
			//wtk_debug("%f+%f %f+%f\n",am.a,am.b,gm.a,gm.b);
		}
	}
	//wtk_debug("%f+%f %f+%f\n",am.a,am.b,gm.a,gm.b);
	//wtk_debug("%f+%f\n",am.a,am.b);
	//exit(0);
	//10*log10(a) => 10*log10(e)*ln(a) => 10*log10(e)*ln(G/A)  => 10*log10(e)*(ln(G)-ln(A));
	//10*log10((ln(f1)+ln(f2))/nbin-ln(A);
	//ln(a+bi)=ln(a^2+b^2)/2+tan-1(b/a)i;
	am.a/=(vad->cfg->stft.win);
	am.b/=(vad->cfg->stft.win);
	c.a=log(am.a*am.a+am.b*am.b)/2;
	c.b=atan(am.b/am.a);
	am=c;
	gm.a/=(vad->cfg->stft.win);
	gm.b/=(vad->cfg->stft.win);
	f=10*log10(2.718281828);
	c.a=f*(gm.a-am.a);
	c.b=f*(gm.b-am.b);
	//wtk_debug("f=%f gm=%f+%f am=%f+%f\n",f,gm.a,gm.b,am.a,am.b);
	sfm=sqrt(c.a*c.a+c.b*c.b);

//	wtk_debug("v[%d,%f]= e=%f/%f/%f f=%f/%f/%f sfm=%f/%f/%f\n",vf->index,vf->index*vf->frame_step*1.0/8000,
//					e,vad->Min_E,vad->Thresh_E,
//					F,vad->Min_F,vad->Thresh_F,
//					sfm,vad->Min_SFM,vad->Thresh_SF);
	switch(vad->state)
	{
	case WTK_FEVAD_INIT:
		///wtk_debug("v[%d,%f]: E=%f F=%f SFM=%f time=%f sil\n",ki,ki*vad->cfg->stft.win*1.0/16000,e,F,sfm,t);
		vad->mean_start_sil_e+=e;
		if(vf->index==1)
		{
			vad->Min_E=e;
			vad->Min_F=F;
			vad->Min_SFM=sfm;
		}else
		{
			if(e<vad->Min_E)
			{
				vad->Min_E=e;
			}
			if(F<vad->Min_F)
			{
				vad->Min_F=F;
			}
			if(sfm<vad->Min_SFM)
			{
				vad->Min_SFM=sfm;
			}
		}
//		wtk_debug("v[%d,%f]= e=%f/%f/%f f=%f/%f/%f sfm=%f/%f/%f\n",vf->index,vf->index*vf->frame_step*1.0/16000,
//						e,vad->Min_E,vad->Thresh_E,
//						F,vad->Min_F,vad->Thresh_F,
//						sfm,vad->Min_SFM,vad->Thresh_SF);
		vf->state=wtk_vframe_sil;
		vad->raise(vad->raise_ths,vf);
		if(vad->idx==vad->cfg->min_start_frame)
		{
			vad->mean_start_sil_e/=vad->cfg->min_start_frame;
			vad->Min_E=vad->mean_start_sil_e;
			//vad->Thresh_E=vad->cfg->e_thresh*log(vad->Min_E*32768*32768)/(32768*32768);
			vad->Thresh_E=vad->cfg->e_thresh*vad->Min_E;//log(vad->Min_E);//*fx1)*fx2;
			//wtk_debug("%f/%f %f/%f\n",vad->Min_E,vad->Thresh_E,vad->Min_E*32768*32768,log(vad->Min_E*32768*32768));
			vad->Thresh_F=vad->cfg->f_thresh;
			vad->Thresh_SF=vad->cfg->sf_thresh;
			vad->sil_cnt=1;
			//wtk_debug("Min_E=%f Min_F=%f Min_SFM=%f Thresh=%f/%f/%f\n",vad->Min_E,vad->Min_F,vad->Min_SFM,vad->Thresh_E,vad->Thresh_F,vad->Thresh_SF);
			vad->state=WTK_FEVAD_UPDATE;
			vad->speech_state=WTK_FEVAD_SIL;
		}
		break;
	case WTK_FEVAD_UPDATE:
		counter=0;
		if((e-vad->Min_E)>=vad->Thresh_E)
		{
			++counter;
		}
		//wtk_debug("%f/%f/%d\n",(F-vad->Min_F),vad->Thresh_F,(F-vad->Min_F)>=vad->Thresh_F);
		if((F-vad->Min_F)>=vad->Thresh_F)
		{
			++counter;
		}
		if((sfm-vad->Min_SFM)>=vad->Thresh_SF)
		{
			++counter;
		}
		speech=counter>1?1:0;
//		wtk_debug("v[%d,%f]=%d %s e=%f/%f/%f f=%f/%f/%f sfm=%f/%f/%f\n",vf->index,vf->index*vf->frame_step*1.0/16000,counter,
//				speech?"speech":"sil",
//						e,vad->Min_E,vad->Thresh_E,
//						F,vad->Min_F,vad->Thresh_F,
//						sfm,vad->Min_SFM,vad->Thresh_SF);
		if(speech==0 && e/vad->Min_E<10.0)
		{
			float alpha=0.9;

			//wtk_debug("update min e\n");
			//vad->Min_E=((vad->sil_cnt*vad->Min_E)+e)/(vad->sil_cnt+1);
			vad->Min_E=vad->Min_E*alpha+(1-alpha)*e;
			//vad->mean_start_sil_e=((vad->sil_cnt*vad->mean_start_sil_e)+e)/(vad->sil_cnt+1);
			//vad->Thresh_E=vad->cfg->e_thresh*log(vad->Min_E*32768*32768)/(32768*32768);
			vad->Thresh_E=vad->cfg->e_thresh*vad->Min_E;//*log(vad->Min_E);////*fx1)*fx2;
			++vad->sil_cnt;
		}
//		wtk_debug("v[%d,%f]= e=%f/%f/%f f=%f/%f/%f sfm=%f/%f/%f speech=%d/%d\n",vf->index,vf->index*vf->frame_step*1.0/vad->cfg->rate,
//						e,vad->Min_E,vad->Thresh_E,
//						F,vad->Min_F,vad->Thresh_F,
//						sfm,vad->Min_SFM,vad->Thresh_SF,speech,vad->cfg->min_speech_frame);
		if(vad->check_speech && speech)
		{
			speech=vad->check_speech(vad->ths,vf->index,e,F,sfm,vad->Min_E,vad->mean_start_sil_e);
		}
		vf->raw_state=speech?wtk_vframe_speech:wtk_vframe_sil;
		//wtk_debug("v[%d,%f]: E=%f F=%f SFM=%f time=%f %s counter=%d\n",ki,ki*vad->cfg->stft.win*1.0/16000,e,F,sfm,t,speech?"speech":"sil",counter);
		//wtk_debug("v[%d,%f]: E=%f F=%f SFM=%f min=%f/%f/%f time=%f %s counter=%d\n",ki,ki*vad->cfg->stft.win*1.0/16000,e,F,sfm,vad->Min_E,vad->Min_F,vad->Min_SFM,t,speech?"speech":"sil",counter);
		switch(vad->speech_state)
		{
		case WTK_FEVAD_SIL:
			if(speech)
			{
				//wtk_debug("v[%d,%f]: E=%f F=%f SFM=%f min=%f/%f/%f time=%f %s counter=%d\n",ki,ki*vad->cfg->stft.win*1.0/16000,e,F,sfm,vad->Min_E,vad->Min_F,vad->Min_SFM,t,speech?"speech":"sil",counter);
				wtk_queue_push(&(vad->bak_q),&(vf->q_n));
				if(vad->bak_q.length>=vad->cfg->min_speech_frame)
				{
					//wtk_debug("%f(%d) got speech \n",(vf->index-vad->bak_q.length)*vad->cfg->stft.win*1.0/16000,vf->index-vad->bak_q.length);
					//exit(0);
					wtk_fevad_flush_bak(vad,0);
					vad->speech_state=WTK_FEVAD_SPEECH;
				}
			}else
			{
				if(vad->bak_q.length>0)
				{
					wtk_fevad_flush_bak(vad,1);
				}
//				vad->Min_E=((vad->sil_cnt*vad->Min_E)+e)/(vad->sil_cnt+1);
//				vad->Thresh_E=vad->cfg->e_thresh*log(vad->Min_E*fx1)*fx2;
//				++vad->sil_cnt;
				vf->state=wtk_vframe_sil;
				vad->raise(vad->raise_ths,vf);
			}
			break;
		case WTK_FEVAD_SPEECH:
			if(speech)
			{
				if(vad->bak_q.length>0)
				{
					wtk_fevad_flush_bak(vad,0);
				}
				vf->state=wtk_vframe_speech;
				vad->raise(vad->raise_ths,vf);
			}else
			{
				wtk_queue_push(&(vad->bak_q),&(vf->q_n));
				if(vad->bak_q.length>=vad->cfg->min_sil_frame)
				{
					//wtk_debug("%f got sil\n",(vf->index-vad->bak_q.length)*vad->cfg->stft.win*1.0/16000);
					wtk_fevad_flush_bak(vad,1);
					vad->speech_state=WTK_FEVAD_SIL;
				}
			}
			break;
		}
		//wtk_debug("v[%d,%f]: E=%f F=%f SFM=%f time=%f %s\n",ki,ki*vad->cfg->stft.win*1.0/16000,e,F,sfm,t,speech?"speech":"sil");
		//exit(0);
		break;
	}
	if(is_end && vad->bak_q.length>0)
	{
		if(vad->speech_state==WTK_FEVAD_SIL)
		{
			wtk_fevad_flush_bak(vad,1);
		}else
		{
			wtk_fevad_flush_bak(vad,0);
		}
		//exit(0);
	}
}

void wtk_fevad_dc(wtk_fevad_t *vad,short *mic,int len)
{
	int i;
	short vin,vout;
	short *mem=vad->notch_mem;
	static float radius=0.9;//nlms->cfg->notch_radius;
	static float den2=0;

	if(den2==0)
	{
		den2=radius*radius+0.7*(1-radius)*(1-radius);
	}
	//den2=0.96;
	for(i=0;i<len;++i)
	{
		vin=mic[i];
		vout=mem[0]+vin;
		mic[i]=radius*vout;
		mem[0]=mem[1]+2*(-vin+mic[i]);
		mem[1]=vin-den2*vout;
		//wtk_debug("v[%d]: mem=%f/%f %f/%f %f %f/%f %f/%f\n",i,mem[0],mem[1],radius,vout,den2,vin,den2*vout,den2,vout);
		//exit(0);
	}
}


void wtk_fevad_feed(wtk_fevad_t *vad,char *data,int len,int is_end)
{
	if(1)
	{
#define DEBUG_DC
#ifdef DEBUG_DC
		static wtk_strbuf_t *buf=NULL;

		if(!buf)
		{
			buf=wtk_strbuf_new(1024,1);
		}
#endif
		wtk_fevad_dc(vad,(short*)data,len/2);
#ifdef DEBUG_DC
		wtk_strbuf_push(buf,data,len);
		if(is_end)
		{
			wave_write_file("tmp.wav",vad->cfg->rate,buf->data,buf->pos);
		}
#endif
	}
//	xinput+=len/2;
	wtk_stft_feed2(vad->stft,(short*)data,len/2,is_end);
//	if(is_end)
//	{
//		wtk_debug("%d/%d len=%d\n",xinput,xoutput,vad->bak_q.length);
//		if(xinput!=xoutput || vad->bak_q.length>0)
//		{
//			exit(0);
//		}
//	}
}

void wtk_fevad_feed_float(wtk_fevad_t *vad,float *data,int len,int is_end)
{
	wtk_stft_feed_float(vad->stft,data,len,is_end);
}

