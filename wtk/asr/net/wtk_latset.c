#include "wtk_latset.h"
#define wtk_latset_new_node(ls) (wtk_netnode_t*)wtk_heap_zalloc((ls)->heap,sizeof(wtk_netnode_t))
//#define wtk_latset_new_node(ls) (wtk_netnode_t*)calloc(1,sizeof(wtk_netnode_t))

wtk_latset_t* wtk_latset_new(wtk_net_cfg_t* cfg,wtk_dict_t *d,wtk_hmmset_t *h,wtk_dict_word_find_f dwf,void *dwf_data)
{
	wtk_latset_t *l;

	l=(wtk_latset_t*)malloc(sizeof(*l));
	wtk_latset_init(l,cfg,d,h,dwf,dwf_data);
	return l;
}

int wtk_latset_delete(wtk_latset_t *l)
{
	wtk_latset_clean(l);
	free(l);
	return 0;
}

int wtk_latset_init(wtk_latset_t *ls,wtk_net_cfg_t* cfg,wtk_dict_t *dict,wtk_hmmset_t *hl,wtk_dict_word_find_f dwf,void *dwf_data)
{
	memset(ls,0,sizeof(*ls));
	ls->cfg=cfg;
	ls->hc=0;
	ls->heap=wtk_heap_new(4096);
	ls->dict=dict;ls->hl=hl;
	if(hl)
	{
		//ls->label=dict->label;
		ls->label=hl->label;
	}else
	{
		ls->label=0;
	}
	ls->wn_nodes=5701;
	ls->wn_node_hash=wtk_calloc(ls->wn_nodes,sizeof(wtk_netnode_t*));
	ls->buf=wtk_strbuf_new(80,1);
	ls->tmp_array=wtk_array_new_h(ls->heap,1,sizeof(wtk_netnode_t*));
	if(dwf)
	{
		ls->dwf=dwf;
		ls->dwf_data=dwf_data;
	}else if(ls->dict)
	{
		ls->dwf=(wtk_dict_word_find_f)wtk_dict_find_word;
		ls->dwf_data=ls->dict;
	}
	return 0;
}

int wtk_latset_reset(wtk_latset_t *ls)
{
	wtk_heap_reset(ls->heap);
	ls->tmp_array=wtk_array_new_h(ls->heap,1,sizeof(wtk_netnode_t*));
	memset(ls->wn_node_hash,0,sizeof(wtk_netnode_t*)*ls->wn_nodes);
	return 0;
}

int wtk_latset_clean(wtk_latset_t *ls)
{
	if(ls->hc)
	{
		wtk_hmmset_ctx_delete(ls->hc);
	}
	if(ls->main)
	{
		wtk_lat_clean(ls->main);
	}
	free(ls->wn_node_hash);
	wtk_strbuf_delete(ls->buf);
	wtk_heap_delete(ls->heap);
	return 0;
}

wtk_lat_t* wtk_latset_new_lat(wtk_latset_t *ls)
{
	wtk_lat_t *lat;

	lat=wtk_lat_new_h(ls->heap);//,ls->cfg->frame_dur);
	wtk_latset_add_lat(ls,lat);
	return lat;
}

int wtk_lat_cmp(wtk_string_t* s,wtk_lat_t *l)
{
	if(l->name)
	{
		return wtk_string_cmp(l->name,s->data,s->len);
	}else
	{
		return -1;
	}
}

wtk_lat_t* wtk_latset_find_lat(wtk_latset_t *ls,char* s,int ns)
{
	wtk_string_t name;

	wtk_string_set(&name,s,ns);
	return (wtk_lat_t*)wtk_queue_find(&(ls->lat_queue),offsetof(wtk_lat_t,set_n),(wtk_cmp_handler_t)wtk_lat_cmp,&name);
}

wtk_proninst_t* wtk_latset_new_proninst(wtk_latset_t *ls,wtk_dict_pron_t *pron)
{
	wtk_proninst_t* inst;

	inst=(wtk_proninst_t*)wtk_heap_zalloc(ls->heap,sizeof(*inst));
	inst->pron=pron;
	inst->nphones=pron->nPhones;
	inst->phones=pron->pPhones;
	inst->tee=1;
	return inst;
}

