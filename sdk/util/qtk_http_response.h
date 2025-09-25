#ifndef QTK_MISC_UTIL_QTK_HTTP_RESPONSE
#define QTK_MISC_UTIL_QTK_HTTP_RESPONSE

#include "wtk/core/wtk_str_hash.h"
#include "wtk/core/wtk_heap.h"
#include "wtk/core/wtk_strbuf.h"
#include "qtk_http_chunk.h"
#include "wtk/core/wtk_type.h"
#include <ctype.h>
#include "wtk/os/wtk_log.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_http_response qtk_http_response_t;

typedef enum {
	QTK_RESPONSE_START = 0,
	QTK_RESPONSE_PROTOCOL,
	QTK_RESPONSE_STATUS_CORE,
	QTK_RESPONSE_STATUS_INFO,
	QTK_RESPONSE_WAIT_CR,
	QTK_RESPONSE_CR,
	QTK_RESPONSE_CL,
	QTK_RESPONSE_KEY,
	QTK_RESPONSE_KEY_SPACE,
	QTK_RESPONSE_VALUE,
	QTK_RESPONSE_HDR_ALMOST_DONE,
	QTK_RESPONSE_HDR_DONE,
	QTK_RESPONSE_CONTENT_DONE,
}qtk_http_response_state_t;

struct qtk_http_response
{
	wtk_log_t *log;
	wtk_heap_t *heap;
	wtk_str_hash_t *hdr_hash;
	wtk_strbuf_t *body;
	wtk_strbuf_t *buf;
	wtk_string_t *hdr_key;
	qtk_http_chunk_t *chunk;
	qtk_http_response_state_t state;
	int status;
	int content_length;
	unsigned is_chunk:1;
	unsigned http_1_1:1;
};

qtk_http_response_t *qtk_http_response_new(wtk_log_t *log);
qtk_http_response_t *qtk_http_response_new2(int hdr_slot,int body_size,wtk_log_t *log);
int qtk_http_response_delete(qtk_http_response_t *rep);

void qtk_http_response_reset(qtk_http_response_t *rep);

int qtk_http_response_feed(qtk_http_response_t *rep,char *data,int bytes,int *left,int *done);

void qtk_http_response_print(qtk_http_response_t *rep);

#ifdef __cplusplus
};
#endif
#endif
