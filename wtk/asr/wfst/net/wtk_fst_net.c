#include "wtk_fst_net.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/asr/wfst/net/kv/wtk_fst_binet.h"
#ifdef WIN32
#include <float.h>
#endif
wtk_fst_state_t* wtk_fst_net_get_fst_state(wtk_fst_net_t *net,unsigned int id);
void wtk_fst_net_set_fst_state(wtk_fst_net_t *net,unsigned int id,wtk_fst_state_t *state);

//#define DEBUG_TIME
//#define DEBUG_FTIME

#ifdef __ANDROID__
#define USE_CHAR_NET
#endif

void wtk_fst_net_load_state_from_bin2(wtk_fst_net_t *net,wtk_fst_state_t *state)
{
	wtk_fst_net_cfg_t *cfg=net->cfg;
	wtk_string_t v;
	int ret;
	char *p;
	int i,j,len;
	unsigned int in,n;
	unsigned int out,to;
	float prob;
	wtk_fst_trans_t *trans;
	//int last_id;
	//char *e;
#ifdef DEBUG_FTIME
	double t,t2;

	t=time_get_ms();
#endif
	ret=wtk_fst_binet_get(net->v.bin,state->id,&v);
	if(ret!=0)
	{
		wtk_debug("found err ret=%d state[%d]\n",ret,state->id);
		exit(0);
		return;
	}
#ifdef DEBUG_FTIME
	t2=time_get_ms();
	t=t2-t;
	net->ftime+=t;
#endif

	//print_char((unsigned char*)v.data,v.len);
	//B[count] B[len] H[in]I[out]I[to]f[prob]
	//B[count] B[len=0]
	//print_hex(v.data,v.len);
	net->bytes+=v.len;
	p=v.data;
	//e=p+v.len;
	n=*((unsigned int *)p);
	//wtk_debug("state id=%d %p n=%d\n",state->id, state, n);
	p+=sizeof(unsigned int);
	state->v.trans=NULL;
	state->ntrans=n;
	//state->v.array.trans_v=0;
	//state->v.array.n=n;
	if(state->type==WTK_FST_TOUCH_STATE)
	{
		state->type=WTK_FST_NORM_STATE;
	}
	state->custom=0;
	j=0;
	//last_id=-1;
	for(i=0;i<n;++i)
	{
		len=*((unsigned char*)p);
		p+=sizeof(unsigned char);
		//wtk_debug("len=%d/%d idx=%d/%d id=%d\n",len,v.len,i,n,state->id);
		switch(len)
		{
		case 3:
			//fall through;
		case 4:
			in=*((unsigned int*)p);
			p+=sizeof(unsigned int);

			out=*((unsigned int*)p);
			p+=sizeof(unsigned int);

			to=*((unsigned int*)p);
			p+=sizeof(unsigned int);
			if(len==4)
			{
#ifndef USE_CHAR_NET
				prob=*(float*)p;
#else
				memcpy((void*)&prob,(void*)p,sizeof(float));
#endif
				p+=sizeof(float);
			}else
			{
				prob=0;
			}
			prob=-prob*cfg->lmscale+((out>0)?cfg->wordpen:0);//?
			//wtk_debug("to=%d, in=%d, out=%d, weight=%f\n",to,in,out,prob);
			if(!state->v.trans)
			{
				if(net->cfg->use_cheap)
				{
					state->v.trans=(wtk_fst_trans_t*)wtk_malloc(sizeof(wtk_fst_trans_t)*n);
				}else
				{
					state->v.trans=(wtk_fst_trans_t*)wtk_heap_malloc(net->heap,sizeof(wtk_fst_trans_t)*n);
				}
			}
			trans=state->v.trans+j;
			++j;
			trans->hook.inst=NULL;
			trans->to_state=wtk_fst_net_get_fst_state(net,to);
			trans->in_id=in;
			trans->out_id=out;
			trans->weight=prob;
			/*
			if(net->cfg->nwlp)
			{
				trans->weight+=net->cfg->nwlp->probs[trans->to_state->id];
			}*/
			//last_id=out;
			break;
		case 0:
			//fall through;
		case 1:
			if(len==1)
			{
#ifndef USE_CHAR_NET
				prob=*(float*)p;
#else
				memcpy((void*)&prob,(void*)p,sizeof(float));
#endif
				p+=sizeof(float);
			}else
			{
				prob=0;
			}
			state->type=WTK_FST_FINAL_STATE;
			state->v.weight=0;//-prob*cfg->lmscale;
			state->ntrans=0;
			state->custom=0;
			//wtk_debug("final=%d\n",state->id);
			break;
		default:
			//print_char((unsigned char*)v.data,v.len);
			//print_data(v.data,v.len);
			wtk_debug("i=%d/%d data=%d id=%d\n",i,n,v.len,state->id);
			wtk_debug("found err,len=%d\n",len);
			exit(0);
			break;
		}
	}
#ifdef ATTACH_NWLP
	if(net->cfg->nwlp)
	{
		state->nwlmp=net->cfg->nwlp->probs[state->id];
	}else
	{
		state->nwlmp=0;
	}
#endif
	/*
	if(state->type!=WTK_FST_FINAL_STATE && j!=n)
	{
		wtk_debug("found bug[j=%d,n=%d]\n",j,n);
		exit(0);
	}*/
	//exit(0);
#ifdef DEBUG_FTIME
	t=time_get_ms()-t2;
	net->xtime+=t;
#endif
}

/*
1) **
  0: final state with weight 0
  1: final state with weight columnt
  2: Input Symbol output symbol and to state wieght weight=0
  3: input symbol output symbol and to_state wieght
3) to state: **
   0:  1 bytes;
   1:  2 bytes;
   2:  3 bytes;
   3:  4 bytes;
2) ****:
   00: input symbol:
       0: eps=>0
       1: 1 bytes
       2: 2 bytes
       3: euqal output symbol
   00: output symbol:
       0: eps => 0
       1: 1 bytes
       2: 2 bytes
       3: 3 bytes
*/

void wtk_fst_net_load_state_from_bin(wtk_fst_net_t *net,wtk_fst_state_t *state)
{	
	
	if(state->load==1)
	{
		return;
	}else
	{
		state->load=1;
	}

	wtk_fst_net_cfg_t *cfg=net->cfg;
	wtk_string_t v;
	int ret;
	char *p;
	int i,j,len;
	unsigned int in,n;
	unsigned int out,to;
	unsigned char c;
	float prob;
	wtk_fst_trans_t *trans;
	//int last_id;
	//char *e;
	int len_type,to_type,in_type,out_type;
#ifdef DEBUG_FTIME
	double t,t2;

	t=time_get_ms();
#endif

	//wtk_debug("lm=%f\n",net->cfg->lmscale);
	ret=wtk_fst_binet_get(net->v.bin,state->id,&v);
	if(ret!=0)
	{
		wtk_debug("found err ret=%d state[%d]\n",ret,state->id);
		exit(0);
		return;
	}
	//print_hex(v.data,v.len);
#ifdef DEBUG_FTIME
	t2=time_get_ms();
	t=t2-t;
	net->ftime+=t;
#endif
	in=out=0;
	//len_type=to_type=in_type=out_type=0;
	//print_char((unsigned char*)v.data,v.len);
	//B[count] B[len] H[in]I[out]I[to]f[prob]
	//B[count] B[len=0]
	//print_hex(v.data,v.len);
	net->bytes+=v.len;
	p=v.data;
	//e=p+v.len;
	n=*((unsigned int *)p);
	//wtk_debug("state id=%d %p n=%d\n", state->id, state, n);
	p+=sizeof(unsigned int);
	state->v.trans=NULL;
	state->ntrans=n;
	state->custom=0;
	if(state->type==WTK_FST_TOUCH_STATE)
	{
		state->type=WTK_FST_NORM_STATE;
	}
	j=0;
	//last_id=-1;
	for(i=0;i<n;++i)
	{
		len=*((unsigned char*)p);
		len_type=len>>6&0x03;
		to_type=len>>4&0x03;
		in_type=len>>2&0x03;
		out_type=len&0x03;
		//wtk_debug("%#x/%#x/%#x/%#x/%#x\n",len,len_type,to_type,in_type,out_type);
		p+=sizeof(unsigned char);
		//wtk_debug("len=%d/%d idx=%d/%d id=%d\n",len,v.len,i,n,state->id);
		/*
		 *   0: final state with weight 0
  	  	 * 	 1: final state with weight columnt
         *   2: Input Symbol output symbol and to state wieght weight=0
         *   3: input symbol output symbol and to_state wieght
		 */
//		wtk_debug("len type:%d\n",len_type);
		switch(len_type)
		{
		case 0:
		//wtk_debug("0\n");
		state->type=WTK_FST_FINAL_STATE;//????
		//state->ntrans=0;             //fixed bug; avoid covering when normal state and final state in same state. by dmd
		state->ntrans--;               //fixed bug; need drop one when it only means final state, not trans.
		state->custom=0;
		break;	//fall through;
		case 1:
	//	wtk_debug("1\n");	//fall through;
			if(len_type==1)
			{
#ifndef USE_CHAR_NET
				prob=*(float*)p;
#else
				memcpy((void*)&prob,(void*)p,sizeof(float));
#endif
				//wtk_debug("%f\n",prob);
				p+=sizeof(float);
			}else
			{
				prob=0;
			}
			state->type=WTK_FST_FINAL_STATE;//????
			state->weight=prob*cfg->lmscale;
			state->ntrans--;
			state->custom=0;
			//wtk_debug("final=%d\n",state->id);
			break;
		case 2:
		//wtk_debug("2\n");	//fall through;
			//fall through;
		case 3:
		//wtk_debug("3\n");	//fall through;
			//wtk_debug("len=%d/%d\n",(int)(p-v.data),v.len);
			/*
		       0: eps=>0
		       1: 1 bytes
		       2: 2 bytes
		       3: euqal output symbol
		    */
			switch(in_type)
			{
			case 0:
				in=0;
				break;
			case 1:
				in=*((unsigned char*)p);
				p+=sizeof(unsigned char);
				break;
			case 2:
				in=*((unsigned short*)p);
				p+=sizeof(unsigned short);
				break;
			case 3:
                                in=*((unsigned int*)p);
                                p+=sizeof(unsigned int);
				break;
			default:
				wtk_debug("unsppored\n");
				exit(0);
				break;
			}
			/*
			   0: eps => 0
			   1: 1 bytes
			   2: 2 bytes
			   3: 3 bytes
			 */
			//wtk_debug("out=%d\n",out_type);
			switch(out_type)
			{
			case 0:
				out=0;
				break;
			case 1:
				out=*((unsigned char*)p);
				p+=sizeof(unsigned char);
				break;
			case 2:
				out=*((unsigned short*)p);
				p+=sizeof(unsigned short);
				break;
			case 3:
                                out=*((unsigned int*)p);
                                p+=sizeof(unsigned int);
                                break;			
			default:
				wtk_debug("unsppored\n");
				exit(0);
				break;
			}
		//	if(in_type==3)
		//	{
		//		in=out;
		//	}
			/*
			 *     0:  1 bytes;
				   1:  2 bytes;
				   2:  3 bytes;
				   3:  4 bytes;
			 */
			//wtk_debug("len=%d/%d\n",(int)(p-v.data),v.len);
			switch(to_type)
			{
			case 0:
				to=*((unsigned char*)p);
				p+=sizeof(unsigned char);
				break;
			case 1:
				to=*((unsigned short*)p);
				p+=sizeof(unsigned short);
				break;
			case 2:
				c=*((unsigned char*)p);
				p+=sizeof(unsigned char);
				to=*((unsigned short*)p);
				to+=c<<16;
				p+=sizeof(unsigned short);
				break;
			case 3:
				to=*((unsigned int*)p);
				p+=sizeof(unsigned int);
				break;
			default:
				wtk_debug("unsppored\n");
				exit(0);
				break;
			}
			//wtk_debug("len=%d/%d\n",(int)(p-v.data),v.len);
			if(len_type==3)
			{
#ifndef USE_CHAR_NET
				prob=*(float*)p;
#else
				memcpy((void*)&prob,(void*)p,sizeof(float));
#endif
				p+=sizeof(float);
			}else
			{
				prob=0;
			}
			//wtk_debug("len=%d/%d\n",(int)(p-v.data),v.len);
			prob=-prob*cfg->lmscale+((out>0)?cfg->wordpen:0);//?
			//wtk_debug("lmscale=%f prob=%f\n",cfg->lmscale,prob);
			//wtk_debug("from= %d to= %d in= %d out= %d weight= %f\n",state->id, to,in,out,prob);
			if(!state->v.trans)
			{	//wtk_debug("1111111111111111\n");
				if(net->cfg->use_cheap)
				{
					//state->v.trans=(wtk_fst_trans_t*)wtk_malloc(sizeof(wtk_fst_trans_t)*state->ntrans);
					state->v.trans=(wtk_fst_trans_t*)wtk_calloc(sizeof(wtk_fst_trans_t), state->ntrans); //avoid null trans info by dmd.
				}else
				{
					//state->v.trans=(wtk_fst_trans_t*)wtk_heap_malloc(net->heap,sizeof(wtk_fst_trans_t)*state->ntrans);
					state->v.trans=(wtk_fst_trans_t*)wtk_heap_zalloc(net->heap,sizeof(wtk_fst_trans_t)*state->ntrans);//avoid null trans info by dmd.
				}
			}
			trans=state->v.trans+j;
//			wtk_debug("%p %p\n",state,state->v.trans);
			++j;
			trans->hook.inst=NULL;
			trans->to_state=wtk_fst_net_get_fst_state(net,to);
			trans->in_id=in;
			trans->out_id=out;
			trans->weight=prob;
			//wtk_debug("from= %d to= %d in= %d out= %d weight= %f\n",state->id, trans->to_state->id,trans->in_id,trans->out_id, trans->weight);
			/*
			if(net->cfg->nwlp)
			{
				trans->weight+=net->cfg->nwlp->probs[trans->to_state->id];
			}*/
			//last_id=out;
//	wtk_debug("===========%p %p %p\n",state,state->v.trans,state->v.trans+1);
			break;
		default:
			wtk_debug("default\n");
			//prpint_char((unsigned char*)v.data,v.len);
			//print_data(v.data,v.len);
			wtk_debug("i=%d/%d data=%d id=%d\n",i,n,v.len,state->id);
			wtk_debug("found err,len=%d\n",len);
			exit(0);
			break;
		}
//	wtk_debug("===========%p %p %p\n",state,state->v.trans,state->v.trans+1);
	}
//	wtk_debug("===========%p %p %p\n",state,state->v.trans,state->v.trans+1);
#ifdef ATTACH_NWLP
	if(net->cfg->nwlp)
	{
		state->nwlmp=net->cfg->nwlp->probs[state->id];
	}else
	{
		state->nwlmp=0;
	}
#endif
#ifdef DEBUG_FTIME
	t=time_get_ms()-t2;
	net->xtime+=t;
#endif
//	wtk_debug("===========%p %p %p\n",state,state->v.trans,state->v.trans+1);
}