wtk_netnode_t* wtk_latset_find_word_node(wtk_latset_t *ls,wtk_dict_pron_t *pron,
		wtk_proninst_t *inst,wtk_netnode_type_t type)
{
	static int size=sizeof(void*)*3;
	union
	{
		void* ptrs[3];
		unsigned char chars[sizeof(void*)*3];
	}un;
	wtk_netnode_t *node;
	unsigned int hash,i;


	un.ptrs[0]=pron;
	un.ptrs[1]=inst;
	un.ptrs[2]=(void*)((long)type);
	hash=0;
	for(i=0;i<size;++i)
	{
		hash=((hash<<8)+un.chars[i])%(ls->wn_nodes);
	}
	for(node=ls->wn_node_hash[hash];node;node=node->chain)
	{
		if(node->info.pron==pron && node->inst==inst && node->type==type)
		{
			//printf(" found");
			break;
		}
	}
	if(!node)
	{
		//node=(wtk_netnode_t*)wtk_heap_malloc(ls->heap,sizeof(*node));
		node=wtk_latset_new_node(ls);
		node->info.pron=pron;
		node->type=type;
		node->nlinks=0;
		node->links=0;
		node->inst=inst;
		node->chain=ls->wn_node_hash[hash];
		ls->wn_node_hash[hash]=node;
	}
	//printf("\n");
	//getchar();
	return node;
}

/**
 *	* if factor_lm is true, set the max pre arc like to the node, and reset arc like to make the path like not change.
 *	* linked all pron inst to lnode pron_chain.
 */
int wtk_latset_init_pron_inst(wtk_latset_t *ls,wtk_lat_t *lat,wtk_hmmset_ctx_t* hc)
{
	wtk_larc_t *arc;
	wtk_lnode_t *node;
	wtk_dict_pron_t *pron;
	wtk_proninst_t *inst;
	float fct;
	int i,j,factor_lm;
	wtk_dict_word_t *word;
	wtk_netnode_t *word_node;
	int nNull;

	factor_lm=ls->cfg->factor_lm;
	//set LM likelihood for nodes and arcs.
	for(i=0;i<lat->nn;++i)
	{
		node=lat->lnodes+i;
		if(factor_lm && node->pred)
		{
			//set node score to the max like of pre arc
			for(arc=node->pred,fct=LZERO;arc;arc=arc->parc)
			{
				if(arc->lmlike>fct)
				{
					fct=arc->lmlike;
				}
			}
		}else
		{
			fct=0;
		}
		node->score=fct;
	}
	if(factor_lm)
	{
		//for node score is set, sub it from  pre arc,make the path like equal.
		for(i=0;i<lat->na;++i)
		{
			arc=lat->larcs+i;
			arc->lmlike -= arc->end->score;
		}
	}
	for(i=0,nNull=0;i<lat->nn;++i)
	{
		node=lat->lnodes+i;
		word=node->info.word;
		//attach pron inst to node pron chain.
		for(j=0,pron=word->pron_list;j<word->npron;++j,pron=pron->next)
		{
			inst=wtk_latset_new_proninst(ls,pron);
			inst->ln=node;
			inst->next=node->pron_chain;
			node->pron_chain=inst;
			if(inst->nphones<=0)
			{
				inst->fct=0;
				++nNull;
			}else
			{
				inst->fct=node->score/inst->nphones;
			}
			if(!node->foll)
			{
				word_node=wtk_latset_find_word_node(ls,pron,inst,n_word);
				word_node->nlinks=0;
				//wtk_debug("foll: %*.*s\n",pron->outsym->len,pron->outsym->len,pron->outsym->data);
			}
		}
	}
	return nNull;
}

/**
 * update word node links.
 */
void wtk_latset_add_word_links(wtk_latset_t *ls,wtk_lat_t *lat)
{
	wtk_netnode_t *node;
	int i;

	for(i=0;i<ls->wn_nodes;++i)
	{
		for(node=ls->wn_node_hash[i];node;node=node->chain)
		{
			if(node->nlinks>0)
			{
				node->links=(wtk_netlink_t*)wtk_heap_malloc(ls->heap,sizeof(wtk_netlink_t)*node->nlinks);
			}else
			{
				node->links=0;
			}
			node->nlinks=0;
		}
	}
}

/**
 *            -----------> B
 *    ( A )   -----------> C
 *            -----------> D
 *  calc the success nodes, and update word netnode's nlinks.
 */
