#include "wtk_xbnf_expand.h" 


wtk_xbnf_expand_t* wtk_xbnf_expand_new(wtk_xbnf_t *xbnf)
{
	wtk_xbnf_expand_t *t;

	t=(wtk_xbnf_expand_t*)wtk_malloc(sizeof(wtk_xbnf_expand_t));
	t->xbnf=xbnf;
	t->heap=wtk_heap_new(4096);
	wtk_queue_init(&(t->inst_q));
	return t;
}

void wtk_xbnf_expand_delete(wtk_xbnf_expand_t *e)
{
	wtk_heap_delete(e->heap);
	wtk_free(e);
}

wtk_xbnf_expand_inst_t* wtk_xbnf_expand_pop_inst(wtk_xbnf_expand_t *e)
{
	wtk_xbnf_expand_inst_t *inst;

	inst=(wtk_xbnf_expand_inst_t*)wtk_heap_malloc(e->heap,sizeof(wtk_xbnf_expand_inst_t));
	inst->prev=NULL;
	wtk_queue_init(&(inst->pth_q));
	return inst;
}

wtk_xbnf_expand_pth_t* wtk_xbnf_pop_path(wtk_xbnf_expand_t *e,char *data,int len)
{
	wtk_heap_t *heap=e->heap;
	wtk_xbnf_expand_pth_t *pth;

	pth=(wtk_xbnf_expand_pth_t*)wtk_heap_malloc(heap,sizeof(wtk_xbnf_expand_pth_t));
	if(len>0)
	{
		pth->str=wtk_heap_dup_string(heap,data,len);
	}else
	{
		pth->str=NULL;
	}
	return pth;
}

void wtk_xbnf_expand_delete_a(wtk_larray_t *a)
{
	wtk_strbuf_t **bufs;
	int i;

	bufs=(wtk_strbuf_t**)a->slot;
	for(i=0;i<a->nslot;++i)
	{
		wtk_strbuf_delete(bufs[i]);
	}
	wtk_larray_delete(a);
}

void wtk_xbnf_expand_process_set(wtk_xbnf_expand_t *e,wtk_xbnf_item_set_t *set,wtk_larray_t *a);

void wtk_xbfn_array_pad_nil(wtk_larray_t *a)
{
	wtk_strbuf_t *buf;

	buf=wtk_strbuf_new(256,1);
	wtk_larray_push2(a,&(buf));
}

void wtk_xbfn_array_pad_str(wtk_larray_t *a,char *data,int len)
{
	wtk_strbuf_t *buf;

	buf=wtk_strbuf_new(256,1);
	if(len>0)
	{
		wtk_strbuf_push(buf,data,len);
	}
	wtk_larray_push2(a,&(buf));
}

void wtk_larray_add_pad_str(wtk_larray_t *a,char *data,int len)
{
	wtk_strbuf_t **bufs;
	int i;

	bufs=(wtk_strbuf_t**)a->slot;
	for(i=0;i<a->nslot;++i)
	{
		wtk_strbuf_push(bufs[i],data,len);
		wtk_debug("[%.*s]\n",bufs[i]->pos,bufs[i]->data);
	}

}

void wtk_larray_add_array(wtk_larray_t *a,wtk_larray_t *b)
{
	wtk_strbuf_t **bufs;
	int i;

	bufs=(wtk_strbuf_t**)b->slot;
	for(i=0;i<b->nslot;++i)
	{
		wtk_xbfn_array_pad_str(a,bufs[i]->data,bufs[i]->pos);
	}

}

wtk_larray_t* wtk_xbnf_array_xyz(wtk_larray_t *a,wtk_larray_t *b)
{
	wtk_larray_t *c;
	wtk_strbuf_t **bufs1,**bufs2;
	int i,j;
	wtk_strbuf_t *buf;

	c=wtk_larray_new(32,sizeof(void*));
	bufs1=(wtk_strbuf_t**)a->slot;
	bufs2=(wtk_strbuf_t**)b->slot;
	if(a->nslot>0)
	{
		for(i=0;i<b->nslot;++i)
		{
			for(j=0;j<a->nslot;++j)
			{
				buf=wtk_strbuf_new(256,1);
				wtk_strbuf_push(buf,bufs1[j]->data,bufs1[j]->pos);
				wtk_strbuf_push(buf,bufs2[i]->data,bufs2[i]->pos);
				if(0)
				{
					static int ki=0;

					++ki;
					wtk_debug("v[%d]=[%.*s]\n",ki,buf->pos,buf->data);
					if(ki==4)
					{
						//exit(0);
					}
				}
				wtk_larray_push2(c,&(buf));
			}
		}
	}else
	{
		for(i=0;i<b->nslot;++i)
		{
			buf=wtk_strbuf_new(256,1);
			wtk_strbuf_push(buf,bufs2[i]->data,bufs2[i]->pos);
			//wtk_debug("[%.*s]\n",buf->pos,buf->data);
			wtk_larray_push2(c,&(buf));
		}
	}
	return c;
}

