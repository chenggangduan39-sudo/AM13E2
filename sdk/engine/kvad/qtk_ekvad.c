#include "qtk_ekvad.h"
#ifndef USE_KFRAME
#ifdef USE_TIME_TEST
static long count1 = 0;
static long count2 = 0;
#endif
#endif
static void qtk_ekvad_notify(qtk_ekvad_t *e, int type, char *data, int bytes) {
    qtk_var_t v;

    if (type < 0) {
        return;
    }

    if (e->cancel) {
        wtk_log_log(e->session->log, "cancel vad type %d", type);
        return;
    }

    if (!e->notify) {
        wtk_log_warn0(e->session->log, "notify not set");
        _qtk_warning(e->session, _QTK_ENGINE_NOTIFY_INVALID);
        return;
    }

    switch (type) {
    case qtk_ekvad_SPEECH_START:
        v.type = QTK_SPEECH_START;
        break;
    case qtk_ekvad_SPEECH_DATA_OGG:
        v.type = QTK_SPEECH_DATA_OGG;
        v.v.str.data = data;
        v.v.str.len = bytes;
        break;
    case qtk_ekvad_SPEECH_DATA_PCM:
        v.type = QTK_SPEECH_DATA_PCM;
        v.v.str.data = data;
        v.v.str.len = bytes;
        break;
    case qtk_ekvad_SPEECH_END:
        v.type = QTK_SPEECH_END;
        break;
    }

    e->notify(e->notify_ths, &(v));
}

static void qtk_ekvad_on_ogg(qtk_ekvad_t *e, int type, char *data, int bytes) {
    int tmp = -1;

    switch (type) {
    case QTK_ENGINE_OGG_START:
        tmp = qtk_ekvad_SPEECH_START;
        break;
    case QTK_ENGINE_OGG_DATA:
        tmp = qtk_ekvad_SPEECH_DATA_OGG;
        break;
    case QTK_ENGINE_OGG_END:
        tmp = qtk_ekvad_SPEECH_END;
        break;
    }
    qtk_ekvad_notify(e, tmp, data, bytes);
}

void qtk_ekvad_init(qtk_ekvad_t *e) {
    qtk_engine_param_init(&e->param);
    e->session = NULL;
    e->mbin_cfg = NULL;
    e->main_cfg = NULL;
    e->cfg = NULL;
    e->v = NULL;
    e->notify = NULL;
    e->notify_ths = NULL;
    e->thread = NULL;
    e->callback = NULL;
    e->ogg = NULL;
    wtk_queue_init(&(e->vad_q));
    e->cancel = 0;
}

int qtk_ekvad_on_start(qtk_ekvad_t *e);
int qtk_ekvad_on_feed(qtk_ekvad_t *e, char *data, int bytes);
int qtk_ekvad_on_feedend(qtk_ekvad_t *e);
void qtk_ekvad_on_reset(qtk_ekvad_t *e);
void qtk_ekvad_on_set_notify(qtk_ekvad_t *e, void *notify_ths,
                             qtk_engine_notify_f notify_f);
void qtk_ekvad_on_cancel(qtk_ekvad_t *e);

qtk_ekvad_t *qtk_ekvad_new(qtk_session_t *session, wtk_local_cfg_t *params) {
    qtk_ekvad_t *e;
    int buf_size;
    int ret = 0;

    e = (qtk_ekvad_t *)wtk_malloc(sizeof(qtk_ekvad_t));
    qtk_ekvad_init(e);
    e->session = session;

    qtk_engine_param_set_session(&e->param, session);
    ret = qtk_engine_param_feed(&e->param, params);
    if (ret != 0) {
        wtk_log_warn0(e->session->log, "params als failed.");
        goto end;
    }

    if (e->param.use_bin) {
        e->mbin_cfg =
            wtk_mbin_cfg_new_type(wtk_kvad_cfg, e->param.cfg, "./kvad.cfg");
        if (!e->mbin_cfg) {
            wtk_log_warn0(e->session->log, "cfg new failed.");
            _qtk_error(e->session, _QTK_CFG_NEW_FAILED);
            ret = -1;
            goto end;
        }
        e->cfg = (wtk_kvad_cfg_t *)e->mbin_cfg->cfg;
    } else {
        e->main_cfg = wtk_main_cfg_new_type(wtk_kvad_cfg, e->param.cfg);
        if (!e->main_cfg) {
            wtk_log_warn0(e->session->log, "cfg new failed.");
            _qtk_error(e->session, _QTK_CFG_NEW_FAILED);
            ret = -1;
            goto end;
        }
        e->cfg = (wtk_kvad_cfg_t *)e->main_cfg->cfg;
    }

    e->v = wtk_kvad_new(e->cfg);
    if (!e->v) {
        wtk_log_warn0(e->session->log, "vad new failed.");
        _qtk_error(e->session, _QTK_INSTANSE_NEW_FAILED);
        ret = -1;
        goto end;
    }

    if (e->param.use_thread) {
        buf_size = 32 * e->param.winStep;

        e->callback = qtk_engine_callback_new();
        e->callback->start_f = (qtk_engine_thread_start_f)qtk_ekvad_on_start;
        e->callback->data_f = (qtk_engine_thread_data_f)qtk_ekvad_on_feed;
        e->callback->end_f = (qtk_engine_thread_end_f)qtk_ekvad_on_feedend;
        e->callback->reset_f = (qtk_engine_thread_reset_f)qtk_ekvad_on_reset;
        e->callback->set_notify_f =
            (qtk_engine_thread_set_notify_f)qtk_ekvad_on_set_notify;
        e->callback->cancel_f = (qtk_engine_thread_cancel_f)qtk_ekvad_on_cancel;
        e->callback->ths = e;

        e->thread = qtk_engine_thread_new(e->callback, e->session->log, "evad",
                                          buf_size, 20, 1, e->param.syn);
    }

    if (e->param.output_ogg) {
        e->ogg = qtk_engine_ogg_new();
        qtk_engine_ogg_set_notify(e->ogg, e,
                                  (qtk_engine_ogg_notify_func)qtk_ekvad_on_ogg);
    }

    ret = 0;
end:
    if (ret != 0) {
        qtk_ekvad_delete(e);
        e = NULL;
    }
    return e;
}

