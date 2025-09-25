#include "qtk_engineaec.h" 
#include "sdk/engine/comm/qtk_engine_hdr.h"

void qtk_engineaec_on_data(qtk_engineaec_t *e,qtk_var_t *var);
int qtk_engineaec_on_start(qtk_engineaec_t *e);
int qtk_engineaec_on_feed(qtk_engineaec_t *e,char *data,int len);
int qtk_engineaec_on_end(qtk_engineaec_t *e);
void qtk_engineaec_on_reset(qtk_engineaec_t *e);
void qtk_engineaec_on_set_notify(qtk_engineaec_t *e,void *notify_ths,qtk_engine_notify_f notify_f);
int qtk_engineaec_on_set(qtk_engineaec_t *e,char *data,int bytes);

void qtk_engineaec_init(qtk_engineaec_t *e)
{
	qtk_engine_param_init(&e->param);

	e->session = NULL;

	e->cfg = NULL;

	e->notify     = NULL;
	e->notify_ths = NULL;

	e->thread   = NULL;
	e->callback = NULL;

}

qtk_engineaec_t* qtk_engineaec_new(qtk_session_t *session,wtk_local_cfg_t *params)
{
	qtk_engineaec_t *e;
	int buf_size = 0;
	int ret;

	e=(qtk_engineaec_t*)wtk_calloc(1, sizeof(qtk_engineaec_t));
	qtk_engineaec_init(e);
	e->session = session;

	qtk_engine_param_set_session(&e->param, e->session);
	ret = qtk_engine_param_feed(&e->param, params);
	if(ret != 0) {
		wtk_log_warn0(e->session->log,"params als failed.");
		goto end;
	}

	if(e->param.use_bin) {
		e->cfg = qtk_aec_cfg_new_bin(e->param.cfg);
	} else {
		e->cfg = qtk_aec_cfg_new(e->param.cfg);
	}
	if(!e->cfg) {
		wtk_log_warn0(e->session->log,"cfg new failed.");
		_qtk_error(e->session,_QTK_CFG_NEW_FAILED);
		ret = -1;
		goto end;
	}
	if(e->param.use_logwav && e->param.log_wav_path->len>0){
		
        e->wav=wtk_wavfile_new(16000);
        e->wav->max_pend=0;
        ret=wtk_wavfile_open(e->wav,e->param.log_wav_path->data);
		if(ret==-1){goto end;}
	}
	
	e->eqform = qtk_aec_new(e->cfg);
	if(!e->eqform){
		wtk_log_err0(e->session->log, "eqform new failed.");
		ret = -1;
		goto end;
	}
	qtk_aec_set_notify2(e->eqform, e, (qtk_engine_notify_f)qtk_engineaec_on_data);
	if(e->param.use_thread) {
		buf_size = 640;

		e->callback = qtk_engine_callback_new();
		e->callback->start_f      = (qtk_engine_thread_start_f)      qtk_engineaec_on_start;
		e->callback->data_f       = (qtk_engine_thread_data_f)       qtk_engineaec_on_feed;
		e->callback->end_f        = (qtk_engine_thread_end_f)        qtk_engineaec_on_end;
		e->callback->reset_f      = (qtk_engine_thread_reset_f)      qtk_engineaec_on_reset;
		e->callback->set_notify_f = (qtk_engine_thread_set_notify_f) qtk_engineaec_on_set_notify;
		e->callback->set_f        = (qtk_engine_thread_set_f)        qtk_engineaec_on_set;
		e->callback->ths          = e;

		e->thread = qtk_engine_thread_new(
				e->callback,
				e->session->log,
				"eeqform",
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
		qtk_engineaec_delete(e);
		e = NULL;
	}
	return e;
}

int qtk_engineaec_delete(qtk_engineaec_t *e)
{
	if(e->thread) {
		qtk_engine_thread_delete(e->thread,1);
	}
	if(e->callback) {
		qtk_engine_callback_delete(e->callback);
	}
	if(e->eqform)
	{
		qtk_aec_delete(e->eqform);
		e->eqform = NULL;
	}
	if(e->cfg)
	{
		if(e->param.use_bin)
		{
			qtk_aec_cfg_delete_bin(e->cfg);
		}else{
			qtk_aec_cfg_delete(e->cfg);
		}
		e->cfg = NULL;
	}
	qtk_engine_param_clean(&e->param);

	wtk_free(e);
	return 0;
}


int qtk_engineaec_on_start(qtk_engineaec_t *e)
{
	return qtk_aec_start(e->eqform);
}

int qtk_engineaec_on_feed(qtk_engineaec_t *e,char *data,int len)
{
	int ret;
	ret = qtk_aec_feed(e->eqform, data, len, 0);
	return ret;
}

int qtk_engineaec_on_end(qtk_engineaec_t *e)
{
	return qtk_aec_feed(e->eqform, NULL, 0, 1);
}

void qtk_engineaec_on_reset(qtk_engineaec_t *e)
{
	qtk_aec_reset(e->eqform);
}

void qtk_engineaec_on_set_notify(qtk_engineaec_t *e,void *notify_ths,qtk_engine_notify_f notify_f)
{
	e->notify_ths = notify_ths;
	e->notify     = notify_f;
}

int qtk_engineaec_on_set(qtk_engineaec_t *e,char *data,int bytes)
{
	wtk_cfg_file_t *cfile = NULL;
	wtk_cfg_item_t *item;
	wtk_queue_node_t *qn;
	int ret;

	wtk_log_log(e->session->log,"set param = %.*s\n",bytes,data);
	cfile = wtk_cfg_file_new();
	if(!cfile) {
		return -1;
	}

	ret = wtk_cfg_file_feed(cfile,data,bytes);
	if(ret != 0) {
		goto end;
	}

	// for(qn=cfile->main->cfg->queue.pop;qn;qn=qn->next) {
	// 	item = data_offset2(qn,wtk_cfg_item_t,n);
	// 	if(wtk_string_cmp2(item->key,&qtk_engine_set_str[16]) == 0) {
	// 		qtk_aec_set_agcenable(e->eqform->vebf3, atoi(item->value.str->data));
	// 	} else if (wtk_string_cmp2(item->key,&qtk_engine_set_str[17]) == 0) {
	// 		qtk_aec_set_echoenable(e->eqform->vebf3, atoi(item->value.str->data));
	// 	} else if (wtk_string_cmp2(item->key,&qtk_engine_set_str[18]) == 0) {
	// 		qtk_aec_set_denoiseenable(e->eqform->vebf3, atoi(item->value.str->data));
	// 	}
	// }
	
end:
	if(cfile) {
		wtk_cfg_file_delete(cfile);
	}
	return 0;
}


int qtk_engineaec_start(qtk_engineaec_t *e)
{
	int ret;

	if(e->param.use_thread){
		qtk_engine_thread_start(e->thread);
	}else{
		qtk_engineaec_on_start(e);
	}
	ret = 0;
	return ret;
}

int qtk_engineaec_feed(qtk_engineaec_t *e,char *data,int bytes,int is_end)
{
	if(e->param.use_thread)
	{
		qtk_engine_thread_feed(e->thread,data,bytes,is_end);
	}else{
		if(bytes > 0) {
			qtk_engineaec_on_feed(e,data,bytes);
		}
		if(is_end) {
			qtk_engineaec_on_end(e);
		}
	}
	return 0;
}

int qtk_engineaec_reset(qtk_engineaec_t *e)
{
	if(e->param.use_thread){
		qtk_engine_thread_reset(e->thread);
	}else{
		qtk_engineaec_on_reset(e);
	}

	return 0;
}

int qtk_engineaec_cancel(qtk_engineaec_t *e)
{
	if(e->param.use_thread) {
		qtk_engine_thread_cancel(e->thread);
	}
	return 0;
}

void qtk_engineaec_set_notify(qtk_engineaec_t *e,void *ths,qtk_engine_notify_f notify_f)
{
	if(e->param.use_thread) {
		qtk_engine_thread_set_notify(e->thread,ths,notify_f);
	} else {
		qtk_engineaec_on_set_notify(e,ths,notify_f);
	}
}

int qtk_engineaec_set(qtk_engineaec_t *e,char *data,int bytes)
{
	int ret = 0;

	if(e->param.use_thread) {
		qtk_engine_thread_set(e->thread,data,bytes);
	} else {
		ret = qtk_engineaec_on_set(e,data,bytes);
	}
	return ret;
}


void qtk_engineaec_on_data(qtk_engineaec_t *e,qtk_var_t *var)
{
	int i;
	if(e->notify){
		e->notify(e->notify_ths, var);
	}
	if(e->param.use_logwav && e->param.log_wav_path->len>0)
	{
        if(var->v.str.len>0 && var->type == QTK_SPEECH_DATA_PCM)
    	{
         	for(i=0;i<var->v.str.len;++i)
         	{
            	var->v.str.data[i]*=1;
         	}
        	wtk_wavfile_write(e->wav,(char *)var->v.str.data,var->v.str.len);       
      	}
	}
}