void wtk_xbnf_expand_process_item(wtk_xbnf_expand_t *e,wtk_xbnf_item_t *item,wtk_larray_t *a)
{
	wtk_xbnf_item_set_t *set;
	wtk_larray_t *b,*c,*d;
	wtk_string_t *v;
	wtk_strbuf_t *buf;
	int i;

	if(item->type==WTK_XBNF_ITEM_STR)
	{
		v=item->v.str;
		if(item->attr.min==0)
		{
			wtk_xbfn_array_pad_nil(a);
		}
		buf=wtk_strbuf_new(256,1);
		for(i=0;i<item->attr.min-1;++i)
		{
			wtk_strbuf_push(buf,v->data,v->len);
		}
		for(i=item->attr.min;i<=item->attr.max;++i)
		{
			wtk_strbuf_push(buf,v->data,v->len);
			wtk_xbfn_array_pad_str(a,buf->data,buf->pos);
		}
		wtk_strbuf_delete(buf);
	}else
	{
		if(item->type==WTK_XBNF_ITEM_VAR)
		{
			set=item->v.expr->set;
		}else
		{
			set=item->v.set;
		}
		if(item->attr.min==0)
		{
			wtk_xbfn_array_pad_nil(a);
		}
		b=wtk_larray_new(32,sizeof(void*));
		wtk_xbnf_expand_process_set(e,set,b);
		c=wtk_larray_new(32,sizeof(void*));
		for(i=0;i<item->attr.min;++i)
		{
			d=wtk_xbnf_array_xyz(c,b);
			wtk_xbnf_expand_delete_a(c);
			c=d;
		}
		for(i=item->attr.min;i<=item->attr.max;++i)
		{
			wtk_larray_add_array(a,c);
			if(i<(item->attr.max-1))
			{
				d=wtk_xbnf_array_xyz(c,b);
				wtk_xbnf_expand_delete_a(c);
				c=d;
			}
		}
		//wtk_debug("a=%d min=%d max=%d\n",a->nslot,item->attr.min,item->attr.max);
		wtk_xbnf_expand_delete_a(c);
		wtk_xbnf_expand_delete_a(b);
	}
}



void wtk_xbnf_expand_process_list(wtk_xbnf_expand_t *e,wtk_xbnf_item_list_t *list,wtk_larray_t *a)
{
	wtk_queue_node_t *qn2;
	wtk_xbnf_item_t *item;
	wtk_larray_t *b,*c,*z;

	c=wtk_larray_new(32,sizeof(void*));
	for(qn2=list->list_q.pop;qn2;qn2=qn2->next)
	{
		item=data_offset2(qn2,wtk_xbnf_item_t,q_n);
		b=wtk_larray_new(32,sizeof(void*));
		wtk_xbnf_expand_process_item(e,item,b);
		//wtk_debug("nslot=%d\n",b->nslot);
		z=wtk_xbnf_array_xyz(c,b);
		wtk_xbnf_expand_delete_a(b);
		wtk_xbnf_expand_delete_a(c);
		c=z;
		//exit(0);
	}
	wtk_larray_cpy(c,a);
	wtk_larray_delete(c);
}

void wtk_xbnf_expand_process_set(wtk_xbnf_expand_t *e,wtk_xbnf_item_set_t *set,wtk_larray_t *a)
{
	wtk_queue_node_t *qn;
	wtk_xbnf_item_list_t *list;
	wtk_larray_t *b;

	b=wtk_larray_new(32,sizeof(void*));
	for(qn=set->set_q.pop;qn;qn=qn->next)
	{
		list=data_offset2(qn,wtk_xbnf_item_list_t,q_n);
		wtk_larray_reset(b);
		wtk_xbnf_expand_process_list(e,list,b);
		wtk_larray_merge(a,b);
	}
	wtk_larray_delete(b);
}



void wtk_xbnf_expand_process(wtk_xbnf_expand_t *e)
{
	wtk_strbuf_t **bufs;
	wtk_larray_t *a;
	int i;

	a=wtk_larray_new(1024,sizeof(void*));
	wtk_xbnf_expand_process_set(e,e->xbnf->main_expr->set,a);
	bufs=(wtk_strbuf_t**)a->slot;
	for(i=0;i<a->nslot;++i)
	{
		//wtk_debug("v[%d]=[%.*s]\n",i,bufs[i]->pos,bufs[i]->data);
		printf("%.*s\n",bufs[i]->pos,bufs[i]->data);
	}
	wtk_xbnf_expand_delete_a(a);

}


void wtk_xbnf_expand_process2(wtk_xbnf_t *x)
{
	wtk_xbnf_expand_t* e;

	e=wtk_xbnf_expand_new(x);
	wtk_xbnf_expand_process(e);
	wtk_xbnf_expand_delete(e);
}
