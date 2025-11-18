#include "wtk_lat_net.h"

wtk_lat_net_t* wtk_lat_net_new()
{
	wtk_lat_net_t *n;

	n=(wtk_lat_net_t*)wtk_malloc(sizeof(wtk_lat_net_t));
	n->input=NULL;
	n->output=NULL;
	n->heap=wtk_heap_new(4096);
	return n;
}

void wtk_lat_net_delete(wtk_lat_net_t *n)
{
	wtk_heap_delete(n->heap);
	wtk_free(n);
}

void wtk_lat_net_reset(wtk_lat_net_t *n)
{
	wtk_heap_reset(n->heap);
}

void wtk_lat_net_link(wtk_fst_trans2_t *trans,wtk_larc_t *arc)
{
	//wtk_debug("trans=%p n=%p\n",trans,arc->end);
	trans->hook2=arc;//n->info.word;
	arc->end->hook=trans;
	//n->hook=trans;
}

wtk_fst_trans2_t* wtk_fst_state2_find_next_hook(wtk_fst_state2_t *s,int in_id,int out_id,void *hook2)
{
	wtk_fst_trans2_t *trans;

	for(trans=(wtk_fst_trans2_t*)s->v.trans;trans;trans=(wtk_fst_trans2_t*)trans->hook.next)
	{
		if(trans->in_id==in_id && trans->out_id==out_id && trans->hook2==hook2)
		{
			return (wtk_fst_trans2_t*)trans;
		}
	}
	return NULL;
}

int wtk_lnode_has_same_output2(wtk_lnode_t *n1,wtk_lnode_t *n2)
{
	if(n1->foll && !n1->foll->farc && n1->foll && !n2->foll->farc)
	{
		return wtk_lnode_has_same_output(n1,n2);
	}else
	{
		return 0;
	}
}

/**
 *        /(五:1) ->(十:2)   ->a
 *    (开)
 *        \(五:3) ->(十:4)   ->b
 *
 *  merge_parent 合并的父节点
 *  当走完a 路径，开始走b路径 merge_parent为(五:1)
 */
