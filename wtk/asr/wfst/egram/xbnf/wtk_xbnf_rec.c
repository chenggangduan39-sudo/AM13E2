#include "wtk_xbnf_rec.h"
#include <ctype.h>
void wtk_xbnf_rec_add_list(wtk_xbnf_rec_t *r,wtk_xbnf_item_set_t *set,wtk_xbnf_item_list_t *list,wtk_xbnf_rec_path_t *prev);
void wtk_xbnf_rec_add_set(wtk_xbnf_rec_t *r,wtk_xbnf_item_set_t *set,wtk_xbnf_rec_path_t *prev);
void wtk_xbnf_rec_add_expr(wtk_xbnf_rec_t *r,wtk_xbnf_expr_t *expr,wtk_xbnf_rec_path_t *prev);

void wtk_xbnf_rec_path_print(wtk_xbnf_rec_path_t *pth)
{
	wtk_xbnf_rec_path_v_t *v;
	int i;

	wtk_debug("=============== pth=%p [min=%d,max=%d,set=%p,type=%c] ==============\n",pth,
			pth->item->attr.min,pth->item->attr.max,pth->set,
			wtk_xbnf_item_type_get_char(pth->item->type));
	wtk_xbnf_item_print(pth->item);
	printf("\n");
	v=pth->v;
	i=0;
	while(v)
	{
		printf("v[%d]=[%.*s] set=%p\n",i,v->v->len,v->v->data,pth->set);
		++i;
		v=v->nxt;
	}
}

void wtk_xbnf_rec_path_print2(wtk_xbnf_rec_path_t *pth)
{
	wtk_xbnf_rec_path_v_t *v;
	int i;

	/*
	wtk_debug("=============== pth=%p [min=%d,max=%d,set=%p,type=%c] ==============\n",pth,
			pth->item->attr.min,pth->item->attr.max,pth->set,
			wtk_xbnf_item_type_get_char(pth->item->type));*/
	if(pth->prev)
	{
		wtk_xbnf_rec_path_print2(pth->prev);
	}else
	{
		if(!pth->item)
		{
			printf("$MAIN\n");
			return;
		}
	}
	wtk_xbnf_item_print(pth->item);
	printf(" pth=%p prev=%p nwrd=%d",pth,pth->prev,pth->nwrd);
	printf("\n");
	v=pth->v;
	i=0;
	while(v)
	{
		printf("   v[%d]=%.*s\n",i,v->v->len,v->v->data);
		++i;
		v=v->nxt;
	}
}

wtk_xbnf_rec_t* wtk_xbnf_rec_new(wtk_xbnf_rec_cfg_t *cfg)
{
	wtk_xbnf_rec_t *r;

	r=(wtk_xbnf_rec_t*)wtk_malloc(sizeof(wtk_xbnf_rec_t));
	r->cfg=cfg;
	r->tmp_buf=wtk_strbuf_new(256,1);
	r->buf=wtk_strbuf_new(256,1);
	r->pth_buf=wtk_strbuf_new(256,1);
	r->heap=wtk_heap_new(4096);
	r->inst_pool=wtk_vpool_new(sizeof(wtk_xbnf_rec_inst_t),cfg->inst_cache);
	wtk_xbnf_rec_reset(r);
	return r;
}

void wtk_xbnf_rec_delete(wtk_xbnf_rec_t *r)
{
	wtk_strbuf_delete(r->pth_buf);
	wtk_vpool_delete(r->inst_pool);
	wtk_strbuf_delete(r->tmp_buf);
	wtk_heap_delete(r->heap);
	wtk_strbuf_delete(r->buf);
	wtk_free(r);
}

void wtk_xbnf_rec_reset(wtk_xbnf_rec_t *r)
{
	wtk_vpool_reset(r->inst_pool);
	wtk_heap_reset(r->heap);
	wtk_queue_init(&(r->inst_q));
	r->output.nwrd=0;
	r->output.v=NULL;
	r->eof=0;
}

wtk_xbnf_rec_path_t *wtk_xbnf_rec_new_path(wtk_xbnf_rec_t *r,wtk_xbnf_item_set_t *set,
		wtk_xbnf_item_t *item)
{
	wtk_xbnf_rec_path_t *pth;

	pth=(wtk_xbnf_rec_path_t*)wtk_heap_malloc(r->heap,sizeof(wtk_xbnf_rec_path_t));
	pth->item=item;
	pth->prev=NULL;
	pth->v=NULL;
	pth->nv=0;
	pth->nwrd=0;
	pth->set=set;
	pth->pos_s=-1;
	return pth;
}