int qtk_ekvad_on_start(qtk_ekvad_t *e) {
    e->sil = 1;
    // wtk_debug("%d  %d\n", e->param.left_margin, e->param.right_margin);
    wtk_kvad_set_margin(e->v, e->param.left_margin, e->param.right_margin);
    wtk_kvad_start(e->v);
    return 0;
}
#ifdef USE_KFRAME
void qtk_ekvad_process(qtk_ekvad_t *e, char *data, int bytes, int is_end) {
    wtk_queue_t *vad_q = &(e->v->output_q);
    wtk_kframe_t *f;
    wtk_queue_node_t *qn;

    wtk_kvad_feed(e->v, (short *)data, bytes / 2, is_end);
    while (1) {
        qn = wtk_queue_pop(vad_q);
        if (!qn) {
            break;
        }
        f = data_offset2(qn, wtk_kframe_t, q_n);
        if (e->sil) {
            if (f->speech == 1) {
                // wtk_debug(">>>>>>>>>>>>.start\n");
                e->ogg ? qtk_engine_ogg_start(e->ogg, 16000, 1, 16)
                       : qtk_ekvad_notify(e, qtk_ekvad_SPEECH_START, NULL, 0);
                if (e->ogg) {
                    qtk_engine_ogg_feed(e->ogg, (char *)f->wav,
                                        e->v->parm->parm->cfg->frame_step * 2,
                                        0);
                }
                qtk_ekvad_notify(e, qtk_ekvad_SPEECH_DATA_PCM, (char *)f->wav,
                                 e->v->parm->parm->cfg->frame_step * 2);
                e->sil = 0;
            }
        } else {
            if (f->speech == 0) {
                e->ogg ? qtk_engine_ogg_feed(e->ogg, NULL, 0, 1)
                       : qtk_ekvad_notify(e, qtk_ekvad_SPEECH_END, NULL, 0);
                e->sil = 1;
            } else {
                if (e->ogg) {
                    qtk_engine_ogg_feed(e->ogg, (char *)f->wav,
                                        e->v->parm->parm->cfg->frame_step * 2,
                                        0);
                }
                qtk_ekvad_notify(e, qtk_ekvad_SPEECH_DATA_PCM, (char *)f->wav,
                                 e->v->parm->parm->cfg->frame_step * 2);
            }
        }
        // wtk_debug(">>>>>speech = %d\n", f->speech);
        wtk_kvad_push_frame(e->v, f);
    }
    if (is_end && !e->sil) {
        e->ogg ? qtk_engine_ogg_feed(e->ogg, NULL, 0, 1)
               : qtk_ekvad_notify(e, qtk_ekvad_SPEECH_END, NULL, 0);
    }
}
#else
void qtk_ekvad_process(qtk_ekvad_t *e, char *data, int bytes, int is_end) {
    wtk_queue_t *vad_q = &(e->v->output_q);
    wtk_vframe_t *f;
    wtk_queue_node_t *qn;

    wtk_kvad_feed(e->v, (short *)data, bytes / 2, is_end);
#ifdef USE_TIME_TEST
    count1 += bytes;
#endif
    while (1) {
        qn = wtk_queue_pop(vad_q);
        if (!qn) {
            break;
        }
        f = data_offset2(qn, wtk_vframe_t, q_n);
        if (e->cancel) {
            wtk_kvad_push_frame(e->v, f);
            wtk_log_log(e->session->log, "cancel %d skip", e->cancel);
            continue;
        }
#ifdef USE_TIME_TEST
        count2 += f->frame_step * 2;
#endif
        if (e->sil) {
            if (f->state == wtk_vframe_speech) {
                wtk_debug(">>>>>>>>>>>>.start\n");
                // wtk_debug("========>goto speech
                // time=%.4fs\n",e->counter*1.0/16000);
                e->ogg ? qtk_engine_ogg_start(e->ogg, 16000, 1, 16)
                       : qtk_ekvad_notify(e, qtk_ekvad_SPEECH_START, NULL, 0);
                if (e->ogg) {
                    qtk_engine_ogg_feed(e->ogg, (char *)f->wav_data,
                                        f->frame_step * 2, 0);
                }
                qtk_ekvad_notify(e, qtk_ekvad_SPEECH_DATA_PCM,
                                 (char *)f->wav_data, f->frame_step * 2);
                e->sil = 0;
            }
        } else {
            if (f->state == wtk_vframe_sil) {
                e->ogg ? qtk_engine_ogg_feed(e->ogg, NULL, 0, 1)
                       : qtk_ekvad_notify(e, qtk_ekvad_SPEECH_END, NULL, 0);
                e->sil = 1;
#ifdef USE_TIME_TEST
                wtk_log_log(e->session->log, "===>vad time delay:%ld\n",
                            (count1 - count2) / 32);
#endif
            } else {
                if (e->ogg) {
                    qtk_engine_ogg_feed(e->ogg, (char *)f->wav_data,
                                        f->frame_step * 2, 0);
                }
                qtk_ekvad_notify(e, qtk_ekvad_SPEECH_DATA_PCM,
                                 (char *)f->wav_data, f->frame_step * 2);
            }
        }
        wtk_kvad_push_frame(e->v, f);
    }
    if (is_end && !e->sil) {
        e->ogg ? qtk_engine_ogg_feed(e->ogg, NULL, 0, 1)
               : qtk_ekvad_notify(e, qtk_ekvad_SPEECH_END, NULL, 0);
#ifdef USE_TIME_TEST
        wtk_log_log(e->session->log, "===>vad time delay:%ld\n",
                    (count1 - count2) / 32);
#endif
    }
}
#endif
int qtk_ekvad_on_feed(qtk_ekvad_t *e, char *data, int len) {
    qtk_ekvad_process(e, data, len, 0);
    return 0;
}

