#include "qtk_alsa_recorder.h" 
#include "wtk/core/wtk_type.h"


#ifdef DEBUG_FILE
#include "wtk/core/wtk_os.h"

qtk_alsa_recorder_t* qtk_alsa_recorder_start(
		char *name,
		int sample_rate,
		int channel,
		int bytes_per_sample,
		int buf_time
		)
{
	qtk_alsa_recorder_t *r;
	char *fn;

	fn="test.wav";
	r=(qtk_alsa_recorder_t*)malloc(sizeof(qtk_alsa_recorder_t));
	r->riff=wtk_riff_new();
	wtk_riff_open(r->riff,fn);
	return r;
}

qtk_alsa_recorder_t* qtk_alsa_recorder_start2(
		char *name,
		int sample_rate,
		int channel,
		int bytes_per_sample,
		int buf_time
		)
{
	qtk_alsa_recorder_t *r;
	char *fn;

	fn="test.wav";
	r=(qtk_alsa_recorder_t*)malloc(sizeof(qtk_alsa_recorder_t));
	r->riff=wtk_riff_new();
	wtk_riff_open(r->riff,fn);
	return r;
}

int alsa_first=0;
long tmplen=0;
int qtk_alsa_recorder_read(qtk_alsa_recorder_t *r,char *data,int bytes)
{
	int ret;
	double t1,t2,t3;
	
	//wtk_debug("check yes");
	t1=time_get_ms();
	if(alsa_first)
	{
		tmplen+=bytes;
		ret=wtk_riff_read(r->riff,data,bytes);
		if(ret==0)
		{
			wtk_riff_rewind(r->riff);
			if(tmplen >9600000)
			{
				wtk_debug("================>>>>>>%d\n",alsa_first);
				alsa_first=0;
				tmplen=0;
			}
		}
	}else{
		memset(data, 0, bytes);
		ret=bytes;
		tmplen+=bytes;
		if(tmplen > 9600000)
		{
			wtk_debug("================>>>>>>%d\n",alsa_first);
			wtk_riff_rewind(r->riff);
			alsa_first=1;
			tmplen=0;
		}
	}
	if(ret>0)
	{
		t2=time_get_ms();
		t3=ret*1000.0/(10*2*16000);//r->riff->fmt.channels*r->riff->fmt.bit_per_sample/8);
		//wtk_debug("t3=%f\n",t3);
		t3=t3-(t2-t1);
		// wtk_debug("===============================================t1=%f t2=%f t3=%f\n",t1,t2,t3);
		//exit(0);
		if(t3>0)
		{
			wtk_msleep(t3);
		}
	}
	return ret;
}

int qtk_alsa_recorder_stop(qtk_alsa_recorder_t *r)
{
	wtk_riff_close(r->riff);
	wtk_riff_delete(r->riff);
	wtk_free(r);
	return 0;
}

#else
static snd_pcm_format_t qtk_alsa_get_fmt(int bytes_per_sample)
{
	snd_pcm_format_t fmt;

	switch(bytes_per_sample) {
	case 1:
		fmt = SND_PCM_FORMAT_S8;
		break;
	case 2:
		fmt = SND_PCM_FORMAT_S16_LE;
		break;
	case 3:
		fmt = SND_PCM_FORMAT_S24_LE;
		break;
	case 4:
		fmt = SND_PCM_FORMAT_S32_LE;
		break;
	default:
		fmt = SND_PCM_FORMAT_UNKNOWN;
		break;
	}

	return fmt;
}

