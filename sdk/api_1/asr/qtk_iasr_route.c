#include "qtk_iasr_route.h"

static qtk_ir_msg_t *qtk_ir_msg_new(qtk_iasr_route_t *route) {
    qtk_ir_msg_t *msg;

    msg = (qtk_ir_msg_t *)wtk_malloc(sizeof(qtk_ir_msg_t));
    msg->buf = wtk_strbuf_new(512, 1);

    return msg;
}

static int qtk_ir_msg_delete(qtk_ir_msg_t *msg) {
    wtk_strbuf_delete(msg->buf);
    wtk_free(msg);
    return 0;
}

static qtk_ir_msg_t *qtk_ir_pop_msg(qtk_iasr_route_t *route) {
    qtk_ir_msg_t *msg;

    msg = (qtk_ir_msg_t *)wtk_lockhoard_pop(&route->msg_hoard);
    wtk_strbuf_reset(msg->buf);
    msg->sem = NULL;

    return msg;
}

static void qtk_ir_push_msg(qtk_iasr_route_t *route, qtk_ir_msg_t *msg) {
    wtk_lockhoard_push(&route->msg_hoard, msg);
}

static void qtk_ir_notify(qtk_iasr_route_t *route, qtk_ir_state_t state,
                          qtk_ir_notify_t *notify) {
    if (route->notify) {
        route->notify(route->notify_ths, state, notify);
    }
}

static int qtk_ir_run(qtk_iasr_route_t *route, wtk_thread_t *thread) {
    qtk_ir_msg_t *msg;
    wtk_queue_node_t *qn;
    qtk_ir_notify_t notify;
    int ret;

    notify.iasr = route->iasr;
    notify.priority = route->iasr->cfg->priority;
    notify.wait = route->iasr->cfg->wait;
    notify.serial = route->serial;
    notify.rec.data = NULL;
    notify.rec.len = 0;

    while (route->run) {
        qn = wtk_blockqueue_pop(&route->input_q, -1, NULL);
        if (!qn) {
            break;
        }
        msg = data_offset2(qn, qtk_ir_msg_t, q_n);

        		// wtk_debug("state %d name %.*s cancel %d msgtype %d\n",route->state,
        		// 		route->iasr->cfg->name.len,route->iasr->cfg->name.data,
        		// 		route->cancel,msg->type);

        if (route->cancel) {
            if (msg->type == QTK_IR_MSG_CANCEL) {
                route->state = QTK_IR_RUN_END;
                route->cancel = 0;
            }
            if (msg->sem) {
                wtk_sem_release(msg->sem, 1);
            }
            qtk_ir_push_msg(route, msg);
            continue;
        }

        switch (route->state) {
        case QTK_IR_RUN_INIT:
            if (msg->type == QTK_IR_MSG_START) {
                ret = qtk_iasr_start(route->iasr);
                if (ret == 0) {
                    route->state = QTK_IR_RUN_PROCESS;
                } else {
                    qtk_ir_notify(route, QTK_IR_ERR, &notify);
                    route->state = QTK_IR_RUN_END;
                }
            }
            break;

        case QTK_IR_RUN_PROCESS:
            switch (msg->type) {
            case QTK_IR_MSG_DATA:
                ret = qtk_iasr_feed(route->iasr, msg->buf->data, msg->buf->pos,
                                    0);
                if (ret != 0) {
                    qtk_ir_notify(route, QTK_IR_ERR, &notify);
                    route->state = QTK_IR_RUN_END;
                }
                break;
            case QTK_IR_MSG_END:
                ret = qtk_iasr_feed(route->iasr, msg->buf->data, msg->buf->pos,1);
                if (ret == 0) {
                    if(route->iasr->cfg->type == QTK_IASR_SPX){
                        qtk_ir_notify(route,QTK_IR_NOTHING,&notify);
                    }else{
#ifdef USE_KSD_EBNF
                        notify.rec = qtk_iasr_get_result(route->iasr);
                        qtk_ir_notify(route,notify.rec.len > 0 ? QTK_IR_DATA : QTK_IR_ERR,&notify);
#else
                        if(route->iasr->cfg->use_general_asr == 0)
                        {
                            if(route->iasr->cfg->type != QTK_IASR_GR_NEW)
                            {
                                notify.rec = qtk_iasr_get_result(route->iasr);
                                qtk_ir_notify(route,notify.rec.len > 0 ? QTK_IR_DATA : QTK_IR_ERR,&notify);
                            }else{
                                qtk_ir_notify(route,QTK_IR_NOTHING,&notify);
                            }
                        }else{
                            notify.rec = qtk_iasr_get_result(route->iasr);
                            qtk_ir_notify(route,notify.rec.len > 0 ? QTK_IR_DATA : QTK_IR_ERR,&notify);
                        }
#endif
                    }
                } else {
                    qtk_ir_notify(route, QTK_IR_ERR, &notify);
                }
                route->state = QTK_IR_RUN_END;
                break;
            default:
                break;
            }
            break;

        case QTK_IR_RUN_END:
            if (msg->type == QTK_IR_MSG_RESET) {
                qtk_iasr_reset(route->iasr);
                route->state = QTK_IR_RUN_INIT;
            }
            break;
        }
        if (msg->sem) {
            wtk_sem_release(msg->sem, 1);
        }
        qtk_ir_push_msg(route, msg);
    }

    return 0;
}

