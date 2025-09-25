#include "wtk_dict.h"
#include "wtk_hmmset.h"
wtk_dict_pron_t* wtk_dict_add_pron(wtk_dict_t *d,wtk_dict_word_t *w,wtk_string_t *outsym,wtk_string_t **phones,int nphones,float prob);

static wtk_string_t pre_dict_words[]=
{
	wtk_string("!SUBLATID"),
};


static wtk_string_t pre_dict_pron_words[][2]=
{
	{wtk_string("!SENT_START"),wtk_string("sil")},
	{wtk_string("!SENT_END"),wtk_string("sil")},
};


static wtk_string_t null_word=wtk_string("!NULL");

typedef struct
{
	wtk_string_t name;
	wtk_phone_type_t type;
}wtk_phn_var_t;

static wtk_phn_var_t pre_dict_phns[]=
{
	{wtk_string("sil"),WTK_PHN_CI},
	{wtk_string("sp"),WTK_PHN_CF}
};

wtk_dict_t* wtk_dict_new(wtk_label_t *l,int use_db)
{
	wtk_dict_t *d;

	d=(wtk_dict_t*)malloc(sizeof(*d));
	wtk_dict_init(d,l,use_db);
	return d;
}

wtk_dict_t* wtk_dict_new2(wtk_label_t *label,int use_db,
		int phn_hash_hint,int wrd_hash_hint)
{
	wtk_dict_t *d;

	d=(wtk_dict_t*)malloc(sizeof(*d));
	wtk_dict_init2(d,label,use_db,phn_hash_hint,wrd_hash_hint);
	return d;
}

int wtk_dict_delete(wtk_dict_t *d)
{
	wtk_dict_clean(d);
	free(d);
	return 0;
}


int wtk_dict_setup(wtk_dict_t *d)
{
	int i,count;
	wtk_dict_phone_t *phn;
	wtk_phn_var_t *pvar;
	wtk_dict_word_t *w;

	d->null_word=w=wtk_dict_get_word(d,&null_word,1);
	d->null_pron=wtk_dict_add_pron(d,w,0,0,0,1.0);
	count=sizeof(pre_dict_words)/sizeof(wtk_string_t);
	for(i=0;i<count;++i)
	{
		wtk_dict_get_word(d,&(pre_dict_words[i]),1);
	}
	count=sizeof(pre_dict_phns)/sizeof(wtk_phn_var_t);
	for(i=0;i<count;++i)
	{
		pvar=&(pre_dict_phns[i]);
		phn=wtk_dict_get_phone(d,&(pvar->name),1);
		phn->type=pvar->type;
	}
	if(d->use_db)
	{
		wtk_dict_add_sent_flat(d);
	}
	return 0;
}

int wtk_dict_add_sent_flat(wtk_dict_t *d)
{
	wtk_string_t *s;
	wtk_dict_word_t *w;
	int i,count;

	count=2;
	for(i=0;i<count;++i)
	{
		w=wtk_dict_get_word(d,&(pre_dict_pron_words[i][0]),1);
		s=&(pre_dict_pron_words[i][1]);
		wtk_dict_add_pron(d,w,0,&(s),1,-1);
	}
	return 0;
}

int wtk_dict_init(wtk_dict_t *d,wtk_label_t *label,int use_db)
{
	int hash=257;

	return wtk_dict_init2(d,label,use_db,hash,hash);
}

int wtk_dict_init2(wtk_dict_t *d,wtk_label_t *label,int use_db,
		int phn_hash_hint,int wrd_hash_hint)
{
	memset(d,0,sizeof(*d));
	d->label=label;
	d->heap=wtk_heap_new(4096);
	d->phone_hash=wtk_str_hash_new(phn_hash_hint);
	d->word_hash=wtk_str_hash_new(wrd_hash_hint);
	d->use_db=use_db;
	wtk_dict_setup(d);
	return 0;
}

int wtk_dict_clean(wtk_dict_t *d)
{
	wtk_str_hash_delete(d->phone_hash);
	wtk_str_hash_delete(d->word_hash);
	wtk_heap_delete(d->heap);
	return 0;
}

int wtk_dict_reset(wtk_dict_t *d)
{
	wtk_heap_reset(d->heap);
	wtk_str_hash_reset(d->phone_hash);
	wtk_str_hash_reset(d->word_hash);
	wtk_dict_setup(d);
	return 0;
}

