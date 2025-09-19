#include "qtk_opusdec.h"
// static void int_to_char(opus_uint32 i, unsigned char ch[4]);
static opus_uint32 char_to_int(unsigned char ch[4]);

qtk_opusdec_t *qtk_opusdec_new(char *params) {
    qtk_opusdec_t *opusdec;
    wtk_cfg_file_t *cfile = NULL;
    wtk_cfg_item_t *item;
    wtk_queue_node_t *qn;
    int ret;
    int err;

    opusdec = (qtk_opusdec_t *)wtk_calloc(1, sizeof(qtk_opusdec_t));
    cfile = wtk_cfg_file_new();
    ret = wtk_cfg_file_feed(cfile, params, strlen(params));
    if (ret != 0) {
        goto end;
    }
    for (qn = cfile->main->cfg->queue.pop; qn; qn = qn->next) {
        item = data_offset2(qn, wtk_cfg_item_t, n);
        if (wtk_string_cmp_s(item->key, "samplerate") == 0) {
            opusdec->samplerate =
                wtk_str_atoi(item->value.str->data, item->value.str->len);
        }
        if (wtk_string_cmp_s(item->key, "channels") == 0) {
            opusdec->channels =
                wtk_str_atoi(item->value.str->data, item->value.str->len);
        }
        if (wtk_string_cmp_s(item->key, "debug") == 0) {
            opusdec->debug =
                wtk_str_atoi(item->value.str->data, item->value.str->len);
        }
    }
    if (opusdec->samplerate == 0 || opusdec->channels == 0) {
        wtk_debug("samplerate or channels not set\n");
        ret = -1;
        goto end;
    }
    opusdec->dec =
        opus_decoder_create(opusdec->samplerate, opusdec->channels, &err);
    if (!opusdec->dec) {
        wtk_debug("opus decoder create failed\n");
        ret = -1;
        goto end;
    }
    opusdec->buf = wtk_strbuf_new(512, 0);
    opusdec->max_frame_size = 48000 * 2;
    opusdec->out = (short *)wtk_malloc(opusdec->max_frame_size *
                                       opusdec->channels * sizeof(short));
    opusdec->fbytes = (unsigned char *)wtk_malloc(
        opusdec->max_frame_size * opusdec->channels * sizeof(short));
    opusdec->skip = 0;
    if (opusdec->debug) {
        opusdec->wav = wtk_wavfile_new(16000);
        opusdec->wav->max_pend = 0;
    }
    ret = 0;
end:
    if (ret != 0) {
        qtk_opusdec_delete(opusdec);
        opusdec = NULL;
    }
    if (cfile) {
        wtk_cfg_file_delete(cfile);
    }
    return opusdec;
}
int qtk_opusdec_start(qtk_opusdec_t *opusdec, void *ths,
                      qtk_opusdec_notify_f notify) {
    char tmp[64];

    opusdec->ths = ths;
    opusdec->notify = notify;
    opusdec->data_type = QTK_OPUSDEC_DATA_LEN;
    if (opusdec->debug) {
        opusdec->count++;
#ifdef __ANDROID__
        snprintf(tmp, 64, "/sdcard/qvoice/opus/%d.opus", opusdec->count);
        opusdec->f = fopen(tmp, "wa+");
        snprintf(tmp, 64, "/sdcard/qvoice/opus/%d.wav", opusdec->count);
        wtk_wavfile_open(opusdec->wav, tmp);
#else
        snprintf(tmp, 64, "./qvoice/opus/%d.opus", opusdec->count);
        opusdec->f = fopen(tmp, "wa+");
        snprintf(tmp, 64, "./qvoice/opus/%d.wav", opusdec->count);
        wtk_wavfile_open(opusdec->wav, tmp);
#endif
    }
    wtk_strbuf_reset(opusdec->buf);
    return 0;
}
int qtk_opusdec_stop(qtk_opusdec_t *opusdec) {
    if (opusdec->debug) {
        fclose(opusdec->f);
        wtk_wavfile_close(opusdec->wav);
    }
    return 0;
}
int qtk_opusdec_delete(qtk_opusdec_t *opusdec) {
    if (opusdec->dec) {
        opus_decoder_destroy(opusdec->dec);
    }
    if (opusdec->buf) {
        wtk_strbuf_delete(opusdec->buf);
    }
    if (opusdec->out) {
        wtk_free(opusdec->out);
    }
    if (opusdec->fbytes) {
        wtk_free(opusdec->fbytes);
    }
    if (opusdec->wav) {
        wtk_wavfile_delete(opusdec->wav);
    }
    wtk_free(opusdec);
    return 0;
}

