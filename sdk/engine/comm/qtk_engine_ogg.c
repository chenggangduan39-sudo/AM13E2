#include "qtk_engine_ogg.h"

static qtk_engine_ogg_msg_t *qtk_engine_ogg_msg_new(qtk_engine_ogg_t *ogg) {
    qtk_engine_ogg_msg_t *msg;

    msg = (qtk_engine_ogg_msg_t *)wtk_malloc(sizeof(qtk_engine_ogg_msg_t));
    msg->buf = wtk_strbuf_new(3200, 1);
    return msg;
}

static int qtk_engine_ogg_msg_delete(qtk_engine_ogg_msg_t *msg) {
    wtk_strbuf_delete(msg->buf);
    wtk_free(msg);
    return 0;
}

static qtk_engine_ogg_msg_t *qtk_engine_ogg_pop_msg(qtk_engine_ogg_t *ogg) {
    qtk_engine_ogg_msg_t *msg;

    msg = (qtk_engine_ogg_msg_t *)wtk_lockhoard_pop(&(ogg->msg_hoard));
    wtk_strbuf_reset(msg->buf);
    return msg;
}

static void qtk_engine_ogg_push_msg(qtk_engine_ogg_t *ogg,
                                    qtk_engine_ogg_msg_t *msg) {
    wtk_lockhoard_push(&(ogg->msg_hoard), msg);
}

static int qtk_engine_ogg_run(qtk_engine_ogg_t *ogg, wtk_thread_t *thread) {
    wtk_queue_node_t *qn;
    qtk_engine_ogg_msg_t *msg;

    while (ogg->run) {
        qn = wtk_blockqueue_pop(&ogg->input_q, -1, NULL);
        if (!qn) {
            break;
        }

        msg = data_offset2(qn, qtk_engine_ogg_msg_t, q_n);
        switch (msg->type) {
        case QTK_ENGINE_OGG_START:
            if (ogg->notify_func) {
                ogg->notify_func(ogg->notify_ths, QTK_ENGINE_OGG_START, NULL,
                                 0);
            }
            qtk_oggenc_start(ogg->ins.ogg, msg->rate, msg->channel, msg->bits);
            break;
        case QTK_ENGINE_OGG_DATA:
            qtk_oggenc_encode(ogg->ins.ogg, msg->buf->data, msg->buf->pos, 0);
            break;
        case QTK_ENGINE_OGG_END:
            qtk_oggenc_encode(ogg->ins.ogg, msg->buf->data, msg->buf->pos, 1);
            if (ogg->notify_func) {
                ogg->notify_func(ogg->notify_ths, QTK_ENGINE_OGG_END, NULL, 0);
            }
            qtk_oggenc_reset(ogg->ins.ogg);
            break;
        case QTK_ENGINE_OGG_CANCEL:
            qtk_oggenc_reset(ogg->ins.ogg);
            ogg->cancel = 0;
            break;
        }

        qtk_engine_ogg_push_msg(ogg, msg);
    }

    return 0;
}

static void qtk_engogg_on_enc(qtk_engine_ogg_t *ogg, char *data, int bytes) {
    if (ogg->cancel) {
        return;
    }
    ogg->notify_func(ogg->notify_ths, QTK_ENGINE_OGG_DATA, data, bytes);
}

qtk_engine_ogg_t *qtk_engine_ogg_new() {
    qtk_engine_ogg_t *ogg;

    ogg = (qtk_engine_ogg_t *)wtk_malloc(sizeof(*ogg));
    qtk_oggenc_cfg_init(&ogg->ins.cfg);
    qtk_oggenc_cfg_update(&ogg->ins.cfg);

    ogg->ins.ogg = qtk_oggenc_new(&ogg->ins.cfg);
    qtk_oggenc_set_write(ogg->ins.ogg, ogg,
                         (qtk_oggenc_write_f)qtk_engogg_on_enc);

    wtk_blockqueue_init(&(ogg->input_q));
    wtk_lockhoard_init(&ogg->msg_hoard, offsetof(qtk_engine_ogg_msg_t, hoard_n),
                       10, (wtk_new_handler_t)qtk_engine_ogg_msg_new,
                       (wtk_delete_handler_t)qtk_engine_ogg_msg_delete, ogg);

    ogg->cancel = 0;
    ogg->run = 1;
    wtk_thread_init(&ogg->thread, (thread_route_handler)qtk_engine_ogg_run,
                    ogg);
    wtk_thread_set_name(&ogg->thread, "engine_ogg");
    wtk_thread_start(&ogg->thread);

    ogg->notify_func = NULL;
    ogg->notify_ths = NULL;

    return ogg;
}

void qtk_engine_ogg_delete(qtk_engine_ogg_t *ogg) {
    ogg->run = 0;
    wtk_blockqueue_wake(&ogg->input_q);
    wtk_thread_join(&ogg->thread);
    wtk_thread_clean(&ogg->thread);

    wtk_blockqueue_clean(&ogg->input_q);
    wtk_lockhoard_clean(&ogg->msg_hoard);

    qtk_oggenc_delete(ogg->ins.ogg);
    qtk_oggenc_cfg_clean(&ogg->ins.cfg);

    wtk_free(ogg);
}

void qtk_engine_ogg_set_notify(qtk_engine_ogg_t *ogg, void *notify_ths,
                               qtk_engine_ogg_notify_func notify_func) {
    ogg->notify_func = notify_func;
    ogg->notify_ths = notify_ths;
}

void qtk_engine_ogg_start(qtk_engine_ogg_t *ogg, int rate, int channels,
                          int bits) {
    qtk_engine_ogg_msg_t *msg;

    msg = qtk_engine_ogg_pop_msg(ogg);
    msg->type = QTK_ENGINE_OGG_START;
    msg->rate = rate;
    msg->channel = channels;
    msg->bits = bits;
    wtk_blockqueue_push(&ogg->input_q, &msg->q_n);
}

void qtk_engine_ogg_feed(qtk_engine_ogg_t *ogg, char *data, int len,
                         int is_end) {
    qtk_engine_ogg_msg_t *msg;

    msg = qtk_engine_ogg_pop_msg(ogg);
    msg->type = is_end ? QTK_ENGINE_OGG_END : QTK_ENGINE_OGG_DATA;
    wtk_strbuf_push(msg->buf, data, len);
    wtk_blockqueue_push(&(ogg->input_q), &(msg->q_n));
}

void qtk_engine_ogg_cancel(qtk_engine_ogg_t *ogg) {
    qtk_engine_ogg_msg_t *msg;

    ogg->cancel = 1;
    msg = qtk_engine_ogg_pop_msg(ogg);
    msg->type = QTK_ENGINE_OGG_CANCEL;
    wtk_blockqueue_push(&ogg->input_q, &msg->q_n);
}
