#ifndef QTK_MISC_UTIL_QTK_HTTP_URL
#define QTK_MISC_UTIL_QTK_HTTP_URL

#include "wtk/core/wtk_str_hash.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/core/wtk_type.h"
#include <ctype.h>

#ifndef IS_ALPHA
#define IS_ALPHA(c) ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
#endif

#ifndef IS_DIGITAL
#define IS_DIGITAL(c) (c >= '0' && c <= '9')
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
	wtk_queue_node_t q_n;
	wtk_string_t key;
	wtk_string_t value;
}qtk_http_url_param_t;

typedef struct{
	wtk_string_t host;
	wtk_string_t port;
	wtk_string_t uri;
	wtk_queue_t params;
}qtk_http_url_t;

qtk_http_url_t* qtk_http_url_new();
void qtk_http_url_delete(qtk_http_url_t *url);
int qtk_http_url_parse(qtk_http_url_t *url,char *urltext,int len);
void qtk_http_url_print(qtk_http_url_t *url);

void qtk_http_url_encode(wtk_strbuf_t *buf,qtk_http_url_t *url);
void qtk_http_url_encode2(wtk_strbuf_t *buf,char *uri,int len,wtk_queue_t *params);
void qtk_http_url_encode_kv(wtk_strbuf_t *buf,char *key,int keylen,char *value,int valuelen);


#ifdef __cplusplus
};
#endif
#endif
