#ifndef QTK_MISC_HTTPC_QTK_HTTPC
#define QTK_MISC_HTTPC_QTK_HTTPC

#include "sdk/nk/qtk_nk.h"
#include "qtk_httpc_cfg.h"
#include "request/qtk_httpc_request.h"

#include "sdk/util/qtk_http_response.h"
#include "sdk/util/qtk_http_url.h"
#include "sdk/session/qtk_session.h"


#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_httpc qtk_httpc_t;

typedef int(*qtk_httpc_add_hdr_f)(void *ths,wtk_strbuf_t *buf);

struct qtk_httpc
{
	qtk_httpc_cfg_t *cfg;
	qtk_session_t *session;
	qtk_httpc_request_t *req;
	wtk_strbuf_t *buf;
};

qtk_httpc_t *qtk_httpc_new(qtk_httpc_cfg_t *cfg,qtk_nk_t *nk,qtk_session_t *session);
void qtk_httpc_delete(qtk_httpc_t *httpc);

void qtk_httpc_set_handler(qtk_httpc_t *httpc,void *handler_ths,qtk_httpc_request_handler_f handler);
void qtk_httpc_set_auth_handler(qtk_httpc_t *httpc,
		void *auth_handler_ths,
		qtk_httpc_request_auth_handler_f auth_handler);
void qtk_httpc_connect_reset(qtk_httpc_t *httpc);

int qtk_httpc_get(qtk_httpc_t *httpc,char *data,int bytes,void *add_hdr_ths,qtk_httpc_add_hdr_f add_hdr);
int qtk_httpc_post(qtk_httpc_t *httpc,char *data,int bytes,void *add_hdr_ths,qtk_httpc_add_hdr_f add_hdr);
int qtk_httpc_head(qtk_httpc_t *httpc,char *data,int bytes,void *add_hdr_ths,qtk_httpc_add_hdr_f add_hdr);
int qtk_httpc_put(qtk_httpc_t *httpc,char *data,int bytes,void *add_hdr_ths,qtk_httpc_add_hdr_f add_hdr);

int qtk_httpc_wget(char *host,int hostlen,char *port,int portlen,char *uri,int urilen,int timeout,
		void *ths,qtk_httpc_request_handler_f handler,
		qtk_session_t *session);
int qtk_httpc_wget2(char *url,int len,int timeout,
		void *ths,qtk_httpc_request_handler_f handler,
		qtk_session_t *session);
void qtk_httpc_update_hostport(qtk_httpc_t *httpc,wtk_string_t *host,wtk_string_t *port);
void qtk_httpc_set_err_notify(qtk_httpc_t *httpc,void *ths,qtk_httpc_request_err_notify_f notify);
#ifdef __cplusplus
};
#endif
#endif
