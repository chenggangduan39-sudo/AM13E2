#include "qtk_eeval.h"
#include "sdk/engine/comm/qtk_engine_hdr.h"

void qtk_eeval_init(qtk_eeval_t *e) {
    qtk_engine_param_init(&e->param);

    e->session = NULL;

    e->cfg = NULL;
    e->eval = NULL;

    e->notify_f = NULL;
    e->notify_ths = NULL;

    e->thread = NULL;
    e->callback = NULL;

    e->evaltxt = NULL;

    e->cancel = 0;
}

int qtk_eeval_on_start(qtk_eeval_t *e);
int qtk_eeval_on_data(qtk_eeval_t *e, char *data, int bytes);
int qtk_eeval_on_end(qtk_eeval_t *e);
void qtk_eeval_on_reset(qtk_eeval_t *e);
void qtk_eeval_on_set_notify(qtk_eeval_t *e, void *notify_ths,
                             qtk_engine_notify_f notify_f);
void qtk_eeval_on_cancel(qtk_eeval_t *e);
void qtk_eeval_on_set(qtk_eeval_t *e, char *data, int bytes);
void qtk_eeval_on_err(qtk_eeval_t *e);

qtk_eeval_t *qtk_eeval_new(qtk_session_t *session, wtk_local_cfg_t *params) {
    qtk_eeval_t *e;
    int buf_size;
    int ret;

    e = (qtk_eeval_t *)wtk_malloc(sizeof(qtk_eeval_t));
    qtk_eeval_init(e);

    e->session = session;

    qtk_engine_param_set_session(&e->param, e->session);
    ret = qtk_engine_param_feed(&e->param, params);
    if (ret != 0) {
        wtk_log_warn0(e->session->log, "params als failed.");
        goto end;
    }

    if (e->param.use_bin) {
        e->cfg = qtk_eval_cfg_new_bin(e->param.cfg);
    } else {
        e->cfg = qtk_eval_cfg_new(e->param.cfg);
    }
    if (!e->cfg) {
        wtk_log_warn0(e->session->log, "cfg new failed.");
        _qtk_error(e->session, _QTK_CFG_NEW_FAILED);
        ret = -1;
        goto end;
    }

    qtk_eval_cfg_update_option(e->cfg, &session->opt);
    qtk_eval_cfg_update_params(e->cfg, params);
    e->eval = qtk_eval_new(e->cfg, session);
    if (!e->eval) {
        wtk_log_warn0(e->session->log, "eval new failed.");
        _qtk_error(e->session, _QTK_INSTANSE_NEW_FAILED);
        ret = -1;
        goto end;
    }

    if (e->param.use_thread) {
        buf_size = 32 * e->param.winStep;

        e->callback = qtk_engine_callback_new();
        e->callback->start_f = (qtk_engine_thread_start_f)qtk_eeval_on_start;
        e->callback->data_f = (qtk_engine_thread_data_f)qtk_eeval_on_data;
        e->callback->end_f = (qtk_engine_thread_end_f)qtk_eeval_on_end;
        e->callback->reset_f = (qtk_engine_thread_reset_f)qtk_eeval_on_reset;
        e->callback->set_notify_f =
            (qtk_engine_thread_set_notify_f)qtk_eeval_on_set_notify;
        e->callback->cancel_f = (qtk_engine_thread_cancel_f)qtk_eeval_on_cancel;
        e->callback->set_f = (qtk_engine_thread_set_f)qtk_eeval_on_set;
        e->callback->err_f = (qtk_engine_thread_err_f)qtk_eeval_on_err;
        e->callback->ths = e;

        e->thread = qtk_engine_thread_new(e->callback, e->session->log, "eeval",
                                          buf_size, 10, 1, e->param.syn);
    }

    e->evaltxt = wtk_strbuf_new(256, 1);

    ret = 0;
end:
    wtk_log_log(e->session->log, "ret = %d", ret);
    if (ret != 0) {
        qtk_eeval_delete(e);
        e = NULL;
    }
    return e;
}

int qtk_eeval_delete(qtk_eeval_t *e) {
    if (e->thread) {
        qtk_engine_thread_delete(e->thread, 0);
    }
    if (e->callback) {
        qtk_engine_callback_delete(e->callback);
    }

    if (e->eval) {
        qtk_eval_delete(e->eval);
    }

    if (e->cfg) {
        if (e->param.use_bin) {
            qtk_eval_cfg_delete_bin(e->cfg);
        } else {
            qtk_eval_cfg_delete(e->cfg);
        }
    }
    qtk_engine_param_clean(&e->param);

    if (e->evaltxt) {
        wtk_strbuf_delete(e->evaltxt);
    }

    wtk_free(e);
    return 0;
}

int qtk_eeval_start(qtk_eeval_t *e) {
    int ret = 0;

    if (e->param.use_thread) {
        qtk_engine_thread_start(e->thread);
    } else {
        ret = qtk_eeval_on_start(e);
        if (ret != 0) {
            wtk_log_warn(e->session->log, "qtk eeval start failed| ret =%d",
                         ret);
        }
    }
    return ret;
}

