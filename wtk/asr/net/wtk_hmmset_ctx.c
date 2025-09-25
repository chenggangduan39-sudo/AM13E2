#include "wtk_hmmset_ctx.h"
wtk_hmmref_t* wtk_hmmset_ctx_new_hmmref(wtk_hmmset_ctx_t *hc);

wtk_hmmset_ctx_t* wtk_hmmset_ctx_new(wtk_hmmset_t *hl,int ctx_ind)
{
	wtk_hmmset_ctx_t* ctx;
	int ret;

	ctx=(wtk_hmmset_ctx_t*)wtk_malloc(sizeof(*ctx));
	ret=wtk_hmmset_ctx_init(ctx,hl,ctx_ind);
	if(ret!=0)
	{
		wtk_hmmset_ctx_delete(ctx);
		ctx=0;
	}
	return ctx;
}

int wtk_hmmset_ctx_delete(wtk_hmmset_ctx_t *ctx)
{
	wtk_hmmset_ctx_clean(ctx);
	wtk_free(ctx);
	return 0;
}

int wtk_hmmset_ctx_init(wtk_hmmset_ctx_t *hc,wtk_hmmset_t *hl,int ctx_ind)
{
	int ret=0,n;

	hc->hl=hl;
	hc->nc=hc->xc=hc->ncf=0;
	hc->s_left=hc->s_right=0;
	if(ctx_ind)
	{
		hc->cfs_hash=hc->cis_hash=hc->cxs_hash=0;
	}else
	{
		n=253;
		hc->cfs_hash=wtk_str_hash_new(n);
		hc->cis_hash=wtk_str_hash_new(n);
		hc->cxs_hash=wtk_str_hash_new(n);
		wtk_hoard_init(&(hc->cis_hoard),offsetof(wtk_hmmref_t,ctx_n),10,
				(wtk_new_handler_t)wtk_hmmset_ctx_new_hmmref,(wtk_delete_handler_t)free,hc);
		ret=wtk_hmmset_ctx_define_ctx(hc);
		/*
		wtk_debug("cfs: %d\n",wtk_str_hash_elems(hc->cfs_hash));
		wtk_debug("cis: %d\n",wtk_str_hash_elems(hc->cis_hash));
		wtk_debug("cxs: %d\n",wtk_str_hash_elems(hc->cxs_hash));
		*/
	}
	return ret;
}

int wtk_hmmset_ctx_clean(wtk_hmmset_ctx_t *hc)
{
	if(hc->cfs_hash)
	{
		wtk_str_hash_delete(hc->cfs_hash);
		wtk_str_hash_delete(hc->cis_hash);
		wtk_str_hash_delete(hc->cxs_hash);
		wtk_hoard_clean(&(hc->cis_hoard));
	}
	return 0;
}

wtk_hmmref_t* wtk_hmmset_ctx_new_hmmref(wtk_hmmset_ctx_t *hc)
{
	//wtk_debug("new ref.\n");
	return (wtk_hmmref_t*)malloc(sizeof(wtk_hmmref_t));
}

wtk_hmmref_t* wtk_hmmset_ctx_pop_hmmref(wtk_hmmset_ctx_t *hc)
{
	wtk_hmmref_t *ref;

	ref=(wtk_hmmref_t*)wtk_hoard_pop(&(hc->cis_hoard));
	//wtk_debug("ref=%p\n",ref);
	ref->count=0;
	return ref;
}

int wtk_hmmset_ctx_push_hmmref(wtk_hmmset_ctx_t *hc,wtk_hmmref_t* ref)
{
	return wtk_hoard_push(&(hc->cis_hoard),ref);
}

int wtk_hmmset_ctx_add_label(wtk_hmmset_ctx_t *hc,wtk_str_hash_t* hash,wtk_string_t *name)
{
	wtk_string_t *n;

	n=(wtk_string_t*)wtk_str_hash_find(hash,name->data,name->len);
	if(!n)
	{
		wtk_str_hash_add(hash,name->data,name->len,name);
	}
	return 0;
}

