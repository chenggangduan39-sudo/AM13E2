#ifndef WTK_HTTP_MISC_HTTPC_DNS_WTK_DNS_CFG
#define WTK_HTTP_MISC_HTTPC_DNS_WTK_DNS_CFG

#include "wtk/core/cfg/wtk_local_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_dns_cfg wtk_dns_cfg_t;
struct wtk_dns_cfg
{
	wtk_string_t cache_path;
	float cache_day;
	int dns_timeout;
	unsigned use_cache:1;
	unsigned use_dnsc:1;
};

int wtk_dns_cfg_init(wtk_dns_cfg_t *cfg);
int wtk_dns_cfg_clean(wtk_dns_cfg_t *cfg);
int wtk_dns_cfg_update_local(wtk_dns_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_dns_cfg_update(wtk_dns_cfg_t *cfg);

void wtk_dns_cfg_update_srv(wtk_dns_cfg_t *cfg,wtk_string_t *cache_path);

#ifdef __cplusplus
};
#endif
#endif
