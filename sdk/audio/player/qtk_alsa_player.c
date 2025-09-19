#ifdef WIN32
#elif __ANDROID__
#elif OPENAL
#elif __mips
#else

#include "qtk_alsa_player.h" 
#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_str.h"
#ifdef DEBUG_FILE
#include "wtk/core/wtk_os.h"
qtk_alsa_player_t* qtk_alsa_player_start(
		void *h,
		char *name,
		int sample_rate,
		int channel,
		int bytes_per_sample,
		int buf_time,
		int period_time,
		int start_time,
		int stop_time,
		int avail_min_time,
		int silence_time,
		int use_uac
		)
{
	qtk_alsa_player_t *player;
	player=(qtk_alsa_player_t*)malloc(sizeof(qtk_alsa_player_t));

	return player;
}

int qtk_alsa_player_stop(void *h, qtk_alsa_player_t *player)
{
	free(player);
	return 0;
}

long qtk_alsa_player_write(void *h, qtk_alsa_player_t *player,char *data,int bytes)
{
	double t;

	t=bytes*1000.0/(16000*2);
	//wtk_debug("t=%f bytes=%d\n",t,bytes);
	if(0)
	{
		// wtk_msleep(t);
		usleep(t *1000);
	}
	return bytes;
}

int qtk_alsa_player_get_count(qtk_alsa_player_t *player)
{
	return 0;
}
#else


qtk_alsa_player_t* qtk_alsa_player_new(
		char *name,
		int sample_rate,
		int channel,
		int bytes_per_sample,
		int buf_time
		)
{
	qtk_alsa_player_t *player;

	//channel=4;
	player=(qtk_alsa_player_t*)malloc(sizeof(qtk_alsa_player_t));
	player->channel=channel;
	player->bytes_per_sample=bytes_per_sample;
	player->rate=sample_rate;
	// player->zerobuf=(char *)malloc(sample_rate/1000*2*20);
	// memset(player->zerobuf, 0, sample_rate/1000*2*20);
	player->snd_name=wtk_str_dup(name);
	player->buf_time=buf_time;
	player->pcm=NULL;
	player->bc=player->bytes_per_sample*player->channel;
	player->on=0;
	player->err=0;
	return player;
}

void qtk_alsa_player_delete(qtk_alsa_player_t *player)
{
	if(player->snd_name)
	{
		free(player->snd_name);
	}
	// wtk_free(player->zerobuf);
	free(player);
}

static int qtk_alsa_player_set_sw_params(qtk_alsa_player_t *player)
{
	snd_pcm_sw_params_t *sw=NULL;
	snd_pcm_uframes_t frame;
	int ret=-1;
	int start_size=-1;
	int stop_size=-1;
	int avail_size=-1;
	int silence_size=-1;

	if(player->start_time >= 0)
	{
		start_size =player->rate*player->channel*player->bytes_per_sample*player->start_time/1000;
	}
	if(player->stop_time >= 0)
	{
		stop_size =player->rate*player->channel*player->bytes_per_sample*player->stop_time/1000;
	}
	if(player->avail_time >= 0)
	{
		avail_size=player->rate*player->channel*player->bytes_per_sample*player->avail_time/1000;
	}
	if(player->silence_time >= 0)
	{
		silence_size = player->rate*player->channel*player->bytes_per_sample*player->silence_time/1000;
	}

	snd_pcm_sw_params_malloc(&sw);
	snd_pcm_sw_params_current(player->pcm, sw);

	if(start_size >= 0)
	{
		frame=start_size/player->bc;
		ret=snd_pcm_sw_params_set_start_threshold(player->pcm,sw,frame);
		if(ret<0){wtk_debug("ret=%d\n",ret);}
	}
	if(stop_size >= 0)
	{
		frame=stop_size/player->bc;
		ret=snd_pcm_sw_params_set_stop_threshold(player->pcm, sw,frame);
		if(ret<0){wtk_debug("ret=%d\n",ret);}
	}
	if(silence_size >= 0)
	{
		frame=silence_size/player->bc;
		ret=snd_pcm_sw_params_set_silence_threshold(player->pcm,sw,frame);
		if(ret<0){wtk_debug("ret=%d\n",ret);}
	}
	if(avail_size >= 0)
	{
		frame=avail_size/player->bc;
		ret=snd_pcm_sw_params_set_avail_min(player->pcm, sw, frame);
		if(ret<0){wtk_debug("ret=%d\n",ret);}
	}
	//snd_pcm_sw_params_get_boundary(sw,&b);
	// ret=snd_pcm_sw_params_set_silence_size(player->pcm,sw,10);
	// wtk_debug("ret=%d\n",ret);
	if(sw)
	{
		ret=snd_pcm_sw_params(player->pcm,sw);
		wtk_debug("ret=%d\n",ret);
		if(sw){snd_pcm_sw_params_free(sw);}
	}
	return ret;
}

