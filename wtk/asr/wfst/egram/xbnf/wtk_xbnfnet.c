#include "wtk_xbnfnet.h"
#include "qtk_xbnf_post_cfg.h"
#include <ctype.h>
void wtk_xbnfnet_expand_expr_set(wtk_xbnfnet_t *xb,wtk_xbnf_item_set_t *set,
		wtk_fst_state2_t *s,wtk_fst_state2_t *e);

wtk_fst_state2_t* wtk_xbnfnet_load_net(wtk_xbnfnet_t *xb, wtk_string_t *data, wtk_fst_state2_t** ps, wtk_fst_state2_t**pe)
{
	wtk_fst_net2_t *enet;
	wtk_strbuf_t *buf;
	wtk_fst_state2_t *s, *e;
	wtk_string_t wrd;
	wtk_source_t src;
	float w;
	int nl, ret;

	enet=xb->output;

	s = wtk_fst_net2_pop_state(enet);
	e = wtk_fst_net2_pop_state(enet);
	wtk_fst_net2_link_state(enet,e,s,0,0,0,0,0);
	*ps=s; *pe=e;

	buf=wtk_strbuf_new(32, 1);
	wtk_source_init_str(&(src),data->data, data->len);
	while(1)
	{
		//wrd weight
		ret=wtk_source_skip_sp(&src,&nl);
		if(ret!=0) goto end;
		wtk_strbuf_reset(buf);
		ret=wtk_source_read_string(&src, buf);
		if(ret!=0) goto end;
		wrd.data = buf->data;
		wrd.len = buf->pos;
		wtk_source_skip_sp(&src, &nl);
		ret=wtk_source_read_float(&src,&w,1,0);
		if(ret!=0)goto end;
		w=log(w);
		wtk_xbnfnet_attach_wrd2(xb,enet,&wrd,s,e,w);
	}

end:
	return s;
}


wtk_xbnfnet_t* wtk_xbnfnet_new(wtk_xbnfnet_cfg_t *cfg,wtk_egram_sym_t *sym)
{
	wtk_xbnfnet_t *xb;

	xb=(wtk_xbnfnet_t*)wtk_malloc(sizeof(wtk_xbnfnet_t));
	xb->cfg=cfg;
	xb->sym=sym;
	xb->xbnf=wtk_xbnf_new(&(cfg->xbnf));
	xb->use_rep=xb->cfg->use_selfloop;
	xb->use_leak=xb->cfg->use_leak;
	xb->use_addre=xb->cfg->use_addre;
	xb->use_wrd=0;
        wtk_slist_init(&(xb->state_l));
        return xb;
}

void wtk_xbnfnet_delete(wtk_xbnfnet_t *xb)
{
	wtk_xbnf_delete(xb->xbnf);
	wtk_free(xb);
}

void wtk_xbnfnet_reset(wtk_xbnfnet_t *xb)
{
	wtk_slist_init(&(xb->state_l));
	wtk_xbnf_reset(xb->xbnf);
	xb->output=NULL;
}

void wtk_xbnfnet_expand_word2(wtk_xbnfnet_t *xb,wtk_string_t *v,
		wtk_fst_state2_t *s,wtk_fst_state2_t *e)
{
	char *ps,*pe;
	int cnt;
	wtk_fst_state2_t *pre_s,*ie;
	wtk_dict_word_t *dw;
	wtk_fst_trans2_t *t;
	int id=0;
	char c;

	//wtk_debug("[%.*s]\n",v->len,v->data);
	ps=v->data;
	pe=ps+v->len;
	pre_s=s;
	while(ps<pe)
	{
		cnt=wtk_utf8_bytes(*ps);
		if(cnt==1)
		{
			if(xb->cfg->lower)
			{
				c=tolower(*ps);
			}else
			{
				c=toupper(*ps);
			}
			if(xb->cfg->use_ctx){
				if(pre_s->in_prev)
					dw=xb->sym->get_word2(xb->sym->ths,&c,cnt, pre_s->in_prev->hook2);
				else
					dw=xb->sym->get_word2(xb->sym->ths,&c,cnt, NULL);
			}else
				dw=xb->sym->get_word(xb->sym->ths,&c,cnt);
			id=xb->sym->get_id(xb->sym->ths,ps,cnt);
		}else
		{
			if(xb->cfg->use_ctx){
				if(pre_s->in_prev)
					dw=xb->sym->get_word2(xb->sym->ths,ps,cnt, pre_s->in_prev->hook2);
				else
					dw=xb->sym->get_word2(xb->sym->ths,ps,cnt, NULL);
			}else
				dw=xb->sym->get_word(xb->sym->ths,ps,cnt);
			if(dw)
			{
				id=(long)(dw->aux);
			}
		}
		if(ps+cnt>=pe)
		{
			ie=e;
		}else
		{
			ie=wtk_fst_net2_pop_state(xb->output);
		}
		if(dw)
		{
			t=wtk_fst_net2_link_state(xb->output,pre_s,ie,0,id,id,0,0);
			t->hook2=dw;
		}else
		{
			wtk_fst_net2_link_state(xb->output,pre_s,ie,0,0,0,0,0);
		}
		pre_s=ie;
		ps+=cnt;
	}
}

