#include "wtk_ebnf.h"
#include <ctype.h>
#include <stdlib.h>
typedef struct wtk_ebnftok wtk_ebnftok_t;
int wtk_ebnftok_get_ident(wtk_ebnftok_t *tok);
int wtk_ebnf_pexpr(wtk_ebnf_t *ebnf,wtk_ebnftok_t* tok,wtk_enode_t** head,wtk_enode_t** tail);

typedef enum
{
	NAMESYM=0, VARSYM, VARATSYM, LPARSYM, RPARSYM, LBRACESYM,
	             RBRACESYM, LANGSYM, RANGSYM, LBRAKSYM, RBRAKSYM,
	             LTRISYM,RTRISYM,EQSYM, SEMISYM, BARSYM, PERCENTSYM,
	             EOFSYM
}wtk_ebnfsym_t;

struct wtk_ebnftok
{
	char *data;
	int len;
	int pos;
	wtk_ebnfsym_t sym;
	wtk_strbuf_t *buf;
};

#ifndef USE_TOK_MAC
#define wtk_ebnftok_getch(tok) ((tok)->pos<(tok)->len)?((tok)->data[(tok)->pos++]):EOF
#define wtk_ebnftok_unget(tok,ch) (--(tok)->pos)
#else
char wtk_ebnftok_getch(wtk_ebnftok_t *tok)
{
	return (tok->pos<tok->len)?(tok->data[tok->pos++]):EOF;
}
void wtk_ebnftok_unget(wtk_ebnftok_t* tok,char ch)
{
	--tok->pos;
}
#endif


void wtk_ebnftok_print(wtk_ebnftok_t* tok)
{
	char* buf[]={
			"NAMESYM","VARSYM", "VARATSYM", "LPARSYM", "RPARSYM", "LBRACESYM",
			"RBRACESYM", "LANGSYM", "RANGSYM", "LBRAKSYM", "RBRAKSYM","LTRISYM",
			"RTRISYM","EQSYM", "SEMISYM", "BARSYM", "PERCENTSYM","EOFSYM"
	};
	printf("sym:\t%s\n",buf[tok->sym]);
	printf("buf:\t%*.*s\n",tok->buf->pos,tok->buf->pos,tok->buf->data);
}

int wtk_ebnftok_get_sym(wtk_ebnftok_t *tok)
{
	char ch;
	int ret=0;

	do
	{
		ch=wtk_ebnftok_getch(tok);
	}while(isspace((int)ch));
	wtk_strbuf_reset(tok->buf);
	switch((int)ch)
	{
	case '$':
		tok->sym=VARSYM;
		ret=wtk_ebnftok_get_ident(tok);
		break;
	case '(':
		tok->sym=LPARSYM;
		break;
	case ')':
		tok->sym=RPARSYM;
		break;
	case '<':
		ch=wtk_ebnftok_getch(tok);
		if(ch==EOF){ret=-1;break;}
		if(ch=='<')
		{
			tok->sym=LTRISYM;
		}else
		{
			wtk_ebnftok_unget(tok,ch);
			tok->sym=LANGSYM;
		}
		break;
	case '>':
		ch=wtk_ebnftok_getch(tok);
		if(ch==EOF){ret=-1;break;}
		if(ch=='>')
		{
			tok->sym=RTRISYM;
		}else
		{
			wtk_ebnftok_unget(tok,ch);
			tok->sym=RANGSYM;
		}
		break;
	case '{':
		tok->sym=LBRACESYM;
		break;
	case '}':
		tok->sym=RBRACESYM;
		break;
	case '[':
		tok->sym=LBRAKSYM;
		break;
	case ']':
		tok->sym=RBRAKSYM;
		break;
	case '=':
		tok->sym=EQSYM;
		break;
	case ';':
		tok->sym=SEMISYM;
		break;
	case '|':
		tok->sym=BARSYM;
		break;
	case '%':
		tok->sym=PERCENTSYM;
		break;
	case EOF:
		tok->sym=EOFSYM;
		break;
	default:
		tok->sym=NAMESYM;
		wtk_ebnftok_unget(tok,ch);
		ret=wtk_ebnftok_get_ident(tok);
		break;
	}
	//wtk_ebnftok_print(tok);
	return ret;
}

int wtk_ebnftok_get_ident(wtk_ebnftok_t *tok)
{
	char ch;

	wtk_strbuf_reset(tok->buf);
	ch=wtk_ebnftok_getch(tok);
	do
	{
		if(ch=='\\')
		{
			ch=wtk_ebnftok_getch(tok);
		}
		wtk_strbuf_push(tok->buf,&ch,1);
		ch=wtk_ebnftok_getch(tok);
	}while( !isspace((int) ch) && ch!='{' && ch!='}' && ch!='[' && ch!=']' &&
            ch!='<' && ch!='>' && ch!='(' && ch!=')' && ch!='=' &&
            ch!=';' && ch!='|' && ch!='/' && ch!='%' &&ch!=EOF);
	if(ch!=EOF)
	{
		wtk_ebnftok_unget(tok,ch);
	}
	return (ch==EOF)?-1:0;
}

void wtk_enode_print(wtk_enode_t *n,int chain);

void wtk_enodeset_print(wtk_enodeset_t *set,char* suf)
{
	wtk_enode_t **p;
	int i;

	p=(wtk_enode_t**)set->link_array->slot;
	for(i=0;i<set->link_array->nslot;++i)
	{
		printf("%s[%d/%d]:\t",suf,i,set->link_array->nslot);
		if(p[i]->name)
		{
			printf("%*.*s",p[i]->name->name->len,p[i]->name->name->len,p[i]->name->name->data);
		}else
		{
			printf("NULL");
		}
		printf("(%p)\n",p[i]);
	}
	/*
	for(i=0;i<set->link_array->nslot;++i)
	{
		wtk_enode_print(p[i]);
	}
	*/
}

void wtk_enode_print(wtk_enode_t *n,int chain)
{
	static int i=0;

	printf("########## enode: [%d,%p] ############\n",++i,n);
	if(n->name)
	{
		printf("name:\t%*.*s\n",n->name->name->len,n->name->name->len,n->name->name->data);
	}else
	{
		printf("name:\tNULL\n");
	}
	if(n->ext_name)
	{
		printf("ext name:\t%*.*s\n",n->ext_name->name->len,n->ext_name->name->len,n->ext_name->name->data);
	}else
	{
		printf("ext name:\tNULL\n");
	}
	wtk_enodeset_print(n->succ,"succ");
	wtk_enodeset_print(n->pred,"pred");
	if(n->chain && n->chain->name)
	{
		printf("chain:\t%*.*s\n",n->chain->name->name->len,n->chain->name->name->len,n->chain->name->name->data);
	}else
	{
		printf("chain:\t\tNULL\n");
	}
	if(n->chain && chain)
	{
		wtk_enode_print(n->chain,chain);
	}
}

