#include "wtk_maxseg.h" 
#include <ctype.h>

#define WTK_SYN_MAX 100000

void wtk_maxseg_init(wtk_maxseg_t *seg,wtk_heap_t *heap,wtk_strbuf_t *buf,void *wrd_ths,wtk_maxseg_is_wrd_f wrd_f)
{
	seg->heap=heap;
	seg->buf=buf;
	seg->wrd_ths=wrd_ths;
	seg->wrd_f=wrd_f;
}

void wtk_maxseg_clean(wtk_maxseg_t *seg)
{
}

wtk_maxseg_wrd_item_t* wtk_maxseg_wrd_item_new(wtk_heap_t *heap,char *data,int bytes)
{
	wtk_maxseg_wrd_item_t *item;

	item=(wtk_maxseg_wrd_item_t*)wtk_heap_malloc(heap,sizeof(wtk_maxseg_wrd_item_t));
	//wtk_debug("[%.*s]\n",bytes,data);
	if(bytes==1)
	{
		data[0]=toupper(data[0]);
	}
	wtk_string_set(&(item->v),data,bytes);
	wtk_queue2_init(&(item->path_q));
	//item->depth=10000.0;
	item->rb_n.key=WTK_SYN_MAX;
	item->min_input_pth=NULL;
	return item;
}

wtk_maxseg_wrd_path_t* wtk_maxseg_wrd_path_new(wtk_heap_t *heap,wtk_maxseg_wrd_item_t *from,wtk_maxseg_wrd_item_t *to,wtk_string_t *v)
{
	wtk_maxseg_wrd_path_t *pth;

	pth=(wtk_maxseg_wrd_path_t*)wtk_heap_malloc(heap,sizeof(wtk_maxseg_wrd_path_t));
	pth->from=from;
	pth->to=to;
	pth->v=v;
	return pth;
}

void wtk_maxseg_wrd_item_print_min_path(wtk_maxseg_wrd_item_t *item)
{
	//wtk_debug("v=%.*s\n",item->v.len,item->v.data);
	while(item && item->min_input_pth)
	{
		wtk_debug("[%.*s]\n",item->min_input_pth->v->len,item->min_input_pth->v->data);
		item=item->min_input_pth->from;
	}
}

wtk_queue_t wtk_maxseg_seg_get_mini_path(wtk_maxseg_t *d,wtk_maxseg_wrd_item_t **items,int n,wtk_rbtree_t *tree)
{
	wtk_rbnode_t *node;
	wtk_maxseg_wrd_item_t *item;
	wtk_queue_node_t *qn;
	wtk_maxseg_wrd_path_t *pth;
	wtk_queue_t q;

	while(tree->root)
	{
		node=(wtk_rbnode_t*)wtk_treenode_min((wtk_treenode_t*)(tree->root));
		wtk_rbtree_remove(tree,node);
		item=data_offset2(node,wtk_maxseg_wrd_item_t,rb_n);
		item->use=0;
		//wtk_debug("key=%f\n",item->rb_n.key);
		for(qn=item->path_q.pop;qn;qn=qn->next)
		{
			pth=data_offset2(qn,wtk_maxseg_wrd_path_t,q_n);
			//wtk_debug("[%.*s]=%f\n",pth->v->len,pth->v->data,pth->to->rb_n.key);
			if(pth->to && (pth->to->rb_n.key > (item->rb_n.key+1)))
			{
				if(pth->to->use)
				{
					wtk_rbtree_remove(tree,&(pth->to->rb_n));
				}else
				{
					pth->to->use=1;
				}
				pth->to->min_input_pth=pth;
				pth->to->rb_n.key=item->rb_n.key+1;
				wtk_rbtree_insert(tree,&(pth->to->rb_n));
			}
		}
	}
	//wtk_maxseg_wrd_item_print_min_path(items[n-1]);
	//exit(0);
	item=items[n-1];
	wtk_queue_init(&(q));
	while(item && item->min_input_pth)
	{
		//wtk_debug("[%.*s]\n",item->v.len,item->v.data);
		wtk_queue_push_front(&(q),&(item->min_input_pth->out_n));
		item=item->min_input_pth->from;
	}
	return q;
}


