#include <ctype.h>
#include "wtk_xbnf.h"
int wtk_xbnf_feed(wtk_xbnf_t *b,wtk_string_t *str);
void wtk_xbnf_merge_item(wtk_xbnf_t *b,wtk_xbnf_item_t *item);
void wtk_xbnf_merge_set(wtk_xbnf_t *b,wtk_queue_t *set_q);

wtk_xbnf_t* wtk_xbnf_new(wtk_xbnf_cfg_t *cfg)
{
	wtk_xbnf_t *b;

	b=(wtk_xbnf_t*)wtk_malloc(sizeof(wtk_xbnf_t));
	b->cfg=cfg;
	b->pool=wtk_strpool_new(cfg->hash_hint);
	b->heap=wtk_heap_new(4096);
	b->buf=wtk_strbuf_new(256,1);
	b->tmp_buf=wtk_strbuf_new(256,1);
	b->attr_buf=wtk_strbuf_new(256,1);
	b->expr_hash=wtk_str_hash_new(cfg->expr_hint);
	wtk_queue_init(&(b->expr_q));
	wtk_xbnf_reset(b);
	b->seg=NULL;
	if(cfg->use_seg)
	{
		b->seg = wtk_segmenter_new(&(cfg->segmenter),NULL);
	}
        b->wdec_cnt = 1;
        memset(b->wrds_cnt, 0, sizeof(int) * 5);
        return b;
}

void wtk_xbnf_delete(wtk_xbnf_t *b)
{
	wtk_strbuf_delete(b->attr_buf);
	wtk_strbuf_delete(b->tmp_buf);
	wtk_strbuf_delete(b->buf);
	wtk_strpool_delete(b->pool);
	wtk_heap_delete(b->heap);
	wtk_str_hash_delete(b->expr_hash);
	if(b->seg)
	{
		wtk_segmenter_delete(b->seg);
	}
	wtk_free(b);
}

void wtk_xbnf_set_state(wtk_xbnf_t *b,wtk_xbnf_state_t state)
{
	b->state=state;
	b->sub_state=-1;
}

void wtk_xbnf_reset(wtk_xbnf_t *b)
{
	b->pwd=NULL;
	wtk_strpool_reset(b->pool);
	wtk_heap_reset(b->heap);
	wtk_str_hash_reset(b->expr_hash);
	wtk_xbnf_set_state(b,WTK_XBNF_EXPR_INIT);
	b->cur_expr=NULL;
	b->cur_item=NULL;
	wtk_queue_init(&(b->stack_q));
	wtk_queue_init(&(b->expr_q));
        b->wdec_cnt = 1;
        memset(b->wrds_cnt, 0, sizeof(int) * 5);
}

void wtk_xbnf_item_attr_init(wtk_xbnf_item_attr_t *attr)
{
	attr->min=1;
	attr->max=1;
	attr->k=NULL;
	attr->v=NULL;
	wtk_queue_init(&(attr->attr_q));
}

wtk_xbnf_item_t* wtk_xbnf_new_item(wtk_xbnf_t *b)
{
	wtk_xbnf_item_t *item;
	wtk_heap_t *heap=b->heap;

	item=(wtk_xbnf_item_t*)wtk_heap_malloc(heap,sizeof(wtk_xbnf_item_t));
	item->v.str=NULL;
	item->hook=NULL;
	item->eof=0;
	item->start=0;
	wtk_xbnf_item_attr_init(&(item->attr));
	return item;
}

wtk_xbnf_item_t* wtk_xbnf_new_item_str(wtk_xbnf_t *b,wtk_string_t *str)
{
	wtk_xbnf_item_t *item;

	item=wtk_xbnf_new_item(b);//(wtk_xbnf_item_t*)wtk_heap_malloc(heap,sizeof(wtk_xbnf_item_t));
	item->type=WTK_XBNF_ITEM_STR;
	item->v.str=wtk_strpool_find(b->pool,str->data,str->len,1);
	if(wtk_string_cmp_s(str,"</s>")==0)
	{
		item->eof=1;
	}else if(wtk_string_cmp_s(str,"<s>")==0)
	{
		item->start=1;
	}
	return item;
}

wtk_xbnf_item_t* wtk_xbnf_new_item_var(wtk_xbnf_t *b,char *name,int name_bytes)
{
	wtk_xbnf_item_t *item;
	//wtk_heap_t *heap=b->heap;
	wtk_xbnf_expr_t *expr;

	//wtk_debug("[%.*s] cur_item=%p cur_expr=%p\n",name_bytes,name,b->cur_item,b->cur_expr);
	expr=wtk_str_hash_find(b->expr_hash,name,name_bytes);
	if(!expr)
	{
		wtk_debug("[%.*s] not found\n",name_bytes,name);
		return NULL;
	}
	item=wtk_xbnf_new_item(b);
	//item=(wtk_xbnf_item_t*)wtk_heap_malloc(heap,sizeof(wtk_xbnf_item_t));
	item->type=WTK_XBNF_ITEM_VAR;
	item->v.expr=expr;
	return item;
}

int wtk_xbnf_item_has_sub(wtk_xbnf_item_t *item)
{
	if(item->type==WTK_XBNF_ITEM_STR || item->type==WTK_XBNF_ITEM_VAR)
	{
		return 0;
	}else
	{
		return 1;
	}
}


wtk_xbnf_item_list_t* wtk_xbnf_new_list(wtk_xbnf_t *b)
{
	wtk_xbnf_item_list_t *l;

	l=(wtk_xbnf_item_list_t*)wtk_heap_malloc(b->heap,sizeof(wtk_xbnf_item_list_t));
	wtk_queue_init(&(l->list_q));
	return l;
}

wtk_xbnf_item_set_t* wtk_xbnf_new_set(wtk_xbnf_t *b,int pad_list)
{
	wtk_xbnf_item_set_t *set;
	wtk_xbnf_item_list_t *list;

	set=(wtk_xbnf_item_set_t*)wtk_heap_malloc(b->heap,sizeof(wtk_xbnf_item_set_t));
	wtk_queue_init(&(set->set_q));
	if(pad_list)
	{
		list=wtk_xbnf_new_list(b);
		wtk_queue_push(&(set->set_q),&(list->q_n));
	}
	set->eof=0;
	return set;
}

wtk_xbnf_item_t* wtk_xbnf_new_item_set(wtk_xbnf_t *b,int pad_list)
{
	wtk_xbnf_item_t *item;
	//wtk_heap_t *heap=b->heap;

	item=wtk_xbnf_new_item(b);
	//item=(wtk_xbnf_item_t*)wtk_heap_malloc(heap,sizeof(wtk_xbnf_item_t));
	item->v.set=wtk_xbnf_new_set(b,pad_list);
	return item;
}

