#ifndef QTK_SESSION_OPTION_QTK_OPTION
#define QTK_SESSION_OPTION_QTK_OPTION

#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/wtk_heap.h"
#include "wtk/os/wtk_log.h"

#include "sdk/session/auth/qtk_rsa.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_option qtk_option_t;

#define QTK_OPTVAL_PUBLIC_KEY "-----BEGIN RSA PUBLIC KEY-----\n\
MIGJAoGBALduScYh/dyHQm5uwZd0h/Wn9iqp7iczKehh2y93oWY8GHVdoFKxFddm\n\
reMrJKZmPzXAN9UfxLbOZagDfa0yZAycUBy6l5v/Avm2b+S0zIm+14T2MGg5MFTv\n\
A8DuIGO/lYyJOgD/vze03YGo3bXXVD+mmZuuXn8LOdlhNnntXV8pAgMBAAE=\n\
-----END RSA PUBLIC KEY-----\n\
"
#define QTK_OPTVAL_PHRASE   "qdreamer"
#define QTK_OPTVAL_PORT     "80"
#define QTK_OPTVAL_URL1     "/api/v3.0/score"
#define QTK_OPTVAL_URL2     "/"
#define QTK_OPTVAL_URL_AUTH "/auth"

#define QTK_OPTVAL_CLDLOG_HOST "cloud.qdreamer.com"
#define QTK_OPTVAL_CLDLOG_PORT "80"
#define QTK_OPTVAL_CLDLOG_URL  "/log/device/"
#define QTK_OPTVAL_CLDLOG_TIMOUT 2000

#define QTK_OPTDEF_LOG  "qvoice.log"
#define QTK_OPTDEF_DNS  "qvoice.dns"
#define QTK_OPTDEF_AUTH "qvoice.auth"

#define QTK_OPTKEY_PUBLIC_KEY "public_key"
#define QTK_OPTKEY_PHRASE     "phrase"
#define QTK_OPTKEY_CACHE_PATH "cache_path"
#define QTK_OPTKEY_LOG		  "log"
#define QTK_OPTKEY_DNS		  "dns"
#define QTK_OPTKEY_AUTH		  "auth"
#define QTK_OPTKEY_APPID	  "appid"
#define QTK_OPTKEY_SECRETKEY  "secretkey"
#define QTK_OPTKEY_USRID	  "usrid"          //reserved
#define QTK_OPTKEY_HOST		  "host"
#define QTK_OPTKEY_PORT		  "port"
#define QTK_OPTKEY_URL        "url"            //reserved
#define QTK_OPTKEY_TIMEOUT    "timeout"
#define QTK_OPTKEY_SRVSEL_FLG "use_srvsel"
#define QTK_OPTKEY_AUTH_FLG   "use_auth_cache"
#define QTK_OPTKEY_LOG_FLG    "use_log"
#define QTK_OPTKEY_CLDHUB_FLG "use_cldhub"
#define QTK_OPTKEY_LOGWAV_FLG "log_wav"
#define QTK_OPTKEY_TIMER_FLG  "use_timer"

#define QTK_OPTKEY_CLDLOG_HOST_FLG "cldlog_host"
#define QTK_OPTKEY_CLDLOG_PORT_FLG "cldlog_port"
#define QTK_OPTKEY_CLDLOG_URL_FLG "cldlog_url"
#define QTK_OPTKEY_CLDLOG_TIMEOUT_FLG "cldlog_timeout"
#define QTK_OPTKEY_CLDLOG_FLG "use_cldlog"

struct qtk_option
{
	wtk_heap_t *heap;
	qtk_rsa_t *rsa;
	wtk_string_t *public_key;
	wtk_string_t *phrase;

	wtk_string_t *cache_path;
	wtk_string_t *log_fn;
	wtk_string_t *dns_fn;
	wtk_string_t *auth_fn;

	wtk_string_t *appid;
	wtk_string_t *secretkey;
	wtk_string_t *usrid;

	wtk_string_t *cldlog_host;
	wtk_string_t *cldlog_port;
	wtk_string_t *cldlog_url;
	int cldlog_timeout;

	wtk_string_t *host;
	wtk_string_t *port;
	wtk_string_t *url1;
	wtk_string_t *url2;
	wtk_string_t *url_auth;
	int timeout;
	unsigned long long srvsel_update_period; //动态获取服务器ip间隔时间，单位h
	unsigned use_srvsel:1;
	unsigned use_auth_cache:1;
	unsigned use_log:1;
	unsigned use_cldhub:1;
	unsigned log_wav:1;
	unsigned use_timer:1;
	unsigned use_cldlog:1;
};

void qtk_option_init(qtk_option_t *opt);
void qtk_option_clean(qtk_option_t *opt);

void qtk_option_new_rsa(qtk_option_t *opt);
void qtk_option_set_usrid(qtk_option_t *opt,char *usrid,int len);
void qtk_option_set_host(qtk_option_t *opt,char *host,int len);
void qtk_option_set_port(qtk_option_t *opt,char *port,int len);

int qtk_option_update_params(qtk_option_t *opt,wtk_local_cfg_t *params);

void qtk_option_print(qtk_option_t *opt,wtk_log_t *log);

#ifdef __cplusplus
};
#endif
#endif
