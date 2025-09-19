#include <ctype.h>
#include "wtk_cosynthesis_dtree.h"
#include "wtk/core/cfg/wtk_source.h"
int wtk_cosynthesis_dtree_load_dur_tree(wtk_cosynthesis_dtree_t *m,wtk_cosynthesis_qs_t *qs,wtk_cosynthesis_tree_item_t *tree,wtk_source_t *src,wtk_strbuf_t *buf);

wtk_cosynthesis_qs_t* wtk_cosynthesis_dtree_new_qs(wtk_cosynthesis_dtree_t *m,int hash_hint)
{
	wtk_heap_t *heap=m->heap;
	wtk_cosynthesis_qs_t *q;

	q=(wtk_cosynthesis_qs_t*)wtk_heap_malloc(heap,sizeof(wtk_cosynthesis_qs_t));
	//wtk_queue_init(&(q->item_q));
	if(hash_hint>0)
	{
		q->hash=wtk_str_hash_new(hash_hint);
	}else
	{
		q->hash=NULL;
	}
	return q;
}

wtk_cosynthesis_qs_item_t* wtk_cosynthesis_dtree_new_qs_item(wtk_cosynthesis_dtree_t *m,char *name,int name_bytes)
{
	wtk_heap_t *heap=m->heap;
	wtk_cosynthesis_qs_item_t *item;

	item=(wtk_cosynthesis_qs_item_t*)wtk_heap_malloc(heap,sizeof(wtk_cosynthesis_qs_item_t));
	wtk_queue_init(&(item->pat_q));
	item->name=wtk_strpool_find(m->pool,name,name_bytes,1);
	return item;
}

void wtk_cosynthesis_pat_update(wtk_cosynthesis_pat_t *p)
{
	char *s,*e;
	int nstar,nques,max;

	nstar=nques=max=0;
	s=p->pat->data;
	e=s+p->pat->len;
	while(s<e)
	{
		switch(*s)
		{
		case '*':
			++nstar;
			break;
		case '?':
			++nques;
			++max;
			break;
		default:
			++max;
			break;
		}
		++s;
	}
	if(nstar==2 && nques==0 && p->pat->data[0]=='*' && p->pat->data[p->pat->len-1]=='*')
	{
		p->use_str=1;
	}else
	{
		p->use_str=0;
	}
	//if(p->nstar==2 && p->nques==0 && p->pat->data[0]=='*' && p->pat->data[p->pat->len-1]=='*')

}

wtk_cosynthesis_pat_t* wtk_cosynthesis_dtree_new_pat(wtk_cosynthesis_dtree_t *m,char *p,int p_bytes)
{
	wtk_heap_t *heap=m->heap;
	wtk_cosynthesis_pat_t *pat;

	pat=(wtk_cosynthesis_pat_t*)wtk_heap_malloc(heap,sizeof(wtk_cosynthesis_pat_t));
	if(m->cfg->use_bin)
	{
		pat->pat=wtk_heap_dup_string(heap,p,p_bytes);
	}else
	{
		pat->pat=wtk_strpool_find(m->pool,p,p_bytes,1);
	}
//	pat->nques=0;
//	pat->nstar=0;
//	pat->max=0;
	wtk_cosynthesis_pat_update(pat);
	return pat;
}

wtk_cosynthesis_tree_item_t* wtk_cosynthesis_dtree_new_tree_item(wtk_cosynthesis_dtree_t *m,int node_cnt)
{
	wtk_cosynthesis_tree_item_t *t;

	t=(wtk_cosynthesis_tree_item_t*)wtk_heap_malloc(m->heap,sizeof(wtk_cosynthesis_tree_item_t));
	t->state=-1;
	wtk_queue_init(&(t->pat_q));
	t->root=NULL;
	t->leaf=NULL;
	t->node_cnt=node_cnt;
	t->nodes=wtk_heap_zalloc(m->heap,sizeof(wtk_cosynthesis_node_t)*node_cnt);
	return t;
}

wtk_cosynthesis_tree_t* wtk_cosynthesis_dtree_new_tree(wtk_cosynthesis_dtree_t *m)
{
	wtk_cosynthesis_tree_t *t;

	t=(wtk_cosynthesis_tree_t*)wtk_heap_malloc(m->heap,sizeof(wtk_cosynthesis_tree_t));
	wtk_queue_init(&(t->item_q));
	return t;
}

void wtk_cosynthesis_node_init(wtk_cosynthesis_node_t *node)
{
	//node->idx=0;
	node->pdf=0;
	node->quest=NULL;
	node->yes=NULL;
	node->no=NULL;
	///node->next=NULL;
}

wtk_cosynthesis_node_t* wtk_cosynthesis_dtree_new_node(wtk_cosynthesis_dtree_t *m)
{
	wtk_cosynthesis_node_t *node;

	node=(wtk_cosynthesis_node_t*)wtk_heap_malloc(m->heap,sizeof(wtk_cosynthesis_node_t));
	wtk_cosynthesis_node_init(node);
	return node;
}