void wtk_enodeset_free(wtk_enodeset_t *p)
{
	if(p->n_use>1)
	{
		--p->n_use;
	}
}

int wtk_enodeset_not_linked(wtk_enodeset_t* s,wtk_enode_t *x)
{
	int i;
	wtk_enode_t **p;

	p=(wtk_enode_t**)s->link_array->slot;
	for(i=0;i<s->link_array->nslot;++i)
	{
		if(x==p[i])
		{
			return 0;
		}
	}
	return 1;
}

void wtk_enode_head_merge(wtk_enode_t *a,wtk_enode_t *b)
{
	wtk_enode_t *sNode;
	wtk_enodeset_t *shared,*asucc;

	asucc=a->succ;
	//assume all succs share same link set
	sNode=((wtk_enode_t**)asucc->link_array->slot)[0];
	shared=sNode->pred;++shared->n_use;
	wtk_enodeset_free(b->pred);
	b->pred=shared;
	if(wtk_enodeset_not_linked(asucc,b))
	{
		((wtk_enode_t**)wtk_array_push(asucc->link_array))[0]=b;
	}
}

void wtk_enode_tail_merge(wtk_enode_t *a,wtk_enode_t *b)
{
	wtk_enode_t *pNode;
	wtk_enodeset_t *shared,*bpred;

	bpred=b->pred;
	pNode=((wtk_enode_t**)bpred->link_array->slot)[0];
	//assume all preds share same link set
	shared=pNode->succ; ++shared->n_use;
	wtk_enodeset_free(a->succ);a->succ=shared;
	if(wtk_enodeset_not_linked(bpred,a))
	{
		((wtk_enode_t**)wtk_array_push(bpred->link_array))[0]=a;
	}
}

void wtk_enode_join(wtk_enode_t *a,wtk_enode_t *b)
{
	wtk_enodeset_t *asucc,*bpred;

	asucc=a->succ;bpred=b->pred;
	if(wtk_enodeset_not_linked(asucc,b))
	{
		((wtk_enode_t**)wtk_array_push(asucc->link_array))[0]=b;
		if(asucc->n_use<0)
		{
			asucc->n_use=-asucc->n_use;
		}
	}
	if(wtk_enodeset_not_linked(bpred,a))
	{
		((wtk_enode_t**)wtk_array_push(bpred->link_array))[0]=a;
		if(bpred->n_use<0)
		{
			bpred->n_use=-bpred->n_use;
		}
	}
}

void wtk_enode_delete_link(wtk_enode_t *x,wtk_enodeset_t *es)
{
	wtk_enode_t **p=(wtk_enode_t**)es->link_array->slot;
	int found=0;
	int i,j,n;

	n=es->link_array->nslot;
	for(i=0;i<n;++i)
	{
		if(p[i]==x)
		{
			found=1;break;
		}
	}
	if(found)
	{
		n-=1;
		--es->link_array->nslot;
		for(j=i;j<n;++j)
		{
			p[j]=p[j+1];
		}
	}
}

//determine if a blue node should be compacted.
int wtk_enode_can_compact(wtk_enode_t *n)
{
	//compact if n either all succs are non-glue or all preds are non-glue
	//or only 1 succ and one pred (both glue) or no succs or no pred.
	wtk_enode_t **p;
	int ok=1,i;

	for(i=0,p=(wtk_enode_t**)n->succ->link_array->slot;(i<n->succ->link_array->nslot) && ok;++i)
	{
		ok=p[i]->name!=0;
	}
	if(ok==0)
	{
		ok=1;
		for(i=0,p=(wtk_enode_t**)n->pred->link_array->slot;(i<n->pred->link_array->nslot)&&ok;++i)
		{
			ok=p[i]->name!=0;
		}
	}
	if(ok==0)
	{
		ok=(n->pred->link_array->nslot==1) &&(n->succ->link_array->nslot==1);
	}
	return ok;
}

int wtk_enode_chains(wtk_enode_t *chain)
{
	int n=0;

	while(chain)
	{
		++n;
		chain=chain->chain;
	}
	return n;
}

wtk_ebnf_t* wtk_ebnf_new(wtk_eos_t *os,wtk_dict_t *d,wtk_dict_word_find_f gwh,void *data)
{
	wtk_ebnf_t *e;

	e=(wtk_ebnf_t*)malloc(sizeof(*e));
	wtk_ebnf_init(e,os,d,gwh,data);
	return e;
}

int wtk_ebnf_delete(wtk_ebnf_t *e)
{
	wtk_ebnf_clean(e);
	free(e);
	return 0;
}

int wtk_ebnf_init(wtk_ebnf_t *ebnf,wtk_eos_t *os,wtk_dict_t *dict,wtk_dict_word_find_f gwh,void *data)
{
	wtk_label_init(&(ebnf->label),133);
	ebnf->heap=ebnf->label.heap;//wtk_heap_new(4096);
	ebnf->link_chunk_size=3;
	ebnf->os=os;
	ebnf->dict=dict;
	ebnf->gwh=gwh;
	ebnf->gwh_data=data;
	ebnf->lat=0;
	wtk_ebnf_reset(ebnf);
	return 0;
}

int wtk_ebnf_clean(wtk_ebnf_t *ebnf)
{
	if(ebnf->lat)
	{
		wtk_lat_clean(ebnf->lat);
	}
	wtk_label_clean(&(ebnf->label));
	//wtk_heap_delete(ebnf->heap);
	return 0;
}

int wtk_ebnf_reset(wtk_ebnf_t *ebnf)
{
	if(ebnf->lat)
	{
		wtk_lat_clean(ebnf->lat);
	}
	wtk_label_reset(&(ebnf->label));
	ebnf->sub_net_id=wtk_label_find_s(&(ebnf->label),"$$$EBNF_SubNet",1);
	ebnf->enter_id=wtk_label_find_s(&(ebnf->label),"$$$EBNF_ENTER",1);
	ebnf->exit_id=wtk_label_find_s(&(ebnf->label),"$$$EBNF_EXIT",1);
	ebnf->enter_exit_id=wtk_label_find_s(&(ebnf->label),"$$$EBNF_ENTEREXIT",1);
	ebnf->sub_list=0;
	ebnf->chain=0;
	ebnf->lat=0;
	return 0;
}

