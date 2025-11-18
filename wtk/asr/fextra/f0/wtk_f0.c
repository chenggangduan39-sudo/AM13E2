#include "wtk_f0.h"
void wtk_f0_init_dp_f0(wtk_f0_t *f,double freq,F0_params *par,long *buffsize,long *sdstep);

F0_params* f0_params_new(wtk_f0_cfg_t *cfg)
{
	F0_params *p;

	p=(F0_params*)calloc(1,sizeof(*p));
	p->cand_thresh=0.3f;
	p->lag_weight=0.3f;
	p->freq_weight=0.02f;
	p->trans_cost=0.005f;
	p->trans_amp=0.5f;
	p->trans_spec=0.5f;
	p->voice_bias=0.0f;
	p->double_cost=0.35f;
	p->min_f0=50;
	p->max_f0=550;
	p->frame_step=0.01f;
	p->frame_step=cfg->frame_dur;
	p->wind_dur=0.0075f;
	p->n_cands=20;
	p->mean_f0=200;
	p->mean_f0_weight=0.0f;
	p->conditioning=0;
	p->dp_circular=DP_CIRCULAR;
	p->dp_hist=DP_HIST;
	return p;
}

wav_params* wav_params_new(void)
{
	wav_params* p;

	p=(wav_params*)calloc(1,sizeof(*p));
	p->rate=16000;
	p->size=2;
	p->nan=-1;
	return p;
}

int wav_parms_delete(wav_params *w)
{
	free(w);
	return 0;
}

wtk_f0_t* wtk_f0_new(wtk_f0_cfg_t *cfg)
{
	wtk_f0_t *f;

	f=(wtk_f0_t*)wtk_malloc(sizeof(*f));
	wtk_f0_init(f,cfg);
	return f;
}

int wtk_f0_delete(wtk_f0_t* f)
{
	int i;
	Frame *frm, *next;

	if(f->post)
	{
		wtk_fpost_delete(f->post);
	}
	if(f->avg)
	{
		wtk_favg_delete(f->avg);
	}
	if(f->pcands){free(f->pcands);}
	if(f->rms_speech){free(f->rms_speech);}
	if(f->f0p){free(f->f0p);}
	if(f->vuvp){free(f->vuvp);}
	if(f->acpkp){free(f->acpkp);}
	if(f->peaks){free(f->peaks);}
	if(f->locs){free(f->locs);}
	if(f->windstat){free(f->windstat);}
	if(f->headF)
	{
		for(i=0,frm=f->headF;i<f->size_cir_buffer;++i)
		{
			next=frm->next;
			free(frm->cp->correl);
			free(frm->dp->locs);
			free(frm->dp->pvals);
			free(frm->dp->mpvals);
			free(frm->dp->prept);
			free(frm->dp->dpvals);
			free(frm->cp);
			free(frm->dp);
			free(frm);
			frm=next;
		}
	}
	if(f->stat)
	{
		free(f->stat->stat);
		free(f->stat->rms);
		free(f->stat->rms_ratio);
		free(f->stat);
	}
	if(f->mem){free(f->mem);}
	if(f->wpar){free(f->wpar);}
	if(f->par){free(f->par);}
	if(f->state){free(f->state);}
	if(f->foutput){free(f->foutput);}
	if(f->co){free(f->co);}
	if(f->mem_do){free(f->mem_do);}
	if(f->din){free(f->din);}
	if(f->wind_c){free(f->wind_c);}
	if(f->wind_h){free(f->wind_h);}
	if(f->wind_hn){free(f->wind_hn);}
	if(f->dwind_e){free(f->dwind_e);}
	if(f->dwind){free(f->dwind);}
	if(f->dbdata){free(f->dbdata);}
	if(f->dbdata_i){free(f->dbdata_i);}
	if(f->b){free(f->b);}
	wtk_vector_buffer_delete(f->vb);
	wtk_heap_delete(f->local_heap);
	wtk_free(f);
	return 0;
}

int wtk_f0_init(wtk_f0_t *f,wtk_f0_cfg_t *cfg)
{
	memset(f,0,sizeof(*f));
	f->frame_index=0;
	f->notify=NULL;
	f->notify_ths=NULL;
	f->cfg=cfg;
	f->ncoeff=127;
	f->par=f0_params_new(cfg);
	f->wpar=wav_params_new();
	f->b=(float*)calloc(2048,sizeof(float));
	f->state=(float*)calloc(1000,sizeof(float));
	wtk_f0_init_dp_f0(f,f->wpar->rate,f->par, &f->buff_size, &(f->sdstep));
	f->vb=wtk_vector_buffer_new(f->buff_size*4);
	f->local_heap=wtk_heap_new(4096);
	wtk_f0_reset(f);
	if(cfg->use_post)
	{
		f->post=wtk_fpost_new(&(cfg->post),f);
	}else
	{
		f->post=0;
	}
	if(cfg->use_avg)
	{
		f->avg=wtk_favg_new(&(cfg->avg));
	}else
	{
		f->avg=0;
	}
	return 0;
}

void wtk_f0_reset_param(wtk_f0_t *f0)
{
	int j,i;
	Frame *tmp=NULL;
	int n_cands=f0->par->n_cands;

	f0->cir_buff_growth_count = 0;
	/********************get_f0s.c********************/
	/*Get_f0()*/
	/*downsample()*/
	memset(f0->b,0,sizeof(float)*2048);
	free(f0->foutput);
	f0->foutput = NULL;
	f0->ncoeff = 127;
	f0->ncoefft = 0;
	/*do_ffir()*/
	memset(f0->state,0,sizeof(float)*1000);
	free(f0->co);
	f0->co=NULL;
	free(f0->mem_do);
	f0->mem_do=NULL;
	f0->fsize=0;
	f0->resid=0;
	/*get_stationarity()*/
	f0->nframes_old = 0;
	f0->memsize=0;

	/********************sigproc.c********************/
	/*xget_window()*/
	free(f0->din);
	f0->din = NULL;
	f0->n0 = 0;
	/*xcwindow()*/
	free(f0->wind_c);
	f0->wsize_c = 0;
	f0->wind_c=NULL;
	/*xhwindow()*/
	f0->wsize_h = 0;
	free(f0->wind_h);
	f0->wind_h=NULL;
	/*xhnwindow()*/
	f0->wsize_hn = 0;
	free(f0->wind_hn);
	f0->wind_hn=NULL;
	/*wind_energy()*/
	f0->nwind_e = 0;
	free(f0->dwind_e);
	f0->dwind_e = NULL;
	/*xlpc()*/
	free(f0->dwind);
	f0->dwind=NULL;
	f0->nwind=0;
	/*crossf()*/
	free(f0->dbdata);
	f0->dbdata=NULL;
	f0->dbsize = 0;
	/*crossfi()*/
	free(f0->dbdata_i);
	f0->dbdata_i=NULL;
	f0->dbsize_i = 0;

	f0->tailF=f0->headF;
	f0->headF->dp->ncands=0;
	for(j = n_cands-1; j>= 0; j--)
		f0->headF->dp->dpvals[j] = 0.0;
	tmp=f0->headF->next;
	while(tmp!=f0->headF){
		tmp->dp->ncands=0;
		tmp=tmp->next;
		for(j = n_cands-1; j>= 0; j--)
			tmp->dp->dpvals[j] = 0.0;
	}
	f0->cmpthF = NULL;
	if (f0->wReuse){
		/*      spsassert(windstat, "windstat ckalloc failed");*/
		for(i=0; i<f0->wReuse; i++){
			f0->windstat[i].err = 0;
			f0->windstat[i].rms = 0;
		}
	}
	f0->num_active_frames = 0;
	f0->first_time=1;
	f0->last_time=0;
}

int wtk_f0_reset(wtk_f0_t *f)
{
	f->frame_index=0;
	if(f->post)
	{
		wtk_fpost_reset(f->post);
	}
	if(f->avg)
	{
		wtk_favg_reset(f->avg);
	}
	wtk_f0_reset_param(f);
	wtk_vector_buffer_reset(f->vb);
	wtk_heap_reset(f->local_heap);
	f->f0_array=wtk_array_new_h(f->local_heap,4096,sizeof(float));
	f->f0e_array=wtk_array_new_h(f->local_heap,4096,sizeof(float));
	f->acpkp_array=wtk_array_new_h(f->local_heap,4096,sizeof(float));
	return 0;
}

