#include "wtk_lmlat.h"

wtk_lmlat_t* wtk_lmlat_new(wtk_lmlat_cfg_t *cfg,wtk_lm_dict_cfg_t *dict_cfg)
{
	wtk_lmlat_t *lat;

	lat=(wtk_lmlat_t*)wtk_malloc(sizeof(wtk_lmlat_t));
	lat->cfg=cfg;
	lat->dict_cfg=dict_cfg;
	//lat->nglm=wtk_nglm_new(&(cfg->nglm),dict_cfg);
	lat->kv=wtk_wfst_kv_new(&(cfg->kv));
	lat->heap=wtk_heap_new(4096);
	lat->expand_net=NULL;
	lat->output_net=wtk_fst_net2_new(&(cfg->output_net));
	lat->buf=wtk_strbuf_new(256,1);
	lat->nback_id=0;
	wtk_queue_init(&(lat->inst_q));
	return lat;
}

void wtk_lmlat_delete(wtk_lmlat_t *lat)
{
	if(lat->kv)
	{
		wtk_wfst_kv_delete(lat->kv);
	}
	if(lat->expand_net)
	{
		wtk_fst_net2_delete(lat->expand_net);
	}
	wtk_fst_net2_delete(lat->output_net);
	wtk_strbuf_delete(lat->buf);
	wtk_heap_delete(lat->heap);
	//wtk_nglm_delete(lat->nglm);
	wtk_free(lat);
}

int wtk_lmlat_bytes(wtk_lmlat_t *lat)
{
	int bytes;

	bytes=wtk_heap_bytes(lat->heap);
	bytes+=wtk_fst_net2_bytes(lat->output_net);
	//bytes+=wtk_nglm_bytes(lat->nglm);
	if(lat->expand_net)
	{
		bytes+=wtk_fst_net2_bytes(lat->expand_net);
	}
	return bytes;
}

void wtk_lmlat_reset(wtk_lmlat_t *lat)
{
	if(lat->expand_net)
	{
		wtk_fst_net2_reset(lat->expand_net);
	}
	wtk_fst_net2_reset(lat->output_net);
	wtk_queue_init(&(lat->inst_q));
	wtk_heap_reset(lat->heap);
	//wtk_nglm_reset(lat->nglm);
	wtk_wfst_kv_reset(lat->kv);
	//wtk_debug("nback=%d\n",lat->nback_id);
	lat->lm_start_node=wtk_wfst_kv_get_root2(lat->kv,lat->back_ids,lat->nback_id);
}

void wtk_lmlat_update_clean_hist(wtk_lmlat_t *lat)
{
	lat->nback_id=0;
}

void wtk_lmlat_update_history(wtk_lmlat_t *lat,int *idx,int nid)
{
	int i;

	lat->nback_id=nid;
	for(i=0;i<nid;++i)
	{
		lat->back_ids[i]=idx[nid-i-1];
		//wtk_debug("v[%d]=%d\n",i,lat->back_ids[i]);
	}
}


wtk_lm_history_t* wtk_lmlat_pop_hist(wtk_lmlat_t *r)
{
	wtk_heap_t *heap=r->heap;
	wtk_lm_history_t *hist;

	hist=(wtk_lm_history_t*)wtk_heap_malloc(heap,sizeof(wtk_lm_history_t));
	hist->node=NULL;
	hist->prob=0;
	return hist;
}

wtk_lm_trans_item_t* wtk_lmlat_pop_trans_item(wtk_lmlat_t *r,wtk_fst_trans2_t *trans)
{
	wtk_heap_t *heap=r->heap;
	wtk_lm_trans_item_t *item;

	item=(wtk_lm_trans_item_t*)wtk_heap_malloc(heap,sizeof(wtk_lm_trans_item_t));
	item->input_trans=trans;
	item->hist_used=0;
	if(r->cfg->ntok>0)
	{
		item->hist=(wtk_lm_history_t**)wtk_heap_malloc(heap,sizeof(wtk_lm_history_t*)*r->cfg->ntok);
	}else
	{
		item->hist=NULL;
	}
	return item;
}