qtk_alsa_recorder_t* qtk_alsa_recorder_start(
		char *name,
		int sample_rate,
		int channel,
		int bytes_per_sample,
		int buf_time
		)
{
	snd_pcm_hw_params_t *hw_params=0;
	snd_pcm_format_t fmt;
	qtk_alsa_recorder_t *r;
	int size;
	int ret = -1;
	snd_pcm_uframes_t sp;

	// printf("name = %s\n",name);
	// printf("rate = %d\n",sample_rate);
	// printf("channel = %d\n",channel);
	// printf("bytes_per_sample = %d\n",bytes_per_sample);
	// printf("buf_time = %d\n",buf_time);
	r=(qtk_alsa_recorder_t*)malloc(sizeof(qtk_alsa_recorder_t));
	r->rate             = sample_rate;
	r->channel          = channel;
	r->bytes_per_sample = bytes_per_sample;
	r->pcm=0;

	size=r->rate*buf_time*bytes_per_sample*channel/1000;
	ret=snd_pcm_open(&(r->pcm),name,SND_PCM_STREAM_CAPTURE,0);
	if(ret<0){;
		printf("open failed: %s\n", strerror(errno));
		goto end;
	}
	ret=snd_pcm_hw_params_malloc(&hw_params);
	if(ret<0){goto end;}
	ret=snd_pcm_hw_params_any(r->pcm,hw_params);
	if(ret<0){goto end;}
	ret=snd_pcm_hw_params_set_access(r->pcm,hw_params,SND_PCM_ACCESS_RW_INTERLEAVED);
	if(ret<0){goto end;}

	fmt = qtk_alsa_get_fmt(bytes_per_sample);
	ret=snd_pcm_hw_params_set_format(r->pcm, hw_params, fmt);
	if(ret<0){goto end;}
	ret=snd_pcm_hw_params_set_rate_near(r->pcm,hw_params,&(r->rate), 0);
	if(ret<0){goto end;}
	ret=snd_pcm_hw_params_set_channels(r->pcm, hw_params, channel);
	if(ret<0){goto end;}
	sp=size/(bytes_per_sample*channel);
	ret=snd_pcm_hw_params_set_buffer_size(r->pcm,hw_params,sp);
	//ret=snd_pcm_hw_params_set_period_time(handler, hw_params,&time,0);
	if(ret<0){goto end;}
	sp=sp/4;
	ret=snd_pcm_hw_params_set_period_size_near(r->pcm,hw_params,&sp,0);
	if(ret<0){goto end;}
	ret=snd_pcm_hw_params(r->pcm,hw_params);
	if(ret<0){goto end;}
	ret=snd_pcm_prepare(r->pcm);
	if(ret!=0)
	{
		snd_strerror(ret);
		goto end;
	}

	ret=0;
end:
	/*
	snd_pcm_hw_params_get_buffer_size(hw_params,&sp);
	printf("%d\n",sp);
	snd_pcm_hw_params_get_period_size(hw_params,&sp,0);
	printf("%d\n",sp);
	*/
	if(hw_params){snd_pcm_hw_params_free(hw_params);}
	if(ret!=0)
	{
		qtk_alsa_recorder_stop(r);
		r=NULL;
	}
	return r;
}

qtk_alsa_recorder_t* qtk_alsa_recorder_start2(
		char *name,
		int sample_rate,
		int channel,
		int bytes_per_sample,
		int buf_time
		)
{
	snd_pcm_hw_params_t *hw_params = NULL;
	snd_pcm_sw_params_t *swparams = NULL;
	snd_pcm_format_t fmt;
	qtk_alsa_recorder_t *r;
	int size;
	int ret = -1;
	snd_pcm_uframes_t sp;
	unsigned buffer_time = 0;
	unsigned period_time = 0;
	snd_pcm_uframes_t period_frames = 0;

	// printf("name = %s\n",name);
	// printf("rate = %d\n",sample_rate);
	// printf("channel = %d\n",channel);
	// printf("bytes_per_sample = %d\n",bytes_per_sample);
	// printf("buf_time = %d\n",buf_time);
	r=(qtk_alsa_recorder_t*)malloc(sizeof(qtk_alsa_recorder_t));
	r->rate             = sample_rate;
	r->channel          = channel;
	r->bytes_per_sample = bytes_per_sample;
	r->pcm=0;

	size=r->rate*buf_time*bytes_per_sample*channel/1000;
	ret=snd_pcm_open(&(r->pcm),name,SND_PCM_STREAM_CAPTURE,0);
	if(ret<0){;
		printf("open failed: %s\n", strerror(errno));
		goto end;
	}
	// snd_pcm_hw_params_alloca(&hw_params);
	// snd_pcm_sw_params_alloca(&swparams);
	ret=snd_pcm_hw_params_malloc(&hw_params);
	if(ret<0){goto end;}
	ret=snd_pcm_hw_params_any(r->pcm,hw_params);
	if(ret<0){goto end;}
	ret=snd_pcm_hw_params_set_access(r->pcm,hw_params,SND_PCM_ACCESS_RW_INTERLEAVED);
	if(ret<0){goto end;}

	fmt = qtk_alsa_get_fmt(bytes_per_sample);
	ret=snd_pcm_hw_params_set_format(r->pcm, hw_params, fmt);
	if(ret<0){goto end;}
	ret=snd_pcm_hw_params_set_rate_near(r->pcm,hw_params,&(r->rate), 0);
	if(ret<0){goto end;}
	ret=snd_pcm_hw_params_set_channels(r->pcm, hw_params, channel);
	if(ret<0){goto end;}

	snd_pcm_hw_params_get_buffer_time_max(hw_params, &buffer_time, 0);
	// buffer_time = buffer_time > 128000 ? 128000 : buffer_time; //buffer_time = buffer_time > 500000 ? 500000 : buffer_time;
	if(buffer_time > (buf_time*1000*4))
	{
		buffer_time = buf_time*1000*4;
	}
	period_time = buffer_time / 4;
	// period_time = 32000;
	// buffer_time = buffer_time/period_time * period_time;
	// wtk_debug("=======================>>>>buffer time=%d period_time=%d\n",buffer_time,period_time);

	ret=snd_pcm_hw_params_set_period_time_near(r->pcm, hw_params, &period_time, 0);
	// ret=snd_pcm_hw_params_set_period_time(r->pcm, hw_params, &period_time, 0);
	if(ret<0){goto end;}
	ret=snd_pcm_hw_params_set_buffer_time_near(r->pcm, hw_params, &buffer_time, 0);
	// ret=snd_pcm_hw_params_set_buffer_time(r->pcm, hw_params, &buffer_time, 0);
	if(ret<0){goto end;}

	ret=snd_pcm_hw_params(r->pcm,hw_params);
	if(ret<0){goto end;}

	// size_t	nSndOneFrameByte	= 0;			// 一帧数据的字节长度
	// snd_pcm_uframes_t nSndOneFrameSize	= 0;

	// snd_pcm_hw_params_get_period_size(hw_params, &nSndOneFrameSize, 0);
	// snd_pcm_sw_params_current(r->pcm, swparams);
	// snd_pcm_sw_params_set_avail_min(r->pcm, swparams, nSndOneFrameSize);
	// if(snd_pcm_sw_params(r->pcm, swparams) < 0){
	// 	printf("snd_pcm_sw_params failed.");
	// 	goto end;
	// }

	// nSndOneFrameByte	= nSndOneFrameSize * 16 * 10 / 8;
	// printf("set sound success: frame[%d] byte:[%d] ...\n", nSndOneFrameSize, nSndOneFrameByte);

	ret=snd_pcm_prepare(r->pcm);
	if(ret!=0)
	{
		snd_strerror(ret);
		goto end;
	}
	ret=0;
end:
	snd_pcm_hw_params_get_buffer_size(hw_params,&sp);
	printf("===>buffer_size=%d\n",sp);
	snd_pcm_hw_params_get_period_size(hw_params,&sp,0);
	printf("===>period_size=%d\n",sp);

	if(hw_params){snd_pcm_hw_params_free(hw_params);}
	if(ret!=0)
	{
		qtk_alsa_recorder_stop(r);
		r=NULL;
	}
	return r;
}