int wtk_dict_is_closed(wtk_dict_t *d,wtk_hmmset_t *hl)
{
#ifdef USE_CHECK
	wtk_str_hash_t *hash=d->phone_hash;
	wtk_queue_t *q;
	wtk_queue_node_t *n;
	hash_str_node_t *hn;
	wtk_dict_phone_t *phn;
	wtk_hmm_t *hmm;
	int i;
	int closed;

	for(i=0;i<hash->nslot;++i)
	{
		//wtk_debug("%d:%d\n",i,hash->nslot);
		q=hash->slot[i];
		if(!q){continue;}
		for(n=q->pop;n;n=n->next)
		{
			hn=wtk_queue_node_data(n,hash_str_node_t,n);
			phn=(wtk_dict_phone_t*)hn->value;
			print_data(phn->name->data,phn->name->len);
			hmm=wtk_hmmset_find_hmm(hl,phn->name->data,phn->name->len);
			if(!hmm){closed=0;goto end;}
		}
	}
	closed=1;
end:
	//wtk_debug("%d\n",closed);
	return closed;
#else
	return 1;
#endif
}

wtk_dict_phone_t* wtk_dict_get_phone(wtk_dict_t* d,wtk_string_t *n,int insert)
{
	wtk_dict_phone_t *p;

	//print_data(n->data,n->len);
	p=(wtk_dict_phone_t*)wtk_str_hash_find(d->phone_hash,n->data,n->len);
	if(p){goto end;}
	if(insert==0){goto end;}
	p=(wtk_dict_phone_t*)wtk_heap_zalloc(d->heap,sizeof(*p));
	p->name=n;
	p->type=WTK_PHN_CN;
	//print_data(p->name->data,p->name->len);
	wtk_str_hash_add(d->phone_hash,p->name->data,p->name->len,p);
	++d->nphone;
end:
	return p;
}

wtk_dict_phone_t* wtk_dict_find_phone(wtk_dict_t *d,char* n,int nl)
{
	return (wtk_dict_phone_t*)wtk_str_hash_find(d->phone_hash,n,nl);
}

wtk_dict_word_t* wtk_dict_find_word(wtk_dict_t *d,char* n,int nl)
{
	return (wtk_dict_word_t*)wtk_str_hash_find(d->word_hash,n,nl);
}

wtk_dict_word_t* wtk_dict_get_dummy_wrd(wtk_dict_t *d,char *w,int bytes)
{
	static wtk_string_t ps[]={
			wtk_string("s"),
			wtk_string("ax"),
			wtk_string("t"),
			wtk_string("n"),
			wtk_string("ih"),
	};
	wtk_string_t name;
	wtk_string_t *v;
	wtk_dict_word_t *dw;
	int i;

	//print_data(w,bytes);
	wtk_string_set(&name,w,bytes);
	dw=wtk_dict_get_word2(d,&name,1);
	for(i=0;i<5;++i)
	{
		v=&(ps[i]);
		wtk_dict_add_pron(d,dw,0,&v,1,-1);
	}
	return dw;
}

void wtk_dict_reset_aux(wtk_dict_t *d)
{
	wtk_str_hash_it_t it;
	hash_str_node_t *node;
	wtk_dict_word_t *dw;

	it=wtk_str_hash_iterator(d->word_hash);
	while(1)
	{
		node=wtk_str_hash_it_next(&(it));
		if(!node){break;}
		dw=(wtk_dict_word_t*)node->value;
		dw->aux=NULL;
	}
}

wtk_dict_word_t* wtk_dict_get_word(wtk_dict_t *d,wtk_string_t *n,int insert)
{
	wtk_dict_word_t *w;

	//print_data(n->data,n->len);
	w=(wtk_dict_word_t*)wtk_str_hash_find(d->word_hash,n->data,n->len);
	if(w){goto end;}
	if(insert==0){goto end;}
	w=(wtk_dict_word_t*)wtk_heap_zalloc(d->heap,sizeof(*w));
	w->name=n;
	wtk_str_hash_add(d->word_hash,w->name->data,w->name->len,w);
	++d->nword;
end:
	return w;
}


wtk_dict_word_t* wtk_dict_get_word2(wtk_dict_t *d,wtk_string_t *n,int insert)
{
	wtk_dict_word_t *w;

	//print_data(n->data,n->len);
	w=(wtk_dict_word_t*)wtk_str_hash_find(d->word_hash,n->data,n->len);
	if(w){goto end;}
	if(insert==0){goto end;}
	w=(wtk_dict_word_t*)wtk_heap_zalloc(d->heap,sizeof(*w));
	w->name=wtk_heap_dup_string(d->heap,n->data,n->len);
	wtk_str_hash_add(d->word_hash,w->name->data,w->name->len,w);
	++d->nword;
end:
	return w;
}