//------------------------------------------------------------------------

wtk_fst_net_t* wtk_fst_net_new(wtk_fst_net_cfg_t *cfg)
{
	wtk_fst_net_t *net;
	int n;

	net=(wtk_fst_net_t*)wtk_calloc(1,sizeof(wtk_fst_net_t));
	net->cfg=cfg;

	net->print=NULL;
	net->init_state=0;
	//wtk_debug("use_bin=%d\n",cfg->use_bin);
	net->heap=wtk_heap_new(4096);
	net->rbin_states=NULL;
	net->nrbin_states=0;
	//wtk_debug("bin=%p\n",net->v.bin);
	n=cfg->bin.ndx;
	n=(n/(cfg->hash_scale*2))*2+1;
	if(n<cfg->min_hash_size)
	{
		n=cfg->min_hash_size;
	}
	if(net->cfg->use_shash)
	{
		net->type=WTK_FST_NET_IDX_HASH;
		net->idx.hash=wtk_shash_new(n);
	}else if(1)
	{
		net->type=WTK_FST_NET_IDX_HINT;
		net->idx.hint.n=n;
		net->idx.hint.valid=0;
		net->idx.hint.p=wtk_calloc(n,sizeof(void*));
	}else
	{
		net->type=WTK_FST_NET_IDX_ARRAY;
		net->idx.array=wtk_harray_new((int)(cfg->bin.ndx*1.0/cfg->array_nslot2+0.5),cfg->array_nslot2);
	}
	net->count=0;
	net->time=0;
	net->bytes=0;
	net->ftime=0;
	net->xtime=0;
	if(cfg->use_bin)
	{
		net->v.bin=wtk_fst_binet_new(&(cfg->bin));
	}
	net->use_rbin=cfg->use_rbin;
	if(cfg->load_all)
	{
		wtk_fst_net_load_all(net);
	}
	return  net;
}

void wtk_fst_net_delete(wtk_fst_net_t *net)
{
	//wtk_debug("delete %p\n",net);
	//wtk_heap_delete(net->heap);
	if(net->rbin_states)
	{
		wtk_free(net->rbin_states);
	}
	wtk_heap_delete(net->heap);
	if(net->cfg->use_bin)
	{
		wtk_fst_binet_delete(net->v.bin);
	}
	switch(net->type)
	{
	case WTK_FST_NET_IDX_HASH:
		wtk_shash_delete(net->idx.hash);
		break;
	case WTK_FST_NET_IDX_ARRAY:
		wtk_harray_delete(net->idx.array);
		break;
	case WTK_FST_NET_IDX_HINT:
		wtk_free(net->idx.hint.p);
		break;
	}
	wtk_free(net);
}

void wtk_fst_net_touch_node(wtk_fst_net_t *net,wtk_fst_state_t *state)
{
	wtk_fst_trans_t *trans;
	int i;

	for(i=0,trans=state->v.trans;i<state->ntrans;++i,++trans)
	{
		//printf("%d %d %d %d\n", state->id, trans->to_state->id, trans->in_id, trans->out_id);
		if(trans->to_state->type==WTK_FST_TOUCH_STATE)
		{
			wtk_fst_net_load_state(net,trans->to_state);
			if(trans->to_state->type==WTK_FST_TOUCH_STATE)
			{
				wtk_debug("found bug\n");
				exit(0);
			}
			wtk_fst_net_touch_node(net,trans->to_state);
		}else
		{
			//printf("%d\n", trans->to_state->id);
		}
	}
}


void wtk_fst_net_load_all(wtk_fst_net_t *net)
{
	//wtk_debug("load all\n");
	net->init_state=wtk_fst_net_get_load_state(net,0);
	wtk_fst_net_touch_node(net,net->init_state);
	//wtk_debug("tot=%d\n",net->count);
	//exit(0);
}



void wtk_fst_state_init(wtk_fst_state_t *state,wtk_fst_state_id_t id)
{
	static wtk_fst_state_t s={
			{NULL},
			NULL,
			{0},
			0,
			0,
//			0,
			WTK_FST_TOUCH_STATE
	};
	/*
#ifdef USE_FST_LIST_NODE
#else
	state->next=NULL;
#endif
	state->hook=0;
	state->v.trans=0;
	state->id=id;;
	state->ntrans=0;
	state->type=WTK_FST_TOUCH_STATE;
	*/
	*state=s;
	state->id=id;
	state->load=0;
	state->frame=-1;
}

wtk_fst_state_t* wtk_fst_net_new_state(wtk_fst_net_t *net,wtk_fst_state_id_t id)
{
	wtk_fst_state_t *state;

	state=(wtk_fst_state_t*)wtk_heap_malloc(net->heap,sizeof(wtk_fst_state_t));
	wtk_fst_state_init(state,id);
	return state;
}

#ifdef USE_FST_LIST_NODE

wtk_fst_state_t*  wtk_shash_get_fst_state(wtk_shash_t *h,unsigned int id)
{
	wtk_fst_state_t *tmp;
	wtk_slist_node_t *s;
	unsigned int index;

	index=id%h->nslot;
	s=h->slot[index].prev;
	//wtk_debug("v[%d/%d]=%p nslot=%d\n",id,index,s,h->nslot)
	while(s)
	{
		tmp=data_offset2(s,wtk_fst_state_t,q_n);
		if(tmp->id==id)
		{
			return tmp;
		}
		s=s->prev;
	}
	return NULL;
}

#endif

wtk_fst_state_t* wtk_fst_net_get_fst_state(wtk_fst_net_t *net,unsigned int id)
{
	unsigned int idx;
	wtk_fst_state_t *state;
	wtk_fst_net_hint_t *hint;
#ifdef USE_FST_LIST_NODE
	wtk_slist_node_t *n,**pn;
#else
	wtk_fst_state_t **pstate;
#endif
	//int i=0;

	//wtk_debug("get state %d\n",net->cfg->use_rbin);
	switch(net->type)
	{
	case WTK_FST_NET_IDX_HINT:
		hint=&(net->idx.hint);
		idx=id%(hint->n);
#ifdef USE_FST_LIST_NODE
		pn=(wtk_slist_node_t**)&(hint->p[idx]);
		n=*pn;
		while(n)
		{
			//++i;
			state=data_offset2(n,wtk_fst_state_t,q_n);
			//state=(wtk_fst_state_t*)n;
			if(state->id==id)
			{
				//wtk_debug("%d %p %d state\n",state->load,state,state->id);
				return state;
			}
			n=n->prev;
		}
		//printf("==> %d\n",i);
		++hint->valid;
		state=wtk_fst_net_new_state(net,id);
		state->q_n.prev=(wtk_slist_node_t*)*pn;//hint->p[idx];//net->idx.hint.p[idx];
		*pn=&(state->q_n);
		return state;
#else
		//state=hint->p[idx];
		pstate=(wtk_fst_state_t**)&(hint->p[idx]);
		state=*pstate;
		if(state)
		{
			if(state->id==id)
			{
				return state;
			}
			state=state->next;
			while(state)
			{
				if(state->id==id)
				{
					return state;
				}
				state=state->next;
			}
		}
		++hint->valid;
		state=wtk_fst_net_new_state(net,id);
		state->next=*pstate;//(wtk_fst_state_t*)hint->p[idx];
		//hint->p[idx]=state;
		*pstate=state;
		return state;
#endif
		break;
	case WTK_FST_NET_IDX_HASH:
#ifdef USE_FST_LIST_NODE
		state=wtk_shash_get_fst_state(net->idx.hash,id);
		if(state)
		{
			return state;
		}
		state=wtk_fst_net_new_state(net,id);
		wtk_fst_net_set_fst_state(net,id,state);
		return state;
#else
		return NULL;
#endif
		break;
	case WTK_FST_NET_IDX_ARRAY:
		state=wtk_harray_get(net->idx.array,id);
		if(state)
		{
			return state;
		}
		state=wtk_fst_net_new_state(net,id);
		wtk_fst_net_set_fst_state(net,id,state);
		return state;
		break;
	}
	return NULL;
}


void wtk_fst_net_set_fst_state(wtk_fst_net_t *net,unsigned int id,wtk_fst_state_t *state)
{
	switch(net->type)
	{
	case WTK_FST_NET_IDX_HASH:
#ifdef USE_FST_LIST_NODE
		wtk_shash_add(net->idx.hash,id,&(state->q_n));
#endif
		break;
	case WTK_FST_NET_IDX_ARRAY:
		wtk_harray_set(net->idx.array,id,(void*)state);
		break;
	case WTK_FST_NET_IDX_HINT:
		{
			unsigned int idx;

			idx=id%(net->idx.hint.n);
#ifdef USE_FST_LIST_NODE
			state->q_n.prev=(wtk_slist_node_t*)net->idx.hint.p[idx];//net->idx.hint.p[idx];
			net->idx.hint.p[idx]=&(state->q_n);
#else
			state->next=(wtk_fst_state_t*)net->idx.hint.p[idx];
			net->idx.hint.p[idx]=state;
#endif
			++net->idx.hint.valid;
		}
		break;
	}
}