int qtk_alsa_recorder_read(qtk_alsa_recorder_t *r,char *data,int bytes)
{
	int ret;
	int step;

	step=bytes/(r->bytes_per_sample*r->channel);
	ret=snd_pcm_readi(r->pcm,data,step);
	if(ret == -32) {
		printf("***********over run\n");
		ret = snd_pcm_prepare(r->pcm);
		if (ret < 0){
			snd_pcm_recover(r->pcm, -32, 0);
			printf("Can't recovery from underrun, prepare failed: %s\n", snd_strerror(ret));
		}
		ret = -32;
	}else if (ret == -86) {
		while((ret = snd_pcm_resume(r->pcm)) == -11){sleep(1);}
		if (ret < 0) {
			ret = snd_pcm_prepare(r->pcm);
			if (ret < 0){
				snd_pcm_recover(r->pcm, -86, 0);
				printf("Can't recovery from suspend, prepare failed: %s\n", snd_strerror(ret));
			}
		}
	}else if(ret < 0){
		ret =snd_pcm_recover(r->pcm, ret, 0);
		if(ret <0)
			goto end;
	}
	snd_pcm_hwsync(r->pcm);

end:
	if(ret>0)
	{
		ret=ret*r->bytes_per_sample*r->channel;
	}
	return ret;
}

int qtk_alsa_recorder_stop(qtk_alsa_recorder_t *r)
{
	if(r){
		if(r->pcm){
			snd_pcm_reset(r->pcm);
			snd_pcm_drain(r->pcm);
			snd_pcm_close(r->pcm);
		}
		free(r);
	}
	return 0;
}

#endif
static char * sctrlstr = "head phone volume";

