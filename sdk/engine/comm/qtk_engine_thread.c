#include "qtk_engine_thread.h"
#include <stdint.h>

qtk_engine_callback_t *qtk_engine_callback_new() {
    qtk_engine_callback_t *callback;

    callback =
        (qtk_engine_callback_t *)wtk_malloc(sizeof(qtk_engine_callback_t));
    memset(callback, 0, sizeof(qtk_engine_callback_t));

    return callback;
}

void qtk_engine_callback_delete(qtk_engine_callback_t *callback) {
    wtk_free(callback);
}

qtk_engine_thread_msg_t *qtk_engine_thread_msg_new(qtk_engine_thread_t *t) {
    qtk_engine_thread_msg_t *msg;

    msg =
        (qtk_engine_thread_msg_t *)wtk_malloc(sizeof(qtk_engine_thread_msg_t));
    msg->buf = wtk_strbuf_new(t->buf_size, 1);
    return msg;
}

int qtk_engine_thread_msg_delete(qtk_engine_thread_msg_t *msg) {
    wtk_strbuf_delete(msg->buf);
    wtk_free(msg);
    return 0;
}

qtk_engine_thread_msg_t *qtk_engine_thread_pop_msg(qtk_engine_thread_t *t) {
    qtk_engine_thread_msg_t *msg;

    msg = (qtk_engine_thread_msg_t *)wtk_lockhoard_pop(&(t->msg_hoard));
    wtk_strbuf_reset(msg->buf);
    return msg;
}

void qtk_engine_thread_push_msg(qtk_engine_thread_t *t,
                                qtk_engine_thread_msg_t *msg) {
    wtk_lockhoard_push(&(t->msg_hoard), msg);
}