void wtk_latset_update_word_nlink(wtk_latset_t *ls,wtk_lat_t *lat)
{
	wtk_proninst_t *linst,*rinst;
	wtk_larc_t *arc;
	wtk_netnode_t *word_node;
	int i,n;

	for(i=0;i<lat->na;++i)
	{
		arc=lat->larcs+i;
		for(n=0,rinst=arc->end->pron_chain;rinst;rinst=rinst->next,++n);
		for(linst=arc->start->pron_chain;linst;linst=linst->next)
		{
			word_node=wtk_latset_find_word_node(ls,linst->pron,linst,n_word);
			word_node->nlinks+=n;
		}
	}
}

void wtk_latset_link_word(wtk_latset_t *ls,wtk_lat_t *lat)
{
	wtk_proninst_t *linst,*rinst;
	wtk_larc_t *arc;
	wtk_netnode_t *word_node,*n;
	wtk_netlink_t *link;
	int i;

	for(i=0;i<lat->na;++i)
	{
		arc=lat->larcs+i;
		for(linst=arc->start->pron_chain;linst;linst=linst->next)
		{
			word_node=wtk_latset_find_word_node(ls,linst->pron,linst,n_word);
			for(rinst=arc->end->pron_chain;rinst;rinst=rinst->next)
			{
				//wtk_netnode_print(rinst->starts);
				//wtk_debug("link=%d,t=%d\n",word_node->nlinks,word_node->t);
				//wtk_netnode_print(word_node);
				n=rinst->starts;
				if((n->type & n_tr0))
				{
					//make n_tr0 at the head link;
					if((word_node->first_none_tr0_link_index>=0) && (word_node->first_none_tr0_link_index<word_node->nlinks))
					{
						link=&(word_node->links[word_node->nlinks]);
						*link=word_node->links[word_node->first_none_tr0_link_index];
						link=&(word_node->links[word_node->first_none_tr0_link_index]);
						++word_node->first_none_tr0_link_index;
					}else
					{
						link=&(word_node->links[word_node->nlinks]);
					}
				}else
				{
					link=&(word_node->links[word_node->nlinks]);
					if(word_node->first_none_tr0_link_index<0)
					{
						word_node->first_none_tr0_link_index=word_node->nlinks;
					}
				}
				link->node=n;
				link->like=arc->lmlike;
				++word_node->nlinks;
			}
		}
	}
}

wtk_string_t* wtk_latset_find_lctx(wtk_latset_t *ls,wtk_proninst_t *inst,int pos,wtk_string_t *bk)
{
	wtk_hmmset_ctx_t *hc=(ls->hc);
	wtk_string_t *phn=0;
	wtk_string_t *n;
	int i;

	for(i=pos-1;i>=0;--i)
	{
		n=inst->phones[i]->name;
		//wtk_debug("[%.*s],bk=%p\n",inst->phones[i]->name->len,inst->phones[i]->name->data,bk);
		phn=wtk_hmmset_ctx_get_hci_ctx(hc,n);
		if(phn){break;}
		if(ls->cfg->sp_word_boundary && hc->cfs_hash)
		{
			if(wtk_str_hash_find(hc->cfs_hash,n->data,n->len))
			{
				phn=0;
				break;
			}
		}
	}
	if(!phn)
	{
		//phn=wtk_hmmset_ctx_get_hci_ctx(hc,inst->phones[lc]->name);
		phn=bk;
	}
	return phn;
}

wtk_string_t* wtk_latset_find_rctx(wtk_latset_t *ls,wtk_proninst_t *inst,int pos,wtk_string_t *bk)
{
	wtk_hmmset_ctx_t *hc=(ls->hc);
	wtk_string_t *phn=0;
	wtk_string_t *n;
	int i;

	for(i=pos+1;i<inst->nphones;++i)
	{
		n=inst->phones[i]->name;
		phn=wtk_hmmset_ctx_get_hci_ctx(hc,n);
		if(phn){break;}
		if(ls->cfg->sp_word_boundary && hc->cfs_hash)
		{
			if(wtk_str_hash_find(hc->cfs_hash,n->data,n->len))
			{
				phn=0;
				break;
			}
		}
	}
	if(!phn)
	{
		//phn=wtk_hmmset_ctx_get_hci_ctx(hc,inst->phones[rc]->name);
		phn=bk;
	}
	return phn;
}