int wtk_f0_save(wtk_f0_t *f,wav_params* wpar)
{
	int i,k,frames;
	int ret;
	float *f0,*fe,*fk;
	//static int j=0;

	ret=wtk_f0_dp_f0(f,wpar->data,wpar->length,(int)f->sdstep,wpar->rate,&frames);
	if(ret!=0){goto end;}
	if(frames>0)
	{
		if(f->notify)
		{
			for(i=frames-1;i>=0;--i)
			{
				//f0[k]=f->f0p[i];
				//fe[k]=f->rms_speech[i];
				//wtk_debug("v[%d]=%f/%f\n",i,f->f0p[i],f->rms_speech[i]);
				f->notify(f->notify_ths,++f->frame_index,f->f0p[i],f->rms_speech[i]);
				if(f->avg)
				{
					wtk_favg_feed(f->avg,f->f0p[i]);
				}
			}
		}else
		{
			//printf("%d,%d\n",f->f0_array->nslot,frames);
			f0=(float*)wtk_array_push_n(f->f0_array,frames);
			fe=(float*)wtk_array_push_n(f->f0e_array,frames);
			fk=(float*)wtk_array_push_n(f->acpkp_array,frames);
			for(k=0,i=frames-1;i>=0;--i,++k)
			{
				f0[k]=f->f0p[i];
				fe[k]=f->rms_speech[i];
				fk[k]=f->acpkp[i];
				//wtk_debug("v[%d/%d]=%f/%f %f/%f\n",i,f->f0_array->nslot-frames+k,f->f0p[i],f->rms_speech[i],f0[k],fe[k]);
				if(f->avg)
				{
					wtk_favg_feed(f->avg,f0[k]);
				}
			}
		}
	}
end:
	return ret;
}

int wtk_f0_feed(wtk_f0_t *f,int state,short *data,int len)
{
	wtk_vector_buffer_t *vb=f->vb;
	short *end=data+len;
	wav_params *wpar=f->wpar;
	int ret=0,buff_size=f->buff_size;

	while(data<end)
	{
		data+=wtk_vector_buffer_push(vb,data,end-data);
		while(1)
		{
			wpar->length=buff_size;
			wpar->data=wtk_vector_buffer_peek_data(vb,wpar->length);
			if(!wpar->data){break;}
			ret=wtk_f0_save(f,wpar);
			wtk_vector_buffer_skip(vb,f->sdstep,wpar->length);
			if(ret!=0){goto end;}
		}
	}
	if(state==1)
	{
		f->last_time=1;
		wpar->length=wtk_vector_buffer_valid_len(vb);
		wpar->data=wtk_vector_buffer_peek_data(vb,wpar->length);
		if(wpar->data)
		{
			ret=wtk_f0_save(f,wpar);
		}
		if(f->avg)
		{
			wtk_favg_flush_end(f->avg);
		}
	}
end:
	return ret;
}

int wtk_f0_feed_char(wtk_f0_t *f,int state,char *data,int len)
{
	wtk_vector_buffer_t *vb=f->vb;
	char *end=data+len;
	wav_params *wpar=f->wpar;
	int ret=0,buff_size=f->buff_size;

	while(data<end)
	{
		data+=wtk_vector_buffer_push_c(vb,data,end-data);
		while(1)
		{
			wpar->length=buff_size;
			wpar->data=wtk_vector_buffer_peek_data(vb,wpar->length);
			if(!wpar->data){break;}
			ret=wtk_f0_save(f,wpar);
			wtk_vector_buffer_skip(vb,f->sdstep,wpar->length);
			if(ret!=0){goto end;}
		}
	}
	if(state==1)
	{
		f->last_time=1;
		wpar->length=wtk_vector_buffer_valid_len(vb);
		wpar->data=wtk_vector_buffer_peek_data(vb,wpar->length);
		if(wpar->data)
		{
			ret=wtk_f0_save(f,wpar);
		}
		if(f->avg)
		{
			wtk_favg_flush_end(f->avg);
		}
	}
end:
	return ret;
}

Frame* wtk_f0_new_frame(wtk_f0_t *f,int nlags,int ncands)
{
	Frame *frm;
	int j;

	frm = (Frame*)malloc(sizeof(Frame));
	frm->dp = (Dprec *) malloc(sizeof(Dprec));
	/*  spsassert(frm->dp,"frm->dp malloc failed in alloc_frame");*/
	frm->dp->ncands = 0;
	frm->cp = (Cross *) malloc(sizeof(Cross));
	/*  spsassert(frm->cp,"frm->cp malloc failed in alloc_frame");*/
	frm->cp->correl = (float *) malloc(sizeof(float) * nlags);
	/*  spsassert(frm->cp->correl, "frm->cp->correl malloc failed");*/
	/* Allocate space for candidates and working arrays. */
	frm->dp->locs = (short*)malloc(sizeof(short) * ncands);
	/*  spsassert(frm->dp->locs,"frm->dp->locs malloc failed in alloc_frame()");*/
	frm->dp->pvals = (float*)malloc(sizeof(float) * ncands);
	/*  spsassert(frm->dp->pvals,"frm->dp->pvals malloc failed in alloc_frame()");*/
	frm->dp->mpvals = (float*)malloc(sizeof(float) * ncands);
	/*  spsassert(frm->dp->mpvals,"frm->dp->mpvals malloc failed in alloc_frame()");*/
	frm->dp->prept = (short*)malloc(sizeof(short) * ncands);
	/*  spsassert(frm->dp->prept,"frm->dp->prept malloc failed in alloc_frame()");*/
	frm->dp->dpvals = (float*)malloc(sizeof(float) * ncands);
	/*  spsassert(frm->dp->dpvals,"frm->dp->dpvals malloc failed in alloc_frame()");*/

	/*  Initialize the cumulative DP costs to zero */
	for(j = ncands-1; j >= 0; j--)
		frm->dp->dpvals[j] = 0.0;

	return(frm);
}

