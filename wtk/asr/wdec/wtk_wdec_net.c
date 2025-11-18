#include "wtk_wdec_net.h" 
#include "ctype.h"

wtk_wdec_node_t* wtk_wdec_net_new_node(wtk_wdec_net_t *net)
{
	wtk_wdec_node_t *node;

	node=(wtk_wdec_node_t*)wtk_heap_malloc(net->heap,sizeof(wtk_wdec_node_t));
	wtk_queue3_init(&(node->input_q));
	wtk_queue3_init(&(node->output_q));
	node->word=NULL;
	return node;
}

void wtk_wdec_node_print_nxt2(wtk_wdec_node_t *node,wtk_strbuf_t *buf)
{
	wtk_wdec_arc_t *arc;
	wtk_queue_node_t *qn;
	int pos;

	if(node->word)
	{
		wtk_strbuf_push(buf,node->word->name->data,node->word->name->len);
		wtk_strbuf_push_s(buf," ");
	}
	if(node->output_q.len==0)
	{
		wtk_debug("[%.*s]\n",buf->pos,buf->data);
		return;
	}
	for(qn=node->output_q.pop;qn;qn=qn->next)
	{
		arc=data_offset2(qn,wtk_wdec_arc_t,output_n);
		pos=buf->pos;
		//wtk_debug("node=%p i=%d arc=%p\n",node,i,arc);
		wtk_strbuf_push(buf,arc->v.phn->name->data,arc->v.phn->name->len);
		wtk_strbuf_push_s(buf," ");
		wtk_wdec_node_print_nxt2(arc->nxt,buf);
		buf->pos=pos;
	}
}

void wtk_wdec_node_print_nxt(wtk_wdec_node_t *node)
{
	wtk_strbuf_t *buf;

	wtk_debug("============  next ===============\n");
	buf=wtk_strbuf_new(1024,1);
	wtk_wdec_node_print_nxt2(node,buf);
	wtk_strbuf_delete(buf);
}

void wtk_wdec_node_print_pre2(wtk_wdec_node_t *node,wtk_strbuf_t *buf)
{
	wtk_wdec_arc_t *arc;
	wtk_queue_node_t *qn;
	int pos;

	if(node->word)
	{
		wtk_strbuf_push(buf,node->word->name->data,node->word->name->len);
		wtk_strbuf_push_s(buf," ");
	}
	if(node->input_q.len==0)
	{
		wtk_debug("[%.*s]\n",buf->pos,buf->data);
		return;
	}
	for(qn=node->input_q.pop;qn;qn=qn->next)
	{
		arc=data_offset2(qn,wtk_wdec_arc_t,input_n);
		pos=buf->pos;
		//wtk_debug("node=%p i=%d arc=%p\n",node,i,arc);
		wtk_strbuf_push(buf,arc->v.phn->name->data,arc->v.phn->name->len);
		wtk_strbuf_push_s(buf," ");
		wtk_wdec_node_print_pre2(arc->pre,buf);
		buf->pos=pos;
	}
}

void wtk_wdec_node_print_pre(wtk_wdec_node_t *node)
{
	wtk_strbuf_t *buf;

	wtk_debug("============  prev ==================\n");
	buf=wtk_strbuf_new(1024,1);
	wtk_wdec_node_print_pre2(node,buf);
	wtk_strbuf_delete(buf);
}

wtk_wdec_arc_t* wtk_wdec_net_new_arc(wtk_wdec_net_t *net,wtk_wdec_node_t *pre,wtk_wdec_node_t *nxt)
{
	wtk_wdec_arc_t *arc;

	arc=(wtk_wdec_arc_t*)wtk_heap_malloc(net->heap,sizeof(wtk_wdec_arc_t));
	arc->pre=pre;
	arc->nxt=nxt;
	arc->v.hmm=NULL;
	arc->label=-1;
	if(pre)
	{
		wtk_queue3_push(&(pre->output_q),&(arc->output_n));
	}
	if(nxt)
	{
		wtk_queue3_push(&(nxt->input_q),&(arc->input_n));
	}
	wtk_queue3_init(&(arc->inst_q));
	return arc;
}


