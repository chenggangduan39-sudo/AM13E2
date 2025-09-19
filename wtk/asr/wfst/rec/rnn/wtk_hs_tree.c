#include "wtk_hs_tree.h" 
#include "wtk/core/cfg/wtk_source.h"
#define wtk_hs_tree_wrd_new_s(h,n) wtk_hs_tree_wrd_new(h,n,sizeof(n)-1)

wtk_hs_tree_wrd_t* wtk_hs_tree_wrd_new(wtk_heap_t *heap,char *name,int bytes)
{
	wtk_hs_tree_wrd_t *w;

	w=(wtk_hs_tree_wrd_t*)wtk_heap_malloc(heap,sizeof(wtk_hs_tree_wrd_t));
	w->name=wtk_heap_dup_string(heap,name,bytes);
	w->code=NULL;
	w->codelen=0;
	w->point=0;
	w->wrd_index=0;
	return w;
}


int wtk_hs_tree_load(wtk_hs_tree_t *tree,wtk_source_t *src)
{
	wtk_strbuf_t *buf;
	wtk_heap_t *heap;
	int ret;
	wtk_hs_tree_wrd_t *wrd;
	int i;
	int wrd_index=0;

	tree->voc_hash=wtk_str_hash_new(43449);
	heap=tree->voc_hash->heap;
	buf=wtk_strbuf_new(256,1);
	while(1)
	{
		ret=wtk_source_read_string(src,buf);
		if(ret!=0){ret=0;goto end;}
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
		wrd=(wtk_hs_tree_wrd_t*)wtk_heap_malloc(heap,sizeof(wtk_hs_tree_wrd_t));
		wrd->name=wtk_heap_dup_string(heap,buf->data,buf->pos);
		ret=wtk_source_read_string(src,buf);
		if(ret!=0){goto end;}
		wrd->wrd_index=wrd_index++;
		wrd->codelen=buf->pos;
		wrd->code=(unsigned char*)wtk_heap_malloc(heap,wrd->codelen*sizeof(unsigned char));
		for(i=0;i<wrd->codelen;++i)
		{
			wrd->code[i]=buf->data[i]-'0';
		}
		wrd->point=(unsigned short*)wtk_heap_malloc(heap,wrd->codelen*sizeof(unsigned short));
		//ret=wtk_source_read_int(src,(int*)(wrd->point),wrd->codelen,0);
		ret=wtk_source_read_ushort(src,wrd->point,wrd->codelen,0);
		if(ret!=0){goto end;}
		wtk_str_hash_add(tree->voc_hash,wrd->name->data,wrd->name->len,wrd);
	}
	ret=0;
end:
	tree->eos=wtk_hs_tree_get_word_s(tree,"</s>");
	tree->oov=wtk_hs_tree_wrd_new_s(heap,"oov");
	wtk_strbuf_delete(buf);
	return ret;
}


int wtk_hs_tree_load_bin(wtk_hs_tree_t *tree,wtk_source_t *src)
{
	wtk_strbuf_t *buf;
	wtk_heap_t *heap;
	int ret;
	wtk_hs_tree_wrd_t *wrd;
	int wrd_index=0;
	unsigned char bi;
	int cnt;
	float f;
	float t=1.0/8;

	src->swap=0;
	tree->voc_hash=wtk_str_hash_new(43449);
	heap=tree->voc_hash->heap;
	buf=wtk_strbuf_new(256,1);
	while(1)
	{
		ret=wtk_source_read_wtkstr(src,buf);
		if(ret!=0)
		{
			//wtk_debug("want end\n");
			ret=0;goto end;
		}
//		wtk_debug("[%.*s]\n",buf->pos,buf->data);
//		if(buf->pos>20)
//		{
//			exit(0);
//		}
		wrd=(wtk_hs_tree_wrd_t*)wtk_heap_malloc(heap,sizeof(wtk_hs_tree_wrd_t));
		wrd->wrd_index=wrd_index++;
		wrd->name=wtk_heap_dup_string(heap,buf->data,buf->pos);

		ret=wtk_source_fill(src,(char*)&bi,1);
		if(ret!=0){goto end;}
		wrd->codelen=bi;
		f=bi*t;
		cnt=(int)f;
		if(f>cnt)
		{
			++cnt;
		}
		//cnt=(int)(((bi*1.0)/8)+0.999999);
		//wtk_debug("bi=%d/%d cnt=%d\n",bi,wrd->codelen,cnt);
		//wtk_debug("cnt=%d %f\n",cnt,((bi*1.0)/8));
		wrd->code=(unsigned char*)wtk_heap_malloc(heap,cnt*sizeof(unsigned char));
		ret=wtk_source_fill(src,(char*)wrd->code,cnt);
		if(ret!=0){goto end;}
		//wtk_debug("cnt=%d\n",cnt);
//		{
//			int i;
//
//			for(i=0;i<cnt;++i)
//			{
//				wtk_debug("[%d]\n",wrd->code[i]);
//			}
//		}
		wrd->point=(unsigned short*)wtk_heap_malloc(heap,wrd->codelen*sizeof(unsigned short));
		ret=wtk_source_read_ushort(src,wrd->point,wrd->codelen,1);
		if(ret!=0){goto end;}
		//wtk_debug("[%.*s]=%d\n",wrd->name->len,wrd->name->data,wrd->codelen);
		wtk_str_hash_add(tree->voc_hash,wrd->name->data,wrd->name->len,wrd);
//		{
//			int i;
//
//			for(i=0;i<wrd->codelen;++i)
//			{
//				wtk_debug("v[%d]=%d\n",i,wrd->point[i]);
//			}
//		}
//		{
//			static int ki=0;
//
//			++ki;
//			if(ki==2)
//			{
//				exit(0);
//			}
//		}
	}
	ret=0;
end:
	//wtk_debug("wrd_indx=%d ret=%d\n",wrd_index,ret);
	tree->eos=wtk_hs_tree_get_word_s(tree,"</s>");
	tree->oov=wtk_hs_tree_wrd_new_s(heap,"oov");
	wtk_strbuf_delete(buf);
	//exit(0);
	return ret;
}

wtk_hs_tree_t* wtk_hs_tree_new(char *fn,int use_bin,wtk_source_loader_t *sl)
{
	wtk_hs_tree_t *tree;

	tree=(wtk_hs_tree_t*)wtk_malloc(sizeof(wtk_hs_tree_t));
	tree->voc_hash=NULL;
	tree->eos=NULL;
	tree->oov=NULL;
	if(sl)
	{
		if(use_bin)
		{
			wtk_source_loader_load(sl,tree,(wtk_source_load_handler_t)wtk_hs_tree_load_bin,fn);
		}else
		{
			wtk_source_loader_load(sl,tree,(wtk_source_load_handler_t)wtk_hs_tree_load,fn);
		}
	}else
	{
		if(use_bin)
		{
			wtk_source_load_file(tree,(wtk_source_load_handler_t)wtk_hs_tree_load_bin,fn);
		}else
		{
			wtk_source_load_file(tree,(wtk_source_load_handler_t)wtk_hs_tree_load,fn);
		}
	}
	return tree;
}

void wtk_hs_tree_delete(wtk_hs_tree_t *tree)
{
	wtk_str_hash_delete(tree->voc_hash);
	wtk_free(tree);
}

wtk_hs_tree_wrd_t* wtk_hs_tree_get_word(wtk_hs_tree_t *r,char *name,int name_bytes)
{
	return  (wtk_hs_tree_wrd_t*)wtk_str_hash_find(r->voc_hash,name,name_bytes);
}
