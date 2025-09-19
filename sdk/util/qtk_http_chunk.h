#ifndef QTK_MISC_UTIL_QTK_HTTP_CHUNK
#define QTK_MISC_UTIL_QTK_HTTP_CHUNK

#include "wtk/core/wtk_strbuf.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_http_chunk qtk_http_chunk_t;

/*
 * chunk define:
   chunked-Body = *chunk
   "0" CRLF
   footer
   CRLF
   chunk = chunk-size [chunk-ext] CRLF
 */

typedef enum{
	QTK_HTTP_CHUNK_WAIT_SIZE = 0,
	QTK_HTTP_CHUNK_SIZE,
	QTK_HTTP_CHUNK_WAIT_CR,
	QTK_HTTP_CHUNK_WAIT_LF,
	QTK_HTTP_CHUNK_HDR_DONE,
	QTK_HTTP_CHUNK_0_WAIT_CR,
	QTK_HTTP_CHUNK_0_WAIT_LF,
	QTK_HTTP_CHUNK_DONE,
}qtk_http_chunk_state_t;

struct qtk_http_chunk
{
	qtk_http_chunk_state_t state;
	int chunk_size;
	wtk_strbuf_t *buf;
};

qtk_http_chunk_t* qtk_http_chunk_new();
void qtk_http_chunk_delete(qtk_http_chunk_t *c);
void qtk_http_chunk_reset(qtk_http_chunk_t *c);
int qtk_http_chunk_feed(qtk_http_chunk_t *c,char *data,int bytes,int *consumed,int *done);
void qtk_http_chunk_print(qtk_http_chunk_t *c);

#ifdef __cplusplus
};
#endif
#endif
