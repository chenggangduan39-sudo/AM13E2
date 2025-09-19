#include "qtk_option.h" 

void qtk_option_init(qtk_option_t *opt)
{
	opt->heap       = wtk_heap_new(2048);
	opt->rsa        = NULL;
	opt->public_key = NULL;
	opt->phrase     = NULL;

	opt->cache_path = NULL;
	opt->log_fn     = NULL;
	opt->dns_fn     = NULL;
	opt->auth_fn    = NULL;

	opt->appid     = NULL;
	opt->secretkey = NULL;
	opt->usrid     = NULL;

	opt->host = NULL;
	opt->port = NULL;
	opt->url1 = NULL;
	opt->url2 = NULL;
	opt->url_auth = NULL;
	opt->timeout = 2000;

	opt->cldlog_host = NULL;
	opt->cldlog_port = NULL;
	opt->cldlog_url  = NULL;
	opt->cldlog_timeout = QTK_OPTVAL_CLDLOG_TIMOUT;
	opt->use_cldlog = 0;

	opt->use_srvsel = 0;
	opt->use_cldhub = 0;
#ifndef USE_SIZHENG
#ifndef USE_3308
#ifndef USE_HUACHUANG
#ifndef USE_303
#ifndef USE_NOSERVER
	opt->use_srvsel = 1;
	opt->use_cldhub = 1;
#endif
#endif
#endif
#endif
#endif
	opt->use_auth_cache = 0;
	opt->use_log = 1;
	opt->log_wav = 0;
	opt->srvsel_update_period = 12;
	opt->use_timer = 0;
}


void qtk_option_clean(qtk_option_t *opt)
{
	if(opt->rsa) {
#ifdef USE_AUTH
		qtk_rsa_delete(opt->rsa);
#endif
	}
	if(opt->heap){
		wtk_heap_delete(opt->heap);
	}
}

int qtk_option_update(qtk_option_t *opt)
{
	char buf[256];
	int ret;

	if(opt->cache_path) {
		if(opt->use_auth_cache && !opt->auth_fn) {
			ret = snprintf(buf,256,"%.*s%c%s",opt->cache_path->len,opt->cache_path->data,DIR_SEP,QTK_OPTDEF_AUTH);
			opt->auth_fn = wtk_heap_dup_string2(opt->heap,buf,ret);
		}
		if(opt->use_log && !opt->log_fn) {
			ret = snprintf(buf,256,"%.*s%c%s",opt->cache_path->len,opt->cache_path->data,DIR_SEP, QTK_OPTDEF_LOG);
			opt->log_fn = wtk_heap_dup_string2(opt->heap,buf,ret);
 		}
		if(!opt->dns_fn) {
			ret = snprintf(buf,256,"%.*s%c%s",opt->cache_path->len,opt->cache_path->data,DIR_SEP, QTK_OPTDEF_DNS);
			opt->dns_fn = wtk_heap_dup_string2(opt->heap,buf,ret);
		}
	} else {
		opt->log_wav = 0;
	}

	if(!opt->public_key) {
		opt->public_key = wtk_heap_dup_string2(opt->heap,QTK_OPTVAL_PUBLIC_KEY,sizeof(QTK_OPTVAL_PUBLIC_KEY)-1);
	}
	if(!opt->phrase) {
		opt->phrase = wtk_heap_dup_string2(opt->heap,QTK_OPTVAL_PHRASE,sizeof(QTK_OPTVAL_PHRASE)-1);
	}
#ifndef USE_SIZHENG
#ifndef USE_3308
#ifndef USE_HUACHUANG
#ifndef USE_303
#ifndef USE_NOSERVER
	if(opt->host) {
		opt->use_srvsel = 0;
	}else{
		opt->use_srvsel=1;
	}
#endif
#endif
#endif
#endif
#endif

	if(!opt->port) {
		opt->port = wtk_heap_dup_string2(opt->heap,QTK_OPTVAL_PORT,sizeof(QTK_OPTVAL_PORT)-1);
	}
	if(!opt->url1) {
		opt->url1 = wtk_heap_dup_string2(opt->heap,QTK_OPTVAL_URL1,sizeof(QTK_OPTVAL_URL1)-1);
	}
	if(!opt->url2) {
		opt->url2 = wtk_heap_dup_string2(opt->heap,QTK_OPTVAL_URL2,sizeof(QTK_OPTVAL_URL2)-1);
	}
	if(!opt->url_auth) {
		opt->url_auth = wtk_heap_dup_string2(opt->heap,QTK_OPTVAL_URL_AUTH,sizeof(QTK_OPTVAL_URL_AUTH)-1);
	}

	if(opt->use_cldlog) {
		if(!opt->cldlog_host) {
			opt->cldlog_host = wtk_heap_dup_string2(opt->heap,QTK_OPTVAL_CLDLOG_HOST,sizeof(QTK_OPTVAL_CLDLOG_HOST)-1);
		}
		if(!opt->cldlog_port) {
			opt->cldlog_port = wtk_heap_dup_string2(opt->heap,QTK_OPTVAL_CLDLOG_PORT,sizeof(QTK_OPTVAL_CLDLOG_PORT)-1);
		}
		if(!opt->cldlog_url) {
			opt->cldlog_url = wtk_heap_dup_string2(opt->heap,QTK_OPTVAL_CLDLOG_URL,sizeof(QTK_OPTVAL_CLDLOG_URL)-1);
		}
	}
	opt->srvsel_update_period = opt->srvsel_update_period*60*60*1000;
	return 0;
}

