#include "wtk/bfio/qtk_wasr.h"

static void _buf_read(void *arg, char *d, int l) {
    qtk_wasr_t *w = cast(qtk_wasr_t *, arg);
    qtk_decoder_wrapper_feed(w->decoder, d, l, 0);
}

static void _on_vad2(qtk_wasr_t *w, wtk_vad2_cmd_t cmd, short *data, int len) {
    float wake_fs, wake_fe;
    wtk_string_t rec_res;
    qtk_wasr_res_t wres;
    float sample_rate = w->cfg->audio_sample_rate;
    int wake_end_pos, skip_len, pad_len, pad_len_bak;

    switch (cmd) {
    case WTK_VAD2_START:
        qtk_decoder_wrapper_start(w->decoder);
        wtk_kwake_start(w->wakeup);
        w->speeching = 1;
        w->wake_start = w->vad_output;
        w->rec_fs = w->vad_output / sample_rate;

        break;
    case WTK_VAD2_DATA:
        wtk_buf_push(w->cache, cast(char *, data), len << 1);
        w->vad_output += len;
        if (w->speeching) {
            qtk_decoder_wrapper_feed(w->decoder, cast(char *, data), len << 1,
                                     0);
            if (w->waked == 0) {
                wtk_kwake_feed(w->wakeup, data, len, 0);
                if (w->wakeup->waked) {
                    wtk_kwake_get_wake_time(w->wakeup, &wake_fs, &wake_fe);
                    wake_fe += w->cfg->wake_fe_adjust;
                    w->waked = 1;
                    wres.t = QTK_WASR_WAKEUP;
                    wres.fs = wake_fs + w->wake_start / sample_rate;
                    wres.fe = wake_fe + w->wake_start / sample_rate;
                    w->notify(w->upval, &wres);
                    w->rec_fs = w->vad_output / sample_rate;
                    qtk_decoder_wrapper_feed(w->decoder, NULL, 0, 1);
                    qtk_decoder_wrapper_reset(w->decoder);
                    qtk_decoder_wrapper_start(w->decoder);
                    wake_end_pos = wake_fe * sample_rate + w->wake_start;
                    if (w->vad_output > wake_end_pos) {
                        pad_len = (w->vad_output << 1) - (wake_end_pos << 1);
                        pad_len_bak = pad_len;
                        skip_len = wtk_buf_len(w->cache) - pad_len;
                        wtk_buf_read(w->cache, max(skip_len, 0), &pad_len,
                                     _buf_read, w);
                        w->rec_fs -= (pad_len_bak - pad_len) /
                                     (sizeof(short) * sample_rate);
                    }
                }
            }
        }
        break;
    case WTK_VAD2_END:
        w->rec_fe = w->vad_output / sample_rate;
        qtk_decoder_wrapper_feed(w->decoder, NULL, 0, 1);
        qtk_decoder_wrapper_get_result(w->decoder, &rec_res);
        wres.t = QTK_WASR_REC;
        wres.fs = w->rec_fs;
        wres.fe = w->rec_fe;
        wres.v.rec.res = rec_res.data;
        wres.v.rec.len = rec_res.len;
        w->notify(w->upval, &wres);
        qtk_decoder_wrapper_reset(w->decoder);
        if (w->waked == 0) {
            wtk_kwake_feed(w->wakeup, NULL, 0, 1);
            if (w->wakeup->waked) {
                wtk_kwake_get_wake_time(w->wakeup, &wake_fs, &wake_fe);
                wake_fe += w->cfg->wake_fe_adjust;
                w->waked = 1;
                wres.t = QTK_WASR_WAKEUP;
                wres.fs = wake_fs + w->wake_start / sample_rate;
                wres.fe = wake_fe + w->wake_start / sample_rate;
                w->notify(w->upval, &wres);
            }
        }
        wtk_kwake_reset(w->wakeup);
        w->speeching = 0;
        w->waked = 0;
        break;
    case WTK_VAD2_CANCEL:
        w->vad_output -= len;
        break;
    }
}