void wtk_xbnfnet_expand_word(wtk_xbnfnet_t *xb,wtk_string_t *v,
		wtk_fst_state2_t *s,wtk_fst_state2_t *e)
{
	char *ps,*pe;
	int cnt;
	wtk_fst_state2_t *pre_s,*ie;
	wtk_dict_word_t *dw;
	wtk_fst_trans2_t *t;
	int id=0;
	int out_id;
	char c;
	int i=0;

	//wtk_debug("[%.*s]\n",v->len,v->data);
	ps=v->data;
	pe=ps+v->len;
	pre_s=s;
	out_id=xb->sym->get_id(xb->sym->ths,v->data,v->len);
	while(ps<pe)
	{
		cnt=wtk_utf8_bytes(*ps);
		if(cnt==1)
		{
			if(xb->cfg->lower)
			{
				c=tolower(*ps);
			}else
			{
				c=toupper(*ps);
			}
			if(xb->cfg->use_ctx){
				if(pre_s->in_prev)
					dw=xb->sym->get_word2(xb->sym->ths,&c,cnt, pre_s->in_prev->hook2);
				else
					dw=xb->sym->get_word2(xb->sym->ths,&c,cnt, NULL);
			}else{
				dw=xb->sym->get_word(xb->sym->ths,&c,cnt);
			}
			id=xb->sym->get_id(xb->sym->ths,ps,cnt);
		}else
		{
			if(xb->cfg->use_ctx){
				if(pre_s->in_prev)
					dw=xb->sym->get_word2(xb->sym->ths,ps,cnt, pre_s->in_prev->hook2);
				else
					dw=xb->sym->get_word2(xb->sym->ths,ps,cnt, NULL);
			}else{
				dw=xb->sym->get_word(xb->sym->ths,ps,cnt);
			}
			if(dw)
			{
				id=(long)(dw->aux);
			}
		}
		//wtk_debug("wrd=[%.*s] id=%d\n",cnt,ps,id);
		if(ps+cnt>=pe)
		{
			ie=e;
		}else
		{
			ie=wtk_fst_net2_pop_state(xb->output);
		}
		if(dw)
		{
			++i;
			if(i==1)
			{
				t=wtk_fst_net2_link_state(xb->output,pre_s,ie,0,id,out_id,0,0);
			}else
			{
				t=wtk_fst_net2_link_state(xb->output,pre_s,ie,0,id,0,0,0);
			}
			t->hook2=dw;
		}else
		{
			t=wtk_fst_net2_link_state(xb->output,pre_s,ie,0,0,0,0,0);
			t->hook2=NULL;
		}
		pre_s=ie;
		ps+=cnt;
	}
}

void wtk_xbnfnet_attach_wrd2(wtk_xbnfnet_t *xb,wtk_fst_net2_t *output, wtk_string_t *str,
		wtk_fst_state2_t *pre_s,wtk_fst_state2_t *ie, float lm_like)
{
	wtk_strbuf_t *buf=xb->xbnf->buf;
	wtk_dict_word_t *dw;
	int id;
	int j;
	char c;
	wtk_fst_trans2_t *t;

	if (xb->cfg->use_ctx){
		if(pre_s->in_prev)
			dw=xb->sym->get_word2(xb->sym->ths,str->data,str->len, pre_s->in_prev->hook2);
		else
			dw=xb->sym->get_word2(xb->sym->ths,str->data,str->len, NULL);
	}else
		dw=xb->sym->get_word(xb->sym->ths,str->data,str->len);

	//wtk_debug("[%.*s]=%p\n",str->len,str->data,dw);
	//if(dw)wtk_dict_word_print(dw, 1);
	if(dw)
	{
		id=(long)dw->aux;
		t=wtk_fst_net2_link_state(output,pre_s,ie,0,id,id,lm_like,0);
		t->hook2=dw;
		//wtk_debug("[%.*s] id=%d\n",str->len,str->data, id);
	}else
	{
		//printf("unsupport word:%.*s\n",str->len,str->data);
		//exit(0);
		wtk_strbuf_reset(buf);
		for(j=0;j<str->len;++j)
		{
			if(xb->cfg->lower)
			{
				c=tolower(str->data[j]);
			}else
			{
				c=toupper(str->data[j]);
			}
			wtk_strbuf_push_c(buf,c);
		}
		//wtk_debug("%.*s\n",buf->pos,buf->data);
		if(xb->cfg->use_ctx){
			if(pre_s->in_prev)
				dw=xb->sym->get_word2(xb->sym->ths,buf->data,buf->pos, pre_s->in_prev->hook2);
			else
				dw=xb->sym->get_word2(xb->sym->ths,buf->data,buf->pos, NULL);
		}else
			dw=xb->sym->get_word(xb->sym->ths,buf->data,buf->pos);

		if(dw)
		{
			id=xb->sym->get_id(xb->sym->ths,str->data,str->len);
			//id=(long)dw->aux;
			t=wtk_fst_net2_link_state(output,pre_s,ie,0,id,id,lm_like,0);
			t->hook2=dw;
			//wtk_debug("[%.*s] id=%d\n",str->len,str->data, id);
		}else
		{
			//wtk_debug("%.*s\n",buf->pos,buf->data);
			wtk_xbnfnet_expand_word(xb,str,pre_s,ie);
		}
	}
}