wtk_dict_word_t* wtk_dict_get_word_handler(wtk_dict_t *d,char *w,int w_bytes)
{
	wtk_string_t *n;

	n=wtk_heap_dup_string(d->heap,w,w_bytes);
	return  wtk_dict_get_word(d,n,0);
}

wtk_dict_pron_t* wtk_dict_add_pron(wtk_dict_t *d,wtk_dict_word_t *w,wtk_string_t *outsym,wtk_string_t **phones,int nphones,float prob)
{
	wtk_heap_t *h=d->heap;
	wtk_dict_pron_t *pron,**tmp;
	int i;

	//wtk_debug("%d\n",d->n)
	pron=(wtk_dict_pron_t*)wtk_heap_zalloc(h,sizeof(*pron));
	pron->word=w;
	pron->nPhones=nphones;
	if (nphones > w->maxnphn) w->maxnphn = nphones;
	if(pron->nPhones>0)
	{
		pron->pPhones=(wtk_dict_phone_t**)wtk_heap_malloc(h,pron->nPhones*sizeof(wtk_dict_phone_t*));
	}
	for(i=0;i<pron->nPhones;++i)
	{
		//print_data(s[i+2]->data,s[i+2]->len);
		pron->pPhones[i]=wtk_dict_get_phone(d,phones[i],1);
	}
	pron->outsym=outsym?outsym:w->name;
	if(prob>=MINPRONPROB && prob<=1.0)
	{
		pron->prob=log(prob);
	}else if(prob>=0.0 && prob<MINPRONPROB)
	{
		pron->prob=LZERO;
	}else
	{
		pron->prob=0.0f;
	}
	tmp=&(w->pron_list);
	i=0;
	//wtk_debug("tmp: %p\n",tmp);
	while(*tmp)
	{
		//wtk_debug("tmp: %p,%p\n",tmp,w->pron_list);
		++i;
		tmp=&((*tmp)->next);
	}
	pron->pnum=i+1;
	*tmp=pron;
	++w->npron;
	++d->npron;
	return pron;
}

