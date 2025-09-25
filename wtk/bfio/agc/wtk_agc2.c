#include "wtk_agc2.h"

wtk_agc2_t* wtk_agc2_new(wtk_agc2_cfg_t *cfg)
{
	wtk_agc2_t *agc2;

	agc2=(wtk_agc2_t *)wtk_malloc(sizeof(wtk_agc2_t));
	agc2->cfg=cfg;
	agc2->ths=NULL;
	agc2->notify=NULL;

	agc2->mic=wtk_strbufs_new(agc2->cfg->nmicchannel);
	agc2->nbin=cfg->wins/2+1;
	agc2->analysis_window=wtk_malloc(sizeof(float)*cfg->wins);///2);
	agc2->synthesis_window=wtk_malloc(sizeof(float)*cfg->wins);///2);
	agc2->analysis_mem=wtk_float_new_p2(cfg->nmicchannel, agc2->nbin-1);
	agc2->synthesis_mem=wtk_malloc(sizeof(float)*(agc2->nbin-1));
	agc2->rfft=wtk_drft_new2(cfg->wins);
	agc2->rfft_in=(float*)wtk_malloc(sizeof(float)*(cfg->wins));

	agc2->fft=wtk_complex_new_p2(cfg->nmicchannel, agc2->nbin);

	agc2->fftx=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*agc2->nbin);

	agc2->qmmse=NULL;
	if(cfg->use_qmmse){
		agc2->qmmse=wtk_qmmse_new(&(cfg->qmmse));
	}

	agc2->audgram=NULL;
	agc2->fbg=NULL;
	agc2->state=NULL;
	agc2->D=NULL;
	agc2->E=NULL;
	if(cfg->use_tf_agc){
		agc2->audgram = (float *)wtk_malloc(sizeof(float)*cfg->nfilts);
		agc2->fbg = (float *)wtk_malloc(sizeof(float)*cfg->nfilts);
		agc2->state = (float *)wtk_malloc(sizeof(float)*cfg->nfilts);
		agc2->D = (float *)wtk_malloc(sizeof(float)*agc2->nbin);
		agc2->E = (float *)wtk_malloc(sizeof(float)*agc2->nbin);
	}

	agc2->eq=NULL;
	if(cfg->use_eq)
	{
		agc2->eq=wtk_equalizer_new(&(cfg->eq));
	}

	agc2->out=wtk_malloc(sizeof(float)*(agc2->nbin-1));

	wtk_agc2_reset(agc2);

	return agc2;
}

void wtk_agc2_delete(wtk_agc2_t *agc2)
{
	wtk_strbufs_delete(agc2->mic,agc2->cfg->nmicchannel);

	wtk_free(agc2->analysis_window);
	wtk_free(agc2->synthesis_window);
	wtk_float_delete_p2(agc2->analysis_mem, agc2->cfg->nmicchannel);
	wtk_free(agc2->synthesis_mem);
	wtk_free(agc2->rfft_in);
	wtk_drft_delete2(agc2->rfft);
	wtk_complex_delete_p2(agc2->fft, agc2->cfg->nmicchannel);

	if(agc2->qmmse)
	{
		wtk_qmmse_delete(agc2->qmmse);
	}
	if(agc2->audgram)
	{
		wtk_free(agc2->audgram);
	}
	if(agc2->fbg)
	{
		wtk_free(agc2->fbg);
	}
	if(agc2->state)
	{
		wtk_free(agc2->state);
	}
	if(agc2->D)
	{
		wtk_free(agc2->D);
	}
	if(agc2->E)
	{
		wtk_free(agc2->E);
	}

	if(agc2->eq)
	{
		wtk_equalizer_delete(agc2->eq);
	}

	wtk_free(agc2->fftx);

	wtk_free(agc2->out);

	wtk_free(agc2);
}


void wtk_agc2_start(wtk_agc2_t *agc2)
{
}

