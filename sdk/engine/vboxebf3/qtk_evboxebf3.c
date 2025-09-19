/*
 * qtk_evboxebf3.c
 *
 *  Created on: Jul 7, 2023
 *      Author: root
 */

#include "qtk_evboxebf3.h"
#include "sdk/engine/comm/qtk_engine_hdr.h"

void qtk_evboxebf3_init(qtk_evboxebf3_t *e) {
    qtk_engine_param_init(&e->param);

    e->session = NULL;

    e->cfg = NULL;
    e->b = NULL;

    e->notify = NULL;
    e->notify_ths = NULL;

    e->thread = NULL;
    e->callback = NULL;
}

int qtk_evboxebf3_on_start(qtk_evboxebf3_t *e);
int qtk_evboxebf3_on_feed(qtk_evboxebf3_t *e, char *data, int len);
int qtk_evboxebf3_on_end(qtk_evboxebf3_t *e);
void qtk_evboxebf3_on_reset(qtk_evboxebf3_t *e);
void qtk_evboxebf3_on_set_notify(qtk_evboxebf3_t *e, void *notify_ths,
                             qtk_engine_notify_f notify_f);
int qtk_evboxebf3_on_set(qtk_evboxebf3_t *e, char *data, int bytes);
void qtk_evboxebf3_on_vboxebf(qtk_evboxebf3_t *bfio, short *data, int len);

qtk_evboxebf3_t *qtk_evboxebf3_new(qtk_session_t *session, wtk_local_cfg_t *params) {
    qtk_evboxebf3_t *e;
    int buf_size;
    int ret;
    int i;

    e = (qtk_evboxebf3_t *)wtk_malloc(sizeof(qtk_evboxebf3_t));
    qtk_evboxebf3_init(e);
    e->session = session;

    qtk_engine_param_set_session(&e->param, e->session);
    ret = qtk_engine_param_feed(&e->param, params);
    if (ret != 0) {
        wtk_log_warn0(e->session->log, "params als failed.");
        goto end;
    }

    if (e->param.use_bin) {
        e->cfg = wtk_vboxebf3_cfg_new_bin(e->param.cfg);
    } else {
        e->cfg = wtk_vboxebf3_cfg_new(e->param.cfg);
    }
    if (!e->cfg) {
        wtk_log_warn0(e->session->log, "cfg new failed.");
        _qtk_error(e->session, _QTK_CFG_NEW_FAILED);
        ret = -1;
        goto end;
    }
    e->b = wtk_vboxebf3_new(e->cfg);
    if (!e->b) {
        wtk_log_err0(e->session->log, "vboxebf3 new failed.");
        ret = -1;
        goto end;
    }
    wtk_vboxebf3_set_notify(e->b, e, (wtk_vboxebf3_notify_f)qtk_evboxebf3_on_vboxebf);
//    e->channel = e->b->cfg->channel + 1;           //?
//    wtk_debug(">>>>channel = %d\n", e->channel);
//    e->buffer = (short **)wtk_malloc(sizeof(short *) * e->channel);
//    for (i = 0; i < e->channel; i++) {
//        e->buffer[i] = (short *)wtk_malloc(sizeof(short *) *
//                                           QTK_BFIO_FEED_STEP * e->channel);
//    }
    if (e->param.use_thread) {
//        buf_size = (e->channel) * 2 * 16 * e->param.winStep;
    	buf_size = 256;

        e->callback = qtk_engine_callback_new();
        e->callback->start_f = (qtk_engine_thread_start_f)qtk_evboxebf3_on_start;
        e->callback->data_f = (qtk_engine_thread_data_f)qtk_evboxebf3_on_feed;
        e->callback->end_f = (qtk_engine_thread_end_f)qtk_evboxebf3_on_end;
        e->callback->reset_f = (qtk_engine_thread_reset_f)qtk_evboxebf3_on_reset;
        e->callback->set_notify_f =
            (qtk_engine_thread_set_notify_f)qtk_evboxebf3_on_set_notify;
        e->callback->set_f = (qtk_engine_thread_set_f)qtk_evboxebf3_on_set;
        e->callback->ths = e;

        e->thread = qtk_engine_thread_new(e->callback, e->session->log, "evboxebf",
                                          buf_size, 20, 1, e->param.syn);
    }

    ret = 0;
end:
    wtk_log_log(e->session->log, "ret = %d", ret);
    if (ret != 0) {
        qtk_evboxebf3_delete(e);
        e = NULL;
    }
    return e;
}

