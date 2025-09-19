#include "qtk_enginebfio.h"
#include "sdk/engine/comm/qtk_engine_hdr.h"
#include "sdk/codec/qtk_audio_conversion.h"

int skip_channels[2]={2,3};
int nskip=2;

void qtk_enginebfio_init(qtk_enginebfio_t *e) {
    qtk_engine_param_init(&e->param);

    e->session = NULL;

    e->bfio_cfg = NULL;
    e->bfio = NULL;
    e->soundscreen_cfg = NULL;
    e->soundscreen = NULL;
    e->vboxebf_cfg = NULL;
    e->vboxebf = NULL;

    e->notify = NULL;
    e->notify_ths = NULL;
}

int qtk_enginebfio_on_start(qtk_enginebfio_t *e);
int qtk_enginebfio_on_feed(qtk_enginebfio_t *e, char *data, int len);
int qtk_enginebfio_on_end(qtk_enginebfio_t *e);
void qtk_enginebfio_on_reset(qtk_enginebfio_t *e);
void qtk_enginebfio_on_set_notify(qtk_enginebfio_t *e, void *notify_ths,
                             qtk_engine_notify_f notify_f);
int qtk_enginebfio_on_set(qtk_enginebfio_t *e, char *data, int bytes);
void qtk_enginebfio_on_bfio(qtk_enginebfio_t *bfio, qtk_var_t *var);
void qtk_enginebfio_on_soundscreen(qtk_enginebfio_t *bfio, qtk_var_t *var);
int qtk_engienbfio_qform_entry(qtk_enginebfio_t *e, wtk_thread_t *t);

qtk_enginebfio_t *qtk_enginebfio_new(qtk_session_t *session, wtk_local_cfg_t *params) {
    qtk_enginebfio_t *e;
    int buf_size;
    int ret;
    int i;

    e = (qtk_enginebfio_t *)wtk_malloc(sizeof(qtk_enginebfio_t));
    qtk_enginebfio_init(e);
    e->session = session;

    qtk_engine_param_set_session(&e->param, e->session);
    ret = qtk_engine_param_feed(&e->param, params);
    if (ret != 0) {
        wtk_log_warn0(e->session->log, "params als failed.");
        goto end;
    }

    if (e->param.use_bin) {
        e->bfio_cfg = qtk_bfio_cfg_new_bin(e->param.cfg);
    } else {
        e->bfio_cfg = qtk_bfio_cfg_new(e->param.cfg);
    }
    if (!e->bfio_cfg) {
        wtk_log_warn0(e->session->log, "bfio_cfg new failed.");
        _qtk_error(e->session, _QTK_CFG_NEW_FAILED);
        ret = -1;
        goto end;
    }
    e->bfio = qtk_bfio_new(e->bfio_cfg);
    if (!e->bfio) {
        wtk_log_err0(e->session->log, "bfio new failed.");
        ret = -1;
        goto end;
    }
    qtk_bfio_set_notify2(e->bfio, e, (qtk_engine_notify_f)qtk_enginebfio_on_bfio);
    if(e->bfio->channel == 8){
        nskip=0;
    }

    if(e->bfio_cfg->use_soundscreen){
        e->soundscreen_cfg = qtk_soundscreen_cfg_new(e->bfio_cfg->soundscreen_fn.data);
        if (!e->soundscreen_cfg) {
            wtk_log_warn0(e->session->log, "soundscreen_cfg new failed.");
            _qtk_error(e->session, _QTK_CFG_NEW_FAILED);
            ret = -1;
            goto end;
        }
        e->soundscreen = qtk_soundscreen_new(e->soundscreen_cfg);
        if (!e->soundscreen) {
            wtk_log_err0(e->session->log, "soundscreen new failed.");
            ret = -1;
            goto end;
        }
        qtk_soundscreen_set_notify2(e->soundscreen, e, (qtk_engine_notify_f)qtk_enginebfio_on_soundscreen);
    }
    if(e->bfio_cfg->use_vboxebf){
        e->vboxebf_cfg = qtk_vboxebf_cfg_new(e->bfio_cfg->vboxebf_fn.data);
        if(!e->vboxebf_cfg){
            wtk_log_warn0(e->session->log, "vboxebf_cfg new failed.");
            _qtk_error(e->session, _QTK_CFG_NEW_FAILED);
            ret = -1;
            goto end;
        }
        e->vboxebf = qtk_vboxebf_new(e->vboxebf_cfg);
        if(!e->vboxebf){
            wtk_log_err0(e->session->log, "vboxebf new failed.");
            ret = -1;
            goto end; 
        }
        qtk_vboxebf_set_notify2(e->vboxebf, e, (qtk_engine_notify_f)qtk_enginebfio_on_soundscreen);
    }

    e->msg = qtk_msg_new();
    wtk_thread_init(&e->qform_t, (thread_route_handler)qtk_engienbfio_qform_entry, e);
    wtk_blockqueue_init(&e->bfio_queue);

    // wtk_debug("mic_shift=%f spk_shift=%f echo_shift=%f\n", e->param.mic_shift, e->param.spk_shift, e->param.echo_shift);
    ret = 0;
end:
    wtk_log_log(e->session->log, "ret = %d", ret);
    if (ret != 0) {
        qtk_enginebfio_delete(e);
        e = NULL;
    }
    return e;
}

