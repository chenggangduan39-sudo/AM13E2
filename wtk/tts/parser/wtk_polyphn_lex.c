#include <ctype.h>
#include "wtk_polyphn_lex.h" 

wtk_polyphn_lex_t* wtk_polyphn_lex_new()
{
	wtk_polyphn_lex_t *l;

	l=(wtk_polyphn_lex_t*)wtk_malloc(sizeof(wtk_polyphn_lex_t));
	l->hash=wtk_str_hash_new(1047);
	return l;
}

void wtk_polyphn_lex_delete(wtk_polyphn_lex_t *l)
{
	wtk_str_hash_delete(l->hash);
	wtk_free(l);
}

wtk_polyphn_wrd_t* wtk_polyphn_new_wrd(wtk_polyphn_lex_t *l)
{
	wtk_polyphn_wrd_t *wrd;

	wrd=(wtk_polyphn_wrd_t*)wtk_heap_malloc(l->hash->heap,sizeof(wtk_polyphn_wrd_t));
	wrd->wrd=NULL;
	wtk_queue_init(&(wrd->item_q));
	return wrd;
}

void wtk_polyphn_wrd_add_item(wtk_polyphn_wrd_t *wrd,wtk_polyphn_expr_item_t *item)
{
	wtk_queue_node_t *qn;
	wtk_polyphn_expr_item_t *item2;

	//wtk_debug("len=%d\n",wrd->item_q.length);
	for(qn=wrd->item_q.push;qn;qn=qn->prev)
	{
		item2=data_offset2(qn,wtk_polyphn_expr_item_t,q_n);
		if(item2->prob>=item->prob)
		{
			wtk_queue_insert_to(&(wrd->item_q),qn,&(item->q_n));
			return;
		}else if(!qn->prev)
		{
			wtk_queue_push_front(&(wrd->item_q),&(item->q_n));
			return;
		}
	}
	wtk_queue_push(&(wrd->item_q),&(item->q_n));
}

wtk_polyphn_expr_item_t* wtk_polyphn_expr_item_new(wtk_polyphn_lex_t *l,float prob)
{
	wtk_polyphn_expr_item_t *item;

	item=(wtk_polyphn_expr_item_t*)wtk_heap_malloc(l->hash->heap,sizeof(wtk_polyphn_expr_item_t));
	item->prob=prob;
	wtk_queue_init(&(item->if_q));
	item->syl=NULL;
	item->pos=NULL;
	return item;
}


wtk_polyphn_expr_item_if_t* wtk_polyphn_new_expr_item_if(wtk_polyphn_lex_t *l)
{
	wtk_polyphn_expr_item_if_t *item;

	item=(wtk_polyphn_expr_item_if_t*)wtk_heap_malloc(l->hash->heap,sizeof(wtk_polyphn_expr_item_if_t));
	item->start=0;
	item->end=0;
	item->not=0;
	item->type=WTK_POLYPHN_EXPR_ITEM_NONE;
	item->nstr=0;
	item->strs=NULL;
//	item->idx=-1;
	return item;
}

void wtk_polyphn_expr_item_update_syl(wtk_polyphn_expr_item_t *item,wtk_heap_t *heap)
{
	wtk_tts_wrd_pron_t *pron;
	char c;
	int len;

	pron=(wtk_tts_wrd_pron_t*)wtk_heap_malloc(heap,sizeof(wtk_tts_wrd_pron_t));
	pron->next=NULL;
	pron->nsyl=1;
	pron->npron=1;
	pron->syls=(wtk_tts_syl_t*)wtk_heap_malloc(heap,sizeof(wtk_tts_syl_t));
	pron->syls->tone=0;
	pron->syls->v=NULL;
	c=item->syl->data[item->syl->len-1];
	if(isdigit(c))
	{
		pron->syls->tone=c-'0';
		len=item->syl->len-1;
	}else
	{
		len=item->syl->len;
	}
	pron->syls->v=wtk_heap_dup_string(heap,item->syl->data,len);
	item->pron=pron;
}