wtk_hmm_t* wtk_latset_get_hci_model(wtk_latset_t *ls,wtk_string_t *ln,wtk_string_t* phn,wtk_string_t *rn)
{
	wtk_hmmset_ctx_t* hc=(ls->hc);
	wtk_net_cfg_t *cfg=ls->cfg;
	wtk_string_t *name=0;
	wtk_strbuf_t *buf;
	wtk_hmm_t* hmm;
	wtk_name_t *n;
	/*
	static int i=0;

	wtk_debug("v[%d]=",++i);
	if(ln)
	{
		printf("ln: %.*s ",ln->len,ln->data);
	}else
	{
		printf("ln: nil ");
	}
	printf("phn: %.*s ",phn->len,phn->data);
	if(rn)
	{
		printf("rn: %.*s ",rn->len,rn->data);
	}else
	{
		printf("rn: nil ");
	}
	printf("\n");
	if(i==24883)
	{
		exit(0);
	}
	*/
	//wtk_debug("v[%d]=%p,%.*s,%p\n",++i,ln,phn->len,phn->data,rn);
#ifdef DEBUG_HCI
	if(ln)
	{
		wtk_debug("ln:  %.*s\n",ln->len,ln->data);
	}else
	{
		wtk_debug("ln: NULL\n");
	}
	wtk_debug("phn: %.*s\n",phn->len,phn->data);
	if(rn)
	{
		wtk_debug("rn:  %.*s\n",rn->len,rn->data);
	}else
	{
		wtk_debug("rn: NULL\n");
	}
#endif
	if((!cfg->allow_ctx_exp)||(!ln && !rn) || wtk_hmmset_ctx_is_hic_ind(hc,phn))
	{
		//wtk_debug("name: %.*s\n",phn->len,phn->data);
		name=phn;
	}else
	{
		buf=ls->buf;
		wtk_strbuf_reset(buf);
		n=0;
		if((!ln || !hc->s_left || cfg->force_right_biphones) && rn && !cfg->force_left_biphones)
		{
			wtk_strbuf_push(buf,phn->data,phn->len);
			wtk_strbuf_push_s(buf,"+");
			wtk_strbuf_push(buf,rn->data,rn->len);
			n=wtk_label_find(ls->label,buf->data,buf->pos,0);
			//wtk_debug("name [%.*s]=%p\n",buf->pos,buf->data,n);
			if(n){name=n->name;}
			//name=wtk_label_find(ls->label,buf->data,buf->pos,0)->name;
			//n=wtk_label_find(ls->label,buf->data,buf->pos,0);
			//name=n?n->name:0;
		}else if((!rn||!hc->s_right||cfg->force_left_biphones) && ln && !cfg->force_right_biphones)
		{
			wtk_strbuf_push(buf,ln->data,ln->len);
			wtk_strbuf_push_s(buf,"-");
			wtk_strbuf_push(buf,phn->data,phn->len);
			n=wtk_label_find(ls->label,buf->data,buf->pos,0);
			//wtk_debug("name [%.*s]=%p\n",buf->pos,buf->data,n);
			if(n){name=n->name;}
			//name=wtk_label_find(ls->label,buf->data,buf->pos,0)->name;
			//n=wtk_label_find(ls->label,buf->data,buf->pos,0);
			//name=n?n->name:0;
		}else if(!cfg->force_left_biphones && !cfg->force_right_biphones)
		{
			wtk_strbuf_push(buf,ln->data,ln->len);
			wtk_strbuf_push_s(buf,"-");
			wtk_strbuf_push(buf,phn->data,phn->len);
			wtk_strbuf_push_s(buf,"+");
			wtk_strbuf_push(buf,rn->data,rn->len);
			n=wtk_label_find(ls->label,buf->data,buf->pos,0);
			//wtk_debug("name: [%.*s]=%p\n",buf->pos,buf->data,n);
			if(n){name=n->name;}
			//name=wtk_label_find(ls->label,buf->data,buf->pos,0)->name;
			//n=wtk_label_find(ls->label,buf->data,buf->pos,0);
			//name=n?n->name:0;
		}
		else
		{
			//wtk_debug("name: %.*s\n",phn->len,phn->data);
			name=phn;
		}
	}
	//if(!name){hmm=0;goto end;}
	/*
	wtk_debug("ln=%p,rn=%p\n",ln,rn);
	if(phn)
	{
		wtk_debug("phn: %.*s\n",phn->len,phn->data);
	}else
	{
		wtk_debug("phn=nil\n");
	}
	if(name)
	{
		wtk_debug("name=%.*s\n",name->len,name->data);
	}else
	{
		wtk_debug("name=nil\n");
	}
	*/
	if(name)
	{
		hmm=wtk_hmmset_find_hmm(ls->hl,name->data,name->len);
	}else
	{
		hmm=0;
	}
	//wtk_debug("%*.*s:%p\n",name->len,name->len,name->data,hmm);
	if(!hmm && (((!ln &&!rn)|| !cfg->force_ctx_exp)||
			(!ln || !cfg->force_left_biphones)||(!rn ||!cfg->force_right_biphones)))
	{
		hmm=wtk_hmmset_find_hmm(ls->hl,phn->data,phn->len);
	}
//end:
	return hmm;
}