int wtk_cosynthesis_dtree_load_dur_qs(wtk_cosynthesis_dtree_t *m,wtk_cosynthesis_qs_t *qs,wtk_source_t *src,wtk_strbuf_t *buf)
{
typedef enum
{
	WTK_cosynthesis_DTREE_QSTATE_INIT,
	WTK_cosynthesis_DTREE_QSTATE_WRD,
	WTK_cosynthesis_DTREE_QSTATE_WAIT,
}wtk_cosynthesis_dtree_qstate_t;
	wtk_cosynthesis_qs_item_t *item;
	wtk_cosynthesis_pat_t *pat;
	wtk_cosynthesis_dtree_qstate_t state;
	int ret;
	int run=1;

	ret=wtk_source_read_string(src,buf);
	if(ret!=0){goto end;}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	item=wtk_cosynthesis_dtree_new_qs_item(m,buf->data,buf->pos);
	state=WTK_cosynthesis_DTREE_QSTATE_INIT;
	while(run)
	{
		ret=wtk_source_read_string3(src,buf);
		if(ret!=0){goto end;}
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
		switch(state)
		{
		case WTK_cosynthesis_DTREE_QSTATE_INIT:
			if(wtk_str_equal_s(buf->data,buf->pos,"{"))
			{
				state=WTK_cosynthesis_DTREE_QSTATE_WRD;
			}
			break;
		case WTK_cosynthesis_DTREE_QSTATE_WRD:
			//wtk_debug("[%.*s]\n",buf->pos,buf->data);
			pat=wtk_cosynthesis_dtree_new_pat(m,buf->data,buf->pos);
			wtk_queue_push(&(item->pat_q),&(pat->q_n));
			state=WTK_cosynthesis_DTREE_QSTATE_WAIT;
			break;
		case WTK_cosynthesis_DTREE_QSTATE_WAIT:
			if(wtk_str_equal_s(buf->data,buf->pos,","))
			{
				state=WTK_cosynthesis_DTREE_QSTATE_WRD;
			}else if(wtk_str_equal_s(buf->data,buf->pos,"}"))
			{
				run=0;
				break;
			}
			break;
		}
	}
	wtk_str_hash_add(qs->hash,item->name->data,item->name->len,item);
	//wtk_queue_push(&(qs->item_q),&(item->q_n));
	ret=0;
end:
	return ret;
}

void wtk_cosynthesis_dtree_dur_parse_tree_pat(wtk_cosynthesis_dtree_t *m,wtk_cosynthesis_tree_item_t *tree,char *p,int p_bytes)
{
typedef enum
{
	WTK_cosynthesis_DTREE_DUR_TREE_INIT,
	WTK_cosynthesis_DTREE_DUR_TREE_WAIT_CAP,
	WTK_cosynthesis_DTREE_DUR_TREE_WRD,
}wtk_cosynthesis_dtree_dur_tree_state_t;
	wtk_cosynthesis_dtree_dur_tree_state_t state;
	wtk_cosynthesis_pat_t *pat;
	char *s,*e,c;
	wtk_string_t v={0,0};
	int has_cap=0;
	int run=1;

	//wtk_debug("[%.*s]\n",p_bytes,p);
	state=WTK_cosynthesis_DTREE_DUR_TREE_INIT;
	s=p;e=s+p_bytes;
	while(s<e && run)
	{
		c=*s;
		switch(state)
		{
		case WTK_cosynthesis_DTREE_DUR_TREE_INIT:
			if(c=='{')
			{
				state=WTK_cosynthesis_DTREE_DUR_TREE_WAIT_CAP;
			}
			break;
		case WTK_cosynthesis_DTREE_DUR_TREE_WAIT_CAP:
			if(c=='}' || (has_cap&& c==')'))
			{
				run=0;
			}else if(c=='(')
			{
				has_cap=1;
			}else if(!isspace(c))
			{
				v.data=s;
				state=WTK_cosynthesis_DTREE_DUR_TREE_WRD;
			}
			break;
		case WTK_cosynthesis_DTREE_DUR_TREE_WRD:
			if(c==',' || c=='}' || (has_cap&& c==')'))
			{
				v.len=s-v.data;
				//wtk_debug("[%.*s]\n",v.len,v.data);
				pat=wtk_cosynthesis_dtree_new_pat(m,v.data,v.len);
				wtk_queue_push(&(tree->pat_q),&(pat->q_n));
				state=WTK_cosynthesis_DTREE_DUR_TREE_WAIT_CAP;
			}
			break;
		}
		++s;
	}
	//exit(0);
}

//wtk_cosynthesis_node_t* wtk_cosynthesis_node_find_idx(wtk_cosynthesis_node_t *node,int idx)
//{
//	//wtk_debug("idx=%d\n",idx);
//	while(node)
//	{
//		if(node->idx==idx)
//		{
//			return node;
//		}else
//		{
//			node=node->next;
//		}
//	}
//	return NULL;
//}

wtk_cosynthesis_qs_item_t* wtk_cosynthesis_qs_get(wtk_cosynthesis_qs_t *qs,char *name,int name_bytes)
{
	if(!qs || !qs->hash)
	{
		return NULL;
	}
	return (wtk_cosynthesis_qs_item_t*)wtk_str_hash_find(qs->hash,name,name_bytes);
//	wtk_queue_node_t *qn;
//	wtk_cosynthesis_qs_item_t *item;
//
//	for(qn=qs->item_q.pop;qn;qn=qn->next)
//	{
//		item=data_offset2(qn,wtk_cosynthesis_qs_item_t,q_n);
//		if(wtk_string_cmp(item->name,name,name_bytes)==0)
//		{
//			return item;
//		}
//	}
//	return NULL;
}

int wtk_cosynthesis_nm2num(char *data,int bytes)
{
	char *s;

	//wtk_debug("[%.*s]\n",bytes,data);
	s=wtk_str_rchr(data,bytes,'_');
	if(s)
	{
		++s;
		bytes=data+bytes-s;
		return wtk_str_atoi(s,bytes);
	}
	return 0;
}