wtk_fst_state_t* wtk_fst_net_get_load_state(wtk_fst_net_t *net,unsigned int id)
{
	wtk_fst_state_t *state;

	if(net->rbin_states >0)
	{
		//wtk_debug("???? %d %d %p\n",id,net->nrbin_states,&(net->rbin_states[id]));
		//return &(net->rbin_states[id]);
		state=&(net->rbin_states[id]);
	}
	else
	{
		state=wtk_fst_net_get_fst_state(net,id);
		//if(state->type!=WTK_FST_TOUCH_STATE)
		//{	wtk_debug("return state\n");
		//	return state;
		//}
		wtk_fst_net_load_state(net,state);
	}

	return state;
}


void wtk_fst_net_load_state(wtk_fst_net_t *net,wtk_fst_state_t *state)
{
//#define DEBUG_TIME
#ifdef DEBUG_TIME
	double t;

	//wtk_debug("load state=%d\n",state->id);
	t=time_get_ms();
#endif

//	wtk_debug("load state=%d\n",state->id);
	//wtk_debug("load state=%d\n",state->id);
	++net->count;
	//wtk_debug("use_array=%d\n",net->cfg->use_trans_array);
	//wtk_debug("load state start\n");
	if(net->cfg->bin.use_pack_bin)
	{
		wtk_fst_net_load_state_from_bin(net,state);
	}else
	{
		wtk_fst_net_load_state_from_bin2(net,state);
	}
	//wtk_debug("load state end\n");
	/*
	{
		static int max=0;
		static int min=0;

		if(state->ntrans>1)
		{
			++max;
		}else
		{
			++min;
		}
		wtk_debug("max/min=%d/%d\n",max,min);
	}*/
#ifdef DEBUG_TIME
	t=time_get_ms()-t;
	net->time+=t;
#endif
}

void wtk_fst_net_reset_state_hook(wtk_fst_state_t *state)
{
	wtk_fst_trans_t *trans;
	int i;

	state->hook=NULL;
	if(state->type==WTK_FST_NORM_STATE)
	{
		for(i=0;i<state->ntrans;++i)
		{
			trans=state->v.trans+i;
			trans->hook.inst=NULL;
			if(trans->to_state->hook)
			{
				wtk_fst_net_reset_state_hook(trans->to_state);
			}
		}
	}
}

void wtk_fst_net_reset_hook(wtk_fst_net_t *net)
{
	wtk_fst_net_reset_state_hook(net->init_state);
}

void wtk_fst_net_clean_trans_inst(wtk_fst_net_t *net)
{
	wtk_slist_node_t *sn;
	wtk_fst_state_t *state;
	wtk_fst_trans_t *trans;
	int i,j;

	for(i=0;i<net->idx.hint.n;++i)
	{
		sn=net->idx.hint.p[i];
		while(sn)
		{
			state=data_offset2(sn,wtk_fst_state_t,q_n);
			state->hook=NULL;
			for(trans=state->v.trans,j=0;j<state->ntrans;++j,++trans)
			{
				trans->hook.inst=NULL;
			}
			sn=sn->prev;
		}
	}
}

void wtk_fst_net_reset_heap(wtk_fst_net_t *net)
{
	net->xtime=0;
	net->bytes=0;
	net->count=0;
	net->time=0;
	net->ftime=0;
	net->init_state=0;
	wtk_heap_reset(net->heap);
}

void wtk_fst_net_reset_rbin(wtk_fst_net_t *net)
{
	wtk_fst_state_t *s;
	wtk_fst_trans_t *trans;
	int i,j;

	net->xtime=0;
	net->bytes=0;
	net->count=0;
	net->time=0;
	net->ftime=0;
	for(i=0;i<net->nrbin_states;++i)
	{
		s=net->rbin_states+i;
		s->hook=NULL;
		s->frame=-1;
		//wtk_debug("v[%d]=%d type=%d net=%p\n",i,s->ntrans,s->type,net);
		for(j=0,trans=s->v.trans;j<s->ntrans;++j,++trans)
		{
			trans->hook.inst=NULL;
		}
//		if(i==10)
//		{
//			exit(0);
//		}
	}
}

void wtk_fst_net_reset_idx_hook(wtk_fst_net_t *net)
{
	wtk_fst_net_hint_t *hint;
	wtk_slist_node_t *n;
	wtk_fst_state_t *state;
	int i,j;
	wtk_fst_trans_t *trans;

	if(!net->init_state)
	{
		return;
	}
	net->init_state->hook=NULL;
	for(j=0,trans=net->init_state->v.trans;j<net->init_state->ntrans;++j,++trans)
	{
		trans->hook.inst=NULL;
	}
	hint=&(net->idx.hint);
	for(i=0;i<hint->n;++i)
	{
		n=(wtk_slist_node_t*)hint->p[i];
		while(n)
		{
			state=data_offset2(n,wtk_fst_state_t,q_n);
			//if(state->hook)
			{
				state->hook=NULL;
				for(j=0,trans=state->v.trans;j<state->ntrans;++j,++trans)
				{
					trans->hook.inst=NULL;
				}
			}
			n=n->prev;
		}
	}
}

void wtk_fst_net_reset(wtk_fst_net_t *net)
{
	//wtk_debug(" reset use_rbin=%d use_bin=%d\n",net->use_rbin,net->cfg->use_bin);
	if(net->cfg->load_all)
	{
		if(net->type==WTK_FST_NET_IDX_HINT)
		{
			wtk_fst_net_reset_idx_hook(net);
		}
		return ;
	}
	if(net->use_rbin && net->nrbin_states>0)
	{
		wtk_fst_net_reset_rbin(net);
		return;
	}
	if(!net->cfg->use_bin)
	{
		switch(net->type)
		{
		case WTK_FST_NET_IDX_HINT:
			wtk_fst_net_clean_trans_inst(net);
			break;
		default:
			wtk_debug("not support\n");
			break;
		}
		/*
		if(net->rbin_states)
		{
			wtk_free(net->rbin_states);
			net->rbin_states=NULL;
		}*/
	}else
	{
		if(net->cfg->use_dynamic_reset)
		{
			if(net->cfg->reset_max_bytes<0 || net->bytes>=net->cfg->reset_max_bytes)
			{
				if(net->type!=WTK_FST_NET_IDX_HINT)
				{
					wtk_debug("found bug reset type=%d/%d n=%d\n",net->type,WTK_FST_NET_IDX_HINT,net->idx.hint.n);
					exit(0);
				}else
				{
//					double t;
//
//					t=time_get_ms();
					wtk_fst_net_reset_idx_hook(net);
//					t=time_get_ms()-t;
//					wtk_debug("reset bytes=%f/%f time=%f load=%d\n",net->bytes,net->cfg->reset_max_bytes,t,net->count);
				}
				return;
			}
		}
		wtk_fst_net_reset_heap(net);
		switch(net->type)
		{
		case WTK_FST_NET_IDX_HASH:
			wtk_shash_reset(net->idx.hash);
			break;
		case WTK_FST_NET_IDX_ARRAY:
			wtk_harray_reset(net->idx.array);
			break;
		case WTK_FST_NET_IDX_HINT:
			if(net->idx.hint.valid>0)
			{
				wtk_free(net->idx.hint.p);
				net->idx.hint.p=wtk_calloc(net->idx.hint.n,sizeof(void*));
				//memset(net->idx.hint.p,0,sizeof(void*)*net->idx.hint.n);
				net->idx.hint.valid=0;
			}
			break;
		default:
			break;
		}
	}
}

int wtk_fst_net_bytes(wtk_fst_net_t *net)
{
	int bytes;
        int t;
        switch(net->type)
	{
	case WTK_FST_NET_IDX_HASH:
		bytes=wtk_shash_bytes(net->idx.hash);
		break;
	case WTK_FST_NET_IDX_ARRAY:
		bytes=wtk_harray_bytes(net->idx.array);
		break;
	case WTK_FST_NET_IDX_HINT:
		bytes=net->idx.hint.n*sizeof(void*);
		break;
	default:
		bytes=0;
		break;
        }
        t=wtk_heap_bytes(net->heap);
	bytes+=t;
	//wtk_debug("tot=%fMB\n",bytes*1.0/(1024*1024));
	return bytes;
}

int wtk_fst_net_print(wtk_fst_net_t *net)
{
	int bytes;
	int t;

	//wtk_debug("use_bin=%d, use_db=%d\n",net->cfg->use_bin,net->cfg->use_db);
	switch(net->type)
	{
	case WTK_FST_NET_IDX_HASH:
		bytes=wtk_shash_bytes(net->idx.hash);
		wtk_debug("hash bytes: %fMB\n",bytes*1.0/(1024*1024));
		break;
	case WTK_FST_NET_IDX_ARRAY:
		bytes=wtk_harray_bytes(net->idx.array);
		wtk_debug("array bytes: %fMB\n",bytes*1.0/(1024*1024));
		break;
	case WTK_FST_NET_IDX_HINT:
		bytes=net->idx.hint.n*sizeof(void*);
		wtk_debug("hint bytes: %fMB\n",bytes*1.0/(1024*1024));
		break;
	default:
		bytes=0;
		break;
	}
	t=wtk_heap_bytes(net->heap);
	wtk_debug("heap bytes: %fMB\n",t*1.0/(1024*1024));
	bytes+=t;
	wtk_debug("bytes: %fMB\n",bytes*1.0/(1024*1024));
	wtk_debug("bin load time=%f\n",net->ftime);
	wtk_debug("xtime=%f\n",net->xtime);
	wtk_debug("state load time=%f\n",net->time);
	wtk_debug("cnt=%d fbytes=%.3fM\n",net->count,net->bytes*1.0/(1024*1024));
	wtk_debug("rate=%.3fM/s\n",net->bytes*1000.0/(1024*1024*net->ftime));
	if(net->cfg->nwlp)
	{
		wtk_debug("nwlp=%.3fM\n",wtk_fst_net_nwlp_bytes(net->cfg->nwlp)*1.0/(1024*1024));
	}
	return bytes;
}

void wtk_fst_net_print_trans(wtk_fst_net_t *net,wtk_fst_trans_t *trans)
{
	wtk_fst_net_cfg_t *cfg=net->cfg;
	wtk_string_t *v;

	wtk_debug("============== trans=%p ============\n",trans);
#ifdef USE_FILE
	printf("to: %d\n",trans->to_state);
#endif
	v=cfg->sym_in->ids[trans->in_id]->str;
	printf("in: [%d:%.*s]\n",trans->in_id,v->len,v->data);
	v=cfg->sym_out->strs[trans->out_id];
	printf("out: [%d:%.*s]\n",trans->out_id,v->len,v->data);
	printf("weight: %f\n",trans->weight);
}

void wtk_fst_net_print_state(wtk_fst_net_t *net,wtk_fst_state_t *state)
{
	wtk_fst_trans_t *trans;
	int i;

	if(state->type==WTK_FST_FINAL_STATE)
	{
		return;
	}
	//for(trans=state->v.trans;trans;trans=trans->hook.next)
	for(trans=state->v.trans,i=0;i<state->ntrans;++trans,++i)
	{
		wtk_debug("[%.*s:%.*s/%f %d=>%d]\n",
				net->cfg->sym_in->ids[trans->in_id]->str->len,
				net->cfg->sym_in->ids[trans->in_id]->str->data,
				net->cfg->sym_out->strs[trans->out_id]->len,
				net->cfg->sym_out->strs[trans->out_id]->data,
				trans->weight,
				state->id,trans->to_state->id);
		wtk_fst_net_print_state(net,trans->to_state);
	}
}

