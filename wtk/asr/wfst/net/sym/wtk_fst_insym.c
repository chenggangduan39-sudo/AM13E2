#include "wtk_fst_insym.h"
#include "wtk/core/wtk_os.h"
#include "wtk/core/cfg/wtk_source.h"

#ifdef USE_TYPE
void wtk_fst_insym_item_update_type(wtk_fst_insym_item_t *item)
{
	int pos;

	pos=wtk_str_str(item->str->data,item->str->len,"sil",3);
	if(pos==0)
	{
		if(item->str->len==3)
		{
			item->type=WTK_FST_INSYM_SIL;
		}else
		{
			item->type=WTK_FST_INSYM_SIL_PRE;
		}
	}else if(pos>0)
	{
		item->type=WTK_FST_INSYM_SIL_SUF;
	}else
	{
		item->type=WTK_FST_INSYM_NORM;
	}
	//wtk_debug("[%.*s]=%d\n",item->str->len,item->str->data,item->type);
}
#endif

int fst_insym_load_file(wtk_fst_insym_t *sym,wtk_source_t *s)
{
	wtk_strbuf_t *buf;
	wtk_label_t *label=sym->label;
	wtk_heap_t *heap;
	wtk_string_t *v;
	int ret;
	int idx;
	wtk_fst_insym_item_t *item;

	buf=wtk_strbuf_new(256,1);
	if(label)
	{
		heap=label->heap;
	}else
	{
		heap=sym->heap;
	}
	while(1)
	{
		ret=wtk_source_skip_sp(s,NULL);
		if(ret!=0)
		{
			ret=0;goto end;
		}
		ret=wtk_source_read_string(s,buf);
		//wtk_debug("ret=%d,[%.*s]\n",ret,buf->pos,buf->data);
		if(ret!=0)
		{
			ret=0;goto end;
		}
		if(label)
		{
			v=wtk_label_find2(label,buf->data,buf->pos,1);
		}else
		{
			//v=wtk_string_dup_data(buf->data,buf->pos);
			v=wtk_heap_dup_string(sym->heap,buf->data,buf->pos);
		}
		ret=wtk_source_read_int(s,&idx,1,0);
		if(ret!=0){goto end;}
		//wtk_debug("[%.*s]=%d\n",buf->pos,buf->data,idx);
		if(idx>(sym->nid-1) || sym->ids[idx])
		{
			if(idx>(sym->nid-1))
			{
				wtk_debug("invalid index [%d/%d]\n",idx,sym->nid);
			}else
			{
				wtk_debug("dup v[%d]=[%.*s,%.*s]\n",idx,v->len,v->data,
						sym->ids[idx]->str->len,sym->ids[idx]->str->data);
			}
			ret=-1;
			goto end;
		}
		//wtk_debug("v[%p/%d]=[%.*s]\n",sym,idx,v->len,v->data);
		item=(wtk_fst_insym_item_t*)wtk_heap_malloc(heap,sizeof(wtk_fst_insym_item_t));
		item->id=idx;
		sym->ids[idx]=item;
		item->str=v;
		//wtk_fst_insym_item_update_type(item);
		if(sym->hash)
		{
			wtk_str_hash_add(sym->hash,v->data,v->len,item);
		}
		if(wtk_string_cmp_s(v,"sil")==0)
		{
			sym->sil_id=idx;
		}
	}
	ret=0;
end:
	//wtk_debug("ret=%d\n",ret);
	wtk_strbuf_delete(buf);
	return ret;
}


