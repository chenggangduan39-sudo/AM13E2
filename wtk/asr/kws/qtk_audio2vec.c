#include "wtk/asr/kws/qtk_audio2vec.h"

qtk_audio2vec_t *qtk_audio2vec_new(qtk_audio2vec_cfg_t *cfg) {
    qtk_audio2vec_t *a;

    a = wtk_malloc(sizeof(*a));
    a->cfg = cfg;
    a->svprint = wtk_svprint_new(&cfg->svprint);
    a->vad = NULL;
    if (cfg->use_vad) {
        a->vad = wtk_kvad_new(&cfg->vad);
    }
    a->wav_buf = NULL;
    if (cfg->use_window_pad) {
        a->wav_buf = wtk_strbuf_new(1024, 1);
    }
    return a;
}

void qtk_audio2vec_start(qtk_audio2vec_t *a) {
    if (a->vad) {
        wtk_kvad_start(a->vad);
    }
    wtk_svprint_start(a->svprint);
}

void qtk_audio2vec_delete(qtk_audio2vec_t *a) {
    wtk_svprint_delete(a->svprint);
    if (a->vad) {
        wtk_kvad_delete(a->vad);
    }
    if (a->wav_buf) {
        wtk_strbuf_delete(a->wav_buf);
    }
    wtk_free(a);
}

static int qtk_audio2vec_process_vadq(qtk_audio2vec_t *a) {
    wtk_queue_t *q = &(a->vad->output_q);
    wtk_vframe_t *vf = NULL;
    wtk_queue_node_t *qn;
    int ret = 0;

    while (1) {
        qn = wtk_queue_pop(q);
        if (!qn) {
            break;
        }
        vf = data_offset2(qn, wtk_vframe_t, q_n);
        switch (vf->state) {
        case wtk_vframe_sil:
            break;
        case wtk_vframe_speech:
        case wtk_vframe_speech_end:
            if (!a->cfg->use_window_pad) {
                wtk_svprint_feed(a->svprint, vf->wav_data, vf->frame_step, 0);
            } else {
                wtk_strbuf_push(a->wav_buf, (char *)vf->wav_data,
                                vf->frame_step * sizeof(short));
            }
            break;
        }
        wtk_kvad_push_frame(a->vad, vf);
    }

    return ret;
}

static short *qtk_audio2vec_pad_data(short *data, int len) {
    short *to_data = NULL;
    short *s;
    int s3l = 16000 * 2; //最低3s
    int pad = s3l / len;
    int i, j = s3l % len;

    to_data = (short *)wtk_calloc(s3l, sizeof(short));
    s = to_data;
    for (i = 0; i < pad; ++i) {
        memcpy(s, data, len * sizeof(short));
        s = s + len;
    }
    memcpy(s, data, j * sizeof(short));

    return to_data;
}

int qtk_audio2vec_feed(qtk_audio2vec_t *a, short *audio, int len) {
    int ret = 0;

    if (a->vad) {
        wtk_kvad_feed(a->vad, audio, len, 0);
        ret = qtk_audio2vec_process_vadq(a);
    } else {
        ret = wtk_svprint_feed(a->svprint, audio, len, 0);
    }

    return ret;
}

int qtk_audio2vec_feed_end(qtk_audio2vec_t *a) {
    int ret = 0;

    if (a->vad) {
        wtk_kvad_feed(a->vad, NULL, 0, 1);
        ret = qtk_audio2vec_process_vadq(a);
        if (a->cfg->use_window_pad) {

            if (a->wav_buf->pos < 3200) {
                return -1;
            }

            if (a->wav_buf->pos > 32000) {
                // wtk_wavfile_open2(kws->log_wav,"vad");
                // wtk_wavfile_write(kws->log_wav,kws->wav_buf->data,kws->wav_buf->pos);
                // wtk_wavfile_close(kws->log_wav);
                wtk_svprint_feed(a->svprint, (short *)a->wav_buf->data,
                                 a->wav_buf->pos >> 1, 0);
                wtk_svprint_feed(a->svprint, NULL, 0, 1);
            } else {
                short *pad_wav;
                wtk_debug("%d\n", a->wav_buf->pos);
                pad_wav = qtk_audio2vec_pad_data((short *)a->wav_buf->data,
                                                 a->wav_buf->pos >> 1);
                // wtk_wavfile_open2(kws->log_wav,"vad");
                // wtk_wavfile_write(kws->log_wav,(char*)pad_wav,64000);
                // wtk_wavfile_close(kws->log_wav);
                wtk_svprint_feed(a->svprint, pad_wav, 32000, 0);
                wtk_svprint_feed(a->svprint, NULL, 0, 1);
                wtk_free(pad_wav);
            }
        }
    } else {
        ret = wtk_svprint_feed(a->svprint, NULL, 0, 1);
    }
    return ret;
}

void qtk_audio2vec_get_result(qtk_audio2vec_t *a, float **vec, int *len) {
    wtk_vecf_t *output;
    int cnt = 0;

    output = wtk_svprint_compute_feat2(a->svprint, &cnt);
    *vec = output->p;
    *len = output->len;
}

void qtk_audio2vec_reset(qtk_audio2vec_t *a) {
    wtk_svprint_reset(a->svprint);
    if (a->vad) {
        wtk_kvad_reset(a->vad);
    }
    if (a->wav_buf) {
        wtk_strbuf_reset(a->wav_buf);
    }
}

float qtk_audio2vec_eval(qtk_audio2vec_t *a, float *sv1, float *sv2, int len) {
    wtk_vecf_t v1, v2;

    v1.p = sv1;
    v1.len = len;
    v2.p = sv2;
    v2.len = len;

    return wtk_svprint_feat_compute_likelihood2(&v1, &v2);
}