int qtk_evboxebf3_delete(qtk_evboxebf3_t *e) {
    if (e->thread) {
        qtk_engine_thread_delete(e->thread, 1);
    }
    if (e->callback) {
        qtk_engine_callback_delete(e->callback);
    }
    if (e->b) {
        wtk_vboxebf3_delete(e->b);
        e->b = NULL;
    }
    if (e->cfg) {
        if (e->param.use_bin) {
            wtk_vboxebf3_cfg_delete_bin(e->cfg);
        } else {
            wtk_vboxebf3_cfg_delete(e->cfg);
        }
        e->cfg = NULL;
    }
    qtk_engine_param_clean(&e->param);

    wtk_free(e);
    return 0;
}

int qtk_evboxebf3_on_start(qtk_evboxebf3_t *e) {
    wtk_vboxebf3_start(e->b);
    return 0;
}

int qtk_evboxebf3_on_feed(qtk_evboxebf3_t *e, char *data, int bytes) {

    if (bytes > 0) {
    	wtk_vboxebf3_feed(e->b, data, bytes, 0);
    }

    return 0;
}

int qtk_evboxebf3_on_end(qtk_evboxebf3_t *e) {
    wtk_vboxebf3_feed(e->b, NULL, 0, 1);
    e->feedend = 1;
    return 0;
}

void qtk_evboxebf3_on_reset(qtk_evboxebf3_t *e) {
    if (e->feedend == 0) {
        qtk_evboxebf3_on_end(e);
    }
    wtk_vboxebf3_reset(e->b);
}

void qtk_evboxebf3_on_set_notify(qtk_evboxebf3_t *e, void *notify_ths,
                             qtk_engine_notify_f notify_f) {
    e->notify_ths = notify_ths;
    e->notify = notify_f;
}

int qtk_evboxebf3_on_set(qtk_evboxebf3_t *e, char *data, int bytes) { return 0; }

int qtk_evboxebf3_start(qtk_evboxebf3_t *e) {
    int ret;

    if (e->param.use_thread) {
        qtk_engine_thread_start(e->thread);
    } else {
        qtk_evboxebf3_on_start(e);
    }
    ret = 0;
    return ret;
}

int qtk_evboxebf3_feed(qtk_evboxebf3_t *e, char *data, int bytes, int is_end) {
    if (e->param.use_thread) {
        qtk_engine_thread_feed(e->thread, data, bytes, is_end);
    } else {
        if (bytes > 0) {
            qtk_evboxebf3_on_feed(e, data, bytes);
        }
        if (is_end) {
            qtk_evboxebf3_on_end(e);
        }
    }
    return 0;
}

int qtk_evboxebf3_reset(qtk_evboxebf3_t *e) {
    if (e->param.use_thread) {
        qtk_engine_thread_reset(e->thread);
    } else {
        qtk_evboxebf3_on_reset(e);
    }

    return 0;
}

int qtk_evboxebf3_cancel(qtk_evboxebf3_t *e) {
    if (e->param.use_thread) {
        qtk_engine_thread_cancel(e->thread);
    }
    return 0;
}

void qtk_evboxebf3_set_notify(qtk_evboxebf3_t *e, void *ths,
                          qtk_engine_notify_f notify_f) {
    if (e->param.use_thread) {
        qtk_engine_thread_set_notify(e->thread, ths, notify_f);
    } else {
        qtk_evboxebf3_on_set_notify(e, ths, notify_f);
    }
}

int qtk_evboxebf3_set(qtk_evboxebf3_t *e, char *data, int bytes) {
    int ret;

    if (e->param.use_thread) {
        qtk_engine_thread_set(e->thread, data, bytes);
    } else {
        ret = qtk_evboxebf3_on_set(e, data, bytes);
    }
    return ret;
}

void qtk_evboxebf3_on_vboxebf(qtk_evboxebf3_t *bfio, short *data,
                       int len) {
    qtk_var_t var;

    var.type = QTK_SPEECH_DATA_PCM;
    var.v.str.data = data;
    var.v.str.len = len;
    if (bfio->notify) {
        bfio->notify(bfio->notify_ths, &var);
    }
}



