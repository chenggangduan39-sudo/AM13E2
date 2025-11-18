#ifndef WTK_HTTP_MISC_HTTPC_DNS_WTK_DNS_CACHE
#define WTK_HTTP_MISC_HTTPC_DNS_WTK_DNS_CACHE

#include "wtk/core/json/wtk_json_parse.h"
#include "wtk/os/wtk_log.h"

#include "qtk/core/qtk_def.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_dns_cache wtk_dns_cache_t;
struct wtk_dns_cache
{
	wtk_log_t *log;
	wtk_json_parser_t *parser;
	wtk_strbuf_t *buf;
	float cache_ms;
	char *cache_fn;
};

wtk_dns_cache_t* wtk_dns_cache_new(wtk_log_t *log,char *cache_path,int len,float cache_day);
void wtk_dns_cache_delete(wtk_dns_cache_t *dc);

wtk_string_t wtk_dns_cache_find(wtk_dns_cache_t *dc,char *domain,int len);
int wtk_dns_cache_save(wtk_dns_cache_t *dc,char *domain,int dlen,char *host,int hlen);
void wtk_dns_cache_clean(wtk_dns_cache_t *dc,char *domain,int len);

#ifdef __cplusplus
};
#endif
#endif