wtk_enodeset_t *wtk_ebnf_new_nodeset(wtk_ebnf_t* ebnf,int size)
{
	wtk_enodeset_t* s;
	wtk_heap_t *heap=ebnf->heap;

	s=(wtk_enodeset_t*)wtk_heap_malloc(heap,sizeof(*s));
	size=max(size,ebnf->link_chunk_size);
	s->link_array=wtk_array_new_h(heap,size,sizeof(wtk_enode_t*));
	//wtk_heap_print(heap);
	s->user=0;
	s->n_use=1;
	return s;
}

wtk_name_t* wtk_ebnf_get_str(wtk_ebnf_t *ebnf,wtk_ebnftok_t* tok)
{
	return 	wtk_label_find(&(ebnf->label),tok->buf->data,tok->buf->pos,1);
}

wtk_name_t* wtk_ebnf_get_name(wtk_ebnf_t *ebnf,char *data,int len)
{
	return 	wtk_label_find(&(ebnf->label),data,len,1);
}

wtk_enode_t* wtk_ebnf_new_node2(wtk_ebnf_t *ebnf,wtk_name_t *name,wtk_enode_t **chain,int max_suc,int max_pre)
{
	wtk_enode_t *p;

	p=(wtk_enode_t*)wtk_heap_malloc(ebnf->heap,sizeof(*p));
	p->name=p->ext_name=name;
	p->succ=(max_suc>0)?wtk_ebnf_new_nodeset(ebnf,max_suc):0;
	p->pred=(max_pre>0)?wtk_ebnf_new_nodeset(ebnf,max_pre):0;
	p->chain=*chain;*chain=p;
	return p;
}

wtk_enode_t* wtk_ebnf_new_node(wtk_ebnf_t *ebnf,wtk_name_t *name)
{
	return wtk_ebnf_new_node2(ebnf,name,&(ebnf->chain),ebnf->link_chunk_size,ebnf->link_chunk_size);
}

void wtk_ebnf_delete_node(wtk_ebnf_t *ebnf,wtk_enode_t *p)
{
	//wtk_debug("%p\n",p);
	wtk_enodeset_free(p->succ);
	wtk_enodeset_free(p->pred);
}

int wtk_ebnf_pmodel(wtk_ebnf_t *ebnf,wtk_ebnftok_t* tok,wtk_enode_t **head,wtk_enode_t **tail)
{
	wtk_enode_t *p;
	wtk_name_t *name;
	int ret;

	name=wtk_ebnf_get_str(ebnf,tok);
	p=wtk_ebnf_new_node(ebnf,name);//,&(ebnf->chain),ebnf->link_chunk_size,ebnf->link_chunk_size);
	*head=*tail=p;
	ret=wtk_ebnftok_get_sym(tok);
	if(ret!=0){goto end;}
	if(tok->sym==PERCENTSYM)
	{
		ret=wtk_ebnftok_get_sym(tok);
		if(ret!=0 ||(tok->sym!=NAMESYM && tok->sym !=PERCENTSYM)){ret=-1;goto end;}
		if(tok->sym==PERCENTSYM)
		{
			p->ext_name=0;
		}else
		{
			p->ext_name=wtk_ebnf_get_str(ebnf,tok);
		}
		ret=wtk_ebnftok_get_sym(tok);
		if(ret!=0){goto end;}
	}
end:
	return ret;
}

int wtk_ebnf_pvar(wtk_ebnf_t *ebnf,wtk_ebnftok_t* tok,wtk_enode_t **head,wtk_enode_t **tail)
{
	wtk_enode_t *p;
	wtk_name_t *name;

	name=wtk_ebnf_get_str(ebnf,tok);
	p=wtk_ebnf_new_node(ebnf,name);//,&(ebnf->chain),ebnf->link_chunk_size,ebnf->link_chunk_size);
	p->ext_name=ebnf->sub_net_id;
	*head=*tail=p;
	return wtk_ebnftok_get_sym(tok);
}

int wtk_ebnf_pgroup(wtk_ebnf_t *ebnf,wtk_ebnftok_t* tok,wtk_enode_t **head,wtk_enode_t **tail)
{
	int ret;

	ret=wtk_ebnftok_get_sym(tok);
	if(ret!=0){goto end;}
	ret=wtk_ebnf_pexpr(ebnf,tok,head,tail);
	if(ret!=0 || tok->sym!=RPARSYM){ret=-1;goto end;}
	ret=wtk_ebnftok_get_sym(tok);
end:
	return ret;
}

int wtk_ebnf_poption(wtk_ebnf_t *ebnf,wtk_ebnftok_t* tok,wtk_enode_t **head,wtk_enode_t **tail)
{
	wtk_enode_t *et;
	int ret;

	ret=wtk_ebnftok_get_sym(tok);
	if(ret!=0){goto end;}
	ret=wtk_ebnf_pexpr(ebnf,tok,head,&et);
	if(ret!=0){goto end;}
	*tail=wtk_ebnf_new_node(ebnf,0);
	wtk_enode_join(*head,*tail);
	wtk_enode_join(et,*tail);
	if(tok->sym!=RBRAKSYM){ret=-1;goto end;}
	ret=wtk_ebnftok_get_sym(tok);
end:
	return ret;
}

// PRepetition0: parse {..}
int wtk_ebnf_prepeat0(wtk_ebnf_t *ebnf,wtk_ebnftok_t* tok,wtk_enode_t **head,wtk_enode_t **tail)
{
	wtk_enode_t *eHead,*eTail;
	int ret;

	ret=wtk_ebnftok_get_sym(tok);
	if(ret!=0){goto end;}
	ret=wtk_ebnf_pexpr(ebnf,tok,&eHead,&eTail);
	if(ret!=0 || tok->sym!=RBRACESYM){ret=-1;goto end;}
	*head=*tail=wtk_ebnf_new_node(ebnf,0);
	wtk_enode_join(*tail,eHead);
	wtk_enode_join(eTail,*tail);
	ret=wtk_ebnftok_get_sym(tok);
end:
	return ret;
}

