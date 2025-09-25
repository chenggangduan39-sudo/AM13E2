#ifndef QTK_MISC_HTTPC_QTK_HTTPC_CFG
#define QTK_MISC_HTTPC_QTK_HTTPC_CFG

#include "wtk/core/cfg/wtk_local_cfg.h"
#include "qtk/dns/qtk_dns_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_httpc_cfg qtk_httpc_cfg_t;
struct qtk_httpc_cfg
{
	qtk_dns_cfg_t dns;
	wtk_string_t host;
	wtk_string_t port;
	wtk_string_t url;
	int timeout;
	int hearbeat_time;
	unsigned http_1_1:1;
	unsigned use_stage:1;
	unsigned log_http:1;
};

int qtk_httpc_cfg_init(qtk_httpc_cfg_t *cfg);
int qtk_httpc_cfg_clean(qtk_httpc_cfg_t *cfg);
int qtk_httpc_cfg_update_local(qtk_httpc_cfg_t *cfg,wtk_local_cfg_t *main);
int qtk_httpc_cfg_update(qtk_httpc_cfg_t *cfg);

void qtk_httpc_cfg_update_opt(qtk_httpc_cfg_t *cfg,
							  wtk_string_t *host,
							  wtk_string_t *port,
							  wtk_string_t *url,
							  wtk_string_t *dns_fn,
							  int timeout);

#ifdef __cplusplus
};
#endif
#endif
