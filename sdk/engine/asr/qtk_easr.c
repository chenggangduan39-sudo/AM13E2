#include "qtk_easr.h"
#include "sdk/engine/comm/qtk_engine_hdr.h"

#ifdef USE_ASR_CUMULA
qtk_easr_msg_t* qtk_easr_msg_new(qtk_easr_t *e)
{
	qtk_easr_msg_t *msg;

	msg = (qtk_easr_msg_t*)wtk_malloc(sizeof(qtk_easr_msg_t));
	return msg;
}

int qtk_easr_msg_delete(qtk_easr_msg_t *msg)
{
	wtk_free(msg);
	return 0;
}

qtk_easr_msg_t* qtk_easr_pop_msg(qtk_easr_t *e,qtk_easr_type_t type,char *data,int len)
{
	qtk_easr_msg_t *msg;

	msg = (qtk_easr_msg_t*) wtk_lockhoard_pop(&e->msg_hoard);
	msg->type = type;
	if(len > 0) {
		msg->v = wtk_string_dup_data(data,len);
	} else {
		msg->v = NULL;
	}
	return msg;
}

void qtk_easr_push_msg(qtk_easr_t *e,qtk_easr_msg_t *msg)
{
	if(msg->v) {
		wtk_string_delete(msg->v);
	}
	wtk_lockhoard_push(&e->msg_hoard,msg);
}
#endif

void qtk_easr_init(qtk_easr_t *e)
{
	qtk_engine_param_init(&e->param);

	e->session = NULL;

	e->c   = NULL;
	e->cfg = NULL;

	e->notify_f   = NULL;
	e->notify_ths = NULL;

	e->callback = NULL;
	e->thread   = NULL;

	e->cancel = 0;
	e->ended = 0;

#ifdef USE_ASR_CUMULA
	e->cumula = 0;
#endif
}

int qtk_easr_on_start(qtk_easr_t *e);
int qtk_easr_on_data(qtk_easr_t *e,char *data,int bytes);
int qtk_easr_on_end(qtk_easr_t *e);
void qtk_easr_on_reset(qtk_easr_t *e);
void qtk_easr_on_set_notify(qtk_easr_t *e,void *notify_ths,qtk_engine_notify_f notify_f);
void qtk_easr_on_cancel(qtk_easr_t *e);
void qtk_easr_on_err(qtk_easr_t *e);
static void qtk_easr_on_set(qtk_easr_t *e,char *data,int bytes);

void qtk_easr_on_hint_notify(qtk_easr_t *e,char *data,int bytes)
{
	qtk_var_t var;

	if(e->notify_f){
		var.type = QTK_ASR_HINT;
		var.v.str.data = data;
		var.v.str.len = bytes;
		e->notify_f(e->notify_ths,&var);
	}
}

void qtk_easr_on_spxfinal_notify(qtk_easr_t *e,char *data,int bytes)
{
	qtk_var_t var;

	if(e->notify_f){
		var.type = QTK_ASR_TEXT;
		var.v.str.data = data;
		var.v.str.len = bytes;
		e->notify_f(e->notify_ths,&var);
	}
}

// 热词回调
void qtk_easr_on_hw_notify(qtk_easr_t *e, char *data, int bytes)
{
	qtk_var_t var;
	if(e->notify_f) {
		var.type = QTK_ASR_HOTWORD;
		var.v.str.data = data;
		var.v.str.len = bytes;
		e->notify_f(e->notify_ths, &var);
	}	
}

void qtk_easr_on_result_notify(qtk_easr_t *e, char *data, int bytes, float fs, float fe, int type)
{
	qtk_var_t var;
	if(e->notify_f)
	{
		if(type)
		{
			var.type = QTK_AEC_WAKE;
			var.v.wakeff.fs = fs;
			var.v.wakeff.fe = fe;
		}else{
			var.type = QTK_ASR_TEXT;
			var.v.str.data = data;
			var.v.str.len = bytes;
		}
		e->notify_f(e->notify_ths, &var);
	}
}