int qtk_eeval_feed(qtk_eeval_t *e, char *data, int bytes, int is_end) {
    int ret = 0;
    if (e->param.use_thread) {
        qtk_engine_thread_feed(e->thread, data, bytes, is_end);
    } else {
        if (bytes > 0) {
            ret = qtk_eeval_on_data(e, data, bytes);
            if (ret != 0) {
                wtk_log_warn(e->session->log,
                             "qtk eeval feed on data failed | ret = %d", ret);
                goto end;
            }
        }
        if (is_end) {
            ret = qtk_eeval_on_end(e);
            if (ret != 0) {
                wtk_log_warn(e->session->log,
                             "qtk eeval feed on end failed | ret = %d", ret);
                goto end;
            }
        }
    }
end:
    return ret;
}

int qtk_eeval_reset(qtk_eeval_t *e) {
    if (e->param.use_thread) {
        qtk_engine_thread_reset(e->thread);
    } else {
        qtk_eeval_on_reset(e);
    }
    return 0;
}

int qtk_eeval_cancel(qtk_eeval_t *e) {
    ++e->cancel;
    if (e->param.use_thread) {
        qtk_engine_thread_cancel(e->thread);
    } else {
        qtk_eeval_on_cancel(e);
    }

    return 0;
}

void qtk_eeval_set_notify(qtk_eeval_t *e, void *notify_ths,
                          qtk_engine_notify_f notify_f) {
    if (e->param.use_thread) {
        qtk_engine_thread_set_notify(e->thread, notify_ths, notify_f);
    } else {
        qtk_eeval_on_set_notify(e, notify_ths, notify_f);
    }
}

int qtk_eeval_set(qtk_eeval_t *e, char *data, int bytes) {
    if (e->param.use_thread) {
        qtk_engine_thread_set(e->thread, data, bytes);
    } else {
        qtk_eeval_on_set(e, data, bytes);
    }
    return 0;
}

int qtk_eeval_on_start(qtk_eeval_t *e) {
    return qtk_eval_start(e->eval, e->evaltxt->data, e->evaltxt->pos);
}

int qtk_eeval_on_data(qtk_eeval_t *e, char *data, int bytes) {
    return qtk_eval_feed(e->eval, data, bytes, 0);
}

int qtk_eeval_on_end(qtk_eeval_t *e) {
    qtk_var_t var;
    wtk_string_t v;
    int ret;
#ifdef USE_TIME_TEST
    double tm1;
    tm1 = time_get_ms();
#endif

    ret = qtk_eval_feed(e->eval, 0, 0, 1);
    if (ret != 0) {
        wtk_log_log0(e->session->log, "eval feed failed.");
        goto end;
    }

    v = qtk_eval_get_result(e->eval);
    wtk_log_log(e->session->log, "result = [%.*s] cancel = %d.", v.len, v.data,
                e->cancel);

    if (e->cancel) {
        return 0;
    }

    if (v.len <= 0) {
        wtk_log_warn0(e->session->log, "get result empty.");
        ret = -1;
        goto end;
    }

    if (!e->notify_f) {
        wtk_log_warn(e->session->log, "notify = %p", e->notify_f);
        _qtk_warning(e->session, _QTK_ENGINE_NOTIFY_INVALID);
        ret = 0;
        goto end;
    }

    var.type = QTK_EVAL_TEXT;
    var.v.str.data = v.data;
    var.v.str.len = v.len;
    e->notify_f(e->notify_ths, &var);
#ifdef USE_TIME_TEST
    // wtk_debug("=====>eval time delay:%lf\n",time_get_ms()-tm1);
    wtk_log_log(e->session->log, "====>eval time delay:%lf\n",
                time_get_ms() - tm1);
#endif
    ret = 0;
end:
    return ret;
}

void qtk_eeval_on_reset(qtk_eeval_t *e) { qtk_eval_reset(e->eval); }

void qtk_eeval_on_set_notify(qtk_eeval_t *e, void *notify_ths,
                             qtk_engine_notify_f notify_f) {
    e->notify_ths = notify_ths;
    e->notify_f = notify_f;
}

void qtk_eeval_on_cancel(qtk_eeval_t *e) {
    qtk_eval_cancel(e->eval);
    --e->cancel;
}

void qtk_eeval_on_set(qtk_eeval_t *e, char *data, int bytes) {
    wtk_cfg_file_t *cfile = NULL;
    wtk_cfg_item_t *item;
    wtk_queue_node_t *qn;
    int ret;

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
        if (wtk_string_cmp2(item->key, &qtk_engine_set_str[2]) == 0) {
            wtk_strbuf_reset(e->evaltxt);
            wtk_strbuf_push(e->evaltxt, item->value.str->data,
                            item->value.str->len);
        }
        if (wtk_string_cmp2(item->key, &qtk_engine_set_str[11]) == 0) {
            qtk_eval_set_coreType(e->eval, item->value.str->data,
                                  item->value.str->len);
        }
        if (wtk_string_cmp2(item->key, &qtk_engine_set_str[12]) == 0) {
            qtk_eval_set_res(e->eval, item->value.str->data,
                             item->value.str->len);
        }
    }
end:
    if (cfile) {
        wtk_cfg_file_delete(cfile);
    }
}

void qtk_eeval_on_err(qtk_eeval_t *e) {

    if (!e->notify_f) {
        wtk_log_warn(e->session->log, "notify = %p", e->notify_f);
        _qtk_warning(e->session, _QTK_ENGINE_NOTIFY_INVALID);
        return;
    }
}