int qtk_ekvad_on_feedend(qtk_ekvad_t *e) {
    qtk_ekvad_process(e, 0, 0, 1);
    return 0;
}

void qtk_ekvad_on_reset(qtk_ekvad_t *e) {
    wtk_kvad_reset(e->v);
    e->sil = 1;
}

void qtk_ekvad_on_set_notify(qtk_ekvad_t *e, void *notify_ths,
                             qtk_engine_notify_f notify_f) {
    e->notify = notify_f;
    e->notify_ths = notify_ths;
}

void qtk_ekvad_on_cancel(qtk_ekvad_t *e) {
    if (e->ogg) {
        qtk_engine_ogg_cancel(e->ogg);
    }
    --e->cancel;
}

int qtk_ekvad_start(qtk_ekvad_t *e) {
    if (e->param.use_thread) {
        qtk_engine_thread_start(e->thread);
    } else {
        qtk_ekvad_on_start(e);
    }
    return 0;
}

int qtk_ekvad_feed(qtk_ekvad_t *e, char *data, int bytes, int is_end) {
    if (e->param.use_thread) {
        qtk_engine_thread_feed(e->thread, data, bytes, is_end);
    } else {
        qtk_ekvad_process(e, data, bytes, is_end);
    }
    return 0;
}

int qtk_ekvad_reset(qtk_ekvad_t *e) {
    if (e->param.use_thread) {
        qtk_engine_thread_reset(e->thread);
    } else {
        qtk_ekvad_on_reset(e);
    }
    return 0;
}

int qtk_ekvad_cancel(qtk_ekvad_t *e) {
    ++e->cancel;
    if (e->thread) {
        qtk_engine_thread_cancel(e->thread);
    } else {
        qtk_ekvad_on_cancel(e);
    }
    return 0;
}

int qtk_ekvad_delete(qtk_ekvad_t *e) {
    if (e->thread) {
        qtk_engine_thread_delete(e->thread, 0);
    }
    if (e->callback) {
        qtk_engine_callback_delete(e->callback);
    }
    if (e->ogg) {
        qtk_engine_ogg_delete(e->ogg);
    }

    if (e->v) {
        wtk_kvad_delete(e->v);
    }
    if (e->param.use_bin) {
        if (e->mbin_cfg) {
            wtk_mbin_cfg_delete(e->mbin_cfg);
        }
    } else {
        if (e->main_cfg) {
            wtk_main_cfg_delete(e->main_cfg);
        }
    }

    qtk_engine_param_clean(&e->param);
    wtk_free(e);
    return 0;
}

void qtk_ekvad_set_notify(qtk_ekvad_t *e, void *notify_ths,
                          qtk_engine_notify_f notify) {
    if (e->param.use_thread) {
        qtk_engine_thread_set_notify(e->thread, notify_ths, notify);
    } else {
        qtk_ekvad_on_set_notify(e, notify_ths, notify);
    }
}

int qtk_ekvad_set(qtk_ekvad_t *e, char *data, int bytes) { return 0; }