char wtk_xbnf_item_type_get_char(wtk_xbnf_item_type_t t)
{
	switch(t)
	{
	case WTK_XBNF_ITEM_STR:
		return 'S';
	case WTK_XBNF_ITEM_PARENTHESIS:
		return ')';
	case WTK_XBNF_ITEM_BRACKET:
		return ']';
	case WTK_XBNF_ITEM_BRACE:
		return '}';
	case WTK_XBNF_ITEM_ANGLE:
		return '>';
	case WTK_XBNF_ITEM_VAR:
		return '$';
	case WTK_XBNF_ITEM_BRANCH:
		return 'B';
	}
	return 'N';
}

wtk_xbnf_item_t* wtk_xbnf_new_item_parenthesis(wtk_xbnf_t *b)
{
	wtk_xbnf_item_t *item;

	item=wtk_xbnf_new_item_set(b,1);
	item->type=WTK_XBNF_ITEM_PARENTHESIS;
	return item;
}

wtk_xbnf_item_t* wtk_xbnf_new_item_bracket(wtk_xbnf_t *b)
{
	wtk_xbnf_item_t *item;

	item=wtk_xbnf_new_item_set(b,1);
	item->type=WTK_XBNF_ITEM_BRACKET;
	item->attr.min=0;
	item->attr.max=1;
	return item;
}

wtk_xbnf_item_t* wtk_xbnf_new_item_brace(wtk_xbnf_t *b)
{
	wtk_xbnf_item_t *item;

	item=wtk_xbnf_new_item_set(b,1);
	item->type=WTK_XBNF_ITEM_BRACE;
	item->attr.min=0;
	item->attr.max=-1;
	return item;
}

wtk_xbnf_item_t* wtk_xbnf_new_item_angle(wtk_xbnf_t *b)
{
	wtk_xbnf_item_t *item;

	item=wtk_xbnf_new_item_set(b,1);
	item->type=WTK_XBNF_ITEM_ANGLE;
	item->attr.min=1;
	item->attr.max=-1;
	return item;
}

wtk_xbnf_item_t* wtk_xbnf_new_item_branch(wtk_xbnf_t *b)
{
	wtk_xbnf_item_t *item;

	item=wtk_xbnf_new_item_set(b,0);
	item->type=WTK_XBNF_ITEM_BRANCH;
	item->v.set->eof=1;
	return item;
}


wtk_xbnf_expr_t* wtk_xbnf_new_expr(wtk_xbnf_t *b)
{
	wtk_xbnf_expr_t *expr;
	wtk_heap_t *heap=b->heap;

	expr=(wtk_xbnf_expr_t*)wtk_heap_malloc(heap,sizeof(wtk_xbnf_expr_t));
	expr->name=NULL;
	//expr->list=wtk_xbnf_new_item_list(b);
	expr->set=wtk_xbnf_new_set(b,1);
	return expr;
}

void wtk_xbnf_item_set_print(wtk_xbnf_item_set_t *set);
void wtk_xbnf_item_list_print(wtk_xbnf_item_list_t *list);

void wtk_xbnf_item_print(wtk_xbnf_item_t *item)
{
	//wtk_debug("item=%p type=%d\n",item,item->type);
	switch(item->type)
	{
	case WTK_XBNF_ITEM_STR:
		printf("%.*s",item->v.str->len,item->v.str->data);
		break;
	//WTK_XBNF_ITEM_OR,
	case WTK_XBNF_ITEM_PARENTHESIS:
		printf("(");
		wtk_xbnf_item_set_print(item->v.set);
		if(item->v.set->eof)
		{
			printf(")");
		}
		break;
	case WTK_XBNF_ITEM_BRACKET:
		printf("[");
		wtk_xbnf_item_set_print(item->v.set);
		if(item->v.set->eof)
		{
			printf("]");
		}
		break;
	case WTK_XBNF_ITEM_BRACE:
		printf("{");
		wtk_xbnf_item_set_print(item->v.set);
		if(item->v.set->eof)
		{
			printf("}");
		}
		break;
	case WTK_XBNF_ITEM_ANGLE:
		printf("<");
		wtk_xbnf_item_set_print(item->v.set);
		if(item->v.set->eof)
		{
			printf(">");
		}
		break;
	case WTK_XBNF_ITEM_BRANCH:
		printf("(!");
		wtk_xbnf_item_set_print(item->v.set);
		if(item->v.set->eof)
		{
			printf("!)");
		}
		break;
	case WTK_XBNF_ITEM_VAR:
		if(0)
		{
			wtk_xbnf_item_set_print(item->v.expr->set);
		}else
		{
			printf("$%.*s ",item->v.expr->name->len,item->v.expr->name->data);
		}
		break;
	}
	if(item->type==WTK_XBNF_ITEM_STR || item->type==WTK_XBNF_ITEM_PARENTHESIS)
	{
		if(item->attr.min!=1 || item->attr.max!=1)
		{
			printf("/min=%d,max=%d/",item->attr.min,item->attr.max);
		}
	}
}

void wtk_xbnf_item_set_print(wtk_xbnf_item_set_t *set)
{
	wtk_queue_node_t *qn;
	wtk_xbnf_item_list_t *list;
	//wtk_xbnf_item_list_t *list;

	for(qn=set->set_q.pop;qn;qn=qn->next)
	{
		list=data_offset(qn,wtk_xbnf_item_list_t,q_n);
		if(qn!=set->set_q.pop)
		{
			printf("|");
		}
		if(list->list_q.length>0)
		{
			wtk_xbnf_item_list_print(list);
		}else
		{
			printf("NIL");
		}
	}
}

void wtk_xbnf_item_list_print(wtk_xbnf_item_list_t *list)
{
	wtk_queue_node_t *qn;
	wtk_xbnf_item_t *item;

	for(qn=list->list_q.pop;qn;qn=qn->next)
	{
		item=data_offset(qn,wtk_xbnf_item_t,q_n);
		wtk_xbnf_item_print(item);
	}
}

void wtk_xbnf_expr_print(wtk_xbnf_expr_t *e)
{
	//wtk_debug("=========== %.*s ==========\n",e->name->len,e->name->data);
	if(e->name)
	{
		printf("$%.*s=",e->name->len,e->name->data);
	}
	wtk_xbnf_item_set_print(e->set);
	if(e->name)
	{
		printf(";\n");
	}else
	{
		printf("\n");
	}
}