// support polyword by dmd at ago
void wtk_polyphn_expr_item_update_syl2(wtk_polyphn_expr_item_t *item,wtk_heap_t *heap)
{
#define MAX_PHN 50
	typedef enum
	{
		WTK_TTS_SEGWRD_INIT,
		WTK_TTS_SEGWRD_WORD,
	}wtk_syndict_state_t;
	wtk_tts_syl_t vx[MAX_PHN];
	char *s,*e,c;
	wtk_syndict_state_t state;
	wtk_string_t k;
	int ki=0;
	int tone;
	wtk_tts_wrd_pron_t *pron=NULL;
	wtk_string_set(&(k),0,0);
	s=item->syl->data;
	e=s+item->syl->len;
	state=WTK_TTS_SEGWRD_INIT;
	while(s<e)
	{
		c=*s;
		switch(state)
		{
		case WTK_TTS_SEGWRD_INIT:
			if(isalpha(c))
			{
				k.data=s;
				k.len=0;
				state=WTK_TTS_SEGWRD_WORD;
				if(s!=e-1)
				{
					break;
				}
			}
			break;
		case WTK_TTS_SEGWRD_WORD:
			if(c=='-' || s==e-1)
			{
				if(c=='-')
				{
					k.len=s-k.data;
				}else
				{
					k.len=s-k.data+1;
				}
				c=k.data[k.len-1];
				if(isdigit(c))
				{
					tone=c-'0';
					--k.len;
				}else
				{
					tone=0;
				}
				//wtk_debug("[%.*s]=%d\n",k.len,k.data,tone);
				vx[ki].v=wtk_heap_dup_string(heap,k.data,k.len);
				//wtk_debug("[%.*s]=%d\n",k.len,k.data,tone);
				vx[ki].tone=tone;
				++ki;
				if(s==e-1)
				{
					pron=(wtk_tts_wrd_pron_t*)wtk_heap_malloc(heap,sizeof(wtk_tts_wrd_pron_t));
					pron->nsyl=ki;
					//wtk_debug("ki=%d [%.*s]\n",ki,v_bytes,v);
					pron->syls=(wtk_tts_syl_t*)wtk_heap_malloc(heap,sizeof(wtk_tts_syl_t)*ki);
					pron->npron=1;
					pron->next=NULL;
					memcpy(pron->syls,vx,sizeof(wtk_tts_syl_t)*ki);
					ki=0;
				}
				state=WTK_TTS_SEGWRD_INIT;
			}
			break;
		}
		++s;
	}
	item->pron=pron;
}

