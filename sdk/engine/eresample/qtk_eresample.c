/*
 * qtk_eresample.c
 *
 *  Created on: Jul 7, 2023
 *      Author: root
 */

#include "qtk_eresample.h"
#include "sdk/engine/comm/qtk_engine_hdr.h"

void qtk_eresample_init(qtk_eresample_t *e) {
    qtk_engine_param_init(&e->param);

    e->session = NULL;

    e->srsp = NULL;
    e->rsp = NULL;

    e->notify = NULL;
    e->notify_ths = NULL;

    e->thread = NULL;
    e->callback = NULL;
    e->outresample = NULL;
    e->feedend = 0;
}

int qtk_eresample_on_start(qtk_eresample_t *e);
int qtk_eresample_on_feed(qtk_eresample_t *e, char *data, int len);
int qtk_eresample_on_end(qtk_eresample_t *e);
void qtk_eresample_on_reset(qtk_eresample_t *e);
void qtk_eresample_on_set_notify(qtk_eresample_t *e, void *notify_ths,
                             qtk_engine_notify_f notify_f);
int qtk_eresample_on_set(qtk_eresample_t *e, char *data, int bytes);
void qtk_eresample_on_vboxebf(qtk_eresample_t *bfio, char *data, int len);

qtk_eresample_t *qtk_eresample_new(qtk_session_t *session, wtk_local_cfg_t *params) {
    qtk_eresample_t *e;
    int buf_size;
    int ret;
    int i;

    e = (qtk_eresample_t *)wtk_malloc(sizeof(qtk_eresample_t));
    qtk_eresample_init(e);
    e->session = session;

    qtk_engine_param_set_session(&e->param, e->session);
    ret = qtk_engine_param_feed(&e->param, params);
    if (ret != 0) {
        wtk_log_warn0(e->session->log, "params als failed.");
        goto end;
    }

    if(e->param.channel == 1)
    {
        e->rsp = wtk_resample_new(384);
        if (!e->rsp) {
            wtk_log_err0(e->session->log, "resample new failed.");
            ret = -1;
            goto end;
        }
        wtk_resample_set_notify(e->rsp, e, (wtk_resample_notify_f)qtk_eresample_on_vboxebf);
    }else{
        e->srsp = speex_resampler_init(e->param.channel, e->param.resample_in_rate, e->param.resample_out_rate, SPEEX_RESAMPLER_QUALITY_DESKTOP, NULL);
        e->outresample_size = 96*100*e->param.channel;
        e->outresample = (char *)wtk_malloc(e->outresample_size);
    }

    if (e->param.use_thread) {
    	buf_size = 256;

        e->callback = qtk_engine_callback_new();
        e->callback->start_f = (qtk_engine_thread_start_f)qtk_eresample_on_start;
        e->callback->data_f = (qtk_engine_thread_data_f)qtk_eresample_on_feed;
        e->callback->end_f = (qtk_engine_thread_end_f)qtk_eresample_on_end;
        e->callback->reset_f = (qtk_engine_thread_reset_f)qtk_eresample_on_reset;
        e->callback->set_notify_f =
            (qtk_engine_thread_set_notify_f)qtk_eresample_on_set_notify;
        e->callback->set_f = (qtk_engine_thread_set_f)qtk_eresample_on_set;
        e->callback->ths = e;

        e->thread = qtk_engine_thread_new(e->callback, e->session->log, "evboxebf",
                                          buf_size, 20, 1, e->param.syn);
    }

    ret = 0;
end:
    wtk_log_log(e->session->log, "ret = %d", ret);
    if (ret != 0) {
        qtk_eresample_delete(e);
        e = NULL;
    }
    return e;
}

int qtk_eresample_delete(qtk_eresample_t *e) {
    if (e->thread) {
        qtk_engine_thread_delete(e->thread, 1);
    }
    if (e->callback) {
        qtk_engine_callback_delete(e->callback);
    }
    if (e->rsp) {
        wtk_resample_delete(e->rsp);
        e->rsp = NULL;
    }
    if (e->srsp) {
        speex_resampler_destroy(e->srsp);
        e->srsp = NULL;
    }
    if(e->outresample)
    {
        wtk_free(e->outresample);
    }
    qtk_engine_param_clean(&e->param);

    wtk_free(e);
    return 0;
}