int qtk_alsa_player_stop2(qtk_alsa_player_t *player)
{
	player->on=0;
	if(player->pcm)
	{
		snd_pcm_drain(player->pcm);
		snd_pcm_drop(player->pcm);
		snd_pcm_reset(player->pcm);
		snd_pcm_close(player->pcm);
		player->pcm=NULL;
	}
	return 0;
}

int qtk_alsa_player_start3(qtk_alsa_player_t *player)
{
	int ret;
	int err = 0;
	snd_pcm_hw_params_t *hw_params=NULL;
	snd_pcm_format_t format;
	snd_pcm_uframes_t frame;
	int buf_size;
	int cnt;
	snd_pcm_sw_params_t *swparams=NULL;
	unsigned buffer_time = 0;
	unsigned period_time = 0;
	snd_pcm_uframes_t buffer_size;
	snd_pcm_uframes_t nSndOneFrameSize;
	size_t nSndOneFrameByte;
	int rate=player->rate;

	// memset(player->zerobuf, 0, rate/1000*2*20);

	buf_size=rate*player->channel*player->bytes_per_sample*player->buf_time/1000;
	player->pcm=NULL;
	cnt=0;
	do{
		ret=snd_pcm_open(&(player->pcm),player->snd_name,SND_PCM_STREAM_PLAYBACK,0);
		if(ret<0)
		{
			wtk_debug("=======================> open pcm failed\n");
			snd_pcm_reset(player->pcm);
			snd_pcm_drain(player->pcm);
			snd_pcm_close(player->pcm);
		}
		++cnt;
	}while(ret<0 && cnt<100);
	if(ret<0){err = -1; goto end;}
	
	snd_config_update_free_global();

	snd_pcm_hw_params_alloca(&hw_params);
	// snd_pcm_sw_params_alloca(&swparams);

	// ret=snd_pcm_hw_params_malloc(&hw_params);
	// if(ret<0){err = -2;goto end;}

	ret=snd_pcm_hw_params_any(player->pcm,hw_params);
	if(ret<0){err = -3; goto end;}

	ret=snd_pcm_hw_params_set_access(player->pcm,hw_params,SND_PCM_ACCESS_RW_INTERLEAVED);
	if(ret<0){err = -4; goto end;}
	
	if(player->bytes_per_sample==4)
	{
		format=SND_PCM_FORMAT_S32_LE;
	}else if(player->bytes_per_sample==3)
	{
		format=SND_PCM_FORMAT_S24_LE;
	}else if(player->bytes_per_sample==2)
	{
		format=SND_PCM_FORMAT_S16_LE;
	}else if(player->bytes_per_sample==1)
	{
		format=SND_PCM_FORMAT_S8;
	}else{
		format=SND_PCM_FORMAT_UNKNOWN;
	}
	ret=snd_pcm_hw_params_set_format(player->pcm, hw_params, format);
	if(ret<0){err = -5; goto end;}
	
	ret=snd_pcm_hw_params_set_channels(player->pcm, hw_params, player->channel);
	if(ret<0){err = -6; goto end;}
	
	
	ret=snd_pcm_hw_params_set_rate_near(player->pcm,hw_params,(unsigned int*)(&rate), 0);
	if(ret<0){err = -7; goto end;}

	// 获取底层支持的最大buffer大小，换算为时间
	// ret = snd_pcm_hw_params_get_buffer_time_max(hw_params, &buffer_time, 0);
	// wtk_debug("=================max======>>>>buffer time=%d\n",buffer_time);
	// ret = snd_pcm_hw_params_get_buffer_time_min(hw_params, &buffer_time, 0);
	// wtk_debug("=================min======>>>>buffer time=%d\n",buffer_time);
	// snd_pcm_hw_params_get_buffer_size_max(hw_params, &buffer_time);
	// wtk_debug("=================max======>>>>buffer size=%d\n",buffer_time);
	// snd_pcm_hw_params_get_buffer_size_min(hw_params, &buffer_time);
	// wtk_debug("=================min======>>>>buffer size=%d\n",buffer_time);
	// snd_pcm_hw_params_get_period_time_max(hw_params, &buffer_time, 0);
	// wtk_debug("=================max======>>>>period time=%d\n",buffer_time);
	// snd_pcm_hw_params_get_period_time_min(hw_params, &buffer_time, 0);
	// wtk_debug("=================min======>>>>period time=%d\n",buffer_time);
	// snd_pcm_hw_params_get_period_size_max(hw_params, &buffer_time, 0);
	// wtk_debug("=================max======>>>>period size=%d\n",buffer_time);
	// snd_pcm_hw_params_get_period_size_min(hw_params, &buffer_time, 0);
	// wtk_debug("=================min======>>>>period size=%d\n",buffer_time);
	// snd_pcm_hw_params_get_channels_max(hw_params, &buffer_time);
	// wtk_debug("=================max======>>>>channel=%d\n",buffer_time);
	// snd_pcm_hw_params_get_channels_min(hw_params, &buffer_time);
	// wtk_debug("=================min======>>>>channel=%d\n",buffer_time);
	// snd_pcm_hw_params_get_rate_max(hw_params, &buffer_time, 0);
	// wtk_debug("=================max======>>>>rate=%d\n",buffer_time);
	// snd_pcm_hw_params_get_rate_min(hw_params, &buffer_time, 0);
	// wtk_debug("=================min======>>>>rate=%d\n",buffer_time);
	// snd_pcm_hw_params_get_format(hw_params, &buffer_time);
	// wtk_debug("=======================>>>>format=%d\n",buffer_time);
	// buffer_time = buffer_time > 128000 ? 128000 : buffer_time; //buffer_time = buffer_time > 500000 ? 500000 : buffer_time;
	// buffer_time = 32000;
	// period_time = buffer_time / 4;
	// if(buffer_time > (player->buf_time*1000*4))
	// {
	// 	buffer_time = player->buf_time*1000*4;
	// }
	// period_time = buffer_time / 4;
	// period_time = 32000;
	period_time = player->period_time *1000;
	buffer_time = player->buf_time*1000/period_time * period_time;
#ifndef USE_SLB
#ifndef USE_3308
	wtk_debug("=======================>>>>buffer time=%d\n",buffer_time);

	wtk_debug("buffer_time:[%d] period_time:[%d]\n", buffer_time, period_time);
#endif
#endif
	ret = snd_pcm_hw_params_set_period_time_near(player->pcm, hw_params, &period_time, 0);
	if(ret<0){err = -8; goto end;}

	ret = snd_pcm_hw_params_set_buffer_time_near(player->pcm, hw_params, &buffer_time, 0);
	if(ret<0){err = -9; goto end;}

	if(player->use_uac)
	{
		ret=snd_pcm_nonblock(player->pcm, 1);
		if(ret<0){err = -11; goto end;}
	}else{
		ret=snd_pcm_nonblock(player->pcm, 0);
		if(ret<0){err = -11; goto end;}
	}

	ret=snd_pcm_hw_params(player->pcm, hw_params);
	if(ret<0){err = -10; goto end;}

	// ret=snd_pcm_prepare(player->pcm);
	// wtk_debug("================================>>>>>>>>>>>>>>>\n");
	// if(ret!=0){goto end;}

	// snd_pcm_hw_params_get_buffer_size(hw_params, &buffer_size);
	// wtk_debug("================================>>>>>>>>>>>>>>>\n");
	// snd_pcm_hw_params_get_period_size(hw_params, &nSndOneFrameSize, 0);
	// wtk_debug("================================>>>>>>>>>>>>>>>\n");
#ifndef USE_SLB
#ifndef USE_3308
	wtk_debug("buffer size=%d period_size=%d\n",buf_size,nSndOneFrameSize);
#endif
#endif

	// snd_pcm_sw_params_current(player->pcm, swparams);
	// snd_pcm_sw_params_set_avail_min(player->pcm, swparams, nSndOneFrameSize);
	// snd_pcm_uframes_t start_threshold, stop_threshold;
	// start_threshold = nSndOneFrameSize + (double) (player->rate) * 0 / 1000000; // 录音是 * 1
	// if (start_threshold < 1)
	// 	start_threshold = 1;
	// if (start_threshold > nSndOneFrameSize)
	// 	start_threshold = nSndOneFrameSize;
	// stop_threshold = buffer_size + (double) (player->rate) * 0 / 1000000;
	// stop_threshold = stop_threshold * 1000;
	// wtk_debug("start_threshold:%d  stop_threshold:%d\n", start_threshold, stop_threshold);
	// snd_pcm_sw_params_set_start_threshold(player->pcm, swparams, start_threshold);
	// snd_pcm_sw_params_set_stop_threshold(player->pcm, swparams, stop_threshold);
	// if(snd_pcm_sw_params(player->pcm, swparams) < 0){
	// 	wtk_debug("snd_pcm_sw_params failed.\n");
	// 	goto end;
	// }

	player->on=1;
	ret=0;
end:
	// if(hw_params){
	// 	wtk_debug("================================>>>>>>>>>>>>>>>\n");
	// 	snd_pcm_hw_params_free(hw_params);
	// 	wtk_debug("================================>>>>>>>>>>>>>>>\n");
	// }
	// if(swparams){snd_pcm_sw_params_free(swparams);}
	if(ret!=0)
	{
		wtk_debug("========================>player start err %d\n",err);
		exit(0);
	}
	return ret;
}