void wtk_f0_init_dp_f0(wtk_f0_t *f,double freq,F0_params *par,long *buffsize,long *sdstep)
{
	int nframes;
	int i;
	int stat_wsize, agap, ind, downpatch;

	/*
	* reassigning some constants
	*/

	f->tcost = par->trans_cost;
	f->tfact_a = par->trans_amp;
	f->tfact_s = par->trans_spec;
	f->vbias = par->voice_bias;
	f->fdouble = par->double_cost;
	f->frame_int = par->frame_step;

	f->step = eround(f->frame_int * freq);
	f->size = eround(par->wind_dur * freq);
	f->frame_int = (float)(((float)f->step)/freq);
	f->wdur = (float)(((float)f->size)/freq);
	f->start = eround(freq / par->max_f0);
	f->stop = eround(freq / par->min_f0);
	f->nlags = f->stop - f->start + 1;
	f->ncomp = f->size + f->stop + 1; /* # of samples required by xcorr
							 comp. per fr. */
	f->maxpeaks = 2 + (f->nlags/2);	/* maximum number of "peaks" findable in ccf */
	f->ln2 = (float)log(2.0);
	f->size_frame_hist = (int) (DP_HIST	 / f->frame_int);
	f->size_frame_out = (int) (DP_LIMIT / f->frame_int);

	/*
	* SET UP THE D.P. WEIGHTING FACTORS:
	*      The intent is to make the effectiveness of the various fudge factors
	*      independent of frame rate or sampling frequency.
	*/

	/* Lag-dependent weighting factor to emphasize early peaks (higher freqs)*/
	f->lagwt = par->lag_weight/f->stop;

	/* Penalty for a frequency skip in F0 per frame */
	f->freqwt = par->freq_weight/f->frame_int;

	i = (int) (READ_SIZE *freq);
	if(f->ncomp >= f->step)
	{
		nframes = ((i-f->ncomp)/f->step ) + 1;
	}else
	{
		nframes = i / f->step;
	}

	/* *buffsize is the number of samples needed to make F0 computation
	of nframes DP frames possible.  The last DP frame is patched with
	enough points so that F0 computation on it can be carried.  F0
	computaion on each frame needs enough points to do

	1) xcross or cross correlation measure:
	enough points to do xcross - ncomp

	2) stationarity measure:
	enough to make 30 msec windowing possible - ind

	3) downsampling:
	enough to make filtering possible -- downpatch

	So there are nframes whole DP frames, padded with pad points
	to make the last frame F0 computation ok.

	*/

	/* last point in data frame needs points of 1/2 downsampler filter length
	long, DOWNSAMPLER_LENGTH is the filter length used in downsampler */
	downpatch = (((int) (freq * DOWNSAMPLER_LENGTH))+1) / 2;

	stat_wsize = (int) (STAT_WSIZE * freq);	// STAT_WSIZE = 0.03
	agap = (int) (STAT_AINT * freq);
	ind = ( agap - stat_wsize ) / 2;
	i = stat_wsize + ind;
	f->pad = downpatch + ((i>f->ncomp) ? i:f->ncomp);
	*buffsize = nframes * f->step + f->pad;
	*sdstep = nframes * f->step;

	/* Allocate space for the DP storage circularly linked data structure */

	f->size_cir_buffer = (int) (DP_CIRCULAR / f->frame_int);

	/* creating circularly linked data structures */
	f->tailF = wtk_f0_new_frame(f,f->nlags, par->n_cands);
	f->headF = f->tailF;

	/* link them up */
	for(i=1; i<f->size_cir_buffer; i++){
		f->headF->next = wtk_f0_new_frame(f,f->nlags, par->n_cands);
		f->headF->next->prev = f->headF;
		f->headF = f->headF->next;
	}
	f->headF->next = f->tailF;
	f->tailF->prev = f->headF;

	f->headF = f->tailF;

	/* Allocate sscratch array to use during backtrack convergence test. */
	if( ! f->pcands ) {
		f->pcands = (int *) malloc( par->n_cands * sizeof(int));
		/*    spsassert(pcands,"can't allocate pathcands");*/
	}

	/* Allocate arrays to return F0 and related signals. */

	/* Note: remember to compare *vecsize with size_frame_out, because
	size_cir_buffer is not constant */
	f->output_buf_size = f->size_cir_buffer;
	f->rms_speech = (float*)malloc(sizeof(float) * f->output_buf_size);
	/*  spsassert(rms_speech,"rms_speech ckalloc failed");*/
	f->f0p = (float*)malloc(sizeof(float) * f->output_buf_size);
	/*  spsassert(f0p,"f0p ckalloc failed");*/
	f->vuvp = (float*)malloc(sizeof(float)* f->output_buf_size);
	/*  spsassert(vuvp,"vuvp ckalloc failed");*/
	f->acpkp = (float*)malloc(sizeof(float) * f->output_buf_size);
	/*  spsassert(acpkp,"acpkp ckalloc failed");*/

	/* Allocate space for peak location and amplitude scratch arrays. */
	f->peaks = (float*)malloc(sizeof(float) * f->maxpeaks);
	/*  spsassert(peaks,"peaks ckalloc failed");*/
	f->locs = (int*)malloc(sizeof(int) * f->maxpeaks);
	/*  spsassert(locs, "locs ckalloc failed");*/

	/* Initialise the retrieval/saving scheme of window statistic measures */
	f->wReuse = agap / f->step;
	if (f->wReuse)
	{
		f->windstat = (Windstat *) malloc( f->wReuse * sizeof(Windstat));
		/*      spsassert(windstat, "windstat ckalloc failed");*/
		for(i=0; i<f->wReuse; i++){
			f->windstat[i].err = 0;
			f->windstat[i].rms = 0;
		}
	}
	f->num_active_frames = 0;
	f->first_time = 1;
	f->last_time=0;
}


/*  ----------------------------------------------------------   */
void wtk_f0_do_ffir(wtk_f0_t *f,register float	*buf,register int in_samps,register float	*bufo,register int *out_samps,int idx,
				   register int ncoef,float *fc,register int invert,register int skip,register int init)
/* fc contains 1/2 the coefficients of a symmetric FIR filter with unity
passband gain.  This filter is convolved with the signal in buf.
The output is placed in buf2.  If(invert), the filter magnitude
response will be inverted.  If(init&1), beginning of signal is in buf;
if(init&2), end of signal is in buf.  out_samps is set to the number of
output points placed in bufo. */
{
	register float *dp1, *dp2, *dp3, sum, integral;
	register int i, j, k, l;
	register float *sp;
	register float *buf1;

	buf1 = buf;
	if(ncoef > f->fsize) {/*allocate memory for full coeff. array and filter memory */
		f->fsize = 0;
		i = (ncoef+1)*2;
		if(!((f->co = (float *)realloc((void *)f->co, sizeof(float)*i)) &&
			(f->mem_do = (float *)realloc((void *)f->mem_do, sizeof(float)*i))))
		{
				fprintf(stderr,"allocation problems in do_fir()\n");
				return;
		}
		f->fsize = ncoef;
	}
	/* fill 2nd half with data */
	for(i=ncoef, dp1=f->mem_do+ncoef-1; i-- > 0; )
	{
		*dp1++ = *buf++;
	}
	if(init & 1)
	{	/* Is the beginning of the signal in buf? */
		/* Copy the half-filter and its mirror image into the coefficient array. */
		for(i=ncoef-1, dp3=fc+ncoef-1, dp2=f->co, dp1 = f->co+((ncoef-1)*2),
			integral = 0.0; i-- > 0; )
		{
			if(!invert) *dp1-- = *dp2++ = *dp3--;
			else {
				integral += (sum = *dp3--);
				*dp1-- = *dp2++ = -sum;
			}
			if(!invert)  *dp1 = *dp3;	/* point of symmetry */
			else {
				integral *= 2;
				integral += *dp3;
				*dp1 = integral - *dp3;
			}

			for(i=ncoef-1, dp1=f->mem_do; i-- > 0; ) *dp1++ = 0;
		}
	}
	else
		for(i=ncoef-1, dp1=f->mem_do, sp=f->state; i-- > 0; ) *dp1++ = *sp++;

	i = in_samps;
	f->resid = 0;

	k = (ncoef << 1) -1;	/* inner-product loop limit */

	if(skip <= 1) {       /* never used */
	}
	else {			/* skip points (e.g. for downsampling) */
		/* the buffer end is padded with (ncoef-1) data points */
		for( l=0 ; l < *out_samps; l++ ) {
			for(j=k-skip, dp1=f->mem_do, dp2=f->co, dp3=f->mem_do+skip, sum=0.0; j-- >0;
				*dp1++ = *dp3++)
				sum += *dp2++ * *dp1;
			for(j=skip; j-- >0; *dp1++ = *buf++) /* new data to memory */
				sum += *dp2++ * *dp1;
			*bufo++ = (sum<0.0) ? sum -0.5f : sum +0.5f;
		}
		if(init & 2){
			f->resid = in_samps - *out_samps * skip;
			for(l=f->resid/skip; l-- >0; ){
				for(j=k-skip, dp1=f->mem_do, dp2=f->co, dp3=f->mem_do+skip, sum=0.0; j-- >0;
					*dp1++ = *dp3++)
					sum += *dp2++ * *dp1;
				for(j=skip; j-- >0; *dp1++ = 0.0)
					sum += *dp2++ * *dp1;
				*bufo++ = (sum<0.0) ? sum -0.5f : sum +0.5f;
				(*out_samps)++;
			}
		}
		else
			for(dp3=buf1+idx-ncoef+1, l=ncoef-1, sp=f->state; l-- >0; ) *sp++ = *dp3++;
	}
} // do_ffir()

/* ----------------------------------------------------------------------- */
/* buffer-to-buffer downsample operation */
/* This is STRICTLY a decimator! (no upsample) */
int wtk_f0_downsamp(wtk_f0_t *f,float *in, float *out,int samples,int *outsamps,int state_idx,int decimate,int ncoef,float fc[],int init)
{
	if(in && out) {
		wtk_f0_do_ffir(f,in, samples, out, outsamps, state_idx, ncoef, fc, 0, decimate, init);
		return(TRUE);
	} else
		printf("Bad signal(s) passed to downsamp()\n");
	return(FALSE);
} // downsamp()