int wtk_xbnf_feed_comment(wtk_xbnf_t *b,wtk_string_t *str)
{
	wtk_strbuf_t *buf=b->buf;
	char c;

	if(buf->pos==0 && wtk_string_cmp_s(str,"include")==0)
	{
		wtk_xbnf_set_state(b,WTK_XBNF_INCLUDE);
		goto end;
	}
	wtk_strbuf_push(buf,str->data,str->len);
	if(str->len!=1){goto end;}
	c=*(str->data);
	if(c=='\n')
	{
		wtk_strbuf_reset(buf);
		wtk_xbnf_set_state(b,WTK_XBNF_EXPR_INIT);
	}
end:
	return 0;
}

int wtk_xbnf_feed_expr_init(wtk_xbnf_t *b,wtk_string_t *str)
{
	char c;
	int ret=-1;

	if(str->len!=1){goto end;}
	c=*(str->data);
	if(c=='#')
	{
		wtk_xbnf_set_state(b,WTK_XBNF_COMMENT);
		wtk_strbuf_reset(b->buf);
	}else if(c=='$')
	{
		b->cur_expr=wtk_xbnf_new_expr(b);
		b->cur_item=NULL;
		wtk_xbnf_set_state(b,WTK_XBNF_EXPR_NAME);
		wtk_strbuf_reset(b->buf);
	}else if(c=='(')
	{
		wtk_xbnf_set_state(b,WTK_XBNF_EXPR_WAIT_V);
		b->cur_expr=wtk_xbnf_new_expr(b);
		b->main_expr=b->cur_expr;
		b->cur_item=NULL;
		wtk_xbnf_feed(b,str);
	}else if(!isspace(c))
	{
		ret=-1;
		goto end;
	}
	ret=0;
end:
	return ret;
}

int wtk_xbnf_feed_expr_name(wtk_xbnf_t *b,wtk_string_t *str)
{
	wtk_strbuf_t *buf=b->buf;
	char c;

	//wtk_debug("[%.*s]\n",str->len,str->data);
	if(str->len!=1)
	{
		wtk_strbuf_push(buf,str->data,str->len);
	}else
	{
		c=*(str->data);
		if(isspace(c))
		{
			//wtk_debug("[%.*s]\n",buf->pos,buf->data);
			b->cur_expr->name=wtk_strpool_find(b->pool,buf->data,buf->pos,1);
			wtk_xbnf_set_state(b,WTK_XBNF_EXPR_WAIT_EQ);
		}else if(c=='=')
		{
			//wtk_debug("[%.*s]\n",buf->pos,buf->data);
			b->cur_expr->name=wtk_strpool_find(b->pool,buf->data,buf->pos,1);
			wtk_xbnf_set_state(b,WTK_XBNF_EXPR_WAIT_V);
		}else
		{
			wtk_strbuf_push_c(buf,c);
		}
	}
	return 0;
}

int wtk_xbnf_feed_expr_wait_eq(wtk_xbnf_t *b,wtk_string_t *str)
{
	char c;
	int ret=-1;

	if(str->len!=1){goto end;}
	c=*(str->data);
	if(c=='=')
	{
		wtk_xbnf_set_state(b,WTK_XBNF_EXPR_WAIT_V);
		ret=0;
	}else if(isspace(c))
	{
		ret=0;
	}else
	{
		ret=-1;
	}
end:
	return ret;
}


wtk_xbnf_item_set_t* wtk_xbnf_get_last_item_set(wtk_xbnf_t *b)
{
	if(b->cur_item)
	{
		return b->cur_item->v.set;
	}else
	{
		return b->cur_expr->set;
	}
}

wtk_xbnf_item_list_t* wtk_xbnf_get_last_item_list(wtk_xbnf_t *b)
{
	wtk_xbnf_item_set_t *set;
	wtk_xbnf_item_list_t *list;

	set=wtk_xbnf_get_last_item_set(b);
	list=data_offset(set->set_q.push,wtk_xbnf_item_list_t,q_n);
	return list;
}

void wtk_xbnf_append_item(wtk_xbnf_t *b,wtk_xbnf_item_t *item)
{
	wtk_xbnf_item_list_t *list;

	list=wtk_xbnf_get_last_item_list(b);
	wtk_queue_push(&(list->list_q),&(item->q_n));
	if(wtk_xbnf_item_has_sub(item))
	{
		wtk_queue_push(&(b->stack_q),&(item->stack_n));
		b->cur_item=item;
	}
	//wtk_xbnf_expr_print(b->cur_expr);
}


void wtk_xbnf_update_or(wtk_xbnf_t *b)
{
	wtk_xbnf_item_set_t *set;
	wtk_xbnf_item_list_t *list;

	set=wtk_xbnf_get_last_item_set(b);
	list=wtk_xbnf_new_list(b);
	wtk_queue_push(&(set->set_q),&(list->q_n));
}

void wtk_xbnf_update_set_eof(wtk_xbnf_t *b,char c)
{
	if(!b->cur_item)
	{
		return;
	}
	//wtk_debug("[%c]=[%c]\n",wtk_xbnf_item_type_get_char(b->cur_item->type),c);
	//wtk_debug("item=%p\n",b->cur_item);
	b->cur_item->v.set->eof=1;
	if(b->cfg->use_merge)// && b->cur_item->v.set->set_q.length>1)
	{
		//wtk_xbnf_merge_last_list(b,b->cur_item->v.set);
		wtk_xbnf_merge_item(b,b->cur_item);
	}
	if(b->stack_q.length>0)
	{
		wtk_queue_pop_back(&(b->stack_q));
		if(b->stack_q.length>0)
		{
			b->cur_item=data_offset(b->stack_q.push,wtk_xbnf_item_t,stack_n);
		}else
		{
			b->cur_item=NULL;
		}
	}else
	{
		b->cur_item=NULL;
	}
}

void wtk_xbnf_update_expr(wtk_xbnf_t *b)
{
	if(b->stack_q.length>0)
	{
		wtk_debug("[%.*s] not end\n",b->cur_expr->name->len,b->cur_expr->name->data);
		//exit(0);
	}
	if(b->cfg->use_merge && b->cur_expr->set->set_q.length>1)
	{
		wtk_xbnf_merge_set(b,&(b->cur_expr->set->set_q));
	}
	if(b->cur_expr->name)
	{
		wtk_str_hash_add(b->expr_hash,b->cur_expr->name->data,b->cur_expr->name->len,b->cur_expr);
	}
	wtk_queue_push(&(b->expr_q),&(b->cur_expr->q_n));
	wtk_queue_init(&(b->stack_q));
	b->cur_expr=NULL;
	b->cur_item=NULL;
}