int qtk_engine_thread_run(qtk_engine_thread_t *t, wtk_thread_t *thread) {
    typedef enum {
        QTK_ENGINE_THREAD_SINIT,
        QTK_ENGINE_THREAD_SPROCESS,
        QTK_ENGINE_THREAD_SEND,
    } qtk_engine_thread_state_t;
    wtk_queue_node_t *qn;
    qtk_engine_thread_msg_t *msg;
    qtk_engine_thread_state_t state;
    int ret;
    int b;

    state = QTK_ENGINE_THREAD_SINIT;
    while (t->run) {
        qn = wtk_blockqueue_pop(&(t->input_q), -1, NULL);
        if (!qn) {
            break;
        }
        msg = data_offset2(qn, qtk_engine_thread_msg_t, q_n);

        b = 1;
        switch (msg->type) {
        case QTK_ENGINE_THREAD_SET:
            if (t->callback->set_f) {
                t->callback->set_f(t->callback->ths, msg->buf->data,
                                   msg->buf->pos);
            }
            break;
        case QTK_ENGINE_THREAD_SET_NOTIFY:
            if (t->callback->set_notify_f) {
                t->callback->set_notify_f(
                    t->callback->ths, (void *)((uintptr_t *)msg->buf->data)[0],
                    (qtk_engine_notify_f)((uintptr_t *)msg->buf->data)[1]);
                wtk_log_log(
                    t->log, "engine [%s] set notify f = %p  ths = %p.",
                    t->thread.name, (void *)((uintptr_t *)msg->buf->data)[0],
                    (qtk_engine_notify_f)((uintptr_t *)msg->buf->data)[1]);
            } else {
                wtk_log_warn(
                    t->log, "engine [%s] no set_notify_f. skip f=%p ths=%p.",
                    t->thread.name, (void *)((uintptr_t *)msg->buf->data)[0],
                    (qtk_engine_notify_f)((uintptr_t *)msg->buf->data)[1]);
            }
            break;
        default:
            b = 0;
            break;
        }

        if (b) {
            qtk_engine_thread_push_msg(t, msg);
            continue;
        }

        if (t->cancel) {
            if (msg->type == QTK_ENGINE_THREAD_CANCEL) {
                if (t->callback->cancel_f) {
                    t->callback->cancel_f(t->callback->ths);
                }
                t->callback->reset_f(t->callback->ths);
                state = QTK_ENGINE_THREAD_SINIT;
                --t->cancel;
            }

            wtk_log_log(t->log, "engine [%s] cancel msg = %d.", t->thread.name,
                        msg->type);
            qtk_engine_thread_push_msg(t, msg);
            continue;
        }

        // wtk_debug("state = %d msgtype %d\n",state,msg->type);

        switch (state) {
        case QTK_ENGINE_THREAD_SINIT:
            switch (msg->type) {
            case QTK_ENGINE_THREAD_START:
                ret = t->callback->start_f(t->callback->ths);
                if (ret == 0) {
                    state = QTK_ENGINE_THREAD_SPROCESS;
                } else {
                    if (t->callback->err_f) {
                        t->callback->err_f(t->callback->ths);
                    }
                    wtk_log_warn(t->log, "name = [%s] start failed ret = %d.",
                                 t->thread.name, ret);
                }
                break;
            default:
                wtk_log_warn(t->log, "name=[%s].======> state/msg [%d/%d] .",
                             t->thread.name, state, msg->type);
                break;
            }
            break;

        case QTK_ENGINE_THREAD_SPROCESS:
            switch (msg->type) {
            case QTK_ENGINE_THREAD_DATA:
                ret = t->callback->data_f(t->callback->ths, msg->buf->data,
                                          msg->buf->pos);
                if (ret != 0) {
                    wtk_log_warn(t->log, "name [%s] feed failed ret = %d",
                                 t->thread.name, ret);
                    state = QTK_ENGINE_THREAD_SEND;
                    if (t->callback->err_f) {
                        t->callback->err_f(t->callback->ths);
                    }
                }
                break;
            case QTK_ENGINE_THREAD_END:
                ret = t->callback->end_f(t->callback->ths);
                // wtk_debug("ret = %d\n",ret);
                if (ret != 0) {
                    wtk_log_warn(t->log, "name [%s] feed end failed ret = %d",
                                 t->thread.name, ret);
                    if (t->callback->err_f) {
                        t->callback->err_f(t->callback->ths);
                    }
                }
                if (t->syn) {
                    if (t->is_released == 0) {
                        wtk_sem_release(&t->sem, 1);
                        t->is_released = 1;
                    }
                }
                state = QTK_ENGINE_THREAD_SEND;
                break;
            default:
                wtk_log_warn(t->log,
                             "Warning:name=[%s].engine run state/msg [%d/%d] .",
                             t->thread.name, state, msg->type);
                break;
            }
            break;

        case QTK_ENGINE_THREAD_SEND:
            switch (msg->type) {
            case QTK_ENGINE_THREAD_RESET:
                t->callback->reset_f(t->callback->ths);
                state = QTK_ENGINE_THREAD_SINIT;
                break;
            default:
                wtk_log_warn(t->log,
                             "Warning:name=[%s].engine run state/msg [%d/%d] .",
                             t->thread.name, state, msg->type);
                break;
            }
            if (t->syn) {
                if (t->is_released == 0) {
                    wtk_sem_release(&t->sem, 1);
                    t->is_released = 1;
                }
            }
            break;
        }
        qtk_engine_thread_push_msg(t, msg);
    }

    return 0;
}

