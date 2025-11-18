#include "qtk_player.h" 

#ifdef MTK8516
void gpio_open(unsigned int gpio)
{
		uiGpioExport(gpio);
}

void gpio_close(unsigned int gpio)
{
		uiGpioUnexport(gpio);
}

//direction: 1:out, 0:in
//value: 1:high, 0:low
void gpio_config(unsigned int gpio,unsigned int direction,unsigned int value)
{
		uiGpioSetDir(gpio, direction);
		uiGpioSetValue(gpio,value);
}
#endif

typedef enum{
	QTK_ASOUND_CARD_S,
	QTK_ASOUND_CARD_E,
	QTK_ASOUND_CARD_ID_S,
	QTK_ASOUND_CARD_ID_E,
	QTK_ASOUND_DEVICE_ID_S,
	QTK_ASOUND_DEVICE_ID_E,
	QTK_ASOUND_CARD_NAME_S,
	QTK_ASOUND_CARD_NAME_E,
	QTK_ASOUND_NEXT,
}qtk_asound_card_t;
#define QTK_ASOUND_BUFSIZE 2048
#define QTK_ASOUND_CARD_PATH "/proc/asound/cards"

int qtk_player_get_sound_card(char *card_name)
{
	qtk_asound_card_t state;
	int cards=-1;
	int i;
	char card[8],cardid[32],deviceid[32],cardname[32];
	char data[QTK_ASOUND_BUFSIZE],*s,*e;
	int len;
	FILE *fp;
	int b;

	printf("cardneme: %s\n",card_name);
	fp = fopen(QTK_ASOUND_CARD_PATH, "rb");
	len = fread(data, 1, QTK_ASOUND_BUFSIZE, fp);
	fclose(fp);

	s=data;e=data+len;
	state=QTK_ASOUND_CARD_S;
	i=0;
	b=0;
	while(s<e)
	{
		switch(state)
		{
		case QTK_ASOUND_CARD_S:
			if(*s!=' ')
			{
				card[i++]=*s;
				state=QTK_ASOUND_CARD_E;
			}
			break;
		case QTK_ASOUND_CARD_E:
			if(*s==' ')
			{
				card[i]='\0';
				i=0;
				state=QTK_ASOUND_CARD_ID_S;
			}else{
				card[i++]=*s;
			}
			break;
		case QTK_ASOUND_CARD_ID_S:
			if(*s!=' ')
			{
				cardid[i++]=*s;
				state=QTK_ASOUND_CARD_ID_E;
			}
			break;
		case QTK_ASOUND_CARD_ID_E:
			if(*s==':')
			{
				cardid[i]='\0';
				i=0;
				state=QTK_ASOUND_DEVICE_ID_S;
			}else{
				cardid[i++]=*s;
			}
			break;
		case QTK_ASOUND_DEVICE_ID_S:
			if(*s!=' ')
			{
				deviceid[i++]=*s;
				state=QTK_ASOUND_DEVICE_ID_E;
			}
			break;
		case QTK_ASOUND_DEVICE_ID_E:
			if(*s==' ')
			{
				deviceid[i]='\0';
				i=0;
				state=QTK_ASOUND_CARD_NAME_S;
			}else{
				deviceid[i++]=*s;
			}
			break;
		case QTK_ASOUND_CARD_NAME_S:
			if(*s!=' ' && *s!='-')
			{
				cardname[i++]=*s;
				state=QTK_ASOUND_CARD_NAME_E;
			}
			break;
		case QTK_ASOUND_CARD_NAME_E:
			if(*s=='\n')
			{
				cardname[i]='\0';
				i=0;
				state=QTK_ASOUND_NEXT;
				if( strncmp(cardname,card_name,strlen(card_name))==0)
				{
					b=1;
					printf("get asound card success card = %s. cardid = %s. deviceid = %s. cardname = %s.\n",card, cardid, deviceid, cardname);
					goto end;
				}
			}else{
				cardname[i++]=*s;
			}
			break;
		case QTK_ASOUND_NEXT:
			if(*s=='\n')
			{
				state=QTK_ASOUND_CARD_S;
			}
			break;
		}
		++s;
	}
end:
	if(b)
	{
		cards=atoi(card);
	}else{
		printf("[ERROR]:Get asound card failed.\n");
	}
	return cards;
}

