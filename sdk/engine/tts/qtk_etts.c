#include "qtk_etts.h"
#include "sdk/engine/comm/qtk_engine_hdr.h"

#ifdef USE_TIME_TEST
static double tm1;
static double tm2;
static int flag = 0;
#endif

void qtk_etts_on_tts(qtk_etts_t *e, char *data, int bytes) {
    qtk_var_t v;

#ifdef USE_TIME_TEST
    if (flag == 0) {
        tm2 = time_get_ms();
        wtk_log_log(e->session->log, "===>tts time delay:%f\n", tm2 - tm1);
        flag = 1;
    }
#endif
    if (e->notify) {
        v.type = QTK_TTS_DATA;
        v.v.str.data = data;
        v.v.str.len = bytes;
        e->notify(e->notify_ths, &v);
    }
}
static void qtk_etts_init(qtk_etts_t *e) {
    qtk_engine_param_init(&e->param);
    e->session = NULL;
    e->t = NULL;
    e->cfg = NULL;
    e->notify = NULL;
    e->notify_ths = NULL;
    e->thread = NULL;
}
static int qtk_etts_on_start(qtk_etts_t *e);
static int qtk_etts_on_txt(qtk_etts_t *e, char *txt, int bytes);
static int qtk_etts_on_end(qtk_etts_t *e);
static void qtk_etts_on_reset(qtk_etts_t *e);
static void qtk_etts_on_set_notify(qtk_etts_t *e, void *ths,
                                   qtk_engine_notify_f notify);
static void qtk_etts_on_set(qtk_etts_t *e, char *data, int bytes);

qtk_etts_t *qtk_etts_new(qtk_session_t *session, wtk_local_cfg_t *params) {
    qtk_etts_t *e;
    int ret;

    e = (qtk_etts_t *)wtk_calloc(1, sizeof(qtk_etts_t));
    qtk_etts_init(e);
    e->session = session;

    qtk_engine_param_set_session(&e->param, session);
    ret = qtk_engine_param_feed(&e->param, params);
    if (ret != 0) {
        wtk_log_warn0(e->session->log, "params als failed.");
        goto end;
    }

    if (e->param.use_bin) {
        e->cfg = qtk_tts_cfg_new_bin(e->param.cfg);
    } else {
        e->cfg = qtk_tts_cfg_new(e->param.cfg);
    }
    if (!e->cfg) {
        wtk_log_warn0(e->session->log, "cfg new failed.");
        _qtk_error(e->session, _QTK_CFG_NEW_FAILED);
        ret = -1;
        goto end;
    }

    /**
     * res
     * tts_pitch
     * tts_volume
     * tts_speed
     */
    qtk_tts_cfg_update_params(e->cfg, params);
    qtk_tts_cfg_update_option(e->cfg, &session->opt);

    e->t = qtk_tts_new(e->cfg, session);
    if (!e->t) {
        wtk_log_warn0(e->session->log, "etts new failed.");
        _qtk_error(e->session, _QTK_INSTANSE_NEW_FAILED);
        ret = -1;
        goto end;
    }
    qtk_tts_set_notify(e->t, e, (qtk_cldtts_notify_f)qtk_etts_on_tts);

#if 0
	if(e->param.tts_pitch > 0) {
		qtk_tts_set_pitch(e->t,e->param.tts_pitch);
	}

	if(e->param.tts_speed > 0) {
		qtk_tts_set_speed(e->t,e->param.tts_speed);
	}

	if(e->param.tts_volume > 0) {
		qtk_tts_set_volume(e->t,e->param.tts_volume);
	}
#endif
    if (e->param.use_thread) {
        e->callback = qtk_engine_callback_new();
        e->callback->start_f = (qtk_engine_thread_start_f)qtk_etts_on_start;
        e->callback->data_f = (qtk_engine_thread_data_f)qtk_etts_on_txt;
        e->callback->end_f = (qtk_engine_thread_end_f)qtk_etts_on_end;
        e->callback->reset_f = (qtk_engine_thread_reset_f)qtk_etts_on_reset;
        e->callback->set_notify_f =
            (qtk_engine_thread_set_notify_f)qtk_etts_on_set_notify;
        e->callback->set_f = (qtk_engine_thread_set_f)qtk_etts_on_set;
        e->callback->ths = e;

        e->thread = qtk_engine_thread_new(e->callback, e->session->log, "etts",
                                          256, 10, 0, e->param.syn);
    }
    ret = 0;
end:
    wtk_log_log(e->session->log, "ret = %d", ret);
    if (ret != 0) {
        qtk_etts_delete(e);
        e = NULL;
    }
    return e;
}

int qtk_etts_delete(qtk_etts_t *e) {
    if (e->thread) {
        qtk_engine_thread_delete(e->thread, 0);
    }
    if (e->callback) {
        qtk_engine_callback_delete(e->callback);
    }

    if (e->t) {
        qtk_tts_delete(e->t);
    }
    if (e->cfg) {
        if (e->param.use_bin) {
            qtk_tts_cfg_delete_bin(e->cfg);
        } else {
            qtk_tts_cfg_delete(e->cfg);
        }
    }

    qtk_engine_param_clean(&e->param);
    wtk_free(e);
    return 0;
}