int wtk_xbnf_feed_expr_wait_v(wtk_xbnf_t *b,wtk_string_t *str)
{
	wtk_xbnf_item_t *item;
	char c;

	if(str->len!=1)
	{
		item=wtk_xbnf_new_item_str(b,str);
		wtk_xbnf_append_item(b,item);
	}else
	{
		c=*(str->data);
		switch(c)
		{
		case '(':
			item=wtk_xbnf_new_item_parenthesis(b);
			wtk_xbnf_append_item(b,item);
			break;
		case '[':
			item=wtk_xbnf_new_item_bracket(b);
			wtk_xbnf_append_item(b,item);
			break;
		case '|':
			wtk_xbnf_update_or(b);
			break;
		case ',':
			wtk_xbnf_update_or(b);
			break;
		case ']':
			wtk_xbnf_update_set_eof(b,c);
			break;
		case ')':
			wtk_xbnf_update_set_eof(b,c);
			break;
		case '<':
			item=wtk_xbnf_new_item_angle(b);
			wtk_xbnf_append_item(b,item);
			break;
		case '>':
			wtk_xbnf_update_set_eof(b,c);
			break;
		case '{':
			item=wtk_xbnf_new_item_brace(b);
			wtk_xbnf_append_item(b,item);
			break;
		case '}':
			wtk_xbnf_update_set_eof(b,c);
			break;
		case ';':
			b->cur_expr->set->eof=1;
			//wtk_xbnf_expr_print(b->cur_expr);
			wtk_xbnf_update_expr(b);
			wtk_xbnf_set_state(b,WTK_XBNF_EXPR_INIT);
			break;
		case '$':
			wtk_xbnf_set_state(b,WTK_XBNF_EXPR_VAR);
			wtk_strbuf_reset(b->buf);
			break;
		case '/':
			wtk_strbuf_reset(b->buf);
			wtk_strbuf_push_s(b->buf,"[");
			wtk_xbnf_set_state(b,WTK_XBNF_EXPR_ATTR);
			break;
		default:
			if(!isspace(c))
			{
				item=wtk_xbnf_new_item_str(b,str);
				wtk_xbnf_append_item(b,item);
				//wtk_queue_push(&(b->cur_expr->item_q),&(item->q_n));
			}
			break;
		}
	}
	return 0;
}

int wtk_xbnf_feed_expr_var(wtk_xbnf_t *b,wtk_string_t *str)
{
	wtk_xbnf_item_t *item;
	char c;

	if(str->len==1)
	{
		c=*(str->data);
		if(c==';'||c=='{'||c=='}'
				||c=='<'||c=='>'
				||c=='['||c==']'
				||c=='('||c==')'
				||c=='|' ||c=='/' ||c==',')
		{
			//wtk_debug("[%.*s]\n",b->buf->pos,b->buf->data);
			item=wtk_xbnf_new_item_var(b,b->buf->data,b->buf->pos);
			//wtk_debug("item=%p\n",item);
			if(!item)
			{
				return -1;
			}
			wtk_xbnf_append_item(b,item);
			//wtk_xbnf_expr_print(b->cur_expr);
			//exit(0);
			//wtk_xbnf_update_expr(b);
			wtk_xbnf_set_state(b,WTK_XBNF_EXPR_WAIT_V);
			wtk_xbnf_feed(b,str);
		}else if(isspace(c))
		{
			//wtk_debug("[%.*s]\n",b->buf->pos,b->buf->data);
			item=wtk_xbnf_new_item_var(b,b->buf->data,b->buf->pos);
			if(!item)
			{
				return -1;
			}
			wtk_xbnf_append_item(b,item);
			wtk_xbnf_set_state(b,WTK_XBNF_EXPR_WAIT_V);
		}else
		{
			wtk_strbuf_push(b->buf,str->data,str->len);
		}
	}else
	{
		wtk_strbuf_push(b->buf,str->data,str->len);
	}
	return 0;
}

void wtk_xbnf_item_update_atrr(void **p,wtk_string_t *k,wtk_string_t *v)
{
	wtk_xbnf_item_t *item;
	wtk_xbnf_item_attr_t *attr;
	wtk_xbnf_t *b;
	wtk_xbnf_item_attr_item_t *ai;

	b=(wtk_xbnf_t*)(p[0]);
	item=(wtk_xbnf_item_t*)(p[1]);
	attr=&(item->attr);
	if(wtk_string_cmp_s(k,"min")==0)
	{
		attr->min=wtk_str_atoi(v->data,v->len);
	}else if(wtk_string_cmp_s(k,"max")==0)
	{
		attr->max=wtk_str_atoi(v->data,v->len);
	}else if(wtk_string_cmp_s(k,"k")==0)
	{
		wtk_strbuf_parse_quote(b->attr_buf,v->data,v->len);
		attr->k=wtk_heap_dup_string(b->heap,b->attr_buf->data,b->attr_buf->pos);
	}else if(wtk_string_cmp_s(k,"v")==0)
	{
		wtk_strbuf_parse_quote(b->attr_buf,v->data,v->len);
		attr->v=wtk_heap_dup_string(b->heap,b->attr_buf->data,b->attr_buf->pos);
	}else
	{
		ai=(wtk_xbnf_item_attr_item_t*)wtk_heap_malloc(b->heap,sizeof(wtk_xbnf_item_attr_item_t));
		wtk_strbuf_parse_quote(b->attr_buf,k->data,k->len);
		ai->k=wtk_heap_dup_string(b->heap,b->attr_buf->data,b->attr_buf->pos);
		wtk_strbuf_parse_quote(b->attr_buf,v->data,v->len);
		ai->v=wtk_heap_dup_string(b->heap,b->attr_buf->data,b->attr_buf->pos);
		wtk_queue_push(&(attr->attr_q),&(ai->q_n));
	}
	//wtk_debug("[%.*s]=[%.*s]\n",k->len,k->data,v->len,v->data);
}

int wtk_xbnf_feed_expr_attr(wtk_xbnf_t *b,wtk_string_t *str)
{
	wtk_strbuf_t *buf=b->buf;
	wtk_xbnf_item_list_t *list;
	wtk_xbnf_item_t *item;
	void *px[2];

	if(str->len==1 && *(str->data)=='/')
	{
		px[0]=b;
		wtk_strbuf_push_s(buf,"]");
		//wtk_debug("%.*s\n",buf->pos,buf->data);
		list=wtk_xbnf_get_last_item_list(b);
		//wtk_debug("list=%p len=%d\n",list,list->list_q.length);
		if(list && list->list_q.length>0)
		{
			item=data_offset(list->list_q.push,wtk_xbnf_item_t,q_n);
			/*
			wtk_xbnf_item_print(item);
			printf("\n");*/
			px[1]=item;
			wtk_str_attr_parse(buf->data,buf->pos,px,(wtk_str_attr_f)wtk_xbnf_item_update_atrr);
		}
		wtk_xbnf_set_state(b,WTK_XBNF_EXPR_WAIT_V);
	}else
	{
		wtk_strbuf_push(buf,str->data,str->len);
	}
	return 0;
}

