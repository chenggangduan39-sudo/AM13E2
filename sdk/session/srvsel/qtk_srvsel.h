#ifndef QTK_SESSION_SRVSEL_QTK_SRVSEL
#define QTK_SESSION_SRVSEL_QTK_SRVSEL

#include "wtk/core/wtk_strbuf.h"
#include "wtk/os/wtk_thread.h"
#include "wtk/core/json/wtk_json_parse.h"

#include "sdk/qtk_api.h"
#include "sdk/session/qtk_session.h"
#include "sdk/httpc/qtk_httpc.h"
#include "sdk/util/qtk_http_url.h"

#define QTK_SRVSEL_REQ_HOST    "cloud.qdreamer.com"
#define QTK_SRVSEL_REQ_POST    "80"
#define QTK_SRVSEL_REQ_URI     "/ip.search/"
#define QTK_SRVSEL_REQ_TIMEOUT 2000

#define QTK_SRVSEL_MIN_DELAY_RATE 0.5
#define QTK_SRVSEL_MAX_CPU_RATE   0.9
#define QTK_SRVSEL_NET_WEIGHT     0.5
#define QTK_SRVSEL_CPU_WEIGHT     1
#define QTK_SRVSEL_ROUTE_TIMEOUT  2000

#define QTK_SRVSEL_FAIL_TRYS 3
#define QTK_SRVSEL_FAIL_WAIT 500

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_srvsel qtk_srvsel_t;

typedef struct
{
	wtk_json_item_t *item;
	wtk_json_item_t *host;
	wtk_json_item_t *port;
	wtk_strbuf_t *buf;
	qtk_srvsel_t *s;
	int err;
	float rate;
	float delay;
	float rate2;
}qtk_srvsel_task_t;

struct qtk_srvsel
{
	qtk_session_t *session;
	wtk_string_t *host;
	wtk_string_t *port;
	wtk_strbuf_t *buf;
};

qtk_srvsel_t* qtk_srvsel_new(qtk_session_t *session);
void qtk_srvsel_delete(qtk_srvsel_t *s);
void qtk_srvsel_clean(qtk_srvsel_t *s);
int qtk_srvsel_process(qtk_srvsel_t *s);

#ifdef __cplusplus
};
#endif
#endif