wtk_netnode_t *wtk_latset_new_netnode(wtk_latset_t *ls,wtk_hmm_t *hmm,int nlinks)
{
	wtk_netnode_t *node;

	node=wtk_latset_new_node(ls);
	node->type = (hmm->transP[1][hmm->num_state] > LSMALL ? (n_hmm | n_tr0): n_hmm);
	node->info.hmm=hmm;
	node->nlinks=nlinks;
	node->first_none_tr0_link_index=-1;
	//node->chain=0;
	if(nlinks>0)
	{
		node->links=(wtk_netlink_t*)wtk_heap_malloc(ls->heap,sizeof(wtk_netlink_t)*nlinks);
	}
	return node;
}

/**
 * attach in-word phone to inst->chain
 */
void wtk_latset_create_wimodels(wtk_latset_t *ls,wtk_proninst_t *inst,int p,int q)
{
	wtk_netnode_t *node;
	wtk_hmm_t *hmm;
	wtk_string_t *ln,*rn;
	int j;

	for(j=q-1;j>p;--j)
	{
		ln=wtk_latset_find_lctx(ls,inst,j,0);
		rn=wtk_latset_find_rctx(ls,inst,j,0);
		//wtk_debug("%.*s\n",inst->phones[j]->name->len,inst->phones[j]->name->data);
		hmm=wtk_latset_get_hci_model(ls,ln,inst->phones[j]->name,rn);
		//if hmm is not found,there is some problem in the resource.
		if(!hmm){continue;}
		//wtk_hmm_print(hmm);
		if(inst->tee && hmm->transP[1][hmm->num_state]<LSMALL)
		{
			inst->tee=0;
		}
		node=wtk_latset_new_netnode(ls,hmm,(inst->chain?1:0));
		if(inst->chain)
		{
			node->links[0].node=inst->chain;
			node->links[0].like=inst->fct;
		}
		node->chain=inst->chain;
		inst->chain=node;
	}
}

wtk_hmm_t* wtk_latset_get_hci_model_t(wtk_latset_t *ls,wtk_proninst_t *inst,wtk_string_t *ln,wtk_string_t* phn,wtk_string_t *rn)
{
	wtk_hmm_t *hmm;

	hmm=wtk_latset_get_hci_model(ls,ln,phn,rn);
	if(hmm && inst->tee && hmm->transP[1][hmm->num_state]<LSMALL)
	{
		inst->tee=0;
	}
	return hmm;
}