void wtk_fst_net_cfg_print_trans2(wtk_fst_net_cfg_t *cfg,wtk_fst_trans2_t *trans)
{
	wtk_fst_state2_t *s;

	s=(wtk_fst_state2_t*)trans->from_state;
	if(s && s->in_prev)
	{
		wtk_fst_net_cfg_print_trans2(cfg,s->in_prev);
	}
	wtk_debug("[%.*s]\n",cfg->sym_out->strs[trans->out_id]->len,
			cfg->sym_out->strs[trans->out_id]->data);
}

void wtk_fst_net_cfg_print_next2(wtk_fst_net_cfg_t *cfg,wtk_fst_trans_t *trans,int depth)
{
	wtk_fst_state_t *s;
	int i;

	for(i=0;i<depth;++i)
	{
		printf(" ");
	}
	printf("[%.*s:%.*s/%f]",cfg->sym_in->ids[trans->in_id]->str->len,
			cfg->sym_in->ids[trans->in_id]->str->data,
			cfg->sym_out->strs[trans->out_id]->len,
			cfg->sym_out->strs[trans->out_id]->data,
			trans->weight);
	s=trans->to_state;
	if(s->type==WTK_FST_NORM_STATE)
	{
		for(i=0,trans=s->v.trans;i<s->ntrans;++i,++trans)
		{
			wtk_fst_net_cfg_print_next2(cfg,trans,depth+1);
		}
	}else
	{
		printf("\n");
	}
}

void wtk_fst_net_cfg_print_next(wtk_fst_net_cfg_t *cfg,wtk_fst_trans_t *trans)
{
	wtk_fst_net_cfg_print_next2(cfg,trans,0);
}

void wtk_fst_net_cfg_print_prev2(wtk_fst_net_cfg_t *cfg,wtk_fst_trans2_t *trans,int depth)
{
	wtk_fst_state2_t *s;

	s=(wtk_fst_state2_t*)trans->from_state;
	if(s && s->in_prev)
	{
		wtk_fst_net_cfg_print_prev2(cfg,s->in_prev,depth+1);
	}
	printf("[%.*s:%.*s/%f]",cfg->sym_in->ids[trans->in_id]->str->len,
			cfg->sym_in->ids[trans->in_id]->str->data,
			cfg->sym_out->strs[trans->out_id]->len,
			cfg->sym_out->strs[trans->out_id]->data,
			trans->weight);
}

void wtk_fst_net_cfg_print_prev(wtk_fst_net_cfg_t *cfg,wtk_fst_trans2_t *trans)
{
	wtk_fst_net_cfg_print_prev2(cfg,trans,0);
}

void wtk_fst_net_print_trans2(wtk_fst_net_print_t *p,wtk_fst_trans2_t *trans)
{
	wtk_string_t *k,*v;

	k=p->get_insym(p->ths,trans->in_id);
	//k=NULL;
	v=p->get_outsym(p->ths,trans->out_id);
	if(k)
	{
		printf("[%.*s:%.*s %d:%d %d=>%d]\n",k->len,k->data,v->len,v->data,
				trans->in_id,trans->out_id,trans->from_state->id,trans->to_state->id);

	}else
	{
		printf("[0:%.*s %d:%d %d=>%d]\n",v->len,v->data,
				trans->in_id,trans->out_id,trans->from_state->id,trans->to_state->id);
	}
}

void wtk_fst_net_print_trans_prev2(wtk_fst_net_print_t *p,wtk_fst_trans2_t *trans,int depth)
{
	wtk_fst_state2_t *s;
	wtk_string_t *k,*v;

#ifndef DEBUG_PRV
	s=(wtk_fst_state2_t*)trans->from_state;
	if(s && s->in_prev)
	{
		wtk_fst_net_print_trans_prev2(p,s->in_prev,depth+1);
	}
#endif
	k=p->get_insym(p->ths,trans->in_id);
	//k=NULL;
	v=p->get_outsym(p->ths,trans->out_id);
	if(k)
	{
		printf("[%.*s:%.*s %d:%d %d=>%d]\n",k->len,k->data,v->len,v->data,
				trans->in_id,trans->out_id,trans->from_state->id,trans->to_state->id);
		/*
		printf("[%.*s:%.*s %d:%d] %p %p:%d=>%p:%d\n",k->len,k->data,v->len,v->data,
				trans->in_id,trans->out_id,trans,
				trans->from_state,trans->from_state->id,
				trans->to_state,trans->to_state->id);
		*/
	}else
	{
		printf("[0:%.*s %d:%d %d=>%d]\n",v->len,v->data,
				trans->in_id,trans->out_id,trans->from_state->id,trans->to_state->id);
	}
#ifdef DEBUG_PRV
	s=(wtk_fst_state2_t*)trans->from_state;
	if(s && s->in_prev)
	{
		wtk_fst_net_print_trans_prev2(p,s->in_prev,depth+1);
	}
#endif
}

void wtk_fst_net_print_trans_prev(wtk_fst_net_print_t *p,wtk_fst_trans2_t *trans)
{
	wtk_debug("=========== prev ==========\n");
	wtk_fst_net_print_trans_prev2(p,trans,0);
}

int wtk_fst_state2_inarcs(wtk_fst_state2_t* s)
{
	wtk_fst_trans2_t *trans;
	int cnt;

	for(cnt=0,trans=s->in_prev;trans;trans=trans->in_prev,++cnt);
	return cnt;

}

int wtk_fst_state2_has_sampe_input(wtk_fst_state2_t *s1,wtk_fst_state2_t *s2)
{
	wtk_fst_trans2_t *t1,*t2;
	int find;
	int b=0;

	if(s1->ntrans==0)
	{
		s1->ntrans=wtk_fst_state2_inarcs(s1);
	}
	if(s2->ntrans==0)
	{
		s2->ntrans=wtk_fst_state2_inarcs(s2);
	}
	if(s1->ntrans!=s2->ntrans){goto end;}
	for(t1=s1->in_prev;t1;t1=t1->in_prev)
	{
		find=0;
		for(t2=s2->in_prev;t2;t2=t2->in_prev)
		{
			if(t2->from_state==t1->from_state
					&& t1->in_id==t2->in_id && t1->out_id==t2->out_id)
			{
				find=1;
				break;
			}
		}
		if(find==0)
		{
			goto end;
		}
	}
	b=1;
end:
	return b;
}


int wtk_fst_state2_outarcs(wtk_fst_state2_t *s)
{
	wtk_fst_trans_t *trans;
	int cnt;

	for(cnt=0,trans=s->v.trans;trans;trans=trans->hook.next,++cnt);
	return cnt;
}

int wtk_fst_state2_has_sampe_output(wtk_fst_state2_t *s1,wtk_fst_state2_t *s2)
{
	wtk_fst_trans2_t *t1,*t2;
	int find;
	int b=0;

	if(s1->ntrans==0)
	{
		s1->ntrans=wtk_fst_state2_outarcs(s1);
	}
	if(s2->ntrans==0)
	{
		s2->ntrans=wtk_fst_state2_outarcs(s2);
	}
	if(s1->ntrans!=s2->ntrans){goto end;}
	for(t1=(wtk_fst_trans2_t*)(s1->v.trans);t1;t1=(wtk_fst_trans2_t*)t1->hook.next)
	{
		find=0;
		for(t2=(wtk_fst_trans2_t*)(s2->v.trans);t2;t2=(wtk_fst_trans2_t*)t2->hook.next)
		{
			if(t2->to_state==t1->to_state && t1->in_id==t2->in_id && t1->out_id==t2->out_id)
			{
				find=1;
				break;
			}
		}
		if(find==0)
		{
			goto end;
		}
	}
	b=1;
end:
	return b;
}

void wtk_fst_net_print_trans_next2(wtk_fst_net_print_t *p,wtk_fst_trans2_t *trans,int depth)
{
	wtk_fst_state_t *s;
	wtk_string_t *k,*v;
	int i;

	for(i=0;i<depth;++i)
	{
		printf(" ");
	}
	k=p->get_insym(p->ths,trans->in_id);
	v=p->get_outsym(p->ths,trans->out_id);
	if(k)
	{
		printf("[%.*s:%.*s %d:%d %d=>%d]\n",k->len,k->data,v->len,v->data,
				trans->in_id,trans->out_id,trans->from_state->id,trans->to_state->id);
	}else
	{
		printf("[0:%.*s %d:%d %d=>%d]\n",v->len,v->data,
				trans->in_id,trans->out_id,trans->from_state->id,trans->to_state->id);
	}
	s=trans->to_state;
	if(s->type==WTK_FST_NORM_STATE)
	{
		for(trans=(wtk_fst_trans2_t*)s->v.trans;trans;trans=(wtk_fst_trans2_t*)trans->hook.next)
		{
			wtk_fst_net_print_trans_next2(p,trans,depth+1);
		}
	}else
	{
		printf("%p EOF\n",trans->to_state);
	}
}

void wtk_fst_net_print_trans_next(wtk_fst_net_print_t *p,wtk_fst_trans2_t *trans)
{
	wtk_debug("=========== nexts ==========\n");
	wtk_fst_net_print_trans_next2(p,trans,0);
}

void wtk_fst_net_print_trans_next_x2(wtk_fst_net_print_t *p,wtk_fst_trans_t *trans,int depth)
{
	wtk_fst_state_t *s;
	wtk_string_t *k,*v;
	int i;

	for(i=0;i<depth;++i)
	{
		printf(" ");
	}
	k=p->get_insym(p->ths,trans->in_id);
	v=p->get_outsym(p->ths,trans->out_id);
	if(k)
	{
		printf("[%.*s:%.*s %d:%d =>%d]\n",k->len,k->data,v->len,v->data,
				trans->in_id,trans->out_id,trans->to_state->id);
	}else
	{
		printf("[0:%.*s %d:%d =>%d]\n",v->len,v->data,
				trans->in_id,trans->out_id,trans->to_state->id);
	}
	s=trans->to_state;
	if(s->type==WTK_FST_NORM_STATE)
	{
		for(i=0,trans=s->v.trans;i<s->ntrans;++i,++trans)
		{
			wtk_fst_net_print_trans_next_x2(p,trans,depth+1);
		}
	}else
	{
		printf("%p EOF\n",trans->to_state);
	}
}

void wtk_fst_net_print_trans_next_x(wtk_fst_net_print_t *p,wtk_fst_trans_t *trans)
{
	wtk_debug("=========== nexts ==========\n");
	wtk_fst_net_print_trans_next_x2(p,trans,0);
}

void wtk_fst_net_cfg_print_trans_next_x2(wtk_fst_net_cfg_t *cfg,wtk_fst_trans_t *trans,int depth)
{
	wtk_fst_state_t *s;
	wtk_string_t *k,*v;
	int i;

	for(i=0;i<depth;++i)
	{
		printf(" ");
	}
	k=cfg->sym_in->ids[trans->in_id]->str;
	v=cfg->sym_out->strs[trans->out_id];
	//k=p->get_insym(p->ths,trans->in_id);
	//v=p->get_outsym(p->ths,trans->out_id);
	if(k)
	{
		printf("[%.*s:%.*s %d:%d =>%d]\n",k->len,k->data,v->len,v->data,
				trans->in_id,trans->out_id,trans->to_state->id);
	}else
	{
		printf("[0:%.*s %d:%d =>%d]\n",v->len,v->data,
				trans->in_id,trans->out_id,trans->to_state->id);
	}
	s=trans->to_state;
	if(s->type==WTK_FST_NORM_STATE)
	{
		for(i=0,trans=s->v.trans;i<s->ntrans;++i,++trans)
		{
			wtk_fst_net_cfg_print_trans_next_x2(cfg,trans,depth+1);
		}
	}else
	{
		printf("%p EOF\n",trans->to_state);
	}
}

void wtk_fst_net_cfg_print_trans_next_x(wtk_fst_net_cfg_t *cfg,wtk_fst_trans_t *trans)
{
	wtk_fst_net_cfg_print_trans_next_x2(cfg,trans,0);
}