wtk_lm_state_item_t* wtk_lmlat_pop_state_item(wtk_lmlat_t *r,wtk_fst_state2_t *state)
{
	wtk_heap_t *heap=r->heap;
	wtk_lm_state_item_t *item;

	item=(wtk_lm_state_item_t*)wtk_heap_malloc(heap,sizeof(wtk_lm_state_item_t));
	item->trans_used=0;
	item->input_state=state;
	item->trans=(wtk_lm_trans_item_t**)wtk_heap_malloc(heap,sizeof(wtk_lm_trans_item_t*)*r->cfg->state_ntok);
	return item;
}

void wtk_lmlat_print_hist(wtk_lmlat_t *r,wtk_lm_history_t *hist,float f)
{
	wtk_strbuf_t *buf;
	wtk_lm_history_t *p=hist;
	//wtk_fst_insym_t *sym=r->dict_cfg->sym;

	buf=wtk_strbuf_new(256,1);
	while(hist && hist->node)
	{
		if(buf->pos>0)
		{
			wtk_strbuf_push_s(buf," ");
		}
		//wtk_strbuf_push(buf,sym->ids[hist->node->v.node.id]->str->data,sym->ids[hist->node->id]->str->len);
		//wtk_strbuf_push_f(buf,"[n=%d,f=%.1f/%.1f]",hist->node->ngram,hist->node->prob,hist->prob);
		hist=hist->prev;
	}
	if(buf->pos>0)
	{
		printf("[%.*s] prob=%.1f/%.1f\n",buf->pos,buf->data,p->prob,f);
		//exit(0);
	}
	wtk_strbuf_delete(buf);
}

void wtk_lmlat_print_hist2(wtk_lmlat_t *r,wtk_lm_history_t *prev,wtk_wfst_kv_env_t *node,float f)
{
	wtk_lm_history_t hist;

	hist.node=node;
	hist.prob=f;
	hist.prev=prev;
	wtk_lmlat_print_hist(r,&hist,f);
}

void wtk_lmlat_print_trans(wtk_lmlat_t *r,wtk_lm_trans_item_t *item)
{
//	wtk_fst_insym_t *sym=r->dict_cfg->sym;
//	int i;
//
//	for(i=0;i<item->hist_used;++i)
//	{
//		wtk_lm_node_print2(r->dict_cfg->sym,item->hist[i]->node);
////		wtk_debug("v[%d/%d] [%.*s] ngram=%f/%d prob=%f node=%p\n",i,item->hist_used,
////				sym->ids[item->hist[i]->node->id]->str->len,
////				sym->ids[item->hist[i]->node->id]->str->data,
////				item->hist[i]->node->prob,
////				item->hist[i]->node->ngram,
////				item->hist[i]->prob,
////				item->hist[i]->node);
//	}
}

void wtk_lmlat_print_trans2(wtk_lmlat_t *r,wtk_lm_trans_item_t *item)
{
//	wtk_fst_insym_t *sym=r->dict_cfg->sym;
//	wtk_strbuf_t *buf;
//	wtk_lm_history_t *hist;
//	int i;
//
//	buf=wtk_strbuf_new(256,1);
//	for(i=0;i<item->hist_used;++i)
//	{
//		hist=item->hist[i];
//		wtk_strbuf_reset(buf);
//		while(hist && hist->node)
//		{
//			if(buf->pos>0)
//			{
//				wtk_strbuf_push_s(buf," ");
//			}
//			wtk_strbuf_push(buf,sym->ids[hist->node->id]->str->data,sym->ids[hist->node->id]->str->len);
//			//wtk_strbuf_push_f(buf,"[n=%d,f=%.1f/%.1f]",hist->node->ngram,hist->node->prob,hist->prob);
//			hist=hist->prev;
//		}
//		printf("v[%d/%d]=[%.*s] prob=%.1f\n",i,item->hist_used,buf->pos,buf->data,item->hist[i]->prob);
//	}
//	wtk_strbuf_delete(buf);
}