void wtk_polyphn_expr_item_if_print(wtk_polyphn_expr_item_if_t *item)
{
	int i;

	printf("(%d,%d,",item->start,item->end);
	if(item->not)
	{
		printf("!");
	}
	switch(item->type)
	{
	case WTK_POLYPHN_EXPR_ITEM_NONE:
		break;
	case WTK_POLYPHN_EXPR_ITEM_C:
		printf("c");
		break;
	case WTK_POLYPHN_EXPR_ITEM_Z:
		printf("z");
		break;
	case WTK_POLYPHN_EXPR_ITEM_P:
		printf("p");
		break;
	case WTK_POLYPHN_EXPR_ITEM_I:
		printf("i");
		break;
	}
	printf(";");
	for(i=0;i<item->nstr;++i)
	{
		printf("%.*s;",item->strs[i]->len,item->strs[i]->data);
	}
	printf(")");
}
int wtk_polyphn_expr_item_if_match(wtk_polyphn_expr_item_if_t *xif,char *wrd,int wrd_bytes,char *pos,int pos_bytes)
{
	return wtk_polyphn_expr_item_if_match2(xif, wrd, wrd_bytes, pos, pos_bytes, 0,0);
}
int wtk_polyphn_expr_item_if_match2(wtk_polyphn_expr_item_if_t *xif,char *wrd,int wrd_bytes,char *pos,int pos_bytes, char *idx,int idx_bytes)
{
	int b=0;
	int i;
	int ret;

	switch(xif->type)
	{
	case WTK_POLYPHN_EXPR_ITEM_NONE:
		return 0;
		break;
	case WTK_POLYPHN_EXPR_ITEM_C:
		//c表示start->end的分词词性分别等于string 1、string 2 … 或者string n
		for(i=0;i<xif->nstr;++i)
		{
			if(wtk_string_cmp(xif->strs[i],pos,pos_bytes)==0)
			{
				b=1;
				break;
			}
		}
		break;
	case WTK_POLYPHN_EXPR_ITEM_Z:
		//z表示start->end的分词中包含单个汉字string 1、string 2 … 或者string n
		for(i=0;i<xif->nstr;++i)
		{
			ret=wtk_str_str(wrd,wrd_bytes,xif->strs[i]->data,xif->strs[i]->len);
			if(ret>=0)
			{
				b=1;
				break;
			}
		}
		break;
	case WTK_POLYPHN_EXPR_ITEM_P:
		//p表示start->end的分词结果等于string 1、string 2 … 或者string n
		for(i=0;i<xif->nstr;++i)
		{
			if(wtk_string_cmp(xif->strs[i],wrd,wrd_bytes)==0)
			{
				b=1;
				break;
			}
		}
		break;
	case WTK_POLYPHN_EXPR_ITEM_I:
		//i表示start->end的分词结果位置等于string 1、string 2 … 或者string n
		for(i=0;i<xif->nstr;++i)
		{
			if(wtk_string_cmp(xif->strs[i],idx,idx_bytes)==0)
			{
				b=1;
				break;
			}
		}
		break;
	}
	return xif->not?(b?0:1):b;
}

void wtk_polyphn_expr_item_print(wtk_polyphn_expr_item_t *item)
{
	wtk_queue_node_t *qn;
	wtk_polyphn_expr_item_if_t *xif;

	printf("%.0f?",item->prob);
	for(qn=item->if_q.pop;qn;qn=qn->next)
	{
		xif=data_offset2(qn,wtk_polyphn_expr_item_if_t,q_n);
		wtk_polyphn_expr_item_if_print(xif);
	}
	printf(":");
	if(item->syl)
	{
		printf("%.*s",item->syl->len,item->syl->data);
	}
	if(item->pos)
	{
		printf(",%.*s",item->pos->len,item->pos->data);
	}
	printf("\n");
}

void wtk_polyphn_wrd_print(wtk_polyphn_wrd_t *w)
{
	wtk_queue_node_t *qn;
	wtk_polyphn_expr_item_t *item;

	printf("[%.*s]\n",w->wrd->len,w->wrd->data);
	for(qn=w->item_q.pop;qn;qn=qn->next)
	{
		item=data_offset2(qn,wtk_polyphn_expr_item_t,q_n);
		wtk_polyphn_expr_item_print(item);
	}
}

typedef enum
{
	WTK_POLYPHN_LEX_INIT,
	WTK_POLYPHN_LEX_COMM,
	WTK_POLYPHN_LEX_WRD_WAIT,
	WTK_POLYPHN_LEX_WRD_VALUE,
	WTK_POLYPHN_LEX_WRD_WAIT_END,
	WTK_POLYPHN_LEX_WRD_WAIT_EXPR,
	WTK_POLYPHN_LEX_WRD_WAIT_EXPR_PROB,
	WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_WAIT,
	WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_START_V,
	WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_END_V,
	WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_OP_WAIT,
	WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_OP,
	WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_OP_WAIT_END,
	WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_OP_VALUE,
	WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_OP_VALUE_WAIT,
	WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_WAIT_SYL,
	WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_SYL,
	WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_POS_WAIT,
	WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_POS,
}wtk_polyphn_lex_state;