int wtk_xbnf_feed_include(wtk_xbnf_t *b,wtk_string_t *str)
{
enum wtk_xbnf_inc_state_t
{
	WTK_XBNF_INC_INIT=0,
	WTK_XBNF_INC_PWD_LEFT,
	WTK_XBNF_INC_PWD,
	WTK_XBNF_INC_FILE,
};
	wtk_strbuf_t *buf=b->buf;
	wtk_string_t *pwd;
	char c;

	if(b->sub_state==-1)
	{
		b->sub_state=WTK_XBNF_INC_INIT;
	}
	//wtk_debug("[%.*s]\n",str->len,str->data);
	c=str->data[0];
	switch(b->sub_state)
	{
	case WTK_XBNF_INC_INIT:
		if(str->len==1)
		{
			if(isspace(c))
			{
			}else if(c=='$')
			{
				b->sub_state=WTK_XBNF_INC_PWD_LEFT;
			}else
			{
				b->sub_state=WTK_XBNF_INC_FILE;
				wtk_strbuf_reset(buf);
			}
		}else
		{
			wtk_xbnf_set_state(b,WTK_XBNF_COMMENT);
		}
		break;
	case WTK_XBNF_INC_PWD_LEFT:
		if(str->len==1)
		{
			if(isspace(c))
			{
			}else if(c=='{')
			{
				wtk_strbuf_reset(buf);
				b->sub_state=WTK_XBNF_INC_PWD;
			}else
			{
				wtk_xbnf_set_state(b,WTK_XBNF_COMMENT);
			}
		}else
		{
			wtk_xbnf_set_state(b,WTK_XBNF_COMMENT);
		}
		break;
	case WTK_XBNF_INC_PWD:
		//wtk_debug("[%.*s]\n",str->len,str->data);
		if(str->len==1 && c=='}')
		{
			wtk_strbuf_strip(b->tmp_buf);
			//wtk_debug("[%.*s]\n",buf->pos,buf->data);
			if(wtk_str_equal_s(buf->data,buf->pos,"pwd"))
			{
				wtk_strbuf_reset(buf);
				if(b->pwd)
				{
					wtk_strbuf_push(buf,b->pwd->data,b->pwd->len);
				}
				b->sub_state=WTK_XBNF_INC_FILE;
			}else
			{
				wtk_xbnf_set_state(b,WTK_XBNF_COMMENT);
			}
		}else
		{
			wtk_strbuf_push(buf,str->data,str->len);
		}
		break;
	case WTK_XBNF_INC_FILE:
		if(str->len==1 &&(isspace(c) || c==';'))
		{
			//wtk_debug("[%.*s]\n",buf->pos,buf->data);
			pwd=b->pwd;
			wtk_strbuf_push_c(buf,0);
			wtk_xbnf_set_state(b,WTK_XBNF_EXPR_INIT);
			wtk_xbnf_compile_file(b,buf->data);
			b->pwd=pwd;
			wtk_xbnf_set_state(b,WTK_XBNF_COMMENT);
			//exit(0);
		}else
		{
			wtk_strbuf_push(buf,str->data,str->len);
		}
		break;
	}
	return 0;
}

int wtk_xbnf_feed(wtk_xbnf_t *b,wtk_string_t *str)
{
	int ret;

	//wtk_debug("[%.*s]\n",str->len,str->data);
	switch(b->state)
	{
	case WTK_XBNF_EXPR_INIT:
		ret=wtk_xbnf_feed_expr_init(b,str);
		break;
	case WTK_XBNF_COMMENT:
		ret=wtk_xbnf_feed_comment(b,str);
		break;
	case WTK_XBNF_EXPR_NAME:
		ret=wtk_xbnf_feed_expr_name(b,str);
		break;
	case WTK_XBNF_EXPR_WAIT_EQ:
		ret=wtk_xbnf_feed_expr_wait_eq(b,str);
		break;
	case WTK_XBNF_EXPR_WAIT_V:
		ret=wtk_xbnf_feed_expr_wait_v(b,str);
		break;
	case WTK_XBNF_EXPR_VAR:
		ret=wtk_xbnf_feed_expr_var(b,str);
		break;
	case WTK_XBNF_EXPR_ATTR:
		ret=wtk_xbnf_feed_expr_attr(b,str);
		break;
	case WTK_XBNF_INCLUDE:
		ret=wtk_xbnf_feed_include(b,str);
		break;
	default:
		ret=-1;
		wtk_debug("never be herer\n");
		break;
	}
	return ret;
}

int wtk_xbnf_array_has(wtk_array_t *a,char *data,int bytes)
{
	wtk_string_t **strs;
	int i;

	strs=(wtk_string_t**)a->slot;
	for(i=0;i<a->nslot;++i)
	{
		if(wtk_string_cmp(strs[i],data,bytes)==0)
		{
			return 1;
		}
	}
	return 0;
}

int wtk_xbnf_span_check(char *s, int cnt)
{
	return(wtk_str_equal_s(s,cnt,"á") || wtk_str_equal_s(s,cnt,"é")
			|| wtk_str_equal_s(s,cnt,"í") ||wtk_str_equal_s(s,cnt,"ñ")
			|| wtk_str_equal_s(s,cnt,"ó") || wtk_str_equal_s(s,cnt,"ú")
			|| wtk_str_equal_s(s,cnt,"ü"));
}

int wtk_xbnf_ger_check(char *s, int cnt)
{
	return(wtk_str_equal_s(s,cnt,"ö") || wtk_str_equal_s(s,cnt,"ü")
			|| wtk_str_equal_s(s,cnt,"β") ||wtk_str_equal_s(s,cnt,"ä"));
}

int wtk_xbnf_eng_check(char *s, int cnt)
{
	return 0;
}