void wtk_lm_trans_item_add_hist(wtk_lm_trans_item_t *item,wtk_lm_history_t *hist2)
{
	wtk_lm_history_t *hist3;
	int i;

	if(item->hist_used>0)
	{
		for(i=item->hist_used-1;i>=0;--i)
		{
			hist3=item->hist[i];
			if(hist3->prob>hist2->prob)
			{
				item->hist[i+1]=hist3;
				if(i==0)
				{
					item->hist[0]=hist2;
				}
			}else
			{
				item->hist[i+1]=hist2;
				break;
			}
		}
	}else
	{
		item->hist[0]=hist2;
	}
	++item->hist_used;
}

void wtk_lm_state_item_print(wtk_lm_state_item_t *item)
{
	int i;

	wtk_debug("============= item=%p =========\n",item);
	for(i=0;i<item->trans_used;++i)
	{
		printf("v[%d/%d]=%f trans=%p\n",i,item->trans_used,item->trans[i]->hist[0]->prob,item->trans[i]);
	}
}

int wtk_lm_trans_item_cmp(const void *v1,const void *v2)
{
	wtk_lm_trans_item_t **p1,**p2;

	p1=(wtk_lm_trans_item_t**)v1;
	p2=(wtk_lm_trans_item_t**)v2;
	if((*p1)->hist[0]->prob<(*p2)->hist[0]->prob)
	{
		return -1;
	}else
	{
		return 1;
	}
}

void wtk_lm_state_item_reorder(wtk_lm_state_item_t *state_item)
{
	if(state_item->trans_used>1)
	{
		//wtk_lm_state_item_print(state_item);
		qsort(state_item->trans,state_item->trans_used,
				sizeof(wtk_lm_trans_item_t*),wtk_lm_trans_item_cmp);
		//wtk_lm_state_item_print(state_item);
		//exit(0);
	}
}

void wtk_lm_state_item_add_trans(wtk_lm_state_item_t *state_item,wtk_lm_trans_item_t *item,double f)
{
	wtk_lm_trans_item_t *item2;
	int i;

	if(state_item->trans_used>0)
	{
		for(i=state_item->trans_used-1;i>=0;--i)
		{
			item2=state_item->trans[i];
			if(item2->hist[0]->prob>f)
			{
				state_item->trans[i+1]=item2;
				if(i==0)
				{
					state_item->trans[0]=item;
				}
			}else
			{
				state_item->trans[i+1]=item;
				break;
			}
		}
		//wtk_lm_state_item_print(state_item);
		//wtk_debug("%d/%p\n",state_item->trans_used,item);
		//exit(0);
	}else
	{
		state_item->trans[0]=item;
	}
	++state_item->trans_used;
}