int wtk_cosynthesis_dtree_load_dur1(wtk_cosynthesis_dtree_t *m,int idx,wtk_source_t *src)
{
	wtk_cosynthesis_qs_t *qs;
	wtk_cosynthesis_tree_item_t *item;
	wtk_cosynthesis_tree_t *tree;
	wtk_strbuf_t *buf;
	char *l,*r;
	int ret;
	int node_cnt=0;
	int qs_cnt=0;

	// wtk_debug("m=%p idx=%d\n",m,idx);
	switch(idx)
	{
	case WTK_SYN_DTREE_TREE_DUR:
		node_cnt=m->cfg->dur_node_cnt;
		qs_cnt=m->cfg->dur_hash_hint;
		break;
	case WTK_SYN_DTREE_TREE_LF0:
		node_cnt=m->cfg->lf0_ndoe_cnt;
		qs_cnt=m->cfg->lf0_hash_hint;
		break;
	case WTK_SYN_DTREE_TREE_MCP:
		node_cnt=m->cfg->mcp_node_cnt;
		qs_cnt=m->cfg->mcp_hash_hint;
		break;
	case WTK_SYN_DTREE_TREE_BAP:
		node_cnt=m->cfg->bap_node_cnt;
		qs_cnt=m->cfg->bap_hash_hint;
		break;
	case WTK_SYN_DTREE_TREE_LF0GV:
		node_cnt=m->cfg->lf0_gv_node_cnt;
		qs_cnt=m->cfg->lf0_gv_hash_hint;
		break;
	case WTK_SYN_DTREE_TREE_MCPGV:
		node_cnt=m->cfg->mcp_gv_node_cnt;
		qs_cnt=m->cfg->mcp_gv_hash_hint;
		break;
	case WTK_SYN_DTREE_TREE_BAPGV:
		node_cnt=m->cfg->bap_node_cnt;
		qs_cnt=m->cfg->bap_gv_hash_hint;
		break;
	}
	qs=wtk_cosynthesis_dtree_new_qs(m,qs_cnt);
	//wtk_debug("id=%d\n",idx);
	tree=wtk_cosynthesis_dtree_new_tree(m);
	m->tree[idx]=tree;
	m->qs[idx]=qs;
	//wtk_debug("idx[%d]=%p %d\n",idx,qs,qs_cnt);
	buf=wtk_strbuf_new(256,1);
	while(1)
	{
		ret=wtk_source_read_string(src,buf);
		if(ret!=0){ret=0;goto end;}
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
		if(buf->pos==2 && buf->data[0]=='Q' && buf->data[1]=='S')//wtk_str_equal_s(buf->data,buf->pos,"QS"))
		{
			ret=wtk_cosynthesis_dtree_load_dur_qs(m,qs,src,buf);
			if(ret!=0){goto end;}
		}else
		{
			//wtk_debug("[%.*s]\n",buf->pos,buf->data);
			if((l=wtk_str_chr(buf->data,buf->pos,'[')) &&
			   (r=wtk_str_chr(buf->data,buf->pos,']')))
			{
				//wtk_debug("[%.*s]=%d\n",(int)(r-l-1),l+1,node_cnt);
				//exit(0);
				//wtk_debug("cnt=%d\n",node_cnt);
				item=wtk_cosynthesis_dtree_new_tree_item(m,node_cnt);
				item->state=wtk_str_atoi(l+1,r-l-1);
				wtk_cosynthesis_dtree_dur_parse_tree_pat(m,item,buf->data,buf->pos);
				ret=wtk_cosynthesis_dtree_load_dur_tree(m,qs,item,src,buf);
				if(ret!=0){goto end;}
				wtk_queue_push(&(tree->item_q),&(item->q_n));
			}
		}
	}
	ret=0;
end:
	wtk_strbuf_delete(buf);
	return ret;
}


