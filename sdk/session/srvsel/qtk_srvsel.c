#include "qtk_srvsel.h"

void qtk_srvsel_on_http(qtk_srvsel_t *s,qtk_http_response_t *rep)
{
	wtk_strbuf_reset(s->buf);
	wtk_strbuf_push(s->buf,rep->body->data,rep->body->pos);
}

void qtk_srvsel_add_hdr(qtk_httpc_t *httpc,wtk_strbuf_t *buf)
{
	wtk_strbuf_push_f(buf,"Host:%.*s:%.*s\r\n",
			httpc->cfg->host.len,
			httpc->cfg->host.data,
			httpc->cfg->port.len,
			httpc->cfg->port.data
			);
}

int qtk_srvsel_get_list(qtk_srvsel_t *s)
{
	qtk_option_t *opt = &s->session->opt;
	qtk_httpc_cfg_t cfg;
	qtk_httpc_t *httpc;
	int ret;

	qtk_httpc_cfg_init(&cfg);
	wtk_string_set_s(&cfg.host,QTK_SRVSEL_REQ_HOST);
	wtk_string_set_s(&cfg.port,QTK_SRVSEL_REQ_POST);
	wtk_string_set_s(&cfg.url,QTK_SRVSEL_REQ_URI);
	qtk_httpc_cfg_update_opt(&cfg,NULL,NULL,NULL,opt->dns_fn,QTK_SRVSEL_REQ_TIMEOUT);
	qtk_httpc_cfg_update(&cfg);

	httpc = qtk_httpc_new(&cfg,NULL,s->session);

	wtk_strbuf_reset(s->buf);
	qtk_http_url_encode_kv(s->buf,"appid",sizeof("appid")-1,opt->appid->data,opt->appid->len);

	qtk_httpc_set_handler(httpc,s,(qtk_httpc_request_handler_f)qtk_srvsel_on_http);
	ret = qtk_httpc_get(httpc,s->buf->data,s->buf->pos,httpc,(qtk_httpc_add_hdr_f)qtk_srvsel_add_hdr);
	if(s->buf->pos<=0){
		ret = -1;
	}
	wtk_log_log(s->session->log,"srvsel list = %.*s",s->buf->pos,s->buf->data);
	//wtk_debug("srvsel list = %.*s\n",s->buf->pos,s->buf->data);
	qtk_httpc_delete(httpc);
	qtk_httpc_cfg_clean(&cfg);
	return ret;
}

void qtk_srvsel_notify_statics(qtk_srvsel_task_t *t,qtk_http_response_t *rep)
{
	wtk_strbuf_reset(t->buf);
	wtk_strbuf_push(t->buf,rep->body->data,rep->body->pos);
}
#if 0
void qtk_srvsel_task_update_rate(qtk_srvsel_task_t *t,double time)
{
	wtk_json_parser_t *p;
	wtk_json_item_t *ncpu,*nstream;
	int ret;

//	wtk_debug("static=[%.*s]\n",t->buf->pos,t->buf->data);
	p=wtk_json_parser_new();
	ret=wtk_json_parser_parse(p,t->buf->data,t->buf->pos);
	if(ret!=0){goto end;}
	ncpu=wtk_json_obj_get_s(p->json->main,"ncpu");
	nstream=wtk_json_obj_get_s(p->json->main,"nstream");
	if(!ncpu || !nstream){goto end;}
	t->rate=nstream->v.number*1.0/ncpu->v.number;
	t->delay=time;
	t->rate2=t->rate*QTK_SRVSEL_NET_WEIGHT+t->delay*QTK_SRVSEL_CPU_WEIGHT/100;
end:
	wtk_json_parser_delete(p);
	return;
}

int qtk_srvsel_run_route(qtk_srvsel_task_t *t,wtk_thread_t *thread)
{
	qtk_httpc_cfg_t cfg;
	qtk_httpc_t *httpc;
	wtk_json_item_t *host;
	wtk_json_item_t *port;
	wtk_strbuf_t *buf = t->buf;
	int ret;
	double xt;

	host = wtk_json_obj_get_s(t->item,"host");
	port = wtk_json_obj_get_s(t->item,"port");
	t->host = host;
	t->port = port;

	qtk_httpc_cfg_init(&cfg);
	wtk_string_set(&cfg.host,host->v.str->data,host->v.str->len);
	if(port) {
		wtk_string_set(&cfg.port,port->v.str->data,port->v.str->len);
	} else {
		wtk_string_set_s(&cfg.port,"80");
	}
	wtk_string_set_s(&cfg.url,"/statics");
	qtk_httpc_cfg_update_opt(&cfg,NULL,NULL,NULL,t->s->session->opt.dns_fn,QTK_SRVSEL_ROUTE_TIMEOUT);
	qtk_httpc_cfg_update(&cfg);

	httpc = qtk_httpc_new(&cfg,NULL,t->s->session);
	qtk_httpc_set_handler(httpc,t,(qtk_httpc_request_handler_f)qtk_srvsel_notify_statics);


	xt=time_get_ms();
	ret = qtk_httpc_get(httpc,NULL,0,NULL,NULL);
	if(ret!=0 || buf->pos==0)
	{
		t->err = 1;
	} else {
		xt = time_get_ms()-xt;
		qtk_srvsel_task_update_rate(t,xt);
	}

	qtk_httpc_delete(httpc);
	qtk_httpc_cfg_clean(&cfg);
	return 0;
}