wtk_xbnf_rec_path_v_t* wtk_xbnf_rec_new_path_v(wtk_xbnf_rec_t *r,char *data,int len)
{
	wtk_xbnf_rec_path_v_t *pth;

	pth=(wtk_xbnf_rec_path_v_t*)wtk_heap_malloc(r->heap,sizeof(wtk_xbnf_rec_path_v_t));
	pth->nxt=NULL;
	pth->v=wtk_heap_dup_string(r->heap,data,len);
	return pth;
}

wtk_xbnf_rec_path_v_t* wtk_xbnf_rec_dup_path_v(wtk_xbnf_rec_t *r,wtk_xbnf_rec_path_v_t *src)
{
	wtk_xbnf_rec_path_v_t *dst,*v1;

	dst=wtk_xbnf_rec_new_path_v(r,src->v->data,src->v->len);
	v1=dst;
	while(src->nxt)
	{
		src=src->nxt;
		v1->nxt=wtk_xbnf_rec_new_path_v(r,src->v->data,src->v->len);
		v1=v1->nxt;
	}
	return dst;
}

void wtk_xbnf_rec_path_nv_tostring(wtk_xbnf_rec_path_t *pth,wtk_xbnf_rec_path_v_t *nxt,wtk_strbuf_t *buf)
{
	wtk_string_t *v;

	if(nxt->nxt)
	{
		wtk_xbnf_rec_path_nv_tostring(pth,nxt->nxt,buf);
	}
	if(pth->item->attr.v)
	{
		v=pth->item->attr.v;
	}else
	{
		v=nxt->v;
	}
	if(pth->item->attr.k)
	{
		wtk_strbuf_push_add_escape_str(buf,v->data,v->len);
	}else
	{
		wtk_strbuf_push(buf,v->data,v->len);
	}
}

void wtk_xbnf_rec_path_attch_info(wtk_xbnf_rec_t *r,wtk_xbnf_rec_path_t *pth,wtk_strbuf_t *buf)
{
	wtk_queue_node_t *qn;

	wtk_xbnf_item_attr_item_t *vi;
	if(pth->item)
	{
		for(qn=pth->item->attr.attr_q.pop;qn;qn=qn->next)
		{
			vi=data_offset(qn,wtk_xbnf_item_attr_item_t,q_n);
			if(vi->k && vi->v)
			{
				wtk_strbuf_push_s(buf,"[");
				wtk_strbuf_push(buf,vi->k->data,vi->k->len);
				//wtk_strbuf_push_add_escape_str(buf,pth->item->attr.k->data,pth->item->attr.k->len);
				wtk_strbuf_push_s(buf,"=\"");
				wtk_strbuf_push(buf,vi->v->data,vi->v->len);
				wtk_strbuf_push_s(buf,"\"]");
			}
		}
	}
}

void wtk_xbnf_rec_path_get_output(wtk_xbnf_rec_t *r,wtk_xbnf_rec_path_t *pth,wtk_strbuf_t *buf)
{
	wtk_strbuf_reset(buf);
	if(pth->nv>1)
	{
		if(pth->item->attr.k)
		{
			wtk_strbuf_push_s(buf,"[");
			wtk_strbuf_push(buf,pth->item->attr.k->data,pth->item->attr.k->len);
			//wtk_strbuf_push_add_escape_str(buf,pth->item->attr.k->data,pth->item->attr.k->len);
			wtk_strbuf_push_f(buf,"-pos=\"%d\"]",pth->pos_s);

			wtk_strbuf_push_s(buf,"[");
			wtk_strbuf_push(buf,pth->item->attr.k->data,pth->item->attr.k->len);
			//wtk_strbuf_push_add_escape_str(buf,pth->item->attr.k->data,pth->item->attr.k->len);
			wtk_strbuf_push_f(buf,"-nwrd=\"%d\"]",pth->nwrd);

			wtk_strbuf_push_s(buf,"[");
			wtk_strbuf_push(buf,pth->item->attr.k->data,pth->item->attr.k->len);
			//wtk_strbuf_push_add_escape_str(buf,pth->item->attr.k->data,pth->item->attr.k->len);
			wtk_strbuf_push_s(buf,"=\"");
			//wtk_debug("nv=%d nwrd=%d %.*s\n",pth->nv,pth->nwrd,buf->pos,buf->data);
		}
		//wtk_debug("pos=%d nv=%d nwrd=%d %.*s\n",pth->pos_s,pth->nv,pth->nwrd,buf->pos,buf->data);
		wtk_xbnf_rec_path_nv_tostring(pth,pth->v,buf);
		if(pth->item->attr.k)
		{
			wtk_strbuf_push_s(buf,"\"]");
		}
	}else if(pth->nv>0)
	{
		//wtk_debug("nv=%d nwrd=%d %.*s\n",pth->nv,pth->nwrd,buf->pos,buf->data);
		wtk_strbuf_push(buf,pth->v->v->data,pth->v->v->len);
	}
	wtk_xbnf_rec_path_attch_info(r,pth,buf);
	//wtk_debug("[%.*s] nv=%d\n",buf->pos,buf->data,pth->nv);
}

