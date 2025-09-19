#include "qtk_ebfio.h"
#include "sdk/engine/comm/qtk_engine_hdr.h"
#include "sdk/codec/qtk_audio_conversion.h"

void qtk_ebfio_init(qtk_ebfio_t *e) {
    qtk_engine_param_init(&e->param);

    e->session = NULL;

    e->cfg = NULL;
    e->b = NULL;

    e->notify = NULL;
    e->notify_ths = NULL;

    e->thread = NULL;
    e->callback = NULL;
}

int qtk_ebfio_on_start(qtk_ebfio_t *e);
int qtk_ebfio_on_feed(qtk_ebfio_t *e, char *data, int len);
int qtk_ebfio_on_end(qtk_ebfio_t *e);
void qtk_ebfio_on_reset(qtk_ebfio_t *e);
void qtk_ebfio_on_set_notify(qtk_ebfio_t *e, void *notify_ths,
                             qtk_engine_notify_f notify_f);
int qtk_ebfio_on_set(qtk_ebfio_t *e, char *data, int bytes);
void qtk_ebfio_on_bfio(qtk_ebfio_t *bfio, wtk_bfio_cmd_t cmd, short *data,
                       int len);

qtk_ebfio_t *qtk_ebfio_new(qtk_session_t *session, wtk_local_cfg_t *params) {
    qtk_ebfio_t *e;
    int buf_size;
    int ret;
    int i;

    e = (qtk_ebfio_t *)wtk_malloc(sizeof(qtk_ebfio_t));
    qtk_ebfio_init(e);
    e->session = session;

    qtk_engine_param_set_session(&e->param, e->session);
    ret = qtk_engine_param_feed(&e->param, params);
    if (ret != 0) {
        wtk_log_warn0(e->session->log, "params als failed.");
        goto end;
    }

    if (e->param.use_bin) {
        e->cfg = wtk_bfio_cfg_new_bin(e->param.cfg);
    } else {
        e->cfg = wtk_bfio_cfg_new(e->param.cfg);
    }
    if (!e->cfg) {
        wtk_log_warn0(e->session->log, "cfg new failed.");
        _qtk_error(e->session, _QTK_CFG_NEW_FAILED);
        ret = -1;
        goto end;
    }
    e->b = wtk_bfio_new(e->cfg);
    if (!e->b) {
        wtk_log_err0(e->session->log, "bfio new failed.");
        ret = -1;
        goto end;
    }
    wtk_bfio_set_notify(e->b, e, (wtk_bfio_notify_f)qtk_ebfio_on_bfio);
    e->channel = e->b->channel + e->b->sp_channel;
    wtk_debug(">>>>channel = %d\n", e->channel);
    e->buffer = (short **)wtk_malloc(sizeof(short *) * e->channel);
    for (i = 0; i < e->channel; i++) {
        e->buffer[i] = (short *)wtk_malloc(sizeof(short *) *
                                           QTK_BFIO_FEED_STEP * e->channel);
    }
    if (e->param.use_thread) {
        buf_size = (e->channel) * 2 * 16 * e->param.winStep;

        e->callback = qtk_engine_callback_new();
        e->callback->start_f = (qtk_engine_thread_start_f)qtk_ebfio_on_start;
        e->callback->data_f = (qtk_engine_thread_data_f)qtk_ebfio_on_feed;
        e->callback->end_f = (qtk_engine_thread_end_f)qtk_ebfio_on_end;
        e->callback->reset_f = (qtk_engine_thread_reset_f)qtk_ebfio_on_reset;
        e->callback->set_notify_f =
            (qtk_engine_thread_set_notify_f)qtk_ebfio_on_set_notify;
        e->callback->set_f = (qtk_engine_thread_set_f)qtk_ebfio_on_set;
        e->callback->ths = e;

        e->thread = qtk_engine_thread_new(e->callback, e->session->log, "ebfio",
                                          buf_size, 20, 1, e->param.syn);
    }

    wtk_debug("mic_shift=%f spk_shift=%f echo_shift=%f\n", e->param.mic_shift, e->param.spk_shift, e->param.echo_shift);
    ret = 0;
end:
    wtk_log_log(e->session->log, "ret = %d", ret);
    if (ret != 0) {
        qtk_ebfio_delete(e);
        e = NULL;
    }
    return e;
}