void wtk_fst_trans2_print_prev(wtk_fst_trans2_t *trans)
{
	wtk_fst_state2_t *s;

	s=(wtk_fst_state2_t*)trans->from_state;
	if(s && s->in_prev)
	{
		wtk_fst_trans2_print_prev(s->in_prev);
	}
	if(trans->out_id>0)
	{
		wtk_debug("[%d:%d/%f]\n",trans->in_id,trans->out_id,trans->weight);
	}
}

void wtk_fst_net_cfg_print_trans(wtk_fst_net_cfg_t *cfg,wtk_fst_trans_t *trans)
{
	wtk_string_t *v;

	wtk_debug("============== trans=%p ============\n",trans);
#ifdef USE_FILE
	printf("to: %d\n",trans->to_state);
#endif
	if(cfg->sym_in)
	{
		v=cfg->sym_in->ids[trans->in_id]->str;
		printf("in: [%d:%.*s]\n",trans->in_id,v->len,v->data);
	}
	v=cfg->sym_out->strs[trans->out_id];
	printf("out: [%d:%.*s]\n",trans->out_id,v->len,v->data);
	printf("weight: %f\n",trans->weight);
}

void wtk_fst_net_cfg_print_state(wtk_fst_net_cfg_t *cfg,wtk_fst_state_t *state)
{
	wtk_fst_trans_t *trans;
	int i;

	if(state->type==WTK_FST_FINAL_STATE)
	{
		return;
	}
	//for(trans=state->v.trans;trans;trans=trans->hook.next)
	for(trans=state->v.trans,i=0;i<state->ntrans;++trans,++i)
	{
		wtk_debug("[%.*s:%.*s/%f %d=>%d]\n",
				cfg->sym_in->ids[trans->in_id]->str->len,
				cfg->sym_in->ids[trans->in_id]->str->data,
				cfg->sym_out->strs[trans->out_id]->len,
				cfg->sym_out->strs[trans->out_id]->data,
				trans->weight,
				state->id,trans->to_state->id);
		wtk_fst_net_cfg_print_state(cfg,trans->to_state);
	}
}

void wtk_fst_state2_print(wtk_fst_state2_t *s)
{
	wtk_fst_trans_t *trans;
	int i;

	for(i=0,trans=s->v.trans;trans;trans=trans->hook.next,++i)
	{
		printf("v[%d]=[%d:%d/%f]\n",i,trans->in_id,trans->out_id,trans->weight);
	}
}

wtk_fst_trans2_t* wtk_fst_state2_find(wtk_fst_state2_t *s,int in_id,int out_id)
{
	wtk_fst_trans_t *trans;

	for(trans=s->v.trans;trans;trans=trans->hook.next)
	{
		if(trans->in_id==in_id && trans->out_id==out_id)
		{
			return (wtk_fst_trans2_t*)trans;
		}
	}
	return NULL;
}

wtk_fst_trans2_t* wtk_fst_state2_find_next(wtk_fst_state2_t *s,int in_id,int out_id,wtk_fst_state2_t *to)
{
	wtk_fst_trans_t *trans;
	if(to)
	{
		//wtk_debug("wtk_fst_state2_find_next\n");
		for(trans=s->v.trans;trans;trans=trans->hook.next)
		{
			//wtk_debug("trans->in_id[%d/%d] out_id[%d/%d] trans->to_state=%p to=%p\n", trans->in_id,in_id, trans->out_id, out_id,trans->to_state, to);
			if(trans->in_id==in_id && trans->out_id==out_id && trans->to_state==(wtk_fst_state_t*)to)
			{
				return (wtk_fst_trans2_t*)trans;
			}
		}
	}else
	{
		for(trans=s->v.trans;trans;trans=trans->hook.next)
		{
			if(trans->in_id==in_id && trans->out_id==out_id)
			{
				return (wtk_fst_trans2_t*)trans;
			}
		}
	}
	return NULL;
}

wtk_fst_trans2_t* wtk_fst_state2_find_prev(wtk_fst_state2_t *s,int in_id,int out_id,wtk_fst_state2_t *from)
{
	wtk_fst_trans2_t *tx;

	if(from)
	{
		for(tx=s->in_prev;tx;tx=tx->in_prev)
		{
			if(tx->in_id==in_id && tx->out_id==out_id && tx->from_state==(wtk_fst_state_t*)from)
			{
				return tx;
			}
		}
	}else
	{
		for(tx=s->in_prev;tx;tx=tx->in_prev)
		{
			if(tx->in_id==in_id && tx->out_id==out_id)
			{
				return tx;
			}
		}
	}
	return NULL;
}


wtk_fst_trans2_t* wtk_fst_state2_find2(wtk_fst_state2_t *s,wtk_fst_state2_t *e,int in_id,int out_id)
{
	wtk_fst_trans_t *trans;

	for(trans=s->v.trans;trans;trans=trans->hook.next)
	{
		if(trans->in_id==in_id && trans->out_id==out_id && trans->to_state==(wtk_fst_state_t*)e)
		{
			return (wtk_fst_trans2_t*)trans;
		}
	}
	return NULL;
}

wtk_fst_trans2_t* wtk_fst_state2_find3(wtk_fst_state2_t *s,wtk_fst_state2_t *from,int in_id,int out_id)
{
	wtk_fst_trans2_t *trans;

	if(from)
	{
		for(trans=s->in_prev;trans;trans=trans->in_prev)
		{
			if(trans->in_id==in_id && trans->out_id==out_id && trans->from_state==(wtk_fst_state_t*)from)
			{
				return (wtk_fst_trans2_t*)trans;
			}
		}
	}else
	{
		for(trans=s->in_prev;trans;trans=trans->in_prev)
		{
			if(trans->in_id==in_id && trans->out_id==out_id)
			{
				return (wtk_fst_trans2_t*)trans;
			}
		}
	}
	return NULL;
}


int wtk_fst_state2_has_eps_to(wtk_fst_state2_t *s,wtk_fst_state2_t *t)
{
	wtk_fst_trans_t *trans;
	int b;

	for(trans=s->v.trans;trans;trans=trans->hook.next)
	{
		if(trans->in_id==0 && trans->out_id==0)
		{
			if(trans->to_state==(wtk_fst_state_t*)t)
			{
				return 1;
			}
			b=wtk_fst_state2_has_eps_to((wtk_fst_state2_t*)(trans->to_state),t);
			if(b)
			{
				return 1;
			}
		}
	}
	return 0;
}

int wtk_fst_state2_has_eps_to2(wtk_fst_state2_t *s,wtk_fst_state2_t *t)
{
	wtk_fst_trans_t *trans;

	for(trans=s->v.trans;trans;trans=trans->hook.next)
	{
		if(trans->in_id==0 && trans->out_id==0)
		{
			if(trans->to_state==(wtk_fst_state_t*)t)
			{
				return 1;
			}
		}
	}
	return 0;
}

wtk_fst_trans2_t* wtk_fst_state2_find_reverse(wtk_fst_state2_t *s,int in_id,int out_id)
{
	wtk_fst_trans_t *trans;
	wtk_fst_trans2_t *t2;

	for(trans=s->v.trans;trans;trans=trans->hook.next)
	{
		if(trans->in_id==in_id && trans->out_id==out_id)
		{
			return (wtk_fst_trans2_t*)trans;
		}
	}
	for(trans=s->v.trans;trans;trans=trans->hook.next)
	{
		if(trans->in_id==0 && trans->out_id==0)
		{
			t2=wtk_fst_state2_find_reverse((wtk_fst_state2_t*)(trans->to_state),in_id,out_id);
			if(t2)
			{
				return t2;
			}
		}
	}
	return NULL;
}

wtk_fst_trans2_t* wtk_fst_state2_find_reverse_prev(wtk_fst_state2_t *s,int in_id,int out_id)
{
	wtk_fst_trans2_t *trans;
	wtk_fst_trans2_t *t2;

	for(t2=s->in_prev;t2;t2=t2->in_prev)
	{
		if(t2->in_id==in_id && t2->out_id==out_id)
		{
			return t2;
		}
	}
	for(t2=s->in_prev;t2;t2=t2->in_prev)
	{
		trans=wtk_fst_state2_find_reverse_prev((wtk_fst_state2_t*)t2->from_state,in_id,out_id);
		if(trans)
		{
			return trans;
		}
	}
	return NULL;
}

wtk_fst_trans2_t* wtk_fst_state2_find_reverse_next(wtk_fst_state2_t *s,int in_id,int out_id)
{
	wtk_fst_trans_t *trans;
	wtk_fst_trans2_t *t2;

	for(trans=s->v.trans;trans;trans=trans->hook.next)
	{
		if(trans->in_id==in_id && trans->out_id==out_id)
		{
			return (wtk_fst_trans2_t*)trans;
		}
	}
	for(trans=s->v.trans;trans;trans=trans->hook.next)
	{
		t2=wtk_fst_state2_find_reverse_next((wtk_fst_state2_t*)(trans->to_state),in_id,out_id);
		if(t2)
		{
			return t2;
		}
	}
	return NULL;
}



float wtk_fst_times(float f1,float f2)
{
	if(f1==FLT_MAX)
	{
		return f1;
	}
	if(f2==FLT_MAX)
	{
		return f2;
	}
	return f1+f2;
}

float wtk_fst_plus(float f1,float f2)
{
	return f1<f2?f1:f2;
}

void wtk_fst_trans_init(wtk_fst_trans_t *trans)
{
	trans->in_id=-1;
	trans->out_id=-1;
	//trans->next=0;
	trans->weight=FLT_MAX;
	//trans->from_state=0;
	trans->to_state=0;
	trans->hook.inst=0;
}

typedef struct
{
	wtk_queue_node_t q_n;
	wtk_fst_state_t *s;
	int n_input;
	int touch;
}wtk_fst_ring_node_t;

void wtk_fst_state_attach_hook(wtk_fst_state_t *s,wtk_heap_t *heap,
		wtk_queue_t *node_q)
{
	wtk_fst_trans_t *trans;
	wtk_fst_ring_node_t *item,*n2;

	item=(wtk_fst_ring_node_t*)wtk_heap_malloc(heap,sizeof(wtk_fst_ring_node_t));
	item->s=s;
	item->n_input=0;
	item->touch=0;
	wtk_queue_push(node_q,&(item->q_n));
	s->hook=item;
	for(trans=s->v.trans;trans;trans=trans->hook.next)
	{
		if(!s->hook)
		{
			wtk_fst_state_attach_hook(s,heap,node_q);
		}
		n2=(wtk_fst_ring_node_t*)s->hook;
		++n2->n_input;
	}
}

void wtk_fst_state_check3(wtk_fst_state_t *s,int depth)
{
	wtk_fst_trans_t *trans;
	wtk_fst_ring_node_t *n;

	n=(wtk_fst_ring_node_t*)s->hook;
	++n->touch;
	if(n->touch>n->n_input)
	{
		wtk_debug("touch=%d input=%d\n",n->touch,n->n_input);
	}
	//wtk_debug("check state v[%d]=[%d]\n",depth,s->id);
	for(trans=s->v.trans;trans;trans=trans->hook.next)
	{
		//wtk_debug("check state v[%d]=[%d=>%d]\n",depth,s->id,trans->to_state->id);
		wtk_fst_state_check3(trans->to_state,depth+1);
	}
}


void wtk_fst_state_check_ring(wtk_fst_state_t *s)
{
	wtk_heap_t *heap;
	wtk_queue_t q;

	wtk_queue_init(&(q));
	heap=wtk_heap_new(4096);
	wtk_fst_state_attach_hook(s,heap,&q);
	wtk_fst_state_check3(s,0);
}

void wtk_fst_state_check2(wtk_fst_state_t *s,int depth)
{
	wtk_fst_trans_t *trans;

	//wtk_debug("check state v[%d]=[%d]\n",depth,s->id);
	for(trans=s->v.trans;trans;trans=trans->hook.next)
	{
		//wtk_debug("check state v[%d]=[%d=>%d]\n",depth,s->id,trans->to_state->id);
		wtk_fst_state_check2(trans->to_state,depth+1);
	}
}