qtk_easr_t* qtk_easr_new(qtk_session_t *s,wtk_local_cfg_t *params)
{
	qtk_easr_t *e;
	int buf_size;
	int ret;

	e = (qtk_easr_t*)wtk_malloc(sizeof(qtk_easr_t));
	qtk_easr_init(e);
	e->session = s;

	qtk_engine_param_set_session(&e->param,e->session);
	ret = qtk_engine_param_feed(&e->param,params);
	if(ret != 0) {
		wtk_log_warn0(e->session->log,"params als failed.");
		goto end;
	}

	e->asr_min_bytes = e->param.asr_min_time * 32;
	e->asr_max_bytes = e->param.asr_max_time * 32;

	if(e->param.use_bin) {
		e->cfg = qtk_asr_cfg_new_bin(e->param.cfg);
	} else {
		e->cfg = qtk_asr_cfg_new(e->param.cfg);
	}
	if(!e->cfg) {
		wtk_log_warn0(e->session->log,"cfg new failed.");
		_qtk_error(e->session,_QTK_CFG_NEW_FAILED);
		ret = -1;
		goto end;
	}

	qtk_asr_cfg_update_option(e->cfg,&s->opt);
	/**
	 * rec_min_conf
	 * usrec_min_conf
	 * grammar_min_conf
	 * use_json
	 * env
	 * usr_ebnf
	 * keyword_fn
	 */
	qtk_asr_cfg_update_params(e->cfg,params);

	e->c = qtk_asr_new(e->cfg,s);
	if(!e->c) {
		wtk_log_warn0(e->session->log,"asr new failed.");
		_qtk_error(e->session,_QTK_INSTANSE_NEW_FAILED);
		ret = -1;
		goto end;
	}
	qtk_asr_set_hint_notify(e->c ,e,(qtk_iasr_set_hint_notify_f)qtk_easr_on_hint_notify);
	qtk_asr_set_spxfinal_notify(e->c ,e,(qtk_iasr_set_hint_notify_f)qtk_easr_on_spxfinal_notify);
	qtk_asr_set_hw_notify(e->c, e, (qtk_iasr_set_hw_notify_f)qtk_easr_on_hw_notify);
	qtk_asr_set_result_notify(e->c, e, (qtk_iasr_set_result_f)qtk_easr_on_result_notify);
#ifdef USE_ASR_CUMULA
	wtk_blockqueue_init(&e->cache_q);
	wtk_lockhoard_init(&e->msg_hoard,offsetof(qtk_easr_msg_t,hoard_n),20,
			(wtk_new_handler_t)qtk_easr_msg_new,
			(wtk_delete_handler_t)qtk_easr_msg_delete,
			e
			);
#endif

	if(e->param.use_thread) {
		buf_size = 32 * e->param.winStep;

		e->callback = qtk_engine_callback_new();
		e->callback->start_f      = (qtk_engine_thread_start_f)      qtk_easr_on_start;
		e->callback->data_f       = (qtk_engine_thread_data_f)       qtk_easr_on_data;
		e->callback->end_f        = (qtk_engine_thread_end_f)        qtk_easr_on_end;
		e->callback->reset_f      = (qtk_engine_thread_reset_f)      qtk_easr_on_reset;
		e->callback->set_notify_f = (qtk_engine_thread_set_notify_f) qtk_easr_on_set_notify;
		e->callback->cancel_f     = (qtk_engine_thread_cancel_f)     qtk_easr_on_cancel;
		e->callback->set_f        = (qtk_engine_thread_set_f)        qtk_easr_on_set;
		e->callback->err_f        = (qtk_engine_thread_err_f)        qtk_easr_on_err;
		e->callback->ths          = e;

		e->thread = qtk_engine_thread_new(
				e->callback,
				e->session->log,
				"easr",
				buf_size,
				20,
				1,
				e->param.syn
				);
	}

	ret = 0;
end:
	wtk_log_log(e->session->log,"ret = %d",ret);
	if(ret != 0) {
		qtk_easr_delete(e);
		e = NULL;
	}
	return e;
}

int qtk_easr_delete(qtk_easr_t *e)
{
#ifdef USE_ASR_CUMULA
	wtk_log_log(e->session->log,"cumula = %d\n",e->cumula);
	//wtk_debug("delete cumula = %d\n",e->cumula);
#endif
	if(e->thread) {
		qtk_engine_thread_delete(e->thread,0);
	}
	if(e->callback) {
		qtk_engine_callback_delete(e->callback);
	}
	if(e->c)
	{
		qtk_asr_delete(e->c);
	}
	if(e->cfg)
	{
		if(e->param.use_bin)
		{
			qtk_asr_cfg_delete_bin(e->cfg);
		}else{
			qtk_asr_cfg_delete(e->cfg);
		}
	}
	qtk_engine_param_clean(&e->param);
	wtk_free(e);
	return 0;
}