static void _on_stft(qtk_wasr_t *w, wtk_stft2_msg_t *msg, int pos, int is_end) {
    int i, len;
    short *output;

    for (i = 0; i < w->stft2->nbin; i++) {
        w->freq[i] = msg->fft[i][0];
    }

    wtk_qmmse_denoise(w->qmmse, w->freq);
    len = wtk_stft2_output_ifft(w->stft2, w->freq, w->denoise_output, w->pad,
                                pos, is_end);

    output = cast(short *, w->denoise_output);
    for (i = 0; i < len; i++) {
        output[i] = QTK_SSAT16f(w->denoise_output[i]);
    }

    wtk_vad2_feed(w->vad, cast(char *, output), len << 1, is_end);
    wtk_stft2_push_msg(w->stft2, msg);
}

static void _wasr_set_notify(qtk_wasr_t *w) {
    wtk_vad2_set_notify(w->vad, w, cast(wtk_vad2_notify_f, _on_vad2));
    if (w->stft2) {
        wtk_stft2_set_notify(w->stft2, w, cast(wtk_stft2_notify_f, _on_stft));
    }
}

qtk_wasr_t *qtk_wasr_new(qtk_wasr_cfg_t *cfg) {
    qtk_wasr_t *w;
    int audio_cache_sz;

    w = wtk_malloc(sizeof(qtk_wasr_t));
    w->cfg = cfg;
    w->decoder = qtk_decoder_wrapper_new(&cfg->decoder);
    w->vad = wtk_vad2_new(&cfg->vad);
    w->vad->use_vad_start = cfg->use_vad_start;
    w->wakeup = wtk_kwake_new(&cfg->wake);

    w->speeching = 0;
    w->waked = 0;
    w->vad_output = 0;
    w->wake_start = 0;

    audio_cache_sz =
        sizeof(short) * cfg->audio_cache_ms * cfg->audio_sample_rate / 1000;
    w->cache = wtk_buf_new(audio_cache_sz);
    w->cache->max_len = audio_cache_sz;
    if (cfg->use_qmmse) {
        w->qmmse = wtk_qmmse_new(&cfg->qmmse);
        w->stft2 = wtk_stft2_new(&cfg->stft2);
        w->denoise_output = wtk_malloc(sizeof(float) * cfg->stft2.win);
        w->freq = wtk_malloc(sizeof(wtk_complex_t) * w->stft2->nbin);
        w->pad = wtk_calloc(sizeof(float), cfg->stft2.win);
    } else {
        w->qmmse = NULL;
        w->stft2 = NULL;
        w->denoise_output = NULL;
        w->pad = NULL;
        w->freq = NULL;
    }

    _wasr_set_notify(w);

    return w;
}

void qtk_wasr_delete(qtk_wasr_t *w) {
    qtk_decoder_wrapper_delete(w->decoder);
    wtk_vad2_delete(w->vad);
    wtk_kwake_delete(w->wakeup);
    wtk_buf_delete(w->cache);
    if (w->cfg->use_qmmse) {
        wtk_qmmse_delete(w->qmmse);
    }
    if (w->stft2) {
        wtk_stft2_delete(w->stft2);
    }
    if (w->denoise_output) {
        wtk_free(w->denoise_output);
    }
    if (w->pad) {
        wtk_free(w->pad);
    }
    if (w->freq) {
        wtk_free(w->freq);
    }
    wtk_free(w);
}

int qtk_wasr_feed(qtk_wasr_t *w, short *d, int len) {
    if (w->cfg->use_qmmse) {
        wtk_stft2_feed2(w->stft2, &d, len, 0);
    } else {
        wtk_vad2_feed(w->vad, cast(char *, d), len << 1, 0);
    }
    return 0;
}

int qtk_wasr_feed_end(qtk_wasr_t *w) {
    if (w->cfg->use_qmmse) {
        wtk_stft2_feed2(w->stft2, NULL, 0, 1);
    } else {
        wtk_vad2_feed(w->vad, NULL, 0, 1);
    }
    return 0;
}

void qtk_wasr_set_notify(qtk_wasr_t *w, qtk_wasr_notify_t notify, void *upval) {
    w->notify = notify;
    w->upval = upval;
}
