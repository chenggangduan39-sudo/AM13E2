#include "wtk_dns_cache.h" 

#define WTK_DNS_CACHE_BUFSIZE 4096
static wtk_lock_t glb_dns_lock;

void wtk_dns_cache_init(wtk_dns_cache_t *dc)
{
	dc->log = NULL;
	dc->parser = NULL;
	dc->buf = NULL;
	dc->cache_fn = NULL;
}

wtk_dns_cache_t* wtk_dns_cache_new(wtk_log_t *log,char *cache_path,int len,float cache_day)
{
	wtk_dns_cache_t *dc;

	dc = (wtk_dns_cache_t*)wtk_malloc(sizeof(wtk_dns_cache_t));
	wtk_dns_cache_init(dc);

	dc->log = log;

	dc->cache_fn = (char*)wtk_malloc(len+sizeof(QTK_DNS_FN)+8);
#ifdef WIN32
	sprintf(dc->cache_fn,"%.*s\\%s",len,cache_path,QTK_DNS_FN);
#else
	sprintf(dc->cache_fn,"%.*s/%s",len,cache_path,QTK_DNS_FN);
#endif

	dc->cache_ms = cache_day * 86400000.0;

	dc->parser = wtk_json_parser_new();
	dc->buf = wtk_strbuf_new(256,1);

	wtk_lock_init(&glb_dns_lock);
	return dc;
}

void wtk_dns_cache_delete(wtk_dns_cache_t *dc)
{
	if(dc->cache_fn) {
		wtk_free(dc->cache_fn);
	}
	wtk_strbuf_delete(dc->buf);
	wtk_json_parser_delete(dc->parser);
	wtk_free(dc);
}

wtk_string_t wtk_dns_cache_find(wtk_dns_cache_t *dc,char *domain,int len)
{
	FILE *fp;
	wtk_string_t v;
	wtk_json_parser_t *parser = dc->parser;
	wtk_json_item_t *item,*item1;
	wtk_strbuf_t *buf = dc->buf;
	double t;
	char tmp[WTK_DNS_CACHE_BUFSIZE];
	int ret;

	wtk_lock_lock(&glb_dns_lock);
	fp = fopen(dc->cache_fn,"rb");
	if(!fp) {
		ret = -1;
		wtk_lock_unlock(&glb_dns_lock);
		goto end;
	}

	wtk_strbuf_reset(buf);
	while(1) {
		ret = fread(tmp,1,WTK_DNS_CACHE_BUFSIZE,fp);
		if(ret <= 0) {
			break;
		}
		wtk_strbuf_push(buf,tmp,ret);
	}
	fclose(fp);
	wtk_lock_unlock(&glb_dns_lock);

	if(buf->pos <= 0) {
		ret = -1;
		goto end;
	}

	wtk_json_parser_reset(parser);
	ret = wtk_json_parser_parse(parser,buf->data,buf->pos);
	if(ret != 0 || !parser->json->main) {
		ret = -1;
		goto end;
	}

	item = wtk_json_obj_get(parser->json->main,domain,len);
	if(!item || item->type != WTK_JSON_OBJECT) {
		ret = -1;
		goto end;
	}

	t = time_get_ms();
	item1 = wtk_json_obj_get_s(item,"deadline");
	if(!item1 || item1->type != WTK_JSON_NUMBER || item1->v.number < t) {
		ret = -1;
		goto end;
	}

	item1 = wtk_json_obj_get_s(item,"host");
	if(item1 && item1->type == WTK_JSON_STRING && item1->v.str->len > 0) {
		wtk_string_set(&v,item1->v.str->data,item1->v.str->len);
	} else {
		ret = -1;
		goto end;
	}

	ret = 0;
end:
	if(ret != 0) {
		wtk_string_set(&v,0,0);
	}
	return v;
}

