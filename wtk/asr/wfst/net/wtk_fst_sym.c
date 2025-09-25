#include "wtk_fst_sym.h"
#include "wtk/core/wtk_os.h"
#include "wtk/core/cfg/wtk_source.h"

int fst_sym_load_file(wtk_fst_sym_t *sym,wtk_source_t *s)
{
	wtk_strbuf_t *buf;
	wtk_label_t *label=sym->label;
	wtk_string_t *v;
	int ret;
	int idx;

	buf=wtk_strbuf_new(256,1);
	while(1)
	{
		ret=wtk_source_skip_sp(s,NULL);
		if(ret!=0)
		{
			ret=0;
			goto end;
		}
		ret=wtk_source_read_normal_string(s,buf);
		//ret=wtk_source_read_string(s,buf);
		if(ret!=0 || buf->pos==0)
		{
			ret=0;
			goto end;
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
		if(ret!=0)
		{
			wtk_debug("read int failed [%.*s]\n",buf->pos,buf->data);
			goto end;
		}
		if(idx>(sym->nstrs-1) || sym->strs[idx])
		{
			if(idx>(sym->nstrs-1))
			{
				wtk_debug("invalid index [%d/%d]\n",idx,sym->nstrs);
			}else
			{
				wtk_debug("dup v[%d]=[%.*s,%.*s]\n",idx,v->len,v->data,
						sym->strs[idx]->len,sym->strs[idx]->data);
			}
			ret=-1;
			goto end;
		}
		//wtk_debug("v[%d]=[%.*s]\n",idx,v->len,v->data);
		sym->strs[idx]=v;
		//sym->len[idx]=wtk_utf8_len(v->data,v->len);
		/*
		if(idx==0)
		{
			wtk_debug("v=%p,%.*s\n",v,v->len,v->data);
		}*/
	}
	ret=0;
end:
	//wtk_debug("ret=%d\n",ret);
	wtk_strbuf_delete(buf);
	return ret;
}


int fst_sym_load_file3(wtk_fst_sym_t *sym,wtk_source_t *src)
{
	wtk_string_t *v;
	int vi;
	char bi;
	int ret;
	int i,idx;
	wtk_strbuf_t *buf;
	wtk_heap_t *heap;

	//wtk_debug("load sym\n");
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
	sym->nstrs=vi;
	sym->strs=(wtk_string_t**)wtk_calloc(sym->nstrs,sizeof(wtk_string_t *));
	for(i=0;i<sym->nstrs;++i)
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

		if(idx>(sym->nstrs-1) || sym->strs[idx])
		{
			if(idx>(sym->nstrs-1))
			{
				wtk_debug("invalid index [%d/%d]\n",idx,sym->nstrs);
			}else
			{
				wtk_debug("dup v[%d]=[%.*s,%.*s]\n",idx,v->len,v->data,
						sym->strs[idx]->len,sym->strs[idx]->data);
			}
			ret=-1;
			goto end;
		}
		//wtk_debug("v[%d]=[%.*s]\n",idx,v->len,v->data);
		sym->strs[idx]=v;
	}
	ret=0;
end:
	wtk_strbuf_delete(buf);
	return ret;
}

int fst_sym_load_file4(wtk_fst_sym_t *sym,wtk_source_t *src)
{
	wtk_string_t *v;
	int vi;
	char bi;
	int ret;
	int i,idx;
	wtk_heap_t *heap;
	wtk_string_t *data;
	char *s;

	//wtk_debug("load sym\n");
	if(sym->label)
	{
		heap=sym->label->heap;//sym->hash->heap;
	}else
	{
		heap=sym->heap;
	}
	data=src->get_file(src->data);
	s=data->data;
	vi=*((int*)s);
	s+=4;
	sym->nstrs=vi;
	sym->strs=(wtk_string_t**)wtk_calloc(sym->nstrs,sizeof(wtk_string_t *));
	for(i=0;i<sym->nstrs;++i)
	{
		bi=*(s++);
		v=wtk_heap_dup_string(heap,s,bi);
		s+=bi;
		vi=*((int*)s);
		s+=4;
		idx=vi;
		if(idx>(sym->nstrs-1) || sym->strs[idx])
		{
			if(idx>(sym->nstrs-1))
			{
				wtk_debug("invalid index [%d/%d]\n",idx,sym->nstrs);
			}else
			{
				wtk_debug("dup v[%d]=[%.*s,%.*s]\n",idx,v->len,v->data,
						sym->strs[idx]->len,sym->strs[idx]->data);
			}
			ret=-1;
			goto end;
		}
		//wtk_debug("v[%d]=[%.*s:%d]\n",idx,v->len,v->data,sym->nstrs);
		sym->strs[idx]=v;
	}
	//exit(0);
	ret=0;
end:
	return ret;
}


