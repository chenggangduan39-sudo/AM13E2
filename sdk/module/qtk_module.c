#include "qtk_module.h" 

int qtk_module_count(int count, char *binfn)
{
	wtk_rbin2_t *rbin;
    wtk_rbin2_item_t *item;
    int ret;
    char *cfg_fn="./cfg";
    wtk_rbin2_t *rbwrite;
    wtk_string_t name;
	int cc;
	wtk_strbuf_t *inbuf;

	// wtk_debug("binfn=%s\n",binfn);

    rbin = wtk_rbin2_new();
	ret = wtk_rbin2_read(rbin, binfn);
	if(ret != 0){
        wtk_debug("error read\n");
	}
	item = wtk_rbin2_get2(rbin, cfg_fn, strlen(cfg_fn));
	if(!item){
		wtk_debug("%s not found\n", cfg_fn);
		ret = -1;
	}
    printf("count=[%.*s]\n",item->data->len,item->data->data);
	cc=wtk_str_atoi2(item->data->data,item->data->len,NULL);
	if(cc > count)
	{
		wtk_rbin2_delete(rbin);
		return -1;
	}

    wtk_rbin2_delete(rbin);

	cc++;
    wtk_string_set(&name,"cfg",sizeof("cfg")-1);
    
    rbwrite = wtk_rbin2_new();
    char *in;
    in = wtk_itoa(cc);

	wtk_heap_add_large(rbwrite->heap,in,strlen(in));
	wtk_rbin2_add2(rbwrite,&name,in,strlen(in));
    wtk_rbin2_write(rbwrite, binfn);

    wtk_rbin2_delete(rbwrite);
	return 0;
}

#ifdef USE_MODULE_MULT
int qtk_module_init_mult(qtk_module_t *m,char *fn)
{
	int ret;

	m->main_cfg = wtk_main_cfg_new_type(qtk_mult_mod_cfg,fn);
	if(!m->main_cfg) {
		ret = -1;
		goto end;
	}

	m->new_f                = (qtk_module_new_f)qtk_mult_mod_new;
	m->delete_f             = (qtk_module_delete_f)qtk_mult_mod_delete;
	m->set_notify_f         = (qtk_module_set_notify_f)qtk_mult_mod_set_notify;
	m->start_f              = (qtk_module_start_f)qtk_mult_mod_start;
	m->stop_f               = (qtk_module_stop_f)qtk_mult_mod_stop;
	m->set_f				= (qtk_module_set_f)qtk_mult_mod_set;
#ifdef OFFLINE_TEST
	m->feed_f               = (qtk_module_feed_f)qtk_mult_mod_feed;
#endif
	m->rtype                = QTK_MODULE_MULT;

	ret = 0;
end:
	return ret;
}
#endif

#ifdef USE_MODULE_MQFORM
int qtk_module_init_mqform(qtk_module_t *m,char *fn)
{
	int ret;

	m->main_cfg = wtk_main_cfg_new_type(qtk_mqform_mod_cfg,fn);
	if(!m->main_cfg) {
		ret = -1;
		goto end;
	}

	m->new_f                = (qtk_module_new_f)qtk_mqform_mod_new;
	m->delete_f             = (qtk_module_delete_f)qtk_mqform_mod_delete;
	m->set_notify_f         = (qtk_module_set_notify_f)qtk_mqform_mod_set_notify;
	m->start_f              = (qtk_module_start_f)qtk_mqform_mod_start;
	m->stop_f               = (qtk_module_stop_f)qtk_mqform_mod_stop;
	m->set_f				= (qtk_module_set_f)qtk_mqform_mod_set;
#ifdef OFFLINE_TEST
	m->feed_f               = (qtk_module_feed_f)qtk_mqform_mod_feed;
#endif
	m->rtype                = QTK_MODULE_MQFORM;

	ret = 0;
end:
	return ret;
}
#endif

#ifdef USE_MODULE_MQFORM2
int qtk_module_init_mqform2(qtk_module_t *m,char *fn)
{
	int ret;

	m->main_cfg = wtk_main_cfg_new_type(qtk_mqform2_mod_cfg,fn);
	if(!m->main_cfg) {
		ret = -1;
		goto end;
	}

	m->new_f                = (qtk_module_new_f)qtk_mqform2_mod_new;
	m->delete_f             = (qtk_module_delete_f)qtk_mqform2_mod_delete;
	m->set_notify_f         = (qtk_module_set_notify_f)qtk_mqform2_mod_set_notify;
	m->start_f              = (qtk_module_start_f)qtk_mqform2_mod_start;
	m->stop_f               = (qtk_module_stop_f)qtk_mqform2_mod_stop;
#ifdef OFFLINE_TEST
	m->feed_f               = (qtk_module_feed_f)qtk_mqform2_mod_feed;
#endif
	m->rtype                = QTK_MODULE_MQFORM2;

	ret = 0;
end:
	return ret;
}
#endif