int fst_insym_load_file3(wtk_fst_insym_t *sym,wtk_source_t *src)
{
	wtk_string_t *v;
	int vi;
	char bi;
	int ret;
	int i,idx;
	wtk_strbuf_t *buf;
	wtk_fst_insym_item_t *item;
	wtk_heap_t *heap;

	if(sym->label)
	{
		heap=sym->label->heap;//sym->hash->heap;
	}else
	{
		heap=sym->heap;
	}
	buf=wtk_strbuf_new(256,1);
	ret=wtk_source_fill(src,(char*)&vi,4);
	if(ret!=0){goto end;}
	//print_data((char*)&(vi),4);
	sym->nid=vi;
	sym->ids=(wtk_fst_insym_item_t**)wtk_calloc(sym->nid,sizeof(wtk_fst_insym_item_t *));
	//wtk_debug("hash=%p\n",sym->hash);
	for(i=0;i<sym->nid;++i)
	{
		ret=wtk_source_fill(src,(char*)&bi,1);
		if(ret!=0)
		{
			wtk_debug("read k len failed\n");
			goto end;
		}
		ret=wtk_source_fill(src,buf->data,(int)bi);
		if(ret!=0)
		{
			wtk_debug("read k v failed(%d)\n",bi);
			goto end;
		}
		v=wtk_heap_dup_string(heap,buf->data,bi);
		ret=wtk_source_fill(src,(char*)&vi,4);
		if(ret!=0)
		{
			wtk_debug("read id failed\n");
			goto end;
		}
		idx=vi;

		if(idx>(sym->nid-1) || sym->ids[idx])
		{
			if(idx>(sym->nid-1))
			{
				wtk_debug("invalid index [%d/%d]\n",idx,sym->nid);
			}else
			{
				wtk_debug("dup v[%d]=[%.*s,%.*s]\n",idx,v->len,v->data,
						sym->ids[idx]->str->len,sym->ids[idx]->str->data);
			}
			ret=-1;
			goto end;
		}
		//wtk_debug("v[%p/%d]=[%.*s]\n",sym,idx,v->len,v->data);
		item=(wtk_fst_insym_item_t*)wtk_heap_malloc(heap,sizeof(wtk_fst_insym_item_t));
		item->id=idx;
		sym->ids[idx]=item;
		item->str=v;
		if(wtk_string_cmp_s(v,"sil")==0)
		{
			sym->sil_id=idx;
		}
		if(sym->hash)
		{
			wtk_str_hash_add(sym->hash,v->data,v->len,item);
		}
	}
	ret=0;
end:
	wtk_strbuf_delete(buf);
	return ret;
}

int fst_insym_load_file4(wtk_fst_insym_t *sym,wtk_source_t *src)
{
	wtk_string_t *v;
	int vi;
	char bi;
	int ret;
	int i,idx;
	wtk_fst_insym_item_t *item;
	wtk_heap_t *heap;
	wtk_string_t *data;
	char *s;

	data=src->get_file(src->data);
	if(sym->label)
	{
		heap=sym->label->heap;//sym->hash->heap;
	}else
	{
		heap=sym->heap;
	}
	s=data->data;
	vi=*((int*)s);
	s+=4;
	sym->nid=vi;
	sym->ids=(wtk_fst_insym_item_t**)wtk_calloc(sym->nid,sizeof(wtk_fst_insym_item_t *));
	for(i=0;i<sym->nid;++i)
	{
		bi=*(s++);
		v=wtk_heap_dup_string(heap,s,bi);
		s+=bi;
		vi=*((int*)s);
		s+=4;
		idx=vi;
		if(idx>(sym->nid-1) || sym->ids[idx])
		{
			if(idx>(sym->nid-1))
			{
				wtk_debug("invalid index [%d/%d]\n",idx,sym->nid);
			}else
			{
				wtk_debug("dup v[%d]=[%.*s,%.*s]\n",idx,v->len,v->data,
						sym->ids[idx]->str->len,sym->ids[idx]->str->data);
			}
			ret=-1;
			goto end;
		}
		//wtk_debug("v[%d]=[%.*s]\n",idx,v->len,v->data);
		item=(wtk_fst_insym_item_t*)wtk_heap_malloc(heap,sizeof(wtk_fst_insym_item_t));
		item->id=idx;
		sym->ids[idx]=item;
		item->str=v;
		if(wtk_string_cmp_s(v,"sil")==0)
		{
			sym->sil_id=idx;
		}
		if(sym->hash)
		{
			wtk_str_hash_add(sym->hash,v->data,v->len,item);
		}
	}
	ret=0;
end:
	return ret;
}