void wtk_xbnf_rec_add_path_v(wtk_xbnf_rec_t *r,wtk_xbnf_rec_path_t *pth,char *data,int bytes)
{
	wtk_xbnf_rec_path_v_t *v1;
	wtk_strbuf_t *buf;

	if(pth->item && (pth->item->attr.max<=1) && (pth->item->attr.k||pth->item->attr.v || pth->item->attr.attr_q.length>0))
	{
		buf=r->pth_buf;
		if(pth->item->attr.k)
		{
			wtk_strbuf_reset(buf);
			wtk_strbuf_push_s(buf,"[");
			wtk_strbuf_push(buf,pth->item->attr.k->data,pth->item->attr.k->len);
			//wtk_strbuf_push_add_escape_str(buf,pth->item->attr.k->data,pth->item->attr.k->len);
			wtk_strbuf_push_s(buf,"=\"");
			if(pth->item->attr.v)
			{
				wtk_strbuf_push_add_escape_str(buf,pth->item->attr.v->data,pth->item->attr.v->len);
			}else
			{
				wtk_strbuf_push_add_escape_str(buf,data,bytes);
			}
			wtk_strbuf_push_s(buf,"\"]");
			//wtk_debug("pos=%d nv=%d nwrd=%d %.*s\n",pth->pos_s,pth->nv,pth->nwrd,buf->pos,buf->data);
			//exit(0);
			wtk_xbnf_rec_path_attch_info(r,pth,buf);
			data=buf->data;
			bytes=buf->pos;
		}else
		{
			if(pth->item && pth->item->attr.attr_q.length>0)
			{
				wtk_strbuf_reset(buf);
				wtk_strbuf_push(buf,data,bytes);
				wtk_xbnf_rec_path_attch_info(r,pth,buf);
				data=buf->data;
				bytes=buf->pos;
			}else
			{
				data=pth->item->attr.v->data;
				bytes=pth->item->attr.v->len;
			}
		}
	}
	v1=wtk_xbnf_rec_new_path_v(r,data,bytes);
	v1->nxt=pth->v;
	pth->v=v1;
	++pth->nv;
}

wtk_xbnf_rec_path_t* wtk_xbnf_rec_dup_path(wtk_xbnf_rec_t *r,wtk_xbnf_rec_path_t *prev)
{
	wtk_xbnf_rec_path_t *pth,*pth1;

	if(!prev){return NULL;}
	pth=wtk_xbnf_rec_new_path(r,prev->set,prev->item);
	pth->pos_s=prev->pos_s;
	pth->nv=prev->nv;
	pth->nwrd=prev->nwrd;
	if(prev->v)
	{
		pth->v=wtk_xbnf_rec_dup_path_v(r,prev->v);
	}
	pth1=pth;
	while(prev->prev)
	{
		prev=prev->prev;
		pth1->prev=wtk_xbnf_rec_new_path(r,prev->set,prev->item);
		pth1=pth1->prev;
		pth1->pos_s=prev->pos_s;
		//wtk_debug("pos_s=%d\n",prev->pos_s);
		pth1->nv=prev->nv;
		pth1->nwrd=prev->nwrd;
		if(prev->v)
		{
			pth1->v=wtk_xbnf_rec_dup_path_v(r,prev->v);
		}
	}
	return pth;
}