qtk_engine_thread_t *qtk_engine_thread_new(qtk_engine_callback_t *callback,
                                           wtk_log_t *log, char *name,
                                           int buf_size, int cache,
                                           int use_step, int syn) {
    qtk_engine_thread_t *t;

    if (!callback || !callback->start_f || !callback->data_f ||
        !callback->end_f || !callback->reset_f || !callback->ths) {
        return NULL;
    }

    t = (qtk_engine_thread_t *)wtk_malloc(sizeof(qtk_engine_thread_t));

    t->callback = callback;
    t->log = log;
    t->buf_size = buf_size;
    t->cache = cache;
    t->use_step = use_step;
    t->syn = syn;

    wtk_log_log(
        t->log,
        "Engine Thread [%s] func start_f=%p  data_f=%p end_f=%p reset_f=%p\
 set_notify_f=%p cancel_f=%p set_f=%p ths=%p .",
        name, t->callback->start_f, t->callback->data_f, t->callback->end_f,
        t->callback->reset_f, t->callback->set_notify_f, t->callback->cancel_f,
        t->callback->set_f, t->callback->ths);

    wtk_log_log(t->log, "use step = %d.", t->use_step);
    wtk_log_log(t->log, "buf_size = %d.", t->buf_size);
    wtk_log_log(t->log, "cache = %d.", t->cache);

    t->cancel = 0;
    wtk_blockqueue_init(&(t->input_q));
    wtk_lockhoard_init(&(t->msg_hoard),
                       offsetof(qtk_engine_thread_msg_t, hoard_n), t->cache,
                       (wtk_new_handler_t)qtk_engine_thread_msg_new,
                       (wtk_delete_handler_t)qtk_engine_thread_msg_delete, t);

    wtk_thread_init(&(t->thread), (thread_route_handler)qtk_engine_thread_run,
                    t);
    wtk_thread_set_name(&(t->thread), name);
    t->run = 1;
    wtk_thread_start(&(t->thread));

    t->buf = wtk_strbuf_new(t->buf_size, 1);
    if (t->syn == 1) {
        wtk_sem_init(&(t->sem), 0);
    }
    return t;
}

void qtk_engine_thread_delete(qtk_engine_thread_t *t, int syn) {
    wtk_log_log(t->log, "Thread engine [%s] syn = %d.", t->thread.name, syn);
    if (syn) {
        wtk_blockqueue_wake(&(t->input_q));
        wtk_thread_join(&(t->thread));
        t->run = 0;
    } else {
        t->run = 0;
        wtk_blockqueue_wake(&(t->input_q));
        wtk_thread_join(&(t->thread));
    }
    if (t->syn == 1) {
        wtk_sem_clean(&t->sem);
    }
    wtk_thread_clean(&(t->thread));
    wtk_blockqueue_clean(&(t->input_q));
    wtk_lockhoard_clean(&(t->msg_hoard));
    wtk_log_log(t->log, "name=[%s].engine thread delete.", t->thread.name);

    wtk_strbuf_delete(t->buf);
    wtk_free(t);
}

void qtk_engine_thread_start(qtk_engine_thread_t *t) {
    qtk_engine_thread_msg_t *msg;

    msg = qtk_engine_thread_pop_msg(t);
    msg->type = QTK_ENGINE_THREAD_START;
    wtk_blockqueue_push(&(t->input_q), &(msg->q_n));
}

void qtk_engine_thread_feed_data(qtk_engine_thread_t *t, char *data,
                                 int bytes) {
    wtk_strbuf_t *buf = t->buf;
    qtk_engine_thread_msg_t *msg;

    wtk_strbuf_push(t->buf, data, bytes);

    while (buf->pos >= t->buf_size) {
        msg = qtk_engine_thread_pop_msg(t);
        msg->type = QTK_ENGINE_THREAD_DATA;
        wtk_strbuf_push(msg->buf, buf->data, t->buf_size);
        wtk_strbuf_pop(buf, NULL, t->buf_size);
        wtk_blockqueue_push(&(t->input_q), &(msg->q_n));
    }
}

