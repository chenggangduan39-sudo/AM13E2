#include "qtk_ewdec.h"
#include "sdk/engine/comm/qtk_engine_hdr.h"

#ifdef USE_TIME_TEST
static double tm1;
static double tm2;
static int flag = 0;
#endif

static int qtk_ewdec_on_data(qtk_ewdec_t *e, char *data, int bytes);
void qtk_ewdec_on_wake(qtk_ewdec_t *e, qtk_var_t *var) {

#ifdef USE_TIME_TEST
    if (flag == 0) {
        tm2 = time_get_ms();
        wtk_log_log(e->session->log, "===>wdec time delay:%f\n", tm2 - tm1);
        flag = 1;
    }
#endif
    if (e->notify) {
        e->notify(e->notify_ths, var);
    }
}
static void qtk_ewdec_init(qtk_ewdec_t *e) {
    qtk_engine_param_init(&e->param);
    e->session = NULL;
    e->wdec = NULL;
    e->cfg = NULL;
    e->notify = NULL;
    e->notify_ths = NULL;
    e->thread = NULL;
    e->callback = NULL;
}
static int qtk_ewdec_on_start(qtk_ewdec_t *e);
//static int qtk_ewdec_on_txt(qtk_ewdec_t *e, char *txt, int bytes);
static int qtk_ewdec_on_end(qtk_ewdec_t *e);
static void qtk_ewdec_on_reset(qtk_ewdec_t *e);
static void qtk_ewdec_on_set_notify(qtk_ewdec_t *e, void *ths,
                                   qtk_engine_notify_f notify);
static void qtk_ewdec_on_set(qtk_ewdec_t *e, char *data, int bytes);

qtk_ewdec_t *qtk_ewdec_new(qtk_session_t *session, wtk_local_cfg_t *params) {
    qtk_ewdec_t *e;
    int ret;

    e = (qtk_ewdec_t *)wtk_malloc(sizeof(qtk_ewdec_t));
    qtk_ewdec_init(e);
    e->session = session;

    qtk_engine_param_set_session(&e->param, session);
    ret = qtk_engine_param_feed(&e->param, params);
    if (ret != 0) {
        wtk_log_warn0(e->session->log, "params als failed.");
        goto end;
    }

    if (e->param.use_bin) {
        e->cfg = qtk_wdec_cfg_new_bin(e->param.cfg);
    } else {
        e->cfg = qtk_wdec_cfg_new(e->param.cfg);
    }
    if (!e->cfg) {
        wtk_log_warn0(e->session->log, "cfg new failed.");
        _qtk_error(e->session, _QTK_CFG_NEW_FAILED);
        ret = -1;
        goto end;
    }

    e->wdec = qtk_wdec_new(e->cfg, session);
    if (!e->wdec) {
        wtk_log_warn0(e->session->log, "ewdec new failed.");
        _qtk_error(e->session, _QTK_INSTANSE_NEW_FAILED);
        ret = -1;
        goto end;
    }
    qtk_wdec_set_notify(e->wdec, e, (qtk_engine_notify_f)qtk_ewdec_on_wake);

    if (e->param.use_thread) {
        e->callback = qtk_engine_callback_new();
        e->callback->start_f = (qtk_engine_thread_start_f)qtk_ewdec_on_start;
        e->callback->data_f = (qtk_engine_thread_data_f)qtk_ewdec_on_data;
        e->callback->end_f = (qtk_engine_thread_end_f)qtk_ewdec_on_end;
        e->callback->reset_f = (qtk_engine_thread_reset_f)qtk_ewdec_on_reset;
        e->callback->set_notify_f =
            (qtk_engine_thread_set_notify_f)qtk_ewdec_on_set_notify;
        e->callback->set_f = (qtk_engine_thread_set_f)qtk_ewdec_on_set;
        e->callback->ths = e;

        e->thread = qtk_engine_thread_new(e->callback, e->session->log, "ewdec",
                                          256, 10, 0, e->param.syn);
    }
    ret = 0;
end:
    wtk_log_log(e->session->log, "ret = %d", ret);
    if (ret != 0) {
        qtk_ewdec_delete(e);
        e = NULL;
    }
    return e;
}

int qtk_ewdec_delete(qtk_ewdec_t *e) {
    if (e->thread) {
        qtk_engine_thread_delete(e->thread, 0);
    }
    if (e->callback) {
        qtk_engine_callback_delete(e->callback);
    }

    if (e->wdec) {
        qtk_wdec_delete(e->wdec);
    }
    if (e->cfg) {
        if (e->param.use_bin) {
            qtk_wdec_cfg_delete_bin(e->cfg);
        } else {
            qtk_wdec_cfg_delete(e->cfg);
        }
    }

    qtk_engine_param_clean(&e->param);
    wtk_free(e);
    return 0;
}