// PRepetition1: parse <..>
int wtk_ebnf_prepeat1(wtk_ebnf_t *ebnf,wtk_ebnftok_t* tok,wtk_enode_t **head,wtk_enode_t **tail)
{
	int ret;

	ret=wtk_ebnftok_get_sym(tok);
	if(ret!=0){goto end;}
	ret=wtk_ebnf_pexpr(ebnf,tok,head,tail);
	wtk_enode_join(*tail,*head);
	if(ret!=0 || tok->sym!=RANGSYM){ret=-1;goto end;}
	ret=wtk_ebnftok_get_sym(tok);
end:
	return ret;
}

//parser << ... >>
int wtk_ebnf_ptriloop(wtk_ebnf_t *ebnf,wtk_ebnftok_t* tok,wtk_enode_t **head,wtk_enode_t **tail)
{
	return -1;
}

int wtk_ebnf_pfactor(wtk_ebnf_t *ebnf,wtk_ebnftok_t* tok,wtk_enode_t **head,wtk_enode_t **tail)
{
	int ret=0;

	switch(tok->sym)
	{
	case NAMESYM:
		ret=wtk_ebnf_pmodel(ebnf,tok,head,tail);
		break;
	case VARSYM:
		ret=wtk_ebnf_pvar(ebnf,tok,head,tail);
		break;
	case LPARSYM:
		ret=wtk_ebnf_pgroup(ebnf,tok,head,tail);
		break;
	case LBRAKSYM:
		ret=wtk_ebnf_poption(ebnf,tok,head,tail);
		break;
	case LBRACESYM:
		ret=wtk_ebnf_prepeat0(ebnf,tok,head,tail);
		break;
	case LANGSYM:
		ret=wtk_ebnf_prepeat1(ebnf,tok,head,tail);
		break;
	case LTRISYM:
		ret=wtk_ebnf_ptriloop(ebnf,tok,head,tail);
		break;
	default:
		ret=-1;
		break;
	}
	return ret;
}

int wtk_ebnf_psequence(wtk_ebnf_t *ebnf,wtk_ebnftok_t* tok,wtk_enode_t **head,
		wtk_enode_t **tail)
{
	wtk_enode_t *hd2,*tl2;
	int ret;

	ret=wtk_ebnf_pfactor(ebnf,tok,head,tail);
	if(ret!=0){goto end;}
	while(tok->sym==NAMESYM ||tok->sym==VARSYM ||tok->sym==LBRAKSYM||
			tok->sym==LBRACESYM||tok->sym==LPARSYM||tok->sym==LTRISYM||tok->sym==LANGSYM)
	{
		hd2=tl2=0;
		ret=wtk_ebnf_pfactor(ebnf,tok,&hd2,&tl2);
		if(ret!=0){goto end;}
		wtk_enode_join(*tail,hd2);
		*tail=tl2;
	}
	if((*head)->pred->link_array->nslot!=0)
	{
		hd2=*head;
		*head=wtk_ebnf_new_node(ebnf,0);//,&(ebnf->chain),ebnf->link_chunk_size,ebnf->link_chunk_size);
		wtk_enode_join(*head,hd2);
	}
	if((*tail)->succ->link_array->nslot!=0)
	{
		tl2=*tail;
		*tail=wtk_ebnf_new_node(ebnf,0);//,&(ebnf->chain),ebnf->link_chunk_size,ebnf->link_chunk_size);
		wtk_enode_join(tl2,*tail);
	}
end:
	return ret;
}


int wtk_ebnf_pexpr(wtk_ebnf_t *ebnf,wtk_ebnftok_t* tok,wtk_enode_t** head,wtk_enode_t** tail)
{
	wtk_enode_t *head2,*tail2;
	int ret;

	ret=wtk_ebnf_psequence(ebnf,tok,head,tail);
	if(ret!=0){goto end;}
	head2=*head;tail2=*tail;
	*head=wtk_ebnf_new_node(ebnf,0);//,&(ebnf->chain),ebnf->link_chunk_size,ebnf->link_chunk_size);
	*tail=wtk_ebnf_new_node(ebnf,0);//,&(ebnf->chain),ebnf->link_chunk_size,ebnf->link_chunk_size);
	wtk_enode_join(*head,head2);
	wtk_enode_join(tail2,*tail);
	while(tok->sym==BARSYM)
	{
		ret=wtk_ebnftok_get_sym(tok);
		if(ret!=0){goto end;}
		ret=wtk_ebnf_psequence(ebnf,tok,&head2,&tail2);
		if(ret!=0){goto end;}
		wtk_enode_head_merge(*head,head2);
		wtk_enode_tail_merge(tail2,*tail);
	}
end:
	return ret;
}

// add all links from from to to.
wtk_enodeset_t* wtk_ebnf_merge_links(wtk_ebnf_t *ebnf,wtk_enodeset_t* from,wtk_enodeset_t* to)
{
	wtk_enode_t **p,**p2;
	int i,n;

	n=from->link_array->nslot;
	p=(wtk_enode_t**)from->link_array->slot;
	for(i=0;i<n;++i)
	{
		wtk_enode_delete_link(p[i],to);
	}
	p2=((wtk_enode_t**)wtk_array_push_n(to->link_array,n));
	for(i=0;i<n;++i)
	{
		p2[i]=p[i];
	}
	return to;
}

