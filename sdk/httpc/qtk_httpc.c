#include "qtk_httpc.h" 

static void qtk_httpc_init(qtk_httpc_t *httpc)
{
	httpc->cfg = NULL;
	httpc->session = NULL;
	httpc->req = NULL;
	httpc->buf = NULL;
}

qtk_httpc_t *qtk_httpc_new(qtk_httpc_cfg_t *cfg,qtk_nk_t *nk,qtk_session_t *session)
{
	qtk_httpc_t *httpc;
	int ret;

	httpc = (qtk_httpc_t *)wtk_malloc(sizeof(qtk_httpc_t));
	qtk_httpc_init(httpc);

	httpc->cfg = cfg;
	httpc->session = session;

	httpc->req = qtk_httpc_request_new(cfg,nk,session);
	if(!httpc->req) {
		ret = -1;
		goto end;
	}

	httpc->buf = wtk_strbuf_new(8192,1);

	ret = 0;
end:
	if(ret != 0) {
		wtk_free(httpc);
		httpc = NULL;
	}
	return httpc;
}

void qtk_httpc_delete(qtk_httpc_t *httpc)
{
	if(httpc->req) {
		qtk_httpc_request_delete(httpc->req);
	}
	if(httpc->buf) {
		wtk_strbuf_delete(httpc->buf);
	}
	wtk_free(httpc);
}

void qtk_httpc_set_handler(qtk_httpc_t *httpc,void *ths,qtk_httpc_request_handler_f handler)
{
	qtk_httpc_request_set_handler(httpc->req,ths,handler);
}

void qtk_httpc_set_auth_handler(qtk_httpc_t *httpc,
		void *auth_handler_ths,
		qtk_httpc_request_auth_handler_f auth_handler)
{
	qtk_httpc_request_set_auth_handler(httpc->req,auth_handler_ths,auth_handler);
}

void qtk_httpc_connect_reset(qtk_httpc_t *httpc)
{
	qtk_httpc_request_connect_reset(httpc->req);
}

int qtk_httpc_get(qtk_httpc_t *httpc,char *data,int bytes,void *add_hdr_ths,qtk_httpc_add_hdr_f add_hdr)
{
	wtk_strbuf_t *buf = httpc->buf;
	int ret;

	wtk_strbuf_reset(buf);

	wtk_strbuf_push_s(buf,"GET ");
	wtk_strbuf_push(buf,httpc->cfg->url.data,httpc->cfg->url.len);
	if(bytes > 0) {
		wtk_strbuf_push_s(buf,"?");
		wtk_strbuf_push(buf,data,bytes);
	}
	if(httpc->cfg->http_1_1) {
		wtk_strbuf_push_s(buf," HTTP/1.1\r\n");
	} else {
		wtk_strbuf_push_s(buf," HTTP/1.0\r\n");
	}
	wtk_strbuf_push_s(buf,"Content-Length: 0\r\n");
	if(add_hdr) {
		add_hdr(add_hdr_ths,buf);
	}
	wtk_strbuf_push_s(buf,"\r\n");

	if(httpc->cfg->log_http) {
		wtk_log_log(httpc->session->log,"httpc [%.*s].",buf->pos,buf->data);
	}

	ret = qtk_httpc_request_feed(httpc->req,buf->data,buf->pos);
	return ret;
}


int qtk_httpc_head(qtk_httpc_t *httpc,char *data,int bytes,void *add_hdr_ths,qtk_httpc_add_hdr_f add_hdr)
{
	wtk_strbuf_t *buf = httpc->buf;
	int ret;

	wtk_strbuf_reset(buf);

	wtk_strbuf_push_s(buf,"HEAD ");
	wtk_strbuf_push(buf,httpc->cfg->url.data,httpc->cfg->url.len);
	if(bytes > 0) {
		wtk_strbuf_push_s(buf,"?");
		wtk_strbuf_push(buf,data,bytes);
	}
	if(httpc->cfg->http_1_1) {
		wtk_strbuf_push_s(buf," HTTP/1.1\r\n");
	} else {
		wtk_strbuf_push_s(buf," HTTP/1.0\r\n");
	}
	wtk_strbuf_push_s(buf,"Content-Length: 0\r\n");
	if(add_hdr) {
		add_hdr(add_hdr_ths,buf);
	}
	wtk_strbuf_push_s(buf,"\r\n");

	if(httpc->cfg->log_http) {
		wtk_log_log(httpc->session->log,"httpc [%.*s].",buf->pos,buf->data);
	}

	ret = qtk_httpc_request_feed(httpc->req,buf->data,buf->pos);
	return ret;
}