typedef struct
{
	wtk_strbuf_t *buf;
	wtk_polyphn_lex_t *lex;
	wtk_polyphn_lex_state state;
	wtk_polyphn_wrd_t* wrd;
	wtk_polyphn_expr_item_t *item;
	wtk_polyphn_expr_item_if_t *xif;
	wtk_larray_t *a;
}wtk_polyphn_lex_parser_t;

wtk_polyphn_lex_parser_t* wtk_polyphn_lex_parser_new()
{
	wtk_polyphn_lex_parser_t *p;

	p=(wtk_polyphn_lex_parser_t*)wtk_malloc(sizeof(wtk_polyphn_lex_parser_t));
	p->buf=wtk_strbuf_new(256,1);
	p->state=WTK_POLYPHN_LEX_INIT;
	p->wrd=NULL;
	p->a=wtk_larray_new(10,sizeof(void*));
	return p;
}



void wtk_polyphn_lex_parser_delete(wtk_polyphn_lex_parser_t *p)
{
	wtk_larray_delete(p->a);
	wtk_strbuf_delete(p->buf);
	wtk_free(p);
}

int wtk_polyphn_lex_parser_feed(wtk_polyphn_lex_parser_t *p,wtk_string_t *v)
{
	wtk_strbuf_t *buf=p->buf;
	float f;
	char c;

	//wtk_debug("[%.*s]\n",v->len,v->data);
	switch(p->state)
	{
	case WTK_POLYPHN_LEX_INIT:
		if(v->len==1)
		{
			c=v->data[0];
			if(c=='[')
			{
				p->state=WTK_POLYPHN_LEX_WRD_WAIT;
			}else if(c=='#')
			{
				p->state=WTK_POLYPHN_LEX_COMM;
			}
		}
		break;
	case WTK_POLYPHN_LEX_COMM:
		if(v->len==1 && v->data[0]=='\n')
		{
			p->state=WTK_POLYPHN_LEX_INIT;
		}
		break;
	case WTK_POLYPHN_LEX_WRD_WAIT:
		if(v->len>1 || !isspace(v->data[0]))
		{
//			{
//				static int ki=0;
//
//				++ki;
//				wtk_debug("v[%d]=[%.*s]\n",ki,v->len,v->data);
//			}
			wtk_strbuf_reset(buf);
			wtk_strbuf_push(buf,v->data,v->len);
			p->wrd=wtk_polyphn_new_wrd(p->lex);
			p->state=WTK_POLYPHN_LEX_WRD_VALUE;
		}
		break;
	case WTK_POLYPHN_LEX_WRD_VALUE:
		if(v->len==1 && (v->data[0]==']'||isspace(v->data[0])))
		{
			p->wrd->wrd=wtk_heap_dup_string(p->lex->hash->heap,buf->data,buf->pos);
			//wtk_debug("[%.*s]\n",buf->pos,buf->data);
			wtk_str_hash_add(p->lex->hash,p->wrd->wrd->data,p->wrd->wrd->len,p->wrd);
			p->state=WTK_POLYPHN_LEX_WRD_WAIT_END;
			if(v->data[0]==']')
			{
				return wtk_polyphn_lex_parser_feed(p,v);
			}
		}else
		{
			wtk_strbuf_push(buf,v->data,v->len);
		}
		break;
	case WTK_POLYPHN_LEX_WRD_WAIT_END:
		if(v->len==1 && v->data[0]==']')
		{
			p->state=WTK_POLYPHN_LEX_WRD_WAIT_EXPR;
		}
		break;
	case WTK_POLYPHN_LEX_WRD_WAIT_EXPR:
		if(v->len==1)
		{
			c=v->data[0];
			if(c=='[')
			{
				p->state=WTK_POLYPHN_LEX_WRD_WAIT;
			}else if(isdigit(c))
			{
				wtk_strbuf_reset(buf);
				wtk_strbuf_push(buf,v->data,v->len);
				p->state=WTK_POLYPHN_LEX_WRD_WAIT_EXPR_PROB;
			}
		}
		break;
	case WTK_POLYPHN_LEX_WRD_WAIT_EXPR_PROB:
		if(v->len==1 &&(v->data[0]=='?'))
		{
			wtk_strbuf_strip(buf);
			//wtk_debug("[%.*s]\n",buf->pos,buf->data);
			f=wtk_str_atof(buf->data,buf->pos);
			p->item=wtk_polyphn_expr_item_new(p->lex,f);
			wtk_polyphn_wrd_add_item(p->wrd,p->item);
			p->state=WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_WAIT;
		}else
		{
			wtk_strbuf_push(buf,v->data,v->len);
		}
		break;
	case WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_WAIT:
		if(v->len==1 )// && (v->data[0]=='('))
		{
			c=v->data[0];
			if(c=='(')
			{
				wtk_strbuf_reset(buf);
				p->xif=wtk_polyphn_new_expr_item_if(p->lex);
				wtk_queue_push(&(p->item->if_q),&(p->xif->q_n));
				p->state=WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_START_V;
			}else if(c==':')
			{
				p->state=WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_WAIT_SYL;
//				wtk_polyphn_expr_item_print(p->item);
//				exit(0);
			}
		}
		break;
	case WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_WAIT_SYL:
		if(v->len>1 || !isspace(v->data[0]))
		{
			wtk_strbuf_reset(buf);
			wtk_strbuf_push(buf,v->data,v->len);
			p->state=WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_SYL;
		}
		break;
	case WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_SYL:
		if(v->len==1 && v->data[0]==',')
		{
			wtk_strbuf_strip(buf);
			p->item->syl=wtk_heap_dup_string(p->lex->hash->heap,buf->data,buf->pos);
			wtk_polyphn_expr_item_update_syl2(p->item,p->lex->hash->heap);
			//wtk_polyphn_expr_item_update_syl(p->item,p->lex->hash->heap);
			wtk_strbuf_reset(buf);
			p->state=WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_POS_WAIT;
		}else
		{
			wtk_strbuf_push(buf,v->data,v->len);
		}
		break;
	case WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_POS_WAIT:
		if(v->len>1 || !isspace(v->data[0]))
		{
			p->state=WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_POS;
			wtk_strbuf_reset(buf);
			return wtk_polyphn_lex_parser_feed(p,v);
		}
		break;
	case WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_POS:
		if(v->len==1 && isspace(v->data[0]))
		{
			p->item->pos=wtk_heap_dup_string(p->lex->hash->heap,buf->data,buf->pos);
			wtk_strbuf_reset(buf);
			p->item=NULL;
			p->state=WTK_POLYPHN_LEX_WRD_WAIT_EXPR;
		}else
		{
			wtk_strbuf_push(buf,v->data,v->len);
		}
		break;
	case WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_START_V:
		if(v->len==1 && v->data[0]==',')
		{
			wtk_strbuf_strip(buf);
			f=wtk_str_atof(buf->data,buf->pos);
			p->xif->start=f;
			wtk_strbuf_reset(buf);
			p->state=WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_END_V;
		}else
		{
			wtk_strbuf_push(buf,v->data,v->len);
		}
		break;
	case WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_END_V:
		if(v->len==1 && v->data[0]==',')
		{
			wtk_strbuf_strip(buf);
			f=wtk_str_atof(buf->data,buf->pos);
			p->xif->end=f;
			wtk_strbuf_reset(buf);
			p->state=WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_OP_WAIT;
		}else
		{
			wtk_strbuf_push(buf,v->data,v->len);
		}
		break;
	case WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_OP_WAIT:
		if(v->len==1)
		{
			c=v->data[0];
			if(c=='!')
			{
				p->xif->not=1;
			}else if(!isspace(c))
			{
				p->state=WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_OP;
				return wtk_polyphn_lex_parser_feed(p,v);
			}
		}
		break;
	case WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_OP:
		if(v->len==1)
		{
			c=v->data[0];
			switch(c)
			{
			case 'c':
				p->xif->type=WTK_POLYPHN_EXPR_ITEM_C;
				p->state=WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_OP_WAIT_END;
				break;
			case 'p':
				p->xif->type=WTK_POLYPHN_EXPR_ITEM_P;
				p->state=WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_OP_WAIT_END;
				break;
			case 'z':
				p->xif->type=WTK_POLYPHN_EXPR_ITEM_Z;
				p->state=WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_OP_WAIT_END;
				break;
//			case 'i':  // index of word in sentence position add by dmd
//				p->xif->type=WTK_POLYPHN_EXPR_ITEM_I;
//				p->state=WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_OP_WAIT_END;
//				break;
			default:
				break;
			}
		}
		break;
	case WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_OP_WAIT_END:
		if(v->len==1 && v->data[0]==';')
		{
			wtk_larray_reset(p->a);
			wtk_strbuf_reset(buf);
			p->state=WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_OP_VALUE;
		}
		break;
	case WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_OP_VALUE:
		if(v->len==1 &&(v->data[0]==';'))
		{
			wtk_strbuf_strip(buf);
			v=wtk_heap_dup_string(p->lex->hash->heap,buf->data,buf->pos);
			//wtk_larray_push()
			wtk_larray_push2(p->a,&(v));
			p->state=WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_OP_VALUE_WAIT;
		}else
		{
			wtk_strbuf_push(buf,v->data,v->len);
		}
		break;
	case WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_OP_VALUE_WAIT:
		if(v->len>1 || !isspace(v->data[0]))
		{
			if(v->len==1 && v->data[0]==')')
			{
				p->xif->nstr=p->a->nslot;
				p->xif->strs=(wtk_string_t**)wtk_heap_malloc(p->lex->hash->heap,sizeof(wtk_string_t*)*p->xif->nstr);
				memcpy(p->xif->strs,p->a->slot,sizeof(wtk_string_t*)*p->xif->nstr);
				wtk_larray_reset(p->a);
				//wtk_polyphn_expr_item_if_print(p->xif);
				p->state=WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_WAIT;
			}else
			{
				wtk_strbuf_reset(buf);
				p->state=WTK_POLYPHN_LEX_WRD_WAIT_EXPR_ITEM_OP_VALUE;
				return wtk_polyphn_lex_parser_feed(p,v);
			}
		}
		break;
	}
	return 0;
}