#ifdef USE_MODULE_MGAINNET
int qtk_module_init_mgainnet(qtk_module_t *m,char *fn)
{
	int ret;

	m->main_cfg = wtk_main_cfg_new_type(qtk_mgainnet_mod_cfg,fn);
	if(!m->main_cfg) {
		ret = -1;
		goto end;
	}

	m->new_f                = (qtk_module_new_f)qtk_mgainnet_mod_new;
	m->delete_f             = (qtk_module_delete_f)qtk_mgainnet_mod_delete;
	m->set_notify_f         = (qtk_module_set_notify_f)qtk_mgainnet_mod_set_notify;
	m->start_f              = (qtk_module_start_f)qtk_mgainnet_mod_start;
	m->stop_f               = (qtk_module_stop_f)qtk_mgainnet_mod_stop;
#ifdef OFFLINE_TEST
	m->feed_f               = (qtk_module_feed_f)qtk_mgainnet_mod_feed;
#endif
	m->rtype                = QTK_MODULE_MGAINNET;

	ret = 0;
end:
	return ret;
}
#endif

#ifdef USE_MODULE_QFORM
int qtk_module_init_qform(qtk_module_t *m,char *fn)
{
	int ret;

	m->main_cfg = wtk_main_cfg_new_type(qtk_mqform_cfg,fn);
	if(!m->main_cfg) {
		ret = -1;
		goto end;
	}

	m->new_f                = (qtk_module_new_f)qtk_mqform_new;
	m->delete_f             = (qtk_module_delete_f)qtk_mqform_delete;
	m->set_notify_f         = (qtk_module_set_notify_f)qtk_mqform_set_notify;
	m->start_f              = (qtk_module_start_f)qtk_mqform_start;
	m->stop_f               = (qtk_module_stop_f)qtk_mqform_stop;
#ifdef OFFLINE_TEST
	m->feed_f               = (qtk_module_feed_f)qtk_mqform_feed;
#endif
	m->rtype                = QTK_MODULE_QFORM;

	ret = 0;
end:
	return ret;
}
#endif

int qtk_module_init_hash(qtk_module_t *m,char *res,char *cfg_fn)
{
	qtk_module_init_func func;
	int ret;

#ifdef USE_MODULE_MULT
	wtk_str_hash_add_s(m->init_hash,"ult",qtk_module_init_mult);
#endif

#ifdef USE_MODULE_QFORM
	wtk_str_hash_add_s(m->init_hash,"qform",qtk_module_init_qform);
#endif

#ifdef USE_HAOYI

#ifdef USE_MODULE_MQFORM2
	wtk_str_hash_add_s(m->init_hash,"mqform",qtk_module_init_mqform2);
#endif

#ifdef USE_MODULE_MQFORM
	wtk_str_hash_add_s(m->init_hash,"mqform2",qtk_module_init_mqform);
#endif

#else
#ifdef USE_MODULE_MQFORM
	wtk_str_hash_add_s(m->init_hash,"mqform",qtk_module_init_mqform);
#endif

#ifdef USE_MODULE_MQFORM2
	wtk_str_hash_add_s(m->init_hash,"mqform2",qtk_module_init_mqform2);
#endif
#endif

#ifdef USE_MODULE_MGAINNET
	wtk_str_hash_add_s(m->init_hash,"mgainnet",qtk_module_init_mgainnet);
#endif

	ret = wtk_file_exist(cfg_fn);
	if(ret != 0) {
		wtk_log_warn(m->session->log,"cfg fn [%s] not exist.",cfg_fn);
		_qtk_error(m->session,_QTK_CFG_NOTEXIST);
		goto end;
	}

	ret = wtk_file_readable(cfg_fn);
	if(ret != 0) {
		wtk_log_warn(m->session->log,"cfg_fn [%s] not readable.",cfg_fn);
		_qtk_error(m->session,_QTK_CFG_UNREADABLE);
		goto end;
	}

	wtk_log_log(m->session->log,"cfg_fn = %s.",cfg_fn);
	wtk_log_log(m->session->log,"res = %s.",res);
	func = wtk_str_hash_find(m->init_hash,res,strlen(res));
	if(func){
		// 执行模块指针函数初始化
		ret = func(m,cfg_fn);
		if(ret != 0) {
			wtk_log_warn0(m->session->log,"cfg new failed.");
			_qtk_error(m->session,_QTK_CFG_NEW_FAILED);
			goto end;
		}
	}else{
		wtk_log_log(m->session->log,"module has no res[%s].",res);
		_qtk_error(m->session,_QTK_MODULE_ROLE_INVALID);
		ret = -1;
		goto end;
	}

	ret = 0;
end:
	return ret;
}

void qtk_module_init(qtk_module_t *m)
{
	m->session   = NULL;

	m->init_hash = NULL;
	m->main_cfg  = NULL;

	m->new_f                = NULL;
	m->delete_f             = NULL;
	m->start_f              = NULL;
	m->stop_f               = NULL;
	m->feed_f               = NULL;
	m->set_f                = NULL;
	m->set_notify_f         = NULL;
	m->module_ths           = NULL;
	m->rtype				= -1;
}