void wtk_wdec_net_add_cross_phn(wtk_wdec_net_t *net,wtk_wdec_node_t *phn_pre,wtk_string_t *pre,wtk_wdec_arc_t *arc,wtk_strbuf_t *buf)
{
	wtk_queue_node_t *qn,*qn2;
	wtk_wdec_arc_t *nxt,*nxt2;
	wtk_wdec_node_t *ne;
	wtk_hmm_t *hmm;
	int j,b;
	wtk_string_t **phns;
	int nphn;
	int label =arc->label;
	//wtk_debug("[%.*s]\n",arc->v.phn->name->len,arc->v.phn->name->data);
	if(pre==NULL)
	{
		phns=(wtk_string_t**)(net->cfg->phn->slot);
		nphn=net->cfg->phn->nslot;
		//wtk_debug("output=%d\n",phn_pre->output_q.len);
		for(qn=arc->nxt->output_q.pop;qn;qn=qn->next)
		{
			nxt=data_offset2(qn,wtk_wdec_arc_t,output_n);
			ne=wtk_wdec_net_new_node(net);
			for(j=0;j<nphn;++j)
			{
				wtk_strbuf_reset(buf);
				wtk_strbuf_push_f(buf,"%.*s-%.*s+%.*s",phns[j]->len,phns[j]->data,arc->v.phn->name->len,arc->v.phn->name->data,nxt->v.phn->name->len,nxt->v.phn->name->data);

				//wtk_debug("%.*s\n",buf->pos,buf->data);
				hmm=wtk_hmmset_find_hmm(net->cfg->hmmset.hmmset,buf->data,buf->pos);
				//wtk_debug("%.*s\n",hmm->name->len,hmm->name->data);
				if(hmm)
				{
					b=0;
					for(qn2=phn_pre->output_q.pop;qn2;qn2=qn2->next)
					{
						nxt2=data_offset2(qn2,wtk_wdec_arc_t,output_n);
						if(nxt2->v.hmm==hmm)
						{
							b=1;
						}
					}
					if(b==0)
					{
						//wtk_debug("v[%d]=%.*s ==> [%.*s]\n",phn_pre->output_q.len,buf->pos,buf->data,hmm->name->len,hmm->name->data);
						nxt2=wtk_wdec_net_new_arc(net,phn_pre,ne);
						nxt2->label=label;
						nxt2->v.hmm=hmm;
					}
				}else
				{
					wtk_debug("v[%d]=%.*s not found.\n",j,buf->pos,buf->data);
				}
			}
			wtk_wdec_net_add_cross_phn(net,ne,arc->v.phn->name,nxt,buf);
		}
	}else
	{
		if(arc->nxt->output_q.len==0)
		{
			//for end;
			//wtk_debug("[%.*s-%.*s]\n",pre->len,pre->data,arc->v.phn->name->len,arc->v.phn->name->data);
			if(net->cfg->use_sil)
			{
				wtk_strbuf_reset(buf);
				wtk_strbuf_push_f(buf,"%.*s-%.*s+sil",pre->len,pre->data,arc->v.phn->name->len,arc->v.phn->name->data);
				//wtk_debug("%.*s wrd=%p [%.*s]\n",buf->pos,buf->data,arc->nxt->word,arc->nxt->word->name->len,arc->nxt->word->name->data);
				hmm=wtk_hmmset_find_hmm(net->cfg->hmmset.hmmset,buf->data,buf->pos);
				//wtk_debug("%.*s\n",hmm->name->len,hmm->name->data);
				//ne=wtk_wdec_net_new_node(net);
				//ne->word=arc->nxt->word;
				nxt2=wtk_wdec_net_new_arc(net,phn_pre,net->sil_end);
				nxt2->label=label;
				nxt2->v.hmm=hmm;
				net->sil_end->word=arc->nxt->word;
				//wtk_debug("%s\n",net->sil_end->word->name->data);
			}else
			{
				phns=(wtk_string_t**)(net->cfg->phn->slot);
				nphn=net->cfg->phn->nslot;
				//ne=wtk_wdec_net_new_node(net);
				ne=net->phn_end;
				for(j=0;j<nphn;++j)
				{
					wtk_strbuf_reset(buf);
					wtk_strbuf_push_f(buf,"%.*s-%.*s+%.*s",pre->len,pre->data,arc->v.phn->name->len,arc->v.phn->name->data,phns[j]->len,phns[j]->data);
					//wtk_debug("%.*s\n",buf->pos,buf->data);
					hmm=wtk_hmmset_find_hmm(net->cfg->hmmset.hmmset,buf->data,buf->pos);
					//wtk_debug("%.*s\n",hmm->name->len,hmm->name->data);
					if(hmm)
					{
						b=0;
						for(qn2=phn_pre->output_q.pop;qn2;qn2=qn2->next)
						{
							nxt2=data_offset2(qn2,wtk_wdec_arc_t,output_n);
							if(nxt2->v.hmm==hmm)
							{
								b=1;
							}
						}
						if(b==0)
						{
							//wtk_debug("v[%d]=%.*s ==> [%.*s]\n",phn_pre->output_q.len,buf->pos,buf->data,hmm->name->len,hmm->name->data);
							ne=wtk_wdec_net_new_node(net);
							nxt2=wtk_wdec_net_new_arc(net,phn_pre,ne);
							nxt2->label=label;
							nxt2->v.hmm=hmm;
						}
					}else
					{
						wtk_debug("v[%d]=%.*s not found.\n",j,buf->pos,buf->data);
					}
				}
			}
			return;
		}
		//wtk_debug("[%.*s-%.*s]\n",pre->len,pre->data,arc->v.phn->name->len,arc->v.phn->name->data);
		for(qn=arc->nxt->output_q.pop;qn;qn=qn->next)
		{
			nxt=data_offset2(qn,wtk_wdec_arc_t,output_n);
			wtk_strbuf_reset(buf);
			wtk_strbuf_push_f(buf,"%.*s-%.*s+%.*s",pre->len,pre->data,arc->v.phn->name->len,arc->v.phn->name->data,nxt->v.phn->name->len,nxt->v.phn->name->data);
			//wtk_debug("[%.*s]\n",buf->pos,buf->data);
			hmm=wtk_hmmset_find_hmm(net->cfg->hmmset.hmmset,buf->data,buf->pos);
			//wtk_debug("%.*s\n",hmm->name->len,hmm->name->data);
			if(hmm)
			{
				b=0;
				for(qn2=phn_pre->output_q.pop;qn2;qn2=qn2->next)
				{
					nxt2=data_offset2(qn2,wtk_wdec_arc_t,output_n);
					if(nxt2->v.hmm==hmm)
					{
						//wtk_debug("%.*s\n",hmm->name->len,hmm->name->data);
						if(net->cfg->words)
						{

						}else
						{
							b=1;
						}				
					}
				}
				if(b==0)
				{
					//wtk_debug("v[%d]=%.*s ==> [%.*s]\n",phn_pre->output_q.len,buf->pos,buf->data,hmm->name->len,hmm->name->data);
					ne=wtk_wdec_net_new_node(net);
					ne->word=arc->nxt->word;
//					if(ne->word)
//					{
//						wtk_debug("%.*s\n",ne->word->name->len,ne->word->name->data);
//					}
					nxt2=wtk_wdec_net_new_arc(net,phn_pre,ne);
					nxt2->label=label;
					nxt2->v.hmm=hmm;
					//exit(0);
				}
				wtk_wdec_net_add_cross_phn(net,nxt2->nxt,arc->v.phn->name,nxt,buf);
			}else
			{
				wtk_debug("%.*s not found.\n",buf->pos,buf->data);
				exit(0);
			}
		}
	}
	//exit(0);
}


