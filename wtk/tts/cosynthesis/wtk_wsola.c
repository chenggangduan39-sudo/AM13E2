#include "wtk_wsola.h"

wtk_wsola_t *wtk_wsola_new(wtk_wsola_cfg_t *cfg)
{
    wtk_wsola_t *wsola;
    wsola = (wtk_wsola_t *)wtk_malloc(sizeof(wtk_wsola_t));
    wsola->cfg = cfg;
    wsola->audio_len = 0;
    wsola->audios = NULL;
    return wsola;
}

int wtk_wsola_reset(wtk_wsola_t *wsola)
{
    wsola->audio_len = 0;
    if(wsola->audios)
    {
        wtk_free(wsola->audios);
    }
    wsola->audios = NULL;
    return 0;
}

int wtk_wsola_delete(wtk_wsola_t *wsola)
{
    if(wsola->audios)
    {
        wtk_free(wsola->audios);
    }
    wtk_free(wsola);
    return 0;
}

int wtk_wsola_feed_flow(wtk_wsola_t *wsola, short * audio, int len, int is_end)
{
    short *tmp_audio,*raw_audio;
    int overlap = wsola->cfg->win_sz;
    float *fade_in = wsola->cfg->window;
    float *fade_out = wsola->cfg->window + overlap;
    int max_idx,start;//,end;
    float max_corr,corr;
    short *pattern,*match;
    int i,j;//,k;
    float sum_xy,sum_xx,sum_yy, scale;
    //float t;
    //wtk_debug("len=%d\n", len);
    if(wsola->audio_len > 0)
    {
        max_idx=0;
        max_corr=0;
        pattern = wsola->audios + wsola->audio_len - overlap;
        for(i=0;i<overlap;++i)
        {
            sum_xy=0;
            sum_xx=0;
            sum_yy=0;
            match = audio + i;
            for(j=0;j<overlap;++j)
            {
                sum_xy += 1.0 * pattern[j] * match[j];
                sum_xx += 1.0 * pattern[j] * pattern[j];
                sum_yy += 1.0 * match[j] * match[j];
            }
            scale = (sqrtf(sum_xx)*sqrtf(sum_yy));
            if (scale > 0)
            	corr = sum_xy/scale;
            else
            	corr = 0;
            if(max_corr<corr)
            {
                max_corr = corr;
                max_idx = i;
            }
        }
        // wtk_debug("max_idx=%d max_corr=%f\n",max_idx,max_corr);
        start = wsola->audio_len - overlap;
        // end = start + len - max_idx;
        tmp_audio = (short*)calloc(sizeof(short),wsola->audio_len + len - max_idx - overlap);
        raw_audio = (short*)calloc(sizeof(short),len);
        memcpy(raw_audio,audio,len*sizeof(short));
        for(i=start;i<start+overlap;++i)
        {
            wsola->audios[i] = roundf(fade_out[i-start] * wsola->audios[i]);
        }
        memcpy(tmp_audio,wsola->audios,sizeof(short) * (wsola->audio_len));
        for(i=max_idx;i<max_idx+overlap;++i)
        {
            raw_audio[i] = roundf(fade_in[i-max_idx] * audio[i]);
        }
        for(i=wsola->audio_len - overlap,j=0;i<wsola->audio_len;++i)
        {
            tmp_audio[i]+=raw_audio[max_idx+j];
            j++;
        }

        if (is_end)
        {
        	memcpy(tmp_audio + wsola->audio_len ,raw_audio + max_idx+overlap,sizeof(short)*(len-max_idx-overlap));
        	wsola->audio_len = wsola->audio_len + len - max_idx - overlap;
        	wsola->notify(wsola->ths,tmp_audio,wsola->audio_len);
        }
        else
        {
        	wsola->notify(wsola->ths,tmp_audio,wsola->audio_len);
        	memcpy(tmp_audio ,raw_audio + max_idx+overlap,sizeof(short)*(len-max_idx-overlap));
        	wsola->audio_len = len - max_idx - overlap;
        }
        wtk_free(wsola->audios);
        wtk_free(raw_audio);
        wsola->audios = tmp_audio;
    }else
    {
        wsola->audio_len = len;
        wsola->audios =  (short*)wtk_malloc(sizeof(short) * wsola->audio_len);
        memcpy(wsola->audios,audio,sizeof(short)*wsola->audio_len);
        if(is_end && (wsola->audio_len > 0 && wsola->audios))
        {
            // print_short(wsola->audios,wsola->audio_len);
            wsola->notify(wsola->ths,wsola->audios,wsola->audio_len);
        }
    }

    if (is_end)
    	wsola->notify(wsola->ths, 0, 0);

    return 0;
}

