#include "qtk_http_url.h"

qtk_http_url_t* qtk_http_url_new()
{
	qtk_http_url_t *url;

	url=(qtk_http_url_t*)wtk_malloc(sizeof(qtk_http_url_t));
	wtk_string_set(&(url->host),0,0);
	wtk_string_set_s(&(url->port),"80");
	wtk_string_set_s(&(url->uri),"/");
	wtk_queue_init(&(url->params));
	return url;
}

void qtk_http_url_delete(qtk_http_url_t *url)
{
	wtk_queue_node_t *qn;
	qtk_http_url_param_t *param;

	while(1) {
		qn=wtk_queue_pop(&(url->params));
		if(!qn) {
			break;
		}
		param=data_offset2(qn,qtk_http_url_param_t,q_n);
		wtk_free(param);
	}
	wtk_free(url);
}

int qtk_http_url_parse(qtk_http_url_t *url,char *urltext,int len)
{
typedef enum{
	QTK_HTTP_URL_PARSE_INIT,
	QTK_HTTP_URL_PARSE_HTTP,
	QTK_HTTP_URL_PARSE_COLON,
	QTK_HTTP_URL_PARSE_SLASH1,
	QTK_HTTP_URL_PARSE_SLASH2,
	QTK_HTTP_URL_PARSE_HOST,
	QTK_HTTP_URL_PARSE_PORT,
	QTK_HTTP_URL_PARSE_URI,
	QTK_HTTP_URL_PARSE_PARAMKEY,
	QTK_HTTP_URL_PARSE_PARAMVALUE,
}qtk_http_url_state_t;
	qtk_http_url_param_t *param;
	qtk_http_url_state_t state;
	wtk_string_t key,value;
	char *s,*e,*k = NULL;
	int ret;

	state = QTK_HTTP_URL_PARSE_INIT;
	s = urltext;
	e = urltext + len;
	while(s < e) {
		switch(state) {
		case QTK_HTTP_URL_PARSE_INIT:
			if(*s == 'h') {
				k = s;
				state = QTK_HTTP_URL_PARSE_HTTP;
			}else if (*s != ' ') {
				ret = -1;	goto end;
			}
			break;
		case QTK_HTTP_URL_PARSE_HTTP:
			if( *s == ':') {
				if((s - k) != 4 && (s - k) != 5) {
					ret = -1; goto end;
				}
				if(strncmp(k,"http",4) == 0 || strncmp(k,"https",5) == 0) {
				} else {
					ret = -1; goto end;
				}
				state = QTK_HTTP_URL_PARSE_COLON;
			}
			break;
		case QTK_HTTP_URL_PARSE_COLON:
			if(*s == '/') {
				state = QTK_HTTP_URL_PARSE_SLASH1;
			}else {
				ret = -1;goto end;
			}
			break;
		case QTK_HTTP_URL_PARSE_SLASH1:
			if(*s == '/') {
				state = QTK_HTTP_URL_PARSE_SLASH2;
			}else {
				ret = -1; goto end;
			}
			break;
		case QTK_HTTP_URL_PARSE_SLASH2:
			if( *s == ' ' ) {
				ret = -1;  goto end;
			}
			k = s;
			state = QTK_HTTP_URL_PARSE_HOST;
			break;
		case QTK_HTTP_URL_PARSE_HOST:
			if( *s == ':' ) {
				wtk_string_set(&(url->host),k,s-k);
				state = QTK_HTTP_URL_PARSE_PORT;
				k = s + 1;
			}else if( *s == '/' ) {
				wtk_string_set(&(url->host),k,s-k);
				state = QTK_HTTP_URL_PARSE_URI;
				k = s;
			}else if( *s == ' ') {
				ret = -1; goto end;
			}
			break;
		case QTK_HTTP_URL_PARSE_PORT:
			if( *s == '/' ) {
				wtk_string_set(&(url->port),k,s-k);
				state = QTK_HTTP_URL_PARSE_URI;
				k = s;
			}else if( *s < '0' || *s > '9' ) {
				ret = -1; goto end;
			}
			break;
		case QTK_HTTP_URL_PARSE_URI:
			if( *s == '?') {
				wtk_string_set(&(url->uri),k,s-k);
				k = s + 1;
				state = QTK_HTTP_URL_PARSE_PARAMKEY;
			}
			break;
		case QTK_HTTP_URL_PARSE_PARAMKEY:
			if( *s == '=') {
				state = QTK_HTTP_URL_PARSE_PARAMVALUE;
				wtk_string_set(&(key),k,s-k);
				k = s + 1;
			}
			break;
		case QTK_HTTP_URL_PARSE_PARAMVALUE:
			if ( *s == '&') {
				state = QTK_HTTP_URL_PARSE_PARAMKEY;
				wtk_string_set(&(value),k,s-k);
				param=(qtk_http_url_param_t*)wtk_malloc(sizeof(qtk_http_url_param_t));
				param->key=key;
				param->value=value;
				wtk_queue_push(&(url->params),&(param->q_n));
			}
			break;
		}
		++s;
	}

	switch(state) {
	case QTK_HTTP_URL_PARSE_HOST:
		wtk_string_set(&(url->host),k,s-k);
		break;
	case QTK_HTTP_URL_PARSE_PORT:
		wtk_string_set(&(url->port),k,s-k);
		break;
	case QTK_HTTP_URL_PARSE_URI:
		wtk_string_set(&(url->uri),k,s-k);
		break;
	case QTK_HTTP_URL_PARSE_PARAMVALUE:
		wtk_string_set(&(value),k,s-k);
		param=(qtk_http_url_param_t*)wtk_malloc(sizeof(qtk_http_url_param_t));
		param->key=key;
		param->value=value;
		wtk_queue_push(&(url->params),&(param->q_n));
		break;
	default:
		ret = -1; goto end;
		break;
	}
	ret = 0;
end:
	//wtk_debug("ret = %d.\n",ret);
	return ret;
}

