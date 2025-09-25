#ifndef QTK_SESSION_CLDHUB_QTK_CLDHUB
#define QTK_SESSION_CLDHUB_QTK_CLDHUB

#include "sdk/httpc/qtk_httpc.h"
#include "sdk/session/qtk_session.h"
#include "sdk/session/auth/qtk_auth.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_cldhub qtk_cldhub_t;

typedef void(*qtk_cldhub_spx_notify_f)(void *spx_ths,qtk_http_response_t *rep);

typedef struct {
	qtk_cldhub_spx_notify_f spx_notify;
	void* spx_ths;
}qtk_cldhub_spx_t;

struct qtk_cldhub
{
	qtk_session_t *session;
	struct{
		qtk_httpc_cfg_t cfg;
		qtk_httpc_t *httpc;
		qtk_nk_t *nk;
	}http;
	qtk_auth_t *auth;
	wtk_strbuf_t *buf;
	wtk_sem_t auth_sem;
	wtk_str_hash_t *spx_hash;
	qtk_auth_result_t rlt;
	wtk_lock_t lock;
};

qtk_cldhub_t* qtk_cldhub_new(qtk_session_t *session);
void qtk_cldhub_delete(qtk_cldhub_t *cldhub);
int qtk_cldhub_set_spx(qtk_cldhub_t *cldhub,char *id,void *spx_ths,qtk_cldhub_spx_notify_f spx_notify);
int qtk_cldhub_del_spx(qtk_cldhub_t *cldhub,char *id);
void qtk_cldhub_connect_reset(qtk_cldhub_t *cldhub);
int qtk_cldhub_feed(qtk_cldhub_t *cldhub,char *data,int bytes,void *add_hdr_ths,qtk_httpc_add_hdr_f add_hdr);
void qtk_cldhub_update_hostport(qtk_cldhub_t *cldhub, wtk_string_t *host,wtk_string_t *port);
void qtk_cldhub_set_err_notify(qtk_cldhub_t *cldhub,void *ths,qtk_httpc_request_err_notify_f notify);
#ifdef __cplusplus
};
#endif
#endif