int wtk_cosynthesis_dtree_load_dur_tree(wtk_cosynthesis_dtree_t *m,wtk_cosynthesis_qs_t *qs,wtk_cosynthesis_tree_item_t *tree,wtk_source_t *src,wtk_strbuf_t *buf)
{
	wtk_cosynthesis_node_t *node,*leaf = NULL;
	int ret;
	int idx;

	ret=wtk_source_read_string(src,buf);
	if(ret!=0){goto end;}
	//wtk_debug("[%.*s]=%d\n",buf->pos,buf->data,tree->node_cnt);
	//node=wtk_cosynthesis_dtree_new_node(m);
	node=tree->nodes+0;
	wtk_cosynthesis_node_init(node);
	tree->root=node;
	leaf=node;
	if(wtk_str_equal_s(buf->data,buf->pos,"{"))
	{
		while(1)
		{
			ret=wtk_source_read_string(src,buf);
			if(ret!=0){goto end;}
			//wtk_debug("[%.*s]\n",buf->pos,buf->data);
			if(buf->pos==1 && buf->data[0]=='}')//wtk_str_equal_s(buf->data,buf->pos,"}"))
			{
				break;
			}
			idx=wtk_str_atoi(buf->data,buf->pos);
			node=tree->nodes-idx;
			/*
			node=wtk_cosynthesis_node_find_idx(leaf,idx);
			if(!node)
			{
				wtk_debug("node not found %p idx=%d\n",node,idx);
				ret=-1;goto end;
			}*/
			ret=wtk_source_read_string(src,buf);
			if(ret!=0){goto end;}
			//wtk_debug("[%.*s]\n",buf->pos,buf->data);
			node->quest=wtk_cosynthesis_qs_get(qs,buf->data,buf->pos);
			//wtk_debug("%p\n",node->quest);
			//node->quest=wtk_cosynthesis_dtree_get_quest(m,buf->data,buf->pos);
			if(!node->quest){goto end;}
			ret=wtk_source_read_string(src,buf);
			if(ret!=0){goto end;}
			//wtk_debug("[%.*s]\n",buf->pos,buf->data);
			if(isdigit(buf->data[0]) || buf->data[0]=='-')
			{
				//node->no->idx
				idx=wtk_str_atoi(buf->data,buf->pos);
				if(idx<=-tree->node_cnt)
				{
					wtk_debug("fond bug %d/%d\n",idx,tree->node_cnt);
					return -1;
				}
				node->no=tree->nodes-idx;
				wtk_cosynthesis_node_init(node->no);
				//wtk_debug("idx=%d\n",node->no->idx);
			}else
			{
				node->no=wtk_cosynthesis_dtree_new_node(m);
				node->no->pdf=wtk_cosynthesis_nm2num(buf->data,buf->pos);
			}
			//wtk_debug("idx=%d pdf=%d\n",node->no->idx,node->no->pdf);
			//node->no->next=leaf;
			//leaf=node->no;

			ret=wtk_source_read_string(src,buf);
			if(ret!=0){goto end;}
			//wtk_debug("[%.*s]\n",buf->pos,buf->data);
			if(isdigit(buf->data[0]) || buf->data[0]=='-')
			{
				//node->yes->idx
				idx=wtk_str_atoi(buf->data,buf->pos);
				if(idx<=-tree->node_cnt)
				{
					wtk_debug("%d/%d\n",idx,tree->node_cnt);
					exit(0);
					return -1;
				}
				node->yes=tree->nodes-idx;
				wtk_cosynthesis_node_init(node->yes);
				//wtk_debug("idx=%d\n",node->yes->idx);
			}else
			{
				node->yes=wtk_cosynthesis_dtree_new_node(m);
				node->yes->pdf=wtk_cosynthesis_nm2num(buf->data,buf->pos);
			}
			//node->yes->next=leaf;
			leaf=node->yes;
			//exit(0);
		}
	}else
	{
		node->pdf=wtk_cosynthesis_nm2num(buf->data,buf->pos);
	}
	ret=0;
end:
	if(ret==0)
	{
		tree->leaf=leaf;
	}
	return ret;
}



int wtk_cosynthesis_dtree_load_dur3(wtk_cosynthesis_dtree_t *m,int idx,wtk_source_t *src)
{
	char tmp[8];
	wtk_cosynthesis_qs_t *qs;
	wtk_cosynthesis_qs_item_t *qitem;
	wtk_cosynthesis_pat_t *pat;
	wtk_cosynthesis_tree_item_t *item;
	wtk_cosynthesis_tree_t *tree;
	wtk_strbuf_t *buf;
	int ret;
	unsigned int vi;
	unsigned char vj,vs,vs2;
	unsigned short s1;
	int i,j;
	wtk_cosynthesis_node_t *node,*leaf;

	src->swap=0;
	buf=wtk_strbuf_new(256,1);
	//read FILE magic;
	ret=wtk_source_fill(src,tmp,8);
	if(ret!=0){goto end;}
	//wtk_debug("[%.*s]\n",4,tmp);
	//read qs; cnt | k,array k=>bytes,string array=[(bytes,string),(bytes,string)]
//	ret=wtk_source_fill(src,(char*)(&vi),4);
//	if(ret!=0){goto end;}
	vi=((unsigned int*)tmp)[1];
	//wtk_debug("vi=%d\n",vi);
	qs=wtk_cosynthesis_dtree_new_qs(m,vi);
	tree=wtk_cosynthesis_dtree_new_tree(m);
	m->tree[idx]=tree;
	m->qs[idx]=qs;
	for(i=0;i<vi;++i)
	{
		ret=wtk_source_read_wtkstr(src,buf);
		if(ret!=0){goto end;}
		qitem=wtk_cosynthesis_dtree_new_qs_item(m,buf->data,buf->pos);
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
		ret=wtk_source_fill(src,(char*)(&vj),1);
		if(ret!=0){goto end;}
		//wtk_debug("vi=%d\n",vj);
		for(j=0;j<vj;++j)
		{
			ret=wtk_source_read_wtkstr(src,buf);
			if(ret!=0){goto end;}
			pat=wtk_cosynthesis_dtree_new_pat(m,buf->data,buf->pos);
			//wtk_debug("[%.*s]\n",buf->pos,buf->data);
			wtk_queue_push(&(qitem->pat_q),&(pat->q_n));
		}
		wtk_str_hash_add(qs->hash,qitem->name->data,qitem->name->len,qitem);
		//exit(0);
	}
	ret=wtk_source_fill(src,(char*)(&vj),1);
	if(ret!=0){goto end;}
	for(j=0;j<vj;++j)
	{
		ret=wtk_source_read_wtkstr(src,buf);
		if(ret!=0){goto end;}
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
		ret=wtk_source_fill(src,(char*)(&vs),1);
		if(ret!=0){goto end;}
		ret=wtk_source_fill(src,(char*)(&vs2),1);
		if(ret!=0){goto end;}
		if(vs2)
		{
			ret=wtk_source_fill(src,(char*)(&vi),4);
			if(ret!=0){goto end;}
		}else
		{
			vi=1;
		}
		//wtk_debug("vs=%d/%d/%d\n",vs,vs2,vi);
		item=wtk_cosynthesis_dtree_new_tree_item(m,vi);
		wtk_queue_push(&(tree->item_q),&(item->q_n));
		item->state=vs;
		wtk_cosynthesis_dtree_dur_parse_tree_pat(m,item,buf->data,buf->pos);
		item->root=item->nodes;
		wtk_cosynthesis_node_init(item->root);
		leaf=item->root;
		if(vs2)
		{
			for(i=0;i<vi;++i)
			{
				ret=wtk_source_fill(src,(char*)(&s1),2);
				if(ret!=0){goto end;}
				//wtk_debug("s1=%d\n",s1);
				node=item->nodes+s1;
				ret=wtk_source_read_wtkstr(src,buf);
				if(ret!=0){goto end;}
				//wtk_debug("[%.*s]\n",buf->pos,buf->data);
				node->quest=wtk_cosynthesis_qs_get(qs,buf->data,buf->pos);
				//wtk_debug("quest=%p\n",node->quest);
				if(!node->quest){goto end;}
				ret=wtk_source_fill(src,(char*)(&vs),1);
				if(ret!=0){goto end;}
				ret=wtk_source_fill(src,(char*)(&s1),2);
				if(ret!=0){goto end;}
				if(vs==0)
				{
					if(s1>=vi)
					{
						wtk_debug("fond bug %d/%d\n",s1,vi);
						return -1;
					}
					node->no=item->nodes+s1;
					wtk_cosynthesis_node_init(node->no);
				}else
				{
					node->no=wtk_cosynthesis_dtree_new_node(m);
					node->no->pdf=s1;
				}
				ret=wtk_source_fill(src,(char*)(&vs),1);
				if(ret!=0){goto end;}
				ret=wtk_source_fill(src,(char*)(&s1),2);
				if(ret!=0){goto end;}
				if(vs==0)
				{
					if(s1>=vi)
					{
						wtk_debug("fond bug %d/%d\n",s1,vi);
						return -1;
					}
					node->yes=item->nodes+s1;
					wtk_cosynthesis_node_init(node->yes);
				}else
				{
					node->yes=wtk_cosynthesis_dtree_new_node(m);
					node->yes->pdf=s1;
				}
				leaf=node->yes;
			}
			item->leaf=leaf;
		}else
		{
			ret=wtk_source_fill(src,(char*)(&vs),1);
			if(ret!=0){goto end;}
			item->root->pdf=vs;
			item->leaf=leaf;
		}
		//exit(0);
	}
	ret=0;
end:
	//exit(0);
	wtk_strbuf_delete(buf);
	return ret;
}