int fst_sym_load_file2(wtk_fst_sym_t *sym,wtk_source_t *src)
{
	if(src->get_file)
	{
		return fst_sym_load_file4(sym,src);
	}else
	{
		return fst_sym_load_file3(sym,src);
	}
}

wtk_fst_sym_t* wtk_fst_sym_new(wtk_label_t *label,char *fn)
{
	wtk_source_loader_t file_sl;

	file_sl.hook=0;
	file_sl.vf=wtk_source_load_file_v;
	return wtk_fst_sym_new2(label,fn,&(file_sl));
}

wtk_fst_sym_t *wtk_fst_sym_new2(wtk_label_t *label,char *fn,wtk_source_loader_t *sl)
{
	return wtk_fst_sym_new3(label,fn,sl,0);
}

wtk_fst_sym_t *wtk_fst_sym_new3(wtk_label_t *label,char *fn,wtk_source_loader_t *sl,int use_bin)
{
	wtk_fst_sym_t *sym;
	int ret;

	sym=(wtk_fst_sym_t*)wtk_malloc(sizeof(*sym));
	//wtk_debug("fn=[%s]\n",fn);
	sym->label=label;
	if(label)
	{
		sym->heap=0;
	}else
	{
		sym->heap=wtk_heap_new(4096);
	}
	if(!use_bin)
	{
		sym->nstrs=wtk_source_loader_file_lines(sl,fn);
		//sym->nstrs=wtk_file_lines(fn);
		//wtk_debug("n=%d\n",sym->nstrs);
		//sym->len=(int*)wtk_calloc(sym->nstrs,sizeof(int));
		sym->strs=(wtk_string_t**)wtk_calloc(sym->nstrs,sizeof(wtk_string_t *));
		//ret=wtk_source_load_file(sym,(wtk_source_load_handler_t)fst_sym_load_file,fn);
		ret=wtk_source_loader_load(sl, sym,(wtk_source_load_handler_t)fst_sym_load_file, fn);
	}else
	{
		ret=wtk_source_loader_load(sl, sym,(wtk_source_load_handler_t)fst_sym_load_file2, fn);
	}
	//exit(0);
	if(ret!=0)
	{
		wtk_fst_sym_delete(sym);
		sym=0;
	}
	return sym;
}

int wtk_fst_sym_bytes(wtk_fst_sym_t *sym)
{
	int bytes=0;

	if(sym->strs)
	{
		bytes+=sym->nstrs+sizeof(wtk_string_t*);
	}
	if(sym->heap)
	{
		bytes+=wtk_heap_bytes(sym->heap);
	}
	return bytes;
}

void wtk_fst_sym_delete(wtk_fst_sym_t *sym)
{
	/*
	if(sym->len)
	{
		wtk_free(sym->len);
	}*/
	if(sym->strs)
	{
		wtk_free(sym->strs);
	}
	if(sym->heap)
	{
		wtk_heap_delete(sym->heap);
	}
	wtk_free(sym);
}

int wtk_fst_sym_get_index(wtk_fst_sym_t *sym,wtk_string_t *v)
{
	int index=-1;
	int i;

	for(i=0;i<sym->nstrs;++i)
	{
		if(wtk_string_cmp(sym->strs[i],v->data,v->len)==0)
		{
			index=i;
			break;
		}
	}
	//wtk_debug("[%.*s]=%d\n",v->len,v->data,index);
	return index;
}