wtk_xbnf_rec_path_t* wtk_xbnf_rec_append_path(wtk_xbnf_rec_t *r,wtk_xbnf_rec_path_t *prev,
		wtk_xbnf_item_set_t *set,wtk_xbnf_item_t *item)
{
	wtk_xbnf_rec_path_t *pth;

	pth=wtk_xbnf_rec_new_path(r,set,item);
	if(prev)
	{
		pth->pos_s=prev->pos_s+prev->nwrd;
		pth->prev=wtk_xbnf_rec_dup_path(r,prev);
	}else
	{
		pth->pos_s=-1;
	}
	return pth;
}

wtk_xbnf_rec_inst_t* wtk_xbnf_rec_pop_inst(wtk_xbnf_rec_t *r,wtk_xbnf_item_t *item)
{
	wtk_xbnf_rec_inst_t *inst;

	inst=(wtk_xbnf_rec_inst_t*)wtk_vpool_pop(r->inst_pool);
	//wtk_queue_init(&(inst->item_path_q));
	inst->item=item;
	inst->last=NULL;
	//inst->nxt=NULL;
	inst->repeat=0;
	inst->path=NULL;
	return inst;
}

void wtk_xbnf_rec_push_inst(wtk_xbnf_rec_t *r,wtk_xbnf_rec_inst_t *inst)
{
	wtk_vpool_push(r->inst_pool,inst);
}


int wtk_xbnf_rec_path_tostring(wtk_xbnf_rec_t *r,wtk_xbnf_rec_path_t *pth,wtk_strbuf_t *buf)
{
	int cnt;

	cnt=0;
	if(pth->prev)
	{
		cnt+=wtk_xbnf_rec_path_tostring(r,pth->prev,buf);
	}
	wtk_xbnf_rec_path_get_output(r,pth,r->pth_buf);
	//wtk_strbuf_push_s(buf," ");
	//wtk_debug("[%.*s]\n",r->pth_buf->pos,r->pth_buf->data);
	wtk_strbuf_push(buf,r->pth_buf->data,r->pth_buf->pos);
	cnt+=pth->nwrd;
	return cnt;
}

void wtk_xbnf_rec_add_output(wtk_xbnf_rec_t *r,wtk_xbnf_rec_path_t *pth)
{
	wtk_strbuf_t *buf=r->buf;
	int nwrd;

	//wtk_xbnf_rec_path_print2(pth);
	wtk_strbuf_reset(buf);
	nwrd=wtk_xbnf_rec_path_tostring(r,pth,buf);
	if(nwrd<=0 || buf->pos<=0){return;}
	//wtk_xbnf_rec_path_print2(pth);
	//exit(0);
	if(nwrd>=r->output.nwrd)
	{
		//wtk_debug("nwrd=%d [%.*s]\n",nwrd,buf->pos,buf->data);
		r->output.nwrd=nwrd;
		r->output.v=wtk_heap_dup_string(r->heap,buf->data,buf->pos);
	}
}

void wtk_xbnf_rec_add_item(wtk_xbnf_rec_t *r,wtk_xbnf_item_set_t *set,wtk_xbnf_item_t *item,wtk_xbnf_rec_path_t *prev)
{
	wtk_xbnf_rec_inst_t *inst;

	prev=wtk_xbnf_rec_append_path(r,prev,set,item);
	switch(item->type)
	{
	case WTK_XBNF_ITEM_STR:
		inst=wtk_xbnf_rec_pop_inst(r,item);
		inst->path=prev;
		//wtk_debug("[%.*s]\n",item->v.str->len,item->v.str->data);
		wtk_queue_push(&(r->inst_q),&(inst->q_n));
		break;
	case WTK_XBNF_ITEM_VAR:
		wtk_xbnf_rec_add_expr(r,item->v.expr,prev);
		break;
	case WTK_XBNF_ITEM_PARENTHESIS:
		wtk_xbnf_rec_add_set(r,item->v.set,prev);
		break;
	case WTK_XBNF_ITEM_BRACKET:
		wtk_xbnf_rec_add_set(r,item->v.set,prev);
		break;
	case WTK_XBNF_ITEM_BRACE:
		wtk_xbnf_rec_add_set(r,item->v.set,prev);
		break;
	case WTK_XBNF_ITEM_ANGLE:
		wtk_xbnf_rec_add_set(r,item->v.set,prev);
		break;
	case WTK_XBNF_ITEM_BRANCH:
		wtk_xbnf_rec_add_set(r,item->v.set,prev);
		break;
	}
}

