#include "qtk_ecsr.h"
#include "sdk/codec/qtk_audio_conversion.h"

static void qtk_ecsr_on_notify(qtk_ecsr_t *e, qtk_var_t *var) {
    if (e->cancel) {
        wtk_log_log(e->session->log, "cancel var type %d", var->type);
        return;
    }
    e->notify_func(e->notify_ths, var);
}

void qtk_ecsr_init(qtk_ecsr_t *e) {
    qtk_engine_param_init(&e->param);

    e->session = NULL;

    e->cfg = NULL;
    e->c = NULL;

    e->thread = NULL;
    e->callback = NULL;

    e->notify_func = NULL;
    e->notify_ths = NULL;

    e->cancel = 0;
}

int qtk_ecsr_on_start(qtk_ecsr_t *e);
int qtk_ecsr_on_feed(qtk_ecsr_t *e, char *data, int len);
int qtk_ecsr_on_end(qtk_ecsr_t *e);
void qtk_ecsr_on_reset(qtk_ecsr_t *e);
void qtk_ecsr_on_set_notify(qtk_ecsr_t *e, void *notify_ths,
                            qtk_engine_notify_f notify_f);
void qtk_ecsr_on_cancel(qtk_ecsr_t *e);

qtk_ecsr_t *qtk_ecsr_new(qtk_session_t *session, wtk_local_cfg_t *params) {
    qtk_ecsr_t *e;
    int buf_size;
    int ret;
    // int i,flag;

    e = (qtk_ecsr_t *)wtk_malloc(sizeof(qtk_ecsr_t));
    qtk_ecsr_init(e);

    e->session = session;

    qtk_engine_param_set_session(&e->param, e->session);
    ret = qtk_engine_param_feed(&e->param, params);
    if (ret != 0) {
        wtk_log_warn0(e->session->log, "params als failed.");
        goto end;
    }

    if (e->param.use_bin) {
        e->cfg = qtk_csr_cfg_new_bin(e->param.cfg);
    } else {
        e->cfg = qtk_csr_cfg_new(e->param.cfg);
    }
    if (!e->cfg) {
        wtk_log_warn0(e->session->log, "cfg new failed.");
        _qtk_error(e->session, _QTK_CFG_NEW_FAILED);
        ret = -1;
        goto end;
    }

    /**
     * rec_min_conf
     * gr_min_conf
     * usrec_min_conf
     * use_json
     * env
     * usr_ebnf
     */
    qtk_csr_cfg_update_params(e->cfg, params);
    qtk_csr_cfg_update_option(e->cfg, &session->opt);

    e->c = qtk_csr_new(e->cfg, session);
    if (!e->c) {
        wtk_log_log0(e->session->log, "csr new failed.");
        _qtk_error(e->session, _QTK_INSTANSE_NEW_FAILED);
        ret = -1;
        goto end;
    }
    qtk_csr_set_notify(e->c, e, (qtk_engine_notify_f)qtk_ecsr_on_notify);
    
    if (e->param.use_thread) {
        buf_size = 32 * e->param.winStep;

        e->callback = qtk_engine_callback_new();
        e->callback->start_f = (qtk_engine_thread_start_f)qtk_ecsr_on_start;
        e->callback->data_f = (qtk_engine_thread_data_f)qtk_ecsr_on_feed;
        e->callback->end_f = (qtk_engine_thread_end_f)qtk_ecsr_on_end;
        e->callback->reset_f = (qtk_engine_thread_reset_f)qtk_ecsr_on_reset;
        e->callback->set_notify_f =
            (qtk_engine_thread_set_notify_f)qtk_ecsr_on_set_notify;
        e->callback->cancel_f = (qtk_engine_thread_cancel_f)qtk_ecsr_on_cancel;
        e->callback->ths = e;

        e->thread = qtk_engine_thread_new(e->callback, e->session->log, "ecsr",
                                          buf_size, 20, 1, e->param.syn);
    }

    ret = 0;
end:
    wtk_log_log(e->session->log, "ret = %d.", ret);
    if (ret != 0) {
        qtk_ecsr_delete(e);
        e = NULL;
    }
    return e;
}

