#include "qtk_esemdlg.h"
#include "sdk/engine/comm/qtk_engine_hdr.h"

void qtk_esemdlg_init(qtk_esemdlg_t *e) {
    qtk_engine_param_init(&e->param);

    e->session = NULL;

    e->cfg = NULL;
    e->s = NULL;

    e->model = NULL;
    e->semdlg_item = NULL;

    e->loop_time = 500;
    e->run = 0;
    e->init = 1;
}

qtk_esemdlg_msg_t *qtk_esemdlg_msg_new(qtk_esemdlg_t *e) {
    qtk_esemdlg_msg_t *msg;

    msg = (qtk_esemdlg_msg_t *)wtk_malloc(sizeof(qtk_esemdlg_msg_t));
    msg->buf = wtk_strbuf_new(256, 1);
    return msg;
}

int qtk_esemdlg_msg_delete(qtk_esemdlg_msg_t *msg) {
    wtk_strbuf_delete(msg->buf);
    wtk_free(msg);
    return 0;
}

qtk_esemdlg_msg_t *qtk_esemdlg_pop_msg(qtk_esemdlg_t *e) {
    qtk_esemdlg_msg_t *msg;

    msg = (qtk_esemdlg_msg_t *)wtk_lockhoard_pop(&(e->msg_hoard));
    wtk_strbuf_reset(msg->buf);
    return msg;
}

void qtk_esemdlg_push_msg(qtk_esemdlg_t *e, qtk_esemdlg_msg_t *msg) {
    wtk_lockhoard_push(&(e->msg_hoard), msg);
}

int qtk_esemdlg_run(qtk_esemdlg_t *e, wtk_thread_t *t);

void qtk_esemdlg_on_semdlg_model(qtk_esemdlg_t *e,
                                 wtk_model_item_v_t *old_value,
                                 wtk_model_item_v_t *new_value) {
    //	if(old_value) {
    //		wtk_debug("old = %d\n",old_value->type);
    //	}
    //	wtk_debug("new = %d\n",new_value->type);

    if (old_value) {
        wtk_log_log(e->session->log, "model \"semdlg\" old/new  = %d / %d.",
                    old_value->v.i, new_value->v.i);
    } else {
        wtk_log_log(e->session->log, "model \"semdlg\" new  = %d.",
                    new_value->v.i);
    }
}

qtk_esemdlg_t *qtk_esemdlg_new(qtk_session_t *session,
                               wtk_local_cfg_t *params) {
    qtk_esemdlg_t *e;
    int ret;

    e = (qtk_esemdlg_t *)wtk_malloc(sizeof(qtk_esemdlg_t));
    qtk_esemdlg_init(e);

    e->session = session;

    qtk_engine_param_set_session(&e->param, session);
    ret = qtk_engine_param_feed(&e->param, params);
    if (ret != 0) {
        wtk_log_warn0(e->session->log, "params als failed.");
        goto end;
    }

    if (e->param.use_bin) {
        e->cfg = qtk_semdlg_cfg_new_bin(e->param.cfg);
    } else {
        e->cfg = qtk_semdlg_cfg_new(e->param.cfg);
    }
    if (!e->cfg) {
        wtk_log_warn0(e->session->log, "cfg new failed.");
        _qtk_error(e->session, _QTK_CFG_NEW_FAILED);
        ret = -1;
        goto end;
    }

    qtk_semdlg_cfg_update_params(e->cfg, params);
    qtk_semdlg_cfg_update_option(e->cfg, &session->opt);

    if (e->param.use_thread) {
        e->model = wtk_model_new(7);
        e->semdlg_item = wtk_model_get_item_s(e->model, "semdlg");
        wtk_model_add_listener2(
            e->model, e->semdlg_item, e,
            (wtk_model_notify_f)qtk_esemdlg_on_semdlg_model);
        e->s = qtk_semdlg_new(e->cfg, session, e->model);
    } else {
        e->s = qtk_semdlg_new(e->cfg, session, NULL);
    }
    if (!e->s) {
        wtk_log_warn0(e->session->log, "semdlg new failed.");
        _qtk_error(e->session, _QTK_INSTANSE_NEW_FAILED);
        ret = -1;
        goto end;
    }

    if (e->param.use_thread) {
        e->run = 1;

        wtk_sem_init(&(e->start_sem), 0);
        wtk_blockqueue_init(&(e->input_q));
        wtk_lockhoard_init(&(e->msg_hoard),
                           offsetof(qtk_esemdlg_msg_t, hoard_n), 10,
                           (wtk_new_handler_t)qtk_esemdlg_msg_new,
                           (wtk_delete_handler_t)qtk_esemdlg_msg_delete, e);
        wtk_thread_init(&(e->thread), (thread_route_handler)qtk_esemdlg_run, e);
        wtk_thread_set_name(&(e->thread), "esemdlg");
        wtk_thread_start(&(e->thread));
    }

    ret = 0;
end:
    wtk_log_log(e->session->log, "ret = %d", ret);
    if (ret != 0) {
        qtk_esemdlg_delete(e);
        e = NULL;
    }
    return e;
}