int qtk_ebfio_delete(qtk_ebfio_t *e) {
    if (e->thread) {
        qtk_engine_thread_delete(e->thread, 1);
    }
    if (e->callback) {
        qtk_engine_callback_delete(e->callback);
    }
    if (e->b) {
        wtk_bfio_delete(e->b);
        e->b = NULL;
    }
    if (e->cfg) {
        if (e->param.use_bin) {
            wtk_bfio_cfg_delete_bin(e->cfg);
        } else {
            wtk_bfio_cfg_delete(e->cfg);
        }
        e->cfg = NULL;
    }
    qtk_engine_param_clean(&e->param);

    int i;
    for (i = 0; i < e->channel; i++) {
        wtk_free(e->buffer[i]);
    }
    wtk_free(e->buffer);

    wtk_free(e);
    return 0;
}

int qtk_ebfio_on_start(qtk_ebfio_t *e) {
    wtk_bfio_start(e->b);
    return 0;
}

int qtk_ebfio_on_feed(qtk_ebfio_t *e, char *data, int bytes) {
    int i, j;
    short *pv = NULL;
    int len;

    if(e->param.mic_shift != 1.0 || e->param.spk_shift != 1.0)
    {
        qtk_data_change_vol2(data, bytes, e->param.mic_shift, e->param.spk_shift, e->b->channel, e->b->sp_channel);
    }
    if (bytes > 0) {
        pv = (short *)data;
        len = bytes / (e->channel * sizeof(short));
        for (i = 0; i < len; ++i) {
            for (j = 0; j < e->channel; ++j) {
                e->buffer[j][i] = pv[i * e->channel + j];
            }
        }
        // wtk_debug(">>>>len %d\n", len);
        wtk_bfio_feed(e->b, e->buffer, len, 0);
    }

    return 0;
}

int qtk_ebfio_on_end(qtk_ebfio_t *e) {
    wtk_bfio_feed(e->b, NULL, 0, 1);
    e->feedend = 1;
    return 0;
}

void qtk_ebfio_on_reset(qtk_ebfio_t *e) {
    if (e->feedend == 0) {
        qtk_ebfio_on_end(e);
    }
    wtk_bfio_reset(e->b);
}

void qtk_ebfio_on_set_notify(qtk_ebfio_t *e, void *notify_ths,
                             qtk_engine_notify_f notify_f) {
    e->notify_ths = notify_ths;
    e->notify = notify_f;
}

int qtk_ebfio_on_set(qtk_ebfio_t *e, char *data, int bytes) {
    wtk_cfg_file_t *cfile = NULL;
	wtk_cfg_item_t *item;
	wtk_queue_node_t *qn;
	int ret;

	wtk_log_log(e->session->log,"set param = %.*s\n",bytes,data);
	cfile = wtk_cfg_file_new();
	if(!cfile) {
		return -1;
	}

	ret = wtk_cfg_file_feed(cfile,data,bytes);
	if(ret != 0) {
		goto end;
	}

	for(qn=cfile->main->cfg->queue.pop;qn;qn=qn->next) {
		item = data_offset2(qn,wtk_cfg_item_t,n);
		if(wtk_string_cmp2(item->key,&qtk_engine_set_str[8]) == 0) {
            printf("set wake words = %s\n", item->value.str->data);
            wtk_bfio_set_wake_words(e->b, item->value.str->data, item->value.str->len);
		}
	}
	
end:
	if(cfile) {
		wtk_cfg_file_delete(cfile);
	}
    return 0;
}

