#include "qtk_eqkws.h"
#include "sdk/engine/comm/qtk_engine_hdr.h"
#include "sdk/codec/qtk_audio_conversion.h"

void qtk_eqkws_init(qtk_eqkws_t *e)
{
    qtk_engine_param_init(&e->param);

    e->session = NULL;

    e->cfg = NULL;
    e->qkws = NULL;

    e->notify_f = NULL;
    e->notify_ths = NULL;

    e->thread = NULL;
    e->callback = NULL;
    e->enrollfn = NULL;
    e->outbuf = NULL;
    e->cancel = 0;
}

int qtk_eqkws_on_start(qtk_eqkws_t *e);
int qtk_eqkws_on_data(qtk_eqkws_t *e, char *data, int bytes);
int qtk_eqkws_on_end(qtk_eqkws_t *e);
void qtk_eqkws_on_reset(qtk_eqkws_t *e);
void qtk_eqkws_on_set_notify(qtk_eqkws_t *e, void *notify_ths,
                             qtk_engine_notify_f notify_f);
void qtk_eqkws_on_cancel(qtk_eqkws_t *e);
void qtk_eqkws_on_set(qtk_eqkws_t *e, char *data, int bytes);
void qtk_eqkws_on_err(qtk_eqkws_t *e);
void qtk_eqkws_on_kws(qtk_eqkws_t *e, qtk_var_t *data);

qtk_eqkws_t *qtk_eqkws_new(qtk_session_t *session, wtk_local_cfg_t *params)
{
    qtk_eqkws_t *e;
    int buf_size;
    int ret;

    e = (qtk_eqkws_t *)wtk_malloc(sizeof(qtk_eqkws_t));
    qtk_eqkws_init(e);

    e->session = session;

    e->enrollfn = (char *)wtk_malloc(128);
    memset(e->enrollfn, 0, 128);

    qtk_engine_param_set_session(&e->param, e->session);
    ret = qtk_engine_param_feed(&e->param, params);
    if (ret != 0) {
        wtk_log_warn0(e->session->log, "params als failed.");
        goto end;
    }

    if (e->param.use_bin) {
        e->cfg = qtk_qkws_cfg_new_bin(e->param.cfg);
    } else {
        e->cfg = qtk_qkws_cfg_new(e->param.cfg);
    }
    if (!e->cfg) {
        wtk_log_warn0(e->session->log, "cfg new failed.");
        _qtk_error(e->session, _QTK_CFG_NEW_FAILED);
        ret = -1;
        goto end;
    }

    e->qkws = qtk_qkws_new(e->cfg, session);
    if (!e->qkws) {
        wtk_log_warn0(e->session->log, "qkws new failed.");
        _qtk_error(e->session, _QTK_INSTANSE_NEW_FAILED);
        ret = -1;
        goto end;
    }
    qtk_qkws_set_notify(e->qkws, e, (qtk_engine_notify_f)qtk_eqkws_on_kws);
    wtk_debug("==============outchannel=%d\n",e->param.out_channel);
    if(e->param.out_channel > 1){
        e->outbuf = wtk_strbuf_new(1024*e->param.out_channel, 1.0f);
        wtk_strbuf_reset(e->outbuf);
    }

    if (e->param.use_thread) {
        buf_size = 32 * e->param.winStep;

        e->callback = qtk_engine_callback_new();
        e->callback->start_f = (qtk_engine_thread_start_f)qtk_eqkws_on_start;
        e->callback->data_f = (qtk_engine_thread_data_f)qtk_eqkws_on_data;
        e->callback->end_f = (qtk_engine_thread_end_f)qtk_eqkws_on_end;
        e->callback->reset_f = (qtk_engine_thread_reset_f)qtk_eqkws_on_reset;
        e->callback->set_notify_f =
            (qtk_engine_thread_set_notify_f)qtk_eqkws_on_set_notify;
        e->callback->cancel_f = (qtk_engine_thread_cancel_f)qtk_eqkws_on_cancel;
        e->callback->set_f = (qtk_engine_thread_set_f)qtk_eqkws_on_set;
        e->callback->err_f = (qtk_engine_thread_err_f)qtk_eqkws_on_err;
        e->callback->ths = e;

        e->thread = qtk_engine_thread_new(e->callback, e->session->log, "eqkws",
                                          buf_size, 10, 1, e->param.syn);
    }

    ret = 0;
end:
    wtk_log_log(e->session->log, "ret = %d", ret);
    if (ret != 0) {
        qtk_eqkws_delete(e);
        e = NULL;
    }
    return e;
}

