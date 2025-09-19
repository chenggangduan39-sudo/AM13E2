#include "qtk_eimg.h"

static long count;

int qtk_eimg_on_start(qtk_eimg_t * e);
void qtk_eimg_on_reset(qtk_eimg_t *e);
int qtk_eimg_on_data(qtk_eimg_t *e,char* data, int bytes);
int qtk_eimg_on_end(qtk_eimg_t *e);
void qtk_eimg_on_set_notify(qtk_eimg_t *e,void *ths,qtk_engine_notify_f notify);
static void notify_f(qtk_eimg_t *e, qtk_api_img_type_t type, char *data, int len);

static void qtk_eimg_init(qtk_eimg_t * e)
{
	qtk_engine_param_init(&e->param);
	e->session = NULL;
	e->img =NULL;
	e->log = NULL;
	e->cfg =NULL;
	e->notify   = NULL;
	e->notify_ths = NULL;
	e->thread   = NULL;
	e->callback = NULL;
}


qtk_eimg_t *qtk_eimg_new(qtk_session_t *s,wtk_local_cfg_t * params)
{
	qtk_eimg_t* e;
	int ret=-1;
	int buf_size;


	e =(qtk_eimg_t*)wtk_malloc(sizeof(qtk_eimg_t));
	qtk_eimg_init(e);

	e->session = s;
	qtk_engine_param_set_session(&e->param,e->session);
	ret=qtk_engine_param_feed(&e->param,params);
	if(ret<0){
		wtk_log_log0(e->session->log,"param feed failed.");
		goto end;
	}

	if (e->param.use_bin) {
		wtk_debug("not support bin fmt\n");
		wtk_log_log0(e->session->log, "bin not support.");
		ret = -1; goto end;
	} else {
		e->cfg = qtk_api_img_cfg_new(e->param.cfg);
		if (!e->cfg) {
			ret=-1;
			wtk_log_err0(e->session->log,"cfg new failed\n");
			_qtk_error(e->session,_QTK_CFG_NEW_FAILED);
			goto end;
		}
	}
	e->img = qtk_api_img_new(e->cfg, e->session);
	if (!e->img) {
		ret = -1;
		wtk_log_err0(e->session->log,"img new failed\n");
		goto end;
	}
	qtk_api_img_set_notify(e->img, e, (qtk_api_img_notify_f)notify_f);

	wtk_debug("use_thread = %d\n", e->param.use_thread);
	if(e->param.use_thread) { 
		buf_size = 32 * e->param.winStep;
		e->callback = qtk_engine_callback_new();
		e->callback->start_f		= (qtk_engine_thread_start_f)		qtk_eimg_on_start;
		e->callback->data_f			= (qtk_engine_thread_data_f)		qtk_eimg_on_data;
		e->callback->end_f			= (qtk_engine_thread_end_f)			qtk_eimg_on_end;
		e->callback->reset_f		= (qtk_engine_thread_reset_f)		qtk_eimg_on_reset;
		e->callback->set_notify_f	= (qtk_engine_thread_set_notify_f)	qtk_eimg_on_set_notify;
		e->callback->ths			= e;
		e->thread = qtk_engine_thread_new(
										  e->callback,
										  e->log,
										  "img",
										  buf_size,
										  20,
										  1,
										  e->param.syn
										 );
	}
		ret = 0;
end:
	if(ret!=0){
		qtk_eimg_delete(e);
		e=NULL;
	}
	return e;
}


void qtk_eimg_delete(qtk_eimg_t *e)
{
	if(e->thread){
		qtk_engine_thread_delete(e->thread,0);
	}

	if(e->callback){
		qtk_engine_callback_delete(e->callback);
	}

	if(e->img){
		qtk_api_img_delete(e->img);
	}

	if(e->cfg){
		qtk_api_img_cfg_delete(e->cfg);
	}


	qtk_engine_param_clean(&e->param);
	wtk_free(e);
}

int qtk_eimg_start(qtk_eimg_t *e)
{
	if(e->param.use_thread){
		qtk_engine_thread_start(e->thread);
	}else{
		qtk_eimg_on_start(e);
	}
	return 0;
}

void qtk_eimg_reset(qtk_eimg_t *e)
{
	if(e->param.use_thread){
		qtk_engine_thread_reset(e->thread);
	}else{
		qtk_eimg_on_reset(e);
	}
}

int qtk_eimg_feed(qtk_eimg_t *e,char *data,int bytes,int is_end)
{
	count += bytes;
	if(e->param.use_thread){
		qtk_engine_thread_feed(e->thread,data,bytes,is_end);
		}else{
			if(bytes>0){
				qtk_eimg_on_data(e,data,bytes);
			}
			if(is_end){
				qtk_eimg_on_end(e);
			}
		}
	return 0;
}
void qtk_eimg_set_notify(qtk_eimg_t *e,void *ths,qtk_engine_notify_f notify)
{
	if(e->param.use_thread) {
		qtk_engine_thread_set_notify(e->thread,ths,notify);
	} else {
		qtk_eimg_on_set_notify(e,ths,notify);
	}
}

static void notify_f(qtk_eimg_t *e, qtk_api_img_type_t type, char *data, int len)
{
	qtk_var_t var;

	switch (type) {
	case QTK_API_IMG_TYPE_SPEECH_START:
		var.type = QTK_SPEECH_START;
		break;
	case QTK_API_IMG_TYPE_SPEECH_END:
		var.type = QTK_SPEECH_END;
		break;
	case QTK_API_IMG_TYPE_WAKE:
		var.type = QTK_AEC_WAKE;
		break;
	}
	if (e->notify) {
		e->notify(e->notify_ths, &var);
	}
}

void qtk_eimg_on_set_notify(qtk_eimg_t *e,void *ths,qtk_engine_notify_f notify)
{
	e->notify_ths = ths;
	e->notify = notify;
}

int qtk_eimg_on_start(qtk_eimg_t * e)
{
	qtk_api_img_start(e->img);
	return 0;
}

void qtk_eimg_on_reset(qtk_eimg_t *e)
{
	qtk_api_img_reset(e->img);
}

int qtk_eimg_on_data(qtk_eimg_t *e,char* data, int bytes)
{
	/* wtk_debug(">>>>>>>>>>>>feed bytes = %d\n", bytes); */
	return qtk_api_img_feed(e->img,data,bytes,0);
}

int qtk_eimg_on_end(qtk_eimg_t *e)
{
	return qtk_api_img_feed(e->img,0,0,1);
}

int qtk_eimg_set(qtk_eimg_t *e,char *data,int bytes)
{
	return 0;
}