char *wtk_xbnf_cfg_next_wrd(wtk_xbnf_cfg_t *cfg, char *s, char *e,
                            wtk_strbuf_t *buf, int *chn_wrd) {
    typedef enum {
        WTK_XBNF_WRD_INIT,
        WTK_XBNF_WRD_EN,
        WTK_XBNF_WRD_ESC,
    } wtk_xbnf_wrd_state_t;
    wtk_xbnf_wrd_state_t state;
    // wtk_xbnf_cfg_t *cfg=xbnf->cfg;
    int cnt;
    char c;

    state = WTK_XBNF_WRD_INIT;
    wtk_strbuf_reset(buf);
    while (s < e) {
        cnt = wtk_utf8_bytes(*s);
        // wtk_debug("%s cnt=%d\n",s,cnt);
        switch (state) {
        case WTK_XBNF_WRD_INIT:
            if (cnt > 1) {
                wtk_strbuf_push(buf, s, cnt);
                if (wtk_xbnf_span_check(s, cnt) || wtk_xbnf_ger_check(s, cnt)) {
                    state = WTK_XBNF_WRD_EN;
                } else if (cfg->use_seg) {
                    state = WTK_XBNF_WRD_EN;
                } else {
                    return s + cnt;
                }
            } else {
                c = *s;
                if (isalnum(c) ||
                    (cfg->en_pre && wtk_xbnf_array_has(cfg->en_pre, s, cnt))) {
                    state = WTK_XBNF_WRD_EN;
                    wtk_strbuf_push(buf, s, cnt);
                } else if (c == '\\') {
                    state = WTK_XBNF_WRD_ESC;
                } else {
                    wtk_strbuf_push(buf, s, cnt);
                    return s + cnt;
                }
            }
            break;
        case WTK_XBNF_WRD_EN:
            *chn_wrd = 0;
            if (cnt != 1) {
                if (wtk_xbnf_span_check(s, cnt) || wtk_xbnf_ger_check(s, cnt)) {
                    wtk_strbuf_push(buf, s, cnt);
                } else if (cfg->en_pst &&
                           wtk_xbnf_array_has(cfg->en_pst, s, cnt)) {
                    return s + cnt;
                } else if (cfg->use_seg) {
                    wtk_strbuf_push(buf, s, cnt);
                } else {
                    return s;
                }
            } else {
                c = *s;
                if (c == '\\') {
                    state = WTK_XBNF_WRD_ESC;
                } else if (isalnum(c) ||
                           (cfg->en_mid &&
                            wtk_xbnf_array_has(cfg->en_mid, s, cnt))) {
                    wtk_strbuf_push(buf, s, cnt);
                } else if (c == '\'') {
                    wtk_strbuf_push(buf, s, cnt);
                } else {
                    if (cfg->en_pst &&
                        wtk_xbnf_array_has(cfg->en_pst, s, cnt)) {
                        return s + cnt;
                    }
                    return s;
                }
            }
            break;
        case WTK_XBNF_WRD_ESC:
            wtk_strbuf_push(buf, s, cnt);
            state = WTK_XBNF_WRD_EN;
            break;
        }
        s += cnt;
    }
    return s;
}

int wtk_xbnf_compile(wtk_xbnf_t *b,char *data,int bytes)
{
    int ret, i, chn_wrd;
    char *s, *e;
    wtk_string_t v;
    wtk_strbuf_t *buf;

    buf = b->tmp_buf;
    ret = -1;
    s = data;
    e = s + bytes;
    while (s < e) {
        chn_wrd = 1;
        s = wtk_xbnf_cfg_next_wrd(b->cfg, s, e, buf, &chn_wrd);
        // wtk_debug("[%.*s]\n",buf->pos,buf->data);
        if (buf->pos > 1 && chn_wrd) {
            // wtk_debug("[%.*s]\n",buf->pos,buf->data);
            if (b->wdec_cnt <= 5) {
                b->wrds_cnt[b->wdec_cnt - 1]++;
            }

        } else if (wtk_str_equal_s(buf->data, buf->pos, "|")) {
            // wtk_debug("[%.*s]\n",buf->pos,buf->data);
            b->wdec_cnt++;
        }

        if (b->cfg->use_seg) {
            wtk_segmenter_parse(b->seg, buf->data, buf->pos, NULL);
            for (i = 0; i < b->seg->wrd_array_n; i++) {
                // wtk_debug("[%.*s]\n",b->seg->wrd_array[i]->len,b->seg->wrd_array[i]->data);
                ret = wtk_xbnf_feed(b, b->seg->wrd_array[i]);
                if (ret != 0) {
                    wtk_debug("err at [%.*s]\n", v.len, v.data);
                    goto end;
                }
            }
            wtk_segmenter_reset(b->seg);
        } else {
            wtk_string_set(&(v), buf->data, buf->pos);
            // wtk_debug("[%.*s]\n",v.len,v.data);
            ret = wtk_xbnf_feed(b, &v);
            if (ret != 0) {
                wtk_debug("err at [%.*s]\n", v.len, v.data);
                goto end;
            }
        }
	}
	if(b->state==WTK_XBNF_EXPR_WAIT_V)
	{
		if(b->stack_q.length==0)
		{
			ret=0;
		}else
		{
			wtk_debug("stack len>0\n");
			ret=-1;
		}
	}else
	{
		//wtk_debug("not waitv %d\n",b->state);
		//ret=-1;
		ret=0;
	}
end:
	return ret;
}

int wtk_xbnf_compile_file(wtk_xbnf_t *b,char *fn)
{
	char *data;
	int len;
	int ret=-1;
	wtk_string_t v;

	//wtk_debug("compile %s\n",fn);
	v=wtk_dir_name2(fn,strlen(fn),'/');
	b->pwd=wtk_heap_dup_string(b->heap,v.data,v.len);
	data=file_read_buf(fn,&len);
	if(!data){goto end;}
	ret=wtk_xbnf_compile(b,data,len);
end:
	//wtk_debug("data=%p len=%d ret=%d\n",data,len,ret);
	if(data)
	{
		wtk_free(data);
	}
	return ret;
}

void wtk_xbnf_print(wtk_xbnf_t *b)
{
	wtk_xbnf_expr_print(b->main_expr);
}

void wtk_xbnf_add_list(wtk_xbnf_t *b,wtk_xbnf_item_list_t *list,wtk_xbnf_item_t *item)
{
	wtk_queue_node_t *qn;

	while(item)
	{
		qn=item->q_n.next;
		wtk_queue_push(&(list->list_q),&(item->q_n));
		//wtk_debug("add q %p:%d\n",item,wtk_queue_node_len(&(item->q_n)));
		if(qn)//item->q_n.next)
		{
			/*
			wtk_debug("add q\n");
			wtk_xbnf_item_print(item);
			printf("\n");*/
			item=data_offset(qn,wtk_xbnf_item_t,q_n);
		}else
		{
			break;
		}
	}
}

void wtk_xbnf_add_branch(wtk_xbnf_t *b,wtk_xbnf_item_t *item,
		wtk_xbnf_item_t *item1,wtk_xbnf_item_t *item2)
{
	wtk_xbnf_item_list_t *list;

	//wtk_debug("add item1=%p item2=%p len=%d %d/%d\n",item1,item2,item->v.set->set_q.length,wtk_queue_node_len(&(item1->q_n)),wtk_queue_node_len(&(item2->q_n)));
	list=wtk_xbnf_new_list(b);
	wtk_xbnf_add_list(b,list,item1);
	wtk_queue_push(&(item->v.set->set_q),&(list->q_n));

	list=wtk_xbnf_new_list(b);
	wtk_xbnf_add_list(b,list,item2);
	wtk_queue_push(&(item->v.set->set_q),&(list->q_n));
}