int wtk_lmlat_add_trans(wtk_lmlat_t *r,wtk_lm_history_t *hist,
		wtk_fst_state2_t *output_from_state,
		wtk_fst_trans2_t *trans,wtk_fst_net2_t *output_net)
{
	wtk_wfst_kv_env_t *node;
	wtk_lm_trans_item_t *item=NULL;
	wtk_lm_history_t *hist2;
	wtk_fst_trans2_t *trans2;
	wtk_fst_state2_t *output_to_state=NULL;
	wtk_lm_state_item_t *state_item=NULL;
	double lm,f;
	int reorder=0;
	int ret=-1;

	hist2=NULL;
	//wtk_debug("trans=%p hook2=%p\n",trans,trans->hook2);
	wtk_fst_state2_t *st;
	//st=(wtk_fst_state2_t*)trans->to_state;
	//wtk_debug("%d %d %p %d %d\n",trans->in_id,output_net->cfg->snt_start_id,trans->to_state,st->type,st->fuck);
	//wtk_debug("%d %d %p %d %d\n",trans->in_id,output_net->cfg->snt_start_id,trans->to_state,trans->to_state->type,trans->to_state->fuck);
	//if(output_from_state!=NULL)
	//{
	//	wtk_debug("process state:%d\n",output_from_state->id);
	//	wtk_debug("process trans:%d to %d\n",trans->from_state->id,trans->to_state->id);
	//}
	if(trans->in_id==output_net->cfg->snt_start_id)
	{
		//wtk_debug("get start node\n");
		//wtk_debug("1111111\n");
		//node=wtk_rel_kv_get_root(r->kv);
		//node=r->nglm->s_node;
		node=r->lm_start_node;
		lm=0;
	}else
	{
//		wtk_debug("1111112\n");
		if(hist)
		{
			//wtk_debug("has hist\n");
			if(trans->in_id==output_net->cfg->eps_id)
			{
				//wtk_debug("trans in id =0\n");
				node=hist->node;
				lm=0;
			}else
			{
#ifdef DEBUG_INPUT
				wtk_debug("[%d]=[%.*s/%.*s]\n",trans->in_id,
						r->cfg->output_net.sym_out->strs[trans->in_id]->len,
						r->cfg->output_net.sym_out->strs[trans->in_id]->data,
						r->output_net->cfg->sym_out->strs[trans->out_id]->len,
						r->output_net->cfg->sym_out->strs[trans->out_id]->data);
				if(hist->node && 0)
				{
					static int ki=0;

					++ki;
					wtk_debug("v[%d]\n",ki);
					if(ki==66)
					{
						wtk_lm_node_print_parent(hist->node);
					}
					wtk_lm_node_print2(r->dict_cfg->sym,hist->node);
				}
#endif
				node=wtk_wfst_kv_get(r->kv,hist->node,trans->in_id,&lm);
//				if(r->kv->cfg->use_rnn)
//				{
//					node=wtk_rel_kv_get(r->kv,hist->node,trans->in_id,&lm);
//				}else
//				{
//					node=(wtk_rel_kv_env_t*)wtk_nglm_get_child_prob(r->kv->ngram,(wtk_lm_node_t*)hist->node,trans->in_id,&lm);
//				}
				//node=wtk_nglm_get_child_prob(r->nglm,hist->node,trans->in_id,&lm);
				if(!node)
				{
					//wtk_lm_node_print2(r->dict_cfg->sym,hist->node);
					//wtk_debug("[%d]\n",trans->in_id)
					ret=-1;goto end;
				}
				//wtk_lm_node_print2(r->dict_cfg->sym,node);
//				wtk_debug("lm=%f/%f\n",lm,node->prob);
				//wtk_lm_node_print2(r->dict_cfg->sym,node);
				//wtk_lm_node_print_child(r->dict_cfg->sym,node);
				//lm+=node->prob;
				//wtk_debug("lm=%f pen=%f scale=%f\n",lm,r->cfg->wrdpen,r->cfg->lmscale);
				if(trans->in_id>0 &&r->cfg->wrdpen!=0)
				{
					lm+=r->cfg->wrdpen;
				}
				lm=lm*r->cfg->lmscale;
			}
		}else
		{
			node=NULL;
			lm=0;
		}
	}
	f=lm+trans->weight;
//	wtk_debug("%f %f\n",lm,trans->weight);
	if(hist)
	{
//		wtk_debug("add hist prob:%f\n",hist->prob);
		f+=hist->prob;
	}
	//wtk_debug("????\n");
	if(r->cfg->ntok>0 || r->cfg->beam>0)
	{
		//wtk_debug("111111c\n");
		st=(wtk_fst_state2_t*)trans->to_state;
		if(st->type==WTK_FST_FINAL_STATE)
		{
//			wtk_debug("final state\n");
			if(!output_net->end)
			{
				output_net->end=wtk_fst_net2_pop_state(output_net);
				output_net->end->type=WTK_FST_FINAL_STATE;
			}
			output_to_state=output_net->end;
		}else
		{
			if(r->cfg->state_ntok>0)
			{
				                st=(wtk_fst_state2_t*)trans->to_state;
				if(!st->hook)
				{
					state_item=(wtk_lm_state_item_t*)wtk_lmlat_pop_state_item(r,(wtk_fst_state2_t*)trans->to_state);
					trans->to_state->hook=state_item;
//					wtk_debug("pop state %p item:%p\n",trans->to_state,state_item);
				}else
				{
					state_item=(wtk_lm_state_item_t*)trans->to_state->hook;
//					wtk_debug("has state %p item:%p\n",trans->to_state,state_item);
				}
			}
			//wtk_debug("state_item=%p/%d\n",state_item,r->cfg->state_ntok);
			if(!trans->hook2)
			{
//				wtk_debug("trans with no hook2\n");
				if(state_item && state_item->trans_used>0)
				{
					item=state_item->trans[state_item->trans_used-1];
					if((f-item->hist[0]->prob)>r->cfg->state_beam)
					{
//						wtk_debug("(f-item->hist[0]->prob)>r->cfg->state_beam return\n");
						return 0;
					}
					if(state_item->trans_used>=r->cfg->state_ntok)
					{
						if(f>=item->hist[0]->prob)
						{
//							wtk_debug("f>=item->hist[0]->prob and ntok full return\n");
							return 0;
						}
						--state_item->trans_used;
					}
				}
				item=wtk_lmlat_pop_trans_item(r,trans);
				trans->hook2=item;
//				wtk_debug("pop item\n");
				if(state_item)
				{
//					wtk_debug("add item to state item\n");
					wtk_lm_state_item_add_trans(state_item,item,f);
				}
			}else
			{
				item=(wtk_lm_trans_item_t*)trans->hook2;
//				wtk_debug("has hook2\n");
				if(item->hist && r->cfg->beam>0 && trans->to_state->type!=WTK_FST_FINAL_STATE)
				{
					if((f-item->hist[0]->prob)>r->cfg->beam)
					{
//						wtk_debug("%f %f\n",f,item->hist[0]->prob);
//						wtk_debug("(f-item->hist[0]->prob)>r->cfg->beam return\n");
						return 0;
					}
				}
			}
		}
		//output_to_state=item->output_to_statep;
                st=(wtk_fst_state2_t*)trans->to_state;

		if((st->type!=WTK_FST_FINAL_STATE))
		{
			if(item->hist && (item->hist_used>=r->cfg->ntok))
			{
				if(f>=(item->hist[item->hist_used-1]->prob))
				{
//					wtk_debug("%d %d\n",item->hist_used,r->cfg->ntok);
//					wtk_debug("return\n");
					//wtk_lm_node_print2(r->nglm->cfg->sym,node);
					//wtk_lmlat_print_trans(r,item);
					return 0;
				}
				item->hist[item->hist_used-1]->used=0;
//				wtk_debug("hist[0]:%p %f\n",item->hist[0],item->hist[0]->prob);
//				wtk_debug("hist[1]:%p %f\n",item->hist[1],item->hist[1]->prob);
//				wtk_debug("hist[2]:%p %f\n",item->hist[2],item->hist[2]->prob);
//				wtk_debug("hist:%p used set 0\n",item->hist[item->hist_used-1]);
				--item->hist_used;
			}
			if(!output_to_state)
			{
				output_to_state=wtk_fst_net2_pop_state(output_net);
//				wtk_debug("pop state:%d\n",output_to_state->id);
			}
			hist2=wtk_lmlat_pop_hist(r);
//			wtk_debug("pop hist:%p prob:%f\n",hist2,f);
			hist2->prev=hist;
			hist2->node=node;
			hist2->prob=f;
			hist2->input_trans=trans;
			hist2->output_state=output_to_state;
			hist2->used=1;
			hist2->item=item;
			wtk_queue_push(&(r->inst_q),&(hist2->inst_n));
			if(item->hist)
			{
				if(state_item && state_item->trans_used>1 && item->hist_used>0  && f<item->hist[0]->prob)
				{
					//wtk_debug("f=%f/%f\n",f,item->hist[0]->prob);
					reorder=1;
				}
				wtk_lm_trans_item_add_hist(item,hist2);
				if(reorder)
				{
					wtk_lm_state_item_reorder(state_item);
				}
			}
		}
	}else
	{
		//wtk_debug("111111d\n");
                st=(wtk_fst_state2_t*)trans->to_state;

		if(st->type==WTK_FST_FINAL_STATE)
		{
			if(!output_net->end)
			{
				output_net->end=wtk_fst_net2_pop_state(output_net);
			}
			output_to_state=output_net->end;
		//	wtk_debug("hell eof=%p\n",output_to_state);
		}else
		{
			output_to_state=wtk_fst_net2_pop_state(output_net);
		}
		hist2=wtk_lmlat_pop_hist(r);
		hist2->node=node;
		hist2->prob=f;
		hist2->input_trans=trans;
		hist2->output_state=output_to_state;
		hist2->used=1;
		hist2->item=NULL;
		wtk_queue_push(&(r->inst_q),&(hist2->inst_n));
	}
	trans2=wtk_fst_net2_pop_trans(output_net);
	trans2->hook.next=output_from_state->v.trans;
	output_from_state->v.trans=(wtk_fst_trans_t*)trans2;
	trans2->in_prev=output_to_state->in_prev;
	output_to_state->in_prev=trans2;
	trans2->from_state=(wtk_fst_state_t*)output_from_state;
	trans2->to_state=(wtk_fst_state_t*)output_to_state;
	trans2->in_id=trans->out_id;
	trans2->out_id=trans->out_id;
//	wtk_debug("trans2 %f %f\n",trans->weight,lm);
	trans2->weight=trans->weight+lm;
	trans2->frame=trans->frame;
	trans2->lm_like=lm;
//	wtk_debug("new trans:%p %d to %p %d %d\n",output_from_state,output_from_state->id,output_to_state,output_to_state->id,trans->out_id);
//	wtk_debug("new trans:%d to %d %d weight:%f\n",output_from_state->id,output_to_state->id,trans2->out_id,trans2->weight);

	ret=0;
end:
	return ret;
}