float* wtk_f0_downsample(wtk_f0_t *f,float *input,int samsin,int state_idx,
		double freq,int *samsout,int decimate,int first_time,int last_time)
{
	float	beta = 0.0f;
	int init;

	if(input && (samsin > 0) && (decimate > 0) && *samsout)
	{
		if(decimate == 1) {return input;}
		if(first_time)
		{
			int nbuff = (samsin/decimate) + (2*f->ncoeff);
			f->ncoeff = ((int)(freq * .005)) | 1;
			beta = .5f/decimate;
			f->foutput = (float*)realloc((void *)f->foutput, sizeof(float) * nbuff);
			for( ; nbuff > 0 ;)
			{
				f->foutput[--nbuff] = 0.0;
			}
			if(!lc_lin_fir(beta,&(f->ncoeff),f->b))
			{
				free((void *)f->foutput);
				f->foutput=0;
				return 0;
			}
			f->ncoefft = (f->ncoeff/2) + 1;
		}
		if(first_time)
		{
			init = 1;
		}else if(last_time)
		{
			init = 2;
		}else
		{
			init = 0;
		}
		if(wtk_f0_downsamp(f,input,f->foutput,samsin,samsout,state_idx,decimate,f->ncoefft,f->b,init))
		{
			return f->foutput;
		}
	}
	return 0;
}