wtk_queue_t wtk_maxseg_seg(wtk_maxseg_t *seg,char *data,int len)
{
	wtk_heap_t *heap=seg->heap;
	wtk_strbuf_t *buf=seg->buf;
	char *s,*e;
	int cnt;
	wtk_string_t *v;
	wtk_maxseg_wrd_item_t **strs;
	wtk_array_t *a;
	wtk_maxseg_wrd_item_t *item,*src;
	wtk_maxseg_wrd_path_t *pth;
	wtk_rbtree_t tree;
	int i,j;

	//wtk_debug("[%.*s]\n",len,data);
	wtk_rbtree_init(&(tree));
	s=data;e=s+len;
	a=wtk_array_new_h(heap,len/2,sizeof(wtk_string_t*));
	while(s<e)
	{
		cnt=wtk_utf8_bytes(*s);
		//wtk_debug("[%.*s]\n", cnt, s);
		item=wtk_maxseg_wrd_item_new(heap,s,cnt);
		if(tree.len==0)
		{
			item->rb_n.key=0;
		}
		item->use=1;
		wtk_rbtree_insert(&(tree),&(item->rb_n));
		wtk_array_push2(a,&(item));
		s+=cnt;
	}
	item=wtk_maxseg_wrd_item_new(heap,0,0);
	item->use=1;
	wtk_rbtree_insert(&(tree),&(item->rb_n));
	wtk_array_push2(a,&(item));
	//wtk_debug("[%d/%d]\n",mat->row,mat->col);
	strs=(wtk_maxseg_wrd_item_t**)a->slot;
	//wtk_debug("strs=%p\n",strs);
	for(i=0;i<a->nslot;++i)
	{
		src=strs[i];
		if(i==a->nslot-1)
		{
			pth=wtk_maxseg_wrd_path_new(heap,src,NULL,&(src->v));
			wtk_queue2_push(&(src->path_q),&(pth->q_n));
			break;
		}
		wtk_strbuf_reset(buf);
		pth=wtk_maxseg_wrd_path_new(heap,src,strs[i+1],&(src->v));
		strs[i+1]->min_input_pth=pth;
		wtk_queue2_push(&(src->path_q),&(pth->q_n));
		wtk_strbuf_push(buf,src->v.data,src->v.len);
		//*wtk_matc_at(mat,i,i)=1;
		for(j=i+1;j<a->nslot;++j)
		{
			item=strs[j];
			if(item->v.len==0){continue;}
			wtk_strbuf_push(buf,item->v.data,item->v.len);
			//wtk_debug("[%.*s]\n",buf->pos,buf->data);
			if(seg->wrd_f(seg->wrd_ths,buf->data,buf->pos))
			{
				//wtk_debug("[%.*s]\n",buf->pos,buf->data);
				v=wtk_heap_dup_string(heap,buf->data,buf->pos);
				if(j!=a->nslot-1)
				{
					pth=wtk_maxseg_wrd_path_new(heap,src,strs[j+1],v);
				}else
				{
					pth=wtk_maxseg_wrd_path_new(heap,src,NULL,v);
				}
				//wtk_debug("[%.*s]\n",wrd->name->len,wrd->name->data);
				wtk_queue2_push(&(src->path_q),&(pth->q_n));
			}
		}
	}
	return wtk_maxseg_seg_get_mini_path(seg,strs,a->nslot,&tree);
}

void wtk_maxseg_print_queue(wtk_queue_t *q)
{
	wtk_queue_node_t *qn;
	wtk_maxseg_wrd_path_t *pth;

	for(qn=q->pop;qn;qn=qn->next)
	{
		pth=data_offset2(qn,wtk_maxseg_wrd_path_t,out_n);
		wtk_debug("[%.*s]\n",pth->v->len,pth->v->data);
	}
}

void wtk_maxseg_print_queue_string(wtk_queue_t *q)
{
	wtk_queue_node_t *qn;
	wtk_maxseg_wrd_path_t *pth;

	for(qn=q->pop;qn;qn=qn->next)
	{
		pth=data_offset2(qn,wtk_maxseg_wrd_path_t,out_n);
		printf("%.*s",pth->v->len,pth->v->data);
		if (qn->next !=NULL){
			printf(" ");
		}
	}
	printf("\n");
}