int wtk_cosynthesis_dtree_load_dur2(wtk_cosynthesis_dtree_t *m,int idx,wtk_source_t *src)
{
	char tmp[16];
	wtk_cosynthesis_qs_t *qs;
	wtk_cosynthesis_qs_item_t *qitem;
	wtk_cosynthesis_pat_t *pat;
	wtk_cosynthesis_tree_item_t *item;
	wtk_cosynthesis_tree_t *tree;
	wtk_strbuf_t *buf;
	int ret;
	unsigned int vi;
	unsigned char vj,vs,vs2;
	unsigned short s1;
	int i,j;
	wtk_cosynthesis_node_t *node,*leaf;

	src->swap=0;
	buf=wtk_strbuf_new(256,1);
	//read FILE magic;
	ret=wtk_source_fill(src,tmp,8);
	if(ret!=0){goto end;}
	// wtk_debug("[%.*s]\n",4,tmp);
	//read qs; cnt | k,array k=>bytes,string array=[(bytes,string),(bytes,string)]
//	ret=wtk_source_fill(src,(char*)(&vi),4);
//	if(ret!=0){goto end;}
	vi=((unsigned int*)tmp)[1];
	// wtk_debug("QS vi=%d\n",vi);
	qs=wtk_cosynthesis_dtree_new_qs(m,vi*3/2);
	tree=wtk_cosynthesis_dtree_new_tree(m);
	m->tree[idx]=tree;
	m->qs[idx]=qs;
	for(i=0;i<vi;++i)
	{
		ret=wtk_source_read_wtkstr(src,buf);
		if(ret!=0){goto end;}
		qitem=wtk_cosynthesis_dtree_new_qs_item(m,buf->data,buf->pos);
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
		ret=wtk_source_fill(src,(char*)(&vj),1);
		if(ret!=0){goto end;}
		//wtk_debug("vi=%d\n",vj);
		for(j=0;j<vj;++j)
		{
			ret=wtk_source_read_wtkstr(src,buf);
			if(ret!=0){goto end;}
			pat=wtk_cosynthesis_dtree_new_pat(m,buf->data,buf->pos);
			// wtk_debug("[%.*s]\n",buf->pos,buf->data);
			wtk_queue_push(&(qitem->pat_q),&(pat->q_n));
		}
		wtk_str_hash_add(qs->hash,qitem->name->data,qitem->name->len,qitem);
		//exit(0);
	}
	ret=wtk_source_fill(src,(char*)(&vj),1);
	if(ret!=0){goto end;}
	for(j=0;j<vj;++j)
	{
		ret=wtk_source_fill(src,tmp,3);
		if(ret!=0){goto end;}
		ret=wtk_source_read_wtkstr2(src,buf,tmp[2]);
		if(ret!=0){goto end;}
		vs=tmp[0];
		vs2=tmp[1];
		if(vs2)
		{
			ret=wtk_source_fill(src,(char*)(&vi),4);
			if(ret!=0){goto end;}
		}else
		{
			vi=1;
		}
		//wtk_debug("vs=%d/%d/%d\n",vs,vs2,vi);
		item=wtk_cosynthesis_dtree_new_tree_item(m,vi);
		wtk_queue_push(&(tree->item_q),&(item->q_n));
		item->state=vs;
		// wtk_debug("dur stat %d\n",vs);
		wtk_cosynthesis_dtree_dur_parse_tree_pat(m,item,buf->data,buf->pos);
		item->root=item->nodes;
		wtk_cosynthesis_node_init(item->root);
		leaf=item->root;
		if(vs2)
		{
			for(i=0;i<vi;++i)
			{
				ret=wtk_source_fill(src,tmp,9);
				if(ret!=0){goto end;}
//				ret=wtk_source_fill(src,(char*)(&s1),2);
//				if(ret!=0){goto end;}
				//print_hex(tmp,9);
				s1=*((unsigned short*)(tmp+6));
				//wtk_debug("s1=%d /%d\n",s1,tmp[8]);
				node=item->nodes+s1;
				ret=wtk_source_read_wtkstr2(src,buf,tmp[8]);
				if(ret!=0){goto end;}
				//wtk_debug("[%.*s]\n",buf->pos,buf->data);
				node->quest=wtk_cosynthesis_qs_get(qs,buf->data,buf->pos);
				//wtk_debug("quest=%p\n",node->quest);
				if(!node->quest){goto end;}

//				ret=wtk_source_fill(src,tmp,6);
//				if(ret!=0){goto end;}
				vs=tmp[0];
				s1=*(unsigned short*)(tmp+1);
				if(vs==0)
				{
					if(s1>=vi)
					{
						wtk_debug("fond bug %d/%d\n",s1,vi);
						return -1;
					}
					node->no=item->nodes+s1;
					wtk_cosynthesis_node_init(node->no);
				}else
				{
					node->no=wtk_cosynthesis_dtree_new_node(m);
					node->no->pdf=s1;
				}
				vs=tmp[3];
				s1=*(unsigned short*)(tmp+4);
				if(vs==0)
				{
					if(s1>=vi)
					{
						wtk_debug("fond bug %d/%d\n",s1,vi);
						return -1;
					}
					node->yes=item->nodes+s1;
					wtk_cosynthesis_node_init(node->yes);
				}else
				{
					node->yes=wtk_cosynthesis_dtree_new_node(m);
					node->yes->pdf=s1;
				}
				leaf=node->yes;
			}
			item->leaf=leaf;
		}else
		{
			ret=wtk_source_fill(src,(char*)(&vs),1);
			if(ret!=0){goto end;}
			item->root->pdf=vs;
			item->leaf=leaf;
		}
		//exit(0);
	}
	ret=0;
end:
	//exit(0);
	wtk_strbuf_delete(buf);
	return ret;
}