int qtk_option_update_params(qtk_option_t *opt,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	v = wtk_local_cfg_find_string_s(lc,QTK_OPTKEY_PUBLIC_KEY);
	if(v) {
		opt->public_key = wtk_heap_dup_string2(opt->heap,v->data,v->len);
	}

	v = wtk_local_cfg_find_string_s(lc,QTK_OPTKEY_PHRASE);
	if(v) {
		opt->phrase = wtk_heap_dup_string2(opt->heap,v->data,v->len);
	}

	v = wtk_local_cfg_find_string_s(lc,QTK_OPTKEY_CACHE_PATH);
	if(v) {
		opt->cache_path = wtk_heap_dup_string2(opt->heap,v->data,v->len);
	}

	v = wtk_local_cfg_find_string_s(lc,QTK_OPTKEY_LOG);
	if(v) {
		opt->log_fn = wtk_heap_dup_string2(opt->heap,v->data,v->len);
	}

	v = wtk_local_cfg_find_string_s(lc,QTK_OPTKEY_DNS);
	if(v) {
		opt->dns_fn = wtk_heap_dup_string2(opt->heap,v->data,v->len);
	}

	v = wtk_local_cfg_find_string_s(lc,QTK_OPTKEY_AUTH);
	if(v) {
		opt->auth_fn = wtk_heap_dup_string2(opt->heap,v->data,v->len);
	}

	v = wtk_local_cfg_find_string_s(lc,QTK_OPTKEY_APPID);
	if(v) {
		opt->appid = wtk_heap_dup_string2(opt->heap,v->data,v->len);
	}

	v = wtk_local_cfg_find_string_s(lc,QTK_OPTKEY_SECRETKEY);
	if(v) {
		opt->secretkey = wtk_heap_dup_string2(opt->heap,v->data,v->len);
	}

	v = wtk_local_cfg_find_string_s(lc,QTK_OPTKEY_USRID);
	if(v) {
		opt->usrid = wtk_heap_dup_string2(opt->heap,v->data,v->len);
 	}

	v = wtk_local_cfg_find_string_s(lc,QTK_OPTKEY_HOST);
	if(v) {
		opt->host = wtk_heap_dup_string2(opt->heap,v->data,v->len);
	}

	v = wtk_local_cfg_find_string_s(lc,QTK_OPTKEY_PORT);
	if(v) {
		opt->port = wtk_heap_dup_string2(opt->heap,v->data,v->len);
 	}

	v = wtk_local_cfg_find_string_s(lc,QTK_OPTKEY_TIMEOUT);
	if(v) {
		opt->timeout = atoi(v->data);
	}

	v = wtk_local_cfg_find_string_s(lc,QTK_OPTKEY_SRVSEL_FLG);
	if(v) {
		opt->use_srvsel = atoi(v->data)==1?1:0;
	}

	v = wtk_local_cfg_find_string_s(lc,QTK_OPTKEY_AUTH_FLG);
	if(v) {
		opt->use_auth_cache = atoi(v->data)==1?1:0;
	}

	v = wtk_local_cfg_find_string_s(lc,QTK_OPTKEY_LOG_FLG);
	if(v) {
		opt->use_log = atoi(v->data)==1?1:0;
	}


	v = wtk_local_cfg_find_string_s(lc,QTK_OPTKEY_CLDHUB_FLG);
	if(v) {
		opt->use_cldhub = atoi(v->data)==1?1:0;
	}

	v = wtk_local_cfg_find_string_s(lc,QTK_OPTKEY_LOGWAV_FLG);
	if(v) {
		opt->log_wav = atoi(v->data)==1?1:0;
	}

	v = wtk_local_cfg_find_string_s(lc,QTK_OPTKEY_TIMER_FLG);
	if(v)  {
		opt->use_timer = atoi(v->data)==1?1:0;
	}

	v = wtk_local_cfg_find_string_s(lc,QTK_OPTKEY_CLDLOG_HOST_FLG);
	if(v) {
		opt->cldlog_host = wtk_heap_dup_string2(opt->heap,v->data,v->len);
	}

	v = wtk_local_cfg_find_string_s(lc,QTK_OPTKEY_CLDLOG_PORT_FLG);
	if(v) {
		opt->cldlog_port = wtk_heap_dup_string2(opt->heap,v->data,v->len);
	}

	v = wtk_local_cfg_find_string_s(lc,QTK_OPTKEY_CLDLOG_URL_FLG);
	if(v) {
		opt->cldlog_url = wtk_heap_dup_string2(opt->heap,v->data,v->len);
	}

	v = wtk_local_cfg_find_string_s(lc,QTK_OPTKEY_CLDLOG_TIMEOUT_FLG);
	if(v) {
		opt->cldlog_timeout = atoi(v->data);
	}

	v = wtk_local_cfg_find_string_s(lc,QTK_OPTKEY_CLDLOG_FLG);
	if(v) {
		opt->use_cldlog = atoi(v->data)==1?1:0;
	}

	qtk_option_update(opt);
	return 0;
}


