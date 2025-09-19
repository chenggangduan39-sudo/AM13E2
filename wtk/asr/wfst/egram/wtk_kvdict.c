#include "wtk_kvdict.h"

wtk_kvdict_t* wtk_kvdict_new(wtk_label_t *label,char *fn,
		int hash_hint,int phn_hash_hint,int wrd_hash_hint)
{
	wtk_kvdict_t *kv;

	kv=(wtk_kvdict_t*)wtk_malloc(sizeof(wtk_kvdict_t));
	kv->fkv=wtk_fkv_new(fn,hash_hint);
	kv->dict=wtk_dict_new2(label,1,phn_hash_hint,wrd_hash_hint);
	kv->phn_hash_hint=phn_hash_hint;
	kv->wrd_hash_hint=wrd_hash_hint;
	kv->label=label;
	return kv;
}


wtk_kvdict_t* wtk_kvdict_new2(wtk_label_t *label,
		int hash_hint,int phn_hash_hint,int wrd_hash_hint,
		FILE *f,int of,int len)
{
	wtk_kvdict_t *kv;

	kv=(wtk_kvdict_t*)wtk_malloc(sizeof(wtk_kvdict_t));
	kv->fkv=wtk_fkv_new2(hash_hint,f,of,len,0);
	kv->dict=wtk_dict_new2(label,1,phn_hash_hint,wrd_hash_hint);
	kv->phn_hash_hint=phn_hash_hint;
	kv->wrd_hash_hint=wrd_hash_hint;
	kv->label=label;
	return kv;
}

void wtk_kvdict_delete(wtk_kvdict_t *kv)
{
	wtk_fkv_delete(kv->fkv);
	wtk_dict_delete(kv->dict);
	wtk_free(kv);
}

void wtk_kvdict_reset(wtk_kvdict_t *kv)
{
	wtk_dict_delete(kv->dict);
	kv->dict=wtk_dict_new2(kv->label,1,kv->phn_hash_hint,kv->wrd_hash_hint);
}

/**
 * item_len(B)
 * v_len(B) v0 v1 v2
 */
wtk_dict_word_t* wtk_kvdict_add_word2(wtk_dict_t *dict,char *w,int bytes, char *s, int len)
{
	wtk_dict_word_t *dw=NULL;
	unsigned char bn,bn1;
	int i,j;
	wtk_dict_pron_t *pron;
	unsigned char *bx;
	//print_data(w,bytes);
	//print_data(s,len);
	//exit(0);
	dw=(wtk_dict_word_t*)wtk_heap_zalloc(dict->heap,sizeof(*dw));
	dw->name=wtk_heap_dup_string(dict->heap,w,bytes);
	wtk_str_hash_add(dict->word_hash,dw->name->data,dw->name->len,dw);
	++dict->nword;

	bn=*(unsigned char*)(s++);
	//wtk_debug("bn=%d\n",bn);
	dw->npron=bn;
	dw->pron_list=NULL;
	for(i=0;i<bn;++i)
	{
		bn1=*(unsigned char*)(s++);
		pron=(wtk_dict_pron_t*)wtk_heap_zalloc(dict->heap,sizeof(*pron));
		pron->word=dw;
		pron->nPhones=bn1;
		pron->prob=0;
		pron->pnum=i;
		pron->next=dw->pron_list;
		pron->outsym=dw->name;
		dw->pron_list=pron;
		bx=(unsigned char*)wtk_heap_malloc(dict->heap,sizeof(unsigned char)*bn1);
		for(j=0;j<bn1;++j)
		{
			bx[j]=*(unsigned char*)(s++);
			//wtk_debug("bx[%d]\n", bx[j]);
		}
		pron->pPhones=(wtk_dict_phone_t**)bx;
		if (bn1 > dw->maxnphn) dw->maxnphn = bn1;
	}
//end:
	return dw;
}

wtk_dict_word_t* wtk_kvdict_add_word(wtk_kvdict_t *kv,char *w,int bytes)
{
	wtk_string_t *str;
	wtk_dict_word_t *dw=NULL;

	//print_data(w,bytes);
	str=wtk_fkv_get_str(kv->fkv,w,bytes);
	//wtk_debug("[%.*s]=%p\n",bytes,w,str);
	if(str)
	{
		dw = wtk_kvdict_add_word2(kv->dict, w, bytes, str->data, str->len);
	}

	return dw;
}

wtk_dict_word_t* wtk_kvdict_get_word(wtk_kvdict_t *kv,char *w,int bytes)
{
	wtk_dict_word_t*  dw;
	wtk_string_t str;

	wtk_string_set(&str,w,bytes);
	dw=wtk_dict_get_word(kv->dict,&str,0);
	if(dw){goto end;}
	dw=wtk_kvdict_add_word(kv,w,bytes);
end:
	return dw;
}

void wtk_kvdict_wrd_print(wtk_dict_word_t* dw)
{
	wtk_dict_pron_t *pron;

	wtk_debug("=======kvword:%.*s[npron:%d]\n", dw->name->len, dw->name->data, dw->npron);
	pron=dw->pron_list;
	while(pron)
	{
		wtk_kvdict_pron_print(pron);
		pron=pron->next;
	}
}

void wtk_kvdict_pron_print(wtk_dict_pron_t* pron)
{
	int i;
	unsigned char phn;

	printf("%.*s", pron->outsym->len,pron->outsym->data);
	for (i=0; i < pron->nPhones; i++){
		phn = ((unsigned char*) pron->pPhones)[i];
		printf(" %d", phn);
	}
	printf("\n");
}
