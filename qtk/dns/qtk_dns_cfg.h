#ifndef QTK_MISC_DNS_QTK_DNS_CFG
#define QTK_MISC_DNS_QTK_DNS_CFG

#include "wtk/core/cfg/wtk_local_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_dns_cfg qtk_dns_cfg_t;
struct qtk_dns_cfg
{
	int timeout;
	unsigned use_dnsc:1;
};

int qtk_dns_cfg_init(qtk_dns_cfg_t *cfg);
int qtk_dns_cfg_clean(qtk_dns_cfg_t *cfg);
int qtk_dns_cfg_update_local(qtk_dns_cfg_t *cfg,wtk_local_cfg_t *main);
int qtk_dns_cfg_update(qtk_dns_cfg_t *cfg);

#ifdef __cplusplus
};
#endif
#endif