//move all the links from/to node p to its neighbours
void wtk_ebnf_compact_node(wtk_ebnf_t *ebnf,wtk_enode_t *n)
{
	wtk_enode_t *pre,*suc,*n2;
	wtk_enode_t **p;
	wtk_enodeset_t *oldls;
	int i,j;

	wtk_enode_delete_link(n,n->succ);
	p=(wtk_enode_t**)n->pred->link_array->slot;
	//process the successors of the predecessors nodes.
	//wtk_enode_print(n,0);
	for(i=0;i<n->pred->link_array->nslot;++i)
	{
		pre=p[i];
		if(pre==n){continue;}
		//wtk_enode_print(pre,0);
		if(pre->succ->link_array->nslot==1)
		{
			wtk_enodeset_free(pre->succ);
			pre->succ=n->succ;
			++n->succ->n_use;
			//wtk_debug("suc0\n");
			//exit(0);
		}else
		{
			if(pre->succ->n_use>1)
			{
				oldls=pre->succ;
				wtk_enode_delete_link(n,pre->succ);
				pre->succ=wtk_ebnf_merge_links(ebnf,n->succ,pre->succ);
				//wtk_debug("x=%d\n",pre->succ->link_array->nslot);
				//exit(0);
				pre->succ->n_use=-oldls->n_use;
				//p2=(wtk_enode_t**)n->pred->link_array->slot;
				for(j=0;j<n->pred->link_array->nslot;++j)
				{
					n2=p[j];
					if(n2->succ==oldls)
					{
						n2->succ=pre->succ;
					}
				}
			}else if(pre->succ->n_use==1)
			{
				wtk_enode_delete_link(n,pre->succ);
				pre->succ=wtk_ebnf_merge_links(ebnf,n->succ,pre->succ);
				//wtk_debug("y=%d\n",pre->succ->link_array->nslot);
				//exit(0);
			}
		}
		//wtk_enode_print(pre,0);
	}
	for(i=0;i<n->pred->link_array->nslot;++i)
	{
		if(p[i]->succ->n_use<0)
		{
			p[i]->succ->n_use=-p[i]->succ->n_use;
		}
	}
	wtk_enode_delete_link(n,n->pred);
	p=(wtk_enode_t**)n->succ->link_array->slot;
	//process predecessors of the successor.
	for(i=0;i<n->succ->link_array->nslot;++i)
	{
		suc=p[i];
		//wtk_enode_print(suc,0);
		if(suc->pred->link_array->nslot==1)
		{
			wtk_enodeset_free(suc->pred);
			suc->pred=n->pred;
			++suc->pred->n_use;
			//wtk_debug("pre0\n");
			//exit(0);
		}else
		{
			if(suc->pred->n_use>1)
			{
				oldls=suc->pred;
				wtk_enode_delete_link(n,suc->pred);
				suc->pred=wtk_ebnf_merge_links(ebnf,n->pred,suc->pred);
				suc->pred->n_use=-oldls->n_use;
				//wtk_debug("x2=%d\n",suc->pred->link_array->nslot);
				//exit(0);
				for(j=0;j<n->succ->link_array->nslot;++j)
				{
					n2=p[j];
					if(n2->pred==oldls)
					{
						n2->pred=suc->pred;
					}
				}
			}else if(suc->pred->n_use==1)
			{
				wtk_enode_delete_link(n,suc->pred);
				suc->pred=wtk_ebnf_merge_links(ebnf,n->pred,suc->pred);
				//wtk_debug("y2=%d\n",suc->pred->link_array->nslot);
				//exit(0);
			}
		}
		//wtk_enode_print(suc,0);
	}
	for(i=0;i<n->succ->link_array->nslot;++i)
	{
		if(p[i]->pred->n_use<0)
		{
			p[i]->pred->n_use=-p[i]->pred->n_use;
		}
	}
}

void wtk_ebnf_remove_glue(wtk_ebnf_t *ebnf,wtk_enk_t* nk)
{
	wtk_enode_t *p,*q;
	int glue_left=0;
	int changed=0,remove_all=0,remove_dp;

	if(!nk->enter->name){nk->enter->name=ebnf->enter_exit_id;}
	if(!nk->exit->name){nk->exit->name=ebnf->enter_exit_id;}
	do
	{
		if(glue_left>0 && (!changed))
		{
			remove_all=1;
		}
		glue_left=0;changed=0;
		p=nk->chain;nk->chain=0;
		while(p)
		{
			remove_dp=0;
			if(!p->name)
			{
				if(remove_all || wtk_enode_can_compact(p))
				{
					//wtk_enode_print(p,0);
					wtk_ebnf_compact_node(ebnf,p);
					//exit(0);
					q=p;p=p->chain;
					wtk_ebnf_delete_node(ebnf,q);
					remove_dp=1;changed=1;
				}else
				{
					++glue_left;
				}
			}
			if(!remove_dp)
			{
				q=p->chain;
				p->chain=nk->chain;
				nk->chain=p;
				p=q;
			}
		}
	}while(glue_left!=0);
	if(nk->enter->name==ebnf->enter_exit_id){nk->enter->name=0;}
	if(nk->exit->name==ebnf->enter_exit_id){nk->exit->name=0;}
}

int wtk_ebnf_define_subnet(wtk_ebnf_t* ebnf,wtk_name_t *name,
		wtk_enode_t* enter,wtk_enode_t *exit,wtk_enode_t *chain)
{
	wtk_heap_t *heap=ebnf->heap;
	wtk_subnet_t *p;

	p=(wtk_subnet_t*)wtk_heap_malloc(heap,sizeof(*p));
	p->name=name;
	p->network.enter=enter;
	p->network.exit=exit;
	p->network.chain=chain;
	wtk_ebnf_remove_glue(ebnf,&(p->network));
	p->next=ebnf->sub_list;
	ebnf->sub_list=p;
	name->data=&(p->network);
	return 0;
}

int wtk_ebnf_subnet(wtk_ebnf_t *ebnf,wtk_ebnftok_t* tok)
{
	wtk_enode_t *head,*tail;
	wtk_name_t *var;
	int ret=-1;

	if(tok->sym!=VARSYM){goto end;}
	var=wtk_ebnf_get_str(ebnf,tok);//wtk_label_find(&(ebnf->label),tok->buf->data,tok->buf->pos,1)->name;
	ret=wtk_ebnftok_get_sym(tok);
	if(ret!=0 || tok->sym!=EQSYM){ret=-1;goto end;}
	ret=wtk_ebnftok_get_sym(tok);
	if(ret!=0){goto end;}
	ebnf->chain=0;
	ret=wtk_ebnf_pexpr(ebnf,tok,&head,&tail);
	if(ret!=0 || tok->sym!=SEMISYM){ret=-1;goto end;}
	ret=wtk_ebnftok_get_sym(tok);
	if(ret!=0){goto end;}
	ret=wtk_ebnf_define_subnet(ebnf,var,head,tail,ebnf->chain);
end:
	//wtk_enode_print(head);
	return ret;
}

