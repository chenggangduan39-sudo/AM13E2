#include <ctype.h>
#include "wtk_ebnfnet.h"

wtk_ebnfnet_t* wtk_ebnfnet_new(wtk_egram_cfg_t *cfg,wtk_egram_sym_t *sym)
{
	wtk_ebnfnet_t *n;

	n=(wtk_ebnfnet_t*)wtk_malloc(sizeof(wtk_ebnfnet_t));
	n->cfg=cfg;
	n->sym=sym;
	n->ebnf=wtk_ebnf_new(NULL,NULL,NULL,NULL);
	n->expand_lat=NULL;
	n->heap=wtk_heap_new(4096);
	n->buf=wtk_strbuf_new(256,1);
	n->lat_net=wtk_lat_net_new();
	return n;
}

void wtk_ebnfnet_delete(wtk_ebnfnet_t *n)
{
	wtk_lat_net_delete(n->lat_net);
	wtk_strbuf_delete(n->buf);
	wtk_heap_delete(n->heap);
	if(n->expand_lat)
	{
		wtk_lat_clean(n->expand_lat);
	}
	wtk_ebnf_delete(n->ebnf);
	wtk_free(n);
}

void wtk_ebnfnet_reset(wtk_ebnfnet_t *n)
{
	wtk_lat_net_reset(n->lat_net);
	wtk_heap_reset(n->heap);
	if(n->expand_lat)
	{
		wtk_lat_clean(n->expand_lat);
		n->expand_lat=NULL;
	}
	wtk_ebnf_reset(n->ebnf);
}

typedef struct
{
	wtk_string_t **strs;
	int n;
}wtk_lat_multwrd_t;

wtk_lat_multwrd_t* wtk_ebnfnet_new_multwrd(wtk_ebnfnet_t *e,wtk_string_t **strs,int n)
{
	wtk_heap_t *heap=e->heap;
	wtk_lat_multwrd_t *wrd;
	int i;

	wrd=(wtk_lat_multwrd_t*)wtk_heap_malloc(heap,sizeof(wtk_lat_multwrd_t));
	wrd->n=n;
	wrd->strs=(wtk_string_t**)wtk_heap_malloc(heap,sizeof(wtk_string_t*)*n);
	for(i=0;i<n;++i)
	{
		wrd->strs[i]=wtk_heap_dup_string(heap,strs[i]->data,strs[i]->len);
	}
	return wrd;
}

wtk_lat_t* wtk_ebnfnet_expand_mul_lat(wtk_ebnfnet_t *ebnfnet,wtk_lat_t *lat,wtk_heap_t *h,int expanded)
{
	wtk_lat_multwrd_t *wrd;
	wtk_larc_t *arc,*arc2;
	wtk_lnode_t *n,*n2,*s,*e;
	wtk_lat_t *cpy;
	int nn,na;
	int i,j,k;
	int jarc;
	wtk_egram_sym_t *sym=ebnfnet->sym;

	cpy=wtk_lat_new_h(h);
	nn=lat->nn+expanded;
	na=lat->na+expanded;
	wtk_lat_create(cpy,nn,na);
	jarc=0;
	for(i=0,j=0;i<lat->nn;++i)
	{
		n=lat->lnodes+i;
		if(n->pron_chain)
		{
			wrd=(wtk_lat_multwrd_t*)n->pron_chain;
			s=NULL;
			for(k=0;k<wrd->n;++k)
			{
				n2=cpy->lnodes+j;
				++j;
				*n2=*n;

				n2->pron_chain=NULL;
				n2->foll=n2->pred=NULL;
				n2->info.word=sym->get_word(sym->ths,wrd->strs[k]->data,wrd->strs[k]->len);
				//n2->info.word=wtk_egram_get_dict_word(egram,wrd->strs[k]->data,wrd->strs[k]->len);
				if(k==0)
				{
					n->hook=n2;
					s=n2;
				}else
				{
					arc2=cpy->larcs+jarc;
					++jarc;
					arc2->start=cpy->lnodes+j-2;
					arc2->end=n2;

					arc2->farc=arc2->start->foll;
					arc2->start->foll=arc2;
					arc2->parc=arc2->end->pred;
					arc2->end->pred=arc2;
				}
				if(k==wrd->n-1)
				{
					s->pron_chain=(wtk_proninst_t*)n2;
				}
			}
		}else
		{
			n2=cpy->lnodes+j;
			if(n->info.word)
			{
				n2->info.word=sym->get_word(sym->ths,n->info.word->name->data,n->info.word->name->len);
				//n2->info.word=wtk_egram_get_dict_word(egram,n->info.word->name->data,n->info.word->name->len);
			}
			++j;
			*n2=*n;
			n2->foll=n2->pred=NULL;
			n->hook=n2;
		}
	}
	for(i=0,j=jarc;i<lat->na;++i)
	{
		arc=lat->larcs+i;
		s=arc->start;
		e=arc->end;
		arc2=cpy->larcs+j;
		++j;
		*arc2=*arc;

		arc2->farc=NULL;
		arc2->parc=NULL;

		arc2->start=(wtk_lnode_t*)s->hook;
		if(arc2->start->pron_chain)
		{
			arc2->start=(wtk_lnode_t*)arc2->start->pron_chain;
		}
		arc2->end=(wtk_lnode_t*)e->hook;

		arc2->farc=arc2->start->foll;
		arc2->start->foll=arc2;
		arc2->parc=arc2->end->pred;
		arc2->end->pred=arc2;
	}
	//exit(0);
	return cpy;
}