void wtk_xbnfnet_attach_wrd(wtk_xbnfnet_t *xb,wtk_string_t *str,
		wtk_fst_state2_t *pre_s,wtk_fst_state2_t *ie)
{
	wtk_xbnfnet_attach_wrd2(xb, xb->output, str, pre_s, ie, 0.0f);
}

void wtk_xbnfnet_expand_item(wtk_xbnfnet_t *xb,wtk_xbnf_item_t *item,
		wtk_fst_state2_t *s,wtk_fst_state2_t *e)
{
	wtk_fst_state2_t *pre_s,*ie;
	wtk_fst_state2_t *s2,*e2;
	int i;

	if(item->attr.max>1)
	{
		pre_s=s;
		//wtk_xbnf_item_print(item);
		for(i=0;i<item->attr.max;++i)
		{
			//wtk_debug("i=%d\n",i);
			if(i==(item->attr.max-1))
			{
				ie=e;
			}else
			{
				ie=wtk_fst_net2_pop_state(xb->output);
			}
			if(i>=item->attr.min)
			{
				wtk_fst_net2_link_state(xb->output,pre_s,e,0,0,0,0,0);
			}
			switch(item->type)
			{
			case WTK_XBNF_ITEM_STR:
				wtk_xbnfnet_attach_wrd(xb,item->v.str,pre_s,ie);
				break;
			case WTK_XBNF_ITEM_VAR:
				wtk_xbnfnet_expand_expr_set(xb,item->v.expr->set,pre_s,ie);
				break;
			default:
				wtk_xbnfnet_expand_expr_set(xb,item->v.set,pre_s,ie);
				break;
			}
			pre_s=ie;
		}
	}else
	{
		if(item->attr.max==-1)// || item->attr.min==0)
		{
			s2=wtk_fst_net2_pop_state(xb->output);
			wtk_fst_net2_link_state(xb->output,s,s2,0,0,0,0,0);
			e2=wtk_fst_net2_pop_state(xb->output);
			wtk_fst_net2_link_state(xb->output,e2,e,0,0,0,0,0);
			s=s2;
			e=e2;
		}
		switch(item->type)
		{
		case WTK_XBNF_ITEM_STR:
			//wtk_debug("[%.*s]\n",item->v.str->len,item->v.str->data);
			wtk_xbnfnet_attach_wrd(xb,item->v.str,s,e);
			break;
		case WTK_XBNF_ITEM_VAR:
			wtk_xbnfnet_expand_expr_set(xb,item->v.expr->set,s,e);
			break;
		default:
			wtk_xbnfnet_expand_expr_set(xb,item->v.set,s,e);
			break;
		}
		if(item->attr.min==0)
		{
			wtk_fst_net2_link_state(xb->output,s,e,0,0,0,0,0);
		}
		if(item->attr.max==-1)
		{
			wtk_fst_net2_link_state(xb->output,e,s,0,0,0,0,0);
		}
	}
}


void wtk_xbnfnet_expand_expr_list(wtk_xbnfnet_t *xb,wtk_xbnf_item_list_t *list,
		wtk_fst_state2_t *s,wtk_fst_state2_t *e)
{
	wtk_queue_node_t *qn;
	wtk_xbnf_item_t *item;
	wtk_fst_state2_t *ie;
	wtk_fst_state2_t *pre_s;

	pre_s=s;
	for(qn=list->list_q.pop;qn;qn=qn->next)
	{
		item=data_offset(qn,wtk_xbnf_item_t,q_n);
		if(qn==list->list_q.push)
		{
			ie=e;
		}else
		{
			ie=wtk_fst_net2_pop_state(xb->output);
			if(xb->use_wrd)
			{
				wtk_slist_push(&(xb->state_l),&(ie->q_n));
			}
		}
		wtk_xbnfnet_expand_item(xb,item,pre_s,ie);
		pre_s=ie;
	}
}