int wtk_dns_cache_save(wtk_dns_cache_t *dc,char *domain,int dlen,char *host,int hlen)
{
	FILE *fp;
	wtk_json_parser_t *parser = dc->parser;
	wtk_json_item_t *item = NULL,*item1;
	wtk_strbuf_t *buf = dc->buf;
	char tmp[WTK_DNS_CACHE_BUFSIZE];
	double t;
	int ret;

	wtk_lock_lock(&glb_dns_lock);
	fp = fopen(dc->cache_fn,"rb");
	if(!fp) {
		goto middle;
	}
	wtk_strbuf_reset(buf);
	while(1) {
		ret = fread(tmp,1,WTK_DNS_CACHE_BUFSIZE,fp);
		if(ret <= 0) {
			break;
		}
		wtk_strbuf_push(buf,tmp,ret);
	}
	fclose(fp);
	if(buf->pos <= 0) {
		goto middle;
	}

	wtk_json_parser_reset(parser);
	ret = wtk_json_parser_parse(parser,buf->data,buf->pos);
	if(ret != 0 || !parser->json->main) {
		goto middle;
	}

	item = parser->json->main;
	wtk_json_obj_remove(item,domain,dlen);

	item1 = wtk_json_new_object(parser->json);
	wtk_json_obj_add_str2_s(parser->json,item1,"host",host,hlen);
	t = time_get_ms() + dc->cache_ms;
	wtk_json_obj_add_ref_number_s(parser->json,item1,"deadline",t);
	wtk_json_obj_add_item2(parser->json,item,domain,dlen,item1);

middle:
	if(!item) {
		item1 = wtk_json_new_object(parser->json);
		wtk_json_obj_add_str2_s(parser->json,item1,"host",host,hlen);
		t = time_get_ms() + dc->cache_ms;
		wtk_json_obj_add_ref_number_s(parser->json,item1,"deadline",t);

		item = wtk_json_new_object(parser->json);
		wtk_json_obj_add_item2(parser->json,item,domain,dlen,item1);
	}

	wtk_strbuf_reset(buf);
	wtk_json_item_print(item,buf);

	fp = fopen(dc->cache_fn,"wb");
	if(fp) {
		fwrite(buf->data,1,buf->pos,fp);
		fclose(fp);
	}
	wtk_lock_unlock(&glb_dns_lock);
	return 0;
}


void wtk_dns_cache_clean(wtk_dns_cache_t *dc,char *domain,int len)
{
	wtk_json_parser_t *parser = dc->parser;
	wtk_json_item_t *item;
	wtk_strbuf_t *buf = dc->buf;
	char tmp[WTK_DNS_CACHE_BUFSIZE];
	FILE *fp;
	int ret;

	wtk_lock_lock(&glb_dns_lock);
	fp = fopen(dc->cache_fn,"rb");
	if(!fp) {
		wtk_lock_unlock(&glb_dns_lock);
		return;
	}

	while(1) {
		ret = fread(tmp,1,WTK_DNS_CACHE_BUFSIZE,fp);
		if(ret <= 0) {
			break;
		}
		wtk_strbuf_push(buf,tmp,ret);
	}
	fclose(fp);
	if(buf->pos < 0) {
		wtk_lock_unlock(&glb_dns_lock);
		return;
	}

	wtk_json_parser_reset(parser);
	ret = wtk_json_parser_parse(parser,buf->data,buf->pos);
	if(ret != 0 || !parser->json->main) {
		wtk_lock_unlock(&glb_dns_lock);
		return;
	}

	item = wtk_json_obj_remove(parser->json->main,domain,len);
	if(!item) {
		wtk_lock_unlock(&glb_dns_lock);
		return;
	}

	wtk_json_item_print(parser->json->main,buf);
	fp = fopen(dc->cache_fn,"wb");
	if(fp) {
		fwrite(buf->data,1,buf->pos,fp);
		fclose(fp);
	}
	wtk_lock_unlock(&glb_dns_lock);
}