wtk_lat_t* wtk_ebnfnet_expand_lat(wtk_ebnfnet_t *e,wtk_lat_t *lat)
{
	wtk_dict_word_t *d_wrd;
	wtk_lat_multwrd_t *wrd;
	wtk_lnode_t *n;
	wtk_string_t *v;
	wtk_string_t **strs;
	//int ret;
	int i,cnt;
	int expanded;
	wtk_heap_t *heap=e->heap;
	wtk_array_t *a;
	wtk_strbuf_t *buf=e->buf;
	wtk_egram_sym_t *sym=e->sym;
	wtk_lat_t *dst=NULL;

	//wtk_lat_write(lat,"x.lat");
	//wtk_lat_print3(lat,stdout);
	//exit(0);
	expanded=0;
	//wtk_debug("nnode=%d\n",lat->nnode);
	for(i=0;i<lat->nn;++i)
	{
		n=lat->lnodes+i;
		//wtk_debug("v[%d]=%d %p\n",i,n->type,n->info.word);
		//n->hook=NULL;
		n->pron_chain=NULL;
		if(n->type==WTK_LNODE_WORD && n->info.word && n->info.word->name)
		{
			v=n->info.word->name;
			cnt=wtk_utf8_len(v->data,v->len);
			if(cnt<=1)
			{
				n->info.word=sym->get_word(sym->ths,v->data,v->len);
				//n->info.word=wtk_egram_get_dict_word(e,v->data,v->len);
				continue;
			}
			if(wtk_string_cmp_s(v,"<s>")==0 || wtk_string_cmp_s(v,"</s>")==0)
			{
				n->info.word=sym->get_word(sym->ths,v->data,v->len);
				//n->info.word=wtk_egram_get_dict_word(e,v->data,v->len);
				continue;
			}
			d_wrd=sym->get_word(sym->ths,v->data,v->len);
			//wtk_debug("v[%d]=[%.*s]\n",i,v->len,v->data);
			//d_wrd=wtk_egram_get_dict_word(e,v->data,v->len);
			if(d_wrd)
			{
				//wtk_debug("[%.*s]\n",v->len,v->data);
				n->info.word=d_wrd;
				continue;
			}
			if(cnt==v->len)
			{
				int j;
				char c;

				wtk_strbuf_reset(buf);
				for(j=0;j<v->len;++j)
				{
					if(e->cfg->lower)
					{
						c=tolower(v->data[j]);
					}else
					{
						c=toupper(v->data[j]);
					}
					wtk_strbuf_push_c(buf,c);
				}
				//n->info.word->name=wtk_heap_dup_string(heap,buf->data,buf->pos);
				//v=n->info.word->name;
				//d_wrd=wtk_egram_get_dict_word(e,buf->data,buf->pos);
				d_wrd=sym->get_word(sym->ths,buf->data,buf->pos);
				if(d_wrd)
				{
					//wtk_debug("[%.*s]\n",v->len,v->data);
					n->info.word=d_wrd;
					continue;
				}
			}
			a=wtk_str_to_chars(heap,v->data,v->len);
			cnt=a->nslot;
			if(cnt>1)
			{
				strs=(wtk_string_t**)a->slot;
				wrd=wtk_ebnfnet_new_multwrd(e,strs,cnt);
				//wrd=wtk_egram_new_multwrd(e,strs,cnt);
				n->pron_chain=(wtk_proninst_t*)wrd;
				expanded+=cnt-1;
			}
		}
	}
	//exit(0);
	//wtk_debug("expanded=%d\n",expanded);
	if(expanded>0)
	{
		e->expand_lat=wtk_ebnfnet_expand_mul_lat(e,lat,lat->heap,expanded);
		if(!e->expand_lat){goto end;}
		dst=e->expand_lat;
	}else
	{
		dst=lat;
	}
end:
	return dst;
}

int wtk_ebnfnet_process(wtk_ebnfnet_t *e,wtk_string_t *ebnf,wtk_fst_net2_t *net)
{
	wtk_lat_t *lat;
	int ret;

	ret=wtk_ebnf_feed(e->ebnf,ebnf->data,ebnf->len);
	if(ret!=0){goto end;}
	//wtk_debug("N=%d L=%d\n",e->ebnf->lat->nn,e->ebnf->lat->na);
	lat=wtk_ebnfnet_expand_lat(e,e->ebnf->lat);
	if(!lat){ret=-1;goto end;}
	//wtk_debug("N=%d L=%d\n",lat->nn,lat->na);
	wtk_lat_net_process(e->lat_net,lat,net);
	ret=0;
end:
	//wtk_debug("N=%d L=%d\n",net->state_id,net->trans_id);
	//wtk_debug("ret=%d\n",ret);
	return ret;
}