int qtk_ewdec_start(qtk_ewdec_t *e) {
    int ret = 0;

    if (e->param.use_thread) {
        qtk_engine_thread_start(e->thread);
    } else {
        ret = qtk_ewdec_on_start(e);
    }
    return ret;
}

int qtk_ewdec_feed(qtk_ewdec_t *e, char *data, int bytes, int is_end) {
    int ret = 0;
    if (e->param.use_thread) {
        qtk_engine_thread_feed(e->thread, data, bytes, is_end);
    } else {
        if (bytes > 0) {
            ret = qtk_ewdec_on_data(e, data, bytes);
            if (ret != 0) {
                wtk_log_log0(e->session->log, " feed data failed.");
                goto end;
            }
        }
        if (is_end) {
            qtk_ewdec_on_end(e);
        }
    }
end:
    return ret;
}

int qtk_ewdec_reset(qtk_ewdec_t *e) {
    if (e->param.use_thread) {
        qtk_engine_thread_reset(e->thread);
    } else {
        qtk_ewdec_on_reset(e);
    }
#ifdef USE_TIME_TEST
    flag = 0;
#endif
    return 0;
}

int qtk_ewdec_cancel(qtk_ewdec_t *e) {
    if (e->param.use_thread) {
        qtk_engine_thread_cancel(e->thread);
    }

    return 0;
}

void qtk_ewdec_set_notify(qtk_ewdec_t *e, void *notify_ths,
                         qtk_engine_notify_f notify) {
    if (e->param.use_thread) {
        qtk_engine_thread_set_notify(e->thread, notify_ths, notify);
    } else {
        qtk_ewdec_on_set_notify(e, notify_ths, notify);
    }
}

int qtk_ewdec_set(qtk_ewdec_t *e, char *data, int bytes) {
    if (e->param.use_thread) {
        qtk_engine_thread_set(e->thread, data, bytes);
    } else {
        qtk_ewdec_on_set(e, data, bytes);
    }
    return 0;
}

static int qtk_ewdec_on_start(qtk_ewdec_t *e) {
//    qtk_var_t v;
//
//    if (e->notify) {
//        v.type = QTK_TTS_START;
//        e->notify(e->notify_ths, &v);
//    }
    e->ended = 0;
    return qtk_wdec_start(e->wdec);
}

static int qtk_ewdec_on_data(qtk_ewdec_t *e, char *data, int bytes) {
#ifdef USE_TIME_TEST
    tm1 = time_get_ms();
#endif
    return qtk_wdec_feed(e->wdec, data, bytes, 0);
}

static int qtk_ewdec_on_end(qtk_ewdec_t *e) {
//    qtk_var_t v;
//
//    if (e->notify) {
//        v.type = QTK_TTS_END;
//        e->notify(e->notify_ths, &v);
//    }
    qtk_wdec_feed(e->wdec, NULL, 0, 1);
    e->ended = 1;
    return 0;
}

static void qtk_ewdec_on_reset(qtk_ewdec_t *e) {
    if (!e->ended) {
        qtk_ewdec_on_end(e);
    }
    qtk_wdec_reset(e->wdec);
}

static void qtk_ewdec_on_set_notify(qtk_ewdec_t *e, void *ths,
                                   qtk_engine_notify_f notify) {
    e->notify_ths = ths;
    e->notify = notify;
}

static void qtk_ewdec_on_set(qtk_ewdec_t *e, char *data, int bytes) {

	// wtk_debug("%.*s\n", bytes, data);
    // qtk_wdec_set_wake_words(e->wdec,data,bytes);
    // wtk_log_log(e->session->log, "set param = %.*s\n", bytes, data);
    wtk_cfg_file_t *cfile = NULL;
	wtk_cfg_item_t *item;
	wtk_queue_node_t *qn;
	int ret;

	wtk_log_log(e->session->log,"set param = %.*s\n",bytes,data);
	cfile = wtk_cfg_file_new();
	if(!cfile) {
		return;
	}

	ret = wtk_cfg_file_feed(cfile,data,bytes);
	if(ret != 0) {
		goto end;
	}

	for(qn=cfile->main->cfg->queue.pop;qn;qn=qn->next) {
		item = data_offset2(qn,wtk_cfg_item_t,n);
		if(wtk_string_cmp2(item->key,&qtk_engine_set_str[8]) == 0) {
            qtk_wdec_set_wake_words(e->wdec,item->value.str->data,item->value.str->len);
		}
	}
	
end:
	if(cfile) {
		wtk_cfg_file_delete(cfile);
	}
}