qtk_player_t* qtk_player_new(qtk_player_cfg_t *cfg,qtk_session_t *session,void *notify_ths,qtk_player_notify_f notify_func)
{
	qtk_player_t *p;

	p = (qtk_player_t*)wtk_malloc(sizeof(qtk_player_t));
	memset(p, 0, sizeof(*p));
	p->cfg = cfg;
	if(session)
	{
		p->log = session->log;
	}

	p->err_notify_func = NULL;
	p->err_notify_ths = NULL;
	p->buf = wtk_strbuf_new(640, 1);
	if(p->cfg->use_uac)
	{
		p->frames = NULL;
	}else{
		p->frames = (audio_frame_t *)malloc(sizeof(audio_frame_t )*AUDIO_DATA_CHANNEL * cfg->buf_time * 32 *100);
	}

	qtk_player_module_init(&p->plyer_module);
#ifdef MTK8516
	//gpio_open(SPEAKER_ENABLE_PIN);
#endif
	return p;
}

int qtk_player_delete(qtk_player_t *p)
{
#ifdef MTK8516
	//gpio_close(SPEAKER_ENABLE_PIN);
#endif
	if(p->buf){
		wtk_strbuf_delete(p->buf);
	}
	if(p->frames)
	{
		wtk_free(p->frames);
	}
	free(p);
	return 0;
}

void qtk_player_set_err_notify(qtk_player_t *p,
		void *err_notify_ths,
		qtk_player_notify_f err_notify_func
		)
{
	p->err_notify_func = err_notify_func;
	p->err_notify_ths = err_notify_ths;
}

void qtk_player_set_callback(qtk_player_t *p,
		void *handler,
		qtk_player_start_func start_func,
		qtk_player_stop_func  stop_func,
		qtk_player_write_func write_func,
		qtk_player_clean_func clean_func
		)
{
	qtk_player_module_set_callback(&p->plyer_module,
			handler,
			start_func,
			stop_func,
			write_func,
			clean_func
			);
}

int qtk_player_start(qtk_player_t *p,char *snd_name,int sample_rate,int channel,int bytes_per_sample)
{
	int ret;

	wtk_log_log(p->log,"sample_rate %d channel %d bytes_per_sample %d buf_time %d",
			sample_rate,channel,bytes_per_sample,p->cfg->buf_time
			);
	p->write_fail_times = 0;
	p->step = sample_rate * channel * bytes_per_sample * p->cfg->buf_time/1000;
#if (defined __ANDROID__) || (defined USE_XDW)
	{
		int card;
		int pdev[2];

		if(p->cfg->use_for_bfio) {
			// card = qtk_asound_get_card();
			// if (card >= 0)
			{
				// pdev[0] = card;
				pdev[0] = atoi(p->cfg->snd_name);
				pdev[1] = p->cfg->device_number;
				wtk_log_log(p->log,"card %d device %d",pdev[0],pdev[1]);
				p->plyer_module.ths = p->plyer_module.start_func(p->plyer_module.handler,
						(char*)&pdev,
						sample_rate,
						channel,
						bytes_per_sample,
						p->cfg->buf_time,
						p->cfg->period_time,
						p->cfg->use_uac
						);
				ret = p->plyer_module.ths?0:-1;
				goto end;
			}
		} else {
			pdev[0] = atoi(p->cfg->snd_name);
			pdev[1] = p->cfg->device_number;
			wtk_log_log(p->log,"card %d device %d",pdev[0],pdev[1]);
			p->plyer_module.ths = p->plyer_module.start_func(p->plyer_module.handler,
					(char*)&pdev,
					p->cfg->sample_rate,
					p->cfg->channel,
					p->cfg->bytes_per_sample,
					p->cfg->buf_time,
					p->cfg->period_time,
					p->cfg->use_uac
					);
			ret = p->plyer_module.ths?0:-1;
			goto end;
		}
	}

#else
	{
		int cards;
		char snd_name[32];


		if(p->cfg->use_for_bfio) {
			cards= qtk_player_get_sound_card(p->cfg->snd_name);
			if(cards < 0){
					return -1;
			}

			if(cards >= 0) {
#ifdef MTK8516

#ifdef USE_PLAY_BIT32
				sprintf(snd_name,"hw:0,6");
#else
				//sprintf(name,"sub0");
#ifdef USE_HAOYI
				sprintf(snd_name,"hw:0,0");
#else
				sprintf(snd_name,"hw:0,6");
#endif
#endif
				//gpio_config(SPEAKER_ENABLE_PIN,1,1);
#else
				snprintf(snd_name, 32, "hw:%d,0", cards);
#endif
				p->plyer_module.ths = p->plyer_module.start_func(p->plyer_module.handler,
						snd_name,
						sample_rate,
						channel,
						bytes_per_sample,
						p->cfg->buf_time,
						p->cfg->period_time,
						p->cfg->start_time,
						p->cfg->stop_time,
						p->cfg->avail_time,
						p->cfg->silence_time,
						p->cfg->use_uac
						);
				ret = p->plyer_module.ths?0:-1;
				goto end;
			}
		} else if(p->cfg->snd_name) {
			wtk_log_log(p->log,"snd_name %s",p->cfg->snd_name);
			p->plyer_module.ths = p->plyer_module.start_func(p->plyer_module.handler,
					p->cfg->snd_name,
					sample_rate,
					channel,
					bytes_per_sample,
					p->cfg->buf_time,
					p->cfg->period_time,
					p->cfg->start_time,
					p->cfg->stop_time,
					p->cfg->avail_time,
					p->cfg->silence_time,
					p->cfg->use_uac
					);
			ret =  p->plyer_module.ths?0:-1;
			goto end;
		}
	}
#endif

	ret = -1;
end:
	if(ret != 0) {
		if(p->err_notify_func) {
			p->err_notify_func(p->err_notify_ths,1);
		}
	}
	return ret;
}

