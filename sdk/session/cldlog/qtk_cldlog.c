#include "qtk_cldlog.h"
#include "../qtk_session.h"
#ifdef _WIN32
#include <objbase.h>
#else
#include "uuid.h"
#endif
#include <stdarg.h>

static qtk_cldlog_msg_t* qtk_cldlog_msg_new(qtk_cldlog_t *clog);
static int qtk_cldlog_msg_delete(qtk_cldlog_msg_t *msg);
static qtk_cldlog_msg_t* qtk_cldlog_pop_msg(qtk_cldlog_t *clog);
static void qtk_cldlog_push_msg(qtk_cldlog_t *clog,qtk_cldlog_msg_t *msg);

static int qtk_cldlog_run(qtk_cldlog_t *clog,wtk_thread_t *t);
static void qtk_cldlog_on_http(qtk_cldlog_t *clog,qtk_http_response_t *rep);

qtk_cldlog_t* qtk_cldlog_new(void *father)
{
	qtk_cldlog_t *clog;
	qtk_session_t *session;
	int ret;

	session = (qtk_session_t*)father;
	if(!session->opt.cldlog_host || !session->opt.cldlog_port || !session->opt.cldlog_url) {
		return NULL;
	}

	clog = (qtk_cldlog_t*)wtk_malloc(sizeof(*clog));
	if(!clog) {
		return NULL;
	}
	memset(clog,0,sizeof(*clog));

	clog->opt = &session->opt;
	qtk_httpc_cfg_init(&clog->http.cfg);
	wtk_string_set(&clog->http.cfg.host,clog->opt->cldlog_host->data,clog->opt->cldlog_host->len);
	wtk_string_set(&clog->http.cfg.port,clog->opt->cldlog_port->data,clog->opt->cldlog_port->len);
	wtk_string_set(&clog->http.cfg.url,clog->opt->cldlog_url->data,clog->opt->cldlog_url->len);
	clog->http.cfg.timeout = clog->opt->cldlog_timeout;
	qtk_httpc_cfg_update(&clog->http.cfg);

	clog->http.httpc = qtk_httpc_new(&clog->http.cfg,NULL,session);
	if(!clog->http.httpc) {
		ret = -1;
		goto end;
	}
	qtk_httpc_set_handler(clog->http.httpc,clog,(qtk_httpc_request_handler_f)qtk_cldlog_on_http);

	clog->buf = wtk_strbuf_new(512,1);

	wtk_blockqueue_init(&clog->msg_q);
	wtk_lockhoard_init(&clog->msg_hoard,offsetof(qtk_cldlog_msg_t,hoard_n),10,
			(wtk_new_handler_t)qtk_cldlog_msg_new,
			(wtk_delete_handler_t)qtk_cldlog_msg_delete,
			clog
			);
	wtk_thread_init(&clog->thread,(thread_route_handler)qtk_cldlog_run,clog);
	wtk_thread_set_name(&clog->thread,"cldlog");

	ret = 0;
end:
	if(ret != 0) {
		qtk_cldlog_delete(clog);
		clog = NULL;
	}
	return clog;
}

void qtk_cldlog_delete(qtk_cldlog_t *clog)
{
	if(clog->run) {
		qtk_cldlog_stop(clog);
	}
	wtk_thread_clean(&clog->thread);
	wtk_blockqueue_clean(&clog->msg_q);
	wtk_lockhoard_clean(&clog->msg_hoard);

	if(clog->buf) {
		wtk_strbuf_delete(clog->buf);
	}

	if(clog->http.httpc) {
		qtk_httpc_delete(clog->http.httpc);
		qtk_httpc_cfg_clean(&clog->http.cfg);
	}

	wtk_free(clog);
}

int qtk_cldlog_start(qtk_cldlog_t *clog)
{
#ifdef _WIN32
    char buf[128] = {0};
    GUID guid;
    CoCreateGuid(&guid);
    snprintf(
        buf, sizeof(buf),
        "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
        guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1],
        guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5],
        guid.Data4[6], guid.Data4[7]
    );
#else
	uuid_t uuid;
	char buf[128];

	uuid_generate(uuid);
	uuid_unparse(uuid,buf);
#endif
	clog->id = wtk_string_dup_data(buf,strlen(buf));
	clog->run = 1;

	return wtk_thread_start(&clog->thread);
}

int qtk_cldlog_stop(qtk_cldlog_t *clog)
{
	clog->run = 0;
	wtk_blockqueue_wake(&clog->msg_q);
	wtk_thread_join(&clog->thread);
	wtk_string_delete(clog->id);
	clog->id = NULL;
	return 0;
}

int qtk_cldlog_feed(qtk_cldlog_t *clog,const char *func,int line,int level,char *fmt,...)
{
	qtk_cldlog_msg_t *msg;
	va_list ap;
	char buffer[512];
	int len;

	msg = qtk_cldlog_pop_msg(clog);
	if(!msg) {
		return -1;
	}
	va_start(ap,fmt);

	len = snprintf(buffer,512,"%s %d %d ",func,line,level);
	wtk_strbuf_push(msg->buf,buffer,len);
	len = vsnprintf(buffer,512,fmt,ap);
	wtk_strbuf_push(msg->buf,buffer,len);
	wtk_strbuf_push_c(msg->buf,0);
	--msg->buf->pos;
	va_end(ap);

	wtk_blockqueue_push(&clog->msg_q,&msg->q_n);
	return 0;
}