void wtk_xbnf_rec_add_list(wtk_xbnf_rec_t *r,wtk_xbnf_item_set_t *set,wtk_xbnf_item_list_t *list,wtk_xbnf_rec_path_t *prev)
{
	wtk_queue_node_t *qn;
	wtk_xbnf_item_t *item;

	for(qn=list->list_q.pop;qn;qn=qn->next)
	{
		item=data_offset(qn,wtk_xbnf_item_t,q_n);
		if(item->start)
		{
			continue;
		}
		if(item->eof)
		{
			wtk_xbnf_rec_add_output(r,prev);
			continue;
		}
		wtk_xbnf_rec_add_item(r,set,item,prev);
		if(item->attr.min!=0)
		{
			break;
		}
	}
}

void wtk_xbnf_rec_add_set(wtk_xbnf_rec_t *r,wtk_xbnf_item_set_t *set,wtk_xbnf_rec_path_t *prev)
{
	wtk_queue_node_t *qn;
	wtk_xbnf_item_list_t *list;

	for(qn=set->set_q.pop;qn;qn=qn->next)
	{
		list=data_offset(qn,wtk_xbnf_item_list_t,q_n);
		wtk_xbnf_rec_add_list(r,set,list,prev);
	}
}

void wtk_xbnf_rec_add_expr(wtk_xbnf_rec_t *r,wtk_xbnf_expr_t *expr,wtk_xbnf_rec_path_t *prev)
{
	wtk_xbnf_rec_add_set(r,expr->set,prev);
}

void wtk_xbnf_rec_start(wtk_xbnf_rec_t *r)
{
	wtk_xbnf_expr_t *expr=r->cfg->xb->main_expr;
	wtk_xbnf_rec_path_t *pth;

	pth=wtk_xbnf_rec_new_path(r,NULL,NULL);
	pth->pos_s=0;
	wtk_xbnf_rec_add_expr(r,expr,pth);
}



wtk_xbnf_rec_path_t* wtk_xbnf_rec_merge_subexpr(wtk_xbnf_rec_t *r,wtk_xbnf_rec_path_t *pth)
{
	wtk_xbnf_rec_path_t *np,*prev;
	wtk_xbnf_rec_path_v_t *v;
	wtk_strbuf_t *buf;
	wtk_xbnf_item_set_t *set;
	int nw=0;

	//wtk_debug("pos=%d nwrd=%d\n",pth->pos_s,pth->nwrd);
	//wtk_debug("-----------------------\n");
	buf=r->tmp_buf;
	np=pth;
	prev=NULL;
	wtk_strbuf_reset(buf);
	set=pth->set;
	while(np)
	{
		if(np->item->type!=WTK_XBNF_ITEM_STR)
		{
			if((np->item->type==WTK_XBNF_ITEM_VAR && np->item->v.expr->set==set)||(np->item->v.set==set))
			{
				//wtk_debug("prev=%p/%p\n",prev,prev->prev);
				prev->prev=NULL;
				//wtk_debug("==> merge x\n");
				//wtk_xbnf_rec_path_print2(np);
				np->nwrd+=nw;
				//wtk_debug("%.*s\n",buf->pos,buf->data);
				wtk_xbnf_rec_add_path_v(r,np,buf->data,buf->pos);
				return np;
			}
		}
		if(np->set!=set)
		{
			//wtk_xbnf_rec_path_print(np);
			wtk_debug("found bug %p/%p\n",np->set,set);
			//exit(0);
		}
		//wtk_debug("set=%p/%p\n",np->set,set);
		//wtk_xbnf_rec_path_print(np);
		prev=np;
		if(prev->pos_s==-1)
		{
			prev->pos_s=np->pos_s;
		}
		nw+=np->nwrd;
		if(np->nv>1)
		{
			wtk_xbnf_rec_path_get_output(r,np,r->pth_buf);
			wtk_strbuf_push_front(buf,r->pth_buf->data,r->pth_buf->pos);
		}else
		{
			v=np->v;
			while(v)
			{
				if(buf->pos>0 && ((unsigned char)v->v->data[0])<='z')
				{
					//print_hex(v->v->data,v->v->len);
					//wtk_debug("push %d %.*s\n",'z',v->v->len,v->v->data);
					wtk_strbuf_push_front_s(buf," ");
				}
				wtk_strbuf_push_front(buf,v->v->data,v->v->len);
				v=v->nxt;
			}
		}
		np=np->prev;
	}
	return NULL;
}