int qtk_ebfio_start(qtk_ebfio_t *e) {
    int ret;

    if (e->param.use_thread) {
        qtk_engine_thread_start(e->thread);
    } else {
        qtk_ebfio_on_start(e);
    }
    ret = 0;
    return ret;
}

int qtk_ebfio_feed(qtk_ebfio_t *e, char *data, int bytes, int is_end) {
    if (e->param.use_thread) {
        qtk_engine_thread_feed(e->thread, data, bytes, is_end);
    } else {
        if (bytes > 0) {
            qtk_ebfio_on_feed(e, data, bytes);
        }
        if (is_end) {
            qtk_ebfio_on_end(e);
        }
    }
    return 0;
}

int qtk_ebfio_reset(qtk_ebfio_t *e) {
    if (e->param.use_thread) {
        qtk_engine_thread_reset(e->thread);
    } else {
        qtk_ebfio_on_reset(e);
    }

    return 0;
}

int qtk_ebfio_cancel(qtk_ebfio_t *e) {
    if (e->param.use_thread) {
        qtk_engine_thread_cancel(e->thread);
    }
    return 0;
}

void qtk_ebfio_set_notify(qtk_ebfio_t *e, void *ths,
                          qtk_engine_notify_f notify_f) {
    if (e->param.use_thread) {
        qtk_engine_thread_set_notify(e->thread, ths, notify_f);
    } else {
        qtk_ebfio_on_set_notify(e, ths, notify_f);
    }
}

int qtk_ebfio_set(qtk_ebfio_t *e, char *data, int bytes) {
    int ret=0;

    if (e->param.use_thread) {
        qtk_engine_thread_set(e->thread, data, bytes);
    } else {
        ret = qtk_ebfio_on_set(e, data, bytes);
    }
    return ret;
}

void qtk_ebfio_on_bfio(qtk_ebfio_t *bfio, wtk_bfio_cmd_t cmd, short *data, int len) {
    qtk_var_t var;
    int b = 0;

    switch (cmd) {
    case WTK_BFIO_VAD_START:
        // wtk_debug("=================> vad start\n");
        var.type = QTK_SPEECH_START;
        b = 1;
        break;
    case WTK_BFIO_VAD_DATA:
        if(bfio->param.echo_shift != 1.0)
        {
            qtk_data_change_vol((char *)data, len << 1, bfio->param.echo_shift);
        }
        if (bfio->notify) {
			qtk_var_t var2;
			var2.type = QTK_AUDIO_RANGE_IDX;
			var2.v.i = 1;
			bfio->notify(bfio->notify_ths, &var2);
		}
        var.type = QTK_SPEECH_DATA_PCM;
        var.v.str.data = (char *)data;
        var.v.str.len = len << 1;
        b = 1;
        break;
    case WTK_BFIO_VAD_END:
        // wtk_debug("=================> vad end\n");
        var.type = QTK_SPEECH_END;
        b = 1;
        break;
    case WTK_BFIO_WAKE:
        // wtk_debug("=================> wake\n");
        var.type = QTK_AEC_WAKE;
        b = 1;
        break;
    case WTK_BFIO_VAD_CANCEL:
        // wtk_debug("=================> vad cancel\n");
        var.type = QTK_AEC_CANCEL_DATA;
        var.v.i = len << 1;
        b = 1;
        break;
    case WTK_BFIO_WAKE_SSL:
        var.type = QTK_AEC_DIRECTION;
        var.v.ii.theta = bfio->b->wake_theta;
        var.v.ii.phi = 0;
        b = 1;
        break;
    case WTK_BFIO_SPEECH_END:
        // wtk_debug("=================> WTK_BFIO_SPEECH_END\n");
        break;
    case WTK_BFIO_SIL_END:
        // wtk_debug("=================> WTK_BFIO_SIL_END\n");
        break;
    default:
        break;
    }
    if (b == 1 && bfio->notify) {
        bfio->notify(bfio->notify_ths, &var);
    }
}