static void qtk_iasr_route_init(qtk_iasr_route_t *route) {
    route->session = NULL;
    route->iasr = NULL;
    route->serial = -1;

    route->notify = NULL;
    route->notify_ths = NULL;

    route->run = 0;
    route->cancel = 0;

    route->state = QTK_IR_RUN_INIT;
}

qtk_iasr_route_t *qtk_iasr_route_new(qtk_iasr_t *iasr, qtk_session_t *session,
                                     int serial) {
    qtk_iasr_route_t *route;

    route = (qtk_iasr_route_t *)wtk_malloc(sizeof(qtk_iasr_route_t));
    qtk_iasr_route_init(route);

    route->iasr = iasr;
    route->session = session;
    route->serial = serial;

    wtk_lockhoard_init(&route->msg_hoard, offsetof(qtk_ir_msg_t, hoard_n), 10,
                       (wtk_new_handler_t)qtk_ir_msg_new,
                       (wtk_delete_handler_t)qtk_ir_msg_delete, route);
    wtk_blockqueue_init(&route->input_q);

    route->run = 1;
    wtk_thread_init(&route->thread, (thread_route_handler)qtk_ir_run, route);
    wtk_thread_set_name(&route->thread, route->iasr->cfg->name.data);
    wtk_thread_start(&route->thread);

    return route;
}

void qtk_iasr_route_delete(qtk_iasr_route_t *route) {
    route->run = 0;
    wtk_blockqueue_wake(&route->input_q);
    wtk_thread_join(&route->thread);
    wtk_thread_clean(&route->thread);

    wtk_blockqueue_clean(&route->input_q);
    wtk_lockhoard_clean(&route->msg_hoard);

    wtk_free(route);
}

void qtk_iasr_route_set_notify(qtk_iasr_route_t *route, void *notify_ths,
                               qtk_iasr_route_notify_f notify) {
    route->notify = notify;
    route->notify_ths = notify_ths;
}

void qtk_iasr_route_start(qtk_iasr_route_t *route) {
    qtk_ir_msg_t *msg;

    // wtk_debug("====> %.*s
    // start\n",route->iasr->cfg->name.len,route->iasr->cfg->name.data);
    msg = qtk_ir_pop_msg(route);
    msg->type = QTK_IR_MSG_START;

    wtk_blockqueue_push(&route->input_q, &msg->q_n);
}

void qtk_iasr_route_feed(qtk_iasr_route_t *route, char *data, int bytes,
                         int is_end) {
    qtk_ir_msg_t *msg;

    // wtk_debug("====> %.*s feed %d\n",route->iasr->cfg->name.len,route->iasr->cfg->name.data,is_end);
    msg = qtk_ir_pop_msg(route);
    msg->type = is_end ? QTK_IR_MSG_END : QTK_IR_MSG_DATA;
    wtk_strbuf_push(msg->buf, data, bytes);

    wtk_blockqueue_push(&route->input_q, &msg->q_n);
}

void qtk_iasr_route_reset(qtk_iasr_route_t *route) {
    qtk_ir_msg_t *msg;
    wtk_sem_t sem;

    // wtk_debug("====> %.*s
    // reset\n",route->iasr->cfg->name.len,route->iasr->cfg->name.data);

    if (route->state == QTK_IR_RUN_PROCESS && !route->cancel) {
        qtk_iasr_cancel(route->iasr);
    }
    msg = qtk_ir_pop_msg(route);
    msg->type = QTK_IR_MSG_RESET;
    wtk_sem_init(&sem, 0);
    msg->sem = &sem;
    wtk_blockqueue_push(&route->input_q, &msg->q_n);
    wtk_sem_acquire(&sem, 1000);
}

void qtk_iasr_route_cancel(qtk_iasr_route_t *route) {
    qtk_ir_msg_t *msg;

    // wtk_debug("====> %.*s
    // cancel\n",route->iasr->cfg->name.len,route->iasr->cfg->name.data);
    route->cancel = 1;
    qtk_iasr_cancel(route->iasr);
    msg = qtk_ir_pop_msg(route);
    msg->type = QTK_IR_MSG_CANCEL;
    wtk_blockqueue_push(&route->input_q, &msg->q_n);
}