void wtk_wdec_node_print_phn_pre2(wtk_wdec_node_t *node,wtk_strbuf_t *buf)
{
	wtk_wdec_arc_t *arc;
	wtk_queue_node_t *qn;
	int pos;

	if(node->input_q.len==0)
	{
		wtk_debug("[%.*s]\n",buf->pos,buf->data);
		return;
	}
	for(qn=node->input_q.pop;qn;qn=qn->next)
	{
		arc=data_offset2(qn,wtk_wdec_arc_t,input_n);
		pos=buf->pos;
		//wtk_debug("node=%p i=%d arc=%p\n",node,i,arc);
		wtk_strbuf_push(buf,arc->v.hmm->name->data,arc->v.hmm->name->len);
		//wtk_debug("nxt=%p [%.*s]\n",arc->nxt,arc->v.hmm->name->len,arc->v.hmm->name->data);
		if(arc->nxt && arc->nxt->word)
		{
			wtk_strbuf_push_s(buf,"[");
			wtk_strbuf_push(buf,arc->nxt->word->name->data,arc->nxt->word->name->len);
			wtk_strbuf_push_s(buf,"]");
		}
		wtk_strbuf_push_s(buf," ");
		wtk_wdec_node_print_phn_pre2(arc->pre,buf);
		buf->pos=pos;
	}
}

