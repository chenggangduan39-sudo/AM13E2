#include "wtk_lmexpand_dict.h"
#include "wtk/core/cfg/wtk_source.h"

int wtk_lmexpand_dict_load(wtk_lmexpand_dict_t *d,wtk_source_t *src)
{
	wtk_strbuf_t *buf;
	int ret;
	int ids[100];
	int n;
	int nl;
	int id;
	wtk_heap_t *heap=d->hash->heap;
	wtk_lmexpand_dict_item_t *item;

	ret=0;
	buf=wtk_strbuf_new(256,1);
	while(1)
	{
		ret=wtk_source_read_string(src,buf);
		if(ret!=0){ret=0;goto end;}
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
		n=0;
		while(1)
		{
			ret=wtk_source_skip_sp(src,&nl);
			if(nl){break;}
			ret=wtk_source_read_int(src,&id,1,0);
			if(ret!=0){goto end;}
			ids[n++]=id;
		}
		item=(wtk_lmexpand_dict_item_t*)wtk_heap_malloc(heap,sizeof(wtk_lmexpand_dict_item_t));
		item->name=wtk_heap_dup_string(heap,buf->data,buf->pos);
		item->nid=n;
		item->ids=(int*)wtk_heap_malloc(heap,sizeof(int)*n);
		memcpy(item->ids,ids,n*sizeof(int));
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
		wtk_str_hash_add(d->hash,item->name->data,item->name->len,item);
	}
end:
	//exit(0);
	wtk_strbuf_delete(buf);
	return ret;
}


wtk_lmexpand_dict_t* wtk_lmexpand_dict_new(char *fn)
{
	wtk_lmexpand_dict_t *d;

	d=(wtk_lmexpand_dict_t*)wtk_malloc(sizeof(wtk_lmexpand_dict_t));
	d->hash=wtk_str_hash_new(25007);
	wtk_source_load_file(d,(wtk_source_load_handler_t)wtk_lmexpand_dict_load,fn);
	return d;
}

void wtk_lmexpand_dict_delete(wtk_lmexpand_dict_t *d)
{
	wtk_str_hash_delete(d->hash);
	wtk_free(d);
}