static qtk_cldlog_msg_t* qtk_cldlog_msg_new(qtk_cldlog_t *clog)
{
	qtk_cldlog_msg_t *msg;

	msg = (qtk_cldlog_msg_t*)wtk_malloc(sizeof(*msg));
	if(!msg) {
		return NULL;
	}

	msg->buf = wtk_strbuf_new(256,1);
	if(!msg->buf) {
		wtk_free(msg);
		return NULL;
	}

	return msg;
}

static int qtk_cldlog_msg_delete(qtk_cldlog_msg_t *msg)
{
	wtk_strbuf_delete(msg->buf);
	wtk_free(msg);
	return 0;
}

static qtk_cldlog_msg_t* qtk_cldlog_pop_msg(qtk_cldlog_t *clog)
{
	qtk_cldlog_msg_t *msg;

	msg = wtk_lockhoard_pop(&clog->msg_hoard);
	if(!msg) {
		return NULL;
	}
	wtk_strbuf_reset(msg->buf);
	return msg;
}

static void qtk_cldlog_push_msg(qtk_cldlog_t *clog,qtk_cldlog_msg_t *msg)
{
	wtk_lockhoard_push(&clog->msg_hoard,msg);
}

#define QTK_CLDLOG_BOUNDARY "cldxbxlog"
static void qtk_clog_add_http_hdr(qtk_cldlog_t *clog,wtk_strbuf_t *buf)
{
	wtk_strbuf_push_f(buf,"Content-Type: multipart/form-data; boundary=%s\r\n",QTK_CLDLOG_BOUNDARY);
	wtk_strbuf_push_f(buf,"Host: %s:%s\r\n",clog->http.cfg.host.data,clog->http.cfg.port.data);
}

static void qtk_cldlog_on_http(qtk_cldlog_t *clog,qtk_http_response_t *rep)
{
	wtk_debug("req = [%.*s]\n",rep->body->pos,rep->body->data);
}

static int qtk_cldlog_run_http(qtk_cldlog_t *clog,char *data,int bytes)
{
	wtk_strbuf_t *buf = clog->buf;
	int ret;

	wtk_strbuf_reset(buf);
	//appid
	wtk_strbuf_push_f(buf,"--%s\r\n",QTK_CLDLOG_BOUNDARY);
	wtk_strbuf_push_f(buf,"content-disposition: form-data;name=\"appid\"\r\n\r\n");
	wtk_strbuf_push_f(buf,"%.*s\r\n",clog->opt->appid->len,clog->opt->appid->data);
	//usrid
	wtk_strbuf_push_f(buf,"--%s\r\n",QTK_CLDLOG_BOUNDARY);
	wtk_strbuf_push_f(buf,"content-disposition: form-data;name=\"deviceid\"\r\n\r\n");
	wtk_strbuf_push_f(buf,"%.*s\r\n",clog->opt->usrid->len,clog->opt->usrid->data);
	//seqno
	wtk_strbuf_push_f(buf,"--%s\r\n",QTK_CLDLOG_BOUNDARY);
	wtk_strbuf_push_f(buf,"content-disposition: form-data;name=\"startid\"\r\n\r\n");
	wtk_strbuf_push_f(buf,"%.*s\r\n",clog->id->len,clog->id->data);
	//content
	wtk_strbuf_push_f(buf,"--%s\r\n",QTK_CLDLOG_BOUNDARY);
	wtk_strbuf_push_f(buf,"content-disposition: form-data;name=\"content\"\r\n\r\n");
	wtk_strbuf_push_f(buf,"%ld %.*s\r\n",clog->seNo++,bytes,data);

//	ret = 0;
//	wtk_debug("[%.*s]\n\n",buf->pos,buf->data);
	ret = qtk_httpc_post(clog->http.httpc,buf->data,buf->pos,clog,(qtk_httpc_add_hdr_f)qtk_clog_add_http_hdr);
	if(ret != 0) {
		qtk_httpc_connect_reset(clog->http.httpc);
		ret = qtk_httpc_post(clog->http.httpc,buf->data,buf->pos,clog,(qtk_httpc_add_hdr_f)qtk_clog_add_http_hdr);
	}
	qtk_httpc_connect_reset(clog->http.httpc);
	return ret;
}

static int qtk_cldlog_run(qtk_cldlog_t *clog,wtk_thread_t *t)
{
	qtk_cldlog_msg_t *msg;
	wtk_queue_node_t *qn;

	while(clog->run) {
		qn = wtk_blockqueue_pop(&clog->msg_q,-1,NULL);
		if(!qn) {
			continue;
		}
		msg = data_offset2(qn,qtk_cldlog_msg_t,q_n);
		qtk_cldlog_run_http(clog,msg->buf->data,msg->buf->pos);
		//wtk_debug("=====> [%.*s] \n",msg->buf->pos,msg->buf->data);
		qtk_cldlog_push_msg(clog,msg);
	}

	return 0;
}