int wtk_hmmset_ctx_add_hci_ctx(wtk_hmmset_ctx_t *hc,wtk_string_t *name)
{
	/* jfyuan add 2013-11-26 */
	wtk_string_t* n=(wtk_string_t*)wtk_str_hash_find(hc->cxs_hash,name->data,name->len);
	if(!n)
	{
		hc->nc++;
	}

	//wtk_debug("%*.*s\n",name->len,name->len,name->data);
	return wtk_hmmset_ctx_add_label(hc,hc->cxs_hash,name);
}

wtk_string_t* wtk_hmmset_ctx_get_hci_ctx(wtk_hmmset_ctx_t *hc,wtk_string_t *name)
{
	wtk_string_t buf;

	if(!hc->cxs_hash){return 0;}
	wtk_hmm_strip_name(name,&buf);
	//wtk_debug("%*.*s\n",name->len,name->len,name->data);
	return (wtk_string_t*)wtk_str_hash_find(hc->cxs_hash,buf.data,buf.len);
}



int wtk_hmmset_ctx_is_hic_ind(wtk_hmmset_ctx_t *hc,wtk_string_t *name)
{
	return wtk_str_hash_find(hc->cis_hash,name->data,name->len)?1:0;
}

int wtk_hmmset_ctx_add_ref_label(wtk_hmmset_ctx_t *hc,wtk_str_hash_t* hash,wtk_string_t *name)
{
	wtk_hmmref_t *ref;

	ref=(wtk_hmmref_t*)wtk_str_hash_find(hash,name->data,name->len);
	if(!ref)
	{
		ref=wtk_hmmset_ctx_pop_hmmref(hc);
		wtk_str_hash_add_node(hash,name->data,name->len,ref,&(ref->hash_n));
	}
	++ref->count;
	return 0;
}

/**
 *	"a-b+c" => "b"
 */
void wtk_hmm_strip_name(wtk_string_t *src,wtk_string_t *dst)
{
	char *p;

	p=wtk_str_chr(src->data,src->len,'-');
	if(p)
	{
		dst->data=p+1;
		dst->len=src->data-dst->data+src->len;
	}else
	{
		dst->data=src->data;
		dst->len=src->len;
	}
	p=wtk_str_chr(dst->data,dst->len,'+');
	if(p)
	{
		dst->len=p-dst->data;
	}
}

void wtk_hmmset_ctx_prune(wtk_hmmset_ctx_t *hc)
{
	wtk_str_hash_t *hash=hc->cis_hash;
	wtk_queue_t *q;
	wtk_queue_node_t *n,*p;
	hash_str_node_t *hn;
	wtk_hmmref_t *ref;
	int i;

	hc->nci=0;
	for(i=0;i<hash->nslot;++i)
	{
		q=hash->slot[i];
		if(!q){continue;}
		for(n=q->pop;n;n=p)
		{
			p=n->next;
			hn=wtk_queue_node_data(n,hash_str_node_t,n);
			ref=(wtk_hmmref_t*)hn->value;
			//wtk_debug("%*.*s:\t%d\n",ref->hash_n.key.len,ref->hash_n.key.len,ref->hash_n.key.data,ref->count);
			if(ref->count>1)
			{
				wtk_queue_remove(q,n);
				wtk_hmmset_ctx_push_hmmref(hc,ref);
			}else
			{
				++hc->nci;
			}
		}
	}
	//wtk_debug("nci: %d\n",hc->nci);
}