void qtk_player_stop(qtk_player_t *p)
{
	if(p->plyer_module.stop_func) {
		wtk_log_log0(p->log,"stop player");
		p->plyer_module.stop_func(p->plyer_module.handler,p->plyer_module.ths);
	}
#ifdef MTK8516
	//gpio_config(SPEAKER_ENABLE_PIN,1,0);
#endif
}

int qtk_player_on_write(qtk_player_t *p,char *data,int bytes)
{
	int ret;

	ret = p->plyer_module.write_func(p->plyer_module.handler,p->plyer_module.ths,data,bytes);
	if(ret <= 0) {
		++p->write_fail_times;
		if(p->err_notify_func && p->write_fail_times > p->cfg->max_write_fail_times) {
			p->err_notify_func(p->err_notify_ths,1);
		}
	} else {
		p->write_fail_times = 0;
	}
	return ret;
}

int qtk_player_write(qtk_player_t *p, char *data, int bytes)
{
#if 0
	int tmp_len;

	while(bytes > 0){
		tmp_len = min(p->step - p->buf->pos, bytes);
		wtk_strbuf_push(p->buf, data, tmp_len);
		data += tmp_len;
		bytes -= tmp_len;
		if(p->buf->pos == p->step){
			qtk_player_on_write(p, p->buf->data, p->buf->pos);
			wtk_strbuf_reset(p->buf);
		}
	}
	return 0;
#else
	return qtk_player_on_write(p, data, bytes);
#endif
}

void audio_frame_fill(audio_frame_t *pframe, unsigned short data, unsigned char number)
{
	pframe->data = data;
	pframe->indx = number;
	pframe->zero = 0x00;
}

int qtk_play_data_frame_fill(qtk_player_t *play, char *data, int len, int channel)
{
	int i,j;
	short *pv = (short *)data;
	int step = len/(channel *sizeof(short));
    
	play->frames_len = 0;
	for(i = 0; i < step; ++i){
		for(j = 0; j < AUDIO_DATA_CHANNEL; ++j){
			if(j < channel){
				play->frames[i * AUDIO_DATA_CHANNEL + j].data = pv[i + j*step];
				play->frames[i * AUDIO_DATA_CHANNEL + j].indx = j + 1;
				play->frames[i * AUDIO_DATA_CHANNEL + j].zero = 0x00;
			}else{
				play->frames[i * AUDIO_DATA_CHANNEL + j].data = 0x00;
				play->frames[i * AUDIO_DATA_CHANNEL + j].indx = j + 1;
				play->frames[i * AUDIO_DATA_CHANNEL + j].zero = 0x00;
			}
			play->frames_len++;
		}
	}
	return play->frames_len;
}

int qtk_player_write2(qtk_player_t *p, char *data, int bytes, int channel)
{
	int ret=-1;

	if(p->cfg->use_uac)
	{
		ret = qtk_player_write(p, data, bytes);
	}else{
		qtk_play_data_frame_fill(p, data, bytes, channel);
		ret = qtk_player_write(p, (char *)p->frames, sizeof(audio_frame_t)*p->frames_len);
	}
	return ret;
}

void qtk_player_clean(qtk_player_t *p)
{
	if(p->plyer_module.clean_func)
	{
		p->plyer_module.clean_func(p->plyer_module.handler,p->plyer_module.ths);
	}
}

int qtk_player_isErr(qtk_player_t *p)
{
	return p->write_fail_times > p->cfg->max_write_fail_times;
}