void qtk_easr_set_notify(qtk_easr_t *e,void *notify_ths,qtk_engine_notify_f notify_f)
{
	wtk_log_log(e->session->log,"ths = %p and notify = %p.",notify_ths,notify_f);
	if(e->param.use_thread) {
		qtk_engine_thread_set_notify(e->thread,notify_ths,notify_f);
	} else {
		qtk_easr_on_set_notify(e,notify_ths,notify_f);
	}
}

int qtk_easr_on_start(qtk_easr_t *e)
{
	e->asr_bytes = 0;
	e->ended = 0;
	return qtk_asr_start(e->c);
}

int qtk_easr_on_data(qtk_easr_t *e,char *data,int bytes)
{
	int is_end=0;

	if(e->ended) {
		wtk_log_log(e->session->log,"skip data ended = %d",e->ended);
		return 0;
	}

	if(e->cfg->iasrs->type != QTK_IASR_GR_NEW)
	{
		e->asr_bytes += bytes;
		is_end = e->asr_bytes >= e->asr_max_bytes ? 1 : 0;
		if(is_end) {
			e->ended = 1;
			wtk_log_log(e->session->log,"audio too long = %d / %d",e->asr_bytes,e->asr_max_bytes);
		}
	}
	return qtk_asr_feed(e->c,data,bytes,is_end);
}

int qtk_easr_on_end(qtk_easr_t *e)
{
	qtk_var_t var;
        wtk_string_t v;
        int ret;

        if (e->cancel) {
                return 0;
        }
#ifdef USE_ASR_CUMULA
		--e->cumula;
#endif

	if(e->c->cfg->iasrs->type != QTK_IASR_GR_NEW)
	{
		if(e->asr_bytes <= e->asr_min_bytes) {
			wtk_log_log(e->session->log,"audio too short = %d  /  %d\n",e->asr_bytes,e->asr_min_bytes);
			var.type = QTK_VAR_ERR;
			var.v.i = _QTK_ASR_AUDIO_SHORT;
			_qtk_error(e->session, _QTK_ASR_AUDIO_SHORT);
			e->notify_f(e->notify_ths,&var);
			qtk_asr_cancel(e->c);
			return 0;
		}
	}

	if(!e->ended) {
		ret = qtk_asr_feed(e->c,0,0,1);
		if(ret != 0) {
			wtk_log_warn(e->session->log,"ret = %d",ret);
			return -1;
		}
	}
	if(e->c->cfg->iasrs->type != QTK_IASR_GR_NEW)
	{
		v = qtk_asr_get_result(e->c);

		if(e->cancel){
			return 0;
		}
#ifdef USE_TIME_TEST
//	printf("delay:%f\n",(time_get_ms()-tm1));
	wtk_log_log(e->session->log,"===>asr time delay:%f",(time_get_ms()-tm1));
#endif
		if(v.len > 0 && e->notify_f) {

			var.type = QTK_ASR_TEXT;
			var.v.str.data = v.data;
			var.v.str.len  = v.len;
			e->notify_f(e->notify_ths,&var);
			return 0;
		}else{
			return -1;
		}
	}else{
		return 0;
	}
}

void qtk_easr_on_reset(qtk_easr_t *e)
{
	qtk_asr_reset(e->c);
}

void qtk_easr_on_cancel(qtk_easr_t *e)
{
	qtk_asr_cancel(e->c);
	--e->cancel;
}

void qtk_easr_on_err(qtk_easr_t *e)
{
	qtk_var_t var;

#ifdef USE_ASR_CUMULA
	e->cancel ? 0 : --e->cumula;
#endif
	if(!e->notify_f){
		wtk_log_warn(e->session->log,"notify_f = %p",e->notify_f);
		_qtk_warning(e->session,_QTK_ENGINE_NOTIFY_INVALID);
		return;
	}
	switch(e->c->err){
	case QTK_ASR_NOT_CREDIBLE:
		var.type = QTK_VAR_ERR;
		var.v.i = _QTK_ASR_NOT_CREDIBLE;
		_qtk_error(e->session, _QTK_ASR_NOT_CREDIBLE);
		e->notify_f(e->notify_ths,&var);
		break;
	case QTK_ASR_INVALID_AUDIO:
		var.type = QTK_VAR_ERR;
		var.v.i = _QTK_ASR_INVALID_AUDIO;
		_qtk_error(e->session, _QTK_ASR_INVALID_AUDIO);
		e->notify_f(e->notify_ths,&var);
		break;
	default:
		var.type = QTK_VAR_ERR;
		e->notify_f(e->notify_ths,&var);
		break;
	}
}