void wtk_fst_state_check(wtk_fst_state_t *s)
{
	wtk_fst_state_check2(s,0);
}

void wtk_fst_state_clean_hook(wtk_fst_state_t *s)
{
	wtk_fst_trans_t *trans;

	if(s->hook)
	{
		for(trans=s->v.trans;trans;trans=trans->hook.next)
		{
			//wtk_debug("hook=%p\n",trans->to_state->hook);
			if(trans->to_state->hook)
			{
				wtk_fst_state_clean_hook(trans->to_state);
			}
		}
		s->hook=NULL;
	}
}

int wtk_fst_state_has_out_id(wtk_fst_state_t *s,unsigned int id)
{
	wtk_fst_trans_t *trans;
	int b;

	if(s->type==WTK_FST_FINAL_STATE)
	{
		return 0;
	}
	for(trans=s->v.trans;trans;trans=trans->hook.next)
	{
		if(trans->out_id==id)
		{
			return 1;
		}
	}
	for(trans=s->v.trans;trans;trans=trans->hook.next)
	{
		b=wtk_fst_state_has_out_id(trans->to_state,id);
		if(b)
		{
			return 1;
		}
	}
	return 0;
}

int wtk_fst_state_reach_end(wtk_fst_state_t *s)
{
	wtk_fst_trans_t *trans;
	int b;

	if(s->type==WTK_FST_FINAL_STATE)
	{
		return 1;
	}
	s->hook=s;
	if(!s->v.trans)
	{
		wtk_debug("reach nil\n");
		return 0;
	}
	for(trans=s->v.trans;trans;trans=trans->hook.next)
	{
		if(trans->to_state->hook)
		{
			continue;
		}
		b=wtk_fst_state_reach_end(trans->to_state);
		if(b)
		{
			return 1;
		}
	}
	return 0;
}

int wtk_fst_state_can_be_end(wtk_fst_state_t *s)
{
	wtk_fst_trans_t *trans;
	int b;

	if(s->type==WTK_FST_FINAL_STATE){return 1;}
	for(trans=s->v.trans;trans;trans=trans->hook.next)
	{
		if(trans->in_id==0)
		{
			b=wtk_fst_state_can_be_end(trans->to_state);
			if(b){return 1;}
		}
	}
	return 0;
}


int wtk_fst_state_ntrans(wtk_fst_state_t *s)
{
	wtk_fst_trans_t *trans;
	int cnt;

	if(s->type==WTK_FST_FINAL_STATE)
	{
		return 0;
	}
	for(trans=s->v.trans,cnt=0;trans;trans=trans->hook.next,++cnt)
	{
	}
	return cnt;
}

wtk_fst_state_t* wtk_fst_net_get_start_state(wtk_fst_net_t *net)
{
	//wtk_debug("init=%p\n",net->init_state);
	if(!net->init_state)
	{
		net->init_state=wtk_fst_net_get_load_state(net,0);
	}
	return net->init_state;
}

//-------------------- dump lm look ahead prob -------------------------

void wtk_fst_net_print_la_state(wtk_fst_net_t *net,wtk_fst_state_t *s,double w,FILE *f)
{
	fprintf(f,"%d %f\n",s->id,w);
}

void wtk_fst_net_print_state_trans(wtk_fst_net_t *net,wtk_fst_state_t *s)
{
	wtk_fst_trans_t *trans;
	int i;

	wtk_debug("========== %d type=%s =========\n",s->id,s->type==WTK_FST_FINAL_STATE?"FINAL":"NORMAL");
	for(i=0,trans=s->v.trans;i<s->ntrans;++i,++trans)
	{
		printf("[%.*s] w=%f\n",
				net->cfg->sym_out->strs[trans->out_id]->len,
				net->cfg->sym_out->strs[trans->out_id]->data,
				trans->weight);
	}

}


typedef struct
{
	double prob;
	unsigned int id;
}wtk_fst_max_wrd_item_t;

wtk_fst_max_wrd_item_t wtk_fst_net_get_max_word_prob(wtk_fst_net_t *net,wtk_fst_state_t *s)
{
	wtk_fst_trans_t *trans;
	int i;
	double w,f;
	wtk_fst_max_wrd_item_t item;
	unsigned int id=0,xid;
	//wtk_string_t *v;
	//int *len;

	if(s->type==WTK_FST_TOUCH_STATE)
	{
		wtk_fst_net_load_state(net,s);
	}
	//wtk_debug("============ %d s=%p:%d================\n",cnt,s,s->ntrans);
	if(s->type==WTK_FST_NORM_STATE)
	{
		w=-1.0E10;
		//len=net->cfg->sym_out->len;
		for(i=0,trans=s->v.trans;i<s->ntrans;++i,++trans)
		{
			if(trans->out_id>0)
			{
				/*
				printf("[%.*s] w=%f\n",
						net->cfg->sym_out->strs[trans->out_id]->len,
						net->cfg->sym_out->strs[trans->out_id]->data,
						trans->weight);
				*/
				f=trans->weight;
				xid=trans->out_id;
				//v=net->cfg->sym_out->strs[trans->out_id];
				//len=wtk_utf8_len(v->data,v->len);
				/*
				if(len[trans->out_id]>1)
				{
					f/=len[trans->out_id];
				}*/
			}else
			{
				if(trans->to_state->hook)
				{
					item=*(wtk_fst_max_wrd_item_t*)trans->to_state->hook;
				}else
				{
					item=wtk_fst_net_get_max_word_prob(net,trans->to_state);
					{
						wtk_fst_max_wrd_item_t *pi;

						pi=(wtk_fst_max_wrd_item_t*)wtk_malloc(sizeof(wtk_fst_max_wrd_item_t));
						*pi=item;
						trans->to_state->hook=pi;
					}
				}
				f=trans->weight+item.prob;
				xid=item.id;
			}
			if(f>w)
			{
				w=f;
				id=xid;
			}
		}
		if(w<-10000.0)
		{
			/*
			wtk_fst_net_print_state_trans(net,s);
			wtk_debug("w=%f\n",w);
			exit(0);
			*/
			w=0;
		}
	}else
	{
		w=s->v.weight;
		id=0;
	}
	item.prob=w;
	item.id=id;
	return item;
}


void wtk_fst_net_dump_lookahead_state2(wtk_fst_net_t *net,wtk_fst_state_t *s,FILE *f)
{
	wtk_fst_max_wrd_item_t item,*p;
	float w;
	//unsigned int id;

	//s->hook=(void*)1;
	if(s->hook)
	{
		item=*(wtk_fst_max_wrd_item_t*)s->hook;
	}else
	{
		item=wtk_fst_net_get_max_word_prob(net,s);
		p=(wtk_fst_max_wrd_item_t*)wtk_malloc(sizeof(wtk_fst_max_wrd_item_t));
		*p=item;
		s->hook=p;
	}
	w=item.prob;
	//id=s->id;
	//wtk_fst_net_print_la_state(net,s,w,f);
	//fwrite(&(id),sizeof(int),1,f);
	fwrite(&(w),sizeof(float),1,f);
	if(s->type==WTK_FST_NORM_STATE && s->v.trans)
	{
		//wtk_free(s->v.trans);
	}
}

void wtk_fst_net_dump_lookahead_bin(wtk_fst_net_t *net,char *fn)
{
	wtk_fst_state_t *s;
	FILE *f;
	unsigned int i,ndx;

	f=fopen(fn,"wb");
	ndx=net->cfg->bin.ndx;
	for(i=0;i<ndx-1;++i)
	{
		s=wtk_fst_net_get_load_state(net,i);
		if(!s)
		{
			wtk_debug("[%d] not found\n",i);
			continue;
		}
		wtk_fst_net_dump_lookahead_state2(net,s,f);
		if(i%1000==0)
		{
			fprintf(stderr,"index=%d/%d\n",i,ndx);
		}
	}
	fclose(f);
}


double wtk_fst_net_get_max_word_prob2(wtk_fst_net_t *net,wtk_fst_state_t *s,double pre)
{
	wtk_fst_trans_t *trans;
	int i;
	double w,f;

	if(s->type==WTK_FST_TOUCH_STATE)
	{
		wtk_fst_net_load_state(net,s);
	}
	w=-1.0E10;
	for(i=0,trans=s->v.trans;i<s->ntrans;++i,++trans)
	{
		if(trans->out_id>0)
		{
			/*
			printf("[%.*s] w=%f\n",
					net->cfg->sym_out->strs[trans->out_id]->len,
					net->cfg->sym_out->strs[trans->out_id]->data,
					trans->weight);
			*/
			f=trans->weight+pre;
		}else
		{
			f=trans->weight+pre;
			if(trans->to_state->hook)
			{
				f+=*(float*)trans->to_state->hook;
			}else
			{
				f=wtk_fst_net_get_max_word_prob2(net,trans->to_state,f);
			}
		}
		if(f>w)
		{
			w=f;
		}
	}
	if(w<-10000.0)
	{
		/*
		wtk_fst_net_print_state_trans(net,s);
		wtk_debug("w=%f\n",w);
		exit(0);
		*/
		w=0;
	}
	return w;
}


void wtk_fst_net_dump_lookahead_state(wtk_fst_net_t *net,wtk_fst_state_t *s,FILE *f)
{
	float w;
	float *p;
	//unsigned int id;

	//s->hook=(void*)1;
	w=wtk_fst_net_get_max_word_prob2(net,s,0);
	p=(float*)wtk_malloc(sizeof(float));
	*p=w;
	s->hook=p;
	//id=s->id;
	//wtk_fst_net_print_la_state(net,s,w,f);
	//fwrite(&(id),sizeof(int),1,f);
	fwrite(&(w),sizeof(float),1,f);
	if(s->type==WTK_FST_NORM_STATE && s->v.trans)
	{
		wtk_free(s->v.trans);
	}
}

void wtk_fst_net_dump_lookahead_bin2(wtk_fst_net_t *net,char *fn)
{
	wtk_fst_state_t *s;
	FILE *f;
	unsigned int i,ndx;

	f=fopen(fn,"wb");
	ndx=net->cfg->bin.ndx;
	for(i=0;i<ndx-1;++i)
	{
		s=wtk_fst_net_get_load_state(net,i);
		if(!s)
		{
			wtk_debug("[%d] not found\n",i);
			continue;
		}
		wtk_fst_net_dump_lookahead_state(net,s,f);
		if(i%1000==0)
		{
			fprintf(stderr,"index=%d/%d\n",i,ndx);
		}
	}
	fclose(f);
}

//---------------------- load net -------------------------
#include "wtk/core/wtk_larray.h"
#include <math.h>