int wtk_lmlat_init_start(wtk_lmlat_t *r,wtk_fst_net2_t *input,wtk_fst_net2_t *output)
{
	wtk_fst_state2_t *output_state,*state;
	wtk_fst_trans2_t *trans;
	int ret;

	ret=0;
	output_state=wtk_fst_net2_pop_state(output);
	output->start=output_state;
	state=input->start;
	for(trans=(wtk_fst_trans2_t*)state->v.trans;trans;trans=(wtk_fst_trans2_t*)trans->hook.next)
	{
//		wtk_debug("init start add initial trans to hist\n");
		ret=wtk_lmlat_add_trans(r,NULL,output_state,trans,output);
		if(ret!=0){goto end;}
	}
end:
	return ret;
}

void wtk_lmlat_expand_strs(wtk_lmlat_t *r,int idx,wtk_string_t **strs,
		int n,wtk_strbuf_t *buf,wtk_fst_net2_t *net2,
		wtk_fst_state2_t *s,wtk_fst_state2_t *t,
		wtk_fst_trans2_t *expand_trans)
{
	wtk_lmexpand_dict_t *dict=r->cfg->dict;
	wtk_lmexpand_dict_item_t *item;
	wtk_fst_trans2_t *trans2;
	wtk_fst_state2_t *s2;
	int i,j;
	char *p;
	int len;

	for(i=0;i<n;++i)
	{
		if(i>0)
		{
			wtk_strbuf_reset(buf);
			for(j=0;j<=i;++j)
			{
				wtk_strbuf_push(buf,strs[j]->data,strs[j]->len);
			}
			p=buf->data;
			len=buf->pos;
		}else
		{
			p=strs[i]->data;
			len=strs[i]->len;
		}
		item=(wtk_lmexpand_dict_item_t*)wtk_str_hash_find(dict->hash,p,len);
		if(item)
		{
			trans2=wtk_fst_net2_pop_trans(net2);
			j=n-i-1;
			if(j>0)
			{
				s2=wtk_fst_net2_pop_state(net2);
			}else
			{
				s2=t;
			}
			if(idx==0)
			{
				trans2->out_id=expand_trans->out_id;
				trans2->weight=expand_trans->weight;
			}else
			{
				trans2->out_id=0;
				trans2->weight=0;
			}
			trans2->in_id=item->ids[0];
			//wtk_debug("[%.*s]\n",len,p);
			trans2->from_state=(wtk_fst_state_t*)s;
			trans2->to_state=(wtk_fst_state_t*)s2;
			trans2->frame=expand_trans->frame;
			trans2->hook.next=s->v.trans;
			s->v.trans=(wtk_fst_trans_t*)trans2;

			trans2->in_prev=s2->in_prev;
			s2->in_prev=trans2;
			trans2->lm_like=0;
			if(j>0)
			{
				wtk_lmlat_expand_strs(r,idx+1,&(strs[i+1]),j,buf,net2,s2,t,expand_trans);
			}
		}
	}
}