void wtk_agc2_reset(wtk_agc2_t *agc2)
{
	int wins=agc2->cfg->wins;
	int i;

	wtk_strbufs_reset(agc2->mic,agc2->cfg->nmicchannel);

	for (i=0;i<wins;++i)
	{
		agc2->analysis_window[i] = sin((0.5+i)*PI/(wins));
	}
	wtk_drft_init_synthesis_window(agc2->synthesis_window, agc2->analysis_window, wins);

	wtk_float_zero_p2(agc2->analysis_mem, agc2->cfg->nmicchannel, (agc2->nbin-1));
	memset(agc2->synthesis_mem, 0, sizeof(float)*(agc2->nbin-1));

	wtk_complex_zero_p2(agc2->fft, agc2->cfg->nmicchannel, agc2->nbin);

	memset(agc2->fftx, 0, sizeof(wtk_complex_t)*(agc2->nbin));

	if(agc2->qmmse)
	{
		wtk_qmmse_reset(agc2->qmmse);
	}

	if(agc2->audgram){
		memset(agc2->audgram, 0, sizeof(float)*agc2->cfg->nfilts);
	}
	if(agc2->fbg){
		memset(agc2->fbg, 0, sizeof(float)*agc2->cfg->nfilts);
	}
	if(agc2->state){
		memset(agc2->state, 0, sizeof(float)*agc2->cfg->nfilts);
	}
	if(agc2->D){
		memset(agc2->D, 0, sizeof(float)*agc2->nbin);
	}
	if(agc2->E){
		memset(agc2->E, 0, sizeof(float)*agc2->nbin);
	}

	if(agc2->eq)
	{
		wtk_equalizer_reset(agc2->eq);
	}

	agc2->mic_silcnt=0;
	agc2->mic_sil=1;

	agc2->bs_scale=1.0;
	agc2->bs_last_scale=1.0;
	agc2->bs_max_cnt=0;
	agc2->min_E=1;
}


void wtk_agc2_set_notify(wtk_agc2_t *agc2,void *ths,wtk_agc2_notify_f notify)
{
	agc2->notify=notify;
	agc2->ths=ths;
}

static float wtk_agc2_fft_energy(wtk_complex_t *fftx,int nbin)
{
	float f;
	int i;

	f=0;
	for(i=1; i<nbin-1; ++i)
	{
		f+=fftx[i].a*fftx[i].a+fftx[i].b*fftx[i].b;
	}

	return f;
}

void wtk_agc2_control_bs(wtk_agc2_t *agc2, float *out, int len)
{
	float out_max;
	int i;

	if(agc2->mic_sil==0)
	{
		out_max=wtk_float_abs_max(out, len);
		if(out_max>32700.0)
		{
			agc2->bs_scale=32700.0f/out_max;
			if(agc2->bs_scale<agc2->bs_last_scale)
			{
				agc2->bs_last_scale=agc2->bs_scale;
			}else
			{
				agc2->bs_scale=agc2->bs_last_scale;
			}
			agc2->bs_max_cnt=5;
		}
		for(i=0; i<len; ++i)
		{
			out[i]*=agc2->bs_scale;
		}
		if(agc2->bs_max_cnt>0)
		{
			--agc2->bs_max_cnt;
		}
		if(agc2->bs_max_cnt<=0 && agc2->bs_scale<1.0)
		{
			agc2->bs_scale*=1.1f;
			agc2->bs_last_scale=agc2->bs_scale;
			if(agc2->bs_scale>1.0)
			{
				agc2->bs_scale=1.0;
				agc2->bs_last_scale=1.0;
			}
		}
	}else
	{
		agc2->bs_scale=1.0;
		agc2->bs_last_scale=1.0;
		agc2->bs_max_cnt=0;
	}
} 

void wtk_agc2_feed_tf_agc(wtk_agc2_t *agc2, wtk_complex_t *fftx)
{
	float alpha=agc2->cfg->alpha;
	int nfilts=agc2->cfg->nfilts;

	float **f2a=agc2->cfg->f2a;
	float **f2a_diag=agc2->cfg->f2a_diag;
	float *audgram=agc2->audgram;
	float *fbg=agc2->fbg;
	float *state=agc2->state;
	float *D=agc2->D;
	float *E=agc2->E;
	int nbin=agc2->nbin;
	float scale = agc2->cfg->wins*1.0/agc2->cfg->rate;
	int i, j;

	for(i=0;i<nbin;++i){
		D[i] = sqrtf(fftx[i].a*fftx[i].a+fftx[i].b*fftx[i].b)*scale;
	}

	for(i=0;i<nfilts;++i){
		audgram[i] = 0;
		for(j=0;j<nbin;++j){
			audgram[i] += f2a[i][j] * D[j];
		}
	}

	for(i=0;i<nfilts;++i){
		state[i] = max(alpha*state[i], audgram[i]);
		fbg[i] = state[i];
		// printf("%f %f %f\n", state[i], audgram[i], alpha);
	}
	// getchar();

	for(i=0;i<nbin;++i){
		E[i] = 0;
		for(j=0;j<nfilts;++j){
			E[i] += f2a_diag[j][i] * fbg[j];
		}
		if(E[i]!=0 && E[i] < agc2->min_E){
			agc2->min_E = max(E[i], agc2->cfg->max_gain_back);
		}
		if(E[i]<=0){
			E[i] = agc2->min_E;
		}
	}
	for(i=0;i<nbin;++i){
		// printf("%f\n", E[i]);
		fftx[i].a *= 1.0/E[i];
		fftx[i].b *= 1.0/E[i];
	}

}

