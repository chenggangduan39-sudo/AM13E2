#include "wtk_mix_speech.h"

wtk_mix_speech_t* wtk_mix_speech_new(wtk_mix_speech_cfg_t *cfg)
{
	wtk_mix_speech_t *mix_speech;

	mix_speech=(wtk_mix_speech_t *)wtk_malloc(sizeof(wtk_mix_speech_t));
	mix_speech->cfg=cfg;
	mix_speech->ths=NULL;
	mix_speech->notify=NULL;

	mix_speech->mic=wtk_strbufs_new(mix_speech->cfg->channel);

	mix_speech->out=wtk_malloc(sizeof(short)*(cfg->wins/2));

	mix_speech->mic_scale=(float *)wtk_malloc(sizeof(float)*cfg->channel);

	wtk_mix_speech_reset(mix_speech);

	return mix_speech;
}

void wtk_mix_speech_delete(wtk_mix_speech_t *mix_speech)
{
	wtk_strbufs_delete(mix_speech->mic,mix_speech->cfg->channel);

	wtk_free(mix_speech->out);
	wtk_free(mix_speech->mic_scale);
	wtk_free(mix_speech);
}


void wtk_mix_speech_start(wtk_mix_speech_t *mix_speech)
{
}

void wtk_mix_speech_reset(wtk_mix_speech_t *mix_speech)
{
	int i;
	int channel = mix_speech->cfg->channel;
	wtk_strbufs_reset(mix_speech->mic,mix_speech->cfg->channel);
	
	for(i=0;i<channel;++i){
		mix_speech->mic_scale[i] = 1.0;
	}
	mix_speech->mic_silcnt=0;
	mix_speech->mic_sil=1;
}


void wtk_mix_speech_set_notify(wtk_mix_speech_t *mix_speech,void *ths,wtk_mix_speech_notify_f notify)
{
	mix_speech->notify=notify;
	mix_speech->ths=ths;
}

static float wtk_mix_speech_mic_energy(short *p,int n)
{
	float f,f2;
	int i;

	f=0;
	for(i=0;i<n;++i)
	{
		f+=p[i];
	}
    f/=n;

    f2=0;
	for(i=0;i<n;++i)
	{
		f2+=(p[i]-f)*(p[i]-f);
	}
	f2/=n;

	return f2;
}

void wtk_mix_speech_norm_mix(wtk_mix_speech_t *mix_speech, short **data, int len)
{
	int channel=mix_speech->cfg->channel;
	int i, j;
	int max_1 = mix_speech->cfg->max_1;
	int min_1 = mix_speech->cfg->min_1;
	int max_2 = mix_speech->cfg->max_2;
	int min_2 = mix_speech->cfg->min_2;
	int f_win = mix_speech->cfg->f_win;
	float *mic_cfg_scale = mix_speech->cfg->mic_scale;
	float *mic_scale = mix_speech->mic_scale;
	static float f = 1.0;
	float min_val;
	int output;
	float micenr;
	float micenr_thresh=mix_speech->cfg->micenr_thresh;
	float *scale_step=mix_speech->cfg->scale_step;
	int micenr_cnt=mix_speech->cfg->micenr_cnt;

	micenr=wtk_mix_speech_mic_energy(data[0], len);
	// static int cnt=0;
	// cnt++;
	if(micenr>micenr_thresh)
	{
		// if(mix_speech->mic_sil==1)
		// {
		// 	printf("sp start %f %f %f\n", 1.0/16000*cnt*(nbin-1),micenr,micenr_thresh);
		// }
		mix_speech->mic_sil=0;
		mix_speech->mic_silcnt=micenr_cnt;
	}else if(mix_speech->mic_sil==0)
	{
		mix_speech->mic_silcnt-=1;
		if(mix_speech->mic_silcnt<=0)
		{
			// printf("sp end %f\n", 1.0/16000*cnt*(nbin-1));
			mix_speech->mic_sil=1;
		}
	}

	for(i=0;i<channel;++i){
		if(mix_speech->mic_sil==1){
			if(mic_scale[i] - 1.0 > scale_step[i]){
				mic_scale[i] -= scale_step[i];
			}else if(mic_scale[i] - 1.0 < - scale_step[i]){
				mic_scale[i] += scale_step[i];
			}
		}else{
			if(mic_scale[i] - mic_cfg_scale[i] > scale_step[i]){
				mic_scale[i] -= scale_step[i];
			}else if(mic_scale[i] - mic_cfg_scale[i] < - scale_step[i]){
				mic_scale[i] += scale_step[i];
			}
		}
	}
	// printf("%f\n", mic_scale[1]);

	for(i=0;i<len;++i){
		min_val = 0;
		for(j=0;j<channel;++j){
			min_val += data[j][i] * mic_scale[j];
		}
		output = (int)(min_val * f);
		if(output > max_2){
			f = (1.0*max_2)*(1.0/output);
			output = min(max_1, output);
		}
		if(output < min_2){
			f = (1.0*min_2)*(1.0/output);
			output = max(min_1, output);
		}
		if (f < 1.0){
			f+=(1.0-f)*1.0/f_win;
		}
		// printf("%f\n", f);
		data[channel][i] = (short)output;
	}
}


void wtk_mix_speech_feed(wtk_mix_speech_t *mix_speech,short *data,int len,int is_end)
{
	int i,j;
	int channel=mix_speech->cfg->channel;
	int nmicchannel=mix_speech->cfg->nmicchannel;
	int *mic_channel=mix_speech->cfg->mic_channel;
	wtk_strbuf_t **mic=mix_speech->mic;
	int wins=mix_speech->cfg->wins;
	int fsize=wins/2;
	int length;
	short *out=mix_speech->out;
	int mix_type=mix_speech->cfg->mix_type;
	short *fp[32];

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
		for(i=0;i<channel;++i){
			fp[i] = (short *)mic[i]->data;
		}
		fp[channel] = out;
		switch (mix_type)
		{
		case WTK_MIX_SPEECH_NORM_MIX:
			wtk_mix_speech_norm_mix(mix_speech, fp, fsize);
			break;

		// case /* constant-expression */:
		// 	/* code */
		// 	break;
		default:
			break;
		}
		if(mix_speech->notify)
		{
			mix_speech->notify(mix_speech->ths,out,fsize);
		}
		wtk_strbufs_pop(mic, channel, fsize*sizeof(short));
		length=mic[0]->pos/sizeof(short);
	}
}