typedef struct
{
	wtk_xbnf_item_t *item1;//list item;
	wtk_xbnf_item_t *item2;//to copy item;
	int cnt;
}wtk_xbnf_merge_ret_t;

int wtk_xbnf_merge_list_item(wtk_xbnf_t *b,wtk_xbnf_item_t *set_item,wtk_queue_t *set_q,wtk_xbnf_item_t *item)
{
	wtk_queue_node_t *qn1,*qn11,*qn;
	wtk_xbnf_item_list_t *list1;
	wtk_xbnf_item_t *item1;
	wtk_xbnf_item_t *item2,*item3;
	wtk_queue_t q;
	int cnt=0;
	int bcnt;
	int branched;

#ifdef DEBUG_X
	wtk_debug("===========================\n");
	wtk_xbnf_item_print(item);
	printf("\n");
#endif
	item1=item2=NULL;
	for(qn1=set_q->pop;qn1;qn1=qn1->next)
	{
		list1=data_offset(qn1,wtk_xbnf_item_list_t,q_n);
		cnt=0;
		item2=item;
		branched=0;
		for(qn11=list1->list_q.pop;qn11;qn11=qn11->next)
		{
			item1=data_offset(qn11,wtk_xbnf_item_t,q_n);
			//wtk_debug("type=[%c]\n",wtk_xbnf_item_type_get_char(item1->type));
			if(item1->type==WTK_XBNF_ITEM_STR)
			{
				if((item1->attr.min==1) && (item1->attr.max==1) && (item1->v.str==item2->v.str))
				{
					++cnt;
					if(item2->q_n.next)
					{
						item2=data_offset(item2->q_n.next,wtk_xbnf_item_t,q_n);
						if(!item1->q_n.next)
						{
							item1=NULL;
						}
					}else
					{
						if(item1->q_n.next)
						{
							item1=data_offset(item1->q_n.next,wtk_xbnf_item_t,q_n);
						}else
						{
							item1=NULL;
						}
						item2=NULL;
						break;
					}
				}else
				{
					break;
				}
			}else if(item1->type==WTK_XBNF_ITEM_BRANCH)
			{
				bcnt=wtk_xbnf_merge_list_item(b,item1,&(item1->v.set->set_q),item2);
				if(bcnt==0)
				{
					break;
				}else
				{
					cnt+=bcnt;
					branched=1;
					break;
				}
			}else
			{
				break;
			}
		}
		if(cnt>0)
		{
			//wtk_debug("item1=%p item2=%p\n",item1,item2);
			if(branched==0)
			{
				q=list1->list_q;
				wtk_queue_init(&(list1->list_q));
				while(1)
				{
					qn=wtk_queue_pop(&(q));
					if(!qn){break;}
					if(item1 && (qn==&(item1->q_n)))
					{
						item3=wtk_xbnf_new_item_branch(b);
						wtk_xbnf_add_branch(b,item3,item1,item2);
						wtk_queue_push(&(list1->list_q),&(item3->q_n));
						break;
					}else
					{
						item3=data_offset(qn,wtk_xbnf_item_t,q_n);
						wtk_queue_push(&(list1->list_q),qn);
					}
				}
				if(!item1)
				{
					item3=wtk_xbnf_new_item_branch(b);
					wtk_xbnf_add_branch(b,item3,item1,item2);
					wtk_queue_push(&(list1->list_q),&(item3->q_n));
				}
			}
			break;
		}
	}
	return cnt;
}

/*
	$expr1=[张三天|张三峰|张会|张三|张三七八|李强|李四段|李四端|Hello|张三五天];
*/
void wtk_xbnf_merge_front(wtk_xbnf_t *b,wtk_queue_t *item_q)// *item)
{
	wtk_queue_node_t *qn1;
	wtk_xbnf_item_list_t *list1;
	wtk_xbnf_item_t *item1;
	wtk_queue_t q;
	wtk_queue_t *set_q;
	wtk_queue_t str_q;
	int cnt;

	set_q=item_q;//&(item->v.set->set_q);
	q=*(set_q);
	wtk_queue_init(set_q);
	wtk_queue_init(&(str_q));
	while(1)
	{
		qn1=wtk_queue_pop(&(q));
		if(!qn1){break;}
		list1=data_offset(qn1,wtk_xbnf_item_list_t,q_n);
		if(list1->list_q.length>0)// && str_q.length>0)
		{
			item1=data_offset(list1->list_q.pop,wtk_xbnf_item_t,q_n);
			if(item1->type==WTK_XBNF_ITEM_STR && item1->attr.min==1 && item1->attr.max==1)
			{
				if(str_q.length>0)
				{
					cnt=wtk_xbnf_merge_list_item(b,item1,&(str_q),item1);
					if(cnt==0)
					{
						wtk_queue_push(&(str_q),qn1);
					}
				}else
				{
					wtk_queue_push(&(str_q),qn1);
				}
			}else
			{
				wtk_queue_push(set_q,qn1);
			}
		}else
		{
			wtk_queue_push(set_q,qn1);
		}
	}
	if(str_q.length>0)
	{
		wtk_queue_link(set_q,&(str_q));
	}
}

void wtk_xbnf_merge_pathesis(wtk_xbnf_t *b, wtk_xbnf_item_t *item)
{
	wtk_xbnf_item_list_t *list;
	wtk_queue_t *q;

	q=&(item->v.set->set_q);
	if(q->length!=1){return;}
	list=data_offset(q->pop,wtk_xbnf_item_list_t,q_n);
	if(list->list_q.length!=1){return;}
}

void wtk_xbnf_merge_set(wtk_xbnf_t *b,wtk_queue_t *set_q)
{
	if(set_q->length<2)
	{
		return;
	}
	wtk_xbnf_merge_front(b,set_q);
}

void wtk_xbnf_merge_item(wtk_xbnf_t *b,wtk_xbnf_item_t *item)
{
	if(item->type!=WTK_XBNF_ITEM_PARENTHESIS
			 && item->type!=WTK_XBNF_ITEM_BRACKET
			 && item->type!=WTK_XBNF_ITEM_BRACE
			 && item->type!=WTK_XBNF_ITEM_ANGLE)
	{
		goto end;
	}
	//wtk_xbnf_merge_pathesis(b,item);
	/*
	wtk_xbnf_item_print(item);
	printf("\n");
	*/
	wtk_xbnf_merge_set(b,&(item->v.set->set_q));
	/*
	wtk_xbnf_item_print(item);
	printf("\n");
	*/
end:
	/*
	wtk_debug("============ 1 ================\n");
	wtk_xbnf_item_print(item);
	printf("\n");
	*/
	//exit(0);
	return;
}