int qtk_eresample_on_start(qtk_eresample_t *e) {
    if(e->param.channel == 1)
    {
        wtk_resample_start(e->rsp, e->param.resample_in_rate, e->param.resample_out_rate);
    }
    return 0;
}

int qtk_eresample_on_feed(qtk_eresample_t *e, char *data, int bytes) {

    if (bytes > 0) {
        if(e->param.channel == 1)
        {
    	    wtk_resample_feed(e->rsp, data, bytes, 0);
        }else{
            int inlen, outlen;
            if(bytes > e->outresample_size)
            {
                wtk_free(e->outresample);
                e->outresample = (char *)wtk_malloc(bytes);
                e->outresample_size = bytes;
            }
            memset(e->outresample, 0, bytes);
            inlen=(bytes >> 1)/e->param.channel;
            outlen=inlen;

            if(e->srsp)
            {
                speex_resampler_process_interleaved_int(e->srsp,
                                            (spx_int16_t *)(data), (spx_uint32_t *)(&inlen), 
                                            (spx_int16_t *)(e->outresample), (spx_uint32_t *)(&outlen));

                if (e->notify) {
                    qtk_var_t var;

                    var.type = QTK_SPEECH_DATA_PCM;
                    var.v.str.data = e->outresample;
                    var.v.str.len = (outlen<<1)*e->param.channel;
                    e->notify(e->notify_ths, &var);
                }
            }
        }
    }

    return 0;
}

int qtk_eresample_on_end(qtk_eresample_t *e) {
    if(e->param.channel == 1)
    {
        wtk_resample_feed(e->rsp, NULL, 0, 1);
        e->feedend = 1;
    }
    return 0;
}

void qtk_eresample_on_reset(qtk_eresample_t *e) {
    if (e->feedend == 0) {
        qtk_eresample_on_end(e);
    }
    if(e->param.channel == 1)
	{
		wtk_resample_close(e->rsp);
	}
}

void qtk_eresample_on_set_notify(qtk_eresample_t *e, void *notify_ths,
                             qtk_engine_notify_f notify_f) {
    e->notify_ths = notify_ths;
    e->notify = notify_f;
}

int qtk_eresample_on_set(qtk_eresample_t *e, char *data, int bytes) { return 0; }

int qtk_eresample_start(qtk_eresample_t *e) {
    int ret;

    if (e->param.use_thread) {
        qtk_engine_thread_start(e->thread);
    } else {
        qtk_eresample_on_start(e);
    }
    ret = 0;
    return ret;
}

int qtk_eresample_feed(qtk_eresample_t *e, char *data, int bytes, int is_end) {
    if (e->param.use_thread) {
        qtk_engine_thread_feed(e->thread, data, bytes, is_end);
    } else {
        if (bytes > 0) {
            qtk_eresample_on_feed(e, data, bytes);
        }
        if (is_end) {
            qtk_eresample_on_end(e);
        }
    }
    return 0;
}

int qtk_eresample_reset(qtk_eresample_t *e) {
    if (e->param.use_thread) {
        qtk_engine_thread_reset(e->thread);
    } else {
        qtk_eresample_on_reset(e);
    }

    return 0;
}

int qtk_eresample_cancel(qtk_eresample_t *e) {
    if (e->param.use_thread) {
        qtk_engine_thread_cancel(e->thread);
    }
    return 0;
}

void qtk_eresample_set_notify(qtk_eresample_t *e, void *ths,
                          qtk_engine_notify_f notify_f) {
    if (e->param.use_thread) {
        qtk_engine_thread_set_notify(e->thread, ths, notify_f);
    } else {
        qtk_eresample_on_set_notify(e, ths, notify_f);
    }
}

int qtk_eresample_set(qtk_eresample_t *e, char *data, int bytes) {
    int ret;

    if (e->param.use_thread) {
        qtk_engine_thread_set(e->thread, data, bytes);
    } else {
        ret = qtk_eresample_on_set(e, data, bytes);
    }
    return ret;
}

void qtk_eresample_on_vboxebf(qtk_eresample_t *bfio, char *data,
                       int len) {
    qtk_var_t var;

    var.type = QTK_SPEECH_DATA_PCM;
    var.v.str.data = data;
    var.v.str.len = len;
    if (bfio->notify) {
        bfio->notify(bfio->notify_ths, &var);
    }
}