void wtk_agc2_feed(wtk_agc2_t *agc2,short *data,int len,int is_end)
{
	int i,j;
	int nbin=agc2->nbin;
	int nmicchannel=agc2->cfg->nmicchannel;
	int *mic_channel=agc2->cfg->mic_channel;
	int channel=agc2->cfg->channel;
	wtk_strbuf_t **mic=agc2->mic;
	int wins=agc2->cfg->wins;
	int fsize=wins/2;
	int length;
	float micenr;
	float micenr_thresh=agc2->cfg->micenr_thresh;
	int micenr_cnt=agc2->cfg->micenr_cnt;
	wtk_drft_t *rfft=agc2->rfft;
	float *rfft_in=agc2->rfft_in;
	wtk_complex_t **fft=agc2->fft;
	float **analysis_mem=agc2->analysis_mem;
	float *synthesis_mem=agc2->synthesis_mem;
	float *analysis_window=agc2->analysis_window, *synthesis_window=agc2->synthesis_window;
	wtk_complex_t *fftx=agc2->fftx;
	int clip_s = agc2->cfg->clip_s;
	int clip_e = agc2->cfg->clip_e;
	float *out=agc2->out;
	short *pv=(short *)out;

	for(i=0;i<len;++i)
	{
		for(j=0; j<nmicchannel; ++j)
		{
			wtk_strbuf_push(mic[j],(char *)(data+mic_channel[j]),sizeof(short));
		}
		data+=channel;
	}
	length=mic[0]->pos/sizeof(short);
	while(length>=fsize)
	{
		for(i=0; i<nmicchannel; ++i)
		{
			wtk_drft_frame_analysis22(rfft, rfft_in, analysis_mem[i], fft[i], (short *)(mic[i]->data), wins, analysis_window);
		}

		memcpy(fftx, fft[0], sizeof(wtk_complex_t)*nbin);
		if(agc2->qmmse)
		{
			wtk_qmmse_denoise(agc2->qmmse, fftx);	
		}else if(agc2->cfg->use_tf_agc){
			wtk_agc2_feed_tf_agc(agc2, fftx);
		}

		for(i=0;i<clip_s;++i){
			fftx[i].a = fftx[i].b = 0;
		}
		for(i=clip_e;i<nbin;++i){
			fftx[i].a = fftx[i].b = 0;
		}
		// static int cnt=0;
		// cnt++;
		micenr=wtk_agc2_fft_energy(fftx, nbin);
		if(micenr>micenr_thresh)
		{
			// if(agc2->mic_sil==1)
			// {
			// 	printf("sp start %f %f %f\n", 1.0/16000*cnt*(nbin-1),micenr,micenr_thresh);
			// }
			agc2->mic_sil=0;
			agc2->mic_silcnt=micenr_cnt;
		}else if(agc2->mic_sil==0)
		{
			agc2->mic_silcnt-=1;
			if(agc2->mic_silcnt<=0)
			{
				// printf("sp end %f\n", 1.0/16000*cnt*(nbin-1));
				agc2->mic_sil=1;
			}
		}

		wtk_strbufs_pop(mic, nmicchannel, fsize*sizeof(short));
		length=mic[0]->pos/sizeof(short);

	    wtk_drft_frame_synthesis22(rfft, rfft_in, synthesis_mem, fftx, out, wins, synthesis_window);
		if(agc2->eq)
		{
			wtk_equalizer_feed_float(agc2->eq, out, fsize);
		}
		wtk_agc2_control_bs(agc2, out, fsize);
		for(i=0; i<fsize; ++i)
		{
			pv[i]=floorf(out[i]+0.5);
		}
		if(agc2->notify)
		{
			agc2->notify(agc2->ths,pv,fsize);
		}
	}
	if(is_end && length>0)
	{
		if(agc2->notify)
		{
			pv=(short *)mic[0]->data;
			agc2->notify(agc2->ths,pv,length);
		}
	}
}