/*
 * wtk_latset_expand.c
 *
 *  Created on: 2013-11-26
 *      Author: jfyuan
 */
#include "wtk_latset.h"
#include "wtk/asr/model/wtk_dict.h"

char *frc_sil=NULL;
unsigned int remDupPron = 1;   /* Remove duplicate pronunciations */
static wtk_string_t SIL = wtk_string("sil");
static wtk_string_t UNDEF = wtk_string("<undef>");

typedef struct pinstinfo {
	wtk_dict_pron_t *pron;
	int silId;
	int n;
	int t;
	wtk_dict_phone_t **phones;
 }PInstInfo;

wtk_hmm_t* wtk_latset_get_hci_model_t(wtk_latset_t *ls,wtk_proninst_t *inst,wtk_string_t *ln,wtk_string_t* phn,wtk_string_t *rn);
wtk_string_t* wtk_latset_find_lctx(wtk_latset_t *ls,wtk_proninst_t *inst,int pos,wtk_string_t *bk);
wtk_string_t* wtk_latset_find_rctx(wtk_latset_t *ls,wtk_proninst_t *inst,int pos,wtk_string_t *bk);
wtk_netnode_t* wtk_latset_find_word_node(wtk_latset_t *ls,wtk_dict_pron_t *pron, wtk_proninst_t *inst,wtk_netnode_type_t type);
wtk_netnode_t *wtk_latset_new_netnode(wtk_latset_t *ls,wtk_hmm_t *hmm,int nlinks);
void wtk_latset_add_chain(wtk_latset_t *ls,wtk_lat_t *lat);
int wtk_netlink_is_wd0(wtk_netlink_t *link);
int wtk_hmmset_ctx_add_ref_label(wtk_hmmset_ctx_t *hc,wtk_str_hash_t* hash,wtk_string_t *name);
int wtk_hmmset_ctx_add_hci_ctx(wtk_hmmset_ctx_t *hc,wtk_string_t *name);
void wtk_hmmset_ctx_prune(wtk_hmmset_ctx_t *hc);
int wtk_hmmset_ctx_add_label(wtk_hmmset_ctx_t *hc,wtk_str_hash_t* hash,wtk_string_t *name);

/*
 * new hmm for cross word
 */