int wtk_fst_net_load_fsm(wtk_fst_net_t *net,wtk_source_t *src)
{
	//wtk_heap_t *heap=net->heap;
	float v[5];
	int i,ret,nl;
	int last_id=-1;
	int from,input,output,to;
	float w;
	wtk_fst_state_t *s=NULL;
	wtk_fst_trans_t *trans,*t;
	wtk_larray_t *a;
	wtk_heap_t *heap2;
	int j;
	//float pen=-net->cfg->wordpen*log2(10);
	//float lw=-net->cfg->lmscale*log2(10);
	float lw=-net->cfg->lmscale*log(10)/log(2);

	heap2=wtk_heap_new(4096);
	a=wtk_larray_new(1024,sizeof(wtk_fst_trans_t*));
	while(1)
	{
		//from to in out weight
		for(i=0;i<5;++i)
		{
			ret=wtk_source_skip_sp(src,&nl);
			//wtk_debug("ret=%d,nl=%d\n",ret,nl);
			if(ret!=0)
			{
				//wtk_debug("ret=%d,nl=%d\n",ret,nl);
				if(i==0){ret=0;}
				goto end;
			}
			//wtk_debug("nl=%d\n",nl);
			if(nl)
			{
				if(i==0)
				{
					ret=0;
					goto end;
				}
				break;
			}
			ret=wtk_source_read_float(src,&v[i],1,0);
			if(ret!=0){goto end;}
			//wtk_debug("v[%d]=%f\n",i,v[i]);
		}
		wtk_source_skip_sp(src,NULL);
		from=(int)v[0];
		//wtk_debug("from=%d i=%d\n",from,i);
		if(from!=last_id)
		{
			if(last_id>=0)
			{
				if(s->type==WTK_FST_FINAL_STATE)
				{
					//wtk_debug("n=%d\n",a->nslot);
				}else
				{
					s->ntrans=a->nslot;
					s->custom=1;
					s->type=WTK_FST_NORM_STATE;
					s->v.trans=(wtk_fst_trans_t*)wtk_heap_malloc(net->heap,sizeof(wtk_fst_trans_t)*a->nslot);
					for(j=0;j<a->nslot;++j)
					{
						t=((wtk_fst_trans_t**)a->slot)[j];
						trans=s->v.trans+j;
						*trans=*t;
					}
					//wtk_debug("s=%d\n",s->id);
				}
			}
			wtk_larray_reset(a);
			s=wtk_fst_net_get_fst_state(net,from);
			last_id=from;
		}
		switch(i)
		{
		case 1:
			//fall through;
		case 2:
			if(i==1)
			{
				w=0;
			}else
			{
				w=v[1];
			}
			s->type=WTK_FST_FINAL_STATE;
			s->v.weight=w;
			s->custom=1;
			//wtk_debug("load final fsm %p:%d:%d\n",s,s->id,s->type);
			break;
		case 4:
		case 5:
			to=(int)v[1];
			input=(int)v[2];
			output=(int)v[3];
			if(i==4)
			{
				w=lw;
			}else
			{
				w=v[4];
			}
			//wtk_debug("w=%f\n",w);
			trans=(wtk_fst_trans_t*)wtk_heap_malloc(heap2,sizeof(wtk_fst_trans_t));
			trans->to_state=wtk_fst_net_get_fst_state(net,to);
			trans->in_id=input;
			trans->out_id=output;
			trans->weight=w;
			trans->hook.inst=NULL;
			wtk_larray_push2(a,&(trans));
			break;
		}
	}
end:
	//wtk_debug("ret=%d\n",ret);
	if(last_id>=0)
	{
		if(s->type==WTK_FST_FINAL_STATE)
		{
			//wtk_debug("n=%d\n",a->nslot);
		}else
		{
			s->ntrans=a->nslot;
			s->type=WTK_FST_NORM_STATE;
			s->custom=1;
			s->v.trans=(wtk_fst_trans_t*)wtk_heap_malloc(net->heap,sizeof(wtk_fst_trans_t)*a->nslot);
			for(j=0;j<a->nslot;++j)
			{
				t=((wtk_fst_trans_t**)a->slot)[j];
				trans=s->v.trans+j;
				*trans=*t;
			}
		}
	}
	net->init_state=wtk_fst_net_get_fst_state(net,0);
	//wtk_debug("[%d]\n",net->init_state->ntrans);
	wtk_heap_delete(heap2);
	wtk_larray_delete(a);
	return ret;
}


int wtk_fst_net_load_str(wtk_fst_net_t *net,char *data,int bytes)
{
	wtk_source_t src;
	int ret;

	wtk_source_init_str(&(src),data,bytes);
	ret=wtk_fst_net_load_fsm(net,&(src));
	wtk_source_clean_str(&(src));
	return ret;
}


void wtk_fst_net_add_filler(wtk_fst_net_t *net,
		wtk_fst_state_t *start,wtk_fst_state_t *final,int max_state,double min_pen,
		wtk_fst_insym_t *in_sym)
{
	wtk_fst_state_t *f,*e,*s;
	wtk_fst_trans_t *trans,*t;
	int id=max_state,i;
	int n;
	//int cnt=10;
	int index;//,tx;
	int j;

	//n=net->cfg->sym_in->nid/cnt;
	n=in_sym->nid-2;
	++id;
	f=wtk_fst_net_get_fst_state(net,id);
	f->ntrans=n;
	f->v.trans=(wtk_fst_trans_t*)wtk_heap_malloc(net->heap,sizeof(wtk_fst_trans_t)*n);
	f->type=WTK_FST_NORM_STATE;

	++id;
	e=wtk_fst_net_get_fst_state(net,id);
	e->type=WTK_FST_NORM_STATE;
	//fx=min_pen*2.0;
	//for(i=0,index=0;i<n;++i,index+=cnt)
	for(i=0,j=0;i<in_sym->nid;++i)
	{
		index=wtk_fst_insym_get_index(net->cfg->sym_in,in_sym->ids[i]->str);
		if(index<=0)
		{
			//wtk_debug("index=%d [%.*s]\n",index,in_sym->ids[i]->str->len,in_sym->ids[i]->str->data);
			//exit(0);
			continue;
		}
		trans=&(f->v.trans[j]);
		++j;
		trans->hook.inst=NULL;
		//tx=random()%cnt;
		//trans->in_id=index+tx;
		trans->in_id=index;
		trans->out_id=i+1;//net->cfg->snt_end_id;
		trans->weight=min_pen;
		trans->to_state=e;
	}
	e->ntrans=2;
	e->v.trans=(wtk_fst_trans_t*)wtk_heap_malloc(net->heap,sizeof(wtk_fst_trans_t)*2);

	trans=&(e->v.trans[0]);
	trans->hook.inst=NULL;
	trans->in_id=0;
	trans->out_id=0;
	trans->weight=min_pen;
	trans->to_state=f;

	trans=&(e->v.trans[1]);
	trans->hook.inst=NULL;
	trans->in_id=0;
	trans->out_id=0;
	trans->weight=min_pen;
	trans->to_state=final;

	s=start;
	n=s->ntrans;
	t=s->v.trans;
	s->ntrans=n+1;
	s->v.trans=(wtk_fst_trans_t*)wtk_heap_malloc(net->heap,sizeof(wtk_fst_trans_t)*s->ntrans);
	for(i=0;i<n;++i)
	{
		s->v.trans[i]=t[i];
	}

	trans=&(s->v.trans[n]);
	trans->hook.inst=0;
	trans->in_id=0;
	trans->out_id=0;
	trans->weight=min_pen;
	trans->to_state=f;
	//wtk_debug("%d %p %p\n",n,f,final);
	//0009, ntrans = 36304, type = 1
	//wtk_debug("id=%d\n",id);
	//exit(0);
}

int wtk_fst_net_load_clg_fsm(wtk_fst_net_t *net,wtk_source_t *src,wtk_fst_insym_t *sym,
		wtk_fst_sym_t *out_sym)
{
	wtk_fst_net_cfg_t *cfg=net->cfg;
	//wtk_heap_t *heap=net->heap;
	float v[5];
	int i,ret,nl;
	int last_id=-1;
	int from,input,output,to;
	float w;
	wtk_fst_state_t *s=NULL;
	wtk_fst_trans_t *trans,*t;
	wtk_larray_t *a;
	wtk_heap_t *heap2;
	wtk_string_t *vx;
	int j;
	int max_state;
	wtk_fst_state_t *final=NULL,*start=NULL;
	double min_pen=0;
	int cnt;

	cnt=0;
	max_state=0;
	heap2=wtk_heap_new(4096);
	a=wtk_larray_new(1024,sizeof(wtk_fst_trans_t*));
	while(1)
	{
		//from to in out weight
		for(i=0;i<5;++i)
		{
			ret=wtk_source_skip_sp(src,&nl);
			//wtk_debug("ret=%d,nl=%d\n",ret,nl);
			if(ret!=0)
			{
				//wtk_debug("ret=%d,nl=%d\n",ret,nl);
				if(i==0){ret=0;}
				goto end;
			}
			//wtk_debug("nl=%d\n",nl);
			if(nl)
			{
				if(i==0)
				{
					ret=0;
					goto end;
				}
				break;
			}
			ret=wtk_source_read_float(src,&v[i],1,0);
			if(ret!=0){goto end;}
			//wtk_debug("v[%d]=%f\n",i,v[i]);
		}
		wtk_source_skip_sp(src,NULL);
		from=(int)v[0];
		//wtk_debug("from=%d i=%d\n",from,i);
		if(from>max_state)
		{
			max_state=from;
		}
		if(from!=last_id)
		{
			if(last_id>=0)
			{
				if(s->type==WTK_FST_FINAL_STATE)
				{
					//wtk_debug("n=%d\n",a->nslot);
				}else
				{
					s->ntrans=a->nslot;
					s->type=WTK_FST_NORM_STATE;
					s->v.trans=(wtk_fst_trans_t*)wtk_heap_malloc(net->heap,sizeof(wtk_fst_trans_t)*a->nslot);
					for(j=0;j<a->nslot;++j)
					{
						t=((wtk_fst_trans_t**)a->slot)[j];
						trans=s->v.trans+j;
						*trans=*t;
					}
					//wtk_debug("s=%d\n",s->id);
				}
			}
			wtk_larray_reset(a);
			s=wtk_fst_net_get_fst_state(net,from);
			last_id=from;
		}
		switch(i)
		{
		case 1:
			//fall through;
		case 2:
			if(i==1)
			{
				w=0;
			}else
			{
				w=v[1];
			}
			s->type=WTK_FST_FINAL_STATE;
			w=-w*cfg->lmscale;
			s->v.weight=w;
			//final=s;
			//wtk_debug("load final fsm %p:%d:%d\n",s,s->id,s->type);
			break;
		case 4:
		case 5:
			to=(int)v[1];
			input=(int)v[2];
			output=(int)v[3];
			if(i==4)
			{
				w=0;
			}else
			{
				w=v[4];
			}
			trans=(wtk_fst_trans_t*)wtk_heap_malloc(heap2,sizeof(wtk_fst_trans_t));
			trans->to_state=wtk_fst_net_get_fst_state(net,to);
			//wtk_debug("in_id=%d\n",input);
			vx=sym->ids[input]->str;
			//wtk_debug("input=[%.*s]\n",vx->len,vx->data);
			input=wtk_fst_insym_get_index(net->cfg->sym_in,vx);
			if(input<0)
			{
				wtk_string_t p=wtk_string("<eps>");
				//wtk_string_t p=wtk_string("sil");

				input=wtk_fst_insym_get_index(net->cfg->sym_in,&p);
				//wtk_debug("input=%d output=%d\n",input,output);
			}
			if(input<0)
			{
				wtk_debug("found [%.*s] failed.\n",vx->len,vx->data);
				ret=-1;
				goto end;
			}
			trans->in_id=input;
			trans->out_id=output;
			if(output==net->cfg->snt_start_id)
			{
				start=trans->to_state;
			}else if(output==net->cfg->snt_end_id)
			{
				final=trans->to_state;
			}
			/*
			if(output==19)
			{
				vx=out_sym->strs[output];
				wtk_debug("found [%.*s] %d->%d\n",vx->len,vx->data,from,to);
			}*/
			w=-w*cfg->lmscale+((output>0)?cfg->wordpen:0);
			min_pen+=w;
			++cnt;
			trans->weight=w;
			trans->hook.inst=NULL;
			wtk_larray_push2(a,&(trans));
			break;
		}
	}
end:
	//exit(0);
	if(last_id>=0)
	{
		if(s->type==WTK_FST_FINAL_STATE)
		{
			//wtk_debug("n=%d\n",a->nslot);
		}else
		{
			s->ntrans=a->nslot;
			s->type=WTK_FST_NORM_STATE;
			s->v.trans=(wtk_fst_trans_t*)wtk_heap_malloc(net->heap,sizeof(wtk_fst_trans_t)*a->nslot);
			for(j=0;j<a->nslot;++j)
			{
				t=((wtk_fst_trans_t**)a->slot)[j];
				trans=s->v.trans+j;
				*trans=*t;
			}
		}
	}
	net->init_state=wtk_fst_net_get_fst_state(net,0);
	if(final && start)
	{
		min_pen/=cnt;
		///wtk_fst_net_add_filler(net,start,final,max_state,min_pen,sym);
	}
	//wtk_debug("[%d]\n",net->init_state->ntrans);
	/*
	wtk_fst_net_print_state(net,net->init_state);
	wtk_debug("ret=%d\n",ret);
	exit(0);
	*/
	wtk_heap_delete(heap2);
	wtk_larray_delete(a);
	return ret;
}