void wtk_lat_net_process_node(wtk_lat_net_t *ln,wtk_fst_state2_t *s,wtk_lnode_t *n,
		wtk_lnode_t *merge_parent)
{
	wtk_fst_net2_t *output=ln->output;
	wtk_fst_trans2_t  *tt;
	wtk_fst_state2_t *ts;
	wtk_lnode_t *tn;
	wtk_larc_t *arc;
	//wtk_string_t *v;
	int id;
	int b;

	if(!n->foll)
	{
		//wtk_debug("attach end\n");
		tt=wtk_fst_net2_link_state(output,s,output->end,0,0,0,0,0);
		//wtk_fst_net_print_trans_prev(output->print,tt);
		//wtk_fst_net_print_trans_next(output->print,tt);
		//void wtk_fst_net_print_trans_prev(wtk_fst_net_print_t *p,wtk_fst_trans2_t *trans);
		//void wtk_fst_net_print_trans_next(wtk_fst_net_print_t *p,wtk_fst_trans2_t *trans);
		//exit(0);
		return;
	}
	for(arc=n->foll;arc;arc=arc->farc)
	{
		if(arc->end->info.word)
		{
			id=(long)arc->end->info.word->aux;
			//v=arc->end->info.word->name;
			//wtk_debug("[%.*s] end=%p\n",v->len,v->data,arc->end);
			if(arc->end->hook)
			{
				/*
				 *   如果节点已经访问过，
				 *   1) 如果input_trans的起始节点只有一个后继，虚连接到input_trans起始节点
				 *   2) 如果input_trans的起始节点有多个后继,如果后继是当前后继的子集，虚连接到input_trans起始节点
				 *   3) 如果input_trans的起始节点有多个后接，如果后继不是当前后继的子集，新建trans到input_trans的结束节点
				 */
				tt=(wtk_fst_trans2_t*)arc->end->hook;
				if(!tt->from_state->v.trans->hook.next)
				{
					wtk_fst_net2_link_state(output,s,(wtk_fst_state2_t*)tt->from_state,0,0,0,0,0);
				}else
				{
					tn=((wtk_larc_t*)tt->hook2)->start;//(wtk_lnode_t*)tt->hook2;
					/*
					if(0 && wtk_fst_state2_has_eps_to((wtk_fst_state2_t*)tt->from_state,s))
					{
						b=0;
					}else
					{
						b=wtk_lnode_has_same_output(tn,arc->start);
					}*/
					//b=wtk_lnode_has_same_output(tn,arc->start);
					/*
					if(tn->foll && !tn->foll->farc && arc->start->foll && !arc->start->foll->farc)
					{
						b=wtk_lnode_has_same_output(tn,arc->start);
					}else
					{
						b=0;
					}*/
					b=wtk_lnode_has_same_output2(tn,arc->start);
					if(b)
					{
						wtk_fst_net2_link_state(output,s,(wtk_fst_state2_t*)tt->from_state,0,0,0,0,0);
						//same output jump
						arc=NULL;
						break;
					}else
					{
						tt=wtk_fst_net2_link_state(output,s,(wtk_fst_state2_t*)tt->to_state,0,id,id,0,0);
						tt->hook2=arc;//->end->info.word;
					}
				}
			}else
			{
				/**
				 *   如果某两个节点的起始节点都相同则合并;
				 * 1)    / (机) -> (你)                 / (你)
				 *	 (电)               =>  (电) -> (机)
				 *	     \ (机) -> (我)                 \ (我)
				 *
				 * 2)  (猜)
				 *       \
				 *       / (机) -> (你)
				 *   (电)                 => 不合并
				 *       \ (机) -> (我)
				 */
				tt=wtk_fst_state2_find_next_hook(s,id,id,NULL);
				if(tt)
				{
					if(!merge_parent)
					{
						tn=((wtk_larc_t*)tt->hook2)->end;
						b=wtk_lnode_has_same_input(tn,arc->end);
						if(b)
						{
							//相同起始节点
							if(arc->end->foll->farc)
							{
								//如果多个后继节点，不使用合并
								tn=NULL;
							}
							wtk_lat_net_process_node(ln,(wtk_fst_state2_t*)tt->to_state,arc->end,tn);
						}else
						{
							ts=wtk_fst_net2_pop_state(output);
							tt=wtk_fst_net2_link_state(output,s,ts,0,id,id,0,0);
							wtk_lat_net_link(tt,arc);
							wtk_lat_net_process_node(ln,ts,arc->end,NULL);
						}
					}else
					{
						if(arc->end->foll->farc)
						{
							tn=NULL;
						}else
						{
							tn=merge_parent;
						}
						//如果多个后继节点，不使用合并
						wtk_lat_net_process_node(ln,(wtk_fst_state2_t*)tt->to_state,arc->end,tn);
					}
				}else
				{
					ts=wtk_fst_net2_pop_state(output);
					tt=wtk_fst_net2_link_state(output,s,ts,0,id,id,0,0);
					wtk_lat_net_link(tt,arc);
					wtk_lat_net_process_node(ln,ts,arc->end,NULL);
				}
			}
		}else
		{
			if(arc->end->hook)
			{
				tt=(wtk_fst_trans2_t*)arc->end->hook;

				wtk_fst_net2_link_state(output,s,(wtk_fst_state2_t*)tt->to_state,0,0,0,0,0);
				//exit(0);
			}else
			{
				ts=wtk_fst_net2_pop_state(output);
				tt=wtk_fst_net2_link_state(output,s,ts,0,0,0,0,0);
				wtk_lat_net_link(tt,arc);
				wtk_lat_net_process_node(ln,ts,arc->end,NULL);
			}
		}
	}
}

void wtk_lat_net_process(wtk_lat_net_t *ln,wtk_lat_t *input,wtk_fst_net2_t *output)
{
	wtk_lnode_t *n;
	wtk_fst_state2_t *s;
	//wtk_fst_trans2_t *trans;
	int id;

	wtk_lat_clean_node_hook(input);
	ln->input=input;
	ln->output=output;

	output->start=wtk_fst_net2_pop_state(output);
	output->end=wtk_fst_net2_pop_state(output);
	output->end->type=WTK_FST_FINAL_STATE;

	n=wtk_lat_start_node(ln->input);
	if(n->info.word)
	{
		s=wtk_fst_net2_pop_state(output);
		id=(long)n->info.word->aux;
		wtk_fst_net2_link_state(output,output->start,s,0,id,id,0,0);
		//wtk_lat_net_link(trans,n);
	}else
	{
		s=output->start;
	}
	wtk_lat_net_process_node(ln,s,n,NULL);
	//wtk_debug("N=%d L=%d\n",output->state_id,output->trans_id);
	//exit(0);
}
