#include "qtk_record.h"

// #define DEBUG_W

typedef enum{
	QTK_RECORD_ASOUND_CARD_S,
	QTK_RECORD_ASOUND_CARD_E,
	QTK_RECORD_ASOUND_CARD_ID_S,
	QTK_RECORD_ASOUND_CARD_ID_E,
	QTK_RECORD_ASOUND_DEVICE_ID_S,
	QTK_RECORD_ASOUND_DEVICE_ID_E,
	QTK_RECORD_ASOUND_CARD_NAME_S,
	QTK_RECORD_ASOUND_CARD_NAME_E,
	QTK_RECORD_ASOUND_NEXT,
}qtk_record_asound_card_t;
#define QTK_RECORD_ASOUND_BUFSIZE 2048
#define QTK_RECORD_ASOUND_CARD_PATH "/proc/asound/cards"

int qtk_record_get_sound_card(char *card_name)
{
	qtk_record_asound_card_t state;
	int cards=-1;
	int i;
	char card[8],cardid[32],deviceid[32],cardname[32];
	char data[QTK_RECORD_ASOUND_BUFSIZE],*s,*e;
	int len;
	FILE *fp;
	int b;

	printf("cardneme: %s\n",card_name);
	fp = fopen(QTK_RECORD_ASOUND_CARD_PATH, "rb");
	len = fread(data, 1, QTK_RECORD_ASOUND_BUFSIZE, fp);
	fclose(fp);

	s=data;e=data+len;
	state=QTK_RECORD_ASOUND_CARD_S;
	i=0;
	b=0;
	while(s<e)
	{
		switch(state)
		{
		case QTK_RECORD_ASOUND_CARD_S:
			if(*s!=' ')
			{
				card[i++]=*s;
				state=QTK_RECORD_ASOUND_CARD_E;
			}
			break;
		case QTK_RECORD_ASOUND_CARD_E:
			if(*s==' ')
			{
				card[i]='\0';
				i=0;
				state=QTK_RECORD_ASOUND_CARD_ID_S;
			}else{
				card[i++]=*s;
			}
			break;
		case QTK_RECORD_ASOUND_CARD_ID_S:
			if(*s!=' ')
			{
				cardid[i++]=*s;
				state=QTK_RECORD_ASOUND_CARD_ID_E;
			}
			break;
		case QTK_RECORD_ASOUND_CARD_ID_E:
			if(*s==':')
			{
				cardid[i]='\0';
				i=0;
				state=QTK_RECORD_ASOUND_DEVICE_ID_S;
			}else{
				cardid[i++]=*s;
			}
			break;
		case QTK_RECORD_ASOUND_DEVICE_ID_S:
			if(*s!=' ')
			{
				deviceid[i++]=*s;
				state=QTK_RECORD_ASOUND_DEVICE_ID_E;
			}
			break;
		case QTK_RECORD_ASOUND_DEVICE_ID_E:
			if(*s==' ')
			{
				deviceid[i]='\0';
				i=0;
				state=QTK_RECORD_ASOUND_CARD_NAME_S;
			}else{
				deviceid[i++]=*s;
			}
			break;
		case QTK_RECORD_ASOUND_CARD_NAME_S:
			if(*s!=' ' && *s!='-')
			{
				cardname[i++]=*s;
				state=QTK_RECORD_ASOUND_CARD_NAME_E;
			}
			break;
		case QTK_RECORD_ASOUND_CARD_NAME_E:
			if(*s=='\n')
			{
				cardname[i]='\0';
				i=0;
				state=QTK_RECORD_ASOUND_NEXT;
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
		case QTK_RECORD_ASOUND_NEXT:
			if(*s=='\n')
			{
				state=QTK_RECORD_ASOUND_CARD_S;
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

qtk_record_t *qtk_record_new(qtk_record_cfg_t *cfg)
{
    qtk_record_t *rcd;
    int ret;
    int size;

    rcd = (qtk_record_t *)calloc(1, sizeof(*rcd));
    rcd->cfg = cfg;
    if(rcd->cfg->use_gain_set){
        qtk_record_set_mic_gain(rcd);
        qtk_record_set_cb_gain(rcd);
    }
	if(cfg->use_get_card == 1){
		int cards;
		char snd_name[32];
		cards= qtk_record_get_sound_card(cfg->snd_name);
		if(cards < 0){
				return NULL;
		}
		snprintf(snd_name, 32, "hw:%d,0", cards);
		wtk_debug("==================>>>>>>>>>>>record snd_name = %s\n", snd_name);
		// rcd->alsa = qtk_alsa_recorder_start(snd_name, cfg->sample_rate, cfg->channel, cfg->bytes_per_sample, cfg->buf_time);
		rcd->alsa = qtk_alsa_recorder_start2(snd_name, cfg->sample_rate, cfg->channel, cfg->bytes_per_sample, cfg->buf_time);
		if(!rcd->alsa){
			printf(">>>>>alsa recorder start failed.\n");
			ret = -1;
			goto end;
		}
	}else{
		// rcd->alsa = qtk_alsa_recorder_start(cfg->snd_name, cfg->sample_rate, cfg->channel, cfg->bytes_per_sample, cfg->buf_time);
		rcd->alsa = qtk_alsa_recorder_start2(cfg->snd_name, cfg->sample_rate, cfg->channel, cfg->bytes_per_sample, cfg->buf_time);
		if(!rcd->alsa){
			printf(">>>>>alsa recorder start failed.\n");
			ret = -1;
			goto end;
		}
	}
#ifndef DEBUG_FILE
    size = cfg->buf_time*cfg->sample_rate*cfg->bytes_per_sample*cfg->channel/1000;
#else
	size = cfg->buf_time*cfg->sample_rate*cfg->bytes_per_sample*(cfg->channel - cfg->nskip)/1000;
#endif
    rcd->buf = wtk_strbuf_new(size, 1.0);
    memset(rcd->buf->data,0,rcd->buf->length);

    ret = 0;
end:
    if(ret != 0){
        qtk_record_delete(rcd);
        rcd = NULL;
    }
    return rcd;
}
void qtk_record_delete(qtk_record_t *rcd)
{
    if(rcd->buf){
        wtk_strbuf_delete(rcd->buf);
    }
    if(rcd->alsa){
        qtk_alsa_recorder_stop(rcd->alsa);
    }
    free(rcd);
}
wtk_strbuf_t *qtk_record_read(qtk_record_t *rcd)
{
    rcd->buf->pos = qtk_alsa_recorder_read(rcd->alsa, rcd->buf->data, rcd->buf->length);
    if(rcd->buf->pos <= 0){
        printf(">>>read bytes = %d\n", rcd->buf->pos);
        return rcd->buf;
    }

    if(rcd->cfg->nskip > 0 && rcd->cfg->use_log_ori_audio){
        short *pv,*pv1;
		int i,j,k,len;
		int pos,pos1;
		int b;

		pv = pv1 = (short*)rcd->buf->data;
		pos = pos1 = 0;
		len = rcd->buf->pos / (2 * rcd->cfg->channel);
		for(i=0;i < len; ++i){
			for(j=0;j < rcd->cfg->channel; ++j) {
				b = 0;
				for(k=0;k<rcd->cfg->nskip;++k) {
					if(j == rcd->cfg->skip_channels[k]) {
						b = 1;
					}
				}
				if(b) {
					++pos1;
				} else {
					pv[pos++] = pv1[pos1++];
				}
			}
		}
		rcd->buf->pos = pos << 1;
    }
    
    return rcd->buf;
}


void qtk_record_set_mic_gain(qtk_record_t *rcd)
{
#ifndef DEBUG_W
    char gain[4];
    char params[256];
	int ret=-1;
    printf("set mic gain: %d\n", rcd->cfg->mic_gain);

#if defined(USE_R328) || defined(USE_R311)
    snprintf(gain, 4, "%2x", rcd->cfg->mic_gain);
    snprintf(params, 256, "echo 1,0x30,0x%s%s%s%s > /sys/class/dmic/dmic_reg_debug", gain,gain,gain,gain);
    printf("%s\n", params);
    ret = system(params);
	while(ret < 0)
	{
		wtk_debug("restart set mic gain!\n");
		ret = system(params);
	}
	
    snprintf(params, 256, "echo 1,0x34,0x%s%s%s%s > /sys/class/dmic/dmic_reg_debug", gain,gain,gain,gain);
    printf("%s\n", params);
    ret = system(params);
	while(ret < 0)
	{
		wtk_debug("restart set mic gain!\n");
		ret = system(params);
	}
#endif
#if defined(USE_802A) || defined(USE_BMC)//def USE_802A
    snprintf(params, 256, "amixer -c 1 cset numid=12 %d", rcd->cfg->mic_gain);
    printf("%s\n", params);
    ret = system(params);
	while(ret < 0)
	{
		wtk_debug("restart set mic gain!\n");
		ret = system(params);
	}
	snprintf(params, 256, "amixer -c 1 cset numid=13 %d", rcd->cfg->mic_gain);
    printf("%s\n", params);
    ret = system(params);
	while(ret < 0)
	{
		wtk_debug("restart set mic gain!\n");
		ret = system(params);
	}
	snprintf(params, 256, "amixer -c 1 cset numid=14 %d", rcd->cfg->mic_gain);
    printf("%s\n", params);
    ret = system(params);
	while(ret < 0)
	{
		wtk_debug("restart set mic gain!\n");
		ret = system(params);
	}
	snprintf(params, 256, "amixer -c 1 cset numid=15 %d", rcd->cfg->mic_gain);
    printf("%s\n", params);
    ret = system(params);
	while(ret < 0)
	{
		wtk_debug("restart set mic gain!\n");
		ret = system(params);
	}
	snprintf(params, 256, "amixer -c 1 cset numid=28 %d", rcd->cfg->mic_gain);
    printf("%s\n", params);
    ret = system(params);
	while(ret < 0)
	{
		wtk_debug("restart set mic gain!\n");
		ret = system(params);
	}
	snprintf(params, 256, "amixer -c 1 cset numid=29 %d", rcd->cfg->mic_gain);
    printf("%s\n", params);
    ret = system(params);
	while(ret < 0)
	{
		wtk_debug("restart set mic gain!\n");
		ret = system(params);
	}
#endif
	// ret = -1;
	// wtk_debug("#################################\n");
	// while(ret < 0)
	// {
	// 	ret = system("cat /sys/class/dmic/dmic_reg_debug");
	// }
	// wtk_debug("#################################\n");
#endif
}

void qtk_record_set_cb_gain(qtk_record_t *rcd)
{
#ifndef DEBUG_W
    printf("set cb gain: %d\n", rcd->cfg->cb_gain);
#ifdef USE_R328
	int ret = -1;
    char params[256];
    snprintf(params, 256, "amixer cset numid=4 %d", rcd->cfg->cb_gain);
    printf("%s\n", params);
    ret = system(params);
	while(ret < 0)
	{
		wtk_debug("restart set cb gain!\n");
		ret = system(params);
	}

    snprintf(params, 256, "amixer cset numid=5 %d", rcd->cfg->cb_gain);
    printf("%s\n", params);
    ret = system(params);
	while(ret < 0)
	{
		wtk_debug("restart set cb gain!\n");
		ret = system(params);
	}
#endif

#ifdef USE_R311
    char params[256];
	int ret;
    snprintf(params, 256, "echo 1A7%02x > /sys/bus/platform/devices/twi0/i2c-0/0-0036/ac107_debug/ac107", rcd->cfg->cb_gain);
    printf("%s\n", params);
    ret = system(params);
	while(ret < 0)
	{
		wtk_debug("restart set cb gain!\n");
		ret = system(params);
	}

    snprintf(params, 256, "echo 1A2%02x > /sys/bus/platform/devices/twi0/i2c-0/0-0036/ac107_debug/ac107", rcd->cfg->cb_gain);
    printf("%s\n", params);
    ret = system(params);
	while(ret < 0)
	{
		wtk_debug("restart set cb gain!\n");
		ret = system(params);
	}
#endif

#if defined(USE_802A) || defined(USE_BMC)//def USE_802A
    char params[256];
	int ret;
	snprintf(params, 256, "amixer -c 1 cset numid=30 %d", rcd->cfg->cb_gain);
    printf("%s\n", params);
    ret = system(params);
	while(ret < 0)
	{
		wtk_debug("restart set cb gain!\n");
		ret = system(params);
	}
	snprintf(params, 256, "amixer -c 1 cset numid=31 %d", rcd->cfg->cb_gain);
    printf("%s\n", params);
    ret = system(params);
	while(ret < 0)
	{
		wtk_debug("restart set cb gain!\n");
		ret = system(params);
	}
#endif

#endif
}
