#include "qtk_eqform9.h" 
#include "sdk/engine/comm/qtk_engine_hdr.h"

void qtk_eqform9_on_data(qtk_eqform9_t *e,qtk_var_t *var);
int qtk_eqform9_on_start(qtk_eqform9_t *e);
int qtk_eqform9_on_feed(qtk_eqform9_t *e,char *data,int len);
int qtk_eqform9_on_end(qtk_eqform9_t *e);
void qtk_eqform9_on_reset(qtk_eqform9_t *e);
void qtk_eqform9_on_set_notify(qtk_eqform9_t *e,void *notify_ths,qtk_engine_notify_f notify_f);
int qtk_eqform9_on_set(qtk_eqform9_t *e,char *data,int bytes);


qtk_eqform9_t* qtk_eqform9_new(qtk_session_t *session,wtk_local_cfg_t *params)
{
	qtk_eqform9_t *e;
	int buf_size = 0;
	int ret;

	e=(qtk_eqform9_t*)wtk_calloc(1, sizeof(qtk_eqform9_t));
	e->session = session;

	qtk_engine_param_set_session(&e->param, e->session);
	ret = qtk_engine_param_feed(&e->param, params);
	if(ret != 0) {
		wtk_log_warn0(e->session->log,"params als failed.");
		goto end;
	}

	if(e->param.use_bin) {
		e->cfg = qtk_qform_cfg_new_bin(e->param.cfg);
	} else {
		e->cfg = qtk_qform_cfg_new(e->param.cfg);
	}
	if(!e->cfg) {
		wtk_log_warn0(e->session->log,"cfg new failed.");
		_qtk_error(e->session,_QTK_CFG_NEW_FAILED);
		ret = -1;
		goto end;
	}
	e->qform = qtk_qform_new(e->cfg);
	if(!e->qform){
		wtk_log_err0(e->session->log, "qform new failed.");
		ret = -1;
		goto end;
	}
	qtk_qform_set_notify2(e->qform, e, (qtk_engine_notify_f)qtk_eqform9_on_data);
	if(e->param.use_thread) {
		buf_size = 640;

		e->callback = qtk_engine_callback_new();
		e->callback->start_f      = (qtk_engine_thread_start_f)      qtk_eqform9_on_start;
		e->callback->data_f       = (qtk_engine_thread_data_f)       qtk_eqform9_on_feed;
		e->callback->end_f        = (qtk_engine_thread_end_f)        qtk_eqform9_on_end;
		e->callback->reset_f      = (qtk_engine_thread_reset_f)      qtk_eqform9_on_reset;
		e->callback->set_notify_f = (qtk_engine_thread_set_notify_f) qtk_eqform9_on_set_notify;
		e->callback->set_f        = (qtk_engine_thread_set_f)        qtk_eqform9_on_set;
		e->callback->ths          = e;

		e->thread = qtk_engine_thread_new(
				e->callback,
				e->session->log,
				"eqform9",
				buf_size,
				20,
				0,
				e->param.syn
				);
	}

	ret = 0;
end:
	wtk_log_log(e->session->log,"ret = %d",ret);
	if(ret != 0) {
		qtk_eqform9_delete(e);
		e = NULL;
	}
	return e;
}

int qtk_eqform9_delete(qtk_eqform9_t *e)
{
	if(e->thread) {
		qtk_engine_thread_delete(e->thread,1);
	}
	if(e->callback) {
		qtk_engine_callback_delete(e->callback);
	}
	if(e->qform)
	{
		qtk_qform_delete(e->qform);
		e->qform = NULL;
	}
	if(e->cfg)
	{
		if(e->param.use_bin)
		{
			qtk_qform_cfg_delete_bin(e->cfg);
		}else{
			qtk_qform_cfg_delete(e->cfg);
		}
		e->cfg = NULL;
	}
	qtk_engine_param_clean(&e->param);

	wtk_free(e);
	return 0;
}


int qtk_eqform9_on_start(qtk_eqform9_t *e)
{
	return qtk_qform_start(e->qform);
}

int qtk_eqform9_on_feed(qtk_eqform9_t *e,char *data,int len)
{
	int ret;
	ret = qtk_qform_feed(e->qform, data, len, 0);
	return ret;
}

int qtk_eqform9_on_end(qtk_eqform9_t *e)
{
	return qtk_qform_feed(e->qform, NULL, 0, 1);
}

void qtk_eqform9_on_reset(qtk_eqform9_t *e)
{
	qtk_qform_reset(e->qform);
}

void qtk_eqform9_on_set_notify(qtk_eqform9_t *e,void *notify_ths,qtk_engine_notify_f notify_f)
{
	e->notify_ths = notify_ths;
	e->notify     = notify_f;
}

int qtk_eqform9_on_set(qtk_eqform9_t *e,char *data,int bytes)
{
	return 0;
}


int qtk_eqform9_start(qtk_eqform9_t *e)
{
	int ret;

	if(e->param.use_thread){
		qtk_engine_thread_start(e->thread);
	}else{
		qtk_eqform9_on_start(e);
	}
	ret = 0;
	return ret;
}

int qtk_eqform9_feed(qtk_eqform9_t *e,char *data,int bytes,int is_end)
{
	if(e->param.use_thread)
	{
		qtk_engine_thread_feed(e->thread,data,bytes,is_end);
	}else{
		if(bytes > 0) {
			qtk_eqform9_on_feed(e,data,bytes);
		}
		if(is_end) {
			qtk_eqform9_on_end(e);
		}
	}
	return 0;
}

int qtk_eqform9_reset(qtk_eqform9_t *e)
{
	if(e->param.use_thread){
		qtk_engine_thread_reset(e->thread);
	}else{
		qtk_eqform9_on_reset(e);
	}

	return 0;
}

int qtk_eqform9_cancel(qtk_eqform9_t *e)
{
	if(e->param.use_thread) {
		qtk_engine_thread_cancel(e->thread);
	}
	return 0;
}

void qtk_eqform9_set_notify(qtk_eqform9_t *e,void *ths,qtk_engine_notify_f notify_f)
{
	if(e->param.use_thread) {
		qtk_engine_thread_set_notify(e->thread,ths,notify_f);
	} else {
		qtk_eqform9_on_set_notify(e,ths,notify_f);
	}
}

int qtk_eqform9_set(qtk_eqform9_t *e,char *data,int bytes)
{
	int ret = 0;

	if(e->param.use_thread) {
		qtk_engine_thread_set(e->thread,data,bytes);
	} else {
		ret = qtk_eqform9_on_set(e,data,bytes);
	}
	return ret;
}


void qtk_eqform9_on_data(qtk_eqform9_t *e,qtk_var_t *var)
{
	if(e->notify){
		e->notify(e->notify_ths, var);
	}

}


