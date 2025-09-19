#include "wtk_checkwav.h"

wtk_checkwav_t* wtk_checkwav_new(int channel, int check_maxtm, int rate, float min_en)
{
    wtk_checkwav_t *checkwav;

    checkwav = (wtk_checkwav_t *)wtk_malloc(sizeof(wtk_checkwav_t));

    checkwav->channel=channel;
    checkwav->check_maxlen=check_maxtm*rate/1000;
    checkwav->rate=rate;
    checkwav->min_en=min_en;
    checkwav->buf=wtk_strbufs_new(channel);

    checkwav->ths=NULL;
    checkwav->notify=NULL;

    wtk_checkwav_reset(checkwav);

    return checkwav;
}

void wtk_checkwav_delete(wtk_checkwav_t *checkwav)
{
    wtk_strbufs_delete(checkwav->buf, checkwav->channel);
    wtk_free(checkwav);
}

void wtk_checkwav_reset(wtk_checkwav_t *checkwav)
{
    checkwav->input=0;
    wtk_strbufs_reset(checkwav->buf,checkwav->channel);
}

void wtk_checkwav_set_notify(wtk_checkwav_t *checkwav, void *ths, wtk_checkwav_notify_f notify)
{
    checkwav->notify=notify;
    checkwav->ths=ths;
}

    // WTK_CHECKWAV_BROKE,
    // WTK_CHECKWAV_BASS,
    // WTK_CHECKWAV_ZERO,
    // WTK_CHECKWAV_CHENERGY,
void wtk_checkwav_feed(wtk_checkwav_t *checkwav,short **data,int len, int is_end)
{

#ifdef DEBUG_WAV
	static wtk_wavfile_t *mic_log=NULL;

	if(!mic_log)
	{
		mic_log=wtk_wavfile_new(16000);
		wtk_wavfile_set_channel(mic_log,checkwav->channel);
		wtk_wavfile_open2(mic_log,"checkwav");
	}
	if(len>0)
	{
		wtk_wavfile_write_mc(mic_log,data,len);
        // fflush(mic_log->file);
	}

	if(is_end && mic_log)
	{
		wtk_wavfile_close(mic_log);
		wtk_wavfile_delete(mic_log);
		mic_log=NULL;
	}
#endif
    wtk_strbuf_t **buf=checkwav->buf;
    int channel=checkwav->channel;
    int i,j,k;
    int check_maxlen=checkwav->check_maxlen;
    int rate=checkwav->rate;
    float min_en=checkwav->min_en;
    short *pv;
    int bass, zero, b;
    float en[10];

    for(j=0;j<len;++j)
    {
        b=0;
        for(i=0;i<channel;++i)
        {
            if(abs(data[i][j])>=32767)
            {
                if(checkwav->notify)
                {
                    checkwav->notify(checkwav->ths,WTK_CHECKWAV_BROKE, (checkwav->input+j+1)*1.0/rate , i+1);
                }
            }

            wtk_strbuf_push(buf[i], (char *)&(data[i][j]), sizeof(short));
            if(buf[i]->pos/sizeof(short) == check_maxlen)
            {
                b=1;
                
                bass=zero=0;

                pv=(short *)(buf[i]->data);

                en[i]=0;
                for(k=0; k<check_maxlen; ++k)
                {
                    en[i]+=pv[k]*pv[k];
                    // printf("%d %f %d %d\n",pv[k],en[i],k,check_maxlen);
                    if(pv[k] != 0)
                    {   
                        zero=1;
                    }
                    if(pv[k] > min_en)
                    {
                        bass=1;
                    }
                }
                if(zero==0)
                {
                    if(checkwav->notify)
                    {
                        checkwav->notify(checkwav->ths,WTK_CHECKWAV_ZERO, (checkwav->input+j+1)*1.0/rate , i+1);
                    }
                }
                if(bass==0)
                {
                    if(checkwav->notify)
                    {
                        checkwav->notify(checkwav->ths,WTK_CHECKWAV_BASS, (checkwav->input+j+1)*1.0/rate , i+1);
                    }
                }
                wtk_strbuf_pop(buf[i],NULL,sizeof(short)*check_maxlen/2);
            }
        }
        if(b==1)
        {
            for(i=0;i<channel;++i)
            {
                for(k=i+1;k<channel;++k)
                {
                    // printf("%f %f %f\n",en[i],en[k],en[i]/en[k]);
                    if(en[i]/en[k]<0.6)
                    {
                        if(checkwav->notify)
                        {
                            checkwav->notify(checkwav->ths,WTK_CHECKWAV_CHENERGY, (checkwav->input+j+1)*1.0/rate , i+1);
                        }
                    }
                }
            }
        }
    }
    checkwav->input+=len;
}