int wtk_wsola_feed(wtk_wsola_t *wsola, short * audio, int len, int is_end)
{
    short *tmp_audio,*raw_audio;
    int overlap = wsola->cfg->win_sz;
    float *fade_in = wsola->cfg->window;
    float *fade_out = wsola->cfg->window + overlap;
    int max_idx,start;//,end;
    float max_corr,corr;
    short *pattern,*match;
    int i,j;//,k;
    float sum_xy,sum_xx,sum_yy, scale;
    //float t;
    //wtk_debug("len=%d\n", len);
    if(wsola->audio_len > 0)
    {
        max_idx=0;
        max_corr=0;
        pattern = wsola->audios + wsola->audio_len - overlap;
        for(i=0;i<overlap;++i)
        {
            sum_xy=0;
            sum_xx=0;
            sum_yy=0;
            match = audio + i;
            for(j=0;j<overlap;++j)
            {
                sum_xy += 1.0 * pattern[j] * match[j];
                sum_xx += 1.0 * pattern[j] * pattern[j];
                sum_yy += 1.0 * match[j] * match[j];
            }            
            scale = (sqrtf(sum_xx)*sqrtf(sum_yy));
            if (scale > 0)
            	corr = sum_xy/scale;
            else
            	corr = 0;
            if(max_corr<corr)
            {
                max_corr = corr;
                max_idx = i;
            }
        }
        // wtk_debug("max_idx=%d max_corr=%f\n",max_idx,max_corr);
        start = wsola->audio_len - overlap;
        // end = start + len - max_idx;
        tmp_audio = (short*)calloc(sizeof(short),wsola->audio_len + len - max_idx - overlap);
        raw_audio = (short*)calloc(sizeof(short),len);
        memcpy(raw_audio,audio,len*sizeof(short));
        for(i=start;i<start+overlap;++i)
        {
            wsola->audios[i] = roundf(fade_out[i-start] * wsola->audios[i]);
        }
        memcpy(tmp_audio,wsola->audios,sizeof(short) * (wsola->audio_len));
        for(i=max_idx;i<max_idx+overlap;++i)
        {
            raw_audio[i] = roundf(fade_in[i-max_idx] * audio[i]);
        }
        for(i=wsola->audio_len - overlap,j=0;i<wsola->audio_len;++i)
        {
            tmp_audio[i]+=raw_audio[max_idx+j];
            j++;
        }
        memcpy(tmp_audio + wsola->audio_len ,raw_audio + max_idx+overlap,sizeof(short)*(len-max_idx-overlap));
        wtk_free(wsola->audios);
        wtk_free(raw_audio);
        wsola->audio_len = wsola->audio_len + len - max_idx - overlap;
        wsola->audios = tmp_audio;
    }else
    {
        wsola->audio_len = len;
        wsola->audios =  (short*)wtk_malloc(sizeof(short) * wsola->audio_len);
        memcpy(wsola->audios,audio,sizeof(short)*wsola->audio_len);
    }
    if(is_end && wsola->audio_len > 0 && wsola->audios)
    {
        // print_short(wsola->audios,wsola->audio_len);
        wsola->notify(wsola->ths,wsola->audios,wsola->audio_len);
    }
    return 0;
}

void wtk_wsola_set_notify(wtk_wsola_t *wsola, void *ths, wtk_wsola_notity_f notify)
{
    wsola->ths = ths;
    wsola->notify = notify;
}