void wtk_lmlat_expand_wrd(wtk_lmlat_t *r,wtk_string_t *v,
		wtk_fst_net2_t *net2,
		wtk_fst_state2_t *s,wtk_fst_state2_t *t,
		wtk_fst_trans2_t *expand_trans)
{
	wtk_array_t *a;
	wtk_string_t **strs;
	wtk_strbuf_t *buf;

	a=wtk_str_to_chars(r->heap,v->data,v->len);
	strs=(wtk_string_t**)a->slot;

	buf=r->buf;
	wtk_lmlat_expand_strs(r,0,strs,a->nslot,buf,net2,s,t,expand_trans);
}

wtk_fst_state2_t* wtk_lmlat_expand_state(wtk_lmlat_t *r,wtk_fst_state2_t *state,wtk_fst_sym_t *in_sym,
		wtk_fst_net2_t *net2)
{
	wtk_fst_trans2_t *trans,*trans2;
	wtk_fst_state2_t *s1,*s2;
	wtk_string_t *v;
	wtk_lmexpand_dict_t *dict=r->cfg->dict;
	wtk_lmexpand_dict_item_t *item;
	int i;

	s1=wtk_fst_net2_pop_state(net2);
	state->hook=s1;
	if(state->type==WTK_FST_FINAL_STATE)
	{
		net2->end=s1;
		s1->v.weight=state->v.weight;
		s1->type=WTK_FST_FINAL_STATE;
		return s1;
	}
	//wtk_fst_state2_t* wtk_fst_net2_pop_state(wtk_fst_net2_t *net);
	//wtk_fst_trans2_t* wtk_fst_net2_pop_trans(wtk_fst_net2_t *net);
	for(trans=(wtk_fst_trans2_t*)state->v.trans;trans;trans=(wtk_fst_trans2_t*)trans->hook.next)
	{
		//wtk_debug("[%d]\n",trans->out_id);
		v=in_sym->strs[trans->out_id];
		//wtk_debug("[%.*s]\n",v->len,v->data);
		item=(wtk_lmexpand_dict_item_t*)wtk_str_hash_find(dict->hash,v->data,v->len);
		if(!trans->to_state->hook)
		{
			wtk_lmlat_expand_state(r,(wtk_fst_state2_t*)trans->to_state,in_sym,net2);
		}
		s2=(wtk_fst_state2_t*)(trans->to_state->hook);
		if(item)
		{
			for(i=0;i<item->nid;++i)
			{
				trans2=wtk_fst_net2_pop_trans(net2);

				trans2->out_id=trans->out_id;

				trans2->in_id=item->ids[i];
				trans2->weight=trans->weight;

				trans2->from_state=(wtk_fst_state_t*)s1;
				trans2->to_state=(wtk_fst_state_t*)s2;
				trans2->frame=trans->frame;
				trans2->hook.next=s1->v.trans;
				s1->v.trans=(wtk_fst_trans_t*)trans2;

				trans2->in_prev=s2->in_prev;
				s2->in_prev=trans2;
				trans2->lm_like=trans->lm_like;
			}
		}else
		{
			//wtk_debug("[%.*s]\n",v->len,v->data);
			wtk_lmlat_expand_wrd(r,v,net2,s1,s2,trans);
			/*
			trans2=wtk_fst_net2_pop_trans(net2);
			trans2->out_id=trans->out_id;
			trans2->in_id=0;
			trans2->weight=trans->weight;
			trans2->weight+=r->cfg->unk_pen;

			trans2->from_state=(wtk_fst_state_t*)s1;
			trans2->to_state=(wtk_fst_state_t*)s2;
			trans2->frame=trans->frame;
			trans2->hook.next=s1->v.trans;
			s1->v.trans=(wtk_fst_trans_t*)trans2;

			trans2->in_prev=s2->in_prev;
			s2->in_prev=trans2;
			trans2->lm_like=0;//trans->lm_like;
			//exit(0);
			 */
		}
	}
	return s1;
}