int qtk_enginebfio_delete(qtk_enginebfio_t *e) {

    if(e->msg){
        qtk_msg_delete(e->msg);
	}

    if(e->bfio_cfg->use_soundscreen){
        if(e->soundscreen){
            qtk_soundscreen_delete(e->soundscreen);
            e->soundscreen = NULL;
        }
        if(e->soundscreen_cfg){
            qtk_soundscreen_cfg_delete(e->soundscreen_cfg);
            e->soundscreen_cfg = NULL;
        }
    }
    if(e->bfio_cfg->use_vboxebf){
        if(e->vboxebf){
            qtk_vboxebf_delete(e->vboxebf);
            e->vboxebf = NULL;
        }
        if(e->vboxebf_cfg){
            qtk_vboxebf_cfg_delete(e->vboxebf_cfg);
            e->vboxebf_cfg = NULL;
        }
    }
    if (e->bfio) {
        qtk_bfio_delete(e->bfio);
        e->bfio = NULL;
    }
    if (e->bfio_cfg) {
        if (e->param.use_bin) {
            qtk_bfio_cfg_delete_bin(e->bfio_cfg);
        } else {
            qtk_bfio_cfg_delete(e->bfio_cfg);
        }
        e->bfio_cfg = NULL;
    }
    qtk_engine_param_clean(&e->param);

    wtk_blockqueue_clean(&e->bfio_queue);

    wtk_free(e);
    return 0;
}

int qtk_enginebfio_on_start(qtk_enginebfio_t *e) {
    if(e->bfio){
        qtk_bfio_start(e->bfio);
    }
    if(e->bfio_cfg->use_soundscreen){
        if(e->soundscreen){
            qtk_soundscreen_start(e->soundscreen);
        }
    }
    if(e->bfio_cfg->use_vboxebf){
        if(e->vboxebf){
            qtk_vboxebf_start(e->vboxebf);
        }
    }
    
    e->qform_run = 1;
    wtk_thread_start(&e->qform_t);
    return 0;
}

int qtk_enginebfio_on_feed(qtk_enginebfio_t *e, char *data, int bytes) {
    if (bytes > 0) {
        if(e->bfio_cfg->use_soundscreen || e->bfio_cfg->use_vboxebf)
        {
            qtk_msg_node_t *msg_node;
            msg_node = qtk_msg_pop_node(e->msg);
            wtk_strbuf_push(msg_node->buf, data, bytes);
            wtk_blockqueue_push(&e->bfio_queue, &msg_node->qn);
        }

        if(e->bfio){
            if(nskip > 0){
                short *pv,*pv1;
                int i,j,k,len;
                int pos,pos1;
                int b;
                int channel=8;

                pv = pv1 = (short*)data;
                pos = pos1 = 0;
                len = bytes / (2 * channel);
                for(i=0;i < len; ++i){
                    for(j=0;j < channel; ++j) {
                        b = 0;
                        for(k=0;k<nskip;++k) {
                            if(j == skip_channels[k]) {
                                b = 1;
                            }
                        }
                        if(b) {
                            ++pos1;
                        } else {
                            pv[pos++] = pv1[pos1++];
                        }
                    }
                }
                bytes = pos << 1;
            }
            qtk_bfio_feed(e->bfio, data, bytes, 0);
        }
    }
    return 0;
}