qtk_module_t* qtk_module_new(qtk_session_t *session,char *res,char *cfg_fn)
{
	qtk_module_t *m;
	int ret;

	if(!session) {
		return NULL;
	}

	ret = qtk_session_check(session);
	if(ret != 0) {
		wtk_log_warn0(session->log,"session process failed");
		return NULL;
	}

	m = (qtk_module_t*)wtk_malloc(sizeof(qtk_module_t));
	qtk_module_init(m);
	m->session = session;

	m->init_hash = wtk_str_hash_new(7);
	ret = qtk_module_init_hash(m,res,cfg_fn);
	if(ret != 0) {
		goto end;
	}

#ifndef USE_MODULE_QFORM
#ifdef USE_QTKCOUNT
	int pos=strlen(cfg_fn);
	while(1)
	{
		if(cfg_fn[pos] == '/')
		{
			break;
		}
		pos--;
	}
	char tmp[128]={0};
	sprintf(tmp,"%.*s%s",pos+1,cfg_fn,"aecsys.bin");
	if(qtk_module_count(2000, tmp) != 0) {
		wtk_log_log0(session->log,"The dynamic library is out of number limit.");
		_qtk_error(session,_QTK_OUT_TIMELIMIT);
		goto end;
	}
	system("sync");
#endif
#endif

	m->module_ths = m->new_f(session,m->main_cfg->cfg);
	if(!m->module_ths) {
		_qtk_error(m->session,_QTK_INSTANSE_NEW_FAILED);
		ret = -1;
		goto end;
	}
	m->starttime = (int)time_get_ms();
	m->overtime = 3600*1000*2;

	ret = 0;
end:
	if(ret != 0) {
		qtk_module_delete(m);
		m = NULL;
	}
	return m;
}

void qtk_module_delete(qtk_module_t *m)
{
	if(m->module_ths && m->delete_f) {
		m->delete_f(m->module_ths);
	}

	if(m->main_cfg) {
		wtk_main_cfg_delete(m->main_cfg);
	}

	if(m->init_hash) {
		wtk_str_hash_delete(m->init_hash);
	}

	wtk_free(m);
}

int qtk_module_start(qtk_module_t *m)
{
	wtk_log_log(m->session->log, "==================>>>>>>>module start %d",__LINE__);
	return m->start_f(m->module_ths);
}

int qtk_module_stop(qtk_module_t *m)
{
	wtk_log_log(m->session->log, "==================>>>>>>>module stop %d",__LINE__);
	return m->stop_f(m->module_ths);
}

int qtk_module_feed(qtk_module_t *m,char *data,int bytes)
{
#ifdef USE_RUN_TIMELIMIT
	if((time_get_ms() - m->starttime) > m->overtime)
	{
		printf("Trial time timeout.\n");
		return -1;
	}
#endif
	if(!m->feed_f) {
		return -1;
	}
	return m->feed_f(m->module_ths,data,bytes);
}

int qtk_module_set(qtk_module_t *m,char *params)
{
	if(!m->set_f) {
		return -1;
	}
	return m->set_f(m->module_ths,params);
}

int qtk_module_set_notify(qtk_module_t *m,void *user_data,qtk_engine_notify_f notify)
{
	m->set_notify_f(m->module_ths,user_data,notify);
	return 0;
}

int qtk_module_set_audio_callback(qtk_module_t *m,
		void *user_data,
		qtk_recorder_start_func recorder_start,
		qtk_recorder_read_func  recorder_read,
		qtk_recorder_stop_func  recorder_stop,
		qtk_recorder_clean_func recorder_clean,
		qtk_player_start_func   player_start,
		qtk_player_write_func   player_write,
		qtk_player_stop_func    player_stop,
		qtk_player_clean_func   player_clean
		)
{
	if(!m->set_audio_callback_f) {
		return -1;
	}
	m->set_audio_callback_f(m->module_ths,user_data,
			recorder_start, recorder_read, recorder_stop, recorder_clean,
			player_start,   player_write,  player_stop,   player_clean
			);
	return 0;
}

qtk_module_avspeech_t* qtk_module_avspeech_new(qtk_session_t *session, char *params)
{

}

int qtk_module_avspeech_delete(qtk_module_avspeech_t *mv)
{

}

int qtk_module_avspeech_start(qtk_module_avspeech_t *mv)
{

}

int qtk_module_avspeech_stop(qtk_module_avspeech_t *mv)
{

}

int qtk_module_avspeech_feed_audio(qtk_module_avspeech_t *mv, char *data, int bytes)
{

}

int qtk_module_avspeech_feed_image(qtk_module_avspeech_t *mv, char *data)
{

}

int qtk_module_avspeech_set(qtk_module_avspeech_t *mv, char *params)
{

}

int qtk_module_avspeech_set_notify(qtk_module_avspeech_t *mv,void *user_data,qtk_engine_notify_f notify)
{
	
}