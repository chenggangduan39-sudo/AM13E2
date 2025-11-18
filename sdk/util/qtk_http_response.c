#include "qtk_http_response.h" 

static void qtk_http_response_init(qtk_http_response_t *rep)
{
	rep->log = NULL;

	rep->heap = NULL;
	rep->hdr_hash = NULL;
	rep->body = NULL;

	rep->buf     = NULL;
	rep->hdr_key = NULL;

	rep->status = 0;
	rep->content_length = -1;
	rep->state = QTK_RESPONSE_START;

	rep->is_chunk = 0;
	rep->chunk = NULL;
}

qtk_http_response_t *qtk_http_response_new(wtk_log_t *log)
{
	return qtk_http_response_new2(13,16384,log);
}

qtk_http_response_t *qtk_http_response_new2(int hdr_slot,int body_size,wtk_log_t *log)
{
	qtk_http_response_t *rep;

	rep = (qtk_http_response_t*)wtk_malloc(sizeof(qtk_http_response_t));
	qtk_http_response_init(rep);

	rep->log = log;

	rep->heap = wtk_heap_new(4096);
	rep->hdr_hash = wtk_str_hash_new2(hdr_slot,rep->heap);
	rep->body = wtk_strbuf_new(body_size,1);
	rep->buf = wtk_strbuf_new(256,1);
	return rep;
}

int qtk_http_response_delete(qtk_http_response_t *rep)
{
	if(rep->chunk) {
		qtk_http_chunk_delete(rep->chunk);
	}
	wtk_strbuf_delete(rep->body);
	wtk_str_hash_delete(rep->hdr_hash);
	wtk_heap_delete(rep->heap);
	wtk_strbuf_delete(rep->buf);
	wtk_free(rep);
	return 0;
}

void qtk_http_response_reset(qtk_http_response_t *rep)
{
	rep->state = QTK_RESPONSE_START;
	rep->content_length = -1;
	rep->is_chunk = 0;
	wtk_strbuf_reset(rep->buf);
	wtk_strbuf_reset(rep->body);
	if(rep->chunk) {
		qtk_http_chunk_reset(rep->chunk);
	}
	wtk_str_hash_reset(rep->hdr_hash);
	wtk_heap_reset(rep->heap);
}