void qtk_easr_on_set_notify(qtk_easr_t *e,void *notify_ths,qtk_engine_notify_f notify_f)
{
	e->notify_ths = notify_ths;
	e->notify_f = notify_f;
}

int qtk_easr_start(qtk_easr_t *e)
{
	int ret = 0;

#ifdef USE_ASR_CUMULA
	qtk_easr_msg_t *msg;

	if(e->param.use_thread) {
		if(qtk_easr_is_cumula(e->cumula)) {
			wtk_log_log(e->session->log,"cumula = %d",e->cumula);
			msg = qtk_easr_pop_msg(e,QTK_EASR_START,NULL,0);
			wtk_blockqueue_push(&e->cache_q,&msg->q_n);
		} else {
			qtk_engine_thread_start(e->thread);
		}
	} else {
		ret = qtk_easr_on_start(e);
		if(ret!=0){
			qtk_easr_on_err(e);
		}
	}
#else
	if(e->param.use_thread) {
		qtk_engine_thread_start(e->thread);
	} else {
		ret = qtk_easr_on_start(e);
		if(ret!=0){
			qtk_easr_on_err(e);
		}
	}
#endif
	return ret;
}


int qtk_easr_feed(qtk_easr_t *e,char *data,int bytes,int is_end)
{
#ifdef USE_ASR_CUMULA
	qtk_var_t var;
	qtk_easr_msg_t *msg;
	wtk_queue_node_t *qn;
#endif
	int ret = 0;

	if(e->param.use_thread) {
#ifdef USE_ASR_CUMULA
		if(qtk_easr_is_cumula(e->cumula)) {
			if(bytes > 0) {
				wtk_log_log(e->session->log,"feed bytes %d and cumula = %d",bytes,e->cumula);
				//wtk_debug("=========================> cumula %d and feed cache\n",e->cumula);
				msg = qtk_easr_pop_msg(e,QTK_EASR_DATA,data,bytes);
				wtk_blockqueue_push(&e->cache_q,&msg->q_n);
			}
			if(is_end) {
				wtk_log_log(e->session->log,"cumula full and skip %d vad_audio",e->cumula);

				msg = qtk_easr_pop_msg(e,QTK_EASR_END,NULL,0);
				wtk_blockqueue_push(&e->cache_q,&msg->q_n);
				var.type = QTK_VAR_ERR;
				var.v.i = _QTK_ASR_CUMULA_FULL;
				e->notify_f(e->notify_ths,&var);
				qtk_easr_cancel(e);
				while(1) {
					qn = wtk_blockqueue_pop(&e->cache_q,0,NULL);
					if(!qn) {
						break;
					}
					msg = data_offset2(qn,qtk_easr_msg_t,q_n);
					switch(msg->type) {
					case QTK_EASR_START:
						qtk_engine_thread_start(e->thread);
						break;
					case QTK_EASR_DATA:
						qtk_engine_thread_feed(e->thread,msg->v->data,msg->v->len,0);
						break;
					case QTK_EASR_END:
						qtk_engine_thread_feed(e->thread,NULL,0,1);
						++e->cumula;
						break;
					}
					qtk_easr_push_msg(e,msg);
				}
			}

		} else {
			if(e->cache_q.length > 0) {
				while(1) {
					qn = wtk_blockqueue_pop(&e->cache_q,0,NULL);
					if(!qn) {
						break;
					}
					msg = data_offset2(qn,qtk_easr_msg_t,q_n);
					switch(msg->type) {
					case QTK_EASR_START:
						qtk_engine_thread_start(e->thread);
						break;
					case QTK_EASR_DATA:
						qtk_engine_thread_feed(e->thread,msg->v->data,msg->v->len,0);
						break;
					default:
						wtk_log_warn0(e->session->log,"easr cache queue unexpect end");
						exit(1);
						break;
					}
					qtk_easr_push_msg(e,msg);
				}
			}
			qtk_engine_thread_feed(e->thread,data,bytes,is_end);
			if(is_end) {
				++e->cumula;
			}
		}
#else
		qtk_engine_thread_feed(e->thread,data,bytes,is_end);
#endif
	} else {
		if(bytes > 0) {
			ret = qtk_easr_on_data(e,data,bytes);
			if(ret !=0){
				qtk_easr_on_err(e);
				goto end;
			}
		}
		if(is_end)
		{
			ret = qtk_easr_on_end(e);
			if(ret !=0){
				qtk_easr_on_err(e);
				goto end;
			}
			e->delay_tm = time_get_ms();
		}
	}

end:
	return ret;
}