void wtk_wdec_node_print_phn_pre(wtk_wdec_node_t *node)
{
	wtk_strbuf_t *buf;

	wtk_debug("============  prev=%d ==================\n",node->input_q.len);
	buf=wtk_strbuf_new(1024,1);
	wtk_wdec_node_print_phn_pre2(node,buf);
	wtk_strbuf_delete(buf);
}

void wtk_wdec_node_print_phn_nxt2(wtk_wdec_node_t *node,wtk_strbuf_t *buf)
{
	wtk_wdec_arc_t *arc;
	wtk_queue_node_t *qn;
	int pos;

	if(node->output_q.len==0)
	{
		wtk_debug("[%.*s]\n",buf->pos,buf->data);
		return;
	}
	for(qn=node->output_q.pop;qn;qn=qn->next)
	{
		arc=data_offset2(qn,wtk_wdec_arc_t,output_n);
		pos=buf->pos;
		wtk_debug("[%.*s]\n",arc->v.hmm->name->len,arc->v.hmm->name->data);
		wtk_strbuf_push(buf,arc->v.hmm->name->data,arc->v.hmm->name->len);
		wtk_strbuf_push_s(buf," ");
		wtk_wdec_node_print_phn_nxt2(arc->nxt,buf);
		buf->pos=pos;
	}
}

void wtk_wdec_node_print_phn_nxt(wtk_wdec_node_t *node)
{
	wtk_strbuf_t *buf;

	wtk_debug("============  nxt ==================\n");
	buf=wtk_strbuf_new(1024,1);
	wtk_wdec_node_print_phn_nxt2(node,buf);
	wtk_strbuf_delete(buf);
}


void wtk_wdec_net_build_cross(wtk_wdec_net_t *net,wtk_wdec_node_t *wrd_pre)
{
	wtk_strbuf_t *buf;
	wtk_wdec_node_t *phn_pre,*node;
	wtk_queue_node_t *qn;
	wtk_wdec_arc_t *arc;
	wtk_string_t *pre;
	wtk_hmm_t *hmm;
	int i=0;

	buf=wtk_strbuf_new(256,1);
	phn_pre=wtk_wdec_net_new_node(net);
	net->phn_start=phn_pre;
	net->phn_end=wtk_wdec_net_new_node(net);
	if(net->cfg->use_sil)
	{
		hmm=wtk_hmmset_find_hmm_s(net->cfg->hmmset.hmmset,"sil");
		net->sil_end=wtk_wdec_net_new_node(net);
		//net->sil_end->word=wrd_pre->word;
		arc=wtk_wdec_net_new_arc(net,net->sil_end,net->phn_end);
		arc->v.hmm=hmm;
		node=wtk_wdec_net_new_node(net);
		arc=wtk_wdec_net_new_arc(net,phn_pre,node);
		arc->v.hmm=hmm;
		phn_pre=arc->nxt;
		pre=arc->v.hmm->name;
	}else
	{
		pre=NULL;
	}
	for(qn=wrd_pre->output_q.pop;qn;qn=qn->next)
	{
		arc=data_offset2(qn,wtk_wdec_arc_t,output_n);
		arc->label =net->label_set[i++];
		//wtk_debug("%d\n",arc->label);
		//wtk_debug("v[]=[%.*s]\n",arc->v.phn->name->len,arc->v.phn->name->data);
		wtk_wdec_net_add_cross_phn(net,phn_pre,pre,arc,buf);
	}
	wtk_strbuf_delete(buf);
	//wtk_wdec_node_print_phn_nxt(net->phn_start);
	//wtk_wdec_node_print_phn_pre(net->phn_end);
	//exit(0);
}