qtk_alsa_player_t* qtk_alsa_player_start(
		void *h,
		char *name,
		int sample_rate,
		int channel,
		int bytes_per_sample,
		int buf_time,
		int period_time,
		int start_time,
		int stop_time,
		int avail_min_time,
		int silence_time,
		int use_uac
		)
{
	qtk_alsa_player_t* player;

	player=qtk_alsa_player_new(name,sample_rate,channel,bytes_per_sample,buf_time);
	player->use_uac = use_uac;
	player->period_time = period_time;
	player->start_time = start_time;
	player->stop_time = stop_time;
	player->avail_time = avail_min_time;
	player->silence_time = silence_time;
	printf("player==>%p card=%s sample_rate = %d channel = %d bytes_per_sample = %d buftime = %d peroidtime=%d use_uac=%d\n",player,player->snd_name,
			player->rate,
			player->channel,
			player->bytes_per_sample,
			player->buf_time,
			player->period_time,
			player->use_uac);
	qtk_alsa_player_start3(player);
	return player;
}

qtk_alsa_player_t* qtk_alsa_player_start2(
		void *h,
		char *name,
		int sample_rate,
		int channel,
		int bytes_per_sample,
		int buf_time,
		int period_time,
		int use_uac
		)
{
	qtk_alsa_player_t* player;

	player=qtk_alsa_player_new(name,sample_rate,channel,bytes_per_sample,buf_time);
	player->use_uac = use_uac;
	qtk_alsa_player_start3(player);

	return player;
}