void wtk_f0_xhwindow(wtk_f0_t *f,register float *din, register float *dout,register int n, register float preemp)
{
	register int i;
	register float *p;
	register float *q;

	if(f->wsize_h != n)
	{		/* Need to create a new Hamming window? */
		register double arg, half=0.5;

		if(f->wind_h)
		{
			f->wind_h = (float*)ckrealloc((void *)f->wind_h,n*sizeof(float));
		}else
		{
			f->wind_h = (float*)ckalloc(n*sizeof(float));
		}
		f->wsize_h = n;
		for(i=0, arg=3.1415927*2.0/(f->wsize_h), q=f->wind_h; i < n; )
		{
			*q++ = (float) (.54 - .46 * cos((half + (double)i++) * arg));
		}
	}
	/* If preemphasis is to be performed,  this assumes that there are n+1 valid
	samples in the input buffer (din). */
	if(preemp != 0.0) {
		for(i=n, p=din+1, q=f->wind_h; i--; )
		{
			*dout++ = (float) (*q++ * ((float)(*p++) - (preemp * *din++)));
		}
	} else {
		for(i=n, q=f->wind_h; i--; )
		{
			*dout++ = *q++ * *din++;
		}
	}
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* Generate a cos^4 window, if one does not already exist. */
void wtk_f0_xcwindow(wtk_f0_t *f,register float *din,register float * dout,register int n,register float  preemp)
{
	register int i;
	register float *p;
	//static int wsize_c = 0;
	//static float *wind_c=NULL;
	register float *q, co;

	if(f->wsize_c != n)
	{		/* Need to create a new cos**4 window? */
		register double arg, half=0.5;

		if(f->wind_c)
		{
			f->wind_c = (float*)ckrealloc((void *)f->wind_c,n*sizeof(float));
		}else
		{
			f->wind_c = (float*)ckalloc(n*sizeof(float));
		}
		f->wsize_c = n;
		for(i=0, arg=3.1415927*2.0/(f->wsize_c), q=f->wind_c; i < n; ) {
			co = (float) (half*(1.0 - cos((half + (double)i++) * arg)));
			*q++ = co * co * co * co;
		}
	}
	/* If preemphasis is to be performed,  this assumes that there are n+1 valid
	samples in the input buffer (din). */
	if(preemp != 0.0) {
		for(i=n, p=din+1, q=f->wind_c; i--; )
		{
			*dout++ = (float) (*q++ * ((float)(*p++) - (preemp * *din++)));
		}
	} else {
		for(i=n, q=f->wind_c; i--; )
		{
			*dout++ = *q++ * *din++;
		}
	}
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* Generate a Hanning window, if one does not already exist. */
void wtk_f0_xhnwindow(wtk_f0_t *f,register float *din, register float *dout, register int n, register float preemp)
{
	register int i;
	register float *p;
	register float *q;

	if(f->wsize_hn != n) {		/* Need to create a new Hanning window? */
		register double arg, half=0.5;

		if(f->wind_hn) f->wind_hn = (float*)ckrealloc((void *)f->wind_hn,n*sizeof(float));
		else f->wind_hn = (float*)ckalloc(n*sizeof(float));
		f->wsize_hn = n;
		for(i=0, arg=3.1415927*2.0/(f->wsize_hn), q=f->wind_hn; i < n; )
			*q++ = (float) (half - half * cos((half + (double)i++) * arg));
	}
	/* If preemphasis is to be performed,  this assumes that there are n+1 valid
	samples in the input buffer (din). */
	if(preemp != 0.0) {
		for(i=n, p=din+1, q=f->wind_hn; i--; )
			*dout++ = (float) (*q++ * ((float)(*p++) - (preemp * *din++)));
	} else {
		for(i=n, q=f->wind_hn; i--; )
			*dout++ = *q++ * *din++;
	}
}

int wtk_f0_window(wtk_f0_t *f,register float *din, register float *dout, register int n, register float preemp,int type)
{
	switch(type) {
  case 0:			/* rectangular */
	  xrwindow(din, dout, n, preemp);
	  break;
  case 1:			/* Hamming */
	  wtk_f0_xhwindow(f,din, dout, n, preemp);
	  break;
  case 2:			/* cos^4 */
	  wtk_f0_xcwindow(f,din, dout, n, preemp);
	  break;
  case 3:			/* Hanning */
	  wtk_f0_xhnwindow(f,din, dout, n, preemp);
	  break;
  default:
	  return(FALSE);
	}
	return(TRUE);
}

int wtk_f0_xlpc(wtk_f0_t *f,int lpc_ord,float lpc_stabl,int wsize,float *data,float *lpca,float *ar,float *lpck,float *normerr,float *rms,float preemp,int type)
{
	float rho[BIGSORD+1], k[BIGSORD], a[BIGSORD+1],*r,*kp,*ap,en,er,wfact=1.0;

	if((wsize <= 0) || (!data) || (lpc_ord > BIGSORD)) return(FALSE);

	if(f->nwind != wsize)
	{
		if(f->dwind)
		{
			f->dwind = (float*)ckrealloc((void *)f->dwind,wsize*sizeof(float));
		}else
		{
			f->dwind = (float*)ckalloc(wsize*sizeof(float));
		}
		if(!f->dwind) {
			return(FALSE);
		}
		f->nwind = wsize;
	}

	wtk_f0_window(f,data, f->dwind, wsize, preemp, type);
	if(!(r = ar)) r = rho;	/* Permit optional return of the various */
	if(!(kp = lpck)) kp = k;	/* coefficients and intermediate results. */
	if(!(ap = lpca)) ap = a;
	xautoc( wsize, f->dwind, lpc_ord, r, &en );
	if(lpc_stabl > 1.0) {	/* add a little to the diagonal for stability */
		int i;
		float ffact;
		ffact = (float) (1.0/(1.0 + exp((-lpc_stabl/20.0) * log(10.0))));
		for(i=1; i <= lpc_ord; i++) rho[i] = ffact * r[i];
		*rho = *r;
		r = rho;
		if(ar)
			for(i=0;i<=lpc_ord; i++) ar[i] = r[i];
	}
	xdurbin ( r, kp, &ap[1], lpc_ord, &er);
	switch(type) {		/* rms correction for window */
  case 0:
	  wfact = 1.0;		/* rectangular */
	  break;
  case 1:
	  wfact = .630397f;		/* Hamming */
	  break;
  case 2:
	  wfact = .443149f;		/* (.5 - .5*cos)^4 */
	  break;
  case 3:
	  wfact = .612372f;		/* Hanning */
	  break;
	}
	*ap = 1.0;
	if(rms) *rms = en/wfact;
	if(normerr) *normerr = er;
	return(TRUE);
}

/*++++++++++++++++++sigproc.c++++++++++++++++++++++++++*/
/* Return a time-weighting window of type type and length n in dout.
* Dout is assumed to be at least n elements long.  Type is decoded in
* the switch statement below.
*/
int wtk_f0_xget_window(wtk_f0_t *f,register float *dout, register int n, register int type)
{
	float preemp = 0.0;

	if(n > f->n0)
	{
		register float *p;
		register int i;

		if(f->din) ckfree((void *)f->din);
		f->din = NULL;
		if(!(f->din = (float*)ckalloc(sizeof(float)*n))) {
			return(FALSE);
		}
		for(i=0, p=f->din; i++ < n; ) *p++ = 1;
		f->n0 = n;
	}
	return(wtk_f0_window(f,f->din, dout, n, preemp, type));
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* Compute the time-weighted RMS of a size segment of data.  The data
* is weighted by a window of type w_type before RMS computation.  w_type
* is decoded above in window().
*/
float wtk_f0_wind_energy(wtk_f0_t *f0,register float *data,register int size,register int w_type)
{
	register float *dp, sum, f;
	register int i;

	if(f0->nwind_e < size)
	{
		if(f0->dwind_e) f0->dwind_e = (float*)ckrealloc((void *)f0->dwind_e,size*sizeof(float));
		else f0->dwind_e = (float*)ckalloc(size*sizeof(float));
		if(!f0->dwind_e) {
			return(0.0);
		}
	}
	if(f0->nwind_e != size) {
		wtk_f0_xget_window(f0,f0->dwind_e, size, w_type);
		f0->nwind_e = size;
	}
	for(i=size, dp = f0->dwind_e, sum = 0.0; i-- > 0; ) {
		f = *dp++ * (float)(*data++);
		sum += f*f;
	}
	return((float)sqrt((double)(sum/size)));
}

/*--------------------------------------------------------------------*/
int wtk_f0_retrieve_windstat(wtk_f0_t *f,float   *rho, int order, float   *err, float   *rms)
{
	Windstat wstat;
	int i;

	if(f->wReuse)
	{
		wstat = f->windstat[0];
		for(i=0; i<=order; i++) rho[i] = wstat.rho[i];
		*err = wstat.err;
		*rms = wstat.rms;
		return 1;
	}
	else return 0;
}

/*--------------------------------------------------------------------*/
/* push window stat to stack, and pop the oldest one */

int wtk_f0_save_windstat(wtk_f0_t *f,float   *rho, int order,float err,float rms)
{
	int i,j;

	if(f->wReuse > 1)
	{               /* push down the stack */
		for(j=1; j<f->wReuse; j++){
			for(i=0;i<=order; i++) f->windstat[j-1].rho[i] = f->windstat[j].rho[i];
			f->windstat[j-1].err = f->windstat[j].err;
			f->windstat[j-1].rms = f->windstat[j].rms;
		}
		for(i=0;i<=order; i++) f->windstat[f->wReuse-1].rho[i] = rho[i]; /*save*/
		f->windstat[f->wReuse-1].err = (float) err;
		f->windstat[f->wReuse-1].rms = (float) rms;
		return 1;
	} else if (f->wReuse == 1) {
		for(i=0;i<=order; i++) f->windstat[0].rho[i] = rho[i];  /* save */
		f->windstat[0].err = (float) err;
		f->windstat[0].rms = (float) rms;
		return 1;
	} else
		return 0;
}

float wtk_f0_get_similarity(wtk_f0_t *f,int order,int size,float *pdata,float *cdata,float *rmsa,float *rms_ratio, float pre, float stab,int w_type,int init)
{
	float rho3[BIGSORD+1], err3, rms3, rmsd3, b0, t, a2[BIGSORD+1],
		rho1[BIGSORD+1], a1[BIGSORD+1], b[BIGSORD+1], err1, rms1, rmsd1;
	//float xitakura(), wind_energy();
	//void xa_to_aca ();
	//int xlpc();

	err3=0;
	/* (In the lpc() calls below, size-1 is used, since the windowing and
	preemphasis function assumes an extra point is available in the
	input data array.  This condition is apparently no longer met after
	Derek's modifications.) */

	/* get current window stat */
	wtk_f0_xlpc(f,order, stab, size-1, cdata,
		a2, rho3, (float *) NULL, &err3, &rmsd3, pre, w_type);
	rms3 = wtk_f0_wind_energy(f,cdata, size, w_type);

	if(!init)
	{
		/* get previous window stat */
		if( !wtk_f0_retrieve_windstat(f,rho1, order, &err1, &rms1))
		{
			wtk_f0_xlpc(f,order, stab, size-1, pdata,
				a1, rho1, (float *) NULL, &err1, &rmsd1, pre, w_type);
			rms1 = wtk_f0_wind_energy(f,pdata, size, w_type);
		}
		xa_to_aca(a2+1,b,&b0,order);
		t = xitakura(order,b,&b0,rho1+1,&err1) - .8f;
		if(rms1 > 0.0)
			*rms_ratio = (0.001f + rms3)/rms1;
		else
			if(rms3 > 0.0)
				*rms_ratio = 2.0;	/* indicate some energy increase */
			else
				*rms_ratio = 1.0;	/* no change */
	} else {
		*rms_ratio = 1.0;
		t = 10.0;
	}
	*rmsa = rms3;
	wtk_f0_save_windstat(f, rho3, order, err3, rms3);
	//wtk_debug("t=%f\n",t);
	return((float)(0.2/t));
}


Stat* wtk_f0_get_stationarity(wtk_f0_t *f,float *fdata,double freq,int buff_size,int nframes,int frame_step,int first_time)
{
	//static int nframes_old = 0, memsize;
	float preemp = 0.4f, stab = 30.0f;
	float *p, *q, *r, *datend;
	int ind, i, j, m, size, order, agap, w_type = 3;

	agap = (int) (STAT_AINT *freq);
	size = (int) (STAT_WSIZE * freq);
	ind = (agap - size) / 2;

	if( f->nframes_old < nframes || !f->stat || first_time)
	{
		/* move this to init_dp_f0() later */
		f->nframes_old = nframes;
		if(f->stat)
		{
			free((char *) f->stat->stat);
			free((char *) f->stat->rms);
			free((char *) f->stat->rms_ratio);
			free((char *) f->stat);
			f->stat=NULL;
		}
		if (f->mem){ free((void *)f->mem); f->mem=NULL;}
		f->stat = (Stat *) malloc(sizeof(Stat));
		/*    spsassert(stat,"stat malloc failed in get_stationarity");*/
		f->stat->stat = (float*)calloc(nframes,sizeof(float));
		/*    spsassert(stat->stat,"stat->stat malloc failed in get_stationarity");*/
		f->stat->rms = (float*)calloc(nframes,sizeof(float));
		/*    spsassert(stat->rms,"stat->rms malloc failed in get_stationarity");*/
		f->stat->rms_ratio = (float*)calloc(nframes,sizeof(float));
		/*    spsassert(stat->rms_ratio,"stat->ratio malloc failed in get_stationarity");*/
		f->memsize = (int) (STAT_WSIZE * freq) + (int) (STAT_AINT * freq);
		f->mem = (float *) calloc(f->memsize,sizeof(float));
		/*    spsassert(mem, "mem malloc failed in get_stationarity()");*/
		for(j=0; j<f->memsize; j++) f->mem[j] = 0;
	}

	if(nframes == 0) return(f->stat);

	q = fdata + ind;
	datend = fdata + buff_size;

	if((order = (int) (2.0 + (freq/1000.0))) > BIGSORD)
	{
		order = BIGSORD;
	}
	/* prepare for the first frame */
	for(j=f->memsize/2, i=0; j<f->memsize; j++, i++)
	{
		f->mem[j] = fdata[i];
	}

	/* never run over end of frame, should already taken care of when read */
	for(j=0, p = q - agap; j < nframes; j++, p += frame_step, q += frame_step)
	{
		if( (p >= fdata) && (q >= fdata) && ( q + size <= datend) )
			f->stat->stat[j] = wtk_f0_get_similarity(f,order,size, p, q,
			&(f->stat->rms[j]),
			&(f->stat->rms_ratio[j]),preemp,
			stab,w_type, 0);
		else
		{
			if(first_time)
			{
				if( (p < fdata) && (q >= fdata) && (q+size <=datend) )
				{
					f->stat->stat[j] = wtk_f0_get_similarity(f,order,size, NULL, q,
					&(f->stat->rms[j]),
					&(f->stat->rms_ratio[j]),
					preemp,stab,w_type, 1);
				}
				else
				{
					f->stat->rms[j] = 0.0;
					f->stat->stat[j] = 0.01f * 0.2f;   /* a big transition */
					f->stat->rms_ratio[j] = 1.0;   /* no amplitude change */
				}
			} else {
				if( (p<fdata) && (q+size <=datend) )
				{
					f->stat->stat[j] = wtk_f0_get_similarity(f,order,size, f->mem,
						f->mem + (f->memsize/2) + ind,
						&(f->stat->rms[j]),
						&(f->stat->rms_ratio[j]),
						preemp, stab,w_type, 0);
					/* prepare for the next frame_step if needed */
					if(p + frame_step < fdata )
					{
						for( m=0; m<(f->memsize-frame_step); m++)
						{
							f->mem[m] = f->mem[m+frame_step];
						}
						r = q + size;
						for( m=0; m<frame_step; m++)
						{
							f->mem[f->memsize-frame_step+m] = *r++;
						}
					}
				}
			}
		}
		//wtk_debug("v[%d]=%f\n",j,f->stat->stat[j]);
		//exit(0);
	}

	/* last frame, prepare for next call */
	for(j=(f->memsize/2)-1, p=fdata + (nframes * frame_step)-1; j>=0 && p>=fdata; j--)
	{
		f->mem[j] = *p--;
	}
	return f->stat;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* Return a sequence based on the normalized crosscorrelation of the signal
in data.
*
data is the input speech array
size is the number of samples in each correlation
start is the first lag to compute (governed by the highest expected F0)
nlags is the number of cross correlations to compute (set by lowest F0)
engref is the energy computed at lag=0 (i.e. energy in ref. window)
maxloc is the lag at which the maximum in the correlation was found
maxval is the value of the maximum in the CCF over the requested lag interval
correl is the array of nlags cross-correlation coefficients (-1.0 to 1.0)
*
*/
void wtk_f0_crossf(wtk_f0_t *f,float *data,int size,int start,int nlags,float * engref, int *maxloc, float *maxval, float *correl)
{
	//static float *dbdata=NULL;
	//static int dbsize = 0;
	register float *dp, *ds, sum, st;
	register int j;
	register  float *dq, t, *p, engr, *dds, amax;
	register  double engc;
	int i, iloc, total;
	//int sizei, sizeo, maxsize;

	/* Compute mean in reference window and subtract this from the
	entire sequence.  This doesn't do too much damage to the data
	sequenced for the purposes of F0 estimation and removes the need for
	more principled (and costly) low-cut filtering. */
	if((total = size+start+nlags) > f->dbsize) {
		if(f->dbdata)
			ckfree((void *)f->dbdata);
		f->dbdata = NULL;
		f->dbsize = 0;
		if(!(f->dbdata = (float*)ckalloc(sizeof(float)*total))) {
			//Fprintf(stderr,"Allocation failure in crossf()\n");
			return;
		}
		f->dbsize = total;
	}
	for(engr=0.0, j=size, p=data; j--; ) engr += *p++;
	engr /= size;
	for(j=size+nlags+start, dq = f->dbdata, p=data; j--; )  *dq++ = *p++ - engr;

	//maxsize = start + nlags;
	//sizei = size + start + nlags + 1;
	//sizeo = nlags + 1;

	/* Compute energy in reference window. */
	for(j=size, dp=f->dbdata, sum=0.0; j--; ) {
		st = *dp++;
		sum += st * st;
	}

	*engref = engr = sum;
	if(engr > 0.0) {    /* If there is any signal energy to work with... */
		/* Compute energy at the first requested lag. */
		for(j=size, dp=f->dbdata+start, sum=0.0; j--; ) {
			st = *dp++;
			sum += st * st;
		}
		engc = sum;

		/* COMPUTE CORRELATIONS AT ALL OTHER REQUESTED LAGS. */
		for(i=0, dq=correl, amax=0.0, iloc = -1; i < nlags; i++) {
			for(j=size, sum=0.0, dp=f->dbdata, dds = ds = f->dbdata+i+start; j--; )
				sum += *dp++ * *ds++;
			*dq++ = t = (float) (sum/sqrt((double)(engc*engr))); /* output norm. CC */
			engc -= (double)(*dds * *dds); /* adjust norm. energy for next lag */
			if((engc += (double)(*ds * *ds)) < 1.0)
				engc = 1.0;		/* (hack: in case of roundoff error) */
			if(t > amax) {		/* Find abs. max. as we go. */
				amax = t;
				iloc = i+start;
			}
		}
		*maxloc = iloc;
		*maxval = amax;
	} else {	/* No energy in signal; fake reasonable return vals. */
		*maxloc = 0;
		*maxval = 0.0;
		for(p=correl,i=nlags; i-- > 0; )
			*p++ = 0.0;
	}
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* Return a sequence based on the normalized crosscorrelation of the
signal in data.  This is similar to crossf(), but is designed to
compute only small patches of the correlation sequence.  The length of
each patch is determined by nlags; the number of patches by nlocs, and
the locations of the patches is specified by the array locs.  Regions
of the CCF that are not computed are set to 0.
*
data is the input speech array
size is the number of samples in each correlation
start0 is the first (virtual) lag to compute (governed by highest F0)
nlags0 is the number of lags (virtual+actual) in the correlation sequence
nlags is the number of cross correlations to compute at each location
engref is the energy computed at lag=0 (i.e. energy in ref. window)
maxloc is the lag at which the maximum in the correlation was found
maxval is the value of the maximum in the CCF over the requested lag interval
correl is the array of nlags cross-correlation coefficients (-1.0 to 1.0)
locs is an array of indices pointing to the center of a patches where the
cross correlation is to be computed.
nlocs is the number of correlation patches to compute.
*
*/
void wtk_f0_crossfi(wtk_f0_t *f,float *data,int size,int start0,int nlags0,int nlags, float *engref, int *maxloc, float *maxval,float * correl, int *locs,int nlocs)
{
	//static float *dbdata_i=NULL;
	//static int dbsize_i = 0;
	register float *dp, *ds, sum, st;
	register int j;
	register  float *dq, t, *p, engr, *dds, amax;
	register  double engc;
	int i, iloc, start, total;

	/* Compute mean in reference window and subtract this from the
	entire sequence. */
	if((total = size+start0+nlags0) > f->dbsize_i) {
		if(f->dbdata_i)
			ckfree((void *)f->dbdata_i);
		f->dbdata_i = NULL;
		f->dbsize_i = 0;
		if(!(f->dbdata_i = (float*)ckalloc(sizeof(float)*total))) {
			//Fprintf(stderr,"Allocation failure in crossfi()\n");
			return;
		}
		f->dbsize_i = total;
	}
	for(engr=0.0, j=size, p=data; j--; ) engr += *p++;
	engr /= size;
	for(j=size+nlags0+start0, dq = f->dbdata_i, p=data; j--; ) {
		*dq++ = *p++ - engr;
	}

	/* Zero the correlation output array to avoid confusing the peak
	picker (since all lags will not be computed). */
	for(p=correl,i=nlags0; i-- > 0; )
		*p++ = 0.0;

	/* compute energy in reference window */
	for(j=size, dp=f->dbdata_i, sum=0.0; j--; ) {
		st = *dp++;
		sum += st * st;
	}

	*engref = engr = sum;
	amax=0.0;
	iloc = -1;
	if(engr > 0.0) {
		for( ; nlocs > 0; nlocs--, locs++ ) {
			start = *locs - (nlags>>1);
			if(start < start0)
				start = start0;
			dq = correl + start - start0;
			/* compute energy at first requested lag */
			for(j=size, dp=f->dbdata_i+start, sum=0.0; j--; ) {
				st = *dp++;
				sum += st * st;
			}
			engc = sum;

			/* COMPUTE CORRELATIONS AT ALL REQUESTED LAGS */
			for(i=0; i < nlags; i++) {
				for(j=size, sum=0.0, dp=f->dbdata_i, dds = ds = f->dbdata_i+i+start; j--; )
					sum += *dp++ * *ds++;
				if(engc < 1.0)
					engc = 1.0;		/* in case of roundoff error */
				*dq++ = t = (float) (sum/sqrt((double)(10000.0 + (engc*engr))));
				engc -= (double)(*dds * *dds);
				engc += (double)(*ds * *ds);
				if(t > amax) {
					amax = t;
					iloc = i+start;
				}
			}
		}
		*maxloc = iloc;
		*maxval = amax;
	} else {
		*maxloc = 0;
		*maxval = 0.0;
	}
}

/* ----------------------------------------------------------------------- */
void wtk_f0_get_fast_cands(wtk_f0_t *f,float *fdata, float *fdsdata,int ind,int step,int size, int	dec,int start,int nlags, float *engref,int *maxloc, float *maxval,Cross *cp, float *peaks, int *locs,int *ncand, F0_params *par)
{
	int decind, decstart, decnlags, decsize, i, j, *lp;
	float *corp, xp, yp, lag_wt;
	register float *pe;

	lag_wt = par->lag_weight/nlags;
	decnlags = 1 + (nlags/dec);
	if((decstart = start/dec) < 1) decstart = 1;
	decind = (ind * step)/dec;
	decsize = 1 + (size/dec);
	corp = cp->correl;

	wtk_f0_crossf(f,fdsdata + decind, decsize, decstart, decnlags, engref, maxloc,
		maxval, corp);
	cp->maxloc = *maxloc;	/* location of maximum in correlation */
	cp->maxval = *maxval;	/* max. correlation value (found at maxloc) */
	cp->rms = (float) sqrt(*engref/size); /* rms in reference window */
	cp->firstlag = decstart;

	get_cand(cp,peaks,locs,decnlags,ncand,par->cand_thresh); /* return high peaks in xcorr */

	/* Interpolate to estimate peak locations and values at high sample rate. */
	for(i = *ncand, lp = locs, pe = peaks; i--; pe++, lp++) {
		j = *lp - decstart - 1;
		peak(&corp[j],&xp,&yp);
		*lp = (*lp * dec) + (int)(0.5+(xp*dec)); /* refined lag */
		*pe = yp*(1.0f - (lag_wt* *lp)); /* refined amplitude */
	}

	if(*ncand >= par->n_cands) {	/* need to prune candidates? */
		register int *loc, *locm, lt;
		register float smaxval, *pem;
		register int outer, inner, lim;
		for(outer=0, lim = par->n_cands-1; outer < lim; outer++)
			for(inner = *ncand - 1 - outer,
				pe = peaks + (*ncand) -1, pem = pe-1,
				loc = locs + (*ncand) - 1, locm = loc-1;
		inner--;
		pe--,pem--,loc--,locm--)
			if((smaxval = *pe) > *pem) {
				*pe = *pem;
				*pem = smaxval;
				lt = *loc;
				*loc = *locm;
				*locm = lt;
			}
		*ncand = par->n_cands-1;  /* leave room for the unvoiced hypothesis */
	}
	wtk_f0_crossfi(f,fdata + (ind * step), size, start, nlags, 7, engref, maxloc,
		maxval, corp, locs, *ncand);

	cp->maxloc = *maxloc;	/* location of maximum in correlation */
	cp->maxval = *maxval;	/* max. correlation value (found at maxloc) */
	cp->rms = (float) sqrt(*engref/size); /* rms in reference window */
	cp->firstlag = start;
	get_cand(cp,peaks,locs,nlags,ncand,par->cand_thresh); /* return high peaks in xcorr */
	if(*ncand >= par->n_cands) {	/* need to prune candidates again? */
		register int *loc, *locm, lt;
		register float smaxval, *pe, *pem;
		register int outer, inner, lim;
		for(outer=0, lim = par->n_cands-1; outer < lim; outer++)
			for(inner = *ncand - 1 - outer,
				pe = peaks + (*ncand) -1, pem = pe-1,
				loc = locs + (*ncand) - 1, locm = loc-1;
		inner--;
		pe--,pem--,loc--,locm--)
			if((smaxval = *pe) > *pem) {
				*pe = *pem;
				*pem = smaxval;
				lt = *loc;
				*loc = *locm;
				*locm = lt;
			}
		*ncand = par->n_cands - 1;  /* leave room for the unvoiced hypothesis */
	}
}

int wtk_f0_dp_f0(wtk_f0_t *f,float *fdata,int buff_size,int sdstep,double freq, int *vecsize)
{
	float  maxval, engref, *sta, *rms_ratio, *dsdata; //*downsample();
	register float ttemp, ftemp, ft1, ferr=0, err=0, errmin=0;
	register int  i, j, k, loc1, loc2;
	int   nframes, maxloc, ncand, ncandp, minloc,
		decimate, samsds;
	Stat *stat = NULL;
	int ret=-1;

	nframes = get_Nframes((long) buff_size, f->pad, f->step); /* # of whole frames */
	/* Now downsample the signal for coarse peak estimates. */
	decimate = (int)(freq/2000.0);    /* downsample to about 2kHz */
	if (decimate <= 1)
	{
		dsdata = fdata;
	}else
	{
		samsds = ((nframes-1) * f->step + f->ncomp) / decimate;
		dsdata = wtk_f0_downsample(f,fdata, buff_size, sdstep, freq, &samsds, decimate,f->first_time,f->last_time);
		if (!dsdata) {goto end;}
	}

	/* Get a function of the "stationarity" of the speech signal. */
	stat = wtk_f0_get_stationarity(f,fdata, freq, buff_size, nframes, f->step, f->first_time);
	if (!stat){goto end;}
	sta = stat->stat;
	rms_ratio = stat->rms_ratio;

	/***********************************************************************/
	/* MAIN FUNDAMENTAL FREQUENCY ESTIMATION LOOP */
	/***********************************************************************/
	if(!f->first_time && nframes > 0)
	{
		f->headF = f->headF->next;
	}
	for(i = 0; i < nframes; i++) {

		/* NOTE: This buffer growth provision is probably not necessary.
		It was put in (with errors) by Derek Lin and apparently never
		tested.  My tests and analysis suggest it is completely
		superfluous. DT 9/5/96 */
		/* Dynamically allocating more space for the circular buffer */
		f->headF->rms = stat->rms[i];
		wtk_f0_get_fast_cands(f,fdata, dsdata, i, f->step, f->size, decimate, f->start,
			f->nlags, &engref, &maxloc,
			&maxval, f->headF->cp, f->peaks, f->locs, &ncand, f->par);

		/*    Move the peak value and location arrays into the dp structure */
		{
			register float *ftp1, *ftp2;
			register short *sp1;
			register int *sp2;

			for(ftp1 = f->headF->dp->pvals, ftp2 = f->peaks,
				sp1 = f->headF->dp->locs, sp2 = f->locs, j=ncand; j--; ) {
					*ftp1++ = *ftp2++;
					*sp1++ = *sp2++;
			}
			*sp1 = -1;		/* distinguish the UNVOICED candidate */
			*ftp1 = maxval;
			f->headF->dp->mpvals[ncand] = f->vbias+maxval; /* (high cost if cor. is high)*/
		}

		/* Apply a lag-dependent weight to the peaks to encourage the selection
		of the first major peak.  Translate the modified peak values into
		costs (high peak ==> low cost). */
		for(j=0; j < ncand; j++)
		{
			ftemp = 1.0f - ((float)f->locs[j] * f->lagwt);
			f->headF->dp->mpvals[j] = 1.0f - (f->peaks[j] * ftemp);
		}
		ncand++;			/* include the unvoiced candidate */
		f->headF->dp->ncands = ncand;

		/*********************************************************************/
		/*    COMPUTE THE DISTANCE MEASURES AND ACCUMULATE THE COSTS.       */
		/*********************************************************************/

		ncandp = f->headF->prev->dp->ncands;
		for(k=0; k<ncand; k++){	/* for each of the current candidates... */
			minloc = 0;
			errmin = FLT_MAX;
			if((loc2 = f->headF->dp->locs[k]) > 0) { /* current cand. is voiced */
				for(j=0; j<ncandp; j++){ /* for each PREVIOUS candidate... */
					/*    Get cost due to inter-frame period change. */
					loc1 = f->headF->prev->dp->locs[j];
					//wtk_debug("%f/%d\n",f->freqwt,loc1 );
					if (loc1 > 0) { /* prev. was voiced */
						ftemp = (float) log(((double) loc2) / loc1);
						ttemp = (float) fabs(ftemp);
						//wtk_debug("%f\n",ttemp);
						ft1 = (float) (f->fdouble + fabs(ftemp + f->ln2));
						if (ttemp > ft1)
							ttemp = ft1;
						ft1 = (float) (f->fdouble + fabs(ftemp - f->ln2));
						if (ttemp > ft1)
							ttemp = ft1;
						//wtk_debug("%f\n",ttemp);
						//wtk_debug("%f\n",f->freqwt);
						ferr = ttemp * f->freqwt;
					} else {		/* prev. was unvoiced */
//						wtk_debug("cost=%f\n",f->tcost);
//						wtk_debug("fact=%f\n",f->tfact_s);
						//wtk_debug("v[%d]=%f\n",i,sta[i]);
//						wtk_debug("%f\n",f->tfact_a );
//						wtk_debug("%f\n",rms_ratio[i]);
						ferr = f->tcost + (f->tfact_s * sta[i]) + (f->tfact_a / rms_ratio[i]);
					}
					/*    Add in cumulative cost associated with previous peak. */
					//wtk_debug("%f/%d\n",ferr,loc1);
					//wtk_debug("%f\n",f->headF->prev->dp->dpvals[j]);
					err = ferr + f->headF->prev->dp->dpvals[j];
					if(err < errmin){	/* find min. cost */
						errmin = err;
						minloc = j;
					}
				}
			} else {			/* this is the unvoiced candidate */
				for(j=0; j<ncandp; j++){ /* for each PREVIOUS candidate... */

					/*    Get voicing transition cost. */
					if (f->headF->prev->dp->locs[j] > 0) { /* previous was voiced */
						ferr = f->tcost + (f->tfact_s * sta[i]) + (f->tfact_a * rms_ratio[i]);
					}
					else
						ferr = 0.0;
					/*    Add in cumulative cost associated with previous peak. */
					err = ferr + f->headF->prev->dp->dpvals[j];
					if(err < errmin){	/* find min. cost */
						errmin = err;
						minloc = j;
					}
				}
			}
			/* Now have found the best path from this cand. to prev. frame */
			if (f->first_time && i==0) {		/* this is the first frame */
				f->headF->dp->dpvals[k] = f->headF->dp->mpvals[k];
				f->headF->dp->prept[k] = 0;
			} else {
				f->headF->dp->dpvals[k] = errmin + f->headF->dp->mpvals[k];
				f->headF->dp->prept[k] = minloc;
			}
		} /*    END OF THIS DP FRAME */

		if (i < nframes - 1)
			f->headF = f->headF->next;

	} /* end for (i ...) */

	/***************************************************************/
	/* DONE WITH FILLING DP STRUCTURES FOR THE SET OF SAMPLED DATA */
	/*    NOW FIND A CONVERGED DP PATH                             */
	/***************************************************************/

	*vecsize = 0;			/* # of output frames returned */

	f->num_active_frames += nframes;

	if( f->num_active_frames >= f->size_frame_hist  || f->last_time ){
		Frame *frm;
		int  num_paths, best_cand, frmcnt, checkpath_done = 1;
		float patherrmin;

		patherrmin = FLT_MAX;
		best_cand = 0;
		num_paths = f->headF->dp->ncands;

		/* Get the best candidate for the final frame and initialize the
		paths' backpointers. */
		frm = f->headF;
		for(k=0; k < num_paths; k++) {
			if (patherrmin > f->headF->dp->dpvals[k]){
				patherrmin = f->headF->dp->dpvals[k];
				best_cand = k;	/* index indicating the best candidate at a path */
			}
			f->pcands[k] = frm->dp->prept[k];
		}

		if(f->last_time){     /* Input data was exhausted. force final outputs. */
			f->cmpthF = f->headF;		/* Use the current frame as starting point. */
		} else
		{
			/* Starting from the most recent frame, trace back each candidate's
			best path until reaching a common candidate at some past frame. */
			frmcnt = 0;
			while (1) {
				frm = frm->prev;
				frmcnt++;
				checkpath_done = 1;
				for(k=1; k < num_paths; k++){ /* Check for convergence. */
					if(f->pcands[0] != f->pcands[k])
						checkpath_done = 0;
				}
				if( ! checkpath_done) { /* Prepare for checking at prev. frame. */
					for(k=0; k < num_paths; k++){
						f->pcands[k] = frm->dp->prept[f->pcands[k]];
					}
				} else {	/* All paths have converged. */
					f->cmpthF = frm;
					best_cand = f->pcands[0];
					break;
				}
				if(frm == f->tailF){	/* Used all available data? */
					if( f->num_active_frames < f->size_frame_out) { /* Delay some more? */
						checkpath_done = 0; /* Yes, don't backtrack at this time. */
						f->cmpthF = NULL;
					} else {		/* No more delay! Force best-guess output. */
						checkpath_done = 1;
						f->cmpthF = f->headF;
						/*	    Fprintf(stderr,
						"WARNING: no converging path found after going back %d frames, will use the lowest cost path\n",num_active_frames);*/
					}
					break;
				} /* end if (frm ...) */
			}	/* end while (1) */
		} /* end if (last_time) ... else */

		/*************************************************************/
		/* BACKTRACKING FROM cmpthF (best_cand) ALL THE WAY TO tailF    */
		/*************************************************************/
		i = 0;
		frm = f->cmpthF;	/* Start where convergence was found (or faked). */
		while( frm != f->tailF->prev && checkpath_done)
		{
			f->rms_speech[i] = frm->rms;
			f->acpkp[i] =  frm->dp->pvals[best_cand];
			loc1 = frm->dp->locs[best_cand];
			f->vuvp[i] = 1.0;
			best_cand = frm->dp->prept[best_cand];
			ftemp = (float) loc1;
			if(loc1 > 0) {		/* Was f0 actually estimated for this frame? */
				if (loc1 > f->start && loc1 < f->stop) { /* loc1 must be a local maximum. */
					float cormax, cprev, cnext, den;

					j = loc1 - f->start;
					cormax = frm->cp->correl[j];
					cprev = frm->cp->correl[j+1];
					cnext = frm->cp->correl[j-1];
					den = (float) (2.0 * ( cprev + cnext - (2.0 * cormax) ));
					/*
					* Only parabolic interpolate if cormax is indeed a local
					* turning point. Find peak of curve that goes though the 3 points
					*/

					if (fabs(den) > 0.000001)
						ftemp += 2.0f - ((((5.0f*cprev)+(3.0f*cnext)-(8.0f*cormax))/den));
				}
				f->f0p[i] = (float) (freq/ftemp);
			} else {		/* No valid estimate; just fake some arbitrary F0. */
				f->f0p[i] = 0;
				f->vuvp[i] = 0.0;
			}
			frm = frm->prev;

			/* f0p[i] starts from the most recent one */
			/* Need to reverse the order in the calling function */
			i++;
		} /* end while() */
		if (checkpath_done){
			*vecsize = i;
			f->tailF = f->cmpthF->next;
			f->num_active_frames -= *vecsize;
		}
	} /* end if() */
	if(f->first_time) f->first_time = 0;
	ret=0;
end:
	return ret;
}


void wtk_f0_print(wtk_f0_t *f0)
{
	float *f;
	float *e;
	float *k;
	int i;

	//wtk_debug("================= f0 ==================\n");
	f=(float*)f0->f0_array->slot;
	e=(float*)f0->f0e_array->slot;
	k=(float*)f0->acpkp_array->slot;
	for(i=0;i<f0->f0_array->nslot;++i)
	{
		//printf("%f\n",f[i]);
		//if(1)
		//if(f[i]>1.0)
		{
			//printf("%f\t%f\n",f[i],e[i]);
			printf(" %f %f %f ",f[i],e[i],k[i]);
		}
		if(i+1==f0->f0_array->nslot)
		{
			printf("]\n");
		}else
		{
			printf("\n");
		}
	}
}

void wtk_f0_print_file(wtk_f0_t *f0,FILE *log)
{
	float *f;
	//float *e;
	int i;

	f=(float*)f0->f0_array->slot;
	//e=(float*)f0->f0e_array->slot;
	//fprintf(log,"f0\tenergy\n");
	fprintf(log,"f0\n");
	for(i=0;i<f0->f0_array->nslot;++i)
	{
		//printf("%f\n",f[i]);
		fprintf(log,"%f\n",f[i]);
		//fprintf(log,"%f\t%f\n",f[i],e[i]);
	}
}

void wtk_f0_set_notify(wtk_f0_t *f0,void *ths,wtk_f0_notify_t notify)
{
	f0->notify_ths=ths;
	f0->notify=notify;
}


