#ifndef QTK_MISC_HTTPC_COOKIE_QTK_COOKIE_CFG
#define QTK_MISC_HTTPC_COOKIE_QTK_COOKIE_CFG

#include "wtk/core/cfg/wtk_local_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_cookie_cfg qtk_cookie_cfg_t;
struct qtk_cookie_cfg
{
	wtk_local_cfg_t *lc;
	wtk_str_hash_t *hash;
	wtk_strbuf_t *cookie;
	wtk_string_t str;
	unsigned update_cookie:1;
};

int qtk_cookie_cfg_init(qtk_cookie_cfg_t *cfg);
int qtk_cookie_cfg_clean(qtk_cookie_cfg_t *cfg);
int qtk_cookie_cfg_update_local(qtk_cookie_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_cookie_cfg_update(qtk_cookie_cfg_t *cfg);

void wtk_cookie_cfg_update_cookie(qtk_cookie_cfg_t *cfg,char *data,int len);

#ifdef __cplusplus
};
#endif
#endif