/**
* should consider diff dict type from fst and gmm
* add by dmd 
*/
//for fst
void wtk_dict_add_merge_wordid2(wtk_dict_t *d,wtk_dict_word_t *dst,wtk_dict_pron_t **pre_pron,int npron,
		wtk_dict_word_t **wrds,int wrd_idx,int nwrd)
{
	wtk_heap_t *h=d->heap;
	wtk_dict_pron_t *pron,*np;
	//unsigned char *phns;
	//wtk_dict_phone_t** phns, *tailphn;
	int nphn;
	int i;
	int ki;

	if(nwrd==wrd_idx)
	{
		//wtk_debug("npron=%d\n",npron);
		for(nphn=0,i=0;i<npron;++i)
		{
			pron=pre_pron[i];
			nphn+=pron->nPhones;
			//wtk_debug("[%.*s] n=%d\n",pron->word->name->len,pron->word->name->data,pron->nPhones);
		}
		np=(wtk_dict_pron_t*)wtk_heap_zalloc(h,sizeof(wtk_dict_pron_t));
		np->word=dst;
		np->nPhones=nphn;
//		wtk_debug("nphn=%d\n",nphn);
		//phns=(unsigned char*)wtk_heap_malloc(h,sizeof(unsigned char)*nphn);
		//phns=(wtk_dict_phone_t**)wtk_heap_malloc(h,sizeof(wtk_dict_phone_t*)*nphn);
		np->pPhones=(wtk_dict_phone_t**)wtk_heap_malloc(h,sizeof(unsigned char)*nphn);
		for(ki=0,nphn=0,i=0;i<npron;++i)
		{
			pron=pre_pron[i];
			//wtk_debug("[%.*s] n=%d\n",pron->word->name->len,pron->word->name->data,pron->nPhones);
			//memcpy(phns+ki,(unsigned char*)pron->pPhones,pron->nPhones*sizeof(unsigned char));
			//memcpy(phns+ki,(wtk_dict_phone_t**)pron->pPhones,pron->nPhones*sizeof(wtk_dict_phone_t*));
			memcpy((unsigned char*)np->pPhones+ki,(unsigned char*)pron->pPhones,pron->nPhones*sizeof(unsigned char));
			ki+=pron->nPhones;
		}
		np->outsym=dst->name;
		np->prob=0; //1.0;
		np->pnum=dst->npron;
		++dst->npron;
		np->next=dst->pron_list;
		dst->pron_list=np;
	}else
	{
		for(pron=wrds[wrd_idx]->pron_list;pron;pron=pron->next)
		{
			pre_pron[npron]=pron;
			//add filter sp/sil, avoid sp/sil is in the middle in multi-word.  by dmd
//			if (pron->nPhones > 0){
//				tailphn=pron->pPhones[pron->nPhones-1];
//				if (wtk_string_equal_s(tailphn->name,"sil") || wtk_string_equal_s(tailphn->name,"sp"))continue;
//			}
			wtk_dict_add_merge_wordid2(d,dst,pre_pron,npron+1,wrds,wrd_idx+1,nwrd);
		}
	}
}
//for dnn
wtk_dict_word_t* wtk_dict_add_merge_wordid(wtk_dict_t *d,wtk_string_t *sym,wtk_dict_word_t **wrds,int nwrd)
{
	wtk_dict_word_t *dst;
	wtk_dict_pron_t **prons;

	prons=wtk_calloc(nwrd,sizeof(wtk_dict_pron_t*));
	//dst=wtk_dict_get_word(d,sym,1);
	dst=wtk_dict_get_word2(d,sym,1);    //avoid maybe string reference error in some error operation.
	wtk_dict_add_merge_wordid2(d,dst,prons,0,wrds,0,nwrd);
	wtk_free(prons);
	return dst;
}
//for gmm
void wtk_dict_add_merge_word2str(wtk_dict_t *d,wtk_dict_word_t *dst,wtk_dict_pron_t **pre_pron,int npron,
		wtk_dict_word_t **wrds,int wrd_idx,int nwrd)
{
	wtk_heap_t *h=d->heap;
	wtk_dict_pron_t *pron,*np;
	//unsigned char *phns;
	wtk_dict_phone_t *tailphn;
	int nphn;
	int i;
	int ki;

	if(nwrd==wrd_idx)
	{
		//wtk_debug("npron=%d\n",npron);
		for(nphn=0,i=0;i<npron;++i)
		{
			pron=pre_pron[i];
			nphn+=pron->nPhones;
			//wtk_debug("[%.*s] n=%d\n",pron->word->name->len,pron->word->name->data,pron->nPhones);
		}
		np=(wtk_dict_pron_t*)wtk_heap_zalloc(h,sizeof(wtk_dict_pron_t));
		np->word=dst;
		np->nPhones=nphn;
		//phns=(unsigned char*)wtk_heap_malloc(h,sizeof(unsigned char)*nphn);
		np->pPhones=(wtk_dict_phone_t**)wtk_heap_malloc(h,sizeof(wtk_dict_phone_t*)*nphn);
		//wtk_debug("pPhones=%p word=%.*s\n", np->pPhones, np->word->name->len, np->word->name->data);
		for(ki=0,nphn=0,i=0;i<npron;++i)
		{
			pron=pre_pron[i];
			//wtk_debug("[%.*s] n=%d\n",pron->word->name->len,pron->word->name->data,pron->nPhones);
			//memcpy(phns+ki,(unsigned char*)pron->pPhones,pron->nPhones*sizeof(unsigned char));
			memcpy(np->pPhones+ki,(wtk_dict_phone_t**)pron->pPhones,pron->nPhones*sizeof(wtk_dict_phone_t*));
			ki+=pron->nPhones;
		}
		np->outsym=dst->name;
		np->prob=1.0;
		++dst->npron;
		np->next=dst->pron_list;
		dst->pron_list=np;
	}else
	{
		for(pron=wrds[wrd_idx]->pron_list;pron;pron=pron->next)
		{
			pre_pron[npron]=pron;
			//add filter sp/sil, avoid sp/sil is in the middle in multi-word.  by dmd
			if (pron->nPhones > 0){
				tailphn=pron->pPhones[pron->nPhones-1];
				if (wtk_string_equal_s(tailphn->name,"sil") || wtk_string_equal_s(tailphn->name,"sp"))continue;
			}
			wtk_dict_add_merge_word2str(d,dst,pre_pron,npron+1,wrds,wrd_idx+1,nwrd);
		}
	}
}

wtk_dict_word_t* wtk_dict_add_merge_wordstr(wtk_dict_t *d,wtk_string_t *sym,wtk_dict_word_t **wrds,int nwrd)
{
	wtk_dict_word_t *dst;
	wtk_dict_pron_t **prons;

	prons=wtk_calloc(nwrd,sizeof(wtk_dict_pron_t*));
	dst=wtk_dict_get_word2(d,sym,1);    //avoid maybe string reference error in some error operation.
	wtk_dict_add_merge_word2str(d,dst,prons,0,wrds,0,nwrd);

	wtk_free(prons);
	return dst;
}