void wtk_wdec_net_build(wtk_wdec_net_t *net,char *word,int label,int is_end,int len)
{
	wtk_dict_t *dec=net->cfg->dict;
	wtk_dict_word_t *wrd;
	wtk_dict_pron_t *pron,*p2;
	wtk_string_t v;
	char *s,*e;
	int n;
	wtk_strbuf_t *buf;
	wtk_wdec_node_t *pre;
	wtk_wdec_node_t *ns,*ne;
	wtk_wdec_node_t *start;
	int i,j;
	wtk_wdec_arc_t *arc;
	int b;
	wtk_queue_node_t *qn;
	if(net->raw_start)
	{
		 pre=net->raw_start;
	}else
	{
		pre=wtk_wdec_net_new_node(net);
		net->raw_start=pre;
	}
	start=pre;
	s=word;
	e=s+len;
	buf=wtk_strbuf_new(256,1);
	//wtk_debug("%s\n",s);
	while(s<e)
	{
		n=wtk_utf8_bytes(*s);
		if(n>1)
		{
			wtk_string_set(&(v),s,n);
			//wtk_debug("[%.*s][len=%d]\n",n,s,n);
			wrd=wtk_dict_get_word(dec,&v,0);
		}else
		{
			if(isspace(*s))
			{
				wtk_string_set(&(v),buf->data,buf->pos);;
				wrd=wtk_dict_get_word(dec,&v,0);
				wtk_strbuf_reset(buf);
			}else if(s+1==e)
			{
				wtk_strbuf_push(buf,s,n);
				wtk_string_set(&(v),buf->data,buf->pos);;
				wrd=wtk_dict_get_word(dec,&v,0);
				wtk_strbuf_reset(buf);
			}else
			{		
				wtk_strbuf_push(buf,s,n);
				//wtk_debug("%s,%d\n",buf->data,buf->pos);
				s=s+n;
				continue;
			}
			
		}
		if(wrd)
		{
			//wtk_dict_word_print(wrd);
			ns=pre;
			//ns->word=wrd;
			ne=wtk_wdec_net_new_node(net);
			ne->word=wrd;
			pron=wrd->pron_list;
			i=0;
			while(pron)
			{
				pre=ns;
				for(j=0;j<pron->nPhones;++j)
				{
					//wtk_debug("v[%d,%d]=[%.*s]\n",i,j,pron->pPhones[j]->name->len,pron->pPhones[j]->name->data);
					b=0;
					if(j==0)
					{
						if(i>0)
						{
							p2=wrd->pron_list;
							for(qn=pre->output_q.pop;qn;qn=qn->next)
							{
								arc=data_offset2(qn,wtk_wdec_arc_t,output_n);
								//wtk_debug("[%.*s]=[%.*s]\n",pre->output_arc[k]->phn->name->len,pre->output_arc[k]->phn->name->data,p2->pPhones[j]->name->len,p2->pPhones[j]->name->data);
								if(wtk_string_cmp(arc->v.phn->name,p2->pPhones[j]->name->data,p2->pPhones[j]->name->len)==0)
								{
									b=1;
								}
							}
							if(!b)
							{
								arc=(wtk_wdec_arc_t*)wtk_heap_malloc(net->heap,sizeof(wtk_wdec_arc_t));
								arc->label=label;
							}
						}else
						{
							arc=(wtk_wdec_arc_t*)wtk_heap_malloc(net->heap,sizeof(wtk_wdec_arc_t));
							arc->label=label;
						}
						if(!b)
						{
							wtk_queue3_push(&(pre->output_q),&(arc->output_n));
							arc->pre=pre;
							arc->v.phn=pron->pPhones[j];
						}
					}else
					{
						arc=(wtk_wdec_arc_t*)wtk_heap_malloc(net->heap,sizeof(wtk_wdec_arc_t));
						wtk_queue3_push(&(pre->output_q),&(arc->output_n));
						arc->pre=pre;
						arc->v.phn=pron->pPhones[j];
						arc->label=label;
						//wtk_debug("%d\n",arc->label);
					}
					if(!b)
					{
						if(j==pron->nPhones-1)
						{
							arc->nxt=ne;
						}else
						{
							arc->nxt=wtk_wdec_net_new_node(net);
						}
						wtk_queue3_push(&(arc->nxt->input_q),&(arc->input_n));
					}
					pre=arc->nxt;
				}
				++i;
				pron=pron->next;
			}
			pre=ne;
		}else
		{
			wtk_debug("Err [%.*s] word not found.\n",n,s);
		}
		s+=n;
	}
	//wtk_debug("============= 1223 ============\n");
	//wtk_wdec_node_print_nxt(start);
	//wtk_wdec_node_print_pre(pre);
	wtk_strbuf_delete(buf);
	if(is_end)
	{
		//wtk_wdec_node_print_nxt(start);
		wtk_wdec_net_build_cross(net,start);
		//exit(0);
	}
	//exit(0);
}