int wtk_hmmset_ctx_define_ctx(wtk_hmmset_ctx_t *hc)
{
	wtk_str_hash_t *hash=hc->hl->hmm_hash;
	wtk_label_t *label=hc->hl->label;
	wtk_queue_t *q;
	wtk_queue_node_t *n;
	hash_str_node_t *hn;
	//wtk_macro_t *m;
	wtk_string_t name,left,right;
	wtk_name_t* id;
	int ret,i;

	ret=-1;
	//find all context dependent hmm, like 'a','c' in 'a-b+c',and record it in cxs_hash.
	for(i=0;i<hash->nslot;++i)
	{
		//wtk_debug("%d:%d\n",i,hash->nslot);
		q=hash->slot[i];
		if(!q){continue;}
		for(n=q->pop;n;n=n->next)
		{
			hn=wtk_queue_node_data(n,hash_str_node_t,n);
			//m=(wtk_macro_t*)hn->value;
			//printf("%c:\t%*.*s\n",m->type,m->name->len,m->name->len,m->name->data);
		    wtk_label_find(label,hn->key.data,hn->key.len,1);
			if(1)
			{
				wtk_hmm_strip_name(&(hn->key),&name);
				//wtk_debug("%.*s: %.*s\n",m->name->len,m->name->data,name.len,name.data);
				id=wtk_label_find(label,name.data,name.len,0);
				if(!id)
				{
					wtk_debug("%*.*s not found.\n",name.len,name.len,name.data);
					goto end;
				}
				//update context referenced hmm refcount like 'b' in 'a-b+c'
				wtk_hmmset_ctx_add_ref_label(hc,hc->cis_hash,id->name);
				//print_data(name.data,name.len);
				if(name.data!=hn->key.data)
				{
					//found '-'
					left.data=hn->key.data;
					left.len=name.data-hn->key.data-1;
					//print_data(left.data,left.len);
					id=wtk_label_find(label,left.data,left.len,1);
					if(!id)
					{
						wtk_debug("%*.*s not found.\n",left.len,left.len,left.data);
						goto end;
					}
					wtk_hmmset_ctx_add_hci_ctx(hc,id->name);
					//wtk_hmmset_ctx_add_label(hc,hc->cxs_hash,id->name);
					hc->s_left=1;
				}
				if(wtk_string_end(&name) != wtk_string_end(&(hn->key)))
				{
					//found '+'
					right.data=name.data+name.len+1;
					right.len=wtk_string_end(&(hn->key))-right.data;
					//print_data(right.data,right.len);
					//getchar();
					id=wtk_label_find(label,right.data,right.len,1);
					if(!id)
					{
						wtk_debug("%*.*s not found.\n",right.len,right.len,right.data);
						goto end;
					}
					wtk_hmmset_ctx_add_hci_ctx(hc,id->name);
					//wtk_hmmset_ctx_add_label(hc,hc->cxs_hash,id->name);
					hc->s_right=1;
				}
			}
		}
	}
	wtk_hmmset_ctx_prune(hc);
	for(i=0;i<hash->nslot;++i)
	{
		//wtk_debug("%d:%d\n",i,hash->nslot);
		q=hash->slot[i];
		if(!q){continue;}
		for(n=q->pop;n;n=n->next)
		{
			hn=wtk_queue_node_data(n,hash_str_node_t,n);
			//m=(wtk_macro_t*)hn->value;
			//printf("%c:\t%*.*s\n",m->type,m->name->len,m->name->len,m->name->data);
			if(1)
			{
				if(!wtk_hmmset_ctx_get_hci_ctx(hc,&(hn->key)))
				{
					id=wtk_label_find(label,hn->key.data,hn->key.len,0);
					if(!id)
					{
						wtk_debug("%*.*s not found.\n", hn->key.len,hn->key.len,hn->key.data);
					}
					//wtk_debug("free=[%.*s]\n",id->name->len,id->name->data);
					wtk_hmmset_ctx_add_label(hc,hc->cfs_hash,id->name);
					//printf("%c:\t%*.*s\n",m->type,m->name->len,m->name->len,m->name->data);
				}
			}
		}
	}
	ret=0;
end:
	return ret;
}