int qtk_easr_reset(qtk_easr_t *e)
{
	if(e->param.use_thread) {
		qtk_engine_thread_reset(e->thread);
	} else {
		qtk_easr_on_reset(e);
	}
	return 0;
}

int qtk_easr_cancel(qtk_easr_t *e)
{
	++e->cancel;
#ifdef USE_ASR_CUMULA
	e->cumula = 0;
#endif
	if(e->param.use_thread) {
		qtk_engine_thread_cancel(e->thread);
	} else {
		qtk_easr_on_cancel(e);
	}
	return 0;
}

static void qtk_easr_on_set(qtk_easr_t *e,char *data,int bytes)
{
	wtk_cfg_file_t *cfile = NULL;
	wtk_cfg_item_t *item;
	wtk_queue_node_t *qn;
	int upload = 0;
	int update = 0;
	int gethotword = 0;
	int ret;

	cfile = wtk_cfg_file_new();
	if(!cfile) {
		return;
	}

	wtk_debug("%.*s\n", bytes, data);
 	ret = wtk_cfg_file_feed(cfile,data,bytes);
	if(ret != 0 ) {
		goto end;
	}

	for(qn=cfile->main->cfg->queue.pop;qn;qn=qn->next) {
		item = data_offset2(qn,wtk_cfg_item_t,n);
		if(wtk_string_cmp_s(item->key, "upload") == 0) {
			upload = wtk_str_atoi(item->value.str->data, item->value.str->len);
		}
		if(wtk_string_cmp_s(item->key, "update") == 0) {
			update = wtk_str_atoi(item->value.str->data, item->value.str->len);
		}
		if(wtk_string_cmp_s(item->key, "gethotword") == 0) {
			gethotword = wtk_str_atoi(item->value.str->data, item->value.str->len);
			if(gethotword == 1) {
				qtk_asr_hw_get_hotword(e->c);
			}
		}
		if(wtk_string_cmp_s(item->key, "hw_res") == 0) {
			if(upload == 1) {
				qtk_asr_hw_upload(e->c, item->value.str->data, 0);
			}
			if(update == 1) {
				qtk_asr_hw_update(e->c, item->value.str->data, 0);
			}
		}
		if(wtk_string_cmp_s(item->key, "hotword") == 0) {
			if(upload == 1) {
				qtk_asr_hw_upload(e->c, item->value.str->data, 1);
			}
			if(update == 1) {
				qtk_asr_hw_update(e->c, item->value.str->data, 1);
			}
		}
		if(wtk_string_cmp_s(item->key, "idle_time") == 0){
			qtk_asr_set_idle_time(e->c, wtk_str_atoi(item->value.str->data, item->value.str->len));
		}
		if(wtk_string_cmp2(item->key, &qtk_engine_set_str[11]) == 0){
			qtk_asr_set_coreType(e->c, item->value.str->data, item->value.str->len);
		}
		if(wtk_string_cmp2(item->key, &qtk_engine_set_str[12]) == 0){
			qtk_asr_set_res(e->c, item->value.str->data, item->value.str->len);
		}
		if(wtk_string_cmp2(item->key, &qtk_engine_set_str[14]) == 0){
			qtk_asr_set_skip_space(e->c, wtk_str_atoi(item->value.str->data, item->value.str->len));
		}

	}

end:
	if(cfile) {
		wtk_cfg_file_delete(cfile);
	}
}

int qtk_easr_set(qtk_easr_t *e,char *data,int bytes)
{
	if(e->param.use_thread) {
		qtk_engine_thread_set(e->thread,data,bytes);
	} else {
		qtk_easr_on_set(e,data,bytes);
	}
	return 0;
}

int qtk_easr_set_xbnf(qtk_easr_t *e, char *data, int len)
{
	qtk_asr_set_xbnf(e->c, data, len);
	return 0;
}

int qtk_easr_update_cmds(qtk_easr_t* e,char* words,int len){
	int ret = qtk_asr_update_cmds(e->c,words,len);
	return ret;
}

void qtk_easr_get_result(qtk_easr_t *e, qtk_var_t *var)
{
	wtk_string_t rst;
	rst = qtk_asr_get_result(e->c);
	var->type = QTK_ASR_TEXT;
	var->v.str.len = rst.len;
	var->v.str.data = rst.data;

	e->delay_tm = time_get_ms() - e->delay_tm;
	qtk_var_t dlv;
	if(e->notify_f)
	{
		dlv.type = QTK_ASR_DELAY;
		dlv.v.f = e->delay_tm;
		e->notify_f(e->notify_ths, &dlv);
	}
}