int qtk_eqkws_delete(qtk_eqkws_t *e)
{
    if (e->thread) {
        qtk_engine_thread_delete(e->thread, 0);
    }
    if (e->callback) {
        qtk_engine_callback_delete(e->callback);
    }

    if (e->qkws) {
        qtk_qkws_delete(e->qkws);
    }

    if (e->cfg) {
        if (e->param.use_bin) {
            qtk_qkws_cfg_delete_bin(e->cfg);
        } else {
            qtk_qkws_cfg_delete(e->cfg);
        }
    }
    qtk_engine_param_clean(&e->param);
    if(e->outbuf){
        wtk_strbuf_delete(e->outbuf);
    }
    if(e->enrollfn){
        wtk_free(e->enrollfn);
    }

    wtk_free(e);
    return 0;
}

int qtk_eqkws_start(qtk_eqkws_t *e)
{
    int ret = 0;

    if (e->param.use_thread) {
        qtk_engine_thread_start(e->thread);
    } else {
        ret = qtk_eqkws_on_start(e);
        if (ret != 0) {
            wtk_log_warn(e->session->log, "qtk eqkws start failed| ret =%d",
                         ret);
        }
    }
    return ret;
}

int qtk_eqkws_feed(qtk_eqkws_t *e, char *data, int bytes, int is_end)
{
    int ret = 0;
    if(e->param.mic_shift != 1.0)
    {
        qtk_data_change_vol(data, bytes, e->param.mic_shift);
    }
    if (e->param.use_thread) {
        qtk_engine_thread_feed(e->thread, data, bytes, is_end);
    } else {
        if (bytes > 0) {
            ret = qtk_eqkws_on_data(e, data, bytes);
            if (ret != 0) {
                wtk_log_warn(e->session->log,
                             "qtk eqkws feed on data failed | ret = %d", ret);
                goto end;
            }
        }
        if (is_end) {
            ret = qtk_eqkws_on_end(e);
            if (ret != 0) {
                wtk_log_warn(e->session->log,
                             "qtk eqkws feed on end failed | ret = %d", ret);
                goto end;
            }
        }
    }
end:
    return ret;
}

int qtk_eqkws_reset(qtk_eqkws_t *e)
{
    if (e->param.use_thread) {
        qtk_engine_thread_reset(e->thread);
    } else {
        qtk_eqkws_on_reset(e);
    }
    return 0;
}

int qtk_eqkws_cancel(qtk_eqkws_t *e)
{
    ++e->cancel;
    if (e->param.use_thread) {
        qtk_engine_thread_cancel(e->thread);
    } else {
        qtk_eqkws_on_cancel(e);
    }

    return 0;
}

void qtk_eqkws_set_notify(qtk_eqkws_t *e, void *notify_ths, qtk_engine_notify_f notify_f)
{
    if (e->param.use_thread) {
        qtk_engine_thread_set_notify(e->thread, notify_ths, notify_f);
    } else {
        qtk_eqkws_on_set_notify(e, notify_ths, notify_f);
    }
}

int qtk_eqkws_set(qtk_eqkws_t *e, char *data, int bytes)
{
    if (e->param.use_thread) {
        qtk_engine_thread_set(e->thread, data, bytes);
    } else {
        qtk_eqkws_on_set(e, data, bytes);
    }
    return 0;
}

int qtk_eqkws_on_start(qtk_eqkws_t *e)
{
    return qtk_qkws_start(e->qkws);
}

int qtk_eqkws_on_data(qtk_eqkws_t *e, char *data, int bytes)
{
    return qtk_qkws_feed(e->qkws, data, bytes, 0);
}

int qtk_eqkws_on_end(qtk_eqkws_t *e)
{
    qtk_var_t var;
    wtk_string_t v;
    int ret;
#ifdef USE_TIME_TEST
    double tm1;
    tm1 = time_get_ms();
#endif

    ret = qtk_qkws_feed(e->qkws, 0, 0, 1);
    if (ret != 0) {
        wtk_log_log0(e->session->log, "qkws feed failed.");
        goto end;
    }

#ifdef USE_TIME_TEST
    // wtk_debug("=====>qkws time delay:%lf\n",time_get_ms()-tm1);
    wtk_log_log(e->session->log, "====>qkws time delay:%lf\n",
                time_get_ms() - tm1);
#endif
    ret = 0;
end:
    return ret;
}

void qtk_eqkws_on_reset(qtk_eqkws_t *e) { qtk_qkws_reset(e->qkws); }