static int qtk_http_response_feed_hdr(qtk_http_response_t *rep,char *data,int bytes,int *consumed)
{
	wtk_strbuf_t *buf = rep->buf;
	wtk_string_t *v;
	char *s,*e,c;
	int ret;

	s = data;
	e = data + bytes;
	while(s < e) {
		c = *s;
		switch(rep->state) {
		case QTK_RESPONSE_START:
			if(c == ' ') {
				wtk_log_warn0(rep->log,"start has space.");
				ret = -1;
				goto end;
			} else {
				wtk_strbuf_reset(buf);
				wtk_strbuf_push_c(buf,c);
				rep->state = QTK_RESPONSE_PROTOCOL;
			}
			break;

		case QTK_RESPONSE_PROTOCOL:
			if(c == ' ') {
				if(strncmp(buf->data,"HTTP/1.1",8) == 0 && buf->pos == 8) {
					rep->http_1_1 = 1;
				} else if(strncmp(buf->data,"HTTP/1.0",8) == 0 && buf->pos == 8) {
					rep->http_1_1 = 0;
				} else {
					wtk_log_warn(rep->log,"protocol illegal [%.*s].",buf->pos,buf->data);
					ret = -1;
					goto end;
				}
				wtk_strbuf_reset(buf);
				rep->state = QTK_RESPONSE_STATUS_CORE;
 			} else {
 				wtk_strbuf_push_c(buf,c);
 			}
			break;

		case QTK_RESPONSE_STATUS_CORE:
			if(c == ' ') {
				rep->status = wtk_str_atoi(buf->data,buf->pos);
				rep->state  = QTK_RESPONSE_STATUS_INFO;
			} else {
				wtk_strbuf_push_c(buf,c);
			}
			break;

		case QTK_RESPONSE_STATUS_INFO:
			if(c == ' ' || c == '\t') {
				rep->state = QTK_RESPONSE_WAIT_CR;
			} else if (c == '\r') {
				rep->state = QTK_RESPONSE_CR;
			}
			break;

		case QTK_RESPONSE_WAIT_CR:
			if(c == '\r') {
				rep->state = QTK_RESPONSE_CR;
			}
			break;

		case QTK_RESPONSE_CR:
			if(c == '\n') {
				rep->state = QTK_RESPONSE_CL;
			} else {
				wtk_log_warn0(rep->log,"CLCR format illegal.");
				ret = -1;
				goto end;
			}
			break;

		case QTK_RESPONSE_CL:
			if(c == '\r') {
				rep->state = QTK_RESPONSE_HDR_ALMOST_DONE;
			} else {
				wtk_strbuf_reset(buf);
				wtk_strbuf_push_c(buf,tolower(c));
				rep->state = QTK_RESPONSE_KEY;
			}
			break;

		case QTK_RESPONSE_KEY:
			if(c == ':') {
				wtk_strbuf_strip(buf);
				rep->hdr_key = wtk_heap_dup_string(rep->heap,buf->data,buf->pos);
				rep->state = QTK_RESPONSE_KEY_SPACE;
			} else {
				wtk_strbuf_push_c(buf,tolower(c));
			}
			break;

		case QTK_RESPONSE_KEY_SPACE:
			if(c != ' ') {
				wtk_strbuf_reset(buf);
				wtk_strbuf_push_c(buf,c);
				rep->state = QTK_RESPONSE_VALUE;
			}
			break;

		case QTK_RESPONSE_VALUE:
			if(c == '\r') {
				hash_str_node_t *node;

				wtk_strbuf_strip(buf);
				node = wtk_str_hash_find_node(rep->hdr_hash,rep->hdr_key->data,rep->hdr_key->len,0);
				if(node) {
					v = (wtk_string_t*)node->value;
					wtk_strbuf_push_front_s(buf,";");
					wtk_strbuf_push_front(buf,v->data,v->len);
					wtk_heap_fill_string(rep->heap,v,buf->data,buf->pos);
				} else {
					v = wtk_heap_dup_string(rep->heap,buf->data,buf->pos);
					wtk_str_hash_add(rep->hdr_hash,rep->hdr_key->data,rep->hdr_key->len,v);
				}
				rep->state = QTK_RESPONSE_CR;
			} else {
				wtk_strbuf_push_c(buf,c);
			}
			break;

		case QTK_RESPONSE_HDR_ALMOST_DONE:
			if(c == '\n') {
				rep->state = QTK_RESPONSE_HDR_DONE;
				++s;
				ret = 0;
				goto end;
			} else {
				wtk_log_warn0(rep->log,"End CLCR format illegal.");
			}
			break;

		default:
			break;
		}
		++s;
	}

	ret = 0;
end:
	if(ret == 0) {
		*consumed = s - data;
	} else {
		*consumed = 0;
	}
	return ret;
}


static int qtk_http_response_update_hdr(qtk_http_response_t *rep)
{
	wtk_string_t *v;
	int ret = 0;

	v = (wtk_string_t*)wtk_str_hash_find_s(rep->hdr_hash,"content-length");
	if(!v) {
		v = (wtk_string_t*)wtk_str_hash_find_s(rep->hdr_hash,"transfer-encoding");
		if(v && (wtk_string_cmp_s(v,"chunked")) == 0) {
			if(!rep->chunk) {
				rep->chunk = qtk_http_chunk_new();
			}
			rep->is_chunk = 1;
			rep->content_length = -1;
		} else {
			ret = -1;
		}
	} else {
		rep->content_length = wtk_str_atoi(v->data,v->len);
	}
	return ret;
}

static int qtk_http_response_hdr(qtk_http_response_t *rep,char *data,int bytes,int *consumed)
{
	int ret = 0;

	ret = qtk_http_response_feed_hdr(rep,data,bytes,consumed);
	if(ret != 0) {
		wtk_log_warn(rep->log,"hdr parse failed [%.*s].",bytes,data);
		goto end;
	}

	ret = qtk_http_response_update_hdr(rep);
	if(ret != 0 && rep->state == QTK_RESPONSE_HDR_DONE) {
		wtk_log_warn0(rep->log,"response has no content-length and has no chunk.");
		goto end;
	}

	ret = 0;
end:
	return ret;
}