int fst_insym_load_file2(wtk_fst_insym_t *sym,wtk_source_t *src)
{
	if(src->get_file)
	{
		return fst_insym_load_file4(sym,src);
	}else
	{
		return fst_insym_load_file3(sym,src);
	}
}

wtk_fst_insym_t *wtk_fst_insym_new(wtk_label_t *label,char *fn,int use_hash)
{
	wtk_source_loader_t file_sl;

	file_sl.hook=0;
	file_sl.vf=wtk_source_load_file_v;
	return wtk_fst_insym_new2(label,fn,use_hash,&(file_sl));
}

wtk_fst_insym_t *wtk_fst_insym_new2(wtk_label_t *label,char *fn,int use_hash,wtk_source_loader_t *sl)
{
	return wtk_fst_insym_new3(label,fn,use_hash,sl,0);
}

wtk_fst_insym_t *wtk_fst_insym_new3(wtk_label_t *label,char *fn,int use_hash,
		wtk_source_loader_t *sl,int use_bin)
{
	wtk_fst_insym_t *sym;
	int ret;

	sym=(wtk_fst_insym_t*)wtk_malloc(sizeof(*sym));
	//wtk_debug("fn=[%s]\n",fn);
	sym->label=label;
	if(label)
	{
		sym->heap=0;
	}else
	{
		sym->heap=wtk_heap_new(4096);
	}
	sym->sil_id=-1;
	if(!use_bin)
	{
		sym->nid=wtk_source_loader_file_lines(sl,fn);
		sym->ids=(wtk_fst_insym_item_t**)wtk_calloc(sym->nid,sizeof(wtk_fst_insym_item_t *));
	}else
	{
		sym->nid=0;
		sym->ids=NULL;
	}
	//wtk_debug("nid=%d\n",sym->nid);
	//sym->nid=wtk_file_lines(fn);
	//wtk_debug("n=%d\n",sym->nstrs);
	if(use_hash && sym->nid>0)
	{
		sym->hash=wtk_str_hash_new(sym->nid);
	}else
	{
		sym->hash=NULL;
	}
	//ret=wtk_source_load_file(sym,(wtk_source_load_handler_t)fst_insym_load_file,fn);
	//wtk_debug("use_bin=%d\n",use_bin);
	if(use_bin)
	{
		ret=wtk_source_loader_load(sl, sym,(wtk_source_load_handler_t)fst_insym_load_file2, fn);
	}else
	{
		ret=wtk_source_loader_load(sl, sym,(wtk_source_load_handler_t)fst_insym_load_file, fn);
	}
	//exit(0);
	if(ret!=0)
	{
		wtk_fst_insym_delete(sym);
		sym=0;
	}
	return sym;
}

int wtk_fst_insym_bytes(wtk_fst_insym_t *sym)
{
	int bytes=0;

	if(sym->ids)
	{
		bytes+=sym->nid*sizeof(wtk_fst_insym_item_t*);
	}
	if(sym->heap)
	{
		bytes+=wtk_heap_bytes(sym->heap);
	}
	return bytes;
}

void wtk_fst_insym_delete(wtk_fst_insym_t *sym)
{
	if(sym->ids)
	{
		wtk_free(sym->ids);
	}
	if(sym->heap)
	{
		wtk_heap_delete(sym->heap);
	}
	if(sym->hash)
	{
		wtk_str_hash_delete(sym->hash);
	}
	wtk_free(sym);
}

int wtk_fst_insym_get_index(wtk_fst_insym_t *sym,wtk_string_t *v)
{
	wtk_fst_insym_item_t *item;
	int index=-1;
	int i;

	if(sym->hash)
	{
		item=wtk_str_hash_find(sym->hash,v->data,v->len);
		if(item)
		{
			index=item->id;
		}
	}else
	{
		for(i=0;i<sym->nid;++i)
		{
			if(wtk_string_cmp(sym->ids[i]->str,v->data,v->len)==0)
			{
				index=i;
				break;
			}
		}
	}
	return index;
}

int wtk_fst_insym_get_index2(wtk_fst_insym_t *sym,char *data,int bytes)
{
	wtk_string_t v;

	wtk_string_set(&(v),data,bytes);
	return wtk_fst_insym_get_index(sym,&v);
}