int qtk_opusdec_feed(qtk_opusdec_t *opusdec, char *data, int len, int is_end) {
    int consume_len = 0;

    if (opusdec->f > 0) {
        fwrite(data, 1, len, opusdec->f);
    }
    // wtk_debug("===============> feed data len = %d\n",len);
    while (len > 0) {
        // wtk_debug(">>>>>>len = %d\n", len);
        // wtk_debug(">>>>>>>>>>>>>>>type = %d\n", opusdec->data_type);
        switch (opusdec->data_type) {
        case QTK_OPUSDEC_DATA_LEN:
            consume_len = min(4 - opusdec->buf->pos, len);
            wtk_strbuf_push(opusdec->buf, data, consume_len);
            len -= consume_len;
            data += consume_len;
            if (opusdec->buf->pos == 4) {
                opusdec->audio_len =
                    char_to_int((unsigned char *)opusdec->buf->data);
                // wtk_debug("audio_len = %d\n", opusdec->audio_len);
                opusdec->data_type = QTK_OPUSDEC_DATA_OR;
                wtk_strbuf_reset(opusdec->buf);
            }
            break;
        case QTK_OPUSDEC_DATA_OR:
            consume_len = min(4 - opusdec->buf->pos, len);
            wtk_strbuf_push(opusdec->buf, data, consume_len);
            len -= consume_len;
            data += consume_len;
            if (opusdec->buf->pos == 4) {
                opusdec->data_type = QTK_OPUSDEC_DATA_DATA;
                wtk_strbuf_reset(opusdec->buf);
            }
        case QTK_OPUSDEC_DATA_DATA:
            consume_len = min(opusdec->audio_len - opusdec->buf->pos, len);
            wtk_strbuf_push(opusdec->buf, data, consume_len);
            len -= consume_len;
            data += consume_len;
            // wtk_debug("buf->pos = %d\n", opusdec->buf->pos);
            if (opusdec->buf->pos == opusdec->audio_len) {
                // qtk_module_feed_data_parser(m, m->cache_buf->data,
                // m->cache_buf->pos);
                opusdec->output_samples = opusdec->max_frame_size;
                opusdec->output_samples = opus_decode(
                    opusdec->dec, (unsigned char *)opusdec->buf->data,
                    opusdec->buf->pos, opusdec->out, opusdec->output_samples,
                    0);
                // wtk_debug(">>>>>>>output_samples = %d\n",
                // opusdec->output_samples);
                if (opusdec->output_samples > 0) {
                    if (opusdec->output_samples > opusdec->skip) {
                        int i;
                        for (i = 0;
                             i < (opusdec->output_samples - opusdec->skip) *
                                     opusdec->channels;
                             i++) {
                            short s;
                            s = opusdec->out[i + (opusdec->skip *
                                                  opusdec->channels)];
                            opusdec->fbytes[2 * i] = s & 0xFF;
                            opusdec->fbytes[2 * i + 1] = (s >> 8) & 0xFF;
                        }
                        if (opusdec->notify) {
                            opusdec->notify(opusdec->ths,
                                            (char *)opusdec->fbytes,
                                            sizeof(short) * opusdec->channels *
                                                opusdec->output_samples);
                        }
                        if (opusdec->wav) {
                            wtk_wavfile_write(
                                opusdec->wav, (char *)opusdec->fbytes,
                                sizeof(short) * opusdec->channels *
                                    opusdec->output_samples);
                        }
                    }
                }
                opusdec->data_type = QTK_OPUSDEC_DATA_LEN;
                wtk_strbuf_reset(opusdec->buf);
            }
        }
    }
    return 0;
    return 0;
}

// static void int_to_char(opus_uint32 i, unsigned char ch[4])
// {
//     ch[0] = i>>24;
//     ch[1] = (i>>16)&0xFF;
//     ch[2] = (i>>8)&0xFF;
//     ch[3] = i&0xFF;
// }

static opus_uint32 char_to_int(unsigned char ch[4]) {
    return ((opus_uint32)ch[0] << 24) | ((opus_uint32)ch[1] << 16) |
           ((opus_uint32)ch[2] << 8) | (opus_uint32)ch[3];
}