int getCapVal(int val)
{
#ifndef DEBUG_FILE
    long volMin, volMax, leftVal, rightVal;
	volMin = 0;  
	volMax = 0;
	leftVal = 0; 
	rightVal = 0;
	int err;
	static snd_mixer_t *handle = NULL;
	snd_mixer_elem_t *elem;
	snd_mixer_selem_id_t *sid;
	snd_mixer_selem_id_alloca(&sid);
	
	if ((err = snd_mixer_open(&handle, 0)) < 0) {
		printf("snd_mixer_open Err\n");
		return err;
	}

	snd_mixer_attach(handle, "default");
	snd_mixer_selem_register(handle, NULL, NULL);
	snd_mixer_load(handle);

	elem = snd_mixer_first_elem(handle);
	while(1)
	{
		wtk_debug( "elem name : %s\n", snd_mixer_selem_get_name (elem) );
	snd_mixer_handle_events(handle);
	snd_mixer_selem_get_capture_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, &leftVal);
	snd_mixer_selem_get_capture_volume(elem, SND_MIXER_SCHN_FRONT_RIGHT, &rightVal);
	printf("当前音量: leftVal = %ld, rightVal = %ld\n", leftVal, rightVal);

		elem = snd_mixer_elem_next(elem);
	}

	while(elem)
	{
		if ( strcmp( sctrlstr, snd_mixer_selem_get_name (elem)) == 0)
		{
			printf( "elem name : %s\n", snd_mixer_selem_get_name (elem) );
			break;
		}
	    elem = snd_mixer_elem_next(elem);
	}
	if (!elem) {
		printf("snd_mixer_find_selem Err\n");
		snd_mixer_close(handle);
		handle = NULL;
		return -ENOENT;
	}
	printf( "elem name : %s\n", snd_mixer_selem_get_name (elem) );	
	snd_mixer_selem_get_capture_volume_range(elem, &volMin, &volMax);
	printf("音量范围: %ld -- %ld\n", volMin, volMax);
	
	snd_mixer_handle_events(handle);
	snd_mixer_selem_get_capture_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, &leftVal);
	snd_mixer_selem_get_capture_volume(elem, SND_MIXER_SCHN_FRONT_RIGHT, &rightVal);
	printf("当前音量: leftVal = %ld, rightVal = %ld\n", leftVal, rightVal);
	
	//关闭混音器设备
	snd_mixer_close(handle);
	handle = NULL;
#endif
}



int setCapVal(int val)
{
#ifndef DEBUG_FILE
    long volMin, volMax, leftVal, rightVal;
	volMin = 0;  
	volMax = 0;
	leftVal = 0; 
	rightVal = 0;
	int err;
	static snd_mixer_t *handle = NULL;
	snd_mixer_elem_t *elem;
	snd_mixer_selem_id_t *sid;
	snd_mixer_selem_id_alloca(&sid);
	
	if ((err = snd_mixer_open(&handle, 0)) < 0) {
		printf("snd_mixer_open Err\n");
		return err;
	}

	snd_mixer_attach(handle, "default");
	snd_mixer_selem_register(handle, NULL, NULL);
	snd_mixer_load(handle);
  
	elem = snd_mixer_first_elem(handle);
	while(elem)
	{
		if ( strcmp( sctrlstr, snd_mixer_selem_get_name (elem)) == 0)
		{
			printf( "elem name : %s\n", snd_mixer_selem_get_name (elem) );
			break;
		}
	    elem = snd_mixer_elem_next(elem);
	}
	if (!elem) {
		printf("snd_mixer_find_selem Err\n");
		snd_mixer_close(handle);
		handle = NULL;
		return -ENOENT;
	}
	printf( "elem name : %s\n", snd_mixer_selem_get_name (elem) );	
	snd_mixer_selem_get_capture_volume_range(elem, &volMin, &volMax);
	printf("音量范围: %ld -- %ld\n", volMin, volMax);
	
	snd_mixer_handle_events(handle);
	snd_mixer_selem_get_capture_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, &leftVal);
	snd_mixer_selem_get_capture_volume(elem, SND_MIXER_SCHN_FRONT_RIGHT, &rightVal);
	// snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, &leftVal);
	// snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_RIGHT, &rightVal);
	printf("当前音量: leftVal = %ld, rightVal = %ld\n", leftVal, rightVal);
	

	if( snd_mixer_selem_is_playback_mono( elem) )
	{
		//单声道
		snd_mixer_selem_set_capture_volume(elem,SND_MIXER_SCHN_FRONT_LEFT, val);
		printf("单声道: %d\n", val);
	}
	else
	{
		//左音量
		snd_mixer_selem_set_capture_volume(elem,SND_MIXER_SCHN_FRONT_LEFT, 45);
		//右音量
		snd_mixer_selem_set_capture_volume(elem,SND_MIXER_SCHN_FRONT_RIGHT, 67);
		printf("双声道: 45,  67\n");
	}
	//关闭混音器设备
	snd_mixer_close(handle);
	handle = NULL;
#endif
}