int qtk_alsa_player_stop(void *h, qtk_alsa_player_t *player)
{
	qtk_alsa_player_stop2(player);
	qtk_alsa_player_delete(player);
	return 0;
}

int qtk_alsa_player_write_pcm(qtk_alsa_player_t *player,char *data,int step)
{
	int vi=0;
	int ret;

	while(1)
	{
		//ret=snd_pcm_avail(player->pcm);
		//wtk_debug("ret=%d\n",ret);
		ret=snd_pcm_writei(player->pcm,data,step);
		if(ret==-11)
		{
			snd_pcm_wait(player->pcm,3);
			continue;
		}
		if(ret>0 && ret<step)
		{
			continue;
		}
		vi+=ret;
		break;
	}
	exit(0);
	return vi;
}


#include "wtk/core/wtk_os.h"

long qtk_alsa_player_write(void *h, qtk_alsa_player_t *player,char *data,int bytes)
{
	long ret;
	int step;
	static double tm=0.0;
	static int sendlen=0;
	double sm;

	if(!player->on)
	{
		qtk_alsa_player_start3(player);
	}
	step=bytes/player->bc;///(player->bytes_per_sample*player->channel);
	if(player->use_uac)
	{
		ret=snd_pcm_writei(player->pcm,data,step);
	}else{
		while(1)
		{
			ret=snd_pcm_writei(player->pcm,data,step);
			if(ret!=-11)
			{
				break;
			}
			wtk_debug("ret=%d\n",ret);
			//++cnt;
			snd_pcm_wait(player->pcm,3);
			//wtk_debug("ret=%d/%d/%d cnt=%d\n",ret,bytes,x,cnt);
			//wtk_msleep(1);
		}
	}
	
	if (ret == -32)
	{
		static int cnt=0;
		static int kx=0;

		player->err=1;
		++cnt;
		if(1)
		{
			//wtk_debug("restart player cnt=%d =================\n",cnt);
			qtk_alsa_player_stop2(player);
			wtk_debug("player underrun ret=%d cnt=%d bytes=%d kx=%d\n",ret,cnt,bytes,kx);
			//wtk_msleep(10);
		}else
		{
			ret = snd_pcm_prepare(player->pcm);
			wtk_debug("player underrun ret=%d cnt=%d bytes=%d kx=%d\n",ret,cnt,bytes,kx);
			if (ret < 0){
				snd_pcm_recover(player->pcm, -32, 0);
				printf("Can't recovery from underrun, prepare failed: %s\n", snd_strerror(ret));
			}
		}
		++kx;
		// snd_pcm_writei(player->pcm,data,step);
		if(kx>1)
		{
			ret=-32;
		}else
		{
			ret=qtk_alsa_player_write(h, player, data, bytes);
		}
		//ret=-32;
		kx=0;
		//ret=-32;
	} else if (ret == -86) {
		wtk_debug("player error ret=%d\n",ret);
		while((ret = snd_pcm_resume(player->pcm)) == -11){sleep(1);}
		if (ret < 0) {
			ret = snd_pcm_prepare(player->pcm);
			if (ret < 0){
				snd_pcm_recover(player->pcm, -86, 0);
#ifndef USE_3308
				printf("Can't recovery from suspend, prepare failed: %s\n", snd_strerror(ret));
#endif
			}
		}
	}else if(ret < 0){
		// wtk_debug("player error ret=%d\n",ret);
		ret =snd_pcm_recover(player->pcm, ret, 0);
		if(ret <0)
			goto end;
	}

	//snd_pcm_hwsync(player->pcm);
end:
	if(ret > 0)
	{
		ret=ret*player->bc;//bytes_per_sample*player->channel;
	}
	return ret;
}

int qtk_alsa_player_get_count(qtk_alsa_player_t *player)
{
	return snd_pcm_poll_descriptors_count(player->pcm);
}
#endif
#endif
