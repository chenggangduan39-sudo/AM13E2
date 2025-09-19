#include "qtk_http_chunk.h" 
#include <ctype.h>

qtk_http_chunk_t* qtk_http_chunk_new()
{
	qtk_http_chunk_t *c;

	c = (qtk_http_chunk_t*)wtk_malloc(sizeof(qtk_http_chunk_t));
	c->buf = wtk_strbuf_new(1024,1);
	qtk_http_chunk_reset(c);
	return c;
}

void qtk_http_chunk_delete(qtk_http_chunk_t *c)
{
	wtk_strbuf_delete(c->buf);
	wtk_free(c);
}

void qtk_http_chunk_reset(qtk_http_chunk_t *c)
{
	c->state = QTK_HTTP_CHUNK_WAIT_SIZE;
	c->chunk_size = 0;
	wtk_strbuf_reset(c->buf);
}

void qtk_http_chunk_feed_hdr(qtk_http_chunk_t *c,char *data,int bytes,int *consumed)
{
	wtk_strbuf_t *buf = c->buf;
	char ch;
	char *s,*e;

	s = data;
	e = data + bytes;
	while(s < e) {
		ch = *s;
		switch(c->state) {
		case QTK_HTTP_CHUNK_WAIT_SIZE:
			if(!isspace(ch)) {
				wtk_strbuf_reset(buf);
				wtk_strbuf_push_c(buf,ch);
				c->state = QTK_HTTP_CHUNK_SIZE;
			}
			break;
		case QTK_HTTP_CHUNK_SIZE:
			if(isspace(ch)) {
				wtk_strbuf_push_c(buf,0);
				c->chunk_size = strtol(buf->data,0,16);
				c->state = (ch=='\r')?QTK_HTTP_CHUNK_WAIT_LF:QTK_HTTP_CHUNK_WAIT_CR;
			} else {
				wtk_strbuf_push_c(buf,ch);
			}
			break;
		case QTK_HTTP_CHUNK_WAIT_CR:
			if(ch == '\r') {
				c->state = QTK_HTTP_CHUNK_WAIT_LF;
			}
			break;
		case QTK_HTTP_CHUNK_WAIT_LF:
			if(ch == '\n') {
				c->state = QTK_HTTP_CHUNK_HDR_DONE;
				wtk_strbuf_reset(buf);
				++s;
				goto end;
			}
			break;
		default:
			goto end;
			break;
		}
		++s;
	}
end:
	*consumed = s - data;
	return;
}

void qtk_http_chunk_feed_chunk0(qtk_http_chunk_t *c,char *data,int bytes,int *consumed)
{
	char *s,*e;
	char ch;

	s = data;
	e = data + bytes;
	while(s < e) {
		ch = *s;
		switch(c->state) {
		case QTK_HTTP_CHUNK_0_WAIT_CR:
			if(ch == '\r') {
				c->state = QTK_HTTP_CHUNK_0_WAIT_LF;
			}
			break;
		case QTK_HTTP_CHUNK_0_WAIT_LF:
			if(ch == '\n') {
				c->state = QTK_HTTP_CHUNK_DONE;
				++s;
				goto end;
			}
			break;
		default:
			goto end;
			break;
		}
		++s;
	}
end:
	*consumed = s - data;
	return;
}

int qtk_http_chunk_feed(qtk_http_chunk_t *c,char *data,int bytes,int *consumed,int *done)
{
	int ret = 0;
	int cnt,step;

	*consumed = 0;
	*done = 0;

	if(c->state < QTK_HTTP_CHUNK_HDR_DONE) {
		qtk_http_chunk_feed_hdr(c,data,bytes,&cnt);
		data += cnt;
		bytes -= cnt;
		*consumed += cnt;
		if(bytes <= 0) {
			goto end;
		}
		if(c->chunk_size == 0) {
			c->state = QTK_HTTP_CHUNK_0_WAIT_CR;
		}
	}

	if(c->chunk_size == 0) {
		qtk_http_chunk_feed_chunk0(c,data,bytes,&cnt);
		data += cnt;
		bytes -= cnt;
		*consumed += cnt;
	} else {
		cnt = c->chunk_size = c->buf->pos;
		step = min(cnt,bytes);
		if(step > 0) {
			wtk_strbuf_push(c->buf,data,step);
			*consumed += step;
		}
		if(cnt <= step) {
			c->state = QTK_HTTP_CHUNK_DONE;
			*done = 1;
		} else {
			*done = 0;
		}
	}
	*done = c->state == QTK_HTTP_CHUNK_DONE;
end:
	return ret;
}


void qtk_http_chunk_print(qtk_http_chunk_t *c)
{
	printf("chunk size: %d\n",c->chunk_size);
	printf("feed bytes: %d\n",c->buf->pos);
}