int wtk_cosynthesis_dtree_load_dur(void **p,wtk_source_t *src)
{
	wtk_cosynthesis_dtree_t *m;
	int idx;

	m=(wtk_cosynthesis_dtree_t*)(p[0]);
	idx=*((int*)p[1]);
	// wtk_debug("use_bin=%d\n",m->cfg->use_bin);
	if(m->cfg->use_bin)
	{
		return wtk_cosynthesis_dtree_load_dur2(m,idx,src);
	}else
	{
		return wtk_cosynthesis_dtree_load_dur1(m,idx,src);
	}
}

wtk_cosynthesis_dtree_t* wtk_cosynthesis_dtree_new(wtk_cosynthesis_dtree_cfg_t *cfg,wtk_source_loader_t *sl,wtk_strpool_t *pool)
{
	wtk_cosynthesis_dtree_t *m;
	int ret;
	int i;
	int idx;
	void *p[2];

	m=(wtk_cosynthesis_dtree_t*)wtk_malloc(sizeof(wtk_cosynthesis_dtree_t));
	m->cfg=cfg;
	m->heap=pool->hash->heap;//wtk_heap_new(4096);
	m->pool=pool;
	for(i=0;i<WTK_SYN_DTREE_TREE_BAPGV+1;++i)
	{
		m->tree[i]=NULL;
		m->qs[i]=NULL;
	}
	p[0]=m;
	p[1]=&idx;
	// wtk_debug("m=%p idx=%d\n",m,idx);
	idx=WTK_SYN_DTREE_TREE_DUR;
	// wtk_debug("%s\n",cfg->dur_fn);
	//wtk_debug("bytes=%f M\n",wtk_strpool_bytes(pool)*1.0/(1024*1024));
	ret=wtk_source_loader_load(sl,p,(wtk_source_load_handler_t)wtk_cosynthesis_dtree_load_dur,cfg->dur_fn);
	if(ret!=0){goto end;}
	//wtk_debug("bytes=%f M\n",wtk_strpool_bytes(pool)*1.0/(1024*1024));
	idx=WTK_SYN_DTREE_TREE_LF0;
	ret=wtk_source_loader_load(sl,p,(wtk_source_load_handler_t)wtk_cosynthesis_dtree_load_dur,cfg->lf0_fn);
	if(ret!=0){goto end;}
	//wtk_debug("bytes=%f M\n",wtk_strpool_bytes(pool)*1.0/(1024*1024));
	if(cfg->lf0_gv_fn)
	{
		idx=WTK_SYN_DTREE_TREE_LF0GV;
		ret=wtk_source_loader_load(sl,p,(wtk_source_load_handler_t)wtk_cosynthesis_dtree_load_dur,cfg->lf0_gv_fn);
		if(ret!=0){goto end;}
	//	wtk_debug("bytes=%f M\n",wtk_strpool_bytes(pool)*1.0/(1024*1024));
	}
	idx=WTK_SYN_DTREE_TREE_MCP;
	ret=wtk_source_loader_load(sl,p,(wtk_source_load_handler_t)wtk_cosynthesis_dtree_load_dur,cfg->mcp_fn);
	if(ret!=0){goto end;}
	//wtk_debug("bytes=%f M\n",wtk_strpool_bytes(pool)*1.0/(1024*1024));
	if(cfg->mcp_gv_fn)
	{
		idx=WTK_SYN_DTREE_TREE_MCPGV;
		ret=wtk_source_loader_load(sl,p,(wtk_source_load_handler_t)wtk_cosynthesis_dtree_load_dur,cfg->mcp_gv_fn);
		if(ret!=0){goto end;}
	//	wtk_debug("bytes=%f M\n",wtk_strpool_bytes(pool)*1.0/(1024*1024));
	}
	if(cfg->bap_fn)
	{
		idx=WTK_SYN_DTREE_TREE_BAP;
		ret=wtk_source_loader_load(sl,p,(wtk_source_load_handler_t)wtk_cosynthesis_dtree_load_dur,cfg->bap_fn);
		if(ret!=0){goto end;}
		if(cfg->bap_gv_fn)
		{
			idx=WTK_SYN_DTREE_TREE_BAPGV;
			ret=wtk_source_loader_load(sl,p,(wtk_source_load_handler_t)wtk_cosynthesis_dtree_load_dur,cfg->bap_gv_fn);
			if(ret!=0){goto end;}
		}
		//wtk_debug("bytes=%f M\n",wtk_strpool_bytes(pool)*1.0/(1024*1024));
	}
	if(cfg->conca_lf0_fn){
		idx=WTK_SYN_DTREE_TREE_CONCA_LF0;
		ret=wtk_source_loader_load(sl,p,(wtk_source_load_handler_t)wtk_cosynthesis_dtree_load_dur,cfg->conca_lf0_fn);
		if(ret!=0){goto end;}
	}
	if(cfg->conca_mcp_fn){
		idx=WTK_SYN_DTREE_TREE_CONCA_MGC;
		ret=wtk_source_loader_load(sl,p,(wtk_source_load_handler_t)wtk_cosynthesis_dtree_load_dur,cfg->conca_mcp_fn);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return m;
}

void wtk_cosynthesis_dtree_delete(wtk_cosynthesis_dtree_t *m)
{
	int i;

	for(i=0;i<WTK_SYN_DTREE_TREE_BAPGV+1;++i)
	{
		//wtk_debug("i[%d]=%p\n",i,m->qs[i]);
		if(m->qs[i] && m->qs[i]->hash)
		{
			wtk_str_hash_delete(m->qs[i]->hash);
		}
	}
	//wtk_heap_delete(m->heap);

	wtk_free(m);
}

int wtk_cosynthesis_dtree_count(wtk_cosynthesis_dtree_t *d,wtk_cosynthesis_dtree_tree_type_t type,int state)
{
	wtk_queue_node_t *qn;
	wtk_cosynthesis_tree_item_t *item;
	int num=0;

	for(qn=d->tree[type]->item_q.pop;qn;qn=qn->next)
	{
		item=data_offset2(qn,wtk_cosynthesis_tree_item_t,q_n);
		if(item->state==state)
		{
			++num;
		}
	}
	return num;
}

int wtk_cosynthesis_pat_dmatch(char *s,int s_bytes,char *ptn,int ptn_bytes)
{
//	static int ki=0;
//
//	++ki;
//	wtk_debug("v[%d]=[%.*s]\n",ki,ptn_bytes,ptn);
	if(s_bytes<=0 || ptn_bytes<=0)
	{
		if(s_bytes==0 && ptn_bytes==0)
		{
			return 1;
		}else
		{
			return 0;
		}
	}
	if(*ptn=='*')
	{
		//match current and keep ptn pos
		if(wtk_cosynthesis_pat_dmatch(s+1,s_bytes-1,ptn,ptn_bytes))
		{
			return 1;
		}else
		{
			//match current and move ptn pos
			return wtk_cosynthesis_pat_dmatch(s+1,s_bytes-1,ptn+1,ptn_bytes-1);
		}
	}
	if(*s==*ptn || *ptn=='?')
	{
		if(wtk_cosynthesis_pat_dmatch(s+1,s_bytes-1,ptn+1,ptn_bytes-1))
		{
			return 1;
		}else
		{
			if(ptn_bytes>1 && *(ptn+1)=='*')
			{
				return wtk_cosynthesis_pat_dmatch(s+1,s_bytes-1,ptn+2,ptn_bytes-2);
			}
		}
	}
	return 0;
}

void wtk_cosynthesis_pat_print(wtk_cosynthesis_pat_t *p)
{
	wtk_debug("pat=[%.*s]\n",p->pat->len,p->pat->data);
}

void wtk_cosynthesis_tree_item_print(wtk_cosynthesis_tree_item_t *item)
{
	wtk_queue_node_t *qn;
	wtk_cosynthesis_pat_t *pat;
	int i;

	wtk_debug("=========== item state=%d =============\n",item->state);
	for(i=0,qn=item->pat_q.pop;qn;qn=qn->next,++i)
	{
		pat=data_offset2(qn,wtk_cosynthesis_pat_t,q_n);
		wtk_debug("v[%d]=[%.*s]\n",i,pat->pat->len,pat->pat->data);
	}
}


int wtk_cosynthesis_pat_match(wtk_cosynthesis_pat_t *p,char *s,int s_bytes)
{
	int ret;

	//if(p->nstar==2 && p->nques==0 && p->pat->data[0]=='*' && p->pat->data[p->pat->len-1]=='*')
	if(p->use_str)
	{
		//wtk_debug("[%.*s]=[%.*s]\n",s_bytes,s,p->pat->len-2,p->pat->data+1);
		ret=wtk_str_str(s,s_bytes,p->pat->data+1,p->pat->len-2);
		//wtk_debug("==== ret=%d\n",ret);
//		if(ret>0)
//		{
//			printf("%.*s\n",s_bytes-ret,s+ret);
//			exit(0);
//		}
		return ret>=0?1:0;
	}else
	{
		//wtk_debug("%.*s\n",s_bytes,s);
		ret=wtk_cosynthesis_pat_dmatch(s,s_bytes,p->pat->data,p->pat->len);
		//wtk_debug("[%.*s]=%d\n",s_bytes,s,ret);
		return ret;
		//return wtk_cosynthesis_pat_str_match(s,s_bytes,);
	}
}


int wtk_cosynthesis_qs_item_match(wtk_cosynthesis_qs_item_t *item,char *s,int s_bytes)
{
	wtk_queue_node_t *qn;
	wtk_cosynthesis_pat_t *pat;
	int ret;

	for(qn=item->pat_q.pop;qn;qn=qn->next)
	{
		pat=data_offset2(qn,wtk_cosynthesis_pat_t,q_n);
		//wtk_debug("v[%d]: %.*s [%.*s]\n",++i,pat->pat->len,pat->pat->data,item->name->len,item->name->data);
		ret=wtk_cosynthesis_pat_match(pat,s,s_bytes);
		if(ret==1)
		{
			return ret;
		}
	}
	return 0;
}

int wtk_cosynthesis_node_search(wtk_cosynthesis_node_t *node,char *s,int s_bytes)
{
	//wtk_debug("[%.*s]\n",s_bytes,s);
	while(node)
	{
		//static int ki=0;
		//wtk_debug("v[%d]: idx=%d, [%.*s]\n",++ki,node->idx,node->quest->name->len,node->quest->name->data);
		//wtk_debug("v[%d]: idx=%d, [%.*s] yes=%d no=%d\n",++ki,node->pdf,node->quest->name->len,node->quest->name->data,node->yes->pdf,node->no->pdf);
		//wtk_debug("v[%d]: [%.*s]\n",++ki,node->quest->name->len,node->quest->name->data);
		//wtk_debug("v[%d]: idx=%d, [%.*s] yes=%d no=%d\n",++ki,node->pdf,node->quest->name->len,node->quest->name->data,node->yes->pdf,node->no->pdf);
		if(wtk_cosynthesis_qs_item_match(node->quest,s,s_bytes))
		{
			//wtk_debug("yes=%d\n",node->yes->pdf);
			if(node->yes->pdf>0)
			{
				return node->yes->pdf;
			}
			node=node->yes;
		}else
		{
			//wtk_debug("no=%d\n",node->no->pdf);
			if(node->no->pdf>0)
			{
				return node->no->pdf;
			}
			node=node->no;
		}
	}
	return -1;
}


int wtk_cosynthesis_tree_item_search(wtk_cosynthesis_tree_item_t *item,char *s,int s_bytes)
{
	//wtk_debug("root=%p leaf=%p\n",item->root,item->leaf);
	if(item->root==item->leaf)
	{
		return item->root->pdf;
	}
	return wtk_cosynthesis_node_search(item->root,s,s_bytes);
}

/**
 * return matched pdf index;
 */
int wtk_cosynthesis_tree_item_pat_search(wtk_cosynthesis_tree_item_t *item,char *s,int s_bytes)
{
	wtk_queue_node_t *qn;
	wtk_cosynthesis_pat_t *pat;
	int ret;
	int idx;
    //int i=0; //debug
	for(qn=item->pat_q.pop;qn;qn=qn->next)
	{
		pat=data_offset2(qn,wtk_cosynthesis_pat_t,q_n);
		// wtk_debug("v[%d]: %.*s state=%d\n",++i,pat->pat->len,pat->pat->data,item->state);
		ret=wtk_cosynthesis_pat_match(pat,s,s_bytes);
		if(ret==1)
		{
			// wtk_debug("state=%d\n",item->state);
			idx=wtk_cosynthesis_tree_item_search(item,s,s_bytes);
			return idx;
		}
	}
	return -1;
}

int wtk_cosynthesis_dtree_search(wtk_cosynthesis_dtree_t *d,wtk_cosynthesis_dtree_tree_type_t type,int state,char *s,int s_bytes,int *idx)
{
	wtk_queue_node_t *qn;
	wtk_cosynthesis_tree_item_t *item;
	int ret;

	idx[0]=0;
	idx[1]=-1;
	//wtk_debug("================= type=%d ======================\n",type);
	for(qn=d->tree[type]->item_q.pop;qn;qn=qn->next)
	{
		item=data_offset2(qn,wtk_cosynthesis_tree_item_t,q_n);
		if(item->state==state)
		{
			ret=wtk_cosynthesis_tree_item_pat_search(item,s,s_bytes);
			if(ret>0)
			{
				idx[1]=ret;
				//wtk_debug("idx[0]=%d idx[1]=%d\n", idx[0], idx[1]);
				return 0;
			}
			++idx[0];
		}
	}

	return -1;
}

wtk_cosynthesis_qs_item_t* wtk_cosynthesis_dtree_search_qs(wtk_cosynthesis_dtree_t *d,wtk_cosynthesis_dtree_tree_type_t type,char *s,int s_bytes)
{
	return  wtk_cosynthesis_qs_get(d->qs[type],s,s_bytes);
}