int wtk_polyphn_lex_load2(wtk_polyphn_lex_t *l,wtk_source_t *src)
{
	wtk_strbuf_t *buf;
	wtk_string_t v;
	int ret;
	wtk_polyphn_lex_parser_t *p;

	p=wtk_polyphn_lex_parser_new();
	p->lex=l;
	buf=wtk_strbuf_new(256,1);
	while(1)
	{
		ret=wtk_source_read_utf8_char(src,buf);
		if(ret!=0){ret=0;break;}
		wtk_string_set(&(v),buf->data,buf->pos);
		ret=wtk_polyphn_lex_parser_feed(p,&v);
		if(ret!=0){goto end;}
	}
	wtk_string_set_s(&(v),"\n");
	ret=wtk_polyphn_lex_parser_feed(p,&v);
	if(ret!=0){goto end;}
	ret=0;
end:
	wtk_polyphn_lex_parser_delete(p);
	wtk_strbuf_delete(buf);
	return ret;
}

int wtk_polyphn_lex_load(wtk_polyphn_lex_t *l,char *fn)
{
	return wtk_source_load_file(l,(wtk_source_load_handler_t)wtk_polyphn_lex_load2,fn);
}


wtk_polyphn_wrd_t* wtk_polyphn_lex_find(wtk_polyphn_lex_t *l,char *data,int bytes)
{
	return (wtk_polyphn_wrd_t*)wtk_str_hash_find(l->hash,data,bytes);
}