static int qtk_http_response_feed_content(qtk_http_response_t *rep,char *data,int bytes,int *consumed)
{
	int cpy;

	cpy = min(bytes,rep->content_length - rep->body->pos);
	wtk_strbuf_push(rep->body,data,cpy);
	if(rep->body->pos == rep->content_length) {
		rep->state = QTK_RESPONSE_CONTENT_DONE;
	}

	*consumed = cpy;

	return 0;
}

static int qtk_http_response_feed_chunk(qtk_http_response_t *rep,char *data,int bytes,int *consumed)
{
	qtk_http_chunk_t *chunk = rep->chunk;
	int ret = 0;
	char *s,*e;
	int cnt,done;

	s = data;
	e = data + bytes;
	*consumed = 0;
	while(s < e) {
		cnt = 0;
		done = 0;
		ret = qtk_http_chunk_feed(chunk,s,e-s,&cnt,&done);
		if(ret != 0) {
			goto end;
		}
		s += cnt;
		if(done) {
			if(chunk->chunk_size > 0) {
				wtk_strbuf_push(rep->body,chunk->buf->data,chunk->buf->pos);
				qtk_http_chunk_reset(chunk);
			} else {
				rep->state = QTK_RESPONSE_CONTENT_DONE;
				qtk_http_chunk_reset(chunk);
				goto end;
			}
		}
	}
	ret = 0;
end:
	if(ret != 0) {
		*consumed = s - data;
	}
	return ret;
}

static int qtk_http_response_body(qtk_http_response_t *rep,char *data,int bytes,int *consumed)
{
	int ret;

	if(rep->is_chunk) {
		ret = qtk_http_response_feed_chunk(rep,data,bytes,consumed);
	} else {
		ret = qtk_http_response_feed_content(rep,data,bytes,consumed);
	}

	return ret;
}

int qtk_http_response_feed(qtk_http_response_t *rep,char *data,int bytes,int *left,int *done)
{
	int ret;
	int consumed;

	consumed = 0;
	*left = bytes;
	if(rep->state != QTK_RESPONSE_HDR_DONE) {
		ret = qtk_http_response_hdr(rep,data,*left,&consumed);
		if(ret != 0) {
			goto end;
		}
		*left -= consumed;
	}

	if(rep->state == QTK_RESPONSE_HDR_DONE && rep->content_length == 0) {
		rep->state = QTK_RESPONSE_CONTENT_DONE;
	}

	if(*left > 0 && rep->state == QTK_RESPONSE_HDR_DONE) {
		ret = qtk_http_response_body(rep,data+consumed,*left,&consumed);
		if(ret != 0) {
			goto end;
		}
		*left -= consumed;
	}

	*done = rep->state == QTK_RESPONSE_CONTENT_DONE;

	ret = 0;
end:
	if(ret != 0) {
		*left = bytes;
		*done = 0;
	}
	return ret;
}

int qtk_http_response_print_hdr(void *ths,hash_str_node_t *node)
{
	wtk_string_t *v;

	v = (wtk_string_t*)node->value;
	printf("%.*s = [%.*s]\n",node->key.len,node->key.data,v->len,v->data);
	return 0;
}

void qtk_http_response_print(qtk_http_response_t *rep)
{
	printf("========================= response =======================.\n");
	printf("http_1_1 = %d\n",rep->http_1_1);
	printf("status = %d\n",rep->status);
	printf("content-length = %d\n",rep->content_length);
	printf("is_chunk = %d\n",rep->is_chunk);
	printf("==== header ====\n");
	wtk_str_hash_walk(rep->hdr_hash,(wtk_walk_handler_t)qtk_http_response_print_hdr,NULL);
	printf("==== body ====\n");
	printf("body = %d [%.*s]\n",rep->body->pos,rep->body->pos,rep->body->data);
	printf("\n");
}
