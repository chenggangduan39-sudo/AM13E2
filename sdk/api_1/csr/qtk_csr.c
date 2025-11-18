#include "qtk_csr.h"

void qtk_csr_on_hint_notify(qtk_csr_t *c, char *data, int bytes) {
    qtk_var_t var;

    if (c->notify) {
        var.type = QTK_ASR_HINT;
        var.v.str.data = data;
        var.v.str.len = bytes;
        c->notify(c->notify_ths, &var);
    }
}

void qtk_csr_on_spxfinal_notify(qtk_csr_t *c, char *data, int bytes) {
    qtk_var_t var;

    if (c->notify) {
        var.type = QTK_ASR_TEXT;
        var.v.str.data = data;
        var.v.str.len = bytes;
        c->notify(c->notify_ths, &var);
    }
}

void qtk_csr_on_result_notify(qtk_csr_t *c, char *data, int bytes, float fs, float fe, int type)
{
    qtk_var_t var;
	if(c->notify)
	{
		if(type)
		{
			var.type = QTK_AEC_WAKE;
		}else{
			var.type = QTK_ASR_TEXT;
			var.v.str.data = data;
			var.v.str.len = bytes;
		}
		c->notify(c->notify_ths, &var);
	}
}

static void qtk_csr_init(qtk_csr_t *c) {
    c->cfg = NULL;
    c->session = NULL;

    c->asr = NULL;
    c->vad = NULL;

    c->notify = NULL;
    c->notify_ths = NULL;
}

qtk_csr_t *qtk_csr_new(qtk_csr_cfg_t *cfg, qtk_session_t *session) {
    qtk_csr_t *c;
    int ret;

    c = (qtk_csr_t *)wtk_malloc(sizeof(qtk_csr_t));
    qtk_csr_init(c);
    c->cfg = cfg;
    c->session = session;

    c->asr = qtk_asr_new(&cfg->asr, session);
    if (!c->asr) {
        wtk_log_log0(c->session->log, "asr new failed.");
        ret = -1;
        goto end;
    }

    wtk_queue_init(&c->vad_q);
    if (cfg->xvad) {
        c->vad = wtk_vad_new(cfg->xvad, &c->vad_q);
    } else {
        c->vad = wtk_vad_new(&cfg->vad, &c->vad_q);
    }
    qtk_asr_set_hint_notify(c->asr, c,
                            (qtk_iasr_set_hint_notify_f)qtk_csr_on_hint_notify);
    qtk_asr_set_spxfinal_notify(c->asr, c,
                            (qtk_iasr_set_spxfinal_notify_f)qtk_csr_on_spxfinal_notify);
    qtk_asr_set_result_notify(c->asr, c, (qtk_iasr_set_result_f)qtk_csr_on_result_notify);

    c->vad_count=1;
    c->start_time=0.0f;
    c->end_time = 0.0f;
    c->feed_bytes=0;
    ret = 0;
end:
    if (ret != 0) {
        qtk_csr_delete(c);
        c = NULL;
    }
    return c;
}

void qtk_csr_delete(qtk_csr_t *c) {
    if (c->vad) {
        wtk_vad_delete(c->vad);
    }
    if (c->asr) {
        qtk_asr_delete(c->asr);
    }
    wtk_free(c);
}

void qtk_csr_set_notify(qtk_csr_t *c, void *notify_ths,
                        qtk_csr_notify_f notify) {
    c->notify_ths = notify_ths;
    c->notify = notify;
}

void qtk_csr_set_idle_time(qtk_csr_t *c, int itime)
{
    qtk_asr_set_idle_time(c->asr, itime);
}

int qtk_csr_start(qtk_csr_t *c, int left, int right) {
    c->sil = 1;
    c->valid = 0;
    c->cancel = 0;
    c->vad_count=1;
    c->start_time=0.0f;
    c->end_time = 0.0f;
    c->feed_bytes=0;
    wtk_vad_set_margin(c->vad, left, right);
    return wtk_vad_start(c->vad);
}

void qtk_csr_reset(qtk_csr_t *c) {
    wtk_vad_reset(c->vad);
    qtk_asr_reset(c->asr);
}

void qtk_csr_cancel(qtk_csr_t *c) {
    c->cancel = 1;
    qtk_asr_cancel(c->asr);
}