void qtk_option_new_rsa(qtk_option_t *opt)
{
#ifdef USE_AUTH
	opt->rsa = qtk_rsa_new(opt->public_key->data,opt->phrase->data);
#endif
}

void qtk_option_set_usrid(qtk_option_t *opt,char *usrid,int len)
{
	opt->usrid = wtk_heap_dup_string2(opt->heap,usrid,len);
}

void qtk_option_set_host(qtk_option_t *opt,char *host,int len)
{
	opt->host = wtk_heap_dup_string2(opt->heap,host,len);
}

void qtk_option_set_port(qtk_option_t *opt,char *port,int len)
{
	opt->port = wtk_heap_dup_string2(opt->heap,port,len);
}

void qtk_option_print(qtk_option_t *opt,wtk_log_t *log)
{
	if(log) {
		wtk_log_log0(log,"========================= opt =======================");
		wtk_log_log(log,"rsa = %p",opt->rsa);
		if(opt->public_key) {
			wtk_log_log(log,"public key = %.*s",opt->public_key->len,opt->public_key->data);
		}
		if(opt->phrase) {
			wtk_log_log(log,"phrase = %.*s",opt->phrase->len,opt->phrase->data);
		}
		if(opt->cache_path) {
			wtk_log_log(log,"cache_path = %.*s",opt->cache_path->len,opt->cache_path->data);
		}
		if(opt->log_fn) {
			wtk_log_log(log,"log_fn = %.*s",opt->log_fn->len,opt->log_fn->data);
		}
		if(opt->dns_fn) {
			wtk_log_log(log,"dns_fn = %.*s",opt->dns_fn->len,opt->dns_fn->data);
		}
		if(opt->auth_fn) {
			wtk_log_log(log,"auth_fn = %.*s",opt->auth_fn->len,opt->auth_fn->data);
		}
		if(opt->appid) {
			wtk_log_log(log,"appid = %.*s",opt->appid->len,opt->appid->data);
		}
		if(opt->secretkey) {
			wtk_log_log(log,"secretkey = %.*s",opt->secretkey->len,opt->secretkey->data);
		}
		if(opt->usrid) {
			wtk_log_log(log,"usrid = %.*s",opt->usrid->len,opt->usrid->data);
		}
		if(opt->host) {
			wtk_log_log(log,"host = %.*s",opt->host->len,opt->host->data);
		}
		if(opt->port) {
			wtk_log_log(log,"port = %.*s",opt->port->len,opt->port->data);
		}
		if(opt->url1) {
			wtk_log_log(log,"url = %.*s",opt->url1->len,opt->url1->data);
		}
		if(opt->url2) {
			wtk_log_log(log,"url2 = %.*s",opt->url2->len,opt->url2->data);
		}
		wtk_log_log(log,"timeout = %d",opt->timeout);
		wtk_log_log(log,"use_srvsel = %d",opt->use_srvsel);
		wtk_log_log(log,"use_auth_cache = %d",opt->use_auth_cache);
		wtk_log_log(log,"use_log = %d",opt->use_log);
		wtk_log_log(log,"use_cldhub = %d",opt->use_cldhub);
		wtk_log_log(log,"log_wav = %d",opt->log_wav);
		wtk_log_log(log,"use_timer = %d",opt->use_timer);
		wtk_log_log0(log,"=====================================================");
	} else {
		wtk_debug("========================= opt =======================\n");
		wtk_debug("rsa = %p\n",opt->rsa);
		if(opt->public_key) {
			wtk_debug("public key = %.*s\n",opt->public_key->len,opt->public_key->data);
		}
		if(opt->phrase) {
			wtk_debug("phrase = %.*s\n",opt->phrase->len,opt->phrase->data);
		}
		if(opt->cache_path) {
			wtk_debug("cache_path = %.*s\n",opt->cache_path->len,opt->cache_path->data);
		}
		if(opt->log_fn) {
			wtk_debug("log_fn = %.*s\n",opt->log_fn->len,opt->log_fn->data);
		}
		if(opt->dns_fn) {
			wtk_debug("dns_fn = %.*s\n",opt->dns_fn->len,opt->dns_fn->data);
		}
		if(opt->auth_fn) {
			wtk_debug("auth_fn = %.*s\n",opt->auth_fn->len,opt->auth_fn->data);
		}
		if(opt->appid) {
			wtk_debug("appid = %.*s\n",opt->appid->len,opt->appid->data);
		}
		if(opt->secretkey) {
			wtk_debug("secretkey = %.*s\n",opt->secretkey->len,opt->secretkey->data);
		}
		if(opt->usrid) {
			wtk_debug("usrid = %.*s\n",opt->usrid->len,opt->usrid->data);
		}
		if(opt->host) {
			wtk_debug("host = %.*s\n",opt->host->len,opt->host->data);
		}
		if(opt->port) {
			wtk_debug("port = %.*s\n",opt->port->len,opt->port->data);
		}
		if(opt->url1) {
			wtk_debug("url1 = %.*s\n",opt->url1->len,opt->url1->data);
		}
		if(opt->url2) {
			wtk_debug("url2 = %.*s\n",opt->url2->len,opt->url2->data);
		}
		wtk_debug("timeout = %d\n",opt->timeout);
		wtk_debug("use_srvsel = %d\n",opt->use_srvsel);
		wtk_debug("use_auth_cache = %d\n",opt->use_auth_cache);
		wtk_debug("use_log = %d\n",opt->use_log);
		wtk_debug("use_cldhub = %d\n",opt->use_cldhub);
		wtk_debug("log_wav = %d\n",opt->log_wav);
		wtk_debug("use_timer = %d\n",opt->use_timer);
		wtk_debug("=====================================================\n");
	}
}