void wtk_xbnf_rec_expand(wtk_xbnf_rec_t *r,wtk_xbnf_rec_path_t *pth,wtk_xbnf_item_t *item)
{
	wtk_xbnf_item_t *item1;
	//wtk_xbnf_item_set_t *set;

	if(item->q_n.next)
	{
		item1=data_offset(item->q_n.next,wtk_xbnf_item_t,q_n);
		if(item1->eof)
		{
			if(r->eof)
			{
				wtk_xbnf_rec_add_output(r,pth);
			}
		}else
		{
			if(item1->start)
			{
				wtk_xbnf_rec_expand(r,pth,item1);
			}else
			{
				wtk_xbnf_rec_add_item(r,pth->set,item1,pth);
			}
		}
		if(item1->attr.min==0)
		{
			pth=wtk_xbnf_rec_append_path(r,pth,pth->set,item1);
			wtk_xbnf_rec_expand(r,pth,item1);
		}
	}else
	{
		if(pth->set==r->cfg->xb->main_expr->set)
		{
			//wtk_xbnf_rec_path_print(pth);
			wtk_debug("found bug next=%p parent=%p\n",item->q_n.next,pth->set);
		}else // if(pth->item->type==WTK_XBNF_ITEM_STR)
		{
			//wtk_xbnf_rec_path_print(pth);
			//wtk_debug("prev=%p\n",pth->prev);
			if(pth->prev)
			{
				pth=wtk_xbnf_rec_dup_path(r,pth);
				pth=wtk_xbnf_rec_merge_subexpr(r,pth);
				//wtk_xbnf_rec_path_print(pth);
				if(!pth)
				{
					wtk_debug("found null\n");
					return;
				}
				if(pth->nv>=pth->item->attr.min)
				{
					wtk_xbnf_rec_expand(r,pth,pth->item);
				}
				if(pth->nv<pth->item->attr.max)
				{
					if(pth->item->type==WTK_XBNF_ITEM_VAR)
					{
						wtk_xbnf_rec_add_expr(r,pth->item->v.expr,pth);
					}else
					{
						wtk_xbnf_rec_add_set(r,pth->item->v.set,pth);
					}
				}
			}else
			{
				//never be herer;
			}
		}
	}
}

void wtk_xbnf_rec_step_inst(wtk_xbnf_rec_t *r,wtk_xbnf_rec_inst_t *inst,wtk_string_t *v)
{
	wtk_xbnf_rec_path_t *pth;//,*pth2;

	pth=inst->path;
	//wtk_debug("pth=%p/%p\n",pth->item,inst->item);
	//wtk_xbnf_rec_path_print(pth);
	if(pth->pos_s==-1)
	{
		pth->pos_s=r->pos;
	}
	++pth->nwrd;
	//wtk_debug("pos=%d [%.*s]\n",r->pos,v->len,v->data);
	wtk_xbnf_rec_add_path_v(r,pth,v->data,v->len);
	//wtk_xbnf_rec_path_print2(pth);
	if(pth->nv>=inst->item->attr.min)
	{
		//pth2=wtk_xbnf_rec_dup_path2(r,pth);
		wtk_xbnf_rec_expand(r,pth,inst->item);
	}
	if(pth->nv<inst->item->attr.max)
	{
		wtk_queue_push(&(r->inst_q),&(inst->q_n));
	}
}