int qtk_ecsr_delete(qtk_ecsr_t *e) {
    if (e->thread) {
        qtk_engine_thread_delete(e->thread, 0);
    }
    if (e->callback) {
        qtk_engine_callback_delete(e->callback);
    }
    if (e->c) {
        qtk_csr_delete(e->c);
    }
    if (e->cfg) {
        if (e->param.use_bin) {
            qtk_csr_cfg_delete_bin(e->cfg);
        } else {
            qtk_csr_cfg_delete(e->cfg);
        }
    }
    wtk_free(e);
    return 0;
}

int qtk_ecsr_on_start(qtk_ecsr_t *e) {
    e->end_flag = 0;
    return qtk_csr_start(e->c, e->param.left_margin, e->param.right_margin);
}

int qtk_ecsr_on_feed(qtk_ecsr_t *e, char *data, int len) {
    return qtk_csr_feed(e->c, data, len, 0);
}

int qtk_ecsr_on_end(qtk_ecsr_t *e) {
    e->end_flag = 1;
    return qtk_csr_feed(e->c, 0, 0, 1);
}

void qtk_ecsr_on_reset(qtk_ecsr_t *e) {
    if (e->end_flag == 0) {
        qtk_ecsr_on_end(e);
    }
    qtk_csr_reset(e->c);
}

void qtk_ecsr_on_set_notify(qtk_ecsr_t *e, void *notify_ths,
                            qtk_engine_notify_f notify_f) {
    e->notify_ths = notify_ths;
    e->notify_func = notify_f;
}

void qtk_ecsr_on_cancel(qtk_ecsr_t *e) {
    qtk_csr_cancel(e->c);
    e->cancel = 0;
}

int qtk_ecsr_start(qtk_ecsr_t *e) {
    if (e->param.use_thread) {
        qtk_engine_thread_start(e->thread);
    } else {
        qtk_ecsr_on_start(e);
    }
    return 0;
}

int qtk_ecsr_reset(qtk_ecsr_t *e) {
    if (e->param.use_thread) {
        qtk_engine_thread_reset(e->thread);
    } else {
        qtk_ecsr_on_reset(e);
    }
    return 0;
}

int qtk_ecsr_feed(qtk_ecsr_t *e, char *data, int len, int is_end) {
    if(e->param.mic_shift != 1.0)
    {
        qtk_data_change_vol(data, len, e->param.mic_shift);
    }
    if (e->param.use_thread) {
        qtk_engine_thread_feed(e->thread, data, len, is_end);
    } else {
        if (len > 0) {
            qtk_ecsr_on_feed(e, data, len);
        }
        if (is_end) {
            qtk_ecsr_on_end(e);
        }
    }
    return 0;
}

int qtk_ecsr_cancel(qtk_ecsr_t *e) {
    ++e->cancel;
    if (e->param.use_thread) {
        qtk_engine_thread_cancel(e->thread);
    } else {
        qtk_ecsr_on_cancel(e);
    }

    return 0;
}

void qtk_ecsr_set_notify(qtk_ecsr_t *e, void *ths, qtk_engine_notify_f notify) {
    if (e->param.use_thread) {
        qtk_engine_thread_set_notify(e->thread, ths, notify);
    } else {
        qtk_ecsr_on_set_notify(e, ths, notify);
    }
}

int qtk_ecsr_set(qtk_ecsr_t *e, char *data, int bytes){
    //直播demo新的res只在cn.asr.rec下才生效
    qtk_asr_set_coreType(e->c->asr,"cn.asr.rec",strlen("cn.asr.rec"));
    qtk_asr_set_res(e->c->asr,data,bytes);
}
