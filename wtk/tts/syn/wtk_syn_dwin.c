#include "wtk_syn_dwin.h" 
#include "wtk/core/cfg/wtk_source.h"

int wtk_syn_dwin_load(void **p,wtk_source_t *src)
{
	wtk_syn_dwin_t *w;
	int idx;
	int ret,i,num,leng;
	float tmp;

	w=(wtk_syn_dwin_t*)(p[0]);
	idx=*((int*)p[1]);
	ret=wtk_source_read_int(src,&num,1,0);
	if(ret!=0){goto end;}
	w->coef[idx]=(wtk_syn_float_t*)wtk_heap_malloc(w->heap,sizeof(wtk_syn_float_t)*num);
	for(i=0;i<num;++i)
	{
		ret=wtk_source_read_float(src,&tmp,1,0);
		if(ret!=0){goto end;}
		w->coef[idx][i]=tmp;
	}
	leng=num/2;
	//wtk_debug("leng=%d\n",leng);
	w->coef[idx]+=leng;
	w->width[idx][0]=-leng;
	w->width[idx][1]=leng;
	if(num%2==0)
	{
		--w->width[idx][1];
	}
	ret=0;
end:
	return ret;
}

wtk_syn_dwin_t* wtk_syn_dwin_new(wtk_syn_dwin_cfg_t *cfg,wtk_source_loader_t *sl,wtk_strpool_t *pool)
{
	wtk_syn_dwin_t *w;
	wtk_heap_t *heap=pool->hash->heap;
	wtk_string_t **strs;
	int num,i;
	void *p[2];
	int ret;
	int maxl;

	w=(wtk_syn_dwin_t*)wtk_malloc(sizeof(wtk_syn_dwin_t));
	w->cfg=cfg;
	w->heap=pool->hash->heap;
	num=cfg->fn->nslot;
	w->num=num;
	w->width=(int**)wtk_heap_malloc(heap,sizeof(int*)*num);
	for(i=0;i<num;++i)
	{
		w->width[i]=(int*)wtk_heap_malloc(heap,sizeof(int)*2);
	}
	w->coef=(wtk_syn_float_t**)wtk_heap_malloc(heap,sizeof(wtk_syn_float_t*)*num);
	strs=(wtk_string_t**)(cfg->fn->slot);
	p[0]=w;p[1]=&i;
	maxl=0;
	for(i=0;i<num;++i)
	{
		ret=wtk_source_loader_load(sl,p,(wtk_source_load_handler_t)wtk_syn_dwin_load,strs[i]->data);
		//ret=wtk_source_load_file(p,(wtk_source_load_handler_t)wtk_syn_dwin_load,strs[i]->data);
		if(ret!=0){goto end;}
		if(w->width[i][1]>maxl)
		{
			maxl=w->width[i][1];
		}
	}
	w->maxl=maxl;
	//wtk_debug("max_l=%d\n",maxl);
	ret=0;
end:
	if(ret!=0)
	{
		wtk_syn_dwin_delete(w);
		w=0;
	}
	return w;
}

void wtk_syn_dwin_delete(wtk_syn_dwin_t *w)
{
	wtk_free(w);
}