void wtk_dict_add_merge_word2(wtk_dict_t *d,wtk_dict_word_t *dst,wtk_dict_pron_t **pre_pron,int npron,
		wtk_dict_word_t **wrds,int wrd_idx,int nwrd)
{
	wtk_heap_t *h=d->heap;
	wtk_dict_pron_t *pron,*np;
	//unsigned char *phns;
	int nphn;
	int i;
	int ki;

	if(nwrd==wrd_idx)
	{
		//wtk_debug("npron=%d\n",npron);
		for(nphn=0,i=0;i<npron;++i)
		{
			pron=pre_pron[i];
			nphn+=pron->nPhones;
			//wtk_debug("[%.*s] n=%d\n",pron->word->name->len,pron->word->name->data,pron->nPhones);
		}
		np=(wtk_dict_pron_t*)wtk_heap_zalloc(h,sizeof(wtk_dict_pron_t));
		np->word=dst;
		np->nPhones=nphn;
		//phns=(unsigned char*)wtk_heap_malloc(h,sizeof(unsigned char)*nphn);
		np->pPhones=(wtk_dict_phone_t**)wtk_heap_malloc(h,sizeof(wtk_dict_phone_t*)*nphn);
		for(ki=0,nphn=0,i=0;i<npron;++i)
		{
			pron=pre_pron[i];
			//wtk_debug("[%.*s] n=%d\n",pron->word->name->len,pron->word->name->data,pron->nPhones);
			memcpy(np->pPhones+ki,(unsigned char*)pron->pPhones,pron->nPhones*sizeof(unsigned char));
			ki+=pron->nPhones;
		}
		np->outsym=dst->name;
		np->prob=1.0;
		++dst->npron;
		np->next=dst->pron_list;
		dst->pron_list=np;
	}else
	{
		for(pron=wrds[wrd_idx]->pron_list;pron;pron=pron->next)
		{
			pre_pron[npron]=pron;
			wtk_dict_add_merge_word2(d,dst,pre_pron,npron+1,wrds,wrd_idx+1,nwrd);
		}
	}
}

wtk_dict_word_t* wtk_dict_add_merge_word(wtk_dict_t *d,wtk_string_t *sym,wtk_dict_word_t **wrds,int nwrd)
{
	wtk_dict_word_t *dst;
	wtk_dict_pron_t **prons;

	prons=wtk_calloc(nwrd,sizeof(wtk_dict_pron_t*));
	dst=wtk_dict_get_word2(d,sym,1);    //avoid maybe string reference error in some error operation.
	wtk_dict_add_merge_word2(d,dst,prons,0,wrds,0,nwrd);
	wtk_free(prons);
	return dst;
}

void wtk_dict_print(wtk_dict_t *d)
{
	printf("nword:\t%d\n",d->nword);
	printf("npron:\t%d\n",d->npron);
	printf("nphone:\t%d\n",d->nphone);
}

int wtk_dict_word_check(wtk_dict_word_t *wrd)
{
	int cnt=0;
	wtk_dict_pron_t *pron;

	for(pron=wrd->pron_list;pron;pron=pron->next)
	{
		++cnt;
	}
	if(cnt!=wrd->npron)
	{
		wtk_dict_word_print(wrd, 0);
		wtk_debug("npron=%d/%d wrong\n",cnt,wrd->npron);
		exit(0);
		return -1;
	}else
	{
		return 0;
	}
}

void wtk_dict_pron_print(wtk_dict_pron_t *pron)
{
	wtk_dict_pron_print2(pron, 0);
}

void wtk_dict_pron_print2(wtk_dict_pron_t *pron, int use_kv)
{
	int i;

	printf("pron: %.*s\n",pron->outsym->len,pron->outsym->data);
	for(i=0;i<pron->nPhones;++i)
	{
		if (use_kv)
			printf("\tp[%d]=%d\n",i,((unsigned char*)pron->pPhones)[i]);
		else
			printf("\tp[%d]=%.*s\n",i,pron->pPhones[i]->name->len,pron->pPhones[i]->name->data);
	}
}

int wtk_dict_word_npron(wtk_dict_word_t *dw)
{
	wtk_dict_pron_t *pron;
	int n=0;

	pron=dw->pron_list;
	while(pron)
	{
		++n;
		pron=pron->next;
	}
	return n;
}

void wtk_dict_word_print(wtk_dict_word_t *dw, int use_kv)
{
	wtk_dict_pron_t *pron;

	pron=dw->pron_list;
	wtk_debug("========= wrd=(%.*s:%d) =========\n",dw->name->len,
			dw->name->data,dw->npron);
	while(pron)
	{
		wtk_debug("---------------\n");
		wtk_dict_pron_print2(pron, use_kv);
		pron=pron->next;
	}
}