void wtk_xbnfnet_expand_expr_set(wtk_xbnfnet_t *xb,wtk_xbnf_item_set_t *set,
		wtk_fst_state2_t *s,wtk_fst_state2_t *e)
{
	wtk_queue_node_t *qn;
	wtk_xbnf_item_list_t *list;

	for(qn=set->set_q.pop;qn;qn=qn->next)
	{
		list=data_offset(qn,wtk_xbnf_item_list_t,q_n);
		if(list->list_q.length>0)
		{
			wtk_xbnfnet_expand_expr_list(xb,list,s,e);
		}else
		{
			wtk_fst_net2_link_state(xb->output,s,e,0,0,0,0,0);
		}
	}
}

int wtk_xbnfnet_tonet(wtk_xbnfnet_t *xb,wtk_xbnf_expr_t *expr,wtk_fst_net2_t *output)
{

	output->start=wtk_fst_net2_pop_state(output);
	output->end=wtk_fst_net2_pop_state(output);
	if(xb->use_wrd)
	{
		wtk_slist_push(&(xb->state_l),&(output->start->q_n));
		wtk_slist_push(&(xb->state_l),&(output->end->q_n));
	}
	output->end->type=WTK_FST_FINAL_STATE;
	xb->output=output;
	wtk_xbnfnet_expand_expr_set(xb,expr->set,output->start,output->end);
	return 0;
}

wtk_string_t* wtk_xbnfnet_get_outsym(wtk_xbnfnet_t *xb,int id)
{
	return xb->sym->get_str(xb->sym->ths,id);
}

void wtk_xbnfnet_print_net(wtk_xbnfnet_t *xb,wtk_fst_net2_t *net)
{
	wtk_fst_net_print_t print;

	wtk_fst_net_print_init(&print,xb,(wtk_fst_net_get_sym_f)wtk_xbnfnet_get_outsym,
			(wtk_fst_net_get_sym_f)wtk_xbnfnet_get_outsym);
	net->print=&(print);
	wtk_fst_net2_write_lat(net,"test.lat");
}

int wtk_xbnfnet_process(wtk_xbnfnet_t *xb,wtk_string_t *ebnf,wtk_fst_net2_t *net)
{
	int ret;

	//wtk_debug("[%.*s]\n",ebnf->len,ebnf->data);
	ret=wtk_xbnf_compile(xb->xbnf,ebnf->data,ebnf->len);
	if(ret!=0)
	{
		wtk_debug("compile failed.\n");
		goto end;
	}
	//wtk_xbnf_zip(xb->xbnf);
	//wtk_xbnf_print(xb->xbnf);
	//word net
	ret=wtk_xbnfnet_tonet(xb,xb->xbnf->main_expr,net);
//	if (xb->use_leak)
//	{
//		ret=wtk_fst_net2_addleakpath(net, &(xb->cfg->xbnf_post));
//	}
//	//self loop word/snt
//	if (xb->use_rep)
//	{
//		ret=wtk_fst_net2_addselfloop(net, &(xb->cfg->xbnf_post));
//	}
//	if(xb->use_addre)
//	{
//		ret=wtk_fst_net2_addreview(net, &(xb->cfg->xbnf_post));
//	}

	ret=wtk_fst_net2_addpath(net, &(xb->cfg->xbnf_post), xb->cfg->use_leak, xb->cfg->use_addre, xb->cfg->use_selfloop);

	//wtk_xbnfnet_print_net(xb,net);
	ret=0;
end:
	//wtk_debug("N=%d L=%d\n",net->state_id,net->trans_id);
	return ret;
}

int wtk_xbnfnet_process2(wtk_xbnfnet_t *xb,char *fn,wtk_fst_net2_t *net)
{
	int ret;

	//wtk_debug("[%.*s]\n",ebnf->len,ebnf->data);
	ret=wtk_xbnf_compile_file(xb->xbnf,fn);
	if(ret!=0)
	{
		wtk_debug("compile failed.\n");
		goto end;
	}
	//wtk_xbnf_zip(xb->xbnf);
	//wtk_xbnf_print(xb->xbnf);
	ret=wtk_xbnfnet_tonet(xb,xb->xbnf->main_expr,net);
	//wtk_xbnfnet_print_net(xb,net);
	ret=0;
end:
	//wtk_debug("N=%d L=%d\n",net->state_id,net->trans_id);
	return ret;
}