int wtk_xbnf_rec_process_inst(wtk_xbnf_rec_t *r,wtk_xbnf_rec_inst_t *inst,wtk_string_t *v)
{
	wtk_xbnf_item_t *item;
	wtk_strbuf_t *buf=r->tmp_buf;
	int ret;

	item=inst->item;
	//wtk_xbnf_item_print(item);
	//printf("\n");
	if(inst->last)
	{
		if(v->len+inst->last->len>item->v.str->len){goto end;}
		wtk_strbuf_reset(buf);
		wtk_strbuf_push(buf,inst->last->data,inst->last->len);
		wtk_strbuf_push(buf,v->data,v->len);
		//wtk_debug("[%.*s]\n",item->v.str->len,item->v.str->data);
		if(wtk_string_cmp(item->v.str,buf->data,buf->pos)==0)
		{
			if(buf->pos==item->v.str->len)
			{
				wtk_xbnf_rec_step_inst(r,inst,item->v.str);
			}else
			{
				inst->last=wtk_heap_dup_string(r->heap,buf->data,buf->pos);
				wtk_queue_push(&(r->inst_q),&(inst->q_n));
			}
		}else
		{
			wtk_xbnf_rec_push_inst(r,inst);
		}
	}else
	{
		ret=item->v.str->len-v->len;
		if(ret<0)
		{
			wtk_xbnf_rec_push_inst(r,inst);
			goto end;
		}
		//wtk_debug("[%.*s]\n",item->v.str->len,item->v.str->data);
		if(ret==0)
		{
			if(wtk_string_cmp(item->v.str,v->data,v->len)==0)
			{
				wtk_xbnf_rec_step_inst(r,inst,item->v.str);
			}else
			{
				wtk_xbnf_rec_push_inst(r,inst);
			}
		}else
		{
			if(wtk_string_cmp(item->v.str,v->data,v->len)==0)
			{
				inst->last=wtk_heap_dup_string(r->heap,v->data,v->len);
				wtk_queue_push(&(r->inst_q),&(inst->q_n));
			}else
			{
				wtk_xbnf_rec_push_inst(r,inst);
			}
		}
	}
end:
	return 0;
}


int wtk_xbnf_rec_feed(wtk_xbnf_rec_t *r,wtk_string_t *v)
{
	wtk_queue_node_t *qn;
	wtk_xbnf_rec_inst_t *inst;
	wtk_queue_t q;
	int ret;

	q=r->inst_q;
	wtk_queue_init(&(r->inst_q));
	while(1)
	{
		qn=wtk_queue_pop(&(q));
		if(!qn){break;}
		inst=data_offset(qn,wtk_xbnf_rec_inst_t,q_n);
		ret=wtk_xbnf_rec_process_inst(r,inst,v);
		if(ret!=0){goto end;}
	}
	//wtk_debug("[%.*s]=%d\n",v->len,v->data,r->inst_q.length);
	ret=0;
end:
	//wtk_debug("[%.*s]=%d\n",v->len,v->data,r->inst_q.length);
	return ret;
}

void wtk_xbnf_rec_update_eof(wtk_xbnf_rec_t *r)
{
	wtk_queue_node_t *qn;
	wtk_xbnf_rec_inst_t *inst;
	//wtk_xbnf_rec_path_t *pth;

	while(1)
	{
		qn=wtk_queue_pop(&(r->inst_q));
		if(!qn){break;}
		inst=data_offset(qn,wtk_xbnf_rec_inst_t,q_n);
		//wtk_xbnf_rec_path_print2(inst->path);
		if(inst->path)
		{
			wtk_xbnf_rec_add_output(r,inst->path);
		}
	}
}

void wtk_xbnf_rec_print(wtk_xbnf_rec_t *r)
{
	wtk_debug("================ nwrd=%d ===========\n",r->output.nwrd);
	if(r->output.v)
	{
		printf("%.*s\n",r->output.v->len,r->output.v->data);
	}
}

