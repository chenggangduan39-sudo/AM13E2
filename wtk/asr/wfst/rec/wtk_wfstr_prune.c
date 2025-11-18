#include "wtk_wfstr_prune.h"
#include "wtk/os/wtk_proc.h"

void wtk_wfstr_lat_add_end(wtk_wfstr_t *r)
{
	wtk_fst_net3_t *net=r->lat_net;
	wtk_queue_node_t *n;
	wtk_queue_node_t *qn;
	wtk_fst_lat_inst_t *from_inst;
	wtk_fst_inst_t *inst;
	wtk_fst_node_t *fn,*en;
	wtk_fst_node_t *end;
	float arc_like,ac_like,lm_like;
	wtk_fst_tok_t *tokset;
	//wtk_fst_arc_t *arc;
	int i;
	int frame=r->frame;
	wtk_wfst_path_t *pth;
	int cnt;

	if(!net->start){goto end;}
	//wtk_debug("end=%p\n",net->end);
	net->last_frame=frame;
	wtk_fst_net3_update_prune_eof(net);
	end=wtk_fst_net3_pop_node(net,frame,NULL,NULL);
	end->arc_like=0;
	end->ac_like=0;
	end->lm_like=0;
	//end->eof=1;
	wtk_fst_node_set_eof(end,1);
	for(cnt=0,n=r->inst_q.pop;n;n=n->next)
	{
		inst=data_offset(n,wtk_fst_inst_t,inst_n);
		for(i=1;i<=r->hmmset->max_hmm_state;++i)
		{
			tokset=inst->states+i;
			if(!tokset->path)
			{
				continue;
			}
			pth=tokset->path;
			if(pth->trans->to_state->custom)
			{
				continue;
			}
			if(pth->hook)
			{
				//arc=(wtk_fst_arc_t*)pth->hook;
				//fn=arc->to_node;
				//fn=(wtk_fst_node_t*)pth->hook;
				fn=wtk_fst_path_get_lat_node(pth);
				arc_like=0;
				ac_like=0;
				lm_like=0;
			}else
			{
				/*
				if(!r->cfg->use_lat_prev_wrd)
				{
					continue;
				}*/
				while(pth)
				{
					if(pth->trans->out_id>0)
					{
						break;
					}
					pth=pth->prev;
				}
				fn=wtk_fst_path_get_lat_node(pth);
				//fn=(wtk_fst_node_t*)pth->hook;
				if(!fn)
				{
					en=NULL;
					from_inst=wtk_fst_net3_get_trans_inst(net,pth->trans);
					//from_inst=(wtk_fst_lat_inst_t*)pth->trans->to_state->hook;
					if(from_inst)
					{
						for(qn=from_inst->list_q.pop;qn;qn=qn->next)
						{
							fn=data_offset(qn,wtk_fst_node_t,inst_n);
							if(fn->frame==pth->frame)
							{
								en=fn;
								break;
							}
						}
					}
					fn=en;
					if(!fn)
					{
						continue;
					}
				}
				ac_like=tokset->path->ac_like-pth->ac_like;
				lm_like=tokset->path->lm_like-pth->lm_like;
#ifdef USE_EXT_LIKE
				arc_like=tokset->path->raw_like-pth->raw_like;
#else
				arc_like=ac_like+lm_like;//tokset->path->ac_like+tokset->path->lm_like-pth->ac_like-pth->lm_like;
#endif

			}
			++cnt;
			wtk_fst_net3_add_link_arc(net,fn,end,0,0,arc_like,ac_like,lm_like,frame);
		}
	}
	if(cnt==0)
	{
		//exit(0);
		goto end;
	}
	net->end=end;
end:
	return;
}
int wtk_wfstr_prune_lat(wtk_wfstr_t *r,float beam,int hg_nodes)
{
	wtk_fst_net3_t *net=r->lat_net;
	wtk_queue_node_t *n;
	wtk_queue_node_t *qn;
	wtk_fst_lat_inst_t *from_inst;
	wtk_fst_inst_t *inst;
	wtk_fst_node_t *fn,*en;
	wtk_fst_node_t *end,*nil;
	wtk_slist_node_t *sn;
	float arc_like,ac_like,lm_like;
	wtk_fst_node_item_t *item;
	wtk_fst_tok_t *tokset;
	//wtk_fst_arc_t *arc;
	int i;
	int frame=r->frame;
	wtk_wfst_path_t *pth;
	int cnt;
	int ret=-1;

	if(!net->start){goto end;}
	//wtk_debug("end=%p\n",net->end);
	net->last_frame=frame;
	wtk_fst_net3_update_prune_eof(net);
	end=wtk_fst_net3_pop_node(net,frame,NULL,NULL);
	end->arc_like=0;
	end->ac_like=0;
	end->lm_like=0;
	//end->eof=1;
	wtk_fst_node_set_eof(end,1);
	for(cnt=0,n=r->inst_q.pop;n;n=n->next)
	{
		inst=data_offset(n,wtk_fst_inst_t,inst_n);
		for(i=1;i<=r->hmmset->max_hmm_state;++i)
		{
			tokset=inst->states+i;
			if(!tokset->path)
			{
				continue;
			}
			pth=tokset->path;
			if(pth->trans->to_state->custom)
			{
				continue;
			}
			if(pth->hook)
			{
				//arc=(wtk_fst_arc_t*)pth->hook;
				//fn=arc->to_node;
				//fn=(wtk_fst_node_t*)pth->hook;
				fn=wtk_fst_path_get_lat_node(pth);
				arc_like=0;
				ac_like=0;
				lm_like=0;
			}else
			{
				/*
				if(!r->cfg->use_lat_prev_wrd)
				{
					continue;
				}*/
				while(pth)
				{
					if(pth->trans->out_id>0)
					{
						break;
					}
					pth=pth->prev;
				}
				fn=wtk_fst_path_get_lat_node(pth);
				//fn=(wtk_fst_node_t*)pth->hook;
				if(!fn)
				{
					en=NULL;
					from_inst=wtk_fst_net3_get_trans_inst(net,pth->trans);
					//from_inst=(wtk_fst_lat_inst_t*)pth->trans->to_state->hook;
					if(from_inst)
					{
						for(qn=from_inst->list_q.pop;qn;qn=qn->next)
						{
							fn=data_offset(qn,wtk_fst_node_t,inst_n);
							if(fn->frame==pth->frame)
							{
								en=fn;
								break;
							}
						}
					}
					fn=en;
					if(!fn)
					{
						continue;
					}
				}
				ac_like=tokset->path->ac_like-pth->ac_like;
				lm_like=tokset->path->lm_like-pth->lm_like;
#ifdef USE_EXT_LIKE
				arc_like=tokset->path->raw_like-pth->raw_like;
#else
				arc_like=ac_like+lm_like;//tokset->path->ac_like+tokset->path->lm_like-pth->ac_like-pth->lm_like;
#endif

			}
			++cnt;
			wtk_fst_net3_add_link_arc(net,fn,end,0,0,arc_like,ac_like,lm_like,frame);
		}
	}
	if(cnt==0)
	{
		//exit(0);
		goto end;
	}
	net->end=end;
	wtk_slist_init(&(net->list_q));
	item=(wtk_fst_node_item_t*)wtk_heap_malloc(net->heap,sizeof(wtk_fst_node_item_t));
	item->node=end;
	end->leaf=1;
	wtk_slist_push(&(net->list_q),&(item->list_n));
#ifdef DUEB_LAT
	{
		static int cnt=0;

		++cnt;
		wtk_debug("cnt=%d\n",cnt);
		if(cnt==2)
		{
			double t;

			net->end=end;
			net->eof=1;
			end->eof=1;
			net->last_frame=r->frame;
			wtk_fst_net3_finish(net);

			//wtk_fst_net3_print_next_arc(net,net->start,100);

			t=time_get_ms();
			wtk_fst_net3_write_lat(net,"m2.lat");
			t=time_get_ms()-t;
			wtk_debug("lat time: %f\n",t);
			exit(0);
		}
	}
#endif
	wtk_fst_net3_collect_node(net,net->start);
	wtk_fst_net3_remove_eps(net);

	if(beam>0)
	{
		nil=wtk_fst_net3_pop_node(net,frame,NULL,NULL);
		nil->arc_like=0;
		nil->ac_like=0;
		nil->lm_like=0;
		//nil->eof=1;
		wtk_fst_node_set_eof(nil,1);
		for(sn=net->list_q.prev;sn;sn=sn->prev)
		{
			item=data_offset(sn,wtk_fst_node_item_t,list_n);
			//item->node->eof=0;
			wtk_fst_node_set_eof(item->node,0);
			wtk_fst_net3_pop_arc(net,item->node,nil,NULL,0,0,0,r->frame);
		}
		net->null_node=nil;
		wtk_fst_net3_prune_node(net,net->start,beam,hg_nodes);
		wtk_fst_net3_remove_unlink_path(net);
		if(net->null_node ==nil)
		{
			wtk_fst_net_remove_node_from_prev(net,nil);
			wtk_fst_net3_push_node(net,nil);
		}
		if(net->end == end)
		{
			wtk_fst_net_remove_node_from_prev(net,end);
			wtk_fst_net3_push_node(net,end);
			net->end=NULL;
		}
	}else
	{
		wtk_fst_net3_remove_unlink_path(net);
		for(sn=net->list_q.prev;sn;sn=sn->prev)
		{
			item=data_offset(sn,wtk_fst_node_item_t,list_n);
			//item->node->eof=0;
			wtk_fst_node_set_eof(item->node,0);
		}
		if(net->end==end)
		{
			//wtk_fst_net3_remove_path(net,end);
			wtk_fst_net_remove_node_from_prev(net,end);
			wtk_fst_net3_push_node(net,end);
			net->end=NULL;
		}
	}
	ret=0;
end:
	net->null_node=0;
	net->end=0;
	wtk_slist_init(&(net->list_q));
	wtk_heap_reset(net->heap);
	return ret;
}