wtk_fst_net2_t* wtk_lmlat_expand(wtk_lmlat_t *r,wtk_fst_net2_t *input)
{
	wtk_fst_net2_t *net2;
	wtk_fst_state2_t *state;

	if(!r->expand_net)
	{
		r->expand_net=wtk_fst_net2_new(input->cfg);
	}
	net2=r->expand_net;
	//wtk_debug("%s\n",r->cfg->dict_fn);
	state=wtk_lmlat_expand_state(r,input->start,input->cfg->sym_out,net2);
	net2->start=state;
	//wtk_debug("s=%p e=%p\n",net2->start,net2->end);
	return net2;
}


wtk_fst_net2_t* wtk_lmlat_process(wtk_lmlat_t *r,wtk_fst_net2_t *input)
{
	wtk_fst_net2_t *output=r->output_net;
	wtk_queue_t *q=&(r->inst_q);
	wtk_queue_node_t *qn;
	wtk_lm_history_t *hist;
	wtk_fst_net2_t *raw_input;
	wtk_fst_trans2_t *trans;//,*trans2;
	//wtk_lmlat_item_t *item2;
	//wtk_lmlat_node_t *rnode;
	double beam=r->cfg->beam;
	int ret;

	raw_input=input;
	r->output_net->cfg=input->cfg;
	if(r->cfg->use_dict)
	{
		//wtk_fst_net2_print_shortest_path(input);
		input=wtk_lmlat_expand(r,input);
	}
//	wtk_debug("N=%d,L=%d eof=%p\n",input->state_id,input->trans_id,input->end);
//	wtk_debug("fuck type:%d %p %d\n",input->end->type,input->end,input->end->fuck);
//	wtk_debug("init start\n");
	ret=wtk_lmlat_init_start(r,input,output);
//	wtk_debug("fuck type:%d %p %d\n",input->end->type,input->end,input->end->fuck);
	if(ret!=0){goto end;wtk_debug("?????\n");}
	//wtk_debug("len=%d\n",q->length);
//	wtk_debug("process\n");
	while(1)
	{
		qn=wtk_queue_pop(q);
//		wtk_debug("len=%d\n",q->length);
		if(!qn){break;}
		hist=data_offset(qn,wtk_lm_history_t,inst_n);
//		wtk_debug("process hist:%p used:%d\n",hist,hist->used);
		if(!hist->used){continue;}
		hist->used=0;
		if(beam>0 && hist->item && hist->item->hist_used>1)
		{
//				wtk_debug("%f %f\n",hist->prob-hist->item->hist[0]->prob,beam);
			if((hist->prob-hist->item->hist[0]->prob)>beam)
			{	
//				wtk_debug("continue\n");
				continue;
			}
		}
		for(trans=(wtk_fst_trans2_t*)hist->input_trans->to_state->v.trans;trans;
				trans=(wtk_fst_trans2_t*)trans->hook.next)
		{
			//wtk_debug("i=%d\n",++i);
//			wtk_debug("add trans %p \n",hist);
			ret=wtk_lmlat_add_trans(r,hist,hist->output_state,trans,output);
			if(ret!=0)
			{
//				wtk_debug("add trans failed\n");
				goto end;
			}
		}
	}
	//wtk_debug("s=%p e=%p\n",output->start,output->end);
	//wtk_debug("n=%d\n",wtk_fst_net2_states(r->output_net));
//	wtk_fst_net2_remove_nil_node(output);//,&(r->state_l));
	//wtk_debug("n=%d\n",wtk_fst_net2_states(r->output_net));
	//exit(0);
//	wtk_debug("N=%d,L=%d eof=%p\n",r->output_net->state_id,r->output_net->trans_id,r->output_net->end);
	//wtk_fst_net2_print(r->output_net);
#ifdef DEBUG_X
	if(r->cfg->use_dict)// && 0)
	{
		//wtk_fst_net2_nbest_path(output,100,100000);
		//wtk_fst_net3_print_nbest(output);
		wtk_fst_net2_print_shortest_path(output);
		exit(0);
	}
#endif
	//wtk_fst_net2_write_lat(r->output_net,"test.lat");
	//exit(0);
	ret=0;
end:
	if(r->cfg->use_dict)
	{
		wtk_fst_net2_clean_hook2(raw_input);
	}else
	{
		wtk_fst_net2_clean_hook(raw_input);
	}
	if(ret!=0)
	{
		output=NULL;
	}
	return output;
}