void wtk_xbnf_rec_add_json(wtk_xbnf_rec_t *r,wtk_json_t *json,	wtk_json_item_t* obj,char* data,int bytes)
{
typedef enum
{
	WTK_XBNF_REC_INIT,
	WTK_XBNF_REC_WAIT_K,
	WTK_XBNF_REC_K,
	WTK_XBNF_REC_WAIT_EQ,
	WTK_XBNF_REC_WAIT_V,
	WTK_XBNF_REC_V,
}wtk_xbnf_rec_output_state_t;
	wtk_xbnf_rec_output_state_t state;
	char *s,*e;
	char c;
	wtk_strbuf_t *buf=r->buf;
	wtk_string_t *k=NULL,*v;
	wtk_json_item_t *item2;
	int depth;
	int dx;

	//wtk_debug("[%.*s]\n",bytes,data);
	s=data;
	e=s+bytes;
	state=WTK_XBNF_REC_INIT;
	depth=dx=0;
	while(s<e)
	{
		c=*s;
		//wtk_debug("%d=%c\n",state,c);
		switch(state)
		{
		case WTK_XBNF_REC_INIT:
			if(c=='[')
			{
				state=WTK_XBNF_REC_WAIT_K;
			}
			break;
		case WTK_XBNF_REC_WAIT_K:
			if(!isspace(c) && c!=',')
			{
				if(c==']')
				{
					state=WTK_XBNF_REC_INIT;
				}else
				{
					wtk_strbuf_reset(buf);
					wtk_strbuf_push_c(buf,c);
					state=WTK_XBNF_REC_K;
				}
			}
			break;
		case WTK_XBNF_REC_K:
			if(c=='=' || isspace(c))
			{
				k=wtk_heap_dup_string(r->heap,buf->data,buf->pos);
				//wtk_debug("[%.*s]\n",k->len,k->data);
				if(c=='=')
				{
					state=WTK_XBNF_REC_WAIT_V;
					depth=0;
				}else
				{
					state=WTK_XBNF_REC_WAIT_EQ;
				}
			}else
			{
				wtk_strbuf_push_c(buf,c);
			}
			break;
		case WTK_XBNF_REC_WAIT_EQ:
			if(c=='=')
			{
				state=WTK_XBNF_REC_WAIT_V;
				depth=0;
			}
			break;
		case WTK_XBNF_REC_WAIT_V:
			if(!isspace(c))
			{
				if(c=='"')
				{
					wtk_strbuf_reset(buf);
					state=WTK_XBNF_REC_V;
					dx=0;
				}else if(c=='\\')
				{
					++depth;
				}
			}
			break;
		case WTK_XBNF_REC_V:
			//wtk_debug("depth=%d\n",depth);
			if(c=='\\')
			{
				++dx;
			}else if(c!='"')
			{
				dx=0;
			}
			if(c=='"' && dx==depth)//||(c==']'&& depth==0))
			{
				//wtk_strbuf_push_c(buf,c);
				v=wtk_heap_dup_string(r->heap,buf->data,buf->pos-depth);
				//wtk_strbuf_parse_quote(buf,v->data,v->len);
				//wtk_debug("%.*s\n",v->len,v->data);
				//wtk_debug("%.*s\n",buf->pos,buf->data);
				if(v->data[0]=='[')
				{
					item2=wtk_json_new_object(json);
					//wtk_debug("[%.*s]\n",k->len,k->data);
					wtk_json_obj_add_item2(json,obj,k->data,k->len,item2);
					wtk_xbnf_rec_add_json(r,json,item2,v->data,v->len);
				}else
				{
					wtk_json_obj_add_ref_str(json,obj,k->data,k->len,v);
				}
				if(c=='"')
				{
					state=WTK_XBNF_REC_WAIT_K;
				}else
				{
					state=WTK_XBNF_REC_INIT;
				}
			}else
			{
				wtk_strbuf_push_c(buf,c);
			}
			break;
		}
		++s;
	}
	return;
}


wtk_json_item_t* wtk_xbnf_rec_get_json(wtk_xbnf_rec_t *r,wtk_json_t *json)
{
	wtk_json_item_t* obj;

	obj=wtk_json_new_object(json);
	if(!r->output.v){goto end;}
	wtk_xbnf_rec_add_json(r,json,obj,r->output.v->data,r->output.v->len);
end:
	return obj;
}


int wtk_xbnf_rec_process(wtk_xbnf_rec_t *r,char *data,int bytes)
{
	wtk_strbuf_t *buf=r->buf;
        int ret, chn_wrd;
        char *s,*e;
	wtk_string_t v;

	wtk_xbnf_rec_start(r);
	s=data;e=s+bytes;
	r->eof=0;
	r->pos=0;
	while(s<e)
	{
            s = wtk_xbnf_cfg_next_wrd(&(r->cfg->xbnf), s, e, buf, &chn_wrd);
            if (s >= e) {
                r->eof = 1;
		}
		wtk_string_set(&(v),buf->data,buf->pos);
		//wtk_debug("[%.*s]\n",v.len,v.data);
		if(v.len==1 && isspace(*v.data))
		{
			continue;
		}
		++r->pos;
		//wtk_debug("v[%.*s]=%d\n",v.len,v.data,r->inst_q.length);
		ret=wtk_xbnf_rec_feed(r,&v);
		if(ret!=0)
		{
			goto end;
		}
		if(r->inst_q.length==0)
		{
			//wtk_debug("break\n");
			break;
		}
	}
	if(r->output.nwrd==0 && r->inst_q.length>0)
	{
		wtk_xbnf_rec_update_eof(r);
	}
	ret=0;
end:
	//wtk_xbnf_rec_print(r);
	//wtk_debug("len=%d\n",r->inst_q.length);
	return ret;
}
