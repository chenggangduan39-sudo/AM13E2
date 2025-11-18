#include "qtk_cldhub.h"

static qtk_cldhub_spx_t* qtk_cldhub_spx_new(qtk_cldhub_spx_notify_f spx_notify,void *spx_ths)
{
	qtk_cldhub_spx_t *spx;

	spx = (qtk_cldhub_spx_t*)wtk_malloc(sizeof(qtk_cldhub_spx_t));
	spx->spx_notify = spx_notify;
	spx->spx_ths    = spx_ths;

	return spx;
}

static void qtk_cldhub_spx_delete(qtk_cldhub_spx_t *spx)
{
	wtk_free(spx);
}

static void qtk_cldhub_on_httpc(qtk_cldhub_t *cldhub,qtk_http_response_t *rep)
{
	qtk_cldhub_spx_t *spx;
	wtk_string_t *v;
	int len;

	v = wtk_str_hash_find_s(rep->hdr_hash,"hook");
	if(!v) {
		wtk_log_warn0(cldhub->session->log,"no hook");
		return;
	}

	len = (unsigned char)v->data[0];
	spx = wtk_str_hash_find(cldhub->spx_hash,v->data+1,len);
	if(spx) {
		spx->spx_notify(spx->spx_ths,rep);
	} else {
		wtk_log_warn(cldhub->session->log,"not find spx = [%.*s]",len,v->data+1);
	}
}
#ifdef USE_AUTH
static void qtk_cldhub_on_httpc_auth(qtk_cldhub_t *cldhub,qtk_http_response_t *rep)
{
	wtk_strbuf_reset(cldhub->buf);

	if(rep->status == 200) {
		cldhub->rlt = QTK_AUTH_SUCCESS;
	} else {
		wtk_log_warn(cldhub->session->log,"cldhub auth status = %d and rep=[%.*s].",rep->status,rep->body->pos,rep->body->data);
		wtk_strbuf_push(cldhub->buf,rep->body->data,rep->body->pos);
	}

	wtk_sem_release(&cldhub->auth_sem,1);
}

static int qtk_cldhub_httpc_auth_add_hdr(qtk_cldhub_t *cldhub,wtk_strbuf_t *buf)
{
	wtk_strbuf_push_s(buf,"Content-Type: text/plain\r\n");
	return 0;
}

static int qtk_cldhub_process_auth(qtk_cldhub_t *cldhub)
{
	qtk_option_t *opt = &cldhub->session->opt;
	qtk_auth_t *auth = cldhub->auth;
	qtk_httpc_t *httpc = cldhub->http.httpc;
	wtk_string_t v;
	int i;

	cldhub->rlt = -1;
	qtk_httpc_set_handler(httpc,cldhub,(qtk_httpc_request_handler_f)qtk_cldhub_on_httpc_auth);
	wtk_string_set(&cldhub->http.cfg.url,opt->url_auth->data,opt->url_auth->len);

	i = 0;
	do {
		v = qtk_auth_mk2(auth);
		if(v.len <= 0) {
			break;
		}

		wtk_strbuf_reset(cldhub->buf);
		qtk_httpc_post(httpc,v.data,v.len,cldhub,(qtk_httpc_add_hdr_f)qtk_cldhub_httpc_auth_add_hdr);
		wtk_sem_acquire(&cldhub->auth_sem,QTK_AUTH_REQ_TIMEOUT);

		if(cldhub->rlt == QTK_AUTH_SUCCESS) {
			break;
		}

		if(cldhub->buf->pos <= 0) {
			goto middle;
		}
		qtk_auth_check(auth,cldhub->buf->data,cldhub->buf->pos,0);
		cldhub->rlt = qtk_auth_get_result(auth);

		switch(cldhub->rlt) {
		case QTK_AUTH_NETERR:
		case QTK_AUTH_ERRRET:
		case QTK_AUTH_TIMEOUT:
		case QTK_AUTH_ERROR:
		case QTK_AUTH_FAILED:
			break;
		default:
			goto end;
			break;
		}
middle:
		++i;
		if(i > QTK_AUTH_FAIL_TRYS) {
			break;
		}
		wtk_msleep(QTK_AUTH_FAIL_WAIT);
	}while(1);

end:
	qtk_httpc_set_handler(httpc,cldhub,(qtk_httpc_request_handler_f)qtk_cldhub_on_httpc);
	wtk_string_set(&cldhub->http.cfg.url,opt->url2->data,opt->url2->len);
	wtk_log_log(cldhub->session->log,"auth ret = %d",cldhub->rlt);
	return cldhub->rlt;
}

static int qtk_cldhub_httpc_auth_handler(qtk_cldhub_t *cldhub)
{
	int ret;
	ret = qtk_cldhub_process_auth(cldhub);
	if(ret != 0) {
		wtk_log_warn(cldhub->session->log,"auth failed and rlt = %d",ret);
	}
	return ret;
}
#endif
static void qtk_cldhub_init(qtk_cldhub_t *cldhub)
{
	cldhub->session = NULL;

	cldhub->http.httpc = NULL;
	cldhub->http.nk    = NULL;
	cldhub->auth       = NULL;
	cldhub->buf        = NULL;

	cldhub->spx_hash   = NULL;

	cldhub->rlt = -1;
}