int wtk_hmmset_ctx_define_ctx2(wtk_hmmset_ctx_t *hc)
{
	wtk_str_hash_t *hash=hc->hl->hmm_hash;
	wtk_label_t *label=hc->hl->label;
	wtk_queue_t *q;
	wtk_queue_node_t *n;
	hash_str_node_t *hn;
	//wtk_hmm_t *m;
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
			//m=(wtk_hmm_t*)hn->value;
			//printf("%c:\t%*.*s\n",m->type,m->name->len,m->name->len,m->name->data);
            //wtk_debug("%.*s: %.*s\n",m->name->len,m->name->data,hn->key.len,hn->key.data);
		    wtk_label_find(label,hn->key.data,hn->key.len,1);
			if(1)
			{
				wtk_hmm_strip_name(&(hn->key),&name);
				//wtk_debug("%.*s: %.*s\n",m->name->len,m->name->data,name.len,name.data);
				id=wtk_label_find(label,name.data,name.len,1);
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
			//m=(wtk_hmm_t*)hn->value;
			//printf("%c:\t%*.*s\n",m->type,m->name->len,m->name->len,m->name->data);
			if(1)
			{
				if(!wtk_hmmset_ctx_get_hci_ctx(hc,&(hn->key)))
				{
					id=wtk_label_find(label,hn->key.data,hn->key.len,0);
					if(!id)
					{
						wtk_debug("%*.*s not found.\n",hn->key.len,hn->key.len,hn->key.data);
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

wtk_hmmref_t* wtk_hmmset_ctx_new_hmmref(wtk_hmmset_ctx_t *hc);

int wtk_hmmset_ctx_init2(wtk_hmmset_ctx_t *hc,wtk_hmmset_t *hl,int ctx_ind)
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
		ret=wtk_hmmset_ctx_define_ctx2(hc);
		/*
		wtk_debug("cfs: %d\n",wtk_str_hash_elems(hc->cfs_hash));
		wtk_debug("cis: %d\n",wtk_str_hash_elems(hc->cis_hash));
		wtk_debug("cxs: %d\n",wtk_str_hash_elems(hc->cxs_hash));
		*/
	}
	return ret;
}

wtk_hmmset_ctx_t* wtk_hmmset_ctx_new2(wtk_hmmset_t *hl,int ctx_ind)
{
	wtk_hmmset_ctx_t* ctx;
	int ret;

	ctx=(wtk_hmmset_ctx_t*)wtk_malloc(sizeof(*ctx));
	ret=wtk_hmmset_ctx_init2(ctx,hl,ctx_ind);
	if(ret!=0)
	{
		wtk_hmmset_ctx_delete(ctx);
		ctx=0;
	}
	return ctx;
}
/************************************************************************************************************************************/

/*
 * key - value (1-1)
 */
static int wtk_latset_str_hash_add(wtk_str_hash_t *h,char* key,int key_bytes,void *value)
{
	hash_str_node_t *node;
	int ret;

	if((node = wtk_str_hash_find_node(h, key, key_bytes, NULL) ))
	{
		node->value = value;
		ret = 0;
	}
	else
	{
		ret = wtk_str_hash_add(h, key, key_bytes, value);
	}

	return ret;
}

/* Return context defined by given labid (after triphone context stripping) */
static int _wtk_latset_get_hci_context(wtk_latset_t *ls, wtk_string_t *n)
{
	wtk_hmmset_ctx_t *hc=(ls->hc);
	wtk_string_t *phn=0;
	int i, c;

	if(hc->nc == 0) { return 0; }
	phn = wtk_hmmset_ctx_get_hci_ctx(hc, n);
	if(phn)
	{
		for(i=0; i < ls->hc->xc; i++)
		{
			if(0 == wtk_string_cmp2(phn, ls->cxs[i]))
			{
				c = i;
				return c;
			}
		}
	}

	if(ls->cfg->sp_word_boundary && hc->cfs_hash)
	{
		if(wtk_str_hash_find(hc->cfs_hash,n->data,n->len))
		{
			if(ls->hc->xc > 0) { return -1; }
		}
		else
		{
			return 0;
		}
	}

	return 0;
}

/*
 * Search through pron for left context for phone in position pos
 * If beginning of word is reached return lc
 */
static int _wtk_latset_find_lcontext(wtk_latset_t *ls, wtk_proninst_t *p, int pos, int lc)
{
	int i, c;

	for(i=pos-1,c=-1; i >= 0; i--)
	{
		if((c = _wtk_latset_get_hci_context(ls, p->phones[i]->name)) >= 0)
			break;
	}

	if(c < 0) { c = lc; }

	return c;
}

/*
 * Search through pron for right context for phone in position pos
 * If end of word is reached return rc
 */
static int _wtk_latset_find_rcontext(wtk_latset_t *ls, wtk_proninst_t *p, int pos, int rc)
{
	int i, c;

	for(i=pos+1,c=-1; i < p->nphones; i++)
	{
		if((c=_wtk_latset_get_hci_context(ls,p->phones[i]->name)) >= 0)
			break;
	}

	if(c < 0) { c = rc; }

	return c;
}

wtk_string_t* wtk_latset_find_ctx(wtk_latset_t *ls, wtk_string_t *n)
{
	wtk_hmmset_ctx_t *hc=(ls->hc);
	wtk_string_t *phn=0;

	if(hc->nc == 0) return NULL;
	//wtk_debug("[%.*s],bk=%p\n",inst->phones[i]->name->len,inst->phones[i]->name->data,bk);
	phn = wtk_hmmset_ctx_get_hci_ctx(hc, n);
	if(phn){ goto end; }
	if(ls->cfg->sp_word_boundary && hc->cfs_hash)
	{
		if(wtk_str_hash_find(hc->cfs_hash,n->data,n->len))
		{
			phn=0;
		}
	}

end:

	return phn;
}

/* Check to see if CI model exists for labid */
static int _wtk_latset_is_hci_context_ind(wtk_latset_t *ls, wtk_string_t *phn)
{
	wtk_hmmset_ctx_t *hc = (ls->hc);

	if(hc->nc == 0) return 1;
	if(wtk_str_hash_find(hc->cis_hash, phn->data, phn->len))
		return 1;
	else
		return 0;
}

/* Determine if phone in position pos is independent of right context */
static int wtk_latset_is_rcontext_ind(wtk_latset_t *ls, wtk_proninst_t *inst, int pos)
{
	wtk_string_t *phn, *lc;
	wtk_hmm_t *hmm, *cmp;
	int i;
	wtk_str_hash_t *hash = ls->hc->cxs_hash;
	wtk_queue_node_t *qn;
	hash_str_node_t *hash_n;

//	for(phn=NULL, i=0; i < inst->nphones; i++ )
//	{
//		printf("%.*s ", inst->phones[i]->name->len, inst->phones[i]->name->data);
//	}
//	printf("\n");

	for(phn=NULL, i=pos-1; i >= 0; i-- )
	{
		if( (phn = wtk_latset_find_ctx(ls, inst->phones[i]->name)) )
			break;
	}

	if(phn != NULL)
	{
		lc = wtk_latset_find_lctx(ls, inst, i, 0);
		if(lc == NULL)
		{
			return _wtk_latset_is_hci_context_ind(ls, phn);
		}
		else
		{
			hmm = NULL;

			for(i=0;i<hash->nslot;++i)
			{
				if(!hash->slot[i]){continue;}
				for(qn=hash->slot[i]->pop;qn;qn=qn->next)
				{
					hash_n=data_offset(qn,hash_str_node_t,n);
					cmp = wtk_latset_get_hci_model_t(ls , inst, lc, phn, &(hash_n->key));
					if(hmm == NULL)
						hmm = cmp;
					else if(hmm != cmp)
						return 0;
					//wtk_debug("index=%d: [%.*s]=[%s]\n",i,hash_n->key.len,hash_n->key.data,(char*)hash_n->value);
				}
			}

			return 1;
		}
	}
	else
	{
		for(i=pos-1; i >= 0; i--)
		{
			if(!_wtk_latset_is_hci_context_ind(ls, inst->phones[i]->name))
				break;
		}
		if(i < 0) return 1;
	}

	return 0;
}

/* Allocate and initialise a pron inst */
static wtk_proninst_t *_wtk_latset_new_proninst(wtk_latset_t *ls, wtk_dict_pron_t *pron, int np, wtk_dict_phone_t **phones)
{
	wtk_proninst_t *inst = NULL;
	wtk_hmmset_ctx_t *hc = (ls->hc);
	int n;

	inst = (wtk_proninst_t*)wtk_heap_zalloc(ls->heap,sizeof(*inst));
	inst->pron = pron;
	inst->nphones = np;
	inst->phones = phones;
	inst->clen = 0;
	inst->nstart = inst->nend = 0;
	inst->starts = inst->ends = NULL;
	inst->chain = NULL;
	inst->tee = 0;
	if(hc->xc > 0)
	{
		inst->lc = wtk_str_hash_new(hc->xc);
		inst->rc = wtk_str_hash_new(hc->xc);
	}
	else
	{
		inst->lc = inst->rc = NULL;
	}

	if(np == 0)
	{
		inst->fc = -1;
		inst->ic = -1;
		inst->fci = 0;
	}
	else if(hc->xc > 0)
	{
		inst->fc = _wtk_latset_find_lcontext(ls, inst, inst->nphones, -1);
		inst->ic = _wtk_latset_find_rcontext(ls, inst, -1, -1);
		for(n=0; n < inst->nphones; n++)
		{
			if(wtk_latset_find_ctx(ls, inst->phones[n]->name))
			{
				inst->clen++;
			}
		}

		if(inst->clen == 0 || inst->fc == -1 || inst->ic == -1)
		{
			wtk_debug("NewPronHolder: Every word must define some context [%.*s=%d/%d/%d]\n",
					pron->outsym ? pron->outsym->len : pron->word->name->len, pron->outsym ? pron->outsym->data : pron->word->name->data,
							inst->ic, inst->clen, inst->fc);
		}

		inst->fci = wtk_latset_is_rcontext_ind(ls, inst, inst->nphones);
	}

	return inst;
}

static int _wtk_latset_init_pron_inst(wtk_latset_t *ls, wtk_lat_t *lat, wtk_heap_t *heap)
{
	wtk_dict_pron_t *pron;
	int i, j, k, l,n, t, n_sil, n_add, n_null=0, factor_lm, npii, lc, type;
	float fct;
	wtk_larc_t *la;
	wtk_lnode_t *node;
	wtk_dict_word_t *word;
	PInstInfo *pii;
	//wtk_heap_t *heap;
	wtk_dict_phone_t *sil_phones[256], *add_phones[256];
	wtk_proninst_t *inst;
	wtk_netnode_t *word_node;

	wtk_heap_reset(heap);
	/* Reset hash table prior to processing lattice */
	//memset(ls->wn_node_hash,0,sizeof(wtk_netnode_t*)*ls->wn_nodes);

	/* Determine if we have a real !NULL word */
	ls->null_word = wtk_dict_get_word(ls->dict, ls->dict->null_word->name, 1);
	for(pron=ls->null_word->pron_list; pron != NULL; pron=pron->next)
	{
		if(pron->nPhones != 0)
		{
			ls->null_word = NULL;
			break;
		}
	}
	if(ls->null_word != NULL)
	{
		if(ls->null_word->pron_list == NULL)
		{
			ls->null_word->pron_list = wtk_dict_add_pron(ls->dict, ls->null_word, ls->null_word->name, NULL, 0, 1.0);
		}
	}
	if(frc_sil != NULL && strlen(frc_sil) > 0)
	{
		wtk_debug("frc_sil\n");
	}
	else
	{
		n_sil = n_add = 0;
	}

	factor_lm = ls->cfg->factor_lm;
	for(i=0; i < lat->nn; i++)
	{
		node = lat->lnodes + i;
		fct = 0.0;
		if(factor_lm && node->pred != NULL)
		{
			for(la=node->pred,fct=LZERO; la != NULL; la=la->parc)
			{
				if(la->lmlike > fct) fct = la->lmlike;
			}
		}
		node->score = fct;
	}

	if(factor_lm)
	{
		for(i=0; i < lat->na; i++)
		{
			la = lat->larcs + i;
			la->lmlike -= la->end->score;
		}
	}

	for(i=0,n_null=0,t=0;i<lat->nn;++i)
	{
		node = lat->lnodes+i;
		word = node->info.word;
		//wtk_lnode_print(node);
		//wtk_debug("naux = %d\n", word->npron);
		if(word == NULL) word = ls->dict->null_word;
		if(word->npron <= 0)
		{
			wtk_debug("InitPronHolders: Word %.*s not defined in dictionary\n", word->name->len, word->name->data);
			goto end;
		}
		pii = (PInstInfo *)wtk_heap_malloc(heap, (word->npron+1) * (n_add+1) *sizeof(PInstInfo) );
		pii--;

		/* Scan current pronunciations and make modified ones */
		for(j=1,pron=word->pron_list,npii=0; pron != NULL; j++,pron=pron->next)
		{
			if(pron->nPhones == 0)
			{
				n = 0;
			}
			else
			{
				for(k=1,n=pron->nPhones; k <= n_sil; k++)
				{
					if(pron->pPhones[pron->nPhones-1] == sil_phones[k])
					{
						/* Strip it */
						n--;
						break;
					}
				}
			}

			if(pron->nPhones == 0 || n_add == 0 || n == 0)
			{
				/* Just need one pronunciation */
				if(pron->nPhones == 0)
				{
					n_null++;
				}

				if(n == 0) n = pron->nPhones;
				pii[++npii].pron = pron;
				pii[npii].silId = -1;
				pii[npii].n = n;
				pii[npii].t = n;
				pii[npii].phones = pron->pPhones;
			}
			else
			{
				/* Make one instance per silence label */
				for(k=0; k <= n_add; k++)
				{
					pii[++npii].pron = pron;
					pii[npii].silId = k;
					pii[npii].n = pii[npii].t = n;
					if(add_phones[k] != NULL)
						pii[npii].t++;
					pii[npii].phones = (wtk_dict_phone_t **)wtk_heap_malloc(ls->heap, sizeof(wtk_dict_phone_t *) * pii[npii].t);
					for(l=0; l<pii[npii].n ; l++ )
					{
						pii[npii].phones[l] = pii[npii].pron->pPhones[l];
					}
					if(add_phones[k] != NULL)
					{
						pii[npii].phones[pii[npii].n] = add_phones[k];
					}
				}
			}
		}

		/* Scan new pronunciations and remove duplicates */
		if(remDupPron)
		{
			for(j=2; j <= npii; j++)
			{
				n = pii[j].t;
				if(pii[j].pron == NULL) continue;
				for(k=1; k < j; k++)
				{
					if(pii[j].pron == NULL || pii[k].pron == NULL
							|| pii[k].t != n || pii[j].pron->prob != pii[k].pron->prob)
						continue;
					for(l=0; l < n; l++)
					{
						if(pii[j].phones[l] != pii[k].phones[l]) break;
					}
					if(l == n) pii[j].pron = NULL,t++;
				}
			}
		} /* if(remDupPron) */

		/* Now make the Pron */
		for(j=1; j <= npii; j++)
		{
			/* Don't add duplicates */
			if(pii[j].pron == NULL) continue;
			/* Build inst for each pron */
			inst = _wtk_latset_new_proninst(ls, pii[j].pron, pii[j].t, pii[j].phones);
			//wtk_debug("%d: fci = %d\n",j, inst->fci);
			inst->ln = node;
			inst->next = node->pron_chain;
			node->pron_chain = inst;
			if(inst->nphones <=0) inst->fct = 0.0;
			else inst->fct = node->score/inst->nphones;

			 /* Fake connections from SENT_[START/END] */
			if(ls->hc->xc > 0)
			{
				if(node->pred == NULL)
					wtk_latset_str_hash_add(inst->lc, UNDEF.data, UNDEF.len, (wtk_netnode_t *)lat);    // ? /* pInst->lc[0]=(NetNode*)lat; */
				if(node->foll == NULL)
				{
					if(inst->nphones == 0) lc = 0;
					else lc = inst->fc;
					type = n_word + lc * n_lcontext;
					word_node = wtk_latset_find_word_node(ls, inst->pron , inst, type);
					word_node->nlinks = 0;
					wtk_latset_str_hash_add(inst->rc, UNDEF.data, UNDEF.len, word_node); // pInst->rc[0]=wordNode;
				}
			}
			else if(node->foll == NULL)
			{
				word_node = wtk_latset_find_word_node(ls, pron , inst, n_word);
				word_node->nlinks = 0;
			}
		}
		wtk_heap_reset(heap);

	}

end:
	if(t != 0)
	{
		//wtk_debug("InitPronHolders: Total of %d duplicate pronunciations removed", t);
	}

	return n_null;
}

/*------------------  set numm context  -----------------*/

#define MAX_DEPTH 10 /* Max number of !NULL !NULL links before assuming loop */

static void _wtk_latset_set_null_lrecurse(wtk_latset_t *ls, wtk_lat_t *lat, wtk_proninst_t *inst)
{
	wtk_proninst_t *linst;
	wtk_lnode_t *node;
	wtk_larc_t *la;
	wtk_string_t *phn;
	int i;
	static int depth = 0;

	if(++depth > MAX_DEPTH)
	{
		wtk_debug("SetNullRecurse: Net probably has loop contain just !NULL\n");
		return;
	}

	node = inst->ln;
	for(la=node->pred; la != NULL; la=la->parc)
	{
		for(linst=la->start->pron_chain; linst != NULL; linst=linst->next)
		{
			if(linst->nphones == 0)
				_wtk_latset_set_null_lrecurse(ls, lat, linst);
		}
	}

	for(la=node->pred; la != NULL; la=la->parc)
	{
		for(linst=la->start->pron_chain; linst != NULL; linst=linst->next)
		{
			if(linst->nphones != 0) continue;
			for(i=0; i < ls->hc->xc; i++)
			{
				phn = ls->cxs[i];
				if( wtk_str_hash_find(linst->lc, phn->data, phn->len)
					&& wtk_str_hash_find(inst->lc, phn->data, phn->len) == NULL )
				{
					wtk_latset_str_hash_add(inst->lc, phn->data, phn->len, (wtk_netnode_t *)lat);
				}
			}

		}
	}
	depth--;
}

static void _wtk_latset_set_null_rrecurse(wtk_latset_t *ls, wtk_lat_t *lat, wtk_proninst_t *inst)
{
	wtk_proninst_t *rinst;
	wtk_lnode_t *node;
	wtk_larc_t *la;
	wtk_string_t *phn;
	int i;
	static int depth = 0;

	if(++depth > MAX_DEPTH)
	{
		wtk_debug("SetNullRecurse: Net probably has loop contain just !NULL\n");
		return;
	}

	node = inst->ln;
	for(la=node->foll; la != NULL; la=la->farc)
	{
		for(rinst=(wtk_proninst_t *)la->end->pron_chain; rinst != NULL; rinst=rinst->next)
		{
			if(rinst->nphones == 0)
				_wtk_latset_set_null_rrecurse(ls, lat, rinst);
		}
	}

	for(la=node->foll; la != NULL; la=la->farc)
	{
		for(rinst=(wtk_proninst_t *)la->end->pron_chain; rinst != NULL; rinst=rinst->next)
		{
			if(rinst->nphones != 0) continue;
			for(i=0; i < ls->hc->xc; i++)
			{
				phn = ls->cxs[i];
				if( wtk_str_hash_find(rinst->rc, phn->data, phn->len)
					&& wtk_str_hash_find(inst->rc, phn->data, phn->len) == NULL )
				{
					wtk_latset_str_hash_add(inst->rc, phn->data, phn->len, (wtk_netnode_t *)lat);
				}
			}

		}
	}
	depth--;
}

static void _wtk_latset_set_null_contexts(wtk_latset_t *ls, wtk_lat_t *lat)
{
	wtk_proninst_t *linst, *rinst, *inst;
	wtk_lnode_t *node;
	wtk_larc_t *la;
	unsigned int doPairs;
	int i, lc, rc;

	doPairs = 0;
	for(i=0; i < lat->na; i++)
	{
		la = lat->larcs + i;
		for(linst=la->start->pron_chain; linst != NULL; linst=linst->next)
		{
			for(rinst=la->end->pron_chain; rinst != NULL; rinst=rinst->next)
			{
				if(rinst->nphones == 0 && linst->nphones == 0)
				{
					doPairs = 1;
					rinst->fci = linst->fci = 1;
				}
				else if(rinst->nphones == 0)
				{
					lc = linst->fc;
					wtk_str_hash_add(rinst->lc, ls->cxs[lc]->data, ls->cxs[lc]->len, (wtk_netnode_t *)lat); /* never returned by New */
				}
				else if(linst->nphones == 0)
				{
					rc = rinst->ic;
					wtk_latset_str_hash_add(linst->rc, ls->cxs[rc]->data, ls->cxs[rc]->len, (wtk_netnode_t *)lat); /* never returned by New */
				}
			}
		}
	}

	if(doPairs)
	{
		for(i=0; i < lat->nn; i++)
		{
			node = lat->lnodes + i;
			for(inst=node->pron_chain; inst != NULL; inst=inst->next)
			{
				if(inst->nphones != 0 || !inst->fci) continue;

				_wtk_latset_set_null_lrecurse(ls, lat, inst);
				_wtk_latset_set_null_rrecurse(ls, lat, inst);
			}
		}
	}
}

/* Make wrd-int cd phones (possibly none!) */
static void _wtk_latset_create_wimodels(wtk_latset_t *ls, wtk_lat_t *lat, wtk_proninst_t *inst, int p, int q)
{
	wtk_netnode_t *node;
	wtk_hmm_t *hmm;
	wtk_string_t *ln,*rn;
	int j;

	for(j=q-1; j > p; j--)
	{
		ln=wtk_latset_find_lctx(ls,inst,j,0);
		rn=wtk_latset_find_rctx(ls,inst,j,0);
		hmm = wtk_latset_get_hci_model_t(ls, inst, ln, inst->phones[j]->name, rn);
		if(!hmm)
		{
			wtk_debug("hmm[%.*s] not found.\n",inst->phones[j]->name->len,inst->phones[j]->name->data);
			continue;
		}
		if(inst->tee && hmm->transP[1][hmm->num_state]<LSMALL)
		{
			inst->tee = 0;
		}
		node = wtk_latset_new_netnode(ls,hmm,(inst->chain?1:0));
		if(inst->chain)
		{
			node->links[0].node = inst->chain;
			node->links[0].like = inst->fct;
		}
		node->chain = inst->chain;
		inst->chain = node;
	}
}

int wtk_latset_create_x1models(wtk_latset_t *ls, wtk_lat_t *lat, wtk_proninst_t *inst, int p, int q, wtk_heap_t *heap)
{
	int ret = 0;
	wtk_netnode_t *node, *dest, *word_node, *link_node;
	wtk_netlink_t *links;
	wtk_hmm_t *hmm;
//	void *tptr;
	unsigned int tee, inittee, anytee;
	int i, j, k, n, xc = ls->hc->xc;
	wtk_string_t *lc_str, *rc_str, *phn;
	wtk_hmmset_ctx_t *hc=(ls->hc);

	/*
	 * Single phone word means that we need to
	 *  build a complete cross-bar of contexts
	 */
	tee = 0; /* Assume okay */
	wtk_heap_reset(heap);

	/* Special case because one phone words so expensive */
	if(_wtk_latset_is_hci_context_ind(ls, inst->phones[p]->name))
	{
		hmm=wtk_latset_get_hci_model_t(ls, inst, 0, inst->phones[p]->name, 0);
		if(!hmm)
		{
			wtk_debug("hmm[%.*s] not found.\n",inst->phones[p]->name->len,inst->phones[p]->name->data);
			ret = -1;
			goto end;
		}
		node = wtk_latset_new_netnode(ls, hmm, 0);

		//wtk_debug("node = %p, hmm = %p\n", node, hmm);

		inst->starts = node;

		/* As well as copies of final context free ones */
		for(n=q+1; n < inst->nphones; n++)
		{
			hmm=wtk_latset_get_hci_model_t(ls, inst, 0, inst->phones[n]->name, 0);
			if(!hmm)
			{
				wtk_debug("hmm[%.*s] not found.\n",inst->phones[n]->name->len,inst->phones[n]->name->data);
				ret = -1;
				goto end;
			}
			if(hmm->transP[1][hmm->num_state] < LSMALL) { inst->tee = 0; }
			dest = wtk_latset_new_netnode(ls, hmm, 0);
			dest->chain = inst->chain;
			inst->chain = dest;

			//wtk_debug("node = %p, hmm = %p\n", node, hmm);

			node->links = (wtk_netlink_t*)wtk_heap_malloc(ls->heap,sizeof(wtk_netlink_t) * 1);
			node->nlinks = 1;
			node->links[0].node = dest;
			node->links[0].like = inst->fct;

			node = dest;
		}

		links = (wtk_netlink_t *)wtk_heap_malloc(heap, sizeof(wtk_netlink_t) * hc->xc);
		word_node = NULL;
		for(i=0; i < xc; i++)
		{
			phn = ls->cxs[i];
			if( ( word_node = wtk_str_hash_find(inst->rc, phn->data, phn->len) ) )
			{
				for(k=0; k < node->nlinks; k++)
				{
					if(links[k].node == word_node)
					{
						word_node = NULL;
						break;
					}
				}
				if(word_node != NULL)
				{
					links[node->nlinks].node = word_node;
					links[node->nlinks++].like = inst->fct;
				}
			}
		} /* for(i=0; i < xc; i++) */

		node->links = (wtk_netlink_t *)wtk_heap_malloc(ls->heap, sizeof(wtk_netlink_t) * node->nlinks);
		for(k=0; k < node->nlinks; k++)
		{
			node->links[k] = links[k];
		}

		/* Create any previous context free nodes */
		node = inst->starts;
		inst->starts = NULL;

		for(n=p-1; n >= 0; n--)
		{
			dest = node;
			dest->chain = inst->chain;
			inst->chain = dest;
			hmm = wtk_latset_get_hci_model_t(ls, inst, 0, inst->phones[n]->name, 0);
			if(!hmm)
			{
				wtk_proninst_print(inst);
				wtk_debug("hmm[%.*s] not found.\n",inst->phones[n]->name->len,inst->phones[n]->name->data);
				ret = -1;
				goto end;
			}
			if(hmm->transP[1][hmm->num_state] < LSMALL) { inst->tee = 0; }
			node = wtk_latset_new_netnode(ls, hmm, 1);
			node->links[0].node = dest;
			node->links[0].like = inst->fct;

		}
		inst->nstart = 1;
		inst->starts = node;

		for(i=0; i < xc; i++)
		{
			phn = ls->cxs[i];
			if(wtk_str_hash_find(inst->lc, phn->data, phn->len))
			{
				wtk_latset_str_hash_add(inst->lc, phn->data, phn->len, node);
			}
		} /* for(i=0; i < xc; i++) */

	} /* if(_wtk_latset_is_hci_context_ind(ls, inst->phones[p]->name)) */
	else if( !ls->hc->s_left )
	{
		if(p == 0)
		{
			/*
			 *	 Create NULL node
			 *	 Used as single collating point for all r contexts
			 */
			node = (wtk_netnode_t *)wtk_heap_zalloc(ls->heap, sizeof(wtk_netnode_t));
			node->nlinks = 0;
			node->links = NULL;
			node->inst = NULL;
			node->type = n_word;
			node->info.pron = NULL;
			node->aux = 0;

			node->chain = inst->starts;
			inst->starts = node;
			inst->nstart++;
		}
		else
		{
			hmm=wtk_latset_get_hci_model_t(ls, inst, 0, inst->phones[0]->name, 0);
			if(!hmm)
			{
				wtk_debug("hmm[%.*s] not found.\n",inst->phones[0]->name->len,inst->phones[0]->name->data);
				ret = -1;
				goto end;
			}
			if(hmm->transP[1][hmm->num_state] < LSMALL) { inst->tee = 0; }
			node=wtk_latset_new_netnode(ls, hmm, 0);

			/* Chain these after NULL node */
			node->chain = inst->starts;
			inst->starts = node;
			inst->nstart++;

			/* Create any previous context free nodes */
			for(n=1; n < p; n++)
			{
				hmm=wtk_latset_get_hci_model_t(ls, inst, 0, inst->phones[n]->name, 0);
				if(!hmm)
				{
					wtk_debug("hmm[%.*s] not found.\n",inst->phones[n]->name->len,inst->phones[n]->name->data);
					ret = -1;
					goto end;
				}
				dest = wtk_latset_new_netnode(ls, hmm, 0);
				node->nlinks = 1;
				node->links = (wtk_netlink_t *)wtk_heap_malloc(ls->heap, sizeof(wtk_netlink_t));

				node->links[0].node = dest;
				node->links[0].like = 0.0;

				/* Chain these after NULL node */
				dest->chain = inst->chain;
				inst->chain = dest;

				node = dest;
			}

		} /* if(p == 0) */
		link_node = node;

		for(i=0; i < xc; i++)
		{
			phn = ls->cxs[i];
			if(wtk_str_hash_find(inst->lc, phn->data, phn->len))
			{
				wtk_latset_str_hash_add(inst->lc, phn->data, phn->len, inst->starts);
			}
		}

		/* Now create actual cd phone */
//		tptr = wtk_heap_malloc(heap, 1); /* Need place holder to free */
		anytee = 0; /* Haven't seen any final tee chains */
		for(i=0; i < xc; i++)
		{
			phn = ls->cxs[i];
			if(wtk_str_hash_find(inst->rc, phn->data, phn->len) == NULL) { continue; }

			hmm=wtk_latset_get_hci_model_t(ls, inst, 0, inst->phones[q]->name, phn);
			if(!hmm)
			{
				wtk_debug("hmm[%.*s] not found.\n",inst->phones[q]->name->len,inst->phones[q]->name->data);
				ret = -1;
				goto end;
			}
			for(node=inst->ends; node != NULL; node=node->chain)
				if(node->info.hmm == hmm) break;

			if(node == NULL)
			{
				/* Create new model */
				node = wtk_latset_new_netnode(ls, hmm, 0);
				node->chain = inst->ends;
				inst->ends = node;
				inst->nend++;
				link_node->nlinks++;

				/* As well as copies of final context free ones */
				for(n=q+1; n < inst->nphones; n++)
				{
					hmm=wtk_latset_get_hci_model_t(ls, inst, 0, inst->phones[n]->name, 0);
					if(!hmm)
					{
						wtk_debug("hmm[%.*s] not found.\n",inst->phones[q]->name->len,inst->phones[q]->name->data);
						ret = -1;
						goto end;
					}
					dest = wtk_latset_new_netnode(ls, hmm, 0);
					dest->chain = inst->chain;
					inst->chain = dest;

					node->links = (wtk_netlink_t *)wtk_heap_malloc(ls->heap, sizeof(wtk_netlink_t));
					node->nlinks = 1;
					node->links[0].node = dest;
					node->links[0].like = 0.0;

					node = dest;
				}
				if(tee) anytee = 1; /* A single tee chain is too many */

				/* Use inst pointer to get to final model in links */
				inst->ends->inst = (wtk_netnode_t *)node;

				node->links = (wtk_netlink_t *)wtk_heap_malloc(heap, sizeof(wtk_netlink_t) * hc->xc);
			}
			else
			{
				/* Find final model in links */
				node = (wtk_netnode_t *)node->inst;
			} /* if(node == NULL) */

			node->links[node->nlinks].node = wtk_str_hash_find(inst->rc, phn->data, phn->len);
			node->links[node->nlinks].like = 0.0;
			node->nlinks++;
		}
		if(!anytee) inst->tee = 0; /* Didn't see any tee chains */

		/* Now allocate and copy links */
		for(node=inst->ends; node != NULL; node=node->chain)
		{
			dest = (wtk_netnode_t *)node->inst;/* Find end of cf models */
			links = (wtk_netlink_t *)wtk_heap_malloc(ls->heap, sizeof(wtk_netlink_t) * dest->nlinks);
			for(n=0; n < dest->nlinks; n++)
				links[n] = dest->links[n];
			dest->links = links;
		}

		/* And finally link null node to models */
		link_node->links = (wtk_netlink_t *)wtk_heap_malloc(ls->heap, sizeof(wtk_netlink_t) * link_node->nlinks);
		for(dest=inst->ends,n=0,node=NULL; dest != NULL; dest=dest->chain)
		{
			node = dest;
			link_node->links[n].node = dest;
			link_node->links[n].like = 0.0;
			n++;
		}

		/* Move these models over to chain */
		node->chain = inst->chain;
		inst->chain = inst->ends;
		inst->ends = NULL;
		inst->nend = 0;

	} /* !ls->hc->s_left */
	else
	{
		/* Otherwise we do it properly */
		anytee = 0; /* Haven't seen any tee chains */
		for(i=0; i < xc; i++)
		{
			lc_str = ls->cxs[i];
			if(NULL == wtk_str_hash_find(inst->lc, lc_str->data, lc_str->len)) { continue; }

			inittee = 1; /* Start off assuming the worse */
			if(0 == p)
			{
				 /* Create NULL node
				  *  Used as single collating point for all r contexts
				  */
				node = (wtk_netnode_t *)wtk_heap_zalloc(ls->heap, sizeof(wtk_netnode_t));
				node->nlinks = 0;
				node->links = NULL;
				node->inst = NULL;
				node->type = n_word;
				node->info.pron = NULL;
				node->aux = 0;

				node->chain = inst->starts;
				inst->starts = node;
				inst->nstart++;

				//wtk_debug("node = %p, hmm = %p\n", node, hmm);

				wtk_latset_str_hash_add(inst->lc, lc_str->data, lc_str->len, node);
			} /* 0 == p */
			else
			{
				hmm = wtk_latset_get_hci_model_t(ls, inst, 0, inst->phones[0]->name, 0);
				if(!hmm)
				{
					wtk_debug("hmm[%.*s] not found.\n",inst->phones[0]->name->len,inst->phones[0]->name->data);
					ret = -1;
					goto end;
				}
			   node = wtk_latset_new_netnode(ls, hmm, 0);
			   wtk_latset_str_hash_add(inst->lc, lc_str->data, lc_str->len, node);

			   //wtk_debug("node = %p, hmm = %p\n", node, hmm);

				/* Chain these after NULL node */
			   node->chain = inst->starts;
			   inst->starts = node;
			   inst->nstart++;

			   for(n=1; n < p; n++)
				{
				   hmm = wtk_latset_get_hci_model_t(ls, inst, 0, inst->phones[n]->name, 0);
				   if(!hmm)
					{
						wtk_debug("hmm[%.*s] not found.\n",inst->phones[n]->name->len,inst->phones[n]->name->data);
						ret = -1;
						goto end;
					}
				   dest = wtk_latset_new_netnode(ls, hmm, 0);
				   node->nlinks = 1;
				   node->links = (wtk_netlink_t *)wtk_heap_malloc(ls->heap, sizeof(wtk_netlink_t));
				   node->links[0].node = dest;
				   node->links[0].like = inst->fct;

				   /* Chain these after NULL node */
				   dest->chain = inst->chain;
				   inst->chain = dest;

				   node = dest;
			   } /* for(n=1; n < p; n++) */

			} /* 0 != p */
			link_node = node;

			/* Now create actual cd phone */
//			tptr = wtk_heap_malloc(heap, 1); /* Need place holder to free */
			for(j=0; j < xc; j++)
			{
				rc_str = ls->cxs[j];

				if( NULL == wtk_str_hash_find(inst->rc, rc_str->data, rc_str->len) ) { continue; }

				hmm = wtk_latset_get_hci_model_t(ls, inst, lc_str, inst->phones[q]->name, rc_str);
			   if(!hmm)
				{
					wtk_debug("hmm[%.*s] not found.\n",inst->phones[q]->name->len,inst->phones[q]->name->data);
					ret = -1;
					goto end;
				}
			   for(node=inst->ends; node != NULL; node=node->chain)
			   {
				   if(node->info.hmm == hmm)
					   break;
			   }

			   if(node == NULL)
			   {
				   if(hmm->transP[1][hmm->num_state] < LSMALL) { tee = 0; }  /* Okay */
				   else { tee = inittee; }  /* Start at end of initial chain */

				   /* Create new model */
				   node = wtk_latset_new_netnode(ls, hmm, 0);
				   node->chain = inst->ends;
				   inst->ends = node;
				   inst->nend++;
				   link_node->nlinks++;

				   //wtk_debug("node = %p, hmm = %p\n", node, hmm);

				   /* As well as copies of final context free ones */
				   for(n=q+1; n < inst->nphones; n++)
				   {
					   hmm = wtk_latset_get_hci_model_t(ls, inst, 0, inst->phones[n]->name, 0);
					   if(!hmm)
						{
							wtk_debug("hmm[%.*s] not found.\n",inst->phones[n]->name->len,inst->phones[n]->name->data);
							ret = -1;
							goto end;
						}
					   if(hmm->transP[1][hmm->num_state] < LSMALL) { tee = 0; }
					   dest = wtk_latset_new_netnode(ls, hmm, 0);
					   dest->chain = inst->chain;
					   inst->chain = dest;

					   node->links = (wtk_netlink_t *)wtk_heap_malloc(ls->heap, sizeof(wtk_netlink_t));
					   node->nlinks = 1;
					   node->links[0].node = dest;
					   node->links[0].like = inst->fct;

					   node = dest;
				   } /* for(n=q+1; n < inst->nphones; n++) */
				   if(tee) { anytee = 1; }  /* A single tee chain is too many */

				   /* Use inst pointer to get to final model in links */
				   inst->ends->inst = (wtk_netnode_t *)node;

				   node->links = (wtk_netlink_t *)wtk_heap_malloc(heap, sizeof(wtk_netlink_t) * hc->xc);
			   } /* if(node == NULL) */
			   else
			   {
				   /* Find final model in links */
				   node = (wtk_netnode_t *)node->inst;
			   }

				   node->links[node->nlinks].node = wtk_str_hash_find(inst->rc, rc_str->data, rc_str->len);
				   node->links[node->nlinks].like = inst->fct;
				   node->nlinks++;

			} /* for(j=0; j < xc; j++)*/

			/* Now allocate and copy links */
			for(node=inst->ends; node != NULL; node = node->chain)
			{
				dest = (wtk_netnode_t *)node->inst; /* Find end of cf models */
				links = (wtk_netlink_t *)wtk_heap_malloc(ls->heap, sizeof(wtk_netlink_t) * dest->nlinks);
				for(n=0; n < dest->nlinks; n++)
				{
					links[n] = dest->links[n];
				}
				dest->links = links;
			}

			 /* And finally link null node to models */
			link_node->links = (wtk_netlink_t *)wtk_heap_malloc(ls->heap, sizeof(wtk_netlink_t) * link_node->nlinks);
			for(dest=inst->ends,n=0,node=NULL; dest != NULL; dest=dest->chain)
			{
				node = dest;
				link_node->links[n].node = dest;
				link_node->links[n].like = (p == 0 ? 0.0 : inst->fct);
				n++;
			}

			/* Move these models over to chain */
			node->chain = inst->chain;
			inst->chain = inst->ends;
			inst->ends = NULL;
			inst->nend = 0;

		} /* for(i=0; i < xc; i++) */

		 if (!anytee) inst->tee = 0; /* Didn't see any completely tee chains */
	}

end:

	return ret;
}

int wtk_latset_create_xemodels(wtk_latset_t *ls, wtk_lat_t *lat, wtk_proninst_t *inst, int p, int q, wtk_heap_t *heap)
{
	int ret = 0;
	wtk_netnode_t *node, *dest, *chain_node;
	wtk_netlink_t *links;
	wtk_hmm_t *hmm;
//	void *tptr;
	int tee, anytee;
	int i, j, n, xc = ls->hc->xc;
	wtk_string_t *lc_str, *rc_str;

	/* Cross word context and more than one phone */

	/* Last cd phone */
	chain_node = NULL;
//	search_node = NULL;
	// tptr=New(heap,1);
	anytee = 0; /* Haven't seen any final tee chains */
	lc_str = wtk_latset_find_lctx(ls, inst, q, 0);
	wtk_heap_reset(heap);
	for(i=0; i < xc; i++)
	{
		rc_str = ls->cxs[i];

		if( wtk_str_hash_find(inst->rc, rc_str->data, rc_str->len) == NULL ) { continue; }

		//wtk_debug("end: rc_str = %.*s\n", rc_str->len, rc_str->data);

		hmm = wtk_latset_get_hci_model_t(ls, inst, lc_str, inst->phones[q]->name, rc_str);
		if(!hmm)
		{
			wtk_debug("hmm[%.*s] not found.\n",inst->phones[q]->name->len,inst->phones[q]->name->data);
			ret = -1;
			goto end;
		}

		for(node=inst->ends; node != NULL; node=node->chain)
		{
			if(node->info.hmm == hmm)
				break;
		}

		if(node == NULL)
		{
			if(hmm->transP[1][hmm->num_state]<LSMALL) tee = 0; /* Okay now */
			else tee = 1;  /* Still could be save by final CF models */

			node = wtk_latset_new_netnode(ls, hmm, 0);
			node->chain = inst->ends;
			inst->ends = node;
			inst->nend++;

			//wtk_debug("node = %p, hmm = %p\n", node, hmm);

			for(n=q+1; n < inst->nphones;n ++)
			{
				hmm = wtk_latset_get_hci_model_t(ls, inst, 0, inst->phones[n]->name, 0);
				if(!hmm)
				{
					wtk_debug("hmm[%.*s] not found.\n",inst->phones[q]->name->len,inst->phones[q]->name->data);
					ret = -1;
					goto end;
				}
				if (hmm->transP[1][hmm->num_state]<LSMALL) tee = 0; /* Saved */
				dest = wtk_latset_new_netnode(ls, hmm, 0);
				dest->chain = chain_node;
				chain_node = dest;

				//wtk_debug("node = %p, hmm = %p\n", dest, hmm);

				node->links = (wtk_netlink_t*)wtk_heap_malloc(ls->heap, sizeof(wtk_netlink_t));
				node->nlinks = 1;
				node->links[0].node = dest;
				node->links[0].like = inst->fct;

				node = dest;
			}

			/* Use inst pointer to get to final model in links */
			inst->ends->inst = (wtk_netnode_t *)node;

			node->links = (wtk_netlink_t *)wtk_heap_malloc(ls->heap, sizeof(wtk_netlink_t) * ls->hc->xc);
			if(tee) { anytee = 1; /* A single tee chain is too many */  }
		}
		else
		{
			/* Find end of cf models */
			node = (wtk_netnode_t *)node->inst;
		}

		//wtk_debug("node[%p]: link-node = %p, rc_str = %.*s\n", node, wtk_str_hash_find(inst->rc, rc_str->data, rc_str->len), rc_str->len, rc_str->data);
		node->links[node->nlinks].node = wtk_str_hash_find(inst->rc, rc_str->data, rc_str->len);
		//wtk_debug("inst[%p]->rc[%.*s]: type = %d, word = %.*s\n", inst, rc_str->len, rc_str->data, node->links[node->nlinks].node->type,
		//		node->links[node->nlinks].node->info.pron->word->name->len, node->links[node->nlinks].node->info.pron->word->name->data);
		node->links[node->nlinks].like = inst->fct;
		node->nlinks++;
		if(inst->fci) { break; /* Only need to do this once */ }
	} /* for(i=0; i < xc; i++) */
	if(!anytee) { inst->tee = 0; } /* Didn't see any tee chains */

	/* Now allocate and copy links */
	for(node=inst->ends; node != NULL; node=node->chain)
	{
		dest = (wtk_netnode_t *)node->inst; /* Find end of cf models */
		links = (wtk_netlink_t *)wtk_heap_malloc(ls->heap, sizeof(wtk_netlink_t) * dest->nlinks);
		for(j=0; j < dest->nlinks; j++)
		{
			links[j] = dest->links[j];
		}
		dest->links = links;
	}

	//wtk_debug("inst->ends[%p]: link-node = %p\n", inst->ends, inst->ends->links[0].node);

	/* And finally link to ci part of word */
	if(inst->chain != NULL)
	{
		for(node=inst->chain; node->chain != NULL; node=node->chain);
		node->nlinks = inst->nend;
		node->links = (wtk_netlink_t *)wtk_heap_malloc(ls->heap, sizeof(wtk_netlink_t) * node->nlinks);
		for(dest=inst->ends,n=0; dest != NULL; dest=dest->chain)
		{
			node->links[n].node = dest;
			node->links[n].like = inst->fct;
			n++;
		}
	}

	//wtk_debug("inst->ends[%p]: link-node = %p\n", inst->ends, inst->ends->links[0].node);

	anytee = 0; /* Haven't seen any initial tee chains */

	/* Create first cd phone */
	rc_str=wtk_latset_find_rctx(ls,inst,p,0);
	for(i=0; i < xc; i++)
	{
		lc_str = ls->cxs[i];
		if( wtk_str_hash_find(inst->lc, lc_str->data, lc_str->len) == NULL ) { continue; }

		//wtk_debug("start: lc_str = %.*s\n", lc_str->len, lc_str->data);

		hmm = wtk_latset_get_hci_model_t(ls, inst, lc_str, inst->phones[p]->name, rc_str);
		if(!hmm)
		{
			wtk_debug("hmm[%.*s] not found.\n",inst->phones[p]->name->len,inst->phones[p]->name->data);
			ret = -1;
			goto end;
		}

		for(node=inst->starts; node != NULL; node=node->chain)
		{
			if(node->info.hmm == hmm)
				break;
		}

		if(node == NULL)
		{
			if(hmm->transP[1][hmm->num_state] < LSMALL)
				tee = 0; /* OKay now */
			else
				tee = 1; /* Still could be save by initial CF models */

			node = wtk_latset_new_netnode(ls, hmm, (inst->chain == NULL ? inst->nend : 1));
			node->chain = inst->starts;
			inst->starts = node;
			inst->nstart++;

			//wtk_debug("node = %p, hmm = %p\n", node, hmm);

			if(inst->chain != NULL)
			{
				node->links[0].node = inst->chain;
				node->links[0].like = inst->fct;
			}
			else
			{
				 /* Two phone words need crossbar linking */
				for(dest=inst->ends,n=0; dest != NULL; dest=dest->chain,n++)
				{
					node->links[n].node = dest;
					node->links[n].like = inst->fct;
				}
			}

			for(n=p-1; n >= 0; n--)
			{
				dest = node;
				hmm = wtk_latset_get_hci_model_t(ls, inst, 0, inst->phones[n]->name, 0);
				if(!hmm)
				{
					wtk_debug("hmm[%.*s] not found.\n",inst->phones[p]->name->len,inst->phones[p]->name->data);
					ret = -1;
					goto end;
				}
				if(hmm->transP[1][hmm->num_state] < LSMALL) tee = 0; /* Saved */
				node = wtk_latset_new_netnode(ls, hmm, 1);

				if(n != 0)
				{
					node->chain =  chain_node;
					chain_node = node;
				}
				node->links[0].node = dest;
				node->links[0].like = inst->fct;
			}
			if(tee) anytee = 1; /*  A single tee chain is too many */

			/* Link to start of cf models */
			inst->starts->inst = (wtk_netnode_t *)node;
		}
		else
		{
			node=(wtk_netnode_t *)node->inst; /* Find start of cf models */
		}

		/* Point to start of cf models */
		//wtk_debug("inst[%p]->lc[%.*s]:\n", inst, lc_str->len, lc_str->data);
		//wtk_netnode_print(node);
		wtk_latset_str_hash_add(inst->lc, lc_str->data, lc_str->len, (void *)node);
		//wtk_netnode_print(wtk_str_hash_find(inst->lc, lc_str->data, lc_str->len));
	} /* for(i=0; i < hash->nslot; i++) */
	if(!anytee) { inst->tee = 0; } /* Didn't see any tee chains */

	if(p != 0)
	{
		/* Need to fix starts list */
		for(node=inst->starts,inst->starts=NULL; node != NULL; node=dest)
		{
			dest = (wtk_netnode_t *)node->inst;
			dest->chain = inst->starts;
			inst->starts = dest;
			dest = node->chain;
			node->chain = chain_node;
			chain_node = node;
		}
	} /* if(p != 0) */

	if(inst->chain == NULL)
	{
		inst->chain =  chain_node;
	}
	else
	{
		for(node=inst->chain; node->chain != NULL; node=node->chain);
		node->chain = chain_node;
	}

end:

	return ret;
}

/*
 * Process the cross word links, the first time heap!=NULL and
 * the links are counted and wordnodes created, the second
 * time heap==NULL and link likelihoods/destinations are set.
 */
static void _wtk_latset_process_cross_word_links(wtk_latset_t *ls, wtk_lat_t *lat, wtk_heap_t *heap)
{
	wtk_proninst_t *linst, *rinst;
	wtk_netnode_t *word_node;
	wtk_larc_t		*la;
	int i, lc, rc, type, xc;
//	wtk_string_t *lc_str, *rc_str;
//	wtk_str_hash_t *hash = ls->hc->cxs_hash;
//	wtk_queue_node_t *l_qn, *r_qn;
//	hash_str_node_t *l_hash_n, *r_hash_n;

	/*
	 * Currently a new word end is created for all logical contexts
	 * This is only needed for single phone words for which several
	 * odels (in different contexts) connect to a single word end.
	 * For multi-phone words no models are shared and so a single
	 * word end per distinct physical model would be fine.
	 */
	xc = ls->hc->xc;
	for(i=0; i < lat->na; i++)
	{
		la = lat->larcs + i;
		for(linst=la->start->pron_chain; linst != NULL; linst=linst->next)
		{
			for(rinst=la->end->pron_chain; rinst != NULL; rinst=rinst->next)
			{
				if(ls->hc->xc == 0)
				{
					word_node = wtk_latset_find_word_node(ls, linst->pron, linst, n_word);
					if(heap != NULL) {  }
					if(heap == NULL)
					{
						word_node->links[word_node->nlinks].node = rinst->starts;
						word_node->links[word_node->nlinks].like = la->lmlike;
					}
					word_node->nlinks++;
				} /* if(ls->hc->xc == 0) */
				else if(rinst->nphones == 0 && linst->nphones == 0)
				{
					//wtk_debug("rinst->nphones == 0 && linst->nphones == 0\n");
					for(lc=0; lc < xc; lc++)
					{
						if( NULL == wtk_str_hash_find(linst->lc, ls->cxs[lc]->data, ls->cxs[lc]->len)
							|| NULL == wtk_str_hash_find(rinst->lc, ls->cxs[lc]->data, ls->cxs[lc]->len) )
						{
							continue;
						}
						for(rc=0; rc < xc; rc++)
						{
							if( NULL == wtk_str_hash_find(linst->rc, ls->cxs[rc]->data, ls->cxs[rc]->len)
								|| NULL == wtk_str_hash_find(rinst->rc, ls->cxs[rc]->data, ls->cxs[rc]->len) )
							{
								continue;
							}
							type = n_word + lc*n_lcontext + rc*n_rcontext;
							word_node = wtk_latset_find_word_node(ls , linst->pron, linst, type);
							//wtk_debug("word_node[%p]: lc = %d, rc = %d, type = %d\n", word_node, lc, rc, type);
							if(heap != NULL) {  }

							if(heap == NULL)
							{
								if(rinst->ln->foll == NULL)
								{
									type = n_word;
								}
								word_node->links[word_node->nlinks].node = wtk_latset_find_word_node(ls, rinst->pron, rinst, type);
								word_node->links[word_node->nlinks].like = la->lmlike;;
							}
							word_node->nlinks++;
						}
					} /* for(lc=0; lc < xc; lc++) */
				} /* e if(rinst->nphones == 0 && linst->nphones == 0) */
				else if(rinst->nphones == 0)
				{
					//wtk_debug("rinst->nphones == 0\n");
					if(la->end->foll == NULL)
						lc = 0;
					else
						lc = linst->fc;
					if(linst->fci)
					{
						type = n_word + lc*n_lcontext;
						word_node = wtk_latset_find_word_node(ls, linst->pron, linst, type);
						//wtk_debug("word_node[%p]: lc = %d, rc = %d, type = %d, linst->fci = %d\n", word_node, lc, rc, type, linst->fci);
						if(heap != NULL) {  }
					}
					else
					{
						word_node = NULL; /* Keeps the compiler happy */
					}

					for(rc=0; rc < xc; rc++)
					{
						if( NULL == wtk_str_hash_find(rinst->rc, ls->cxs[rc]->data, ls->cxs[rc]->len) )
						{
							continue;
						}

						if(!linst->fci)
						{
							type = n_word + lc*n_lcontext + rc*n_rcontext;
							word_node = wtk_latset_find_word_node(ls, linst->pron, linst, type);
							//wtk_debug("word_node[%p]: lc = %d, rc = %d, type = %d, linst->fci = %d\n", word_node, lc, rc, type, linst->fci);
							if(heap != NULL) {}
						} /* if(!linst->fci) */

						if(heap == NULL)
						{
							type = n_word + lc*n_lcontext + rc*n_rcontext;
							word_node->links[word_node->nlinks].node = wtk_latset_find_word_node(ls, rinst->pron, rinst, type);
							word_node->links[word_node->nlinks].like = la->lmlike;
						}
						else
						{
							wtk_latset_str_hash_add(linst->rc, ls->cxs[rc]->data, ls->cxs[rc]->len, word_node);
						}
						word_node->nlinks++;

					}
				} /* if(rinst->nphones == 0) */
				else if(linst->nphones == 0)
				{
					//wtk_debug("linst->nphones == 0\n");
					if(la->start->pred == NULL) { rc = 0; }
					else { rc = rinst->ic; }
					for(lc=0; lc < xc; lc++)
					{
						if( NULL == wtk_str_hash_find(linst->lc, ls->cxs[lc]->data, ls->cxs[lc]->len) )
						{
							continue;
						}
						type = n_word + lc*n_lcontext + rc*n_rcontext;
						word_node = wtk_latset_find_word_node(ls, linst->pron, linst, type);
						//wtk_debug("word_node[%p]: lc = %d, rc = %d, type = %d\n", word_node, lc, rc, type);
						if(heap != NULL) {}
						if(heap == NULL)
						{
							word_node->links[word_node->nlinks].node = wtk_str_hash_find(rinst->lc, ls->cxs[lc]->data, ls->cxs[lc]->len);
							word_node->links[word_node->nlinks].like = la->lmlike;
						}
						else
						{
							wtk_latset_str_hash_add(rinst->lc, ls->cxs[lc]->data, ls->cxs[lc]->len, (wtk_netnode_t *)lat);
						}
						word_node->nlinks++;
					} /* for(lc=0; lc < xc; lc++) */
				} /* if(linst->nphones == 0) */
				else
				{
					lc = linst->fc;
					rc = rinst->ic;
					//wtk_debug("lc = %d, rc = %d\n", lc, rc);
					if(linst->fci) { type = n_word + lc*n_lcontext; }
					else { type = n_word + lc*n_lcontext + rc*n_rcontext; }

					word_node = wtk_latset_find_word_node(ls, linst->pron, linst, type);
					//wtk_debug("word_node[%p]: lc = %d, rc = %d, type = %d, linst->fci = %d\n", word_node, lc, rc, type, linst->fci);
					if(heap != NULL) {}
					if(heap == NULL)
					{
						//wtk_debug("rinst[%p]->lc: %.*s\n", rinst, ls->cxs[lc]->len, ls->cxs[lc]->data);
						word_node->links[word_node->nlinks].node = wtk_str_hash_find(rinst->lc, ls->cxs[lc]->data, ls->cxs[lc]->len);
						word_node->links[word_node->nlinks].like = la->lmlike;
					}
					else
					{
						wtk_latset_str_hash_add(linst->rc, ls->cxs[rc]->data, ls->cxs[rc]->len, word_node);
						wtk_latset_str_hash_add(rinst->lc, ls->cxs[lc]->data, ls->cxs[lc]->len, (wtk_netnode_t *)lat);
					}
					word_node->nlinks++;

				}

			} /* for(rinst=la->end->pron_chain; rinst != NULL; rinst=rinst->next) */
		} /* for(linst=la->start->pron_chain; linst != NULL; linst=linst->next) */
	} /* for(i=0; i < lat->na; i++) */

}

/* AddInitialFinal: Add links to/from initial/final net nodes */
static void _wtk_latset_add_initial_final(wtk_latset_t *ls, wtk_lat_t *lat)
{
	wtk_proninst_t *inst;
	wtk_lnode_t *node,*node2;
	int i,ninit,type;
	wtk_array_t *a=ls->tmp_array;
	wtk_lnode_t **pn;
	wtk_netnode_t *n,*n2;

	ninit=0;
	//this need done when in network scan before.
	wtk_array_reset(a);
	for(i=0;i<lat->nn;++i)
	{
		node=lat->lnodes+i;
		if(!node->pred)
		{
			pn=(wtk_lnode_t**)wtk_array_push(a);
			pn[0]=node;
			for(inst=node->pron_chain;inst;inst=inst->next,++ninit);
		}
	}
	n=&(lat->initial);
	n->type=n_word;
	n->info.pron=0;
	n->nlinks=0;
	n->links=(wtk_netlink_t*)wtk_heap_malloc(ls->heap,ninit*sizeof(wtk_netlink_t));
	for(i=0;i<a->nslot;++i)
	{
		node2=((wtk_lnode_t**)a->slot)[i];
		for(inst=node2->pron_chain;inst;inst=inst->next)
		{
			//wtk_lnode_print(inst->ln,0);
			if(ls->hc->xc == 0)
				n2 = inst->starts;
			else if(inst->nphones != 0)
				n2 = wtk_str_hash_find(inst->lc, UNDEF.data, UNDEF.len);
			else
				n2 = wtk_latset_find_word_node(ls,inst->pron,inst,n_word);

			n->links[n->nlinks].node=n2;
			n->links[n->nlinks].like=0;
			++n->nlinks;
		}
	}
	n=&(lat->final);
	n->type=n_word;
	n->info.pron=0;
	n->nlinks=0;
	n->links=0;
	for(i=0;i<lat->nn;++i)
	{
		node=lat->lnodes+i;
		if(!node->foll)
		{
			for(inst=node->pron_chain;inst;inst=inst->next)
			{
				if(ls->hc->xc == 0 || inst->nphones == 0)
					n2 = wtk_latset_find_word_node(ls,inst->pron,inst,n_word);
				else
				{
					type = n_word + inst->fc * n_lcontext; /* rc==0 */
					n2 = wtk_latset_find_word_node(ls, inst->pron, inst, type);
				}
				n2->nlinks=1;
				n2->links=(wtk_netlink_t*)wtk_heap_malloc(ls->heap,sizeof(wtk_netlink_t));
				n2->links[0].node=n;
				n2->links[0].like=0;
				//printf("%*.*s\n",n2->info.pron->outsym->len,n2->info.pron->outsym->len,n2->info.pron->outsym->data);
			}
		}
	}
}

static int _wtk_netlink_is_wd0(wtk_netlink_t *link)
{
	wtk_netnode_t *node;
	int ret,i;

	node=link->node;
	if((node->type&n_nocontext)==n_word){ret=1;goto end;}
	if(node->type & n_tr0)
	{
		for(i=0;i<node->nlinks;++i)
		{
			if(_wtk_netlink_is_wd0(node->links+i))
			{
				ret=1;goto end;
			}
		}
	}
	ret=0;

end:
	return ret;
}

void _wtk_netnode_print(wtk_netnode_t *node)
{
	int i;
	char *name;

	if(node->type == n_word)
	{
		name=wtk_netnode_name2(node);
		printf("word[%p]: %s, pron = %p, type = %d, nlinks = %d\n", node, name ? name:"NULL", node->info.pron, node->type, node->nlinks);
	}
	else
	{
		printf("node[%p]: hmm = %p, type = %d\n", node, node->info.hmm, node->type);
	}

	for(i=0; i < node->nlinks; i++)
	{
		if(node->links[i].node->type == n_word)
		{
			name = wtk_netnode_name2(node->links[i].node);
			printf("links[%d/%d]: node = %p, word[%p] = %s, type = %d\n", i , node->nlinks, node->links[i].node, node->links[i].node->info.pron, name, node->links[i].node->type);
		}
		else if(node->links[i].node->type == n_hmm)
		{
			printf("links[%d/%d]: node = %p, hmm = %p, type = %d\n", i, node->nlinks, node->links[i].node, node->links[i].node->info.hmm, node->links[i].node->type);
		}
		else
		{
			printf("links[%d/%d]: node = %p, pron = %p, hmm = %p, type = %d\n", i, node->nlinks, node->links[i].node, node->links[i].node->info.pron, node->links[i].node->info.hmm, node->links[i].node->type);
		}

	}
}

/*
 * Surport cross word
 */
int wtk_latset_expand_lat2(wtk_latset_t *ls,wtk_lat_t *lat)
{
	int ret, n_null, i, j, n, p, q;
	wtk_net_cfg_t *cfg=ls->cfg;
	wtk_hmmset_ctx_t *hc=NULL;
	wtk_lnode_t 		*lnode;
	wtk_netnode_t		*node, *word_node, *chain_node;
	wtk_dict_word_t 	*word;
	wtk_proninst_t 	*inst;
	wtk_netlink_t		netlink;

	wtk_str_hash_t	*hash;
	wtk_queue_node_t	*qn;
	hash_str_node_t	*hash_n;
	wtk_heap_t			*heap = NULL;

	if(ls->hc)
	{
		wtk_hmmset_ctx_delete(ls->hc);
		ls->hc=0;
	}

	if(!(cfg->allow_xwrd_exp||cfg->allow_ctx_exp) || (!cfg->force_ctx_exp
			&& !cfg->force_left_biphones && !cfg->force_right_biphones
			&&  wtk_dict_is_closed(ls->dict,ls->hl)))
	{
		//wtk_debug("use 1\n");
		ls->hc=wtk_hmmset_ctx_new(ls->hl,1);
		//ret=wtk_hmmset_ctx_init(hc,ls->hl,1);
	}else
	{
		//wtk_debug("use 0\n");
		ls->hc=wtk_hmmset_ctx_new2(ls->hl,0);

	}
	ret=ls->hc?0:-1;
	if(ret!=0){goto end;}

	hc = ls->hc;
	/*  add sil in cis */
	wtk_hmmset_ctx_add_ref_label(hc,hc->cis_hash,&SIL);

	if(cfg->allow_xwrd_exp && hc->nc > 0 && (cfg->force_right_biphones || cfg->force_left_biphones || cfg->force_ctx_exp))
	{
		hc->xc = hc->nc + 1;
		heap = wtk_heap_new(1024);
		ls->cxs = wtk_heap_malloc(ls->heap, sizeof(wtk_string_t *) * hc->xc);

		hash = ls->hc->cxs_hash;
		ls->cxs[0] = &UNDEF;
		n = 1;
		for(i=0; i < hash->nslot; i++)
		{
			if(!hash->slot[i]) { continue; }
			for(qn=hash->slot[i]->pop; qn; qn=qn->next)
			{
				hash_n = data_offset(qn, hash_str_node_t, n);

				ls->cxs[n] = &(hash_n->key);
				n++;
			}
		}

//		wtk_debug("xc = %d, n = %d\n", hc->xc, n);
//		for(i=0; i < hc->xc; i++ )
//		{
//			wtk_debug("name[%d] = %.*s\n", i, ls->cxs[i]->len, ls->cxs[i]->data);
//		}

	}
	else
	{
		hc->xc = 0;
	}

	/* First create context arrays and pronunciation instances */
	n_null = _wtk_latset_init_pron_inst(ls, lat, heap);

	/* Need to find out the phonetic contexts for all NULL words */
	if(hc->xc > 0 && n_null > 0)
		_wtk_latset_set_null_contexts(ls, lat);

	/* Count xwrd links and create word ends */
	_wtk_latset_process_cross_word_links(ls, lat, ls->heap);

	/* Build models on basis of contexts seen */
	ls->tee_words = 0;
	for(i=0; i < lat->nn; i++)
	{
		lnode = lat->lnodes + i;
		word = lnode->info.word;
		//wtk_lnode_print(lnode);
		if(word == NULL) word = ls->dict->null_word;
		for(inst=lnode->pron_chain; inst != NULL; inst=inst->next)
		{
			/* !NULL consists only of word ends */
			if(inst->nphones == 0)
			{
				/* Flawed */
				if(hc->xc == 0)
				{
					/* But we need a pointer for xc==0 cases */
					word_node = wtk_latset_find_word_node(ls, inst->pron, inst, n_word);
					inst->starts = word_node;
					inst->nstart = 0; /* Stops us adding node into chain twice */
				}
				continue;
			}

			/* Determine which bits of word are l and r cd */
			if(hc->xc > 0)
			{
				for(p=0; p < inst->nphones; p++)
				{
					if(wtk_latset_find_ctx(ls, inst->phones[p]->name)) { break; }
				}
				for(q=inst->nphones-1; q >= 0; q--)
				{
					if(wtk_latset_find_ctx(ls, inst->phones[q]->name)) { break; }
				}

			}
			else
			{
				p = 0;
				q = inst->nphones - 1;
			}

//			wtk_debug("p = %d, q = %d, clen = %d, end=%.*s\n", p ,q, inst->clen, inst->phones[inst->nphones-1]->name->len, inst->phones[inst->nphones-1]->name->data);
//			{
//				for(j=0; j < inst->nphones; j++)
//				{
//					printf("%.*s ", inst->phones[j]->name->len, inst->phones[j]->name->data);
//				}
//				printf("\n");
//			}

			inst->tee = 1;
			/* Make wrd-int cd phones (possibly none!) */
			_wtk_latset_create_wimodels(ls, lat, inst, p, q);

			if(ls->hc->xc == 0)
			{
				/* Word internal context only */
				wtk_debug("Word internal context only\n");
			}
			/* Cross word context */
			else if(inst->clen == 1)
			{
				/*
				 *  Single phone word means that we need to
				 *  build a complete cross-bar of contexts
				 */
				ret = wtk_latset_create_x1models(ls, lat, inst, p, q, heap);
				if(0 != ret) { goto end; }
			}
			else
			{
				/* Cross word context and more than one phone */
				ret = wtk_latset_create_xemodels(ls, lat, inst, p, q, heap);
				if(0 != ret) { goto end; }
			}

			if(inst->tee)
			{
				wtk_debug("ExpandWordNet: Pronunciation %d of %.*s is 'tee' word\n", inst->pron->pnum,
						inst->pron->outsym->len, inst->pron->outsym->data);
				ls->tee_words = 1;
			}
		}
	} /* for(i=0; i < lat->nn; i++) */

	/* Allocate NetLinks from hash table stats. Zero counters */
	for(i=0; i < ls->wn_nodes; i++)
	{
		/* Build links for each word end model */
		for(node=ls->wn_node_hash[i]; node != NULL; node=node->chain)
		{
			//wtk_debug("word[%p]: %.*s, nlinks = %d, type = %d\n", node, node->info.pron->word->name->len, node->info.pron->word->name->data, node->nlinks, node->type);
			//wtk_debug("type = %d, nlinks = %d, word = %.*s\n", node->type, node->nlinks, node->info.pron->word->name->len, node->info.pron->word->name->data);
			if(node->nlinks > 0)
			{
				node->links = (wtk_netlink_t *)wtk_heap_malloc(ls->heap, sizeof(wtk_netlink_t) * node->nlinks);
			}
			else
			{
				node->links = NULL;
			}

			node->nlinks = 0;
			node->aux = 0;

			//wtk_netnode_print(node);
		}
	}

	/* Finally put in the cross word links */
	_wtk_latset_process_cross_word_links(ls, lat, NULL);

	/* First disassemble wnHashTab and link to end nodes as necessary */
	_wtk_latset_add_initial_final(ls, lat);

	wtk_latset_add_chain(ls,lat);

	/* Count the initial/final nodes/links */
	lat->nlink=lat->initial.nlinks;
	lat->nnode=2;
	/* now reorder links and identify wd0 nodes */
	for(chain_node=lat->chain; chain_node != NULL; chain_node=chain_node->chain,lat->nnode++)
	{
		chain_node->inst = NULL;
		//wtk_debug("chain_node->type = %d\n", chain_node->type);
		chain_node->type = chain_node->type & n_nocontext;
		//wtk_debug("chain_node->type = %d\n", chain_node->type);
		lat->nlink += chain_node->nlinks;

		//_wtk_netnode_print(chain_node);

		/* Make !NULL words really NULL */
		if(chain_node->type == n_word && chain_node->info.pron != NULL
			&& ls->null_word != NULL &&	chain_node->info.pron->word == ls->null_word)
		{
//			wtk_debug("type = %d, nlinks = %d, word = %.*s\n", chain_node->type, chain_node->nlinks,
//					chain_node->info.pron->word->name->len, chain_node->info.pron->word->name->data);
			chain_node->info.pron = NULL;

		}
		/* First make n_wd0 nodes */
		if(chain_node->type & n_hmm)
		{
			for(i=0; i < chain_node->nlinks; i++)
			{
				if(_wtk_netlink_is_wd0(chain_node->links+i))
				{
					chain_node->type |= n_wd0;
					break;
				}
			}
		}

		/* Then put all n_tr0 nodes first */
		for(i=0; i < chain_node->nlinks; i++)
		{
			/* Don't need to move any initial n_tr0 links */
			if(chain_node->links[i].node->type & n_tr0) { continue; }
			/* Find if there are any n_tr0 ones to swap with */
			for(j=i+1; j < chain_node->nlinks; j++)
			{
				if(chain_node->links[j].node->type & n_tr0) { break; }
			}

			/* No, finished */
			if(j >= chain_node->nlinks) { break; }

			/* Yes, swap then carry on */
			netlink = chain_node->links[i];
			chain_node->links[i] = chain_node->links[j];
			chain_node->links[j] = netlink;
		}

		//_wtk_netnode_print(chain_node);
	}

	//wtk_debug("nnode = %d\n", lat->nnode);

end:
	for(i=0; i < lat->nn; i++)
	{
		lnode = lat->lnodes + i;
		for(inst=lnode->pron_chain; inst != NULL; inst=inst->next)
		{
			if(inst->lc) { wtk_str_hash_delete(inst->lc); }
			if(inst->rc) { wtk_str_hash_delete(inst->rc); }
		}
	}
	if(heap) { wtk_heap_delete(heap); }

	//exit(0);

	return ret;
}