void qtk_eqkws_on_set_notify(qtk_eqkws_t *e, void *notify_ths, qtk_engine_notify_f notify_f)
{
    e->notify_ths = notify_ths;
    e->notify_f = notify_f;
}

void qtk_eqkws_on_cancel(qtk_eqkws_t *e)
{
    qtk_qkws_cancel(e->qkws);
    --e->cancel;
}

void qtk_eqkws_on_set(qtk_eqkws_t *e, char *data, int bytes)
{
    wtk_cfg_file_t *cfile = NULL;
    wtk_cfg_item_t *item;
    wtk_queue_node_t *qn;
    char *enrollname=NULL;
    int ret;
    int set_enroll=0;
    int set_vad=0;
    int enrollnamelen=0;
    int isend=-1;
    float stime=0.0f;
    float etime=0.0f;

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
        if (wtk_string_cmp2(item->key, &qtk_engine_set_str[21]) == 0) {
            set_enroll=1;
            enrollname=item->value.str->data;
            enrollnamelen=item->value.str->len;
        }
        if (wtk_string_cmp2(item->key, &qtk_engine_set_str[22]) == 0) {
            isend=atoi(item->value.str->data);
        }
        if (wtk_string_cmp2(item->key, &qtk_engine_set_str[23]) == 0) {
            memcpy(e->enrollfn, item->value.str->data, item->value.str->len);
            qtk_qkws_set_enroll_fn(e->qkws, e->enrollfn, strlen(e->enrollfn));
        }
        if (wtk_string_cmp2(item->key, &qtk_engine_set_str[24]) == 0) {
            qtk_qkws_set_max_people(e->qkws, atoi(item->value.str->data));
        }
        if (wtk_string_cmp2(item->key, &qtk_engine_set_str[25]) == 0) {
            qtk_qkws_clean(e->qkws);
        }
        if (wtk_string_cmp2(item->key, &qtk_engine_set_str[26]) == 0) {
            qtk_qkws_reload(e->qkws);
        }
        if (wtk_string_cmp2(item->key, &qtk_engine_set_str[27]) == 0) {
            qtk_qkws_set_sv_thresh(e->qkws, atof(item->value.str->data));
        }
        if (wtk_string_cmp2(item->key, &qtk_engine_set_str[28]) == 0) {
            enrollnamelen=atoi(item->value.str->data);
        }
        if (wtk_string_cmp2(item->key, &qtk_engine_set_str[29]) == 0) {
            set_vad=1;
            stime=atof(item->value.str->data);
        }
        if (wtk_string_cmp2(item->key, &qtk_engine_set_str[30]) == 0) {
            etime=atof(item->value.str->data);
        }
        if (wtk_string_cmp2(item->key, &qtk_engine_set_str[31]) == 0) {
            qtk_qkws_set_spk_nums(e->qkws, atoi(item->value.str->data));
        }
    }

    if(set_enroll==1)
    {
        qtk_qkws_set_enroll(e->qkws, enrollname, enrollnamelen, isend);
    }
    if(set_vad==1)
    {
        qtk_qkws_set_vad_time(e->qkws, stime, etime);
    }

end:
    if (cfile) {
        wtk_cfg_file_delete(cfile);
    }
}

char *qtk_eqkws_get_fn(qtk_eqkws_t *e)
{
    return qtk_qkws_get_fn(e->qkws);
}

float qtk_eqkws_get_prob(qtk_eqkws_t *e)
{
    return qtk_qkws_get_prob(e->qkws);
}

void qtk_eqkws_get_result(qtk_eqkws_t *e, qtk_var_t *var)
{
    qtk_qkws_get_result(e->qkws, var);
}

void qtk_eqkws_on_kws(qtk_eqkws_t *e, qtk_var_t *data)
{
    if(data->type == QTK_SPEECH_DATA_PCM && e->param.out_channel > 1){
        wtk_strbuf_reset(e->outbuf);
        int i=0,j=0;
        while(i < data->v.str.len){
            for(j=0;j<e->param.out_channel;++j)
            {
                wtk_strbuf_push(e->outbuf, data->v.str.data+i, 2);
            }
            i+=2;
        }
        data->v.str.len = e->outbuf->pos;
        data->v.str.data = e->outbuf->data;
    }
	if(e->notify_f){
		e->notify_f(e->notify_ths, data);
	}
}

void qtk_eqkws_on_err(qtk_eqkws_t *e)
{

    if (!e->notify_f) {
        wtk_log_warn(e->session->log, "notify = %p", e->notify_f);
        _qtk_warning(e->session, _QTK_ENGINE_NOTIFY_INVALID);
        return;
    }
}