int qtk_enginebfio_on_end(qtk_enginebfio_t *e) {
    if(e->bfio){
        qtk_bfio_feed(e->bfio, NULL, 0, 1);
    }
    e->feedend = 1;
    return 0;
}

void qtk_enginebfio_on_reset(qtk_enginebfio_t *e) {
    if (e->feedend == 0) {
        qtk_enginebfio_on_end(e);
    }
    if(e->bfio){
        qtk_bfio_reset(e->bfio);
    }
	e->qform_run = 0;
	wtk_blockqueue_wake(&e->bfio_queue);
	wtk_thread_join(&e->qform_t);

    if(e->bfio_cfg->use_soundscreen){
        if(e->soundscreen){
            qtk_soundscreen_reset(e->soundscreen);
        }
    }
    if(e->bfio_cfg->use_vboxebf){
        if(e->vboxebf){
            qtk_vboxebf_reset(e->vboxebf);
        }
    }
}

void qtk_enginebfio_on_set_notify(qtk_enginebfio_t *e, void *notify_ths,
                             qtk_engine_notify_f notify_f) {
    e->notify_ths = notify_ths;
    e->notify = notify_f;
}

int qtk_enginebfio_on_set(qtk_enginebfio_t *e, char *data, int bytes) {

    return 0;
}

int qtk_enginebfio_start(qtk_enginebfio_t *e) {
    int ret;
    qtk_enginebfio_on_start(e);
    ret = 0;
    return ret;
}

int qtk_enginebfio_feed(qtk_enginebfio_t *e, char *data, int bytes, int is_end) {
    if (bytes > 0) {
        qtk_enginebfio_on_feed(e, data, bytes);
    }
    if (is_end) {
        qtk_enginebfio_on_end(e);
    }
    return 0;
}

int qtk_enginebfio_reset(qtk_enginebfio_t *e) {
    qtk_enginebfio_on_reset(e);
    return 0;
}

int qtk_enginebfio_cancel(qtk_enginebfio_t *e) {
    return 0;
}

void qtk_enginebfio_set_notify(qtk_enginebfio_t *e, void *ths,
                          qtk_engine_notify_f notify_f) {
    qtk_enginebfio_on_set_notify(e, ths, notify_f);
}

int qtk_enginebfio_set(qtk_enginebfio_t *e, char *data, int bytes) {
    int ret=0;

    ret = qtk_enginebfio_on_set(e, data, bytes);
    return ret;
}

void qtk_enginebfio_on_bfio(qtk_enginebfio_t *bfio, qtk_var_t *var) {
    if (bfio->notify) {
        bfio->notify(bfio->notify_ths, var);
    }
}

void qtk_enginebfio_on_soundscreen(qtk_enginebfio_t *bfio, qtk_var_t *var){
    if (bfio->notify) {
        bfio->notify(bfio->notify_ths, var);
    }
}

int qtk_engienbfio_qform_entry(qtk_enginebfio_t *e, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;

	while(e->qform_run){
		qn= wtk_blockqueue_pop(&e->bfio_queue,-1,NULL);
		if(!qn) {
			continue;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);

        if(e->bfio_cfg->use_soundscreen){
            if(e->soundscreen){
                qtk_soundscreen_feed(e->soundscreen, msg_node->buf->data, msg_node->buf->pos, 0);
            }
        }
        if(e->bfio_cfg->use_vboxebf){
            if(e->vboxebf){
                qtk_vboxebf_feed(e->vboxebf, msg_node->buf->data, msg_node->buf->pos, 0);
            }
        }

        qtk_msg_push_node(e->msg, msg_node);
    }

    if(e->bfio_cfg->use_soundscreen){
        if(e->soundscreen){
            qtk_soundscreen_feed(e->soundscreen, NULL, 0, 1);
        }
    }
    if(e->bfio_cfg->use_vboxebf){
        if(e->vboxebf){
            qtk_vboxebf_feed(e->vboxebf, NULL, 0, 1);
        }
    }
}