void qtk_esemdlg_stop(qtk_esemdlg_t *e) {
    e->run = 0;
    wtk_blockqueue_wake(&(e->input_q));
    wtk_thread_join(&(e->thread));
    wtk_thread_clean(&(e->thread));

    wtk_blockqueue_clean(&(e->input_q));
    wtk_lockhoard_clean(&(e->msg_hoard));
    wtk_sem_clean(&(e->start_sem));
}

int qtk_esemdlg_delete(qtk_esemdlg_t *e) {
    if (e->param.use_thread && e->run) {
        qtk_esemdlg_stop(e);
        wtk_model_delete(e->model);
    }
    if (e->s) {
        qtk_semdlg_delete(e->s);
    }
    if (e->cfg) {
        e->param.use_bin ? qtk_semdlg_cfg_delete_bin(e->cfg)
                         : qtk_semdlg_cfg_delete(e->cfg);
    }
    wtk_free(e);
    return 0;
}

void qtk_esemdlg_on_err(qtk_esemdlg_t *e) {
    qtk_var_t var;

    if (!e->notify_f) {
        wtk_log_warn(e->session->log, "notify_f=%p", e->notify_f);
        _qtk_warning(e->session, _QTK_ENGINE_NOTIFY_INVALID);
        return;
    }
    var.type = QTK_VAR_ERR;
    var.v.i = _QTK_ENGINE_NO_RESPONSE;
    e->notify_f(e->notify_ths, &var);
}

int qtk_esemdlg_on_txt(qtk_esemdlg_t *e, char *txt, int bytes, int use_json) {
    wtk_string_t v;
    qtk_var_t var;
    int ret;
#ifdef USE_TIME_TEST
    double tm1;
    tm1 = time_get_ms();
#endif

    wtk_log_log(e->session->log, "feed [%.*s] use_json = %d.", bytes, txt,
                use_json);
    v = qtk_semdlg_process(e->s, txt, bytes, use_json);
    if (!e->notify_f) {
        ret = -1;
        qtk_esemdlg_on_err(e);
        goto end;
    }
    if (v.len <= 0) {
        ret = -1;
        qtk_esemdlg_on_err(e);
        goto end;
    }

    wtk_log_log(e->session->log, "rlt = [%.*s]", v.len, v.data);
#ifdef USE_TIME_TEST
    wtk_log_log(e->session->log, "===>semdlg time delay:%f\n",
                (time_get_ms() - tm1));
#endif
    var.type = QTK_SEMDLG_TEXT;
    var.v.str.data = v.data;
    var.v.str.len = v.len;

    e->notify_f(e->notify_ths, &var);

    ret = 0;
end:
    return ret;
}

void qtk_esemdlg_on_set(qtk_esemdlg_t *e, char *data, int bytes) {
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
        if (wtk_string_cmp2(item->key, &qtk_engine_set_str[3]) == 0) {
            e->param.use_json = atoi(item->value.str->data) == 1 ? 1 : 0;
        }
        if (wtk_string_cmp2(item->key, &qtk_engine_set_str[4]) == 0) {
            qtk_semdlg_set_env(e->s, item->value.str->data,
                               item->value.str->len);
        }
    }
end:
    if (cfile) {
        wtk_cfg_file_delete(cfile);
    }
}

void qtk_esemdlg_on_set_notify(qtk_esemdlg_t *e, void *notify_ths,
                               qtk_engine_notify_f notify_f) {
    e->notify_f = notify_f;
    e->notify_ths = notify_ths;
    if (e->param.use_thread) {
        wtk_sem_release(&(e->start_sem), 1);
    }
}

