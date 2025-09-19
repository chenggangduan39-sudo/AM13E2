#if (defined __ANDROID__) || (defined USE_XDW)

#include "qtk_tinyalsa_recorder.h"

static enum pcm_format qtk_tinyalsa_recorder_get_fmt(int bytes_per_sample)
{
	enum pcm_format fmt;

	switch(bytes_per_sample)
	{
	case 1:
		fmt = PCM_FORMAT_S8;
		break;
	case 2:
		fmt = PCM_FORMAT_S16_LE;
		break;
	case 3:
		fmt = PCM_FORMAT_S24_LE;
		break;
	case 4:
		fmt = PCM_FORMAT_S32_LE;
		break;
	default:
		// fmt = PCM_FORMAT_INVALID;
		fmt = -1;
		break;
	}
	return fmt;
}

qtk_tinyalsa_recorder_t* qtk_tinyalsa_recorder_start(void *h,
		char *pdev,
		int sample_rate,
		int channel,
		int bytes_per_sample,
		int buf_time
		)
{
	qtk_tinyalsa_recorder_t* r=0;
	enum pcm_format fmt;
	int card;
	int device;
	int ret=-1;

	r = calloc(1, sizeof(*r));
	if (!r) {goto end;}
	/*
	 * set params
	 */
	r->config.channels = channel;
	r->config.rate = sample_rate;

#ifdef USE_XDW
	r->config.period_size = 1024;//buf_time*sample_rate/1000;;
#else
#ifdef USE_DESAIXIWEI
	r->config.period_size = 1024;
#else
	r->config.period_size = buf_time*sample_rate/1000;
#endif
#endif
	r->config.period_count = 4;
	fmt = qtk_tinyalsa_recorder_get_fmt(bytes_per_sample);
	r->config.format = fmt;
#ifdef USE_DESAIXIWEI
	r->config.start_threshold = 1;
	r->config.stop_threshold = 40960;
#else
	r->config.start_threshold = 0;
	r->config.stop_threshold = 0;
#endif
	r->config.silence_threshold = 0;

	//wtk_debug("%s\n",dev);
	card = ((int*)(pdev))[0];
	device = ((int*)(pdev))[1];
	r->pcm=0;
	wtk_debug(" %d %d %d %d %d\n",card,device,sample_rate,channel,buf_time);
	r->pcm = pcm_open(card, device, PCM_IN, &r->config);
	if(!r->pcm || !pcm_is_ready(r->pcm)){
		wtk_debug("pcm is not ready.[%s]\n",pcm_get_error(r->pcm));
		goto end;
	}
#ifdef USE_XDW
	r->size=pcm_frames_to_bytes(r->pcm,pcm_get_buffer_size(r->pcm));
	r->readbuf = wtk_strbuf_new(r->size, 1.0);
	r->data = (char *)wtk_malloc(r->size);
	wtk_debug("======================>>>>>>>>>>>>size=%d\n",r->size);
#else
	pcm_frames_to_bytes(r->pcm,pcm_get_buffer_size(r->pcm));
#endif
	ret=pcm_start(r->pcm);
	if(ret!=0){
		wtk_debug("%s.\n",pcm_get_error(r->pcm));
		goto end;
	}
	ret=0;
end:
	if(ret<0){
		if(r){
			qtk_tinyalsa_recorder_stop(NULL,r);
			r=0;
		}
	}
	return r;
}

int qtk_tinyalsa_recorder_read(void *h,qtk_tinyalsa_recorder_t* r,char *buf,int bytes)
{
	int ret;

#ifdef USE_XDW
	while(1)
	{
		if(r->readbuf->pos >= bytes)
		{
			memcpy(buf, r->readbuf->data, bytes);
			wtk_strbuf_pop(r->readbuf, NULL, bytes);
			return bytes;
		}else{
			ret = pcm_read(r->pcm,r->data, r->size);
			if(ret < 0) {
				wtk_debug("%s\n",pcm_get_error(r->pcm));
				return -1;
			}else{
				wtk_strbuf_push(r->readbuf, r->data, r->size);
			}
		}
	}
#else
	//wtk_debug("qtk_tinyalsa_recorder_read===============>>>>>>>>>>>>>>%d\n",bytes);
	ret = pcm_read(r->pcm,buf, bytes);
	if(ret < 0) {
		wtk_debug("%s\n",pcm_get_error(r->pcm));
		return -1;
	} else {
		return bytes;
	}
#endif
}

int qtk_tinyalsa_recorder_stop(void*h,qtk_tinyalsa_recorder_t* r)
{
	if(r){
		pcm_stop(r->pcm);
		pcm_close(r->pcm);
#ifdef USE_XDW
		wtk_strbuf_delete(r->readbuf);
		free(r->data);
#endif
		free(r);
	}
	return 0;
}

#endif
