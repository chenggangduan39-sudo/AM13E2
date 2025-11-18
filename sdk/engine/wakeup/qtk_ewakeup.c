#include "qtk_ewakeup.h"

#include "sdk/engine/comm/qtk_engine_hdr.h"

#ifdef USE_TIME_TEST
static double tm1;
static double tm2;
static int flag = 0;
#endif

static int qtk_ewakeup_on_data(qtk_ewakeup_t *e, char *data, int bytes);
void qtk_ewakeup_on_wakeup(qtk_ewakeup_t *e, int res, int start, int end) {
    qtk_var_t v;

#ifdef USE_TIME_TEST
    if (flag == 0) {
        tm2 = time_get_ms();
        wtk_log_log(e->session->log, "===>wakeup time delay:%f\n", tm2 - tm1);
        flag = 1;
    }
#endif
    // wtk_debug("===>>>>>>>>>>>>>>>>..res=%d start=%d end=%d\n",res,start,end);
    if (e->notify) {
        v.type = QTK_AEC_WAKE;
        v.v.i = res;
        e->notify(e->notify_ths, &v);
    }
}
static void qtk_ewakeup_init(qtk_ewakeup_t *e) {
    qtk_engine_param_init(&e->param);
    e->session = NULL;
    e->t = NULL;
    e->cfg = NULL;
    e->notify = NULL;
    e->notify_ths = NULL;
    e->thread = NULL;
}
static int qtk_ewakeup_on_start(qtk_ewakeup_t *e);
//static int qtk_ewakeup_on_txt(qtk_ewakeup_t *e, char *txt, int bytes);
static int qtk_ewakeup_on_end(qtk_ewakeup_t *e);
static void qtk_ewakeup_on_reset(qtk_ewakeup_t *e);
static void qtk_ewakeup_on_set_notify(qtk_ewakeup_t *e, void *ths,
                                   qtk_engine_notify_f notify);
static void qtk_ewakeup_on_set(qtk_ewakeup_t *e, char *data, int bytes);

qtk_ewakeup_t *qtk_ewakeup_new(qtk_session_t *session, wtk_local_cfg_t *params) {
    qtk_ewakeup_t *e;
    int ret;

    e = (qtk_ewakeup_t *)wtk_calloc(1, sizeof(qtk_ewakeup_t));
    qtk_ewakeup_init(e);
    e->session = session;

    qtk_engine_param_set_session(&e->param, session);
    ret = qtk_engine_param_feed(&e->param, params);
    if (ret != 0) {
        wtk_log_warn0(e->session->log, "params als failed.");
        goto end;
    }

    if (e->param.use_bin) {
        e->cfg = qtk_wakeup_cfg_new_bin(e->param.cfg);
    } else {
        e->cfg = qtk_wakeup_cfg_new(e->param.cfg);
    }
    if (!e->cfg) {
        wtk_log_warn0(e->session->log, "cfg new failed.");
        _qtk_error(e->session, _QTK_CFG_NEW_FAILED);
        ret = -1;
        goto end;
    }

    e->t = qtk_wakeup_new(e->cfg, session);
    if (!e->t) {
        wtk_log_warn0(e->session->log, "ewakeup new failed.");
        _qtk_error(e->session, _QTK_INSTANSE_NEW_FAILED);
        ret = -1;
        goto end;
    }
    qtk_wakeup_set_notify(e->t, e, (qtk_wakeup_notify_f)qtk_ewakeup_on_wakeup);

    if (e->param.use_thread) {
        e->callback = qtk_engine_callback_new();
        e->callback->start_f = (qtk_engine_thread_start_f)qtk_ewakeup_on_start;
        e->callback->data_f = (qtk_engine_thread_data_f)qtk_ewakeup_on_data;
        e->callback->end_f = (qtk_engine_thread_end_f)qtk_ewakeup_on_end;
        e->callback->reset_f = (qtk_engine_thread_reset_f)qtk_ewakeup_on_reset;
        e->callback->set_notify_f =
            (qtk_engine_thread_set_notify_f)qtk_ewakeup_on_set_notify;
        e->callback->set_f = (qtk_engine_thread_set_f)qtk_ewakeup_on_set;
        e->callback->ths = e;

        e->thread = qtk_engine_thread_new(e->callback, e->session->log, "ewakeup",
                                          256, 10, 0, e->param.syn);
    }
    ret = 0;
end:
    wtk_log_log(e->session->log, "ret = %d", ret);
    if (ret != 0) {
        qtk_ewakeup_delete(e);
        e = NULL;
    }
    return e;
}

