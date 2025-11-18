#ifndef QTK_MISC_HTTPC_REQUEST_QTK_HTTPC_REQUEST
#define QTK_MISC_HTTPC_REQUEST_QTK_HTTPC_REQUEST

#include "wtk/os/wtk_pipequeue.h"
#include "wtk/os/wtk_lockhoard.h"
#include "wtk/core/wtk_stack.h"

#include "sdk/nk/qtk_nk.h"
#include "sdk/nk/qtk_event.h"
#include "qtk/dns/qtk_dns.h"
#include "sdk/httpc/qtk_httpc_cfg.h"
#include "sdk/util/qtk_http_response.h"

#include "sdk/qtk_err.h"
#include "sdk/session/qtk_session.h"

#define QTK_HTTPC_REQUEST_DNS_TRYCNT 1
#define QTK_HTTPC_REQUEST_CONNECT_TRYCNT 1
#define QTK_HTTPC_REQUEST_WAIT_TIME 500
#define QTK_HTTPC_REQUEST_RCV_BUFSIZE 4096

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_httpc_request qtk_httpc_request_t;

typedef void(*qtk_httpc_request_handler_f)(void *ths,qtk_http_response_t *rep);
typedef int(*qtk_httpc_request_auth_handler_f)(void *ths);
typedef void(*qtk_httpc_request_err_notify_f)(void *ths);

typedef struct{
	wtk_queue_node_t hoard_n;
	wtk_queue_node_t q_n;
	wtk_strbuf_t *buf;
}qtk_httpc_request_msg_t;

struct qtk_httpc_request
{
	qtk_httpc_cfg_t *cfg;
	qtk_session_t *session;
	qtk_dns_t *dns;
	qtk_http_response_t *response;
	qtk_nk_t *nk;
	wtk_stack_t *wstack;
	qtk_httpc_request_handler_f handler;
	void* handler_ths;
	qtk_httpc_request_auth_handler_f auth_handler;
	void* auth_handler_ths;
	qtk_httpc_request_err_notify_f err_notify;
	void *err_ths;
	wtk_pipequeue_t input_q;
	wtk_lockhoard_t msg_hoard;
	qtk_event_t in_event;
	qtk_event_t http_event;
	double last_recved;
 	int fd;
 	unsigned auth_req:1;
};

qtk_httpc_request_t* qtk_httpc_request_new(qtk_httpc_cfg_t *cfg,qtk_nk_t *nk,qtk_session_t *session);
void qtk_httpc_request_delete(qtk_httpc_request_t *req);
void qtk_httpc_request_set_handler(qtk_httpc_request_t *req,void *handler_ths,qtk_httpc_request_handler_f handler);
void qtk_httpc_request_set_auth_handler(qtk_httpc_request_t *req,
		void *auth_handler_ths,
		qtk_httpc_request_auth_handler_f auth_handler);
void qtk_httpc_request_connect_reset(qtk_httpc_request_t *req);

int qtk_httpc_request_feed(qtk_httpc_request_t *req,char *data,int bytes);
void qtk_httpc_request_update_hostport(qtk_httpc_request_t *req,wtk_string_t *host,wtk_string_t *port);
void qtk_httpc_request_set_err_notify(qtk_httpc_request_t *req,void *ths,qtk_httpc_request_err_notify_f notify);
#ifdef __cplusplus
};
#endif
#endif