int qtk_esemdlg_start(qtk_esemdlg_t *e) { return 0; }

int qtk_esemdlg_feed(qtk_esemdlg_t *e, char *data, int bytes, int is_end) {
    qtk_esemdlg_msg_t *msg;
    int ret = 0;

    if (bytes <= 0) {
        return -1;
    }

    if (e->param.use_thread) {
        msg = qtk_esemdlg_pop_msg(e);
        msg->type = QTK_ESEMDLG_TXT;
        wtk_strbuf_push(msg->buf, data, bytes);
        wtk_blockqueue_push(&(e->input_q), &(msg->q_n));
    } else {
        ret = qtk_esemdlg_on_txt(e, data, bytes, e->param.use_json);
    }
    return ret;
}

int qtk_esemdlg_set(qtk_esemdlg_t *e, char *data, int bytes) {
    qtk_esemdlg_msg_t *msg;

    if (bytes <= 0) {
        return -1;
    }

    if (e->param.use_thread) {
        msg = qtk_esemdlg_pop_msg(e);
        msg->type = QTK_ESEMDLG_SET;
        wtk_strbuf_push(msg->buf, data, bytes);
        wtk_blockqueue_push(&e->input_q, &msg->q_n);
    } else {
        qtk_esemdlg_on_set(e, data, bytes);
    }
    return 0;
}

int qtk_esemdlg_reset(qtk_esemdlg_t *e) { return 0; }

void qtk_esemdlg_set_notify(qtk_esemdlg_t *e, void *notify_ths,
                            qtk_engine_notify_f notify_f) {
    qtk_esemdlg_msg_t *msg;
    uintptr_t p[2];

    if (e->param.use_thread && !e->init) {
        msg = qtk_esemdlg_pop_msg(e);
        msg->type = QTK_ESEMDLG_NOTIFY;
        p[0] = (uintptr_t)notify_ths;
        p[1] = (uintptr_t)notify_f;
        wtk_strbuf_push(msg->buf, (char *)p, sizeof(p));
        wtk_blockqueue_push(&e->input_q, &msg->q_n);
    } else {
        qtk_esemdlg_on_set_notify(e, notify_ths, notify_f);
    }
    e->init = 0;
}

int qtk_esemdlg_run(qtk_esemdlg_t *e, wtk_thread_t *t) {
    wtk_queue_node_t *qn;
    qtk_esemdlg_msg_t *msg;
    double active_time;

    wtk_sem_acquire(&(e->start_sem), 400);
    if (e->param.active) {
        qtk_esemdlg_on_txt(e, QTK_ESEMDLG_ACTIVE,
                           sizeof(QTK_ESEMDLG_ACTIVE) - 1, 0);
        wtk_model_item_set_i(e->semdlg_item, 1);
    }
    active_time = time_get_ms();

    while (e->run) {
        qn = wtk_blockqueue_pop(&(e->input_q), e->loop_time, NULL);
        if (!qn) {
            if (e->param.idle) {
                if ((time_get_ms() - active_time) > e->param.idle_time) {
                    qtk_esemdlg_on_txt(e, QTK_ESEMDLG_IDLE,
                                       sizeof(QTK_ESEMDLG_IDLE) - 1, 0);
                    active_time = time_get_ms();
                    wtk_model_item_set_i(e->semdlg_item, 0);
                }
            }
            continue;
        }

        msg = data_offset2(qn, qtk_esemdlg_msg_t, q_n);

        switch (msg->type) {
        case QTK_ESEMDLG_TXT:
            qtk_esemdlg_on_txt(e, msg->buf->data, msg->buf->pos,
                               e->param.use_json);
            active_time = time_get_ms();
            break;
        case QTK_ESEMDLG_SET:
            qtk_esemdlg_on_set(e, msg->buf->data, msg->buf->pos);
            break;
        case QTK_ESEMDLG_NOTIFY:
            qtk_esemdlg_on_set_notify(
                e, (void *)((uintptr_t *)msg->buf->data)[0],
                (qtk_engine_notify_f)((uintptr_t *)msg->buf->data)[1]);
            break;
        }
        qtk_esemdlg_push_msg(e, msg);
    }
    return 0;
}