int qtk_httpc_post(qtk_httpc_t *httpc,char *data,int bytes,void *add_hdr_ths,qtk_httpc_add_hdr_f add_hdr)
{
	wtk_strbuf_t *buf = httpc->buf;
	char tmp[32];
	int n;
	int ret;

	wtk_strbuf_reset(buf);
	wtk_strbuf_push_s(buf,"POST ");
	wtk_strbuf_push(buf,httpc->cfg->url.data,httpc->cfg->url.len);
	if(httpc->cfg->http_1_1) {
		wtk_strbuf_push_s(buf," HTTP/1.1\r\n");
	} else {
		wtk_strbuf_push_s(buf," HTTP/1.0\r\n");
	}

	wtk_strbuf_push_s(buf,"Content-Length: ");
	n = sprintf(tmp,"%d",bytes);

	wtk_strbuf_push(buf,tmp,n);
	wtk_strbuf_push_s(buf,"\r\n");

	if(add_hdr) {
		add_hdr(add_hdr_ths,buf);
	} else {
		wtk_strbuf_push_s(buf,"Content-Type: application/x-www-form-urlencoded\r\n");
	}
	wtk_strbuf_push_s(buf,"\r\n");

	if(bytes > 0) {
		wtk_strbuf_push(buf,data,bytes);
	}

	if(httpc->cfg->log_http) {
		wtk_log_log(httpc->session->log,"httpc [%.*s].",buf->pos,buf->data);
	}
	ret = qtk_httpc_request_feed(httpc->req,buf->data,buf->pos);
	return ret;
}

int qtk_httpc_put(qtk_httpc_t *httpc,char *data,int bytes,void *add_hdr_ths,qtk_httpc_add_hdr_f add_hdr)
{
	wtk_strbuf_t *buf = httpc->buf;
	char tmp[32];
	int n;
	int ret;

	wtk_strbuf_reset(buf);
	wtk_strbuf_push_s(buf,"PUT ");
	wtk_strbuf_push(buf,httpc->cfg->url.data,httpc->cfg->url.len);
	if(httpc->cfg->http_1_1) {
		wtk_strbuf_push_s(buf," HTTP/1.1\r\n");
	} else {
		wtk_strbuf_push_s(buf," HTTP/1.0\r\n");
	}

	wtk_strbuf_push_s(buf,"Content-Length: ");
	n = sprintf(tmp,"%d",bytes);

	wtk_strbuf_push(buf,tmp,n);
	wtk_strbuf_push_s(buf,"\r\n");

	if(add_hdr) {
		add_hdr(add_hdr_ths,buf);
	} else {
		wtk_strbuf_push_s(buf,"Content-Type: application/x-www-form-urlencoded\r\n");
	}
	wtk_strbuf_push_s(buf,"\r\n");

	if(bytes > 0) {
		wtk_strbuf_push(buf,data,bytes);
	}

	if(httpc->cfg->log_http) {
		wtk_log_log(httpc->session->log,"httpc [%.*s].",buf->pos,buf->data);
	}
	ret = qtk_httpc_request_feed(httpc->req,buf->data,buf->pos);
	return ret;
}

int qtk_httpc_wget(char *host,int hostlen,char *port,int portlen,char *uri,int urilen,int timeout,
		void *ths,qtk_httpc_request_handler_f handler,
		qtk_session_t *session)
{
	qtk_httpc_cfg_t cfg;
	qtk_httpc_t *httpc;
	int ret;

	qtk_httpc_cfg_init(&(cfg));
	wtk_string_set(&(cfg.host),host,hostlen);
	wtk_string_set(&(cfg.port),port,portlen);
	wtk_string_set(&(cfg.url),uri,urilen);
	cfg.timeout = timeout;
	qtk_httpc_cfg_update(&cfg);

	httpc = qtk_httpc_new(&(cfg),NULL,session);
	qtk_httpc_set_handler(httpc,ths,handler);
	ret = qtk_httpc_get(httpc,NULL,0,NULL,NULL);

	qtk_httpc_delete(httpc);
	qtk_httpc_cfg_update(&(cfg));
	return ret;
}

int qtk_httpc_wget2(char *url,int len,int timeout,
		void *ths,qtk_httpc_request_handler_f handler,
		qtk_session_t *session)
{
	qtk_http_url_t *u = NULL;
	wtk_strbuf_t *params = NULL;
	int ret;

	u = qtk_http_url_new();
	ret = qtk_http_url_parse(u,url,len);
	params = wtk_strbuf_new(128,1);

	qtk_http_url_encode(params,u);
	ret = qtk_httpc_wget(u->host.data,u->host.len,u->port.data,u->port.len,params->data,params->pos,timeout,ths,handler,session);

	wtk_strbuf_delete(params);
	qtk_http_url_delete(u);
	return ret;
}

void qtk_httpc_update_hostport(qtk_httpc_t *httpc,wtk_string_t *host,wtk_string_t *port)
{
	qtk_httpc_request_update_hostport(httpc->req,host,port);
}
void qtk_httpc_set_err_notify(qtk_httpc_t *httpc,void *ths,qtk_httpc_request_err_notify_f notify)
{
	qtk_httpc_request_set_err_notify(httpc->req,ths,notify);
}