int wtk_fst_net_load_clg(wtk_fst_net_t *net,char *fsm_fn,char *in_sym_fn,char *out_sym_fn)
{
	wtk_fst_insym_t *sym=NULL;
	wtk_fst_sym_t *out_sym=NULL;
	wtk_source_t src;
	int ret;

	/*
	wtk_debug("%s\n",fsm_fn);
	wtk_debug("%s\n",in_sym_fn);
	wtk_debug("%s\n",out_sym_fn);
	*/
	out_sym=wtk_fst_sym_new(NULL,out_sym_fn);
	sym=wtk_fst_insym_new(NULL,in_sym_fn,0);
	ret=wtk_source_init_file(&(src),fsm_fn);
	if(ret!=0){goto end;}
	if(net->cfg->sym_out)
	{
		wtk_fst_sym_delete(net->cfg->sym_out);
		net->cfg->sym_out=NULL;
	}
	net->cfg->sym_out=out_sym;
	//very ugly;
	ret=wtk_fst_net_load_clg_fsm(net,&(src),sym,out_sym);
	out_sym=NULL;
	ret=0;
end:
	wtk_source_clean_file(&(src));
	if(sym)
	{
		wtk_fst_insym_delete(sym);
	}
	if(out_sym)
	{
		wtk_fst_sym_delete(out_sym);
	}
	return ret;
}

int wtk_fst_net_load_clg2(wtk_fst_net_t *net,char *dn,int bytes)
{
	wtk_strbuf_t *buf;
	int ret;
	char *fsm,*in,*out;

	//wtk_debug("[%.*s]\n",bytes,dn);
	buf=wtk_strbuf_new(512,1);

	wtk_strbuf_reset(buf);
	wtk_strbuf_push(buf,dn,bytes);
	wtk_strbuf_push_s(buf,"/final.fsm");
	wtk_strbuf_push_c(buf,0);
	fsm=wtk_heap_dup_str(net->heap,buf->data);

	wtk_strbuf_reset(buf);
	wtk_strbuf_push(buf,dn,bytes);
	wtk_strbuf_push_s(buf,"/final.insyms");
	wtk_strbuf_push_c(buf,0);
	in=wtk_heap_dup_str(net->heap,buf->data);

	wtk_strbuf_reset(buf);
	wtk_strbuf_push(buf,dn,bytes);
	wtk_strbuf_push_s(buf,"/final.outsyms");
	wtk_strbuf_push_c(buf,0);
	out=wtk_heap_dup_str(net->heap,buf->data);

	ret=wtk_fst_net_load_clg(net,fsm,in,out);
	wtk_strbuf_delete(buf);
	return ret;
}

#include <math.h>

float wtk_fst_net_add_rbin3_weight2(wtk_fst_net_t *net,wtk_fst_state_t *state,int depth,float pen,float lw)
{
	wtk_fst_trans_t *trans;
	float f=0;
	int i;
	//float pen=-net->cfg->wordpen*log2(10);
	//float lw=-net->cfg->lmscale*log2(10);

	for(i=0,trans=state->v.trans;i<state->ntrans;++i,++trans)
	{
		//wtk_debug("in=%d/%d\n",trans->in_id,trans->out_id)
		if(trans->weight!=0)
		{
			continue;
		}
		if(trans->out_id>0)
		{
			if(trans->out_id!=net->cfg->snt_start_id)
			{
				trans->weight=pen;
			}else
			{
				trans->weight=0;
			}
			f=lw/(depth+1);
			//wtk_debug("depth=%d f=%f\n",depth,f);
			//exit(0);
//			if(trans->in_id>0)
//			{
//				trans->weight+=f;
//			}
			trans->weight+=wtk_fst_net_add_rbin3_weight2(net,trans->to_state,0,pen,lw);
			//wtk_fst_net_add_rbin3_weight2(net,trans->to_state,0,pen,lw);
			//wtk_debug("pen=%f/%f\n",pen,trans->weight);
		}else
		{
			trans->weight=wtk_fst_net_add_rbin3_weight2(net,trans->to_state,depth+1,pen,lw);
			if(trans->weight<f)
			{
				f=trans->weight;
			}
			//f=trans->weight;
		}
	}
	return f;
}

void wtk_fst_net_add_rbin3_weight(wtk_fst_net_t *net,wtk_fst_state_t *state)
{
#ifdef __ANDROID__
	float pen=-net->cfg->wordpen*log(10)/log(2);
	float lw=-net->cfg->lmscale*log(10)/log(2);
#else
	float pen=-net->cfg->wordpen*log2(10);
	float lw=-net->cfg->lmscale*log2(10);
#endif

	//wtk_debug("pen=%f\n",pen);
	//exit(0);
	net->cfg->wordpen=pen;
	wtk_fst_net_add_rbin3_weight2(net,state,0,pen,lw);
}

int wtk_fst_net_load_rbin3(wtk_fst_net_t *net,wtk_source_t *src)
{
	int ret;
	wtk_fst_rnet_type_t type;
	int s_id,e_id,i_id,o_id;
	wtk_fst_state_t *s,*e;
	wtk_fst_trans_t *trans;
	int vi;
	int i;
	wtk_string_t *data;
	char *ps,*pe;
#ifdef __ANDROID__
	float pen=-net->cfg->wordpen*log(10)/log(2);
	float lw=-net->cfg->lmscale*log(10)/log(2);
#else
	float pen=-net->cfg->wordpen*log2(10);
	float lw=-net->cfg->lmscale*log2(10);
#endif
	//char *pt;

	//wtk_debug("pen=%f lw=%f/%f\n",pen,lw,net->cfg->lmscale);
	//exit(0);
	data=src->get_file(src->data);
	ps=data->data;
	//wtk_debug("ps=%p\n",ps);
	pe=ps+data->len;
#ifdef USE_CHAR_NET
	memcpy(&(vi),ps,4);
#else
	vi=*((int*)ps);
#endif
	//wtk_debug("vi=%d\n",vi);
	ps+=4;
	//wtk_debug("vi=%d\n",vi);
	net->nrbin_states=vi;
	net->rbin_states=(wtk_fst_state_t*)wtk_calloc(vi,sizeof(wtk_fst_state_t));
	//wtk_debug("n=%d\n",vi);
	for(i=0;i<vi;++i)
	{
		wtk_fst_state_init(net->rbin_states+i,i);
		(net->rbin_states+i)->custom=1;
	}
	s=NULL;e=NULL;
	//pt=ps;
	while(ps<pe)
	{
#ifdef USE_CHAR_NET
		memcpy(&(vi),ps,4);
#else
		vi=*((int*)ps);
#endif
		ps+=4;
#ifdef USE_CHAR_NET
		memcpy(&(s_id),ps,4);
#else
		s_id=*((int*)ps);
#endif
		//wtk_debug("s=%d/%d/%d,ps=%p,pt=%p,n=%d\n",s_id,*((int*)ps),vi,ps,pt,ps-pt);
		//pt=ps;
		ps+=4;
		//s=wtk_fst_net_get_fst_state(net,s_id);
		s=net->rbin_states+s_id;
		if(vi<0)
		{
			s->ntrans=0;
			s->type=WTK_FST_FINAL_STATE;
		}else if(vi>0)
		{
			s->ntrans=vi;
			s->type=WTK_FST_NORM_STATE;
			s->v.trans=(wtk_fst_trans_t*)wtk_heap_malloc(net->heap,vi*sizeof(wtk_fst_trans_t));
			//swtk_debug("vi=%d\n",vi);
			for(i=0;i<s->ntrans;++i)
			{
#ifdef USE_CHAR_NET
				//memcpy(&(type),ps,1);
				type=*ps;
#else
				type=*ps;
#endif
				ps+=1;
#ifdef USE_CHAR_NET
				memcpy(&(e_id),ps,4);
#else
				e_id=*((int*)(ps));
#endif
				ps+=4;
				//wtk_debug("type=%d\n",type);
				i_id=o_id=0;
				switch(type)
				{
				case WTK_FST_RNET_NIL:
					break;
				case WTK_FST_RNET_IN_NIL:
#ifdef USE_CHAR_NET
					memcpy(&(o_id),ps,4);
#else
					o_id=*((int*)ps);
#endif
					ps+=4;
					break;
				case WTK_FST_RNET_OUT_NIL:
#ifdef USE_CHAR_NET
					memcpy(&(i_id),ps,4);
#else
					i_id=*((int*)ps);
#endif
					ps+=4;
					break;
				case WTK_FST_RNET_IN_OUT:
#ifdef USE_CHAR_NET
					memcpy(&(i_id),ps,4);
#else
					i_id=*((int*)ps);
#endif
					ps+=4;
#ifdef USE_CHAR_NET
					memcpy(&(o_id),ps,4);
#else
					o_id=*((int*)ps);
#endif
					ps+=4;
					break;
				}
				//e=wtk_fst_net_get_fst_state(net,e_id);
				e=net->rbin_states+e_id;
				trans=s->v.trans+i;
				trans->in_id=i_id;
				trans->out_id=o_id;
				trans->hook.inst=NULL;
				trans->to_state=e;
				//trans->weight=(trans->in_id>0||trans->out_id>0)?lw:0+trans->out_id>0?pen:0;//1.0+trans->out_id>0?net->cfg->wordpen:0;
				trans->weight=(trans->in_id>0)?lw:0+trans->out_id>0?pen:0;
				//trans->weight=0;
				//wtk_debug("%d/%d %f\n",trans->in_id,trans->out_id,trans->weight);
//				{
//					wtk_string_t *str,*str1;
//
//					str=net->cfg->sym_in->ids[trans->in_id]->str;
//					str1=net->cfg->sym_out->strs[trans->out_id];
//					wtk_debug("[%.*s]=[%.*s] %d/%d %f trans=%p\n",str->len,str->data,str1->len,str1->data,trans->in_id,trans->out_id,trans->weight,trans);
//
//				}
				/*
				if(i_id<0)
				{
					wtk_debug("i_id=%d\n",i_id);
					exit(0);
				}*/
				//wtk_debug("trans=%p inst=%p\n",trans,trans->hook.inst);
			}
		}else
		{
			s->ntrans=0;
			s->type=WTK_FST_NORM_STATE;
		}
	}
	ret=0;
	net->init_state=net->rbin_states;
//	wtk_fst_net_add_rbin3_weight(net,net->init_state);
	//wtk_debug("%p n=%d\n",net->init_state,net->init_state->ntrans);
	//wtk_debug("ret=%d\n",ret);
	//exit(0);
	return ret;
}

int wtk_fst_net_load_rbin(wtk_fst_net_t *net,wtk_source_t *src)
{
	return wtk_fst_net_load_rbin3(net,src);
	/*
#ifndef __APPLE__
	if(src->get_file)
	{
		return wtk_fst_net_load_rbin3(net,src);
	}else
#endif
	{
		return wtk_fst_net_load_rbin2(net,src);
	}*/
}



void wtk_fst_net_print_init(wtk_fst_net_print_t *p,void *ths,wtk_fst_net_get_sym_f get_insym,
		wtk_fst_net_get_sym_f get_outsym)
{
	p->ths=ths;
	p->get_insym=get_insym;
	p->get_outsym=get_outsym;
}

wtk_string_t* wtk_fst_net_print_get_insym(wtk_fst_net_print_t *p,unsigned int id)
{
	return p->get_insym(p->ths,id);
}

wtk_string_t* wtk_fst_net_print_get_outsym(wtk_fst_net_print_t *p,unsigned int id)
{
	return p->get_outsym(p->ths,id);
}