int wtk_ebnf_copy_nk(wtk_ebnf_t *ebnf,wtk_enk_t *nk,wtk_enk_t *dst)
{
	wtk_enode_t *p=nk->chain,*q,*p2,*q2;
	wtk_enode_t **pp1,**pp2;
	int i,n;

	dst->chain=0;
	//wtk_debug("%p\n",p);
	//exit(0);
	while(p)
	{
		q=wtk_ebnf_new_node2(ebnf,p->name,&dst->chain,0,0);
		q->ext_name=p->ext_name;
		p->user=q;
		p=p->chain;
	}
	p=nk->chain;
	while(p)
	{
		q=(wtk_enode_t*)p->user;
		if(!q->succ && p->succ)
		{
			n=p->succ->link_array->nslot;
			q->succ=wtk_ebnf_new_nodeset(ebnf,n);
			pp1=(wtk_enode_t**)p->succ->link_array->slot;
			pp2=(wtk_enode_t**)wtk_array_push_n(q->succ->link_array,n);//q->succ->link_array->slot;
			for(i=0;i<n;++i)
			{
				pp2[i]=(wtk_enode_t*)pp1[i]->user;
			}
			if(p->succ->n_use>1)
			{
				p2=nk->chain;
				while(p2)
				{
					if(p!=p2 && p->succ==p2->succ)
					{
						q2=(wtk_enode_t*)p2->user;
						q2->succ=q->succ;
						++q->succ->n_use;
					}
					p2=p2->chain;
				}
			}
		}
		if(!q->pred && p->pred)
		{
			n=p->pred->link_array->nslot;
			q->pred=wtk_ebnf_new_nodeset(ebnf,n);
			pp1=(wtk_enode_t**)p->pred->link_array->slot;
			pp2=(wtk_enode_t**)wtk_array_push_n(q->pred->link_array,n);
			for(i=0;i<n;++i)
			{
				pp2[i]=(wtk_enode_t*)pp1[i]->user;
			}
			if(p->pred->n_use>1)
			{
				p2=nk->chain;
				while(p2)
				{
					if(p!=p2 && p->pred==p2->pred)
					{
						q2=(wtk_enode_t*)p2->user;
						q2->pred=q->pred;
						++q2->pred->n_use;
					}
					p2=p2->chain;
				}
			}
		}
		p=p->chain;
	}
	dst->enter=(wtk_enode_t*)nk->enter->user;
	dst->exit=(wtk_enode_t*)nk->exit->user;
	return 0;
}

void wtk_ebnf_replace_subnet(wtk_ebnf_t *ebnf,wtk_enk_t* net,wtk_enode_t *n)
{
	wtk_enode_t **p,**p2;
	wtk_enode_t *n2;
	int  i,j,nl;

	//self loop.
	nl=n->pred->link_array->nslot;
	p=(wtk_enode_t**)n->pred->link_array->slot;
	for(i=0;i<nl;++i)
	{
		if(p[i]==n){p[i]=net->exit;}
	}
	nl=n->succ->link_array->nslot;
	p=(wtk_enode_t**)n->succ->link_array->slot;
	for(i=0;i<nl;++i)
	{
		if(p[i]==n){p[i]=net->enter;}
	}

	//go through all the pred nodes of p.
	nl=n->pred->link_array->nslot;
	p=(wtk_enode_t**)n->pred->link_array->slot;
	for(i=0;i<nl;++i)
	{
		n2=p[i];
		if(n2->succ->n_use>0)
		{
			n2->succ->n_use=-n2->succ->n_use;
			p2=(wtk_enode_t**)n2->succ->link_array->slot;
			for(j=0;j<n2->succ->link_array->nslot;++j)
			{
				if(p2[j]==n)
				{
					p2[j]=net->enter;
				}
			}
		}
	}
	for(i=0;i<nl;++i)
	{
		if(p[i]->succ->n_use<0)
		{
			p[i]->succ->n_use=-p[i]->succ->n_use;
		}
	}

	//all succ nodes of p.
	nl=n->succ->link_array->nslot;
	p=(wtk_enode_t**)n->succ->link_array->slot;
	for(i=0;i<nl;++i)
	{
		n2=p[i];
		if(n2->pred->n_use>0)
		{
			n2->pred->n_use=-n2->pred->n_use;
			p2=(wtk_enode_t**)n2->pred->link_array->slot;
			for(j=0;j<n2->pred->link_array->nslot;++j)
			{
				if(p2[j]==n)
				{
					p2[j]=net->exit;
				}
			}
		}
	}
	for(i=0;i<nl;++i)
	{
		if(p[i]->pred->n_use<0)
		{
			p[i]->pred->n_use=-p[i]->pred->n_use;
		}
	}
	wtk_enodeset_free(net->enter->pred);
	wtk_enodeset_free(net->enter->succ);
	net->enter->pred=n->pred;
	++net->enter->pred->n_use;
	net->exit->succ=n->succ;
	++net->exit->succ->n_use;
}

int wtk_ebnf_expand_subnet(wtk_ebnf_t *ebnf,wtk_enode_t **chain)
{
	wtk_enk_t *proto;
	wtk_enk_t subnet;
	wtk_enode_t *p,*q;
	int ret=-1;

	//wtk_enode_print(chain);
	p=*chain;*chain=0;
	while(p)
	{
		if(p->ext_name==ebnf->sub_net_id)
		{
			//wtk_enode_print(p,0);
			//wtk_debug("expand ...\n");
			//exit(0);
			proto=(wtk_enk_t*)p->name->data;
			if(!proto){ret=-1;goto end;}
			wtk_ebnf_copy_nk(ebnf,proto,&subnet);
			wtk_ebnf_replace_subnet(ebnf,&subnet,p);
			q=subnet.chain;
			while(q->chain){q=q->chain;}
			q->chain=p->chain;
			wtk_ebnf_delete_node(ebnf,p);
			p=subnet.chain;
		}else
		{
			q=p->chain;
			p->chain=*chain;
			*chain=p;
			p=q;
		}
	}
	ret=0;
	//wtk_enode_print(*chain,1);
	//wtk_debug("%d\n",ret);
	//exit(0);
end:
	return ret;
}

int wtk_ebnf_pnetwork(wtk_ebnf_t *ebnf,wtk_ebnftok_t* tok,wtk_enode_t **hd,wtk_enode_t **tl)
{
	wtk_enode_t *enter,*exit;
	int ret;

	ret=wtk_ebnftok_get_sym(tok);
	if(ret!=0)
	{
		wtk_ebnf_set_err_s(ebnf,"get sym failed.");
		goto end;
	}
	while(tok->sym!=EOFSYM && tok->sym!=LPARSYM)
	{
		ret=wtk_ebnf_subnet(ebnf,tok);
		if(ret!=0)
		{
			wtk_ebnf_set_err_s(ebnf,"subnet failed.");
			goto end;
		}
	}
	ebnf->chain=0;
	*hd=*tl=0;
	if(tok->sym!=LPARSYM)
	{
		wtk_ebnf_set_err_s(ebnf,"expect LPARSYM.");
		ret=-1;goto end;
	}
	ret=wtk_ebnftok_get_sym(tok);
	if(ret!=0)
	{
		wtk_ebnf_set_err_s(ebnf,"get sym1 failed.");
		goto end;
	}
	enter=wtk_ebnf_new_node(ebnf,ebnf->enter_id);
	exit=wtk_ebnf_new_node(ebnf,ebnf->exit_id);
	//wtk_debug("hd=%p,tl=%p\n",*hd,*tl);
	ret=wtk_ebnf_pexpr(ebnf,tok,hd,tl);
	if(ret!=0)
	{
		wtk_ebnf_set_err_s(ebnf,"pexpr failed.");
		goto end;
	}
	wtk_enode_join(enter,*hd);
	wtk_enode_join(*tl,exit);
	*hd=enter;*tl=exit;
	if(tok->sym!=RPARSYM){ret=-1;goto end;}
	ret=wtk_ebnftok_get_sym(tok);
	if(ret!=0 || tok->sym!=EOFSYM){ret=-1;goto end;}
	ret=wtk_ebnf_expand_subnet(ebnf,&(ebnf->chain));
end:
	//exit(0);
	return ret;
}