int qtk_etts_start(qtk_etts_t *e) {
    int ret = 0;

    if (e->param.use_thread) {
        qtk_engine_thread_start(e->thread);
    } else {
        ret = qtk_etts_on_start(e);
    }
    return ret;
}

int qtk_etts_feed(qtk_etts_t *e, char *data, int bytes, int is_end) {
    int ret = 0;
    if (e->param.use_thread) {
        qtk_engine_thread_feed(e->thread, data, bytes, is_end);
    } else {
        if (bytes > 0) {
            ret = qtk_etts_on_txt(e, data, bytes);
            if (ret != 0) {
                wtk_log_log0(e->session->log, "tts feed txt failed.");
                goto end;
            }
        }
        if (is_end) {
            qtk_etts_on_end(e);
        }
    }
end:
    return ret;
}

int qtk_etts_reset(qtk_etts_t *e) {
    if (e->param.use_thread) {
        qtk_engine_thread_reset(e->thread);
    } else {
        qtk_etts_on_reset(e);
    }
#ifdef USE_TIME_TEST
    flag = 0;
#endif
    return 0;
}

int qtk_etts_cancel(qtk_etts_t *e) {
    qtk_tts_set_stop_hint(e->t);
    if (e->param.use_thread) {
        qtk_engine_thread_cancel(e->thread);
    }

    return 0;
}

void qtk_etts_set_notify(qtk_etts_t *e, void *notify_ths,
                         qtk_engine_notify_f notify) {
    if (e->param.use_thread) {
        qtk_engine_thread_set_notify(e->thread, notify_ths, notify);
    } else {
        qtk_etts_on_set_notify(e, notify_ths, notify);
    }
}

int qtk_etts_set(qtk_etts_t *e, char *data, int bytes) {
    if (e->param.use_thread) {
        qtk_engine_thread_set(e->thread, data, bytes);
    } else {
        qtk_etts_on_set(e, data, bytes);
    }
    return 0;
}

static int qtk_etts_on_start(qtk_etts_t *e) {
    qtk_var_t v;

    if (e->notify) {
        v.type = QTK_TTS_START;
        e->notify(e->notify_ths, &v);
    }
    e->ended = 0;
    return qtk_tts_start(e->t);
}

static int qtk_etts_on_txt(qtk_etts_t *e, char *txt, int bytes) {
#ifdef USE_TIME_TEST
    tm1 = time_get_ms();
#endif
    wtk_log_log(e->session->log, "tts text = %.*s", bytes, txt);
    return qtk_tts_feed(e->t, txt, bytes);
}

static int qtk_etts_on_end(qtk_etts_t *e) {
    qtk_var_t v;

    if (e->notify) {
        v.type = QTK_TTS_END;
        e->notify(e->notify_ths, &v);
    }
    e->ended = 1;
    return 0;
}

static void qtk_etts_on_reset(qtk_etts_t *e) {
    if (!e->ended) {
        qtk_etts_on_end(e);
    }
    qtk_tts_reset(e->t);
}

static void qtk_etts_on_set_notify(qtk_etts_t *e, void *ths,
                                   qtk_engine_notify_f notify) {
    e->notify_ths = ths;
    e->notify = notify;
}

static void qtk_etts_on_set(qtk_etts_t *e, char *data, int bytes) {
    wtk_cfg_file_t *cfile = NULL;
    wtk_cfg_item_t *item;
    wtk_queue_node_t *qn;
    int ret;

    wtk_log_log(e->session->log, "set param = %.*s\n", bytes, data);
    cfile = wtk_cfg_file_new();
    if (!cfile) {
        return;
    }

    ret = wtk_cfg_file_feed(cfile, data, bytes);
    if (ret != 0) {
        goto end;
    }

    for (qn = cfile->main->cfg->queue.pop; qn; qn = qn->next) {
        item = data_offset2(qn, wtk_cfg_item_t, n);
        if (wtk_string_cmp2(item->key, &qtk_engine_set_str[5]) == 0) {
            qtk_tts_set_speed(e->t, atof(item->value.str->data));
        } else if (wtk_string_cmp2(item->key, &qtk_engine_set_str[6]) == 0) {
            qtk_tts_set_volume(e->t, atof(item->value.str->data));
        } else if (wtk_string_cmp2(item->key, &qtk_engine_set_str[7]) == 0) {
            qtk_tts_set_pitch(e->t, atof(item->value.str->data));
        } else if (wtk_string_cmp2(item->key, &qtk_engine_set_str[11]) == 0) {
            qtk_tts_set_coreType(e->t, item->value.str->data,
                                 item->value.str->len);
        } else if (wtk_string_cmp2(item->key, &qtk_engine_set_str[12]) == 0) {
            qtk_tts_set_res(e->t, item->value.str->data, item->value.str->len);
        } else if (wtk_string_cmp2(item->key, &qtk_engine_set_str[13]) == 0) {
            qtk_tts_set_useStream(e->t, atoi(item->value.str->data));
        }
    }
end:
    if (cfile) {
        wtk_cfg_file_delete(cfile);
    }
}
