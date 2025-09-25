#include <stdlib.h>
#include "wtk_dict.h"
#include "wtk_hmmset.h"


int wtk_dict_add_word(wtk_dict_t *d,wtk_array_t *a,float prob)
{
	wtk_dict_word_t *w;
	wtk_string_t** s;

	s=(wtk_string_t**)a->slot;
	//print_data(s[0]->data,s[0]->len);
	w=wtk_dict_get_word(d,s[0],1);
	//wtk_debug("[%.*s]=%d/%p\n",s[0]->len,s[0]->data,a->nslot,w);
	wtk_dict_add_pron(d,w,s[1],s+2,a->nslot-2,prob);
	return 0;
}

int wtk_dict_read_word(wtk_dict_t*d,wtk_source_t *s,wtk_strbuf_t *b,wtk_array_t *pa,float *prob)
{
	wtk_string_t* nw;
	wtk_label_t *l=d->label;
	wtk_string_t **st;
	int ret,nl,nphones;
	float p=-1,v;
	char *ptr;

	ret=wtk_source_read_string(s,b);
	if(ret!=0){goto end;}
	//wtk_debug("word: %.*s\n",b->pos,b->data);
	nw=wtk_label_find(l,b->data,b->pos,1)->name;
	//print_data(nw->data,nw->len);
	st=(wtk_string_t**)wtk_array_push_n(pa,2);
	st[0]=nw;st[1]=0;
	nphones=0;
	while(1)
	{
		ret=wtk_source_skip_sp(s,&nl);
		if(ret!=0){goto end;}
		if(nl){break;}
		ret=wtk_source_read_string(s,b);
		if(ret!=0){goto end;}
		//print_data(b->data,b->pos);
		if(b->data[0]=='[' && b->data[b->pos-1]==']')
		{
			if(st[1]){ret=-1;goto end;}
			st[1]=wtk_label_find(l,&(b->data[1]),b->pos-2,1)->name;
		}else
		{
			if(nphones==0 && p<0)
			{
				//char c=0;
				wtk_strbuf_push_c(b,0);
				b->pos-=1;
				//wtk_debug("%s\n",b->data);
				ptr=NULL;
				v=strtof(b->data,&ptr);
				//wtk_debug("[%s]=%f pos=%d %p/%p\n",b->data,v,b->pos,ptr,b->data);
				//exit(0);
			}else
			{
				v=0.0;ptr=b->data;
			}
			if(ptr!=b->data)
			{
				if(v<=0.0 || v>1.0 ||*ptr!=0){ret=-1;goto end;}
				p=v;
			}else
			{
				st=(wtk_string_t**)wtk_array_push(pa);
				//print_data(b->data,b->pos);
				st[0]=wtk_label_find(l,b->data,b->pos,1)->name;
				//print_data(st[0]->data,st[0]->len);
			}
		}
	}
	*prob=p;
end:
	return ret;
}


int wtk_dict_load(wtk_dict_t *d,wtk_source_t *s)
{
	wtk_strbuf_t *b;
	wtk_heap_t *h;
	wtk_array_t *a;
	int ret,eof;
	float prob=0;

	ret=-1;
	h=wtk_heap_new(4096);
	a=wtk_array_new_h(h,256,sizeof(wtk_string_t*));
	b=wtk_strbuf_new(64,1);eof=0;
	while(1)
	{
		wtk_array_reset(a);
		ret=wtk_dict_read_word(d,s,b,a,&prob);
		if(ret!=0)
		{
			eof=wtk_source_get(s)==EOF;
			if(!eof){break;}
			ret=0;
			if(a->nslot==0){break;}
		}
		ret=wtk_dict_add_word(d,a,prob);
		if(ret!=0){goto end;}
		if(eof){break;}
	}
end:
	//wtk_debug("%d\n",ret);
	wtk_strbuf_delete(b);
	wtk_heap_delete(h);
	return ret;
}