int wtk_enode_get_wdbegin_num(wtk_enode_t *n)
{
	wtk_enodeinfo_t *ni;

	ni=(wtk_enodeinfo_t*)n->user;
	if(ni->type==wdEnd)
	{
		return ni->nodes;
	}else if(n->succ->link_array->nslot>=1)
	{
		return wtk_enode_get_wdbegin_num(((wtk_enode_t**)n->succ->link_array->slot)[0]);
	}
	return -1;
}

void wtk_lat_link_lnode(wtk_lat_t *lat,float lmlike,int j,int fromNode,int toNode)
{
	wtk_lnode_t *from,*to;
	wtk_larc_t *la;

	if (j >= lat->na) {
		wtk_debug("warnning: over max arcs of lat\n");
		return;
	}
	la=&lat->larcs[j];
	from=&lat->lnodes[fromNode];
	to=&lat->lnodes[toNode];
	//wtk_debug("%d,%d\n",fromNode,toNode);
	la->start=from;la->end=to;la->lmlike=lmlike;
	la->parc=to->pred;
	la->farc=from->foll;
	to->pred=from->foll=la;
}

wtk_dict_word_t* wtk_ebnf_new_word(wtk_ebnf_t *ebnf,const char *data,int bytes)
{
	wtk_heap_t *heap;
	wtk_dict_word_t *wrd;

	heap=ebnf->heap;
	wrd=(wtk_dict_word_t*)wtk_heap_malloc(heap,sizeof(*wrd));
	wrd->npron=0;
	wrd->pron_list=0;
	wrd->name=wtk_heap_dup_string(heap,(char*)data,bytes);
	return wrd;
}

wtk_lat_t* wtk_ebnf_new_lat(wtk_ebnf_t *ebnf,wtk_enk_t *nk)
{
	wtk_heap_t *heap=ebnf->heap;
	wtk_enode_t *p;
	wtk_enodeinfo_t *ni,*ni2;
	int nodes,links;
	wtk_lat_t *lat;
	wtk_dict_t *dict=ebnf->dict;
	wtk_dict_word_t *word;
	wtk_lnode_t *ln;
	int j,i,nn;
	wtk_enode_t **pp;
	double lmlike;

	nodes=links=0;
	for(p=nk->chain;p;p=p->chain)
	{
		ni=(wtk_enodeinfo_t*)p->user;
		if(ni->type==wdExternal || ni->type==nullNode)
		{
			ni->nodes=nodes++;
			if(p->succ)
			{
				ni2=(wtk_enodeinfo_t*)p->succ->user;
				if(ni2)
				{
					++links;
					if(!ni2->seen)
					{
						links+=p->succ->link_array->nslot;
						ni2->nodes=nodes++;
						ni2->seen=1;
					}
				}else
				{
					links+=p->succ->link_array->nslot;
				}
			}
		}
	}
	for(p=nk->chain;p;p=p->chain)
	{
		ni=(wtk_enodeinfo_t*)p->user;
		if(ni->type==wdExternal)
		{
			ni2=(wtk_enodeinfo_t*)p->succ->user;
			if(ni2 && ni2->seen)
			{
				ni2->seen=0;
			}
		}
	}
	lat=wtk_lat_new_h(heap);
	wtk_lat_create(lat,nodes,links);
	lat->lmscale=0;lat->wdpenalty=0;lat->prscale=0;
	//wtk_debug("%d\n",nodes);
	for(p=nk->chain;p;p=p->chain)
	{
		ni=(wtk_enodeinfo_t*)p->user;
		//wtk_enode_print(p,0);
		if(ni->type==wdExternal || ni->type==nullNode)
		{
			word = 0;
			if(ni->type==wdExternal)
			{
				if(ebnf->gwh)
				{
					//check word exist or not.
					//wtk_debug("[%.*s]\n",p->name->name->len,p->name->name->data);
					word=ebnf->gwh(ebnf->gwh_data,p->name->name->data,p->name->name->len);
					//word=wtk_dict_get_word(dict,p->name->name,0);
					if(!word)
					{
						wtk_debug("[%*.*s] not found.\n",p->name->name->len,p->name->name->len,p->name->name->data);
						wtk_ebnf_set_word_err(ebnf,p->name->name);
						wtk_lat_clean(lat);
						lat=0;
						goto end;
					}
				}else
				{
					word=wtk_ebnf_new_word(ebnf,p->name->name->data,p->name->name->len);
				}
			}else
			{
				word=dict?dict->null_word:0;
			}
			ln=&lat->lnodes[ni->nodes];
			ln->info.word=word;
			//wtk_debug("%p\n",word);
			//wtk_debug("%*.*s\n",word->name->len,word->name->len,word->name->data);
			ln->n=0;
			ni2=(wtk_enodeinfo_t*)p->succ->user;
			if(ni2 && !ni2->seen)
			{
				ln=&lat->lnodes[ni2->nodes];
				ln->info.word=dict?dict->null_word:0;
				ln->n=0;
				ni2->seen=1;
			}
		}
	}
	for(p=nk->chain;p;p=p->chain)
	{
		ni=(wtk_enodeinfo_t*)p->user;
		if(ni->type==wdExternal)
		{
			ni2=(wtk_enodeinfo_t*)p->succ->user;
			if(ni2 && ni2->seen)
			{
				ni2->seen=0;
			}
		}
	}
	j=0;
	for(p=nk->chain;p;p=p->chain)
	{
		ni=(wtk_enodeinfo_t*)p->user;
		//wtk_enode_print(p,0);
		if(ni->type==wdExternal ||(ni->type==nullNode&&p->succ))
		{
			ni2=(wtk_enodeinfo_t*)p->succ->user;
			if(ni2)
			{
				wtk_lat_link_lnode(lat,0,j++,ni->nodes,ni2->nodes);
				//wtk_debug("%d,%d\n",fromNode,toNode);
				if(!ni2->seen)
				{
					pp=(wtk_enode_t**)p->succ->link_array->slot;
					nn=p->succ->link_array->nslot;
					lmlike=-log((double)nn);
					for(i=0;i<nn;++i)
					{
						//lmlike=-log((double)nn);
						//wtk_debug("%f:%d\n",lmlike,p->succ->link_array->nslot);
						wtk_lat_link_lnode(lat,lmlike,j++,ni2->nodes,((wtk_enodeinfo_t*)pp[i]->user)->nodes);
					}
					ni2->seen=1;
				}
			}else
			{
				pp=(wtk_enode_t**)p->succ->link_array->slot;
				nn=p->succ->link_array->nslot;
				lmlike=-log((double)nn);
				for(i=0;i<nn;++i)
				{
					//lmlike=-log((double)p->succ->link_array->nslot);
					//wtk_debug("%f:%d\n",lmlike,nn);
					wtk_lat_link_lnode(lat,lmlike,j++,ni->nodes,((wtk_enodeinfo_t*)pp[i]->user)->nodes);
				}
			}
		}
	}
end:
	return lat;
}