qtk_srvsel_task_t* qtk_srvsel_task_new(qtk_srvsel_t *s,wtk_json_item_t *item)
{
	qtk_srvsel_task_t *task;

	task=(qtk_srvsel_task_t*)wtk_malloc(sizeof(qtk_srvsel_task_t));
	task->item=item;
	task->s=s;
	task->buf=wtk_strbuf_new(256,1);
	task->err=0;
	task->rate=1;
	task->delay=100000;
	task->rate2=1;
	task->host=NULL;
	task->port=NULL;
	return task;
}

void qtk_srvsel_task_delete(qtk_srvsel_task_t *t)
{
	wtk_strbuf_delete(t->buf);
	wtk_free(t);
}
#endif
int qtk_srvsel_update_list(qtk_srvsel_t *s)
{
	wtk_strbuf_t *buf=s->buf;
	wtk_json_parser_t *p;
	wtk_json_item_t *item,*vi,*host,*port;
	int ret;
//	static int c;
//	char *tmp = "{\"servers\": [{\"host\": \"192.168.0.98\", \"port\": \"80\"}]}";
//
//	c++;
//	if(c>1){
//		wtk_strbuf_reset(buf);
//		wtk_strbuf_push(buf,tmp,strlen(tmp));
//	}

	p=wtk_json_parser_new();
	ret=wtk_json_parser_parse(p,buf->data,buf->pos);
	if(ret!=0)
	{
		wtk_log_warn0(s->session->log,"list format illegal[json].");
		ret=-1;
		goto end;
	}
	item=wtk_json_obj_get_s(p->json->main,"servers");
	if(!item)
	{
		wtk_log_warn0(s->session->log,"list don't have valid servers");
		ret=-1;
		goto end;
	}
	if(item->type!=WTK_JSON_ARRAY)
	{
		wtk_log_warn0(s->session->log,"list servers format illegal[json array]");
		ret=-1;
		goto end;
	}
	vi=wtk_json_array_get(item,0);
	if(!vi || vi->type!=WTK_JSON_OBJECT)
	{
		wtk_log_warn0(s->session->log,"json array has no member or the member format illegal [json object]");
		ret = -1;
		goto end;
	}
	host=wtk_json_obj_get_s(vi,"host");
	if(!host || host->type != WTK_JSON_STRING){
		wtk_log_warn0(s->session->log,"host not found");
		ret  = -1;
		goto end;
	}
	s->host = wtk_string_dup_data(host->v.str->data,host->v.str->len);
	port=wtk_json_obj_get_s(vi,"port");
	if(!port || port->type != WTK_JSON_STRING){
		s->port = wtk_string_dup_data("80",sizeof("80"));
	}else{
		s->port = wtk_string_dup_data(port->v.str->data,port->v.str->len);
	}
	ret=0;
end:
	wtk_json_parser_delete(p);
	return ret;
}

int qtk_srvsel_process(qtk_srvsel_t *s)
{
	int ret;
	int i;

	i = 0;
	do{
		++i;
		ret = qtk_srvsel_get_list(s);
		if(ret == 0) {
			ret = qtk_srvsel_update_list(s);
			if(ret == 0) {
				break;
			} else {
				wtk_log_warn(s->session->log,"update list failed,trys %d",i);
			}
		} else {
			wtk_log_warn(s->session->log,"get list failed,trys %d",i);
		}
		if(i >= QTK_SRVSEL_FAIL_TRYS) {
			break;
		}
		wtk_msleep(QTK_SRVSEL_FAIL_WAIT);
	}while(1);

	return ret;
}

void qtk_srvsel_init(qtk_srvsel_t *s)
{
	s->session = NULL;

	s->host = NULL;
	s->port = NULL;

	s->buf = NULL;
}

qtk_srvsel_t* qtk_srvsel_new(qtk_session_t *session)
{
	qtk_srvsel_t *s;

	s=(qtk_srvsel_t*)wtk_malloc(sizeof(qtk_srvsel_t));
	qtk_srvsel_init(s);
	s->session = session;
	s->buf=wtk_strbuf_new(256,1);
	return s;
}

void qtk_srvsel_clean(qtk_srvsel_t *s)
{
	if(s->host) {
		wtk_string_delete(s->host);
		s->host  =NULL;
	}
	if(s->port) {
		wtk_string_delete(s->port);
		s->port = NULL;
	}
}
void qtk_srvsel_delete(qtk_srvsel_t *s)
{
	wtk_strbuf_delete(s->buf);
	if(s->host) {
		wtk_string_delete(s->host);
	}
	if(s->port) {
		wtk_string_delete(s->port);
	}
	wtk_free(s);
}