int wtk_wdec_get_label(wtk_wdec_net_cfg_t *cfg,wtk_string_t *word)
{
	int i;
	wtk_wdec_words_set_t *set;
	int label=0;
	for(i=0;i<cfg->n_words;++i)
	{
		set=cfg->set+i;
		if(strncmp(set->word,word->data,word->len)==0)
		{
			label = set->label;
		}
	}
	return label;
}

wtk_wdec_net_t* wtk_wdec_net_new(wtk_wdec_net_cfg_t *cfg)
{
	wtk_wdec_net_t *net;
	int i;
	wtk_string_t **words;
        int n_words = 0;
        net = (wtk_wdec_net_t *)wtk_malloc(sizeof(wtk_wdec_net_t));
        net->cfg=cfg;
	net->heap=wtk_heap_new(4096);
	net->raw_start=NULL;
	int label=0;
	if(net->cfg->words)
	{
		words=(wtk_string_t**)(net->cfg->words->slot);
		n_words=cfg->words->nslot;
	}
	if(cfg->word)
	{
		wtk_debug("%s\n",cfg->word);
		wtk_wdec_net_build(net,cfg->word,0,1,strlen(cfg->word));
	}else if(cfg->words)
	{
		memset(net->label_set,0,sizeof(net->label_set));
		for(i=0;i<n_words;++i)
		{
			label=wtk_wdec_get_label(cfg,words[i]);
			//wtk_debug("hhhhhhhhhhhhh   label=%d %d\n",label,i);
			net->label_set[i]=label;
			//wtk_debug("%s\n",words[i]->data);
			wtk_wdec_net_build(net,words[i]->data,label,i==n_words-1,words[i]->len);	
		}
	}
	return net;
}


void wtk_wdec_net_delete(wtk_wdec_net_t *net)
{
	wtk_heap_delete(net->heap);
	wtk_free(net);
}

void wtk_wdec_node_reset(wtk_wdec_node_t *node)
{
	wtk_queue_node_t *qn;
	wtk_wdec_arc_t *arc;

	for(qn=node->output_q.pop;qn;qn=qn->next)
	{
		arc=data_offset2(qn,wtk_wdec_arc_t,output_n);
		wtk_queue3_init(&(arc->inst_q));
		wtk_wdec_node_reset(arc->nxt);
	}
}

void wtk_wdec_net_reset(wtk_wdec_net_t *net)
{
	wtk_wdec_node_reset(net->phn_start);
}