void qtk_http_url_print(qtk_http_url_t *url)
{
	wtk_queue_node_t *qn;
	qtk_http_url_param_t *param;

	printf("=================url===========.\n");
	printf("host = %.*s.\n",url->host.len,url->host.data);
	printf("port = %.*s.\n",url->port.len,url->port.data);
	printf("uri = %.*s.\n",url->uri.len,url->uri.data);
	printf("=======params=====\n");
	for(qn = url->params.pop;qn;qn = qn->next) {
		param = data_offset2(qn,qtk_http_url_param_t,q_n);
		printf("key/value = %.*s/%.*s\n",param->key.len,param->key.data,param->value.len,param->value.data);
	}
}

static void qtk_http_url_encode_uri(wtk_strbuf_t *buf,char *uri,int bytes)
{
	static const char *digits = "0123456789ABCDEF";
	char *s,*e;
	char *dst;
	char c;

	s = uri;
	e = uri + bytes;
	if((buf->length - buf->pos) <= bytes) {
		wtk_strbuf_expand(buf,bytes * 3);
	}
	dst = buf->data + buf->pos;

	while(s < e) {
		c = *s;

		if(isascii(c))
		{
			if(c==' ')
			{
				*dst++ = '%';
				*dst++ = '2';
				*dst++ = '0';
			}else
			{
				*dst++ = c;
			}
		}else
		{
			*dst++ = '%';
			*dst++ = digits[(c>>4) & 0x0F];
			*dst++ = digits[c & 0x0F];
		}
		++s;
	}
	buf->pos = dst - buf->data;
}

void qtk_http_url_encode_param(wtk_strbuf_t *buf,char *data,int bytes)
{
	static const char *digits = "0123456789ABCDEF";
	char *s,*e;
	char *dst;
	char c;

	s = data;
	e = data + bytes;
	if((buf->length - buf->pos) <= bytes) {
		wtk_strbuf_expand(buf,bytes * 3);
	}
	dst = buf->data + buf->pos;

	while(s < e) {
		c = *s;

		if(IS_ALPHA(c) || IS_DIGITAL(c) || c == '.' || c == '_') {
			*dst++ = c;
		} else if(c == ' ') {
			*dst++ = '+';
		} else {
			*dst++ = '%';
			*dst++ = digits[(c >> 4) & 0X0F];
			*dst++ = digits[c & 0X0F];
		}
		++s;
	}
	buf->pos = dst - buf->data;
}

void qtk_http_url_encode(wtk_strbuf_t *buf,qtk_http_url_t *url)
{
	qtk_http_url_encode2(buf,url->uri.data,url->uri.len,&(url->params));
}

void qtk_http_url_encode2(wtk_strbuf_t *buf,char *uri,int len,wtk_queue_t *params)
{
	wtk_queue_node_t *qn;
	qtk_http_url_param_t *param;
	int b = 1;

	wtk_strbuf_reset(buf);
	qtk_http_url_encode_uri(buf,uri,len);

	for(qn = params->pop;qn;qn = qn->next) {
		if(b) {
			wtk_strbuf_push_c(buf,'?');
			b = 0;
		} else {
			wtk_strbuf_push_c(buf,'&');
		}
		param = data_offset2(qn,qtk_http_url_param_t,q_n);
		qtk_http_url_encode_uri(buf,param->key.data,param->key.len);

		wtk_strbuf_push_c(buf,'=');

		qtk_http_url_encode_uri(buf,param->value.data,param->value.len);
	}
}


void qtk_http_url_encode_kv(wtk_strbuf_t *buf,char *key,int keylen,char *value,int valuelen)
{
	wtk_strbuf_reset(buf);
	qtk_http_url_encode_uri(buf,key,keylen);
	wtk_strbuf_push_c(buf,'=');
	qtk_http_url_encode_uri(buf,value,valuelen);
}

