#include "qtk_alsa_recorder.h"

static snd_pcm_format_t qtk_alsa_get_fmt(int bytes_per_sample) {
    snd_pcm_format_t fmt;

    switch (bytes_per_sample) {
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

qtk_alsa_recorder_t *qtk_alsa_recorder_start(char *name, int sample_rate,
                                             int channel, int bytes_per_sample,
                                             int buf_time) {
    snd_pcm_hw_params_t *hw_params = 0;
    snd_pcm_format_t fmt;
    qtk_alsa_recorder_t *r;
    int size;
    int ret = -1;
    snd_pcm_uframes_t sp;

    wtk_debug("name = %s\n", name);
    //	wtk_debug("rate = %d\n",sample_rate);
    //	wtk_debug("channel = %d\n",channel);
    //	wtk_debug("bytes_per_sample = %d\n",bytes_per_sample);
    //	wtk_debug("buf_time = %d\n",buf_time);

    r = (qtk_alsa_recorder_t *)wtk_malloc(sizeof(qtk_alsa_recorder_t));
    r->rate = sample_rate;
    r->channel = channel;
    r->bytes_per_sample = bytes_per_sample;
    r->pcm = 0;
    size = r->rate * buf_time * bytes_per_sample * channel / 1000;
    ret = snd_pcm_open(&(r->pcm), name, SND_PCM_STREAM_CAPTURE, 0);
    if (ret < 0) {
        wtk_debug("ret = %d  %s\n", ret, strerror(errno));
        goto end;
    }
    ret = snd_pcm_hw_params_malloc(&hw_params);
    if (ret < 0) {
        wtk_debug("ret = %d  %s\n", ret, strerror(errno));
        goto end;
    }
    ret = snd_pcm_hw_params_any(r->pcm, hw_params);
    if (ret < 0) {
        wtk_debug("ret = %d  %s\n", ret, strerror(errno));
        goto end;
    }
    ret = snd_pcm_hw_params_set_access(r->pcm, hw_params,
                                       SND_PCM_ACCESS_RW_INTERLEAVED);
    if (ret < 0) {
        wtk_debug("ret = %d  %s\n", ret, strerror(errno));
        goto end;
    }

    fmt = qtk_alsa_get_fmt(bytes_per_sample);
    ret = snd_pcm_hw_params_set_format(r->pcm, hw_params, fmt);
    if (ret < 0) {
        wtk_debug("ret = %d  %s\n", ret, strerror(errno));
        goto end;
    }
    ret = snd_pcm_hw_params_set_rate_near(r->pcm, hw_params, &(r->rate), 0);
    if (ret < 0) {
        goto end;
    }
    ret = snd_pcm_hw_params_set_channels(r->pcm, hw_params, channel);
    if (ret < 0) {
        wtk_debug("ret = %d  %s\n", ret, strerror(errno));
        goto end;
    }
    sp = size / (bytes_per_sample * channel);
    /* ret=snd_pcm_hw_params_set_buffer_size(r->pcm,hw_params,sp); */
    /* ret=snd_pcm_hw_params_set_period_time(handler, hw_params,&time,0); */
    /* if(ret<0){ */
    /* 	wtk_debug("ret = %d  %s", ret, strerror(errno)); */
    /* 	goto end;} */
    ret = snd_pcm_hw_params_set_period_size_near(r->pcm, hw_params, &sp, 0);
    if (ret < 0) {
        wtk_debug("ret = %d  %s", ret, strerror(errno));
        goto end;
    }
    ret = snd_pcm_hw_params(r->pcm, hw_params);
    if (ret < 0) {
        wtk_debug("ret = %d  %s", ret, strerror(errno));
        goto end;
    }
    ret = snd_pcm_prepare(r->pcm);
    if (ret != 0) {
        snd_strerror(ret);
        goto end;
    }
    ret = 0;
end:
    /*
    snd_pcm_hw_params_get_buffer_size(hw_params,&sp);
    printf("%d\n",sp);
    snd_pcm_hw_params_get_period_size(hw_params,&sp,0);
    printf("%d\n",sp);
    */
    if (hw_params) {
        snd_pcm_hw_params_free(hw_params);
    }
    if (ret != 0) {
        qtk_alsa_recorder_stop(r);
        r = NULL;
    }
    return r;
}

int qtk_alsa_recorder_read(qtk_alsa_recorder_t *r, char *data, int bytes) {
    int ret;
    int step;
    step = bytes / (r->bytes_per_sample * r->channel);
    ret = snd_pcm_readi(r->pcm, data, step);
    if (ret == -32) {
        wtk_debug(">>>>>>>>>>>>>>>>>>>>>over run\n");
        ret = snd_pcm_prepare(r->pcm);
        if (ret < 0) {
            snd_pcm_recover(r->pcm, -32, 0);
            printf("Can't recovery from underrun, prepare failed: %s\n",
                   snd_strerror(ret));
        }
    } else if (ret == -86) {
        while ((ret = snd_pcm_resume(r->pcm)) == -11) {
            sleep(1);
        }
        if (ret < 0) {
            ret = snd_pcm_prepare(r->pcm);
            if (ret < 0) {
                snd_pcm_recover(r->pcm, -86, 0);
                printf("Can't recovery from suspend, prepare failed: %s\n",
                       snd_strerror(ret));
            }
        }
    } else if (ret < 0) {
        ret = snd_pcm_recover(r->pcm, ret, 0);
        if (ret < 0)
            goto end;
    }
    snd_pcm_hwsync(r->pcm);

end:
    if (ret > 0) {
        ret = ret * r->bytes_per_sample * r->channel;
    }
    return ret;
}

int qtk_alsa_recorder_stop(qtk_alsa_recorder_t *r) {
    if (r) {
        snd_pcm_reset(r->pcm);
        snd_pcm_drain(r->pcm);
        snd_pcm_close(r->pcm);
        wtk_free(r);
    }
    return 0;
}
