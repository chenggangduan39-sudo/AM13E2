#if (defined __ANDROID__) || (defined USE_XDW)

#include "qtk_tinyalsa_player.h" 

static enum pcm_format qtk_tinyalsa_player_get_fmt(int bytes_per_sample)
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

qtk_tinyalsa_player_t* qtk_tinyalsa_player_start(void *h,
		char *pdev,
		int sample_rate,
		int channel,
		int bytes_per_sample,
		int buf_time,
		int period_time
		)
{
	qtk_tinyalsa_player_t* r=0;
	enum pcm_format fmt;
	int card;
	int device;
	int ret=-1;

	card = ((int*)(pdev))[0];
	device = ((int*)(pdev))[1];
	r = calloc(1, sizeof(*r));
	if (!r) {goto end;}
	/*
	 * set params
	 */
	r->config.channels = channel;
	r->config.rate = sample_rate;
	r->config.period_size = period_time*sample_rate/1000;
	r->config.period_count = buf_time/period_time;
#ifdef USE_XDW
	r->config.period_size = 1024;//buf_time*sample_rate/1000;
	if(card == 3)
	{
		r->config.period_count = 4;
	}else{
		r->config.period_count = 2;
	}
#endif
	fmt = qtk_tinyalsa_player_get_fmt(bytes_per_sample);
	r->config.format = fmt;
	// r->config.start_threshold = 1024;
	// r->config.stop_threshold = 1024*2;
	// r->config.silence_threshold = 1024*2;
	//r->config.avail_min=0;

	//wtk_debug("%s\n",dev);
	r->pcm=0;
	wtk_debug("card:%d device:%d sample_rate:%d channel:%d buf_time:%d peroid_time:%d bytes_per_sample:%d\n",
																card,
																device,
																sample_rate,
																channel,buf_time,
																period_time,
																bytes_per_sample);

	r->pcm = pcm_open(card, device, PCM_OUT, &(r->config));

	if(r->pcm == NULL)
	{
		wtk_debug("failed to allocate memory for pcm\n");
	}else if (!pcm_is_ready(r->pcm))
	{
		wtk_debug("pcm is not ready.[%s]\n",pcm_get_error(r->pcm));
		goto end;
	}

	r->size = pcm_frames_to_bytes(r->pcm,pcm_get_buffer_size(r->pcm));
	wtk_debug("=========================>>>>>>>>>>%d\n",r->size);
	// r->size = 9600;
	r->writebuf = wtk_strbuf_new(r->size, 1.0);

	// ret=pcm_start(r->pcm);
	// if(ret!=0){
	// 	wtk_debug("%s.\n",pcm_get_error(r->pcm));
	// 	goto end;
	// }
	ret=0;
end:
	if(ret<0){
		if(r){
			qtk_tinyalsa_player_stop(NULL,r);
			r=0;
		}
	}
	return r;
}

int qtk_tinyalsa_player_write(void *h,qtk_tinyalsa_player_t* r,char *buf,int bytes)
{
	int ret=0;

	if(bytes > 0)
	{
		wtk_strbuf_push(r->writebuf, buf, bytes);
		while(r->writebuf->pos >= r->size)
		{
			ret = pcm_write(r->pcm, r->writebuf->data, r->size);
			if(ret < 0) {
				wtk_debug("==>%d==>[%s]\n",ret,pcm_get_error(r->pcm));
			}
			wtk_strbuf_pop(r->writebuf, NULL, r->size);
		}
	}
	
	if(ret < 0) {
		return -1;
	} else {
		return bytes;
	}
}

int qtk_tinyalsa_player_stop(void*h,qtk_tinyalsa_player_t* r)
{
	if(r){
		pcm_stop(r->pcm);
		pcm_close(r->pcm);
		wtk_strbuf_delete(r->writebuf);
		free(r);
	}
	return 0;
}

#endif