int qtk_ewakeup_delete(qtk_ewakeup_t *e) {
    if (e->thread) {
        qtk_engine_thread_delete(e->thread, 0);
    }
    if (e->callback) {
        qtk_engine_callback_delete(e->callback);
    }

    wtk_debug("=====================>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
    if (e->t) {
        qtk_wakeup_delete(e->t);
    }
    wtk_debug("=====================>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
    if (e->cfg) {
        if (e->param.use_bin) {
            qtk_wakeup_cfg_delete_bin(e->cfg);
        } else {
            qtk_wakeup_cfg_delete(e->cfg);
        }
    }
    wtk_debug("=====================>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");

    qtk_engine_param_clean(&e->param);
    wtk_free(e);
    return 0;
}

int qtk_ewakeup_start(qtk_ewakeup_t *e) {
    int ret = 0;

    if (e->param.use_thread) {
        qtk_engine_thread_start(e->thread);
    } else {
        ret = qtk_ewakeup_on_start(e);
    }
    return ret;
}

int qtk_ewakeup_feed(qtk_ewakeup_t *e, char *data, int bytes, int is_end) {
    int ret = 0;
    if (e->param.use_thread) {
        qtk_engine_thread_feed(e->thread, data, bytes, is_end);
    } else {
        if (bytes > 0) {
            ret = qtk_ewakeup_on_data(e, data, bytes);
            if (ret != 0) {
                wtk_log_log0(e->session->log, " feed data failed.");
                goto end;
            }
        }
        if (is_end) {
            qtk_ewakeup_on_end(e);
        }
    }
end:
    return ret;
}

int qtk_ewakeup_reset(qtk_ewakeup_t *e) {
    if (e->param.use_thread) {
        qtk_engine_thread_reset(e->thread);
    } else {
        qtk_ewakeup_on_reset(e);
    }
#ifdef USE_TIME_TEST
    flag = 0;
#endif
    return 0;
}

int qtk_ewakeup_cancel(qtk_ewakeup_t *e) {
    if (e->param.use_thread) {
        qtk_engine_thread_cancel(e->thread);
    }

    return 0;
}

void qtk_ewakeup_set_notify(qtk_ewakeup_t *e, void *notify_ths,
                         qtk_engine_notify_f notify) {
    if (e->param.use_thread) {
        qtk_engine_thread_set_notify(e->thread, notify_ths, notify);
    } else {
        qtk_ewakeup_on_set_notify(e, notify_ths, notify);
    }
}

int qtk_ewakeup_set(qtk_ewakeup_t *e, char *data, int bytes) {
    if (e->param.use_thread) {
        qtk_engine_thread_set(e->thread, data, bytes);
    } else {
        qtk_ewakeup_on_set(e, data, bytes);
    }
    return 0;
}

static int qtk_ewakeup_on_start(qtk_ewakeup_t *e) {
    e->ended = 0;
    return qtk_wakeup_start(e->t);
}

static int qtk_ewakeup_on_data(qtk_ewakeup_t *e, char *data, int bytes) {
#ifdef USE_TIME_TEST
    tm1 = time_get_ms();
#endif
    return qtk_wakeup_feed(e->t, data, bytes, 0);
}

static int qtk_ewakeup_on_end(qtk_ewakeup_t *e) {
    e->ended = 1;
    qtk_wakeup_feed(e->t, 0, 0, 1);
    return 0;
}

static void qtk_ewakeup_on_reset(qtk_ewakeup_t *e) {
    if (!e->ended) {
        qtk_ewakeup_on_end(e);
    }
    qtk_wakeup_reset(e->t);
}

static void qtk_ewakeup_on_set_notify(qtk_ewakeup_t *e, void *ths,
                                   qtk_engine_notify_f notify) {
    e->notify_ths = ths;
    e->notify = notify;
}

static void qtk_ewakeup_on_set(qtk_ewakeup_t *e, char *data, int bytes) {
    wtk_cfg_file_t *cfile = NULL;
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

end:
    if (cfile) {
        wtk_cfg_file_delete(cfile);
    }
}