void qtk_engine_thread_feed(qtk_engine_thread_t *t, char *data, int bytes,
                            int is_end) {
    qtk_engine_thread_msg_t *msg;

    if (bytes <= 0) {
        goto end;
    }

    if (t->use_step) {
        qtk_engine_thread_feed_data(t, data, bytes);
    } else {
        msg = qtk_engine_thread_pop_msg(t);
        msg->type = QTK_ENGINE_THREAD_DATA;
        wtk_strbuf_push(msg->buf, data, bytes);
        wtk_blockqueue_push(&(t->input_q), &(msg->q_n));
    }

end:
    if (is_end) {
        if (t->use_step && t->buf->pos > 0) {
            msg = qtk_engine_thread_pop_msg(t);
            msg->type = QTK_ENGINE_THREAD_DATA;
            wtk_strbuf_push(msg->buf, t->buf->data, t->buf->pos);
            wtk_strbuf_reset(t->buf);
            wtk_blockqueue_push(&(t->input_q), &(msg->q_n));
        }
        msg = qtk_engine_thread_pop_msg(t);
        msg->type = QTK_ENGINE_THREAD_END;
        wtk_blockqueue_push(&(t->input_q), &(msg->q_n));
        if (t->syn == 1) {
            t->is_released = 0;
            wtk_sem_acquire(&t->sem, -1);
        }
    }
}

void qtk_engine_thread_reset(qtk_engine_thread_t *t) {
    qtk_engine_thread_msg_t *msg;

    msg = qtk_engine_thread_pop_msg(t);
    msg->type = QTK_ENGINE_THREAD_RESET;
    wtk_blockqueue_push(&(t->input_q), &(msg->q_n));
}

void qtk_engine_thread_set_notify(qtk_engine_thread_t *t, void *notify_ths,
                                  qtk_engine_notify_f notify_f) {
    qtk_engine_thread_msg_t *msg;
    uintptr_t p[2];

    msg = qtk_engine_thread_pop_msg(t);
    msg->type = QTK_ENGINE_THREAD_SET_NOTIFY;

    p[0] = (uintptr_t)notify_ths;
    p[1] = (uintptr_t)notify_f;
    wtk_strbuf_push(msg->buf, (char *)p, sizeof(p));

    wtk_blockqueue_push(&(t->input_q), &(msg->q_n));
}

void qtk_engine_thread_set(qtk_engine_thread_t *t, char *data, int bytes) {
    qtk_engine_thread_msg_t *msg;

    msg = qtk_engine_thread_pop_msg(t);
    msg->type = QTK_ENGINE_THREAD_SET;
    wtk_strbuf_push(msg->buf, data, bytes);
    wtk_blockqueue_push(&t->input_q, &msg->q_n);
}

void qtk_engine_thread_cancel(qtk_engine_thread_t *t) {
    qtk_engine_thread_msg_t *msg;

    ++t->cancel;
    msg = qtk_engine_thread_pop_msg(t);
    msg->type = QTK_ENGINE_THREAD_CANCEL;
    wtk_blockqueue_push(&(t->input_q), &(msg->q_n));
}

void qtk_engine_thread_set_str(qtk_engine_thread_t *t,
                               qtk_engine_thread_type_t type, char *data,
                               int bytes) {
    qtk_engine_thread_msg_t *msg;

    msg = qtk_engine_thread_pop_msg(t);
    msg->type = type;
    wtk_strbuf_push(msg->buf, data, bytes);
    wtk_blockqueue_push(&(t->input_q), &(msg->q_n));
}

void qtk_engine_thread_set_num(qtk_engine_thread_t *t,
                               qtk_engine_thread_type_t type, double num) {
    qtk_engine_thread_msg_t *msg;
    double p[1];

    msg = qtk_engine_thread_pop_msg(t);
    msg->type = type;

    p[0] = num;
    wtk_strbuf_push(msg->buf, (char *)p, sizeof(double));

    wtk_blockqueue_push(&(t->input_q), &(msg->q_n));
}

void qtk_engine_thread_set_int(qtk_engine_thread_t *t,
                               qtk_engine_thread_type_t type, int i) {
    qtk_engine_thread_msg_t *msg;
    int p[1];

    msg = qtk_engine_thread_pop_msg(t);
    msg->type = type;

    p[0] = i;
    wtk_strbuf_push(msg->buf, (char *)p, sizeof(int));

    wtk_blockqueue_push(&(t->input_q), &(msg->q_n));
}