void wtk_ebnf_attach_node_info(wtk_ebnf_t *ebnf,wtk_enk_t *nk)
{
	wtk_heap_t *heap=ebnf->heap;
	wtk_enodeinfo_t* ni;
	wtk_enode_t *p;

	p=nk->chain;
	while(p)
	{
		ni=(wtk_enodeinfo_t*)wtk_heap_malloc(heap,sizeof(*ni));
		p->user=ni;
		if((p->name==ebnf->enter_id) || (p->name==ebnf->exit_id))
		{
			ni->type=nullNode;
		}else
		{
			ni->type=wdExternal;
		}
		ni->seen=0;
		ni->history=0;
		if(p->succ->n_use>1)
		{
			ni=(wtk_enodeinfo_t*)wtk_heap_malloc(heap,sizeof(*ni));
			p->succ->user=ni;
			ni->type=nullNode;
			ni->seen=0;
			ni->history=0;
		}
		p=p->chain;
	}
	//exit(0);
}

int wtk_enode_discon(wtk_enode_t *p)
{
	if(!p->succ || !p->pred || p->succ->link_array->nslot==0 || p->pred->link_array->nslot==0)
	{
		return 1;
	}
	return 0;
}

void wtk_ebnf_remove_discon(wtk_ebnf_t *ebnf,wtk_enk_t *net)
{
	wtk_enode_t **pp;
	wtk_enode_t *p,*q;
	int remove_dp,changed;
	int i;

	do
	{
		changed=0;
		p=net->chain;net->chain=0;
		while(p)
		{
			remove_dp=0;
			if(p!=net->enter && p!=net->exit)
			{
				if(wtk_enode_discon(p))
				{
					if(p->succ)
					{
						pp=(wtk_enode_t**)p->succ->link_array->slot;
						for(i=0;i<p->succ->link_array->nslot;++i)
						{
							wtk_enode_delete_link(p,pp[i]->pred);
						}
					}
					if(p->pred)
					{
						pp=(wtk_enode_t**)p->pred->link_array->slot;
						for(i=0;i<p->pred->link_array->nslot;++i)
						{
							wtk_enode_delete_link(p,pp[i]->succ);
						}
					}
					q=p;p=p->chain;
					wtk_ebnf_delete_node(ebnf,p);
					remove_dp=1;changed=1;
				}
			}
			if(!remove_dp)
			{
				q=p->chain;
				p->chain=net->chain;
				net->chain=p;
				p=q;
			}
		}
	}while(changed);
}

int wtk_ebnf_build_net(wtk_ebnf_t *ebnf,wtk_ebnftok_t *tok)
{
	wtk_enode_t *hd=0,*tl=0;
	wtk_enk_t *net=&(ebnf->net);
	int ret;

	ret=wtk_ebnf_pnetwork(ebnf,tok,&hd,&tl);
	if(ret!=0){goto end;}
	net->enter=hd;
	net->exit=tl;
	net->chain=ebnf->chain;
	wtk_ebnf_remove_discon(ebnf,net);
	wtk_ebnf_remove_glue(ebnf,net);
end:
	return ret;
}


int wtk_ebnf_feed(wtk_ebnf_t *ebnf,char* data,int len)
{
	wtk_strbuf_t *buf=wtk_strbuf_new(64,1);
	wtk_ebnftok_t tok={data,len,0,EOFSYM,buf};
	wtk_enk_t *net=&(ebnf->net);
	int ret;

	//printf("%*.*s\n",len,len,data);
	ret=wtk_ebnf_build_net(ebnf,&tok);
	if(ret!=0)
	{
		wtk_ebnf_set_err_s(ebnf,"build network failed.");
		goto end;
	}
	wtk_ebnf_attach_node_info(ebnf,net);
	ebnf->lat=wtk_ebnf_new_lat(ebnf,net);
	//wtk_lat_print(ebnf->lat);
	//wtk_lat_print2(ebnf->lat,stdout);
	ret=ebnf->lat?0:-1;
end:
	wtk_strbuf_delete(buf);
	return ret;
}

void wtk_ebnf_set_err(wtk_ebnf_t *e,char *msg,int msg_bytes)
{
	if(e->os)
	{
		wtk_errno_set(e->os->err,WTK_EVAL_EBNF_INVALID,msg,msg_bytes);
	}else
	{
		wtk_debug("%*.*s\n",msg_bytes,msg_bytes,msg);
	}
}

void wtk_ebnf_set_word_err(wtk_ebnf_t *e,wtk_string_t *w)
{
	wtk_errno_t *err;

	if(e->os)
	{
		err=e->os->err;
		err->no=WTK_EVAL_REF_INVALID;
		wtk_strbuf_reset(err->buf);
		wtk_strbuf_push(err->buf,w->data,w->len);
		wtk_strbuf_push_s(err->buf," not exist.");
	}else
	{
		wtk_debug("%*.*s not exist.",w->len,w->len,w->data);
	}
}