int qtk_csr_feed(qtk_csr_t *c, char *data, int bytes, int is_end) {
    wtk_queue_t *vad_q = (c->vad->output_queue);
    wtk_vframe_t *f;
    wtk_queue_node_t *qn;
    wtk_json_parser_t *parser = NULL;
    wtk_json_item_t *result = NULL;
    wtk_strbuf_t *buf = NULL;
    wtk_string_t v;
    qtk_var_t var;
    int ret;
    double t1, t2;

    // wtk_debug("%s:%d==========================>\n",__FUNCTION__,__LINE__);
    parser = wtk_json_parser_new();
    buf = wtk_strbuf_new(256, 1);
    wtk_vad_feed(c->vad, data, bytes, is_end);
    while (1) {
        qn = wtk_queue_pop(vad_q);
        if (!qn) {
            break;
        }
        f = data_offset2(qn, wtk_vframe_t, q_n);

        c->feed_bytes+=(f->frame_step << 1)/32;
        if (c->cancel) {
            wtk_vad_push_vframe(c->vad, f);
            continue;
        }

        if (c->sil) {
            if (f->state == wtk_vframe_speech) {
                var.type = QTK_SPEECH_START;
                c->start_time=c->feed_bytes/1000.0;
                if (c->notify) {
                    var.v.fi.on=c->vad_count;
                    var.v.fi.theta=c->start_time;
                    c->notify(c->notify_ths, &var);
                }

                if (c->notify) {
                    var.type = QTK_SPEECH_DATA_PCM;
                    var.v.str.data=(char *)f->wav_data;
                    var.v.str.len=f->frame_step << 1;
                    c->notify(c->notify_ths, &var);
                }
#if 1
                ret = qtk_asr_start(c->asr);
                c->valid = ret == 0 ? 1 : 0;
                ret = c->valid ? qtk_asr_feed(c->asr, (char *)f->wav_data,
                                              f->frame_step << 1, 0)
                               : -1;
                c->valid = ret == 0 ? 1 : 0;
#endif
                c->sil = 0;
            }

        } else {
            if (f->state == wtk_vframe_sil) {
                c->end_time=c->feed_bytes/1000.0;
                // wtk_log_log(c->session->log,"vad第%d段音频: start_time==%fms end_time==%fms",vad_count,start_time,end_time);
                // wtk_debug("vad第%d段音频: start_time==%fms end_time==%fms\n",vad_count,start_time,end_time);
                c->vad_count++;

                var.type = QTK_SPEECH_END;
                if (c->notify) {
                    var.v.fi.on=c->vad_count;
                    var.v.fi.theta=c->end_time;
                    c->notify(c->notify_ths, &var);
                }
#if 1
                t1 = time_get_ms();
                ret = c->valid ? qtk_asr_feed(c->asr, 0, 0, 1) : -1;
                c->valid = ret == 0 ? 1 : 0;
                // wtk_debug("==================>>>>>>>>>>>>ret=%d valid=%d\n",ret,c->valid);

                if (c->valid) {
                    v = qtk_asr_get_result(c->asr);
                    // wtk_debug("asr result=%d:%s\n",v.len,v.data);
                    t2 = time_get_ms();
                    if (v.len > 0) {
                        wtk_json_parser_reset(parser);
                        wtk_json_parser_parse(parser, v.data, v.len);
                        result = parser->json->main;
                        wtk_json_obj_add_ref_number_s(parser->json, result,
                                                      "delay", (int)(t2 - t1));
                        wtk_json_item_print(result, buf);
                        var.type = QTK_ASR_TEXT;
                        var.v.str.data = buf->data;
                        var.v.str.len = buf->pos;
                        if (c->notify) {
                            c->notify(c->notify_ths, &var);
                        }
                    }
                }
                qtk_asr_reset(c->asr);
#endif
                c->sil = 1;
            } else {
                if (c->notify) {
                    var.type = QTK_SPEECH_DATA_PCM;
                    var.v.str.data=(char *)f->wav_data;
                    var.v.str.len=f->frame_step << 1;
                    c->notify(c->notify_ths, &var);
                }
#if 1
                ret = c->valid ? qtk_asr_feed(c->asr, (char *)f->wav_data,
                                              f->frame_step << 1, 0)
                               : -1;
                c->valid = ret == 0 ? 1 : 0;
#endif
            }
        }
        wtk_vad_push_vframe(c->vad, f);
    }

#if 1
    if (is_end && !c->sil && !c->cancel) {
        t1 = time_get_ms();
        ret = c->valid ? qtk_asr_feed(c->asr, 0, 0, 1) : -1;
        c->valid = ret == 0 ? 1 : 0;
        if (c->valid) {
            v = qtk_asr_get_result(c->asr);
            t2 = time_get_ms();
            if (v.len > 0) {
                wtk_json_parser_reset(parser);
                wtk_json_parser_parse(parser, v.data, v.len);
                result = parser->json->main;
                wtk_json_obj_add_ref_number_s(parser->json, result, "delay",
                                              (int)(t2 - t1));
                wtk_json_item_print(result, buf);
                var.type = QTK_ASR_TEXT;
                var.v.str.data = buf->data;
                var.v.str.len = buf->pos;
                if (c->notify) {
                    c->notify(c->notify_ths, &var);
                }
            }
        }
        qtk_asr_reset(c->asr);
    }
#endif

    if (buf) {
        wtk_strbuf_delete(buf);
    }
    if (parser) {
        wtk_json_parser_delete(parser);
    }

    return 0;
}