void wtk_latset_create_iemodels(wtk_latset_t* ls,wtk_proninst_t *inst,int p,int q)
{
	wtk_netnode_t *node,*word_node;
	wtk_hmm_t *hmm;
	wtk_string_t *ln,*rn;

	//wtk_debug("word: %s\n",gbk_to_utf8_3(inst->pron->outsym->data,inst->pron->outsym->len));
	if(q==p)
	{
		hmm=wtk_latset_get_hci_model_t(ls,inst,0,inst->phones[0]->name,0);
		if(!hmm)
		{
			wtk_proninst_print(inst);
			wtk_debug("hmm[%.*s] not found.\n",inst->phones[0]->name->len,inst->phones[0]->name->data);
			return;
		}
		word_node=wtk_latset_find_word_node(ls,inst->pron,inst,n_word);
		node=wtk_latset_new_netnode(ls,hmm,1);
		node->links[0].node=word_node;
		node->links[0].like=inst->fct;
		inst->starts=node;
		//wtk_netnode_print(node);
		inst->nstart=1;
#ifdef DEBUG_IE
		wtk_debug("pron=%*.*s,nlinks=%d,fct=%f\n",inst->pron->outsym->len,inst->pron->outsym->len,
				inst->pron->outsym->data,word_node->nlinks,inst->fct);
#endif
	}else
	{
		//attach end node.
		ln=wtk_latset_find_lctx(ls,inst,q,0);
		hmm=wtk_latset_get_hci_model_t(ls,inst,ln,inst->phones[q]->name,0);
		if(!hmm)
		{
			wtk_debug("hmm[%.*s] not found.\n",inst->phones[q]->name->len,inst->phones[q]->name->data);
			return;
		}
		word_node=wtk_latset_find_word_node(ls,inst->pron,inst,n_word);
		node=wtk_latset_new_netnode(ls,hmm,1);
		node->links[0].node=word_node;
		node->links[0].like=inst->fct;
		inst->ends=node;
		inst->nend=1;

		//attach start node.
		rn=wtk_latset_find_rctx(ls,inst,p,0);
		hmm=wtk_latset_get_hci_model_t(ls,inst,0,inst->phones[p]->name,rn);
		if(!hmm)
		{
			wtk_debug("hmm[%.*s] not found.\n",inst->phones[p]->name->len,inst->phones[p]->name->data);
			return;
		}
		node=wtk_latset_new_netnode(ls,hmm,1);
		node->links[0].node=inst->chain?inst->chain:inst->ends;
		node->links[0].like=inst->fct;
		inst->starts=node;
		//wtk_netnode_print(inst->starts);
		inst->nstart=1;
		if(inst->chain)
		{
			for(node=inst->chain;node->chain;node=node->chain);
			node->nlinks=1;
			node->links=(wtk_netlink_t*)wtk_heap_malloc(ls->heap,sizeof(wtk_netlink_t));
			node->links[0].node=inst->ends;
			node->links[0].like=inst->fct;
		}
	}
}

void wtk_latset_init_hmm(wtk_latset_t *ls,wtk_lat_t *lat)
{
	wtk_lnode_t *node;
	wtk_netnode_t *word_node;
	wtk_proninst_t *inst;
	int i,q;

	for(i=0;i<lat->nn;++i)
	{
		node=lat->lnodes+i;

		/*
		if(node->type==WTK_LNODE_WORD)
		{
			if(wtk_string_cmp_s(node->info.word->name,"doctor")==0)
			{
				wtk_debug("==========================\n");
			}
		}
		*/
		for(inst=node->pron_chain;inst;inst=inst->next)
		{
			/*
			if(node->type==WTK_LNODE_WORD)
			{
				if(wtk_string_cmp_s(node->info.word->name,"doctor")==0)
				{
					wtk_debug("----------------------------------\n");
				}
			}*/
			if(inst->nphones==0)
			{
				word_node=wtk_latset_find_word_node(ls,inst->pron,inst,n_word);
				inst->starts=word_node;
				inst->nstart=0;
				continue;
			}
			//wtk_lnode_print(node,0);
			q=inst->nphones-1;
			wtk_latset_create_wimodels(ls,inst,0,q);
			/*
			if(node->type==WTK_LNODE_WORD)
			{
				if(wtk_string_cmp_s(node->info.word->name,"doctor")==0)
				{
					wtk_debug("+++++++++++++++++++++++++++++++++++\n");
				}
			}*/
			wtk_latset_create_iemodels(ls,inst,0,q);
		}
		/*
		if(node->type==WTK_LNODE_WORD)
		{
			if(wtk_string_cmp_s(node->info.word->name,"doctor")==0)
			{
				exit(0);
			}
		}
		*/
	}
}

int wtk_latset_add_initial_final(wtk_latset_t *ls,wtk_lat_t *lat)
{
	wtk_proninst_t *inst;
	wtk_lnode_t *node,*node2;
	int i,ninit;
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
			n->links[n->nlinks].node=inst->starts;
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
				n2=wtk_latset_find_word_node(ls,inst->pron,inst,n_word);
				n2->nlinks=1;
				n2->links=(wtk_netlink_t*)wtk_heap_malloc(ls->heap,sizeof(wtk_netlink_t));
				n2->links[0].node=n;
				n2->links[0].like=0;
				//printf("%*.*s\n",n2->info.pron->outsym->len,n2->info.pron->outsym->len,n2->info.pron->outsym->data);
			}
		}
	}
	return 0;
}