qtk_cldhub_t* qtk_cldhub_new(qtk_session_t *session)
{
	qtk_cldhub_t *cldhub;

	cldhub = (qtk_cldhub_t*)wtk_malloc(sizeof(qtk_cldhub_t));
	qtk_cldhub_init(cldhub);

	cldhub->session = session;

	qtk_httpc_cfg_init(&cldhub->http.cfg);
	qtk_httpc_cfg_update_opt(&cldhub->http.cfg,
			session->opt.host,
			session->opt.port,
			session->opt.url2,
			session->opt.dns_fn,
			session->opt.timeout
			);
	qtk_httpc_cfg_update(&cldhub->http.cfg);

	cldhub->http.nk = qtk_nk_new(session->log,1000,20);
	if(!cldhub->http.nk) {
		wtk_log_warn(session->log,"cldhub nk = %p\n",cldhub->http.nk);
	}

	cldhub->buf = wtk_strbuf_new(256,1);
#ifdef USE_AUTH
	cldhub->auth = qtk_auth_new2(session,0);
	wtk_sem_init(&cldhub->auth_sem,0);
#endif
	cldhub->http.httpc = qtk_httpc_new(&cldhub->http.cfg,cldhub->http.nk,session);
	qtk_httpc_set_handler(cldhub->http.httpc,cldhub,(qtk_httpc_request_handler_f)qtk_cldhub_on_httpc);
#ifdef USE_AUTH
	qtk_httpc_set_auth_handler(cldhub->http.httpc,cldhub,(qtk_httpc_request_auth_handler_f)qtk_cldhub_httpc_auth_handler);
#endif
	cldhub->spx_hash = wtk_str_hash_new(7);
	wtk_lock_init(&cldhub->lock);
	return cldhub;
}

static int qtk_cldhub_delete_spxs(qtk_cldhub_t *cldhub,hash_str_node_t *node)
{
	qtk_cldhub_spx_t *spx;

	spx = (qtk_cldhub_spx_t*)node->value;
	qtk_cldhub_spx_delete(spx);
	return 0;
}

void qtk_cldhub_delete(qtk_cldhub_t *cldhub)
{
#ifdef USE_AUTH
	qtk_auth_delete(cldhub->auth);
	wtk_sem_clean(&cldhub->auth_sem);
#endif
	wtk_strbuf_delete(cldhub->buf);

	wtk_str_hash_walk(cldhub->spx_hash,(wtk_walk_handler_t)qtk_cldhub_delete_spxs,cldhub);
	wtk_str_hash_delete(cldhub->spx_hash);

	qtk_httpc_delete(cldhub->http.httpc);
	qtk_nk_delete(cldhub->http.nk);
	qtk_httpc_cfg_clean(&cldhub->http.cfg);

	wtk_lock_clean(&cldhub->lock);

	wtk_free(cldhub);
}

int qtk_cldhub_set_spx(qtk_cldhub_t *cldhub,char *id,void *spx_ths,qtk_cldhub_spx_notify_f spx_notify)
{
	qtk_cldhub_spx_t *spx;
	int len;

	len = (unsigned char)id[0];
	spx = qtk_cldhub_spx_new(spx_notify,spx_ths);
	wtk_str_hash_add(cldhub->spx_hash,id+1,len,spx);
	wtk_log_log(cldhub->session->log,"cldhub set spx=%.*s spx_ths=%p spx_notify=%p",len,id+1,spx_ths,spx_notify);

	return 0;
}

int qtk_cldhub_del_spx(qtk_cldhub_t *cldhub,char *id)
{
	qtk_cldhub_spx_t *spx;
	int len;

	len = (unsigned char)id[0];
	spx = (qtk_cldhub_spx_t*)wtk_str_hash_find(cldhub->spx_hash,id+1,len);
	wtk_str_hash_remove(cldhub->spx_hash,id+1,len);
	qtk_cldhub_spx_delete(spx);

	return 0;
}

int qtk_cldhub_feed(qtk_cldhub_t *cldhub,char *data,int bytes,void *add_hdr_ths,qtk_httpc_add_hdr_f add_hdr)
{
	int ret;
	//wtk_lock_lock(&cldhub->lock);
	ret = qtk_httpc_post(cldhub->http.httpc,data,bytes,add_hdr_ths,add_hdr);
	//wtk_lock_unlock(&cldhub->lock);
	return ret;
}


void qtk_cldhub_connect_reset(qtk_cldhub_t *cldhub)
{
	//wtk_lock_lock(&cldhub->lock);
	qtk_httpc_connect_reset(cldhub->http.httpc);
	//wtk_lock_unlock(&cldhub->lock);
}
void qtk_cldhub_update_hostport(qtk_cldhub_t *cldhub, wtk_string_t *host,wtk_string_t *port)
{
	qtk_httpc_update_hostport(cldhub->http.httpc,host,port);
}
void qtk_cldhub_set_err_notify(qtk_cldhub_t *cldhub,void *ths,qtk_httpc_request_err_notify_f notify)
{
	qtk_httpc_set_err_notify(cldhub->http.httpc,ths,notify);
}