void wtk_xbnf_dup_lex_item(wtk_xbnf_t *b,wtk_xbnf_item_t *item,wtk_strbuf_t *buf,wtk_strbuf_t *output);
void wtk_xbnf_dup_lex_item_set(wtk_xbnf_t *b,wtk_xbnf_item_set_t *set,wtk_strbuf_t *buf,wtk_strbuf_t *output);
void wtk_xbnf_dump_lex_item_list(wtk_xbnf_t *b,wtk_xbnf_item_list_t *list,wtk_strbuf_t *buf,wtk_strbuf_t *output);


void wtk_xbnf_dump_lex_item_list(wtk_xbnf_t *b,wtk_xbnf_item_list_t *list,wtk_strbuf_t *buf,wtk_strbuf_t *output)
{
	wtk_queue_node_t *qn1;
	wtk_xbnf_item_t *item;

	for(qn1=list->list_q.pop;qn1;qn1=qn1->next)
	{
		item=data_offset(qn1,wtk_xbnf_item_t,q_n);
		//wtk_xbnf_item_print(item);
		wtk_xbnf_dup_lex_item(b,item,buf,output);
	}
}


void wtk_xbnf_dup_lex_item_set(wtk_xbnf_t *b,wtk_xbnf_item_set_t *set,wtk_strbuf_t *buf,wtk_strbuf_t *output)
{
	wtk_queue_node_t *qn;
	wtk_xbnf_item_list_t *list;
	//wtk_xbnf_item_list_t *list;

	for(qn=set->set_q.pop;qn;qn=qn->next)
	{
		list=data_offset(qn,wtk_xbnf_item_list_t,q_n);
		if(qn!=set->set_q.pop)
		{
			wtk_strbuf_push_s(buf,"|");
		}
		if(list->list_q.length>0)
		{
			wtk_xbnf_dump_lex_item_list(b,list,buf,output);
		}
	}
}


void wtk_xbnf_dup_lex_item(wtk_xbnf_t *b,wtk_xbnf_item_t *item,wtk_strbuf_t *buf,wtk_strbuf_t *output)
{
	switch(item->type)
	{
	case WTK_XBNF_ITEM_STR:
		wtk_strbuf_push(buf,item->v.str->data,item->v.str->len);
		break;
	case WTK_XBNF_ITEM_VAR:
		wtk_strbuf_push_s(buf,"${");
		wtk_strbuf_push(buf,item->v.expr->name->data,item->v.expr->name->len);
		wtk_strbuf_push_s(buf,"}");
		break;
	case WTK_XBNF_ITEM_PARENTHESIS:
		wtk_strbuf_push_s(buf,"(");
		wtk_xbnf_dup_lex_item_set(b,item->v.set,buf,output);
		wtk_strbuf_push_s(buf,")");
		break;
	case WTK_XBNF_ITEM_BRACKET:
		wtk_strbuf_push_s(buf,"(");
		wtk_xbnf_dup_lex_item_set(b,item->v.set,buf,output);
		wtk_strbuf_push_s(buf,")");
		break;
	case WTK_XBNF_ITEM_BRACE:
		wtk_strbuf_push_s(buf,"(");
		wtk_xbnf_dup_lex_item_set(b,item->v.set,buf,output);
		wtk_strbuf_push_s(buf,")");
		break;
	case WTK_XBNF_ITEM_ANGLE:
		wtk_strbuf_push_s(buf,"(");
		wtk_xbnf_dup_lex_item_set(b,item->v.set,buf,output);
		wtk_strbuf_push_s(buf,")");
		break;
	case WTK_XBNF_ITEM_BRANCH:
		wtk_strbuf_push_s(buf,"(");
		wtk_xbnf_dup_lex_item_set(b,item->v.set,buf,output);
		wtk_strbuf_push_s(buf,")");
		break;
	}
	if(item->attr.min!=1 || item->attr.max!=1)
	{
		wtk_strbuf_push_f(buf,"{%d,%d}",item->attr.min,item->attr.max);
	}
}

int wtk_xbnf_dump_lex_expr(wtk_xbnf_t *b,wtk_xbnf_expr_t *expr,wtk_strbuf_t *buf,wtk_strbuf_t *output)
{
	if(expr->name)
	{
		wtk_strbuf_push(buf,expr->name->data,expr->name->len);
		wtk_strbuf_push_s(buf,"=");
	}
	wtk_xbnf_dup_lex_item_set(b,expr->set,buf,output);
	wtk_strbuf_push_s(buf,";\n");
	wtk_debug("[%.*s]\n",buf->pos,buf->data);
	return 0;
}

int wtk_xbnf_dump_lex2(wtk_xbnf_t *b,wtk_strbuf_t *buf)
{
	wtk_queue_node_t *qn;
	wtk_xbnf_expr_t *expr;
	wtk_strbuf_t *buf2;
	int ret;

	buf2=wtk_strbuf_new(256,1);
	for(qn=b->expr_q.pop;qn;qn=qn->next)
	{
		expr=data_offset(qn,wtk_xbnf_expr_t,q_n);
		wtk_strbuf_reset(buf);
		ret=wtk_xbnf_dump_lex_expr(b,expr,buf,buf2);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	wtk_strbuf_delete(buf2);
	return ret;
}

int wtk_xbnf_dump_lex(wtk_xbnf_t *b,char *fn)
{
	int ret;
	wtk_strbuf_t *buf;

	buf=wtk_strbuf_new(1024,1);
	ret=wtk_xbnf_dump_lex2(b,buf);
	if(ret!=0){goto end;}
	file_write_buf(fn,buf->data,buf->pos);
end:
	wtk_strbuf_delete(buf);
	return ret;
}


int wtk_xbnf_rec(wtk_xbnf_t *b,char *data,int bytes)
{
    int ret, chn_wrd = 1;
    char *s, *e;
    wtk_string_t v;
    wtk_strbuf_t *buf;

    buf = b->tmp_buf;
    ret = -1;
    s = data;
    e = s + bytes;
    while (s < e) {
        s = wtk_xbnf_cfg_next_wrd(b->cfg, s, e, buf, &chn_wrd);
        wtk_string_set(&(v), buf->data, buf->pos);
        wtk_debug("[%.*s]\n", v.len, v.data);
        // if(ret!=0){goto end;}
	}
	ret=0;
//end:
	return ret;
}


void wtk_xbnf_print_expand(wtk_xbnf_t *b)
{

}