void wtk_latset_add_node_chain(wtk_latset_t *ls,wtk_lat_t *lat,wtk_netnode_t *hn)
{
	wtk_netnode_t *n=hn;

	while(n->chain)
	{
		n=n->chain;
	}
	//wtk_netnode_print(n);
	n->chain=lat->chain;
	lat->chain=hn;
}


void wtk_latset_add_chain(wtk_latset_t *ls,wtk_lat_t *lat)
{
	wtk_proninst_t *inst;
	wtk_lnode_t *n;
	int i;

	lat->chain=0;
	for(i=0;i<ls->wn_nodes;++i)
	{
		if(ls->wn_node_hash[i])
		{
			wtk_latset_add_node_chain(ls,lat,ls->wn_node_hash[i]);
		}
	}
	for(i=0;i<lat->nn;++i)
	{
		n=lat->lnodes+i;
		for(inst=n->pron_chain;inst;inst=inst->next)
		{
			//wtk_proninst_print(inst);
			if(inst->nstart>0)
			{
				//wtk_netnode_print(inst->starts);
				wtk_latset_add_node_chain(ls,lat,inst->starts);
			}
			if(inst->chain)
			{
				wtk_latset_add_node_chain(ls,lat,inst->chain);
			}
			if(inst->ends)
			{
				wtk_latset_add_node_chain(ls,lat,inst->ends);
			}
		}
	}
}

int wtk_netlink_is_wd0(wtk_netlink_t *link)
{
	wtk_netnode_t *node;
	int ret,i;

	node=link->node;
	if(node->type==n_word){ret=1;goto end;}
	if(node->type & n_tr0)
	{
		for(i=0;i<node->nlinks;++i)
		{
			if(wtk_netlink_is_wd0(node->links+i))
			{
				ret=1;goto end;
			}
		}
	}
	ret=0;
end:
	return ret;
}

int wtk_latset_reorder_links(wtk_latset_t *ls,wtk_lat_t *lat)
{
	wtk_netnode_t *node;
	int i;

	lat->nlink=lat->initial.nlinks;
	lat->nnode=2;
	for(node=lat->chain;node;node=node->chain,++lat->nnode)
	{
		//wtk_netnode_print(node);
		lat->nlink+=node->nlinks;
		//First make n_wd0 nodes.
		if(node->type & n_hmm)
		{
			for(i=0;i<node->nlinks;++i)
			{
				if(wtk_netlink_is_wd0(node->links+i))
				{
					node->type |=n_wd0;
				}
			}
		}else
		{
			//remove !NULL pron. add for temp debug. must rewrite.
			if(node->info.pron==ls->dict->null_pron)
			{
				node->info.pron=0;
			}
		}
#ifdef LATSET_REORDER
		if(node->nlinks>1)
		{
			//this is done in link word
			wtk_netlink_t link;
			int j;
			//put all n_tr0 node first. must rewrite, for speed must depends on n_tr0 node counts.
			//wtk_netnode_print2(node);
			for(i=0;i<node->nlinks;++i)
			{
				if(node->links[i].node->type & n_tr0){continue;}
				//wtk_debug("%d/%d\n",i,node->nlinks);
				for(j=i+1;j<node->nlinks;++j)
				{
					if(node->links[j].node->type & n_tr0)
					{
						link=node->links[i];
						node->links[i]=node->links[j];
						node->links[j]=link;
						break;
					}
				}
			}
		}
#endif
	}
	return 0;
}

int wtk_latset_pron_nodes(wtk_latset_t *ls)
{
	wtk_netnode_t *n;
	int i,count;

	count=0;
	for(i=0;i<ls->wn_nodes;++i)
	{
		for(n=ls->wn_node_hash[i];n;n=n->chain,++count);
	}
	return count;
}

/**
 * algorithm:
 * a) if use hmm context dependents, go through hmmset,and found all have "+","-" hmm, set up context.
 * b) if LM_FACTOR is true, recalc lmlike to arc and node(which will divde into link between phone net node
 *    and word node); and attach pron list to every word lattice node.
 * c) update phone(hmm) netnode to every pron inst.
 * d) link all pron inst, which means all word is linked,and word have phone netnodes.
 * e) found start(which have no pre),end node(which have no successor).
 * f) link all netnode.
 * h) reorder links of netnode, make tr_0 before other.
 */
