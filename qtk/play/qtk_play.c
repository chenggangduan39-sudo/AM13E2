#include "qtk_play.h"

typedef enum{
	QTK_PLAY_ASOUND_CARD_S,
	QTK_PLAY_ASOUND_CARD_E,
	QTK_PLAY_ASOUND_CARD_ID_S,
	QTK_PLAY_ASOUND_CARD_ID_E,
	QTK_PLAY_ASOUND_DEVICE_ID_S,
	QTK_PLAY_ASOUND_DEVICE_ID_E,
	QTK_PLAY_ASOUND_CARD_NAME_S,
	QTK_PLAY_ASOUND_CARD_NAME_E,
	QTK_PLAY_ASOUND_NEXT,
}qtk_play_asound_card_t;
#define QTK_PLAY_ASOUND_BUFSIZE 2048
#define QTK_PLAY_ASOUND_CARD_PATH "/proc/asound/cards"

int qtk_play_get_sound_card(char *card_name)
{
	qtk_play_asound_card_t state;
	int cards=-1;
	int i;
	char card[8],cardid[32],deviceid[32],cardname[32];
	char data[QTK_PLAY_ASOUND_BUFSIZE],*s,*e;
	int len;
	FILE *fp;
	int b;

	printf("cardneme: %s\n",card_name);
	fp = fopen(QTK_PLAY_ASOUND_CARD_PATH, "rb");
	len = fread(data, 1, QTK_PLAY_ASOUND_BUFSIZE, fp);
	fclose(fp);

	s=data;e=data+len;
	state=QTK_PLAY_ASOUND_CARD_S;
	i=0;
	b=0;
	while(s<e)
	{
		switch(state)
		{
		case QTK_PLAY_ASOUND_CARD_S:
			if(*s!=' ')
			{
				card[i++]=*s;
				state=QTK_PLAY_ASOUND_CARD_E;
			}
			break;
		case QTK_PLAY_ASOUND_CARD_E:
			if(*s==' ')
			{
				card[i]='\0';
				i=0;
				state=QTK_PLAY_ASOUND_CARD_ID_S;
			}else{
				card[i++]=*s;
			}
			break;
		case QTK_PLAY_ASOUND_CARD_ID_S:
			if(*s!=' ')
			{
				cardid[i++]=*s;
				state=QTK_PLAY_ASOUND_CARD_ID_E;
			}
			break;
		case QTK_PLAY_ASOUND_CARD_ID_E:
			if(*s==':')
			{
				cardid[i]='\0';
				i=0;
				state=QTK_PLAY_ASOUND_DEVICE_ID_S;
			}else{
				cardid[i++]=*s;
			}
			break;
		case QTK_PLAY_ASOUND_DEVICE_ID_S:
			if(*s!=' ')
			{
				deviceid[i++]=*s;
				state=QTK_PLAY_ASOUND_DEVICE_ID_E;
			}
			break;
		case QTK_PLAY_ASOUND_DEVICE_ID_E:
			if(*s==' ')
			{
				deviceid[i]='\0';
				i=0;
				state=QTK_PLAY_ASOUND_CARD_NAME_S;
			}else{
				deviceid[i++]=*s;
			}
			break;
		case QTK_PLAY_ASOUND_CARD_NAME_S:
			if(*s!=' ' && *s!='-')
			{
				cardname[i++]=*s;
				state=QTK_PLAY_ASOUND_CARD_NAME_E;
			}
			break;
		case QTK_PLAY_ASOUND_CARD_NAME_E:
			if(*s=='\n')
			{
				cardname[i]='\0';
				i=0;
				state=QTK_PLAY_ASOUND_NEXT;
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
		case QTK_PLAY_ASOUND_NEXT:
			if(*s=='\n')
			{
				state=QTK_PLAY_ASOUND_CARD_S;
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

// FILE *ff;

qtk_play_t *qtk_play_new(qtk_play_cfg_t *cfg)
{
	qtk_play_t *play;

	play = (qtk_play_t *)calloc(1, sizeof(*play));
	play->cfg = cfg;
	if(play->cfg->use_uac == 0)
	{
		play->frames = (audio_frame_t *)malloc(sizeof(audio_frame_t )*AUDIO_DATA_CHANNEL * cfg->buf_time * 32 *100);
	}else{
		play->frames = NULL;
	}

	// ff = fopen("/mnt/UDISK/consist/aa.pcm","wb+");
	return play;
    
}

int qtk_play_delete(qtk_play_t *play)
{
	// fclose(ff);
	free(play->frames);
	free(play);
	return 0;
}

int qtk_play_start(qtk_play_t *play)
{
	if(play->cfg->use_get_soundcard)
	{
		int cards=0;
		char snd_name[32];
		
		cards= qtk_play_get_sound_card(play->cfg->snd_name.data);
		if(cards < 0){
			return -1;
		}
		snprintf(snd_name, 32, "hw:%d,0", cards);
		// play->alsa = qtk_alsa_player_start(snd_name, play->cfg->sample_rate, play->cfg->channel, play->cfg->bytes_per_sample, play->cfg->buf_time, play->cfg->use_uac);
		play->alsa = qtk_alsa_player_start4(snd_name, play->cfg->sample_rate,
		 play->cfg->channel,
		 play->cfg->bytes_per_sample,
		 play->cfg->buf_time,
		 play->cfg->period_time,
		 play->cfg->start_time,
		 play->cfg->stop_time,
		 play->cfg->avail_time,
		 play->cfg->silence_time,
		 play->cfg->use_uac);
	}else{
		play->alsa = qtk_alsa_player_start(play->cfg->snd_name.data, play->cfg->sample_rate, play->cfg->channel, play->cfg->bytes_per_sample, play->cfg->buf_time, play->cfg->period_time, play->cfg->use_uac);
	}
	if(!play->alsa){
		return -1;
	}
	return 0;
}

int qtk_play_stop(qtk_play_t *play)
{
    return qtk_alsa_player_stop(play->alsa);
}

long qtk_play_write_on_write(qtk_play_t *play, char *data, int len)
{ 
	// fwrite(data, len, 1, ff);
	// fflush(ff);
    return qtk_alsa_player_write(play->alsa, data, len);
}

void audio_frame_fill(audio_frame_t *pframe, unsigned short data, unsigned char number)
{
	pframe->data = data;
	pframe->indx = number;
	pframe->zero = 0x00;
}

int qtk_play_data_frame_fill(qtk_play_t *play, char *data, int len, int channel)
{
	int i,j;
	short *pv = (short *)data;
	int step = len/(channel *sizeof(short));
	audio_frame_t *tmpf;

    tmpf=play->frames;
	play->frames_len = 0;
	for(i = 0; i < step; ++i){
		for(j = 0; j < AUDIO_DATA_CHANNEL; ++j){
			if(j < channel){
				// wtk_debug("========================<>>>>i=%d j=%d\n",i,j);
				// play->frames[i * AUDIO_DATA_CHANNEL + j].data = pv[i + j*step];
				// play->frames[i * AUDIO_DATA_CHANNEL + j].indx = j + 1;
				// play->frames[i * AUDIO_DATA_CHANNEL + j].zero = 0x00;
				tmpf->data =  *(pv+(i + j*step));
				tmpf->indx = j + 1;
				tmpf->zero = 0x00;
			}else{
				// play->frames[i * AUDIO_DATA_CHANNEL + j].data = 0x00;
				// play->frames[i * AUDIO_DATA_CHANNEL + j].indx = j + 1;
				// play->frames[i * AUDIO_DATA_CHANNEL + j].zero = 0x00;
				tmpf->data = 0x00;
				tmpf->indx = j + 1;
				tmpf->zero = 0x00;
			}
			tmpf++;
			play->frames_len++;
		}
	}
	return play->frames_len;
}

#if 0
int qtk_play_data_fill(qtk_play_t *play, char *data, int len)
{
    int i,j;
    short *pv = (short *)data;
    
    play->data_8ch_len = 0;
    for(i = 0; i < len >>1; ++i){
        for(j = 0; j < AUDIO_DATA_CHANNEL; ++j){
            if(j == 0){
                play->data_8ch[i * AUDIO_DATA_CHANNEL + j] = pv[i];
            }else{
                play->data_8ch[i * AUDIO_DATA_CHANNEL + j] = (short)0;
            }
            play->data_8ch_len++;
        }
    }
    return play->data_8ch_len;
}
#endif

long qtk_play_write(qtk_play_t *play, char *data, int len, int channel)
{
	long ret;
	// wtk_debug("------------------->>>>>>>>play->cfg->use_uac = %d\n",play->cfg->use_uac);
	if(play->cfg->use_uac)
	{	
		ret = qtk_play_write_on_write(play, data, len);
	}else{
		qtk_play_data_frame_fill(play, data, len, channel);
		ret = qtk_play_write_on_write(play, (char *)play->frames, sizeof(audio_frame_t)*play->frames_len);
	}
	return ret;
}

int qtk_play_get_count(qtk_play_t *play)
{
	return qtk_alsa_player_get_count(play->alsa);
}
