#include "qtk_recorder.h"

void qtk_record_set_mic_gain(qtk_recorder_t *rcd);
qtk_recorder_t *qtk_recorder_new(qtk_recorder_cfg_t *cfg, wtk_log_t *log) {
    qtk_recorder_t *r;

    r = (qtk_recorder_t *)wtk_malloc(sizeof(qtk_recorder_t));
    memset(r, 0, sizeof(sizeof(qtk_recorder_t)));
    r->cfg = cfg;
    if (r->cfg->use_gain_set) {
        qtk_record_set_mic_gain(r);
    }
    r->log = log;

    r->sample_rate = r->cfg->sample_rate;
    r->channel = r->cfg->channel;
    r->bytes_per_sample = r->cfg->bytes_per_sample;

    r->buf = wtk_strbuf_new(cfg->buf_size, 1);
    memset(r->buf->data, 0, r->buf->length);

    return r;
}

void qtk_recorder_delete(qtk_recorder_t *r) {
    wtk_strbuf_delete(r->buf);
    wtk_free(r);
}

void qtk_recorder_set_fmt(qtk_recorder_t *r, int sample_rate, int channel,
                          int bytes_per_sample) {
    r->sample_rate = sample_rate > 0 ? sample_rate : r->sample_rate;
    r->channel = channel > 0 ? channel : r->channel;
    r->bytes_per_sample =
        bytes_per_sample > 0 ? bytes_per_sample : r->bytes_per_sample;

    wtk_log_log(r->log, "rate/channel/bytes_per_sample %d/%d/%d  %d/%d/%d",
                sample_rate, channel, bytes_per_sample, r->sample_rate,
                r->channel, r->bytes_per_sample);
}

wtk_strbuf_t *qtk_recorder_read(qtk_recorder_t *r) {
    r->buf->pos = qtk_alsa_recorder_read(r->rcd, r->buf->data, r->buf->length);
    if (r->buf->pos <= 0) {
        ++r->read_fail_times;
        wtk_log_warn(r->log, "read bytes %d", r->buf->pos);
        return r->buf;
    } else {
        r->read_fail_times = 0;
    }

    if (r->cfg->skip_channels) {
        short *pv, *pv1;
        int i, j, k, len;
        int pos, pos1;
        int b;

        pv = pv1 = (short *)r->buf->data;
        pos = pos1 = 0;
        len = r->buf->pos / (2 * r->cfg->channel);
        for (i = 0; i < len; ++i) {
            for (j = 0; j < r->cfg->channel; ++j) {
                b = 0;
                for (k = 0; k < r->cfg->nskip; ++k) {
                    if (j == r->cfg->skip_channels[k]) {
                        b = 1;
                    }
                }
                if (b) {
                    ++pos1;
                } else {
                    pv[pos++] = pv1[pos1++];
                }
            }
        }
        r->buf->pos = pos << 1;
    }

    return r->buf;
}

int qtk_recorder_start(qtk_recorder_t *r) {
    int ret;

    r->rcd =
        qtk_alsa_recorder_start(r->cfg->snd_name, r->sample_rate, r->channel,
                                r->bytes_per_sample, r->cfg->buf_time);
    if (r->rcd) {
        ret = 0;
    } else {
        ret = -1;
    }
    return ret;
}

void qtk_recorder_stop(qtk_recorder_t *r) { qtk_alsa_recorder_stop(r->rcd); }

int qtk_recorder_get_channel(qtk_recorder_t *r) {
    return r->cfg->channel - r->cfg->nskip;
}

void qtk_record_set_mic_gain(qtk_recorder_t *r) {
    char gain[4];
    char params[256];
    printf("set mic gain: %d\n", r->cfg->mic_gain);
    snprintf(gain, 4, "%2x", r->cfg->mic_gain);
    snprintf(params, 256,
             "echo 1,0x30,0x%s%s%s%s > /sys/class/dmic/dmic_reg_debug", gain,
             gain, gain, gain);
    printf("%s\n", params);
    int ret=-1;
    ret=system(params);
    if(ret<0)
    {
        printf("system %s fails\n",params);
    }
}