int wtk_latset_expand_lat(wtk_latset_t *ls,wtk_lat_t *lat)
{
	wtk_net_cfg_t *cfg=ls->cfg;
	wtk_hmmset_ctx_t *hc=(ls->hc);
	int ret;

	/* surport cross word expand */
	if(ls->cfg->allow_xwrd_exp)
	{
		ret = wtk_latset_expand_lat2(ls, lat);

		goto end;
	}

	//wtk_lat_print2(lat);
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
		ls->hc=wtk_hmmset_ctx_new(ls->hl,0);
		//ret=wtk_hmmset_ctx_init(hc,ls->hl,0);
	}
	ret=ls->hc?0:-1;
	if(ret!=0){goto end;}
	//attach pron inst list to word node.
	wtk_latset_init_pron_inst(ls,lat,hc);
	//calc nlink for every word net node.
	wtk_latset_update_word_nlink(ls,lat);
	//expand phone in word
	wtk_latset_init_hmm(ls,lat);
	wtk_latset_add_word_links(ls,lat);
	wtk_latset_link_word(ls,lat);
	wtk_latset_add_initial_final(ls,lat);
	wtk_latset_add_chain(ls,lat);
	wtk_latset_reorder_links(ls,lat);
end:
	return ret;
}

int wtk_latset_expand(wtk_latset_t *ls)
{
	//current not support sub-lattice.
	return wtk_latset_expand_lat(ls,ls->main);
}

void wtk_latset_print_netnode(wtk_latset_t *ls,wtk_netnode_t *node)
{
	printf("Node[%05ld] ",(((unsigned long)node)/sizeof(wtk_netnode_t))%100000);
	if(node->type & n_hmm)
	{
		printf("{%*.*s}\n",node->info.hmm->name->len,node->info.hmm->name->len,node->info.hmm->name->data);
	}else if(node->type==n_word)
	{
		if(node->info.pron)
		{
			if(wtk_string_cmp_s(node->info.pron->word->name,"!NULL")==0)
			{
				printf("NULL\n");
			}else
			{
				printf("%*.*s\n",node->info.pron->word->name->len,node->info.pron->word->name->len,node->info.pron->word->name->data);
			}
		}else
		{
			printf("NULL\n");
		}
	}else
	{
		printf("{%d}\n",node->type);
	}
}

void wtk_latset_print_links(wtk_latset_t *ls,wtk_netnode_t* n)
{
	int i;

	for(i=0;i<n->nlinks;++i)
	{
		printf("    %-2d: -> [%05ld] == %7.3f\n", i, (((unsigned long) n->links[i].node)/ sizeof(wtk_netnode_t) % 100000), n->links[i].like);
	}
}

void wtk_latset_print_network(wtk_latset_t *ls,wtk_lat_t *lat)
{
	wtk_netnode_t *node=lat->chain;

	while(node)
	{
		wtk_latset_print_netnode(ls,node);
		wtk_latset_print_links(ls,node);
		node=node->chain;
	}
}

void wtk_latset_print_words(wtk_latset_t *ls,wtk_lat_t *lat)
{
	wtk_lnode_t *lnode;
	wtk_proninst_t *inst;
	wtk_dict_pron_t *pron;
	wtk_netnode_t *node;
	wtk_hmm_t *hmm;
	int i;

	for(i=0;i<lat->nn;++i)
	{
		lnode=lat->lnodes+i;
		for(inst=lnode->pron_chain;inst;inst=inst->next)
		{
			pron=inst->pron;
			if(pron->outsym!=pron->word->name)
			{
				printf("[%*.*s]\n",pron->word->name->len,pron->word->name->len,pron->word->name->data);
			}else
			{
				printf("%*.*s\n",pron->outsym->len,pron->outsym->len,pron->outsym->data);
			}
			for(node=inst->starts;(node->type & n_nocontext)!=n_word;node=node->links[0].node)
			{
				hmm=node->info.hmm;
				if(node->type & n_tr0)
				{
					printf(" (%*.*s)",hmm->name->len,hmm->name->len,hmm->name->data);
				}else
				{
					printf(" %*.*s",hmm->name->len,hmm->name->len,hmm->name->data);
				}
			}
			printf(" ==> %d\n",node->nlinks);
		}
	}
}
