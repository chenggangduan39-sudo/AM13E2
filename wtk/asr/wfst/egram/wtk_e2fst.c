#include "wtk_e2fst.h"
#include "wtk_egram.h"
int wtk_e2fst_add_sym(wtk_e2fst_t *e,char *data,int bytes);
#define wtk_e2fst_add_sym_s(e,s) wtk_e2fst_add_sym(e,s,sizeof(s)-1)

#define SLOG(a) log(a)
#define SEXP(a) exp(a)

void wtk_e2fst_init_bin(wtk_e2fst_t *e)
{
	char c=e->cfg->sil_id;

	if(e->cfg->type == 0)
	{
		e->sil_in_id=wtk_e2fst_get_phnid(e,&(c),1);
	}else
	{
		e->sil_in_id=wtk_e2fst_get_phnid4(e,1);
	}
	//wtk_debug("%d\n",e->sil_in_id);
	e->sil_S_in_id=e->sil_in_id;
	if(e->cfg->use_posi)
	{
		c=e->cfg->sil_S_id;
		e->sil_S_in_id=wtk_e2fst_get_phnid(e,&(c),1);
	}
}

void wtk_e2fst_init_txt(wtk_e2fst_t *e)
{
	wtk_string_t v=wtk_string("sil");

	e->cfg->sil_id=wtk_fst_insym_get_index(e->net->cfg->sym_in,&v);
	if (e->cfg->use_posi)
	{
		wtk_string_t v1=wtk_string("sil_S");
		e->cfg->sil_S_id=wtk_fst_insym_get_index(e->net->cfg->sym_in,&v1);
	}
}

wtk_e2fst_t* wtk_e2fst_new(wtk_e2fst_cfg_t *cfg,struct wtk_egram *egram,wtk_rbin2_t *rbin)
{
	wtk_e2fst_t *e;

	e=(wtk_e2fst_t*)wtk_malloc(sizeof(wtk_e2fst_t));
	e->cfg=cfg;
	e->egram=egram;
	e->net=wtk_fst_net2_new(&(cfg->net));
	e->hmm_net=wtk_fst_net2_new(&(cfg->net));
	e->buf=wtk_strbuf_new(256,1);
	e->hash=wtk_str_hash_new(cfg->sym_hash_hint);
	//wtk_fkv_load_all(e->fkv);
	e->wrd_net=wtk_fst_net2_new(&(cfg->net));
	e->mono_net=wtk_fst_net2_new(&(cfg->net));
        e->out_id = 122;
        e->mono_cnt = 0;
        e->full_mono_net = NULL;
        e->filler_net = NULL;
        if (e->cfg->add_filler || e->cfg->filler) {
            e->filler_net = wtk_fst_net_new(&(cfg->filler_net));
            e->full_mono_net = wtk_fst_net2_new(&(cfg->net));
        }

        //wtk_debug("bin=%d %s\n",egram->cfg->use_bin,cfg->phn_map_fn);
	if(egram->cfg->use_bin && cfg->phn_map_fn)
	{
		if(rbin)
		{
			wtk_rbin2_item_t *item;

			item=wtk_rbin2_get(rbin,cfg->phn_map_fn,strlen(cfg->phn_map_fn));
			if(!item)
			{
				wtk_debug("[%s] not found\n",cfg->phn_map_fn);
				return NULL;
			}
			e->fkv=wtk_fkv_new2(cfg->phn_hash_hint,rbin->f,item->pos,item->len,0);
		}else
		{
			e->fkv=wtk_fkv_new(cfg->phn_map_fn,cfg->phn_hash_hint);
		}
		wtk_e2fst_init_bin(e);
	}else
	{
		wtk_e2fst_init_txt(e);
		e->fkv=NULL;
	}
	wtk_e2fst_reset(e);
	return e;
}

void wtk_e2fst_delete(wtk_e2fst_t *e)
{
	wtk_fst_net2_delete(e->wrd_net);
	wtk_fst_net2_delete(e->mono_net);
	if (e->filler_net) {
		wtk_fst_net2_delete(e->full_mono_net);
		wtk_fst_net_delete(e->filler_net);
	}
	if(e->fkv)
	{
		wtk_fkv_delete(e->fkv);
	}
	wtk_str_hash_delete(e->hash);
	wtk_strbuf_delete(e->buf);
	wtk_fst_net2_delete(e->net);
	wtk_fst_net2_delete(e->hmm_net);
	wtk_free(e);
}

void wtk_e2fst_reset(wtk_e2fst_t *e)
{
	wtk_fst_net2_reset(e->wrd_net);
	wtk_fst_net2_reset(e->mono_net);
	if (e->filler_net) {
		wtk_fst_net2_reset(e->full_mono_net);
		wtk_fst_net_reset(e->filler_net);
	}
	e->out_id = 122;
	e->mono_cnt = 0;
	wtk_slist_init(&(e->state_l));
	wtk_slist_init(&(e->hmm_l));
	wtk_slist_init(&(e->hmm_l));
	wtk_slist_init(&(e->filler_l));
	wtk_slist_init(&(e->sym_l));
	wtk_str_hash_reset(e->hash);
	wtk_fst_net2_reset(e->net);
	wtk_fst_net2_reset(e->hmm_net);
	e->sym_out_id=-1;
	wtk_e2fst_add_sym_s(e,"<eps>");
	e->snt_end_out_id=wtk_e2fst_add_sym_s(e,"</s>");
	wtk_e2fst_add_sym_s(e,"<s>");
	e->sil_snt_end=NULL;
	e->snt_end=NULL;
	e->hmms_cnt=0;
}

wtk_string_t* wtk_e2fst_get_phnstr(wtk_e2fst_t *e,int id)
{
	wtk_string_t *k,*v;
	wtk_strbuf_t *buf=e->buf;
	int i;

	//wtk_debug("id=%d\n",id);
	if(id==0)
	{
		return NULL;
	}
	k=wtk_fkv_get_int_key(e->fkv,id);
	wtk_strbuf_reset(buf);
	for(i=0;i<k->len;++i)
	{
		v=e->cfg->phn_ids[(int)(k->data[i])];
		if(i==1)
		{
			wtk_strbuf_push_s(buf,"-");
		}else if(i==2)
		{
			wtk_strbuf_push_s(buf,"+");
		}
		wtk_strbuf_push(buf,v->data,v->len);
	}
	return wtk_heap_dup_string(e->hash->heap,buf->data,buf->pos);
}

wtk_string_t* wtk_e2fst_get_phnstr_bit(wtk_e2fst_t *e,int id)
{
	wtk_string_t *v;
	wtk_strbuf_t *buf=e->buf;
	int i;
	int cnt;

	cnt=(id>>24) &0x00FF;
	wtk_strbuf_reset(buf);
	if(cnt>0)
	{
		i=(id>>16) & 0x00FF;
		v=e->cfg->phn_ids[i];
		wtk_strbuf_push(buf,v->data,v->len);
	}
	if(cnt>1)
	{
		i=(id>>8) & 0x00FF;
		v=e->cfg->phn_ids[i];
		wtk_strbuf_push_s(buf,"-");
		wtk_strbuf_push(buf,v->data,v->len);
	}
	if(cnt>2)
	{
		i=(id) & 0x00FF;
		v=e->cfg->phn_ids[i];
		wtk_strbuf_push_s(buf,"+");
		wtk_strbuf_push(buf,v->data,v->len);
	}
	return wtk_heap_dup_string(e->hash->heap,buf->data,buf->pos);
}

int wtk_e2fst_get_phnid(wtk_e2fst_t *e,char *data,int bytes)
{
	int id;

	id=wtk_fkv_get_int(e->fkv,data,bytes,NULL);
//	print_hex(data,bytes);
	if(id<0)
	{
		wtk_debug("not found ");
		print_hex(data,bytes);
		//print_data(data,bytes);
		//wtk_debug("[%.*s] not found\n",bytes,data);
		//id=e->sil_in_id;
		//id=e->sil_in_id;
//		id=wtk_fkv_get_int(e->fkv,data+1,1,NULL);
//		wtk_debug("id=%d\n",id);
		//exit(0);
	}
	return id;
}

int wtk_e2fst_get_id(int cnt,char id1,char id2,char id3)
{
	int v;

	//wtk_debug("cnt=%d id=%d/%d/%d\n",cnt,id1,id2,id3);
	//exit(0);
	v=cnt<<24;
	if(cnt>0)
	{
		v+=id1<<16;
	}
	if(cnt>1)
	{
		v+=id2<<8;
	}
	if(cnt>2)
	{
		v+=id3;
	}
	return v;
}

int wtk_e2fst_get_id2(int cnt,char id)
{
	int v;

	//wtk_debug("cnt=%d id=%d/%d/%d\n",cnt,id1,id2,id3);
	//exit(0);
	v=cnt<<24;
	v+=id;
	return v;
}

int wtk_e2fst_get_phnid2(wtk_e2fst_t *e,int id)
{
	char buf[3];
	int cnt;
	cnt=(id>>24) &0x00FF;
	if(cnt>0)
	{
		buf[0]=(id>>16)&0x00FF;
		//wtk_debug("buf[0]=%d\n", (unsigned char)buf[0]);
	}
	if(cnt>1)
	{
		buf[1]=(id>>8)&0x00FF;
		//wtk_debug("buf[1]=%d\n", (unsigned char)buf[1]);
		if(buf[1]==e->cfg->sil_id)
		{
			return e->sil_in_id;
		}
		if (e->cfg->use_posi && buf[1]==e->cfg->sil_S_id)
		{
			return e->sil_S_in_id;
		}
	}
	if(cnt>2)
	{
		buf[2]=id&0x00FF;
		//wtk_debug("buf[2]=%d\n", (unsigned char)buf[2]);
	}
	return wtk_e2fst_get_phnid(e,buf,cnt);
}

int wtk_e2fst_get_phnid3(wtk_e2fst_t *e,int id)
{
	short buf[3];
	int cnt;

	cnt=(id>>24) &0x00FF;
	if(cnt == 1)
	{
		buf[0]=id&0x01FF;
		cnt = 2;
		//wtk_debug("buf[0]=%d\n", (unsigned char)buf[0]);
	}else if(cnt>1)
	{
		buf[0]=(id>>9)&0x01FF;
		buf[1]=id&0x01FF;
		buf[2]=0;
		//wtk_debug("%d %d %d\n",id,buf[0],buf[1]);
		cnt = 6;
	}

	return wtk_e2fst_get_phnid(e,(char*)buf,cnt);
}

int wtk_e2fst_get_phnid4(wtk_e2fst_t *e,int id)
{
	short buf[3];
	int cnt;

	cnt=1;
	if(cnt == 1)
	{
		buf[0]=id&0x01FF;
		cnt = 2;
	}

	return wtk_e2fst_get_phnid(e,(char*)buf,cnt);
}

wtk_fst_state2_t* wtk_e2fst_pop_state(wtk_e2fst_t *e)
{
	wtk_fst_state2_t *state;

	state=wtk_fst_net2_pop_state(e->net);
	wtk_slist_push(&(e->state_l),&(state->q_n));
	//wtk_debug("pop %d\n",state->id);
	return state;
}

wtk_fst_state2_t* wtk_e2fst_pop_state2(wtk_e2fst_t *e)
{
	wtk_fst_state2_t *state;

	state=wtk_fst_net2_pop_state(e->hmm_net);
	wtk_slist_push_front2(&(e->hmm_l),&(state->q_n));
	e->hmms_cnt++;
	//wtk_debug("pop %d\n",state->id);
	return state;
}

wtk_fst_state2_t *wtk_e2fst_pop_state_filler(wtk_e2fst_t *e) {
    wtk_fst_state2_t *state;

    state = wtk_fst_net2_pop_state(e->full_mono_net);
    wtk_slist_push(&(e->filler_l), &(state->q_n));
    // wtk_debug("pop %d\n",state->id);
    return state;
}

int wtk_e2fst_add_sym(wtk_e2fst_t *e,char *data,int bytes)
{
	wtk_string_t *v;
	wtk_e2fst_id_t *id;
	wtk_heap_t *heap=e->hash->heap;

	v=wtk_heap_dup_string(heap,data,bytes);
	id=(wtk_e2fst_id_t*)wtk_heap_malloc(heap,sizeof(wtk_e2fst_id_t));
	id->id=++e->sym_out_id;
	id->v=v;
	wtk_slist_push(&(e->sym_l),&(id->s_n));
	wtk_str_hash_add(e->hash,v->data,v->len,id);
	//wtk_debug("id=%p idx=%d [%.*s] sn=%p e=%p sym_l=%p\n",id, id->id,bytes,data, &(id->s_n), e, &(e->sym_l));
	return id->id;
}

wtk_string_t* wtk_e2fst_get_outsym(wtk_e2fst_t *e,unsigned int xid)
{
	wtk_slist_node_t *sn;
	wtk_e2fst_id_t *id;

	//wtk_debug("xid=%d\n",xid);
	for(sn=e->sym_l.prev;sn;sn=sn->prev)
	{
		id=data_offset(sn,wtk_e2fst_id_t,s_n);
		//wtk_debug("id=%d/%d\n",id->id,xid);
		if(id->id==xid)
		{
			//wtk_debug("[%.*s]\n",id->v->len,id->v->data);
			//exit(0);
			return id->v;
		}
	}
	return NULL;
}

wtk_string_t* wtk_e2fst_get_insym(wtk_e2fst_t *e,unsigned int id)
{
	return  e->cfg->phn_ids[id];
}


int wtk_e2fst_get_symid(wtk_e2fst_t *e,char *data,int bytes)
{
	wtk_e2fst_id_t *id;

	//wtk_debug("[%.*s] use_bin=%d\n",bytes,data,e->egram->cfg->use_bin);
	if(e->egram->cfg->use_bin)
	{
		id=(wtk_e2fst_id_t*)wtk_str_hash_find(e->hash,data,bytes);
		if(id)
		{
			return id->id;
		}
		return wtk_e2fst_add_sym(e,data,bytes);
	}else
	{
		if(e->cfg->net.sym_out)
		{
			wtk_string_t v;

			wtk_string_set(&(v),data,bytes);
			return wtk_fst_sym_get_index(e->net->cfg->sym_out,&v);
		}else
		{
			id=(wtk_e2fst_id_t*)wtk_str_hash_find(e->hash,data,bytes);
			//wtk_debug("[%.*s]=%p\n",bytes,data,id);
			if(id)
			{
				return id->id;
			}
			return wtk_e2fst_add_sym(e,data,bytes);
		}
	}
}

void wtk_e2fst_get_outsym_node(wtk_strbuf_t *buf,wtk_slist_node_t *sn)
{
	wtk_e2fst_id_t *id;

	if(sn->prev)
	{
		wtk_e2fst_get_outsym_node(buf,sn->prev);
	}
	id=data_offset(sn,wtk_e2fst_id_t,s_n);
	//wtk_debug("id=%p sn=%p\n", id, sn);
	wtk_strbuf_push_f(buf,"%.*s %d\n",id->v->len,id->v->data,id->id-1);
}

void wtk_e2fst_get_outsym_node2(wtk_strbuf_t *buf,wtk_slist_node_t *sn)
{
	wtk_e2fst_id_t *id;

	if(sn->prev)
	{
		wtk_e2fst_get_outsym_node2(buf,sn->prev);
	}
	id=data_offset(sn,wtk_e2fst_id_t,s_n);
	wtk_strbuf_push(buf,(char*)&id->v->len,sizeof(char));
	wtk_strbuf_push(buf,id->v->data,id->v->len);
	wtk_strbuf_push(buf,(char*)&id->id,sizeof(int));
}

void wtk_e2fst_get_outsym_node3(wtk_strbuf_t *buf,wtk_slist_node_t *sn)
{
	wtk_e2fst_id_t *id;

	if(sn->prev)
	{
		wtk_e2fst_get_outsym_node3(buf,sn->prev);
	}
	id=data_offset(sn,wtk_e2fst_id_t,s_n);
	if(id->id > 2 || id->id == 0)
	{
		wtk_strbuf_push(buf,(char*)&id->v->len,sizeof(char));
		wtk_strbuf_push(buf,id->v->data,id->v->len);
	}
	//wtk_strbuf_push(buf,(char*)&id->id,sizeof(int));
}

void wtk_e2fst_get_outsym_bin(wtk_e2fst_t *e,wtk_strbuf_t *buf)
{
	wtk_e2fst_id_t *id;
	int max_sym;
	wtk_slist_t *l=&(e->sym_l);

	wtk_strbuf_reset(buf);
	id=data_offset(l->prev,wtk_e2fst_id_t,s_n);
	max_sym=id->id+1;
	wtk_strbuf_push(buf,(char*)&max_sym,sizeof(int));
	wtk_e2fst_get_outsym_node2(buf,l->prev);
}

void wtk_e2fst_get_outsym_bin2(wtk_e2fst_t *e,wtk_strbuf_t *buf)
{
	wtk_e2fst_id_t *id;
	int max_sym;
	wtk_slist_t *l=&(e->sym_l);

	wtk_strbuf_reset(buf);
	id=data_offset(l->prev,wtk_e2fst_id_t,s_n);
	max_sym=id->id-1;
	wtk_strbuf_push(buf,(char*)&max_sym,sizeof(int));
	wtk_e2fst_get_outsym_node3(buf,l->prev);
}

void wtk_e2fst_get_outsym_txt(wtk_e2fst_t *e,wtk_strbuf_t *buf)
{
	wtk_slist_t *l=&(e->sym_l);

	//wtk_debug("e=%p sym_l=%p prev=%p\n",e, &(e->sym_l), l->prev);
	wtk_strbuf_reset(buf);
	wtk_e2fst_get_outsym_node(buf,l->prev);
}

void wtk_e2fst_print_outsym(wtk_e2fst_t *e)
{
	wtk_strbuf_t *buf;

	buf=wtk_strbuf_new(1024,1);
	wtk_e2fst_get_outsym_txt(e,buf);
	//printf("%.*s\n",buf->pos,buf->data);
	wtk_strbuf_delete(buf);
}

static int wtk_fst_trans_cmp(const void *src1,const void *src2)
{
	wtk_fst_trans_t *trans1,*trans2;

	trans1=*(wtk_fst_trans_t**)src1;
	trans2=*(wtk_fst_trans_t**)src2;

	//wtk_debug("[%d/%d]\n",trans1->in_id,trans2->in_id);
	if(trans1->in_id>trans2->in_id)
	{
		return 1;
	}else
	{
		return -1;
	}
}


void wtk_e2fst_print_net_fsm_state(wtk_e2fst_t *e,wtk_fst_net2_t *net,wtk_fst_state_t *s,wtk_strbuf_t *buf,wtk_larray_t *a)
{
	wtk_fst_trans_t *trans;
	wtk_fst_trans_t **ptrans;
	int i;
	wtk_string_t *k,*v;

	//wtk_debug("%d=%d hook=%p\n",s->id,s->type,s->hook)
	s->hook=s;
	if(s->type==WTK_FST_FINAL_STATE)
	{
		//printf("%d\n",s->id);
		wtk_strbuf_push_f(buf,"%d\n",s->id);
		return;
	}
	wtk_larray_reset(a);
	for(trans=s->v.trans;trans;trans=trans->hook.next)
	{
		wtk_larray_push2(a,&(trans));
		/*
		if(trans->weight!=0)
		{
			//printf("%d %d %d %d %f\n",s->id,trans->to_state->id,trans->in_id,trans->out_id,trans->weight);
			wtk_strbuf_push_f(buf,"%d %d %d %d %f\n",s->id,trans->to_state->id,trans->in_id,trans->out_id,trans->weight);
		}else
		{
			//printf("%d %d %d %d\n",s->id,trans->to_state->id,trans->in_id,trans->out_id);
			wtk_strbuf_push_f(buf,"%d %d %d %d\n",s->id,trans->to_state->id,trans->in_id,trans->out_id);
		}*/
	}
	ptrans=(wtk_fst_trans_t**)a->slot;
	qsort(ptrans,a->nslot,sizeof(wtk_fst_trans_t*),wtk_fst_trans_cmp);
	for(i=0;i<a->nslot;++i)
	{
		trans=ptrans[i];
		if(trans->in_id>0)
		{
			k=e->cfg->phn_ids[trans->in_id];
			if(trans->out_id>0)
			{
				v=wtk_e2fst_get_outsym(e,trans->out_id);
				wtk_strbuf_push_f(buf,"%d %d %.*s %.*s\n",s->id,trans->to_state->id,
					k->len,k->data,v->len,v->data);
			}else
			{
				wtk_strbuf_push_f(buf,"%d %d %.*s 0\n",s->id,trans->to_state->id,
					k->len,k->data);
			}
		}else
		{
			if(trans->out_id>0)
			{
				v=wtk_e2fst_get_outsym(e,trans->out_id);
				wtk_strbuf_push_f(buf,"%d %d 0 %.*s\n",s->id,trans->to_state->id,
						v->len,v->data);
			}else
			{
				wtk_strbuf_push_f(buf,"%d %d 0 0\n",s->id,trans->to_state->id);
			}
		}
	}
	for(trans=s->v.trans;trans;trans=trans->hook.next)
	{
		//wtk_debug("to=%p\n",trans->to_state->hook);
		if(!trans->to_state->hook)
		{
			wtk_e2fst_print_net_fsm_state(e,net,trans->to_state,buf,a);
		}
	}
}

void wtk_e2fst_print_net_fsm(wtk_e2fst_t *e,wtk_fst_net2_t *net,wtk_strbuf_t *buf)
{
	wtk_larray_t *a;

	wtk_fst_net2_clean_hook2(net);
	a=wtk_larray_new(1024,sizeof(wtk_fst_trans_t *));
	wtk_strbuf_reset(buf);
	wtk_e2fst_print_net_fsm_state(e,net,(wtk_fst_state_t*)net->start,buf,a);
	//wtk_strbuf_push_s(buf,"X==A");
	wtk_larray_delete(a);
}

void wtk_e2fst_print_net(wtk_e2fst_t *e,wtk_fst_net2_t *net)
{
	wtk_strbuf_t *buf;

	buf=wtk_strbuf_new(1024,1);
	wtk_e2fst_print_net_fsm(e,net,buf);
	printf("%.*s\n",buf->pos,buf->data);
	wtk_strbuf_delete(buf);
}

typedef struct  wtk_e2fst_trans_link wtk_e2fst_trans_link_t;

struct wtk_e2fst_trans_link
{
	wtk_fst_trans2_t *trans;
	wtk_e2fst_trans_link_t *next_link;
};

void wtk_e2fst_link_trans(wtk_e2fst_t *e,wtk_fst_trans2_t *input_trans,
		wtk_fst_trans2_t *input_nxt,
		wtk_fst_trans2_t *output_trans)
{
	wtk_e2fst_trans_link_t *link;

	link=(wtk_e2fst_trans_link_t*)wtk_heap_malloc(e->hash->heap,sizeof(wtk_e2fst_trans_link_t));
	link->trans=output_trans;
	link->next_link=input_trans->hook2;
	input_trans->hook2=link;
	output_trans->hook2=input_nxt;
}

wtk_fst_trans2_t* wtk_e2fst_find_input_trans2(wtk_fst_trans2_t *b,int in_id,int out_id,
		wtk_fst_trans2_t *nxt)
{
	wtk_e2fst_trans_link_t *t;

	for(t=(wtk_e2fst_trans_link_t*)(b->hook2);t;t=t->next_link)
	{
		//wtk_debug("%d:%d %d:%d %f %f\n",in_id,out_id,t->trans->in_id,t->trans->out_id, t->trans->lm_like, t->trans->weight);
		if(t->trans->hook2==nxt && t->trans->in_id==in_id && t->trans->out_id==out_id)
		{
			return t->trans;
		}
	}
	return NULL;
}

//here no consider lm_like and weight not zero.
wtk_fst_trans2_t* wtk_e2fst_find_input_trans3(wtk_fst_trans2_t *b,int in_id,int out_id,
		wtk_fst_trans2_t *nxt, float lm_like, float lm_weight)
{
	wtk_e2fst_trans_link_t *t;

	for(t=(wtk_e2fst_trans_link_t*)(b->hook2);t;t=t->next_link)
	{
		//wtk_debug("%d:%d %d:%d %f %f\n",in_id,out_id,t->trans->in_id,t->trans->out_id, t->trans->lm_like, t->trans->weight);
		if(t->trans->hook2==nxt && t->trans->in_id==in_id && t->trans->out_id==out_id && fabs(t->trans->lm_like-lm_like)<0.00001 && fabs(t->trans->weight-lm_weight)<0.00001)
		{
			return t->trans;
		}
	}
	return NULL;
}

void wtk_e2fst_expand_tri_net_node2(wtk_e2fst_t *e,wtk_fst_state2_t* pre,
		wtk_fst_trans2_t *a,wtk_fst_state2_t *nxt_state);

void wtk_e2fst_expand_tri_net_eof(wtk_e2fst_t *e,wtk_fst_state2_t *pre,wtk_fst_trans2_t *a,wtk_fst_trans2_t *b)
{
	wtk_fst_net2_t *net=e->net;
	wtk_fst_state2_t *ts;
	int in_id;

	//wtk_debug("===> eof=%d\n",pre->id);
	//if(b->to_state->hook)
	if(e->net->end->in_prev)
	{
		ts=(wtk_fst_state2_t*)(e->net->end->in_prev->from_state);
		wtk_fst_net2_link_state(net,pre,ts,0,0,0,0,0);
	}else
	{
		ts=wtk_e2fst_pop_state(e);
		wtk_fst_net2_link_state(net,pre,ts,0,0,0,0,0);
		pre=ts;
		//in_id=b->in_id<<16;
		in_id=wtk_e2fst_get_id(1,b->in_id,0,0);
		//wtk_debug("in_id=%d b->in_id=%d\n", in_id,b->in_id);
		//wtk_debug("%d: %d\n",in_id,b->out_id);
		wtk_fst_net2_link_state(net,pre,net->end,0,in_id,b->out_id,0,0);
		/*
		if(e->cfg->use_opt_sil)
		{
			wtk_fst_net2_link_state(net,pre,net->end,0,0,b->out_id,0,0);
		}*/
		//wtk_e2fst_link_trans(e,b,tt);
	}
}

void wtk_e2fst_expand_tri_net_eof2(wtk_e2fst_t *e,wtk_fst_state2_t *pre,wtk_fst_trans2_t *a,
		wtk_fst_trans2_t *b)
{
	wtk_fst_net2_t *net=e->net;
	wtk_fst_state2_t *ts;
	int in_id;

	//wtk_debug("===> eof=%d\n",pre->id);
	//if(b->to_state->hook)
	if(a && a->in_id==b->in_id)
	{
		//sil;
		if(e->snt_end)
		{
			ts=e->snt_end;
			wtk_fst_net2_link_state(net,pre,ts,0,0,0,0,0);
		}else
		{
			ts=wtk_e2fst_pop_state(e);
			e->snt_end=ts;
			wtk_fst_net2_link_state(net,pre,ts,0,0,0,0,0);
			wtk_fst_net2_link_state(net,pre,net->end,0,0,b->out_id,0,0);
		}
	}else
	{
		if(e->sil_snt_end)
		{
			ts=e->sil_snt_end;
			wtk_fst_net2_link_state(net,pre,ts,0,0,0,0,0);
		}else
		{
			ts=wtk_e2fst_pop_state(e);
			e->sil_snt_end=ts;
			wtk_fst_net2_link_state(net,pre,ts,0,0,0,0,0);
			pre=ts;
			//in_id=b->in_id<<16;
			in_id=wtk_e2fst_get_id(1,b->in_id,0,0);
			//wtk_debug("%d: %d\n",in_id,b->out_id);
			wtk_fst_net2_link_state(net,pre,net->end,0,in_id,b->out_id,0,0);
		}
	}
}

void wtk_e2fst_expand_tri_net_eof_eval(wtk_e2fst_t *e,wtk_fst_state2_t *pre,wtk_fst_trans2_t *a,wtk_fst_trans2_t *b)
{
	wtk_fst_net2_t *net=e->net;
	wtk_fst_state2_t *ts;
	int in_id, out_id;

	//wtk_debug("===> eof=%d\n",pre->id);
	//if(b->to_state->hook)
//	if(e->net->end->in_prev)
//	{
//		ts=(wtk_fst_state2_t*)(e->net->end->in_prev->from_state);
//		wtk_fst_net2_link_state(net,pre,ts,0,0,0,0,0);
//	}else
//	{
		ts=wtk_e2fst_pop_state(e);
		wtk_fst_net2_link_state(net,pre,ts,0,0,0,0,0);
		pre=ts;
		//in_id=b->in_id<<16;
		if(b)
		{
			in_id=wtk_e2fst_get_id(1,b->in_id,0,0);
			out_id=b->out_id;
		}
		else
		{
			in_id=0;
			out_id=0;
		}
		//wtk_debug("in_id=%d out_id=%d\n", in_id, out_id);
		wtk_fst_net2_link_state(net,pre,net->end,0,in_id,out_id,0,0);
		/*
		if(e->cfg->use_opt_sil)
		{
			wtk_fst_net2_link_state(net,pre,net->end,0,0,b->out_id,0,0);
		}*/
		//wtk_e2fst_link_trans(e,b,tt);
//	}
}

void wtk_e2fst_print_phnstr(wtk_e2fst_t *e,wtk_fst_trans2_t *trans)
{
	wtk_fst_net_print_t print;

	wtk_fst_net_print_init(&print,e,(wtk_fst_net_get_sym_f)wtk_e2fst_get_phnstr_bit,
			(wtk_fst_net_get_sym_f)wtk_e2fst_get_outsym);

	wtk_fst_net_print_trans_prev(&(print),trans);
	//wtk_fst_net_print_trans2(&(print),trans);
	wtk_fst_net_print_trans_next(&(print),trans);
}

void wtk_e2fst_print_phnstr_trans(wtk_e2fst_t *e,wtk_fst_trans2_t *trans)
{
	wtk_fst_net_print_t print;

	wtk_fst_net_print_init(&print,e,(wtk_fst_net_get_sym_f)wtk_e2fst_get_phnstr_bit,
			(wtk_fst_net_get_sym_f)wtk_e2fst_get_outsym);

	//wtk_fst_net_print_trans_prev(&(print),trans);
	wtk_fst_net_print_trans2(&(print),trans);
	//wtk_fst_net_print_trans_next(&(print),trans);
}


void wtk_e2fst_print_mono(wtk_e2fst_t *e,wtk_fst_trans2_t *trans)
{
	wtk_fst_net_print_t print;

	wtk_fst_net_print_init(&print,e,(wtk_fst_net_get_sym_f)wtk_e2fst_get_insym,
			(wtk_fst_net_get_sym_f)wtk_e2fst_get_outsym);

	wtk_fst_net_print_trans_prev(&(print),trans);
	//wtk_fst_net_print_trans2(&(print),trans);
	wtk_fst_net_print_trans_next(&(print),trans);
}

void wtk_e2fst_print_mono_trans(wtk_e2fst_t *e,wtk_fst_trans2_t *trans)
{
	wtk_fst_net_print_t print;

	wtk_fst_net_print_init(&print,e,(wtk_fst_net_get_sym_f)wtk_e2fst_get_insym,
			(wtk_fst_net_get_sym_f)wtk_e2fst_get_outsym);

	//wtk_fst_net_print_trans_prev(&(print),trans);
	wtk_fst_net_print_trans2(&(print),trans);
	//wtk_fst_net_print_trans_next(&(print),trans);
}


void wtk_e2fst_expand_tri_net_node4(wtk_e2fst_t *e,wtk_fst_state2_t* pre,
		wtk_fst_trans2_t *a,wtk_fst_trans2_t *b,wtk_fst_state2_t *nxt_state)
{
	wtk_fst_net2_t *net=e->net;
	wtk_fst_trans2_t *tt;
	wtk_fst_state2_t *ts;
	wtk_fst_trans2_t *trans;
	int in_id;
	int id_ab;

	if(nxt_state->type==WTK_FST_FINAL_STATE)
	{
		//wtk_debug("[%d/%d]\n",a->in_id,b->in_id);
		if (e->cfg->use_eval)
			wtk_e2fst_expand_tri_net_eof_eval(e,pre,a,b);
		else
			wtk_e2fst_expand_tri_net_eof(e,pre,a,b);
		return;
	}
	if((b->in_id==e->cfg->sil_id||(e->cfg->use_posi && b->in_id == e->cfg->sil_S_id))) //  && 0)
	{
		if(!b->to_state->hook)
		{
			//wtk_e2fst_print_mono(e,b);
			//wtk_debug("goto sil type=%d\n",b->to_state->type);
			ts=wtk_e2fst_pop_state(e);
			b->to_state->hook=ts;
			//b->hook2=ts;
			wtk_fst_net2_link_state(net,pre,ts,0,0,0,0,0);
			pre=ts;
			ts=wtk_e2fst_pop_state(e);
			in_id=wtk_e2fst_get_id(1,b->in_id,0,0);
			//wtk_fst_net2_link_state(net,pre,ts,0,in_id,0,0,0);
			wtk_fst_net2_link_state(net,pre,ts,0,in_id,b->out_id,0,0);
			if(e->cfg->use_opt_sil)
			{
				wtk_fst_net2_link_state(net,pre,ts,0,0,0,0,0);
			}
			wtk_e2fst_expand_tri_net_node2(e,ts,b,nxt_state);
		}else
		{
			ts=(wtk_fst_state2_t*)b->to_state->hook;
			//ts=(wtk_fst_state2_t*)b->hook2;
			if(!wtk_fst_state2_has_eps_to(pre,ts))
			{
				wtk_fst_net2_link_state(net,pre,ts,0,0,0,0,0);
			}
		}
		return;
		//for ques? by dmd
//	}else if(b->to_state->hook){
//		return;
	}
	if(e->cfg->use_sil_ctx || (a->in_id!=e->cfg->sil_id && ( 0==e->cfg->use_posi|| a->in_id!=e->cfg->sil_S_id)))
	{
		id_ab=(3<<24)+(a->in_id<<16)+(b->in_id<<8);
	}else
	{
		id_ab=(3<<24)+(b->in_id<<8);	//0 for sil;
	}
	//wtk_debug("a->in_id=%d b->in_id=%d out_id=%d\n", a->in_id, b->in_id, b->out_id);
	//wtk_debug("use_sil=%d\n",e->cfg->use_sil_ctx);
	//wtk_fst_net_print_trans2(&(print),b);
	for(trans=(wtk_fst_trans2_t*)nxt_state->v.trans;trans;trans=(wtk_fst_trans2_t*)trans->hook.next)
	{
		//wtk_debug("tran->in_id=%d\n", trans->in_id);
		//wtk_e2fst_print_mono_trans(e,trans);
		if(e->cfg->use_sil_ctx || (trans->in_id!=e->cfg->sil_id && (0==e->cfg->use_posi || trans->in_id!=e->cfg->sil_S_id)))
		{
			in_id=id_ab+trans->in_id;
		}else
		{
			in_id=id_ab;
		}
//		if(trans->in_id==1 && a->in_id==0x1e && b->in_id==0x1b)
//		{
//			wtk_debug("found bug %#x %d-%d-%d\n",in_id,e->cfg->use_sil_ctx,trans->in_id,e->cfg->sil_id);
//			exit(0);
//		}
		if(b->hook2 && trans->in_id>0)
		{
			//如果有相同
			//tt=wtk_e2fst_find_input_trans2(b,in_id,b->out_id,trans);
			tt=wtk_e2fst_find_input_trans3(b,in_id,b->out_id,trans, b->lm_like, b->weight);
			if(tt)
			{
				if(tt->from_state==(void*)pre)
				{
					continue;
				}
				ts=(wtk_fst_state2_t*)tt->to_state;
				if(b->to_state==(wtk_fst_state_t*)nxt_state)
				{
					wtk_fst_trans2_t *tt1;

					tt1=wtk_fst_state2_find_next(pre,0,0,(wtk_fst_state2_t*)tt->from_state);
					if(!tt1)
					{
						if(wtk_fst_state2_has_eps_to((wtk_fst_state2_t*)tt->from_state,pre))
						{
							//防止环的问题如  <$DIGIT> ;  $DIGIT=(0|1|2);
							//tt=wtk_fst_net2_link_state(net,pre,ts,0,in_id,b->out_id,0,0);
							tt=wtk_fst_net2_link_state(net,pre,ts,0,in_id,b->out_id,b->lm_like,b->weight);
							wtk_e2fst_link_trans(e,b,trans,tt);
							if(!trans->hook2)
							{
								wtk_e2fst_expand_tri_net_node4(e,ts,b,trans,(wtk_fst_state2_t*)trans->to_state);
							}
							continue;
						}
						wtk_fst_net2_link_state(net,pre,(wtk_fst_state2_t*)tt->from_state,0,0,0,0,0);
					}
				}
				if(!trans->hook2)
				{
					wtk_e2fst_expand_tri_net_node4(e,ts,b,trans,(wtk_fst_state2_t*)trans->to_state);
				}
				continue;

			}
		}
		if(trans->in_id>0)
		{
			if(e->cfg->use_sil_ctx || (trans->in_id!=e->cfg->sil_id && (0==e->cfg->use_posi || trans->in_id!=e->cfg->sil_S_id)))
			{
				in_id=id_ab+trans->in_id;
			}else
			{
				in_id=id_ab;
			}
			//in_id=id_ab+trans->in_id;
			ts=wtk_e2fst_pop_state(e);
			//wtk_debug("link %d-%d\n",pre->id,ts->id);
			//wtk_debug("trans->in_id=%d  in_id=%d\n", trans->in_id, in_id);
			/*
			wtk_debug("[%.*s-%.*s+%.*s] %d=>%d\n",
				e->cfg->phn_ids[a->in_id]->len,e->cfg->phn_ids[a->in_id]->data,
				e->cfg->phn_ids[b->in_id]->len,e->cfg->phn_ids[b->in_id]->data,
				e->cfg->phn_ids[trans->in_id]->len,e->cfg->phn_ids[trans->in_id]->data,
				pre->id,ts->id
				);
			*/

			//tt=wtk_fst_net2_link_state(net,pre,ts,0,in_id,b->out_id,0,0);
			tt=wtk_fst_net2_link_state(net,pre,ts,0,in_id,b->out_id,b->lm_like,b->weight);

			wtk_e2fst_link_trans(e,b,trans,tt);
			wtk_e2fst_expand_tri_net_node4(e,ts,b,trans,(wtk_fst_state2_t*)trans->to_state);
		}else
		{
			if(trans->out_id>0)
			{
				//wtk_debug("found bug\n");
				ts=wtk_e2fst_pop_state(e);
				//wtk_fst_net2_link_state(net,pre,ts,0,0,trans->out_id,0,0);
				wtk_fst_net2_link_state(net,pre,ts,0,0,trans->out_id,trans->lm_like,trans->weight);
			}else
			{
				ts=pre;
			}
			wtk_e2fst_expand_tri_net_node4(e,ts,a,b,(wtk_fst_state2_t*)trans->to_state);
		}
	}
}

void wtk_e2fst_expand_tri_net_node2(wtk_e2fst_t *e,wtk_fst_state2_t* pre,
		wtk_fst_trans2_t *a,wtk_fst_state2_t *nxt_state)
{
	wtk_fst_net2_t *net=e->net;
	wtk_fst_trans2_t *trans,*tt;
	wtk_fst_state2_t *ts;
	int in_id;

	/*
	wtk_fst_net_print_t print;
	wtk_fst_net_print_init(&print,e,(wtk_fst_net_get_sym_f)wtk_e2fst_get_insym,
			(wtk_fst_net_get_sym_f)wtk_e2fst_get_outsym);
	*/
	if(nxt_state->type==WTK_FST_FINAL_STATE)
	{
		wtk_debug("found bug\n");
		//exit(0);
		if (e->cfg->use_eval)
			wtk_e2fst_expand_tri_net_eof_eval(e,pre,a,0);
		else
			wtk_e2fst_expand_tri_net_eof(e,pre,0,a);
		return;
	}
	//wtk_debug("a=%p b=%p\n",a,b);
	for(trans=(wtk_fst_trans2_t*)nxt_state->v.trans;trans;trans=(wtk_fst_trans2_t*)trans->hook.next)
	{
		/**
		 * (a12) . (a13:cur)   (c11) .  (c12)
		 *                   .
		 * (b12) . (b23:cur)   (d11) .  (d12)
		 *
		 *					 (a13-c11+c12)
		 *   (a12-a13+c11) .
		 *                   (a13-d11+d12)
		 *
		 *  在原始mono net的state上有相同的input_trans:
		 *  1) 如果input_trans的起始节点有相同的后继节点，虚连接到已连接的相同的trans的起始节点
		 *  2) 如果input_trans的没有相同的后继节点，建立(in_id,out_id)到trans的结束节点;
		 */
#ifdef DEBUG_NET
		{
			wtk_fst_net_print_t print;

			wtk_fst_net_print_init(&print,e,(wtk_fst_net_get_sym_f)wtk_e2fst_get_insym,
					(wtk_fst_net_get_sym_f)wtk_e2fst_get_outsym);
			//net->print=&(print);
			wtk_fst_net_print_trans2(&(print),trans);
			//wtk_fst_net_print_trans_prev(net->print,trans);
			//wtk_fst_net_print_trans_next(net->print,trans);
			//exit(0);
		}
#endif
#ifdef DEUBG_B
		printf("a===>");
		wtk_fst_net_print_trans2(&(print),trans);
#endif
		//wtk_debug("hook=%p\n", trans->to_state->hook);
		//wtk_debug("trans=%p trans->in_id=%d out_id=%d state=%p a=%p\n", trans, trans->in_id, trans->out_id, trans->to_state, a);
		if(a)
		{
			//wtk_debug("a->in_id=%d\n",a->in_id);
			if(trans->in_id>0)
			{
				//wtk_fst_net_print_trans2(&(print),trans);
				wtk_e2fst_expand_tri_net_node4(e,pre,a,trans,(wtk_fst_state2_t*)trans->to_state);
			}else
			{
				wtk_e2fst_expand_tri_net_node2(e,pre,a,(wtk_fst_state2_t*)trans->to_state);
			}
		}else
		{
			if(trans->in_id==0)
				in_id=0;
			else
				in_id=wtk_e2fst_get_id(1,trans->in_id,0,0);    //note: trans->in_id=1(sil)
			ts=wtk_e2fst_pop_state(e);
			//tt=wtk_fst_net2_link_state(net,pre,ts,0,in_id,trans->out_id,0,0);
			//wtk_debug("b->in_id=%d out_id=%d\n", in_id, trans->out_id);
			tt=wtk_fst_net2_link_state(net,pre,ts,0,in_id,trans->out_id,trans->lm_like,trans->weight);
			wtk_e2fst_link_trans(e,trans,NULL,tt);
			if(in_id>0)
			{
				wtk_e2fst_expand_tri_net_node2(e,ts,trans,(wtk_fst_state2_t*)trans->to_state);
			}else
			{
				wtk_e2fst_expand_tri_net_node2(e,ts,NULL,(wtk_fst_state2_t*)trans->to_state);
			}
		}
	}
}

int wtk_e2fst_expand_tri_net(wtk_e2fst_t *e,wtk_fst_net2_t *mono_net)
{
	wtk_fst_state2_t *state;
	wtk_fst_net2_t *net=e->net;

	state=wtk_e2fst_pop_state(e);
	//state->hook=(void*)(long)-1;
	net->start=state;
	net->end=wtk_e2fst_pop_state(e);
	net->end->type=WTK_FST_FINAL_STATE;
	wtk_e2fst_expand_tri_net_node2(e,state,NULL,(wtk_fst_state2_t*)mono_net->start);
	return 0;
}


void wtk_e2fst_expand_pron_mono_wrd(wtk_e2fst_t *e,wtk_fst_net2_t *net,
		wtk_fst_state2_t *wrd_s,wtk_fst_state2_t *wrd_e,
		wtk_fst_trans2_t *trans)
{
	wtk_fst_trans2_t *tx=NULL;
	wtk_fst_state2_t *pre,*t;
	wtk_dict_pron_t *pron;
	wtk_dict_word_t *wrd;
	unsigned char* ids;
	wtk_fst_state2_t *first_pron_s;
	int out_id, link_oid;
	int x_id;
	int i, npron;
	float w=0.0f;

	//wrd=(wtk_dict_word_t*)trans->hook2;
	if(e->egram->cfg->use_ebnf)
	{
		wrd=((wtk_larc_t*)trans->hook2)->end->info.word;
		out_id=(long)wrd->aux;
	}else
	{
		wrd=(wtk_dict_word_t*)trans->hook2;
		out_id=trans->out_id;
	}

	if (e->cfg->add_filler && out_id > 2) {
		if (e->mono_cnt == 0) {
			e->out_id++;
			if (e->out_id == 124) {
				e->out_id++;
			}
		}
		e->mono_cnt++;
		// wtk_debug("%d %d
		// %d\n",e->mono_cnt,e->egram->xbnf->xbnf->wrds_cnt[e->wdec_cnt-1],e->out_id);
		if (e->mono_cnt ==
			e->egram->xbnf->xbnf->wrds_cnt[e->wdec_cnt - 1]) {
			e->mono_cnt = 0;
			e->wdec_cnt--;
		}
	}

	//wtk_debug("out_id=%d\n", out_id);
	first_pron_s=NULL;
	npron=0;
	for(pron=wrd->pron_list;pron;pron=pron->next)
		npron++;
	//wtk_debug("[%.*s] pron_list=%p npron=%d\n",wrd->name->len,wrd->name->data, wrd->pron_list, npron);
	for(pron=wrd->pron_list;pron;pron=pron->next)
	{
		ids=(unsigned char*)pron->pPhones;
		pre=wrd_s;
		//wtk_debug("%d\n",pron->nPhones);
		for(i=0;i<pron->nPhones;++i)
		{
			if(e->cfg->use_pre_wrd)
			{
				x_id=i==0?out_id:0;
			}else
			{
				x_id=(i==pron->nPhones-1)?out_id:0;
			}
			if(first_pron_s)
			{
				if(i==0)
				{
					tx=wtk_fst_state2_find_next(pre,ids[i],x_id,first_pron_s);
				}else
				{
					tx=wtk_fst_state2_find_next(pre,ids[i],x_id,NULL);
				}
//				if(tx)
//				{
//					pre=(wtk_fst_state2_t*)tx->to_state;
//					continue;
//				}
			}
			// wtk_debug("%d %d\n",ids[i],x_id);
			if (e->cfg->add_filler) {
				link_oid = e->out_id;
			} else {
				link_oid = x_id;
			}
			// wtk_debug("%d %d\n",ids[i],x_id);
			if(i==(pron->nPhones-1))
			{
				//wtk_debug("add sil=%d ids[%d]=%d\n",e->cfg->add_sil, i, ids[i]);
				if(e->cfg->add_sil &&(ids[i]!=e->cfg->sil_id && (0==e->cfg->use_posi || ids[i]!=e->cfg->sil_S_id)))
				{
					if(e->cfg->use_cross_wrd)
					{
						t=wtk_fst_net2_pop_state(net);
					}else
					{
						t=wtk_e2fst_pop_state(e);
					}
					//wtk_debug("add sil\n");
					if (x_id > 0)
					{
						w=SLOG(SEXP(trans->lm_like)/npron);
							wtk_fst_net2_link_state(
								net, pre, t, 0, ids[i],
								link_oid, w, 0);
					}
					else
						wtk_fst_net2_link_state(
							net, pre, t, 0, ids[i],
							link_oid, 0, 0);
					wtk_fst_net2_link_state(net,t,wrd_e,0,0,0,0,0);
					wtk_fst_net2_link_state(net,t,wrd_e,0,e->cfg->sil_id,0,0,0);
				}else
				{
					if (x_id > 0)
					{
						w=SLOG(SEXP(trans->lm_like)/npron);
						wtk_fst_net2_link_state(
							net, pre, wrd_e, 0, ids[i],
							link_oid, w, 0);
					}
					else
						wtk_fst_net2_link_state(
							net, pre, wrd_e, 0, ids[i],
							link_oid, 0, 0);
				}
				pre=wrd_e;
			}else
			{
				if(tx)
				{
					pre=(wtk_fst_state2_t*)tx->to_state;
					continue;
				}
				if(e->cfg->use_cross_wrd)
				{
					t=wtk_fst_net2_pop_state(net);
				}else
				{
					t=wtk_e2fst_pop_state(e);
				}
				//t=wtk_fst_net2_pop_state(net);
//				if(i==0)
//				{
//					wtk_fst_net2_link_state(net,pre,t,0,ids[i],x_id,0,0);
//				}else
//				{
//					wtk_fst_net2_link_state(net,pre,t,0,ids[i],0,0,0);
//				}
				if (x_id>0)
				{
					w=SLOG(SEXP(trans->lm_like)/npron);
					wtk_fst_net2_link_state(net, pre, t, 0,
											ids[i],
											link_oid, w, 0);
				}
				else
					wtk_fst_net2_link_state(
						net, pre, t, 0, ids[i], link_oid, 0, 0);
				pre=t;
			}
			if(i==0 && !first_pron_s)
			{
				first_pron_s=pre;
			}
		}
	}
	//wtk_debug("============\n");
}

void wtk_e2fst_expand_pron_mono_wrd_short(wtk_e2fst_t *e,wtk_fst_net2_t *net,
		wtk_fst_state2_t *wrd_s,wtk_fst_state2_t *wrd_e,
		wtk_fst_trans2_t *trans)
{
	wtk_fst_trans2_t *tx=NULL;
	wtk_fst_state2_t *pre,*t;
	wtk_dict_pron_t *pron;
	wtk_dict_word_t *wrd;
	unsigned short* ids;
	wtk_fst_state2_t *first_pron_s;
	int out_id;
	int x_id;
	int i, npron,nprons;
	float w=0.0f;

	//wrd=(wtk_dict_word_t*)trans->hook2;
	if(e->egram->cfg->use_ebnf)
	{
		wrd=((wtk_larc_t*)trans->hook2)->end->info.word;
		out_id=(long)wrd->aux;
	}else
	{
		wrd=(wtk_dict_word_t*)trans->hook2;
		out_id=trans->out_id;
	}
	//wtk_debug("out_id=%d\n", out_id);
	first_pron_s=NULL;
	npron=0;
	for(pron=wrd->pron_list;pron;pron=pron->next)
		npron++;
	//wtk_debug("[%.*s] pron_list=%p npron=%d\n",wrd->name->len,wrd->name->data, wrd->pron_list, npron);
	for(pron=wrd->pron_list;pron;pron=pron->next)
	{
		ids=(unsigned short*)pron->pPhones;
		pre=wrd_s;
		nprons = pron->nPhones/2;
		//wtk_debug("%d\n",pron->nPhones);
		for(i=0;i<nprons;++i)
		{
			if(e->cfg->use_pre_wrd)
			{
				x_id=i==0?out_id:0;
			}else
			{
				x_id=(i==nprons-1)?out_id:0;
			}
			if(first_pron_s)
			{
				if(i==0)
				{
					tx=wtk_fst_state2_find_next(pre,ids[i],x_id,first_pron_s);
				}else
				{
					tx=wtk_fst_state2_find_next(pre,ids[i],x_id,NULL);
				}
//				if(tx)
//				{
//					pre=(wtk_fst_state2_t*)tx->to_state;
//					continue;
//				}
			}
			//wtk_debug("%d\n",ids[i]);

			if(i==(nprons-1))
			{
				//wtk_debug("add sil=%d ids[%d]=%d\n",e->cfg->add_sil, i, ids[i]);
				if(e->cfg->add_sil &&(ids[i]!=e->cfg->sil_id && (0==e->cfg->use_posi || ids[i]!=e->cfg->sil_S_id)))
				{
					if(e->cfg->use_cross_wrd)
					{
						t=wtk_fst_net2_pop_state(net);
					}else
					{
						t=wtk_e2fst_pop_state(e);
					}
					//wtk_debug("add sil\n");
					if (x_id > 0)
					{
						w=SLOG(SEXP(trans->lm_like)/npron);
						wtk_fst_net2_link_state(net,pre,t,0,ids[i],x_id,w, 0);
					}
					else
						wtk_fst_net2_link_state(net,pre,t,0,ids[i],x_id,0,0);
					wtk_fst_net2_link_state(net,t,wrd_e,0,0,0,0,0);
					wtk_fst_net2_link_state(net,t,wrd_e,0,e->cfg->sil_id,0,0,0);
				}else
				{
					if (x_id > 0)
					{
						w=SLOG(SEXP(trans->lm_like)/npron);
						wtk_fst_net2_link_state(net,pre,wrd_e,0,ids[i],x_id,w,0);
					}
					else
						wtk_fst_net2_link_state(net,pre,wrd_e,0,ids[i],x_id,0,0);
				}
				pre=wrd_e;
			}else
			{
				if(tx)
				{
					pre=(wtk_fst_state2_t*)tx->to_state;
					continue;
				}
				if(e->cfg->use_cross_wrd)
				{
					t=wtk_fst_net2_pop_state(net);
				}else
				{
					t=wtk_e2fst_pop_state(e);
				}
				//t=wtk_fst_net2_pop_state(net);
//				if(i==0)
//				{
//					wtk_fst_net2_link_state(net,pre,t,0,ids[i],x_id,0,0);
//				}else
//				{
//					wtk_fst_net2_link_state(net,pre,t,0,ids[i],0,0,0);
//				}
				if (x_id>0)
				{
					w=SLOG(SEXP(trans->lm_like)/npron);
					wtk_fst_net2_link_state(net,pre,t,0,ids[i],x_id,w, 0);
				}
				else
					wtk_fst_net2_link_state(net,pre,t,0,ids[i],x_id,0,0);
				pre=t;
			}
			if(i==0 && !first_pron_s)
			{
				first_pron_s=pre;
			}
		}
	}
}

void wtk_e2fst_expand_pron_mono_wrd2(wtk_e2fst_t *e,wtk_fst_net2_t *net,
		wtk_fst_state2_t *wrd_s,wtk_fst_state2_t *wrd_e,
		wtk_fst_trans2_t *trans)
{
	wtk_fst_trans2_t *tx;
	wtk_fst_state2_t *pre,*t;
	wtk_dict_pron_t *pron;
	wtk_dict_word_t *wrd;
	wtk_fst_state2_t *first_pron_s;
	int out_id;
	int in_id;
	int x_id;
	int i;

	//wrd=(wtk_dict_word_t*)trans->hook2;

	if(e->egram->cfg->use_ebnf)
	{
		wrd=((wtk_larc_t*)trans->hook2)->end->info.word;
		out_id=(long)wrd->aux;
	}else
	{
		wrd=(wtk_dict_word_t*)trans->hook2;
		out_id=trans->out_id;
	}
	first_pron_s=NULL;
	wtk_debug("[%.*s]\n",wrd->name->len,wrd->name->data);
	for(pron=wrd->pron_list;pron;pron=pron->next)
	{
		//wtk_debug("npron=%d\n",pron->nPhones);
		//exit(0);
		pre=wrd_s;
		for(i=0;i<pron->nPhones;++i)
		{
			in_id=wtk_fst_insym_get_index(e->net->cfg->sym_in,pron->pPhones[i]->name);
			//wtk_debug("test[%.*s]=%d\n",pron->pPhones[i]->name->len,pron->pPhones[i]->name->data,in_id);
			//exit(0);
			if(e->cfg->use_pre_wrd)
			{
				x_id=i==0?out_id:0;
			}else
			{
				x_id=(i==pron->nPhones-1)?out_id:0;
			}
			if(first_pron_s)
			{
				if(i==0)
				{
					tx=wtk_fst_state2_find_next(pre,in_id,x_id,first_pron_s);
				}else
				{
					tx=wtk_fst_state2_find_next(pre,in_id,x_id,NULL);
				}
				if(tx)
				{
					pre=(wtk_fst_state2_t*)tx->to_state;
					continue;
				}
			}
			if(i==(pron->nPhones-1))
			{
				if(e->cfg->add_sil &&(in_id!=e->cfg->sil_id && (0==e->cfg->use_posi || in_id!=e->cfg->sil_S_id)))
				{
					if(e->cfg->use_cross_wrd)
					{
						t=wtk_fst_net2_pop_state(net);
					}else
					{
						t=wtk_e2fst_pop_state(e);
					}
					wtk_fst_net2_link_state(net,pre,t,0,in_id,x_id,0,0);

					wtk_fst_net2_link_state(net,t,wrd_e,0,0,0,0,0);
					wtk_fst_net2_link_state(net,t,wrd_e,0,e->cfg->sil_id,0,0,0);
				}else
				{
					//wtk_debug("x_id=%d:%d %d=>%d\n",in_id,x_id,pre->id,wrd_e->id);
					wtk_fst_net2_link_state(net,pre,wrd_e,0,in_id,x_id,0,0);
				}
				pre=wrd_e;
			}else
			{
				if(e->cfg->use_cross_wrd)
				{
					t=wtk_fst_net2_pop_state(net);
				}else
				{
					//wtk_debug("pop state\n");
					t=wtk_e2fst_pop_state(e);
				}
				//t=wtk_fst_net2_pop_state(net);
//				if(i==0)
//				{
//					wtk_fst_net2_link_state(net,pre,t,0,in_id,x_id,0,0);
//				}else
//				{
//					wtk_fst_net2_link_state(net,pre,t,0,in_id,0,0,0);
//				}
				wtk_fst_net2_link_state(net,pre,t,0,in_id,x_id,0,0);
				pre=t;
			}
			if(i==0 && !first_pron_s)
			{
				first_pron_s=pre;
			}
		}
	}
}

void wtk_e2fst_expand_pron_mono_wrd_eval(wtk_e2fst_t *e,wtk_fst_net2_t *net,
		wtk_fst_state2_t *wrd_s,wtk_fst_state2_t *wrd_e,
		wtk_fst_trans2_t *trans)
{
	wtk_fst_trans2_t *tx=NULL;
	wtk_fst_state2_t *pre,*t;
	wtk_dict_pron_t *pron;
	wtk_dict_word_t *wrd;
	unsigned char* ids;
	wtk_fst_state2_t *first_pron_s;
	int out_id;
	int x_id;
	int i, npron;
	float w=0.0f;

	//wrd=(wtk_dict_word_t*)trans->hook2;
	if(e->egram->cfg->use_ebnf)
	{
		wrd=((wtk_larc_t*)trans->hook2)->end->info.word;
		out_id=(long)wrd->aux;
	}else
	{
		wrd=(wtk_dict_word_t*)trans->hook2;
		out_id=trans->out_id;
	}
	//wtk_debug("out_id=%d\n", out_id);
	first_pron_s=NULL;
	//wtk_debug("[%.*s]\n",wrd->name->len,wrd->name->data);
	npron=0;
	for(pron=wrd->pron_list;pron;pron=pron->next)
		npron++;
	for(pron=wrd->pron_list;pron;pron=pron->next)
	{
		ids=(unsigned char*)pron->pPhones;
		pre=wrd_s;
		for(i=0;i<pron->nPhones;++i)
		{
			if(e->cfg->use_pre_wrd)
			{
				x_id=i==0?out_id:0;
			}else
			{
				x_id=(i==pron->nPhones-1)?out_id:0;
			}
			if(first_pron_s)
			{
				if(i==0)
				{
					tx=wtk_fst_state2_find_next(pre,ids[i],x_id,first_pron_s);
				}else
				{
					tx=wtk_fst_state2_find_next(pre,ids[i],x_id,NULL);
				}
//				if(tx)
//				{
//					pre=(wtk_fst_state2_t*)tx->to_state;
//					continue;
//				}
			}
			//wtk_debug("add sil=%d sil_id=%d sil_S_id=%d ids[%d]=%d x_id=%d nphn=%d\n",e->cfg->add_sil, e->cfg->sil_id, e->cfg->sil_S_id, i, ids[i], x_id, pron->nPhones);
			if(i==(pron->nPhones-1))
			{
				//if(e->cfg->add_sil &&(ids[i]!=e->cfg->sil_id && (0==e->cfg->use_posi || ids[i]!=e->cfg->sil_S_id)))
				//if(e->cfg->add_sil && wrd_e->type != WTK_FST_FINAL_STATE)
				if(e->cfg->add_sil)
				{
					if(e->cfg->use_cross_wrd)
					{
						t=wtk_fst_net2_pop_state(net);
					}else
					{
						t=wtk_e2fst_pop_state(e);
					}

					//wtk_debug("add sil\n");
					if (x_id > 0)
					{
						w=SLOG(SEXP(trans->lm_like)/npron);
						//wtk_debug("w=%f trans=%p\n", trans->lm_like, trans);
						wtk_fst_net2_link_state(net,pre,t,0,ids[i],x_id,w, 0);
					}
					else
						wtk_fst_net2_link_state(net,pre,t,0,ids[i],x_id,0,0);
					//wtk_debug("pre=%p t=%p wrd_e=%p type=%d idx[%d]=%d x_id=%d\n",pre, t, wrd_e,wrd_e->type, i, ids[i], x_id);
					wtk_fst_net2_link_state(net,t,wrd_e,0,0,0,0,0);
					wtk_fst_net2_link_state(net,t,wrd_e,0,e->cfg->sil_id,0,0,0);
				}else
				{
					if (x_id > 0)
					{
						w=SLOG(SEXP(trans->lm_like)/npron);
						wtk_fst_net2_link_state(net,pre,wrd_e,0,ids[i],x_id,w,0);
					}
					else
						wtk_fst_net2_link_state(net,pre,wrd_e,0,ids[i],x_id,0,0);
				}
				pre=wrd_e;
			}else
			{
				if(tx)
				{
					pre=(wtk_fst_state2_t*)tx->to_state;
					continue;
				}
				if(e->cfg->use_cross_wrd)
				{
					t=wtk_fst_net2_pop_state(net);
				}else
				{
					t=wtk_e2fst_pop_state(e);
				}
				//t=wtk_fst_net2_pop_state(net);
//				if(i==0)
//				{
//					wtk_fst_net2_link_state(net,pre,t,0,ids[i],x_id,0,0);
//				}else
//				{
//					wtk_fst_net2_link_state(net,pre,t,0,ids[i],0,0,0);
//				}
				if (x_id>0)
				{
					w=SLOG(SEXP(trans->lm_like)/npron);
					//wtk_debug("w=%f trans=%p\n", trans->lm_like, trans);
					wtk_fst_net2_link_state(net,pre,t,0,ids[i],x_id,w, 0);
				}
				else
					wtk_fst_net2_link_state(net,pre,t,0,ids[i],x_id,0,0);
				pre=t;
			}
			if(i==0 && !first_pron_s)
			{
				first_pron_s=pre;
			}
		}
	}
}

void wtk_e2fst_expand_pron_mono_node(wtk_e2fst_t *e,wtk_fst_net2_t *net,wtk_fst_state2_t *wrd_s,
		wtk_fst_state2_t *from)
{
	wtk_fst_trans2_t *trans;
	wtk_fst_state2_t *wrd_e;

        // wtk_debug("state=%d\n",from->id);
        for(trans=(wtk_fst_trans2_t*)from->v.trans;trans;trans=(wtk_fst_trans2_t*)trans->hook.next)
	{
		if(trans->to_state->hook)
		{
			wrd_e=(wtk_fst_state2_t*)trans->to_state->hook;
		}else
		{
			if(e->cfg->use_cross_wrd)
			{
				wrd_e=wtk_fst_net2_pop_state(net);
			}else
			{
				wrd_e=wtk_e2fst_pop_state(e);
			}
		}
		//wtk_debug("type=%d\n",wrd_e->type);
		//wtk_debug("in=%d out=%d\n",trans->in_id, trans->out_id);
		if(trans->in_id>0)
		{
			if(e->egram->cfg->use_bin)
			{
				if(e->egram->cfg->e2fst.use_eval)
				{
					wtk_e2fst_expand_pron_mono_wrd_eval(e,net,wrd_s,wrd_e,trans);
				}
				else if (e->cfg->type == 0)
				{
					wtk_e2fst_expand_pron_mono_wrd(e,net,wrd_s,wrd_e,trans);
				}else
				{
					wtk_e2fst_expand_pron_mono_wrd_short(e,net,wrd_s,wrd_e,trans);
				}
			}else
			{
				wtk_e2fst_expand_pron_mono_wrd2(e,net,wrd_s,wrd_e,trans);
			}
		}else
		{
			//if(!wtk_fst_state2_has_eps_to(wrd_s,wrd_e))
			{
				wtk_fst_net2_link_state(net,wrd_s,wrd_e,0,0,0,0,0);
			}
		}
		if(!trans->to_state->hook)
		{
			trans->to_state->hook=wrd_e;
			wtk_e2fst_expand_pron_mono_node(e,net,wrd_e,(wtk_fst_state2_t*)trans->to_state);
		}
	}
}



void wtk_e2fst_expand_pron_mono(wtk_e2fst_t *e,wtk_fst_net2_t *input,wtk_fst_net2_t *output)
{
	wtk_fst_state2_t *state;

	//wtk_debug("expand mono \n");
	if(e->cfg->use_cross_wrd)
	{
		state=wtk_fst_net2_pop_state(output);
	}else
	{
		state=wtk_e2fst_pop_state(e);
	}
	output->start=state;
	if(e->cfg->use_cross_wrd)
	{
		state=wtk_fst_net2_pop_state(output);
	}else
	{
		state=wtk_e2fst_pop_state(e);
	}
	output->end=state;
	output->end->type=WTK_FST_FINAL_STATE;

	input->end->hook=output->end;
	input->start->hook=state;
	wtk_e2fst_expand_pron_mono_node(e,output,output->start,input->start);
}

#include "wtk/core/wtk_robin.h"

void wtk_e2fst_link_state_roboin(wtk_fst_net2_t *net,wtk_fst_state2_t *s,
		wtk_fst_state2_t *e,unsigned int frame,unsigned int in_id,unsigned int out_id,
		wtk_robin_t *rb)
{
	wtk_fst_trans2_t *tt;

	if(rb->used>0)
	{
		tt=wtk_robin_pop(rb);
		//tt->in_id=0;
		//tt->out_id=0;
		wtk_fst_net2_link_state2(net,s,e,tt,frame,in_id,out_id,0,0);
	}else
	{
		wtk_fst_net2_link_state(net,s,e,frame,in_id,out_id,0,0);
	}
}

void wtk_e2fst_remove_dup_out(wtk_e2fst_t *e)
{
	wtk_fst_trans2_t *trans;
	wtk_fst_state2_t *s;
	wtk_fst_state2_t *ts;
	wtk_slist_node_t *sn;
	wtk_fst_trans2_t *tt,*tv,*tx;
	wtk_robin_t *rb;

	rb=wtk_robin_new(1024);
	for(sn=e->state_l.prev;sn;sn=sn->prev)
	{
		s=data_offset(sn,wtk_fst_state2_t,q_n);
		if(s->type!=WTK_FST_FINAL_STATE)
		{
			if(s->v.trans && s->v.trans->hook.next)
			{
				trans=(wtk_fst_trans2_t*)s->v.trans;
				tx=trans;
				s->v.trans=NULL;
				for(;trans;trans=(wtk_fst_trans2_t*)trans->hook.next)
				{
					if(trans->in_id==0)
					{
						wtk_e2fst_link_state_roboin(e->net,s,(wtk_fst_state2_t*)trans->to_state,trans->frame,trans->in_id,
								trans->out_id,rb);
					}else
					{
						for(tv=NULL,tt=(wtk_fst_trans2_t*)s->v.trans;tt;tt=(wtk_fst_trans2_t*)tt->hook.next)
						{
							if(tt->in_id==trans->in_id && tt->out_id==trans->out_id)
							{
								tv=tt;
								break;
							}
						}
						if(tv)
						{
							ts=(wtk_fst_state2_t*)tv->to_state;
						}else
						{
							ts=wtk_e2fst_pop_state(e);
							wtk_e2fst_link_state_roboin(e->net,s,ts,trans->frame,trans->in_id,trans->out_id,rb);
						}
						if(ts!=(void*)trans->to_state)
						{
							wtk_e2fst_link_state_roboin(e->net,ts,(wtk_fst_state2_t*)trans->to_state,0,0,0,rb);
						}
					}
				}
				trans=tx;
				for(;trans;trans=(wtk_fst_trans2_t*)trans->hook.next)
				{
					if(rb->used<rb->nslot)
					{
						wtk_robin_push(rb,trans);
					}else
					{
						break;
					}
				}
			}
		}
	}
	wtk_robin_delete(rb);
}

void wtk_e2fst_convert_id(wtk_e2fst_t *e)
{
	wtk_fst_trans2_t *trans;
	wtk_fst_state2_t *s;
	wtk_slist_node_t *sn;

	for(sn=e->state_l.prev;sn;sn=sn->prev)
	{
		s=data_offset(sn,wtk_fst_state2_t,q_n);
		if(s->type!=WTK_FST_FINAL_STATE)
		{
			for(trans=(wtk_fst_trans2_t*)s->v.trans;trans;trans=(wtk_fst_trans2_t*)trans->hook.next)
			{
				if(trans->in_id>0)
				{
					//wtk_debug("trans->in_id=%d\n", trans->in_id);
					((wtk_fst_trans2_t*)trans)->frame=(int)(trans->in_id);
					trans->in_id=wtk_e2fst_get_phnid2(e,trans->in_id);
					//wtk_debug("trans->in_id=%d\n", trans->in_id);
				}
			}
		}
	}
}

void wtk_e2fst_convert_id_ger(wtk_e2fst_t *e)
{
	wtk_fst_trans2_t *trans;
	wtk_fst_state2_t *s;
	wtk_slist_node_t *sn;

	for(sn=e->state_l.prev;sn;sn=sn->prev)
	{
		s=data_offset(sn,wtk_fst_state2_t,q_n);
		if(s->type!=WTK_FST_FINAL_STATE)
		{
			for(trans=(wtk_fst_trans2_t*)s->v.trans;trans;trans=(wtk_fst_trans2_t*)trans->hook.next)
			{
				if(trans->in_id>0)
				{
					//wtk_debug("trans->in_id=%d\n", trans->in_id);
					((wtk_fst_trans2_t*)trans)->frame=(int)(trans->in_id);//TODO pay attention
					trans->in_id=wtk_e2fst_get_phnid3(e,trans->in_id);
					//wtk_debug("trans->in_id=%d\n", trans->in_id);
				}
			}
		}
	}
}

void wtk_e2fst_convert_id2(wtk_e2fst_t *e)
{
	wtk_fst_trans2_t *trans;
	wtk_fst_state2_t *s;
	wtk_slist_node_t *sn;
	char phns[3];

	phns[0]=0;
	phns[2]=0;
	for(sn=e->state_l.prev;sn;sn=sn->prev)
	{
		s=data_offset(sn,wtk_fst_state2_t,q_n);
		if(s->type!=WTK_FST_FINAL_STATE)
		{
			for(trans=(wtk_fst_trans2_t*)s->v.trans;trans;trans=(wtk_fst_trans2_t*)trans->hook.next)
			{
				if(trans->in_id>0)
				{
					((wtk_fst_trans2_t*)trans)->frame=(int)(trans->in_id);
					phns[1]=trans->in_id;
					//wtk_debug("id=%d\n",trans->in_id);
					if(trans->in_id==e->cfg->sil_id)
					{
						trans->in_id=e->sil_in_id;
					}if(e->cfg->use_posi && trans->in_id==e->cfg->sil_S_id)
					{
						trans->in_id=e->sil_S_in_id;
					}else
					{
						trans->in_id=wtk_e2fst_get_phnid(e,phns,3);
					}
					//wtk_debug("id=%d\n",trans->in_id);
					//exit(0);
				}
			}
		}
	}
}


void wtk_e2fst_check_dup(wtk_e2fst_t *e)
{
	wtk_fst_trans_t *trans;
	wtk_fst_state2_t *s;
	wtk_slist_node_t *sn;
	wtk_fst_trans_t *t;
	int cnt;
	int nx=0;
	int b;

	cnt=0;
	for(sn=e->state_l.prev;sn;sn=sn->prev)
	{
		s=data_offset(sn,wtk_fst_state2_t,q_n);
		if(s->type!=WTK_FST_FINAL_STATE)
		{
			for(trans=s->v.trans;trans;trans=trans->hook.next)
			{
				++nx;
				if(trans->in_id==0 && trans->out_id==0)
				{
					continue;
				}
				for(t=trans->hook.next;t;t=t->hook.next)
				{
					if(t->in_id==trans->in_id && t->out_id==trans->out_id)
					{
						b=wtk_fst_state2_has_sampe_input((wtk_fst_state2_t*)t->to_state,(wtk_fst_state2_t*)trans->to_state);
						if(b)
						{
							cnt+=1;
						}
						break;
					}
				}
			}
		}
	}
	wtk_debug("nx=%d cnt=%d\n",nx,cnt);
	//exit(0);
}


void wtk_e2fst_remove_dup_trans(wtk_e2fst_t *e,wtk_fst_state2_t *s)
{
	wtk_fst_trans2_t *t1,*t2,*pre;//,*t3;
	//int b;
	//int cnt=0;

	//wtk_debug("idx=%d\n",s->id);
	s->hook=s;
	if(s->type==WTK_FST_FINAL_STATE){return;}
	for(t1=(wtk_fst_trans2_t*)s->v.trans;t1;t1=(wtk_fst_trans2_t*)t1->hook.next)
	{
		if(!((wtk_fst_state2_t*)t1->to_state)->in_prev->in_prev)
		{
			pre=t1;
			for(t2=(wtk_fst_trans2_t*)t1->hook.next;t2;)
			{
				if(((wtk_fst_state2_t*)t2->to_state)->in_prev->in_prev)
				{
					t2=(wtk_fst_trans2_t*)t2->hook.next;
					pre=t2;
					continue;
				}
				if(t1->in_id==t2->in_id && t1->out_id==t2->out_id)// && t1->from_state==t2->from_state)
				{
					if(t1->to_state==t2->to_state)
					{
						pre->hook.next=t2->hook.next;
						t2=(wtk_fst_trans2_t*)t2->hook.next;
						//pre->hook.next=t2->hook.next;
						//wtk_debug("%p:%p\n",t1->to_state,t2->to_state);
						//exit(0);
					}else
					{
#ifndef DEBUG_USE_NXT
						wtk_fst_trans2_t *t3;

						if(t2->to_state->type==WTK_FST_FINAL_STATE)
						{
							wtk_fst_net2_link_state(e->net,(wtk_fst_state2_t*)t1->to_state,
									(wtk_fst_state2_t*)t2->to_state,0,0,0,0,0);
						}else
						{
							for(t3=(wtk_fst_trans2_t*)t2->to_state->v.trans;t3;t3=(wtk_fst_trans2_t*)t3->hook.next)
							{
								if(t3->to_state==t1->to_state)
								{
									//逻辑判断，never be here
									continue;
								}
								if(t3->in_id==0 && t3->out_id==0 && wtk_fst_state2_has_eps_to((wtk_fst_state2_t*)t3->to_state,
										(wtk_fst_state2_t*)t1->to_state))
								{
									//逻辑判断，never be here
									continue;
								}
								wtk_fst_net2_link_state(e->net,(wtk_fst_state2_t*)t1->to_state,
										(wtk_fst_state2_t*)t3->to_state,0,t3->in_id,t3->out_id,0,0);
							}
							if(t1->to_state->hook)
							{
								t1->to_state->hook=NULL;
							}
							t2->to_state->v.trans=NULL;
							t2->to_state->hook=t2;
						}
						pre->hook.next=t2->hook.next;
						t2=(wtk_fst_trans2_t*)t2->hook.next;

#else
						if(!wtk_fst_state2_has_eps_to((wtk_fst_state2_t*)t2->to_state,(wtk_fst_state2_t*)t1->to_state))
						{
							//wtk_debug("prev=%p:%p\n",t1->in_prev,t2->in_prev);

							wtk_fst_net2_link_state(e->net,(wtk_fst_state2_t*)t1->to_state,
									(wtk_fst_state2_t*)t2->to_state,0,0,0,0,0);

							pre->hook.next=t2->hook.next;
							t2=(wtk_fst_trans2_t*)t2->hook.next;
						}else
						{
							pre=t2;
							t2=(wtk_fst_trans2_t*)t2->hook.next;
						}
#endif
					}
#ifdef DEBUG_X
					//wtk_debug("foudn\n");
					if(t1->to_state==t2->to_state)
					{
						pre->hook.next=t2->hook.next;
						t2=(wtk_fst_trans2_t*)t2->hook.next;
						//pre->hook.next=t2->hook.next;
						//wtk_debug("%p:%p\n",t1->to_state,t2->to_state);
						//exit(0);
						//wtk_debug("dup\n");
					}else
					{
						b=wtk_fst_state2_has_sampe_input((wtk_fst_state2_t*)t1->to_state,(wtk_fst_state2_t*)t2->to_state);
						if(b)
						{
							pre->hook.next=t2->hook.next;
							wtk_fst_net2_link_state(e->net,(wtk_fst_state2_t*)t1->to_state,
									(wtk_fst_state2_t*)t2->to_state,0,0,0,0,0);
							//wtk_debug("b=%d %d:%d %d:%d\n",b,t1->in_id,t1->out_id,t1->to_state->ntrans,t2->to_state->ntrans);
							//wtk_debug("%p:%p\n",t1->to_state,t2->to_state);
							//exit(0);
							//t2=NULL;
							//break;
							t2=(wtk_fst_trans2_t*)t2->hook.next;
						}else
						{
							t2=(wtk_fst_trans2_t*)t2->hook.next;
							pre=t2;
						}
					}
#endif
				}else
				{
					pre=t2;
					t2=(wtk_fst_trans2_t*)t2->hook.next;
					//wtk_debug("foudn t2=%p\n",t2);
				}
			}
		}
	}
	for(t1=(wtk_fst_trans2_t*)s->v.trans;t1;t1=(wtk_fst_trans2_t*)t1->hook.next)
	{
		if(!t1->to_state->hook)
		{
			wtk_e2fst_remove_dup_trans(e,(wtk_fst_state2_t*)t1->to_state);
		}
	}
}

void wtk_e2fst_clean_hook(wtk_e2fst_t *e)
{
	wtk_slist_node_t *sn;
	wtk_fst_state2_t *s;

	for(sn=e->state_l.prev;sn;sn=sn->prev)
	{
		s=data_offset(sn,wtk_fst_state2_t,q_n);
		s->hook=NULL;
	}
}

void wtk_e2fst_clean_filler_hook(wtk_e2fst_t *e) {
    wtk_slist_node_t *sn;
    wtk_fst_state2_t *s;

    for (sn = e->filler_l.prev; sn; sn = sn->prev) {
        s = data_offset(sn, wtk_fst_state2_t, q_n);
        s->hook = NULL;
    }
}

void wtk_e2fst_remove_dup(wtk_e2fst_t *e)
{
	wtk_fst_state2_t* s;

	wtk_e2fst_clean_hook(e);
	s=e->net->start;
	wtk_e2fst_remove_dup_trans(e,s);
}

void wtk_e2fst_write_mono(wtk_e2fst_t *e)
{
	wtk_fst_net_print_t print;

	wtk_fst_net_print_init(&print,e,(wtk_fst_net_get_sym_f)wtk_e2fst_get_insym,
			(wtk_fst_net_get_sym_f)wtk_e2fst_get_outsym);
	e->mono_net->print=&(print);
	wtk_fst_net2_write_lat(e->mono_net,"mono.lat");
}

int wtk_e2fst_find_dup_state_trans(wtk_e2fst_t *e,wtk_fst_state2_t *s,wtk_fst_trans2_t *t)
{
	wtk_fst_trans2_t *trans;
	int b;

	for(trans=(wtk_fst_trans2_t*)(s->v.trans);trans;trans=(wtk_fst_trans2_t*)(trans->hook.next))
	{
		if(trans!=t)
		{
			if(trans->in_id==0 && trans->out_id==0)
			{
				b=wtk_e2fst_find_dup_state_trans(e,(wtk_fst_state2_t*)(trans->to_state),t);
				if(b){return 1;}
			}else if(trans->in_id==t->in_id && trans->out_id==t->out_id)
			{
				//wtk_debug("%d=>%d %d=>%d\n",trans->from_state->id,trans->to_state->id,t->from_state->id,t->to_state->id);
				//wtk_debug("trans=%p/%p %d:%d\n",trans,t,t->in_id,t->out_id);
				return 1;
			}
		}
	}
	return 0;
}

void wtk_e2fst_find_dup_state(wtk_e2fst_t *e,wtk_fst_state2_t *s)
{
	wtk_fst_trans2_t *trans;
	int b;
	static int cnt=0;
	static int tx=0;

	for(trans=(wtk_fst_trans2_t*)(s->v.trans);trans;trans=(wtk_fst_trans2_t*)(trans->hook.next))
	{
		if(trans->in_id!=0 || trans->out_id!=0)
		{
			b=wtk_e2fst_find_dup_state_trans(e,s,trans);
			//if(b)
			{
				//wtk_debug("found dup %d/%d b=%d\n",trans->in_id,trans->out_id,b);
				if(b)
				{
					++cnt;
					wtk_debug("cnt=%d/%d\n",cnt,tx);
					//exit(0);
				}else
				{
					++tx;
				}
			}
		}
	}
	for(trans=(wtk_fst_trans2_t*)(s->v.trans);trans;trans=(wtk_fst_trans2_t*)(trans->hook.next))
	{
		wtk_e2fst_find_dup_state(e,(wtk_fst_state2_t*)(trans->to_state));
	}
}

void wtk_e2fst_find_dup(wtk_e2fst_t *e)
{
	wtk_e2fst_find_dup_state(e,e->net->start);
}

void wtk_e2fst_expand_bi_net_node2(wtk_e2fst_t *e,wtk_fst_state2_t* pre,
		wtk_fst_trans2_t *a,wtk_fst_state2_t *nxt_state)//pre:biphone net state a:mono phone trans nxt_state:mono phone state
{
	wtk_fst_net2_t *net=e->net;
	wtk_fst_trans2_t *trans,*tt;
	wtk_fst_state2_t *ts;
	int in_id, out_id;
	if(nxt_state->type == WTK_FST_FINAL_STATE)
	{
		if(a)
		{
                    // in_id=wtk_e2fst_get_id(1,0,a->in_id,0);
                    in_id = (3 << 24) + (a->in_id << 8);
                    out_id = a->out_id;
		}
		else
		{
			in_id=0;
			out_id=0;
		}
		//wtk_debug("%d %d\n",in_id,a->in_id);
		//wtk_debug("biphn from=%d to=%d in=%d out=%d\n", pre->id, net->end->id, in_id, out_id);
		wtk_fst_net2_link_state(net,pre,net->end,0,in_id,out_id,0,0);
                // wtk_debug("add trans from %d to %d %d
                // %d\n",pre->id,net->end->id,in_id,out_id);
        }

	for(trans=(wtk_fst_trans2_t*)nxt_state->v.trans;trans;trans=(wtk_fst_trans2_t*)trans->hook.next)
	{
		//wtk_debug("from=%d to=%d in=%d out=%d\n", trans->from_state->id, trans->to_state->id, trans->in_id, trans->out_id);
		if(a)
		{
			if(trans->in_id > 0)
			{
                            // wtk_e2fst_expand_bi_net_node4(e,pre,a,trans,(wtk_fst_state2_t*)trans->to_state);
                            // wtk_debug("%p %p %p
                            // %d\n",trans,trans->to_state,trans->to_state->hook,trans->to_state->id);
                            if (!trans->to_state->hook) {
                                ts = wtk_e2fst_pop_state(e);
                                trans->to_state->hook = ts;
                                // if(a->in_id == trans->in_id && trans->in_id
                                // == 1)
                                if (a->in_id == trans->in_id &&
                                    (trans->in_id == e->cfg->sil_id ||
                                     (e->cfg->use_posi &&
                                      trans->in_id == e->cfg->sil_S_id))) {
                                    in_id = wtk_e2fst_get_id(1, 1, 0, 0);
                                } else {
                                    in_id = (3 << 24) + (a->in_id << 16) +
                                            (trans->in_id << 8);
                                }
                                // tt=wtk_fst_net2_link_state(net,pre,ts,0,in_id,trans->out_id,0,0);
                                // wtk_debug("biphn from=%d to=%d in=%d out=%d
                                // lmlike=%f weight=%f\n", pre->id, ts->id,
                                // in_id,
                                // trans->out_id,trans->lm_like,trans->weight);
                                tt = wtk_fst_net2_link_state(
                                    net, pre, ts, 0, in_id, trans->out_id,
                                    trans->lm_like, trans->weight);
                                // wtk_debug("add trans from %d to %d    %d %d
                                // %d
                                // %d\n",pre->id,ts->id,a->in_id,trans->in_id,in_id,trans->out_id);
                                wtk_e2fst_link_trans(e, trans, NULL, tt); // TODO
                                wtk_e2fst_expand_bi_net_node2(
                                    e, ts, trans,
                                    (wtk_fst_state2_t *)trans->to_state);
				}else
				{
					in_id=(3<<24)+(a->in_id<<16)+(trans->in_id<<8);
					//tt=wtk_fst_net2_link_state(net,pre,(wtk_fst_state2_t*)trans->to_state->hook,0,in_id,trans->out_id,0,0);
					//wtk_debug("biphn from=%d to=%d in=%d out=%d lmlike=%f weight=%f\n", pre->id, ((wtk_fst_state2_t*)trans->to_state->hook)->id, in_id, trans->out_id,trans->lm_like,trans->weight);
					tt=wtk_fst_net2_link_state(net,pre,(wtk_fst_state2_t*)trans->to_state->hook,0,in_id,trans->out_id,trans->lm_like,trans->weight);
					//wtk_debug("add trans from %d to %d    %d %d  %d %d\n",tt->from_state->id,tt->to_state->id,a->in_id,trans->in_id,in_id,trans->out_id);
				}
			}else
			{
                            // wtk_debug("1111 %d\n",trans->to_state->id);
                            wtk_e2fst_expand_bi_net_node2(
                                e, pre, a, (wtk_fst_state2_t *)trans->to_state);
			}
		}else
		{
			in_id=wtk_e2fst_get_id(1,trans->in_id,0,0);
			ts=wtk_e2fst_pop_state(e);
			//tt=wtk_fst_net2_link_state(net,pre,ts,0,in_id,trans->out_id,0,0);
			//wtk_debug("biphn from=%d to=%d in=%d out=%d\n", pre->id, ts->id, in_id, trans->out_id);
			tt=wtk_fst_net2_link_state(net,pre,ts,0,in_id,trans->out_id,trans->lm_like,trans->weight);
			//wtk_debug("%d\n",in_id);
			//wtk_debug("add trans from %d to %d\n",pre->id,ts->id);
			wtk_e2fst_link_trans(e,trans,NULL,tt);//TODO
			if(in_id>0)
			{
				wtk_e2fst_expand_bi_net_node2(e,ts,trans,(wtk_fst_state2_t*)trans->to_state);
			}else
			{
				wtk_e2fst_expand_bi_net_node2(e,ts,NULL,(wtk_fst_state2_t*)trans->to_state);
			}
		}
	}
}

void wtk_e2fst_expand_bi_net_node3(wtk_e2fst_t *e,wtk_fst_state2_t* pre,
		wtk_fst_trans2_t *a,wtk_fst_state2_t *nxt_state)//pre:biphone net state a:mono phone trans nxt_state:mono phone state
{
	wtk_fst_net2_t *net=e->net;
	wtk_fst_trans2_t *trans,*tt;
	wtk_fst_state2_t *ts;
	int in_id, out_id;

	if(nxt_state->type == WTK_FST_FINAL_STATE)
	{
		if(a)
		{
			in_id=wtk_e2fst_get_id2(1,a->in_id);
			out_id=a->out_id;
		}
		else
		{
			in_id=0;
			out_id=0;
		}
		//wtk_debug("%d %d\n",in_id,a->in_id);
		//wtk_debug("biphn from=%d to=%d in=%d out=%d\n", pre->id, net->end->id, in_id, out_id);
		wtk_fst_net2_link_state(net,pre,net->end,0,in_id,out_id,0,0);
		//wtk_debug("add trans from %d to %d\n",pre->id,net->end->id);
	}

	for(trans=(wtk_fst_trans2_t*)nxt_state->v.trans;trans;trans=(wtk_fst_trans2_t*)trans->hook.next)
	{
		//wtk_debug("from=%d to=%d in=%d out=%d\n", trans->from_state->id, trans->to_state->id, trans->in_id, trans->out_id);
		if(a)
		{
			if(trans->in_id > 0)
			{
				//wtk_e2fst_expand_bi_net_node4(e,pre,a,trans,(wtk_fst_state2_t*)trans->to_state);
				//wtk_debug("%p %p %d\n",trans,trans->to_state->hook,trans->to_state->id);
				if(!trans->to_state->hook)
				{
					ts=wtk_e2fst_pop_state(e);
					trans->to_state->hook=ts;
					//if(a->in_id == trans->in_id && trans->in_id == 1)
					if(a->in_id == trans->in_id && (trans->in_id == e->cfg->sil_id|| (e->cfg->use_posi && trans->in_id == e->cfg->sil_S_id)))
					{
						in_id=wtk_e2fst_get_id2(1,1);
					}else
					{
						//in_id=(3<<24)+(a->in_id<<16)+(trans->in_id<<8);//TODO
						in_id=(3<<24)+(a->in_id<<9)+  trans->in_id;
						//wtk_debug("%d %d %d\n",in_id,a->in_id,trans->in_id);
					}
					//tt=wtk_fst_net2_link_state(net,pre,ts,0,in_id,trans->out_id,0,0);
					//wtk_debug("biphn from=%d to=%d in=%d out=%d lmlike=%f weight=%f\n", pre->id, ts->id, in_id, trans->out_id,trans->lm_like,trans->weight);
					tt=wtk_fst_net2_link_state(net,pre,ts,0,in_id,trans->out_id,trans->lm_like,trans->weight);
					//wtk_debug("add trans from %d to %d    %d %d   %d %d\n",pre->id,ts->id,a->in_id,trans->in_id,in_id,trans->out_id);
					wtk_e2fst_link_trans(e,trans,NULL,tt);//TODO
					wtk_e2fst_expand_bi_net_node3(e,ts,trans,(wtk_fst_state2_t*)trans->to_state);
				}else
				{
					//in_id=(3<<24)+(a->in_id<<16)+(trans->in_id<<8);
					in_id=(3<<24)+(a->in_id<<9)+  trans->in_id;
					//wtk_debug("%d %d %d\n",in_id,a->in_id,trans->in_id);
					//tt=wtk_fst_net2_link_state(net,pre,(wtk_fst_state2_t*)trans->to_state->hook,0,in_id,trans->out_id,0,0);
					//wtk_debug("biphn from=%d to=%d in=%d out=%d lmlike=%f weight=%f\n", pre->id, ((wtk_fst_state2_t*)trans->to_state->hook)->id, in_id, trans->out_id,trans->lm_like,trans->weight);
					tt=wtk_fst_net2_link_state(net,pre,(wtk_fst_state2_t*)trans->to_state->hook,0,in_id,trans->out_id,trans->lm_like,trans->weight);
					//wtk_debug("add trans from %d to %d    %d %d  %d %d\n",tt->from_state->id,tt->to_state->id,a->in_id,trans->in_id,in_id,trans->out_id);
				}
			}else
			{
				wtk_e2fst_expand_bi_net_node3(e,pre,a,(wtk_fst_state2_t*)trans->to_state);
			}
		}else
		{
			in_id=wtk_e2fst_get_id2(1,trans->in_id);
			ts=wtk_e2fst_pop_state(e);
			//tt=wtk_fst_net2_link_state(net,pre,ts,0,in_id,trans->out_id,0,0);
			//wtk_debug("biphn from=%d to=%d in=%d out=%d\n", pre->id, ts->id, in_id, trans->out_id);
			tt=wtk_fst_net2_link_state(net,pre,ts,0,in_id,trans->out_id,trans->lm_like,trans->weight);
			//wtk_debug("%d\n",in_id);
			//wtk_debug("add trans from %d to %d\n",pre->id,ts->id);
			wtk_e2fst_link_trans(e,trans,NULL,tt);//TODO
			if(in_id>0)
			{
				wtk_e2fst_expand_bi_net_node3(e,ts,trans,(wtk_fst_state2_t*)trans->to_state);
			}else
			{
				wtk_e2fst_expand_bi_net_node3(e,ts,NULL,(wtk_fst_state2_t*)trans->to_state);
			}
		}
	}
}

int wtk_e2fst_expand_bi_net(wtk_e2fst_t *e,wtk_fst_net2_t *mono_net)
{
	wtk_fst_state2_t *state;
	wtk_fst_net2_t *net=e->net;

	state=wtk_e2fst_pop_state(e);
	//state->hook=(void*)(long)-1;
	net->start=state;
	net->end=wtk_e2fst_pop_state(e);
	net->end->type=WTK_FST_FINAL_STATE;
	//wtk_strbuf_t* buf = wtk_strbuf_new(1024,1);
	//wtk_fst_net2_print_fsm(mono_net,buf);
	//printf("%.*s\n",buf->pos,buf->data);
	wtk_e2fst_clean_filler_hook(e);
	if(e->cfg->type == 0)
	{
		wtk_e2fst_expand_bi_net_node2(e,state,NULL,(wtk_fst_state2_t*)mono_net->start);
	}else
	{
		wtk_e2fst_expand_bi_net_node3(e,state,NULL,(wtk_fst_state2_t*)mono_net->start);
	}
	return 0;
}


//static int inmap[3288]={
//		2828,337,1765,1385,67,3098,2387,2387,2387,1,2255,1018,878,1558,655,2902,898,1764,1312,2899,2740,3089,1965,3033,3033,2730,2771,2327,2805,2159,2746,2073,3018,2538,389,2819,3049,1859,2824,1555,1894,2830,2731,2511,1891,2783,2614,1245,1174,2834,1946,2161,294,861,2609,191,2421,2959,2552,2609,3084,1920,2935,354,1030,569,1345,2202,1383,1925,1289,2427,1263,507,280,335,78,101,728,881,1448,459,793,1142,548,924,529,1292,1394,2656,1696,1235,3011,2254,2973,1889,3010,2721,2943,2441,2441,851,3009,3009,2655,3009,2918,3079,959,3072,3072,930,1271,975,2155,833,2898,2838,2838,2838,2838,2644,2644,1214,2391,2391,2565,1908,786,1817,2504,2516,2322,2852,2974,1902,2718,1457,2718,2837,1942,1211,557,2957,3042,1144,1113,2906,2167,1849,1619,792,1876,1486,1223,1089,2632,2352,2976,572,456,2252,2397,2529,2224,3091,962,3029,2981,2784,2282,2758,1782,2272,275,2249,2921,1602,2887,2983,625,1657,2848,264,922,2985,885,1255,1992,2283,1785,1691,1895,187,1808,401,1471,1824,2507,200,1838,1152,1894,1894,2023,2260,2869,2826,2535,1885,2861,2354,1516,3013,3078,2595,963,1781,750,1781,1781,2661,2427,2967,1079,2980,2289,2264,1193,1122,1949,2396,4,2074,1099,2967,1631,2855,2806,778,2339,2652,1884,2919,2806,2427,2427,3073,3073,3073,1192,468,1305,965,2728,2326,2866,2456,2415,1741,1249,1016,2513,638,1475,1663,1683,2964,2707,2356,3020,3020,3101,3101,2182,3101,2693,2643,626,2059,2303,2628,293,1023,2892,2962,864,2460,2460,2460,2460,2971,981,2920,1780,2079,1769,2428,1391,2822,985,2581,1797,2618,3081,2846,2103,2503,767,1158,873,1711,2961,679,242,1323,1204,1033,1773,1630,1077,2704,2602,2579,686,1308,3008,2294,2307,473,1945,2701,2701,2701,2701,2820,1057,309,3063,552,419,2823,2550,303,2808,2606,2600,167,574,1648,131,733,3051,2001,2452,798,2953,2328,2835,221,2583,2321,1928,2496,2200,1815,3032,2658,2419,2608,2125,2853,640,1393,1877,2351,2364,2364,2364,463,1482,3089,2687,2966,1748,2620,2864,1635,2129,2768,2615,2989,1346,2300,2300,2242,2300,2537,2552,2552,2948,3079,2912,1029,553,2568,1343,1253,3069,2221,761,2099,2743,1715,869,65,2014,859,2606,1573,482,471,2941,2686,2897,1153,496,1959,690,2901,521,1270,880,2003,2995,1096,1626,2332,1574,804,862,1720,2769,2800,2573,1543,2140,1229,2051,1591,3087,553,1930,1544,785,2115,2591,212,2115,2115,504,2430,1244,2296,1360,3078,2787,818,3052,2904,1395,1227,1563,1248,2566,2857,1519,1511,2177,1686,393,646,49,2505,2091,2965,2546,806,394,2561,1265,2813,2813,2870,2672,2971,2664,2664,2604,428,1333,130,1319,83,676,1706,2423,332,2526,940,1826,1792,1528,1181,1335,1121,917,1358,912,1300,2464,1759,1403,1524,195,821,97,2357,2442,2610,2690,2700,2505,936,2605,2245,2970,2799,2676,2508,2765,1240,3085,2674,2234,1095,2793,1881,831,1664,410,684,2816,2624,3059,1001,720,248,480,1717,247,2355,262,2840,484,634,478,2239,2194,2501,1126,814,3076,2489,692,1507,2063,780,1129,1941,2534,1258,2138,1347,1416,3036,1336,956,2683,274,2341,388,2591,2236,2591,1703,119,1036,2800,2769,3062,1213,1213,409,530,2440,932,377,1043,698,1238,1239,2095,1015,658,1678,2560,265,2164,826,2493,1028,886,1647,364,603,726,734,2117,1518,961,2027,1611,540,600,3001,2824,2730,2824,2824,1434,3067,2417,2572,2137,386,469,623,506,84,1913,3063,1294,1829,971,58,1173,1778,973,381,441,1081,2881,2556,340,1059,856,849,800,2477,2574,2554,1770,847,2845,1348,2196,565,2524,472,1614,1032,1966,474,1752,2884,2490,2490,2591,2490,2490,2913,2960,2872,1170,1170,1170,1170,1531,241,241,241,241,2248,3097,2207,3006,2667,2633,1733,1432,2312,909,2329,503,2273,12,253,2366,1828,2842,982,2386,1644,1526,1421,605,2993,159,794,1114,2398,2886,374,2518,2545,2545,2924,2443,2399,2569,427,1044,2998,667,1206,966,485,2738,2281,1392,402,2978,706,1493,742,564,610,2459,2172,2519,2154,1860,2122,3039,3070,2914,2860,2406,2450,3037,3015,2285,2349,2381,2565,2846,1919,2325,2940,2882,836,149,1513,1848,1848,1835,2147,2729,1208,2311,2747,2046,1014,2482,2705,2582,2862,1655,2455,2991,1606,891,3093,2679,3068,2498,1161,2776,11,2990,2467,2850,2131,2010,1027,2148,1970,3046,3046,1681,2689,1632,2695,2160,1572,1423,1215,1873,1437,1447,1845,1502,2734,111,2657,1477,911,2384,1800,2654,2409,2825,2612,2008,845,415,2664,1634,1534,2334,2021,454,1480,929,2877,629,453,1814,217,1912,1575,2885,1106,719,635,1242,60,700,1943,2785,2785,2785,2785,1440,2278,92,2102,258,258,1040,2886,1342,136,2501,2229,59,3057,683,278,113,2515,3094,3094,1978,1138,457,1978,1978,2878,1675,1427,809,2684,1330,2559,2806,1281,2323,1350,815,2251,748,2162,1329,1076,846,190,277,1874,2759,2393,2778,1454,1579,2197,2713,1867,693,1104,429,32,2823,2823,2766,2469,2469,2469,2469,489,835,122,1203,2248,2059,2059,2059,1328,118,405,1209,2814,1937,2663,2253,2665,258,1745,1903,2547,258,3014,1760,2774,2269,2812,2956,2346,931,2649,3002,2313,1900,2650,271,163,287,3100,685,1961,915,156,1010,1010,1010,2697,2697,2697,2697,2607,2871,2033,1159,1784,160,57,577,1793,251,1793,1793,1793,1298,2709,1453,2709,1914,1856,346,1205,2286,2286,339,1461,2394,732,650,2955,1094,499,1334,2266,960,1875,1154,186,1241,933,285,2089,1649,1977,984,1775,307,2291,1111,1562,1771,2004,1456,3056,1010,1049,613,328,3082,3074,722,2458,1505,1967,2199,2809,436,2542,461,892,2670,2088,1316,164,197,1539,2134,2671,1595,587,1641,1589,579,762,899,1422,1523,725,718,1677,619,70,2928,1734,1313,458,2130,2735,1561,1232,642,1666,2466,2218,979,2198,2959,2761,134,1866,1863,1844,2361,810,2449,834,1698,1478,2646,974,1498,1495,2487,1381,2810,1124,2209,2875,948,255,2767,2233,2233,668,1624,539,2742,1979,2379,2201,820,747,508,2013,178,1652,848,1055,390,2726,1062,356,2308,2142,1283,1598,1934,1252,1592,1816,1727,622,1387,855,1327,2908,1658,566,1593,2392,957,2020,926,545,1622,2950,2191,674,1125,2453,1837,1355,954,2259,652,2796,2418,2127,2867,1612,681,1469,1086,2237,107,107,2872,1483,2527,1615,1990,1615,1938,2008,1101,758,1704,2118,2639,1736,2737,2302,2698,1200,425,63,166,491,3034,832,1957,2193,1384,358,1996,76,3,2165,2165,1020,154,288,1359,426,172,850,229,1485,1485,1485,712,254,1786,1599,378,404,209,1616,1805,1584,2782,1892,1078,1090,2433,1429,1339,2047,1825,3021,2747,2522,2330,2331,2370,2126,1417,2702,1857,2779,1743,702,1445,2247,1546,2727,648,934,727,902,2984,711,1639,923,2438,2645,2858,2751,938,716,660,546,2287,1133,1542,772,21,2977,2213,1646,1670,476,465,905,651,621,2462,699,1466,1386,2413,1807,210,703,2648,591,1017,22,2637,10,901,790,2946,2426,322,2372,16,1264,2900,2457,2514,1999,2514,671,2748,1789,827,44,369,104,30,589,1119,29,143,33,2895,444,5,691,344,193,8,3055,479,2500,751,2818,893,765,1151,295,1697,1406,3092,272,510,1290,231,1783,829,1123,2692,1107,467,1879,2662,2905,2250,3025,1213,334,2936,1674,578,2936,2133,2794,2847,392,2854,1382,1530,2407,2206,1322,528,2478,2113,1721,1441,1988,1994,3030,3007,1156,1762,2661,594,1974,1603,696,81,994,649,250,3044,2919,2258,2919,2919,3053,3053,2962,724,2395,1730,1220,555,2557,2896,2896,882,865,1402,582,2242,318,1237,2744,413,445,243,435,124,2151,1997,1642,3088,1806,663,853,2216,997,2286,2286,109,1462,1216,908,1662,1034,1991,1091,2104,533,1177,1295,1980,1296,1823,1341,1141,2158,1779,980,624,2168,919,1164,1408,501,1201,173,207,357,1226,773,919,812,2187,2889,1768,2484,2484,615,129,123,115,2,301,2280,1433,1636,1719,219,2525,1147,2525,633,2238,2112,1729,3024,580,1850,1377,1256,1108,414,697,1777,1041,137,1222,257,1512,1246,3077,2942,1326,1852,2651,2939,89,608,1139,449,2521,672,2169,18,2929,2929,1353,1356,2638,3004,2571,2675,2362,2347,2141,2212,2789,2613,1297,268,1004,777,941,2034,2040,2348,1496,2297,789,789,789,789,860,1947,1947,2288,2290,2626,2278,1740,2678,2486,3047,1710,1404,2781,2811,2756,2226,2488,2365,2587,2157,2804,1840,1571,1449,2022,2739,597,2833,2078,1685,866,1362,495,2144,224,1274,1796,746,2139,992,2470,1284,326,1490,1303,509,2220,1982,1927,1065,2483,576,498,2012,1951,1492,391,687,398,1688,2630,1474,1072,895,1587,2474,1307,2268,2666,2069,2868,972,1810,925,1590,2235,3104,2382,2184,1535,796,1656,604,520,446,320,871,396,1545,852,2851,1827,1165,3028,1425,165,1325,2968,424,1510,102,1267,1510,2888,1301,542,1332,2740,2740,2740,1801,989,147,1499,628,989,330,538,2083,2668,1550,1753,2262,1820,475,17,140,158,1971,1532,2230,1409,1450,325,1679,2741,3096,2523,437,1157,3095,168,3026,741,1171,636,2876,2101,2100,1732,14,3003,1143,2448,1293,1039,2741,970,1430,1321,688,1363,1473,1804,977,2802,342,2621,2621,2621,2621,2874,2124,1799,2909,1811,2821,1744,237,1046,2031,743,2617,1981,2405,2755,1179,2109,1716,2996,1661,3058,1163,300,1802,1761,1183,331,1989,2306,1909,2263,2173,2097,2465,1766,2660,3027,174,1402,2208,2404,249,3066,630,858,2577,2477,657,1118,371,744,412,1455,609,2299,230,2447,2447,2447,1795,2827,1128,2178,2815,2038,2075,2075,2075,2075,1160,451,2359,3040,2634,3012,2754,2181,1132,1985,2043,2190,2594,1628,787,1576,1309,1851,1742,1309,1026,304,2910,2333,19,1936,2593,2277,36,2725,246,302,353,1929,1929,2955,2955,2955,2791,214,1794,1708,1361,260,2682,3075,232,717,2736,199,2333,2363,50,1150,1186,583,1601,1958,606,807,1087,1922,807,1491,2528,2434,1191,314,470,1500,1933,752,1986,2849,3050,292,41,986,1758,291,267,2087,675,6,760,541,585,1552,226,2367,682,2094,2265,2017,488,355,1100,110,383,399,2461,1541,1189,2411,2829,1763,153,1068,347,620,622,1948,2933,2057,2951,701,2256,759,2293,185,95,913,2585,1726,1420,1962,2189,2586,3061,2856,2006,1497,2136,2930,1412,1604,1617,1707,2276,1567,1344,1317,1317,2780,2780,2780,2780,1700,37,1700,1272,1369,1369,236,138,52,55,919,88,2979,2564,2149,47,2305,2110,2619,2844,1633,2243,614,1379,2843,2373,2485,953,1625,2463,558,1687,2066,1738,2724,2724,1609,2893,1621,2018,1182,2479,2763,1847,2030,1197,438,1167,1964,234,2214,2067,2754,25,1712,2972,1843,2183,708,3023,2371,2093,1102,1389,2798,1906,2958,2232,1787,593,791,2694,1324,2564,450,1911,1166,876,1578,1372,611,1618,2410,1484,1484,1484,2803,1476,1365,904,1337,784,939,2432,2060,1812,2623,1120,1207,2429,1340,828,2823,2841,3035,2932,756,995,883,969,366,2543,2156,3019,1580,2025,2926,2517,2084,1424,2376,1056,2836,2836,2706,2279,598,422,161,406,169,3086,1525,1525,319,1525,2831,2559,1196,1196,2475,945,549,584,643,1218,311,1855,345,641,1699,313,317,24,407,2123,417,2225,1533,1438,2749,987,2096,799,2553,273,1723,680,710,27,551,466,2720,99,2720,896,2174,1364,612,343,1591,1675,1169,823,3017,505,1529,1400,1522,550,854,2049,988,1701,1515,2911,2627,2945,2270,105,601,94,397,639,1931,2439,2092,2092,2092,2092,1883,991,903,669,2714,2205,2745,2381,2548,1645,2244,2431,1003,2499,2153,1098,824,2416,967,1269,211,2533,1905,2105,1259,2009,1443,753,2077,2342,2821,2314,312,1514,2999,1287,1547,1932,1878,2611,1975,1564,1842,801,202,1131,448,2106,321,1638,1410,310,2497,2414,1146,179,2080,2923,2257,1898,2454,1199,2975,1854,2570,2540,2795,2712,2616,1418,1540,2502,227,1254,1470,1536,2424,2292,1233,1650,1007,1064,2072,900,2071,2567,2531,1739,2468,1803,2883,1637,1488,857,2055,1047,2228,1582,705,968,1276,1882,1987,1755,2685,2219,2210,1749,1998,2673,204,2377,1813,2673,3041,2304,1832,1984,1790,1210,2495,3080,2114,2512,1623,2480,2374,868,664,2402,2647,2180,206,61,297,403,205,654,808,1809,808,2944,2476,2179,2949,570,783,2015,819,993,1568,779,1172,2544,825,2653,2555,352,588,1757,1318,2773,735,2777,362,2680,914,1654,1419,2481,1399,1724,1917,1373,2589,1939,1180,1747,2635,1861,1861,1861,1861,764,432,1929,1693,1085,1872,2915,592,2952,141,3040,338,1261,1452,1559,874,2596,874,874,2076,1527,1234,1897,2284,1112,1112,1375,2274,2223,544,1354,2358,368,194,730,1451,1556,2338,2369,3083,1682,1038,2261,2879,561,2879,1908,64,514,1021,418,176,2320,943,1058,192,373,483,2336,2336,2336,950,1228,1228,1228,2703,998,2108,1024,1554,942,139,3060,10,10,10,240,1788,1051,2085,1025,1504,616,1714,1136,162,416,983,2236,2236,2170,2584,400,2969,537,738,2425,864,586,864,1935,2056,2120,2937,2880,1643,72,20,439,189,90,359,617,125,144,23,844,897,2948,632,239,370,745,245,531,259,259,259,259,2412,2408,1952,1397,1109,2039,677,1093,3031,486,2710,1067,2603,2592,2817,1651,2308,376,524,1185,1608,2298,2588,1008,2669,3079,3079,2062,2217,2195,333,2917,2192,2280,1944,1031,481,79,1371,1022,1901,731,379,380,656,656,1311,1972,1893,2520,1225,1134,1149,1468,1924,1501,2317,3103,2688,1673,1435,837,1405,1168,2760,2068,487,1178,1187,1349,2163,1145,440,1718,462,713,1904,678,1839,184,43,1950,2132,2578,2383,1115,2836,2368,7,1436,843,1695,1459,1864,2954,1073,2316,1627,1722,1280,2065,870,1896,1767,782,2111,2601,805,423,1694,2563,1190,1071,1653,1251,452,1969,1725,877,1195,3045,2032,2717,2717,1411,2310,2907,1772,1735,1310,811,653,39,2045,1012,2786,1955,1517,662,661,223,203,1831,2772,2389,2337,2422,2472,1048,1105,2435,1610,1585,2451,1956,1243,595,949,1465,1853,2116,1995,2947,2723,1257,455,2050,1565,1250,1953,1702,525,1306,928,2240,2987,1910,1731,2058,1439,1035,1074,918,1520,2510,502,951,218,635,2677,2741,631,635,635,1464,522,181,2149,1357,2149,2378,133,1117,695,694,571,77,513,575,442,816,148,2145,1746,1846,2536,2536,2536,2536,1088,2081,2541,2295,2471,3038,2024,2048,1684,2143,2037,2997,1277,1570,2176,2385,2691,127,2215,990,958,375,958,220,68,1583,1551,1075,152,890,1042,523,155,518,306,770,1011,1538,1407,2642,1338,766,2641,2558,1858,1993,31,1600,2792,673,2873,1916,894,2752,2757,1414,2461,1818,1116,1580,3005,2036,1705,887,3022,1751,2121,1230,1728,2963,2446,781,1066,1097,2640,1315,2832,2739,2019,739,336,1963,573,2061,45,2986,1553,1426,2770,2551,2994,1092,286,2211,2708,2380,2344,1689,2625,1597,736,1888,1224,1060,670,282,1266,2309,2659,1487,13,225,822,803,2166,1286,1862,757,2580,363,477,2350,2436,1489,2719,1137,1834,1278,2403,2494,2444,944,2324,2119,581,1390,2598,2135,2530,2622,2318,3090,1282,2775,1398,1415,1671,637,408,2797,647,952,82,1886,1577,2203,1388,1672,1750,2353,1376,817,526,2005,2005,2005,2005,1045,2839,721,2026,222,771,497,879,879,430,348,3043,86,1006,1923,1923,26,1923,2681,2800,431,704,2506,1494,121,241,1291,1148,935,1080,443,740,1285,599,112,1444,2903,2562,494,1002,1302,2086,1442,2090,2007,1586,2227,1926,921,1217,1431,2016,1836,2629,2934,1871,1588,1566,1273,2000,208,1660,228,1063,182,434,1162,1446,323,1069,28,1262,1053,256,666,500,511,867,563,1669,737,907,349,3035,2360,66,910,910,910,1667,659,2054,1596,132,1798,1798,1692,536,1798,776,2267,2128,2300,2938,2938,863,87,2831,1613,372,906,261,1001,1236,1140,270,1629,515,1548,2082,2082,1870,1833,1822,3000,709,2473,627,1479,1506,3048,1467,1921,2029,2733,2231,1175,2807,2890,2222,2925,2801,215,556,296,937,2152,1231,2028,755,872,1380,554,42,1352,1368,1821,1521,999,754,955,2064,180,2064,775,2421,2421,2445,2275,2722,1037,341,2549,2991,1690,360,1607,2042,2042,2042,2042,2340,2052,519,1083,1135,1472,1260,284,2400,1605,315,2098,2420,1915,1907,1314,1314,1054,1212,2600,602,567,763,763,460,460,1009,2598,665,2575,1351,361,34,813,996,2982,1791,1110,2570,2570,464,1370,596,382,106,1709,150,1176,266,1830,543,1776,493,2246,707,532,1887,723,1396,1460,769,1868,1756,2570,46,919,73,145,54,100,74,69,71,175,85,53,48,116,75,108,365,91,1668,2271,1640,1640,1640,1640,1188,2315,1070,1221,2696,3034,1463,3071,1000,1918,1304,841,146,2931,1973,916,1537,2070,830,188,2241,1680,749,2750,1155,1374,517,1331,802,420,235,1127,1899,1899,2509,1865,1659,1247,2762,395,327,1737,714,1184,2390,1202,715,1052,2107,1549,281,308,1130,1557,689,1880,2716,170,768,1968,1103,252,512,1275,421,840,1976,875,1228,2711,2492,2788,290,2491,1401,2927,2335,1665,2597,2988,2916,1940,3016,2863,305,1954,213,1953,1953,80,126,238,1367,1428,774,171,263,839,788,920,590,2859,1194,1481,244,888,1960,842,1713,1869,978,976,2035,964,233,1413,490,547,547,411,51,889,1050,946,1268,559,38,1890,433,1508,1013,2992,1676,1082,2865,838,645,2011,350,289,2186,795,15,385,299,492,1709,1709,387,618,1503,534,276,927,2590,324,351,516,2401,560,2753,2067,1581,884,1841
//};

//void wtk_e2fst_expand_hmm_net_node2_raw(wtk_e2fst_t *e,wtk_fst_state2_t* pre2,
//		wtk_fst_trans2_t *a,wtk_fst_state2_t *nxt_state)
//{
//	wtk_fst_net2_t *net=e->hmm_net;
//	wtk_fst_trans2_t *trans;
//	wtk_fst_state2_t *pre;
//	wtk_fst_state2_t *ts=0,*ts2=0,*ts3=0,*ts4=0,*ts5=0,*ts6=0;
//	wtk_fst_state2_t *ts1=0,*ts12=0,*ts13=0,*ts16=0;
//	wtk_fst_state2_t *ts21=0,*ts22=0,*ts23=0;
//	wtk_fst_state2_t *ts31=0,*ts32=0,*ts33=0;
//	wtk_fst_state2_t *ts41=0;
//	int in_id,out_id=0;
//	int i;
//	float selfloop_scale=e->cfg->selfloop_scale;
//	wtk_e2fst_hmm_expand_t *expand;
//	wtk_e2fst_hmm_expand_pdf_t *pdf;
//	//wtk_debug("%d\n",nxt_state->id);
//	if(nxt_state->type == WTK_FST_FINAL_STATE)
//	{
//		//in_id=inmap[a->in_id-1];
//		in_id=e->cfg->hmm_maps[a->in_id-1]->pdf->forward_id;
//		//wtk_debug("%d %d\n",in_id,a->in_id);
//
//		wtk_fst_net2_link_state3(net,pre2,net->end,0,in_id,a->out_id,0,0.41589);
//
//		//wtk_debug("add trans from %d to %d\n",pre->id,net->end->id);
//		//printf("%d %d %d %d 0.41589\n",pre->id,net->end->id,in_id,a->out_id);
//
//	}
//	for(trans=(wtk_fst_trans2_t*)nxt_state->v.trans;trans;trans=(wtk_fst_trans2_t*)trans->hook.next)
//	{
//		//wtk_debug("process trans: from %d to %d %d %d\n",trans->from_state->id,trans->to_state->id,trans->in_id,trans->out_id);
//		if(trans->in_id!=0)
//		{
//			if(!trans->to_state->hook)
//			{
//				expand=e->cfg->hmm_maps[trans->in_id-1];
//				if(!e->cfg->use_chain && trans->in_id==e->sil_in_id)
//				{
//					pdf=expand->pdf;
//					//0
//					ts=wtk_e2fst_pop_state2(e);
//					ts2=wtk_e2fst_pop_state2(e);
//					ts3=wtk_e2fst_pop_state2(e);
//
//					wtk_fst_net2_link_state3(net,pre2,ts,0,pdf->id[1],trans->out_id,0,pdf->weight[1]*selfloop_scale);//0-1
//					wtk_fst_net2_link_state3(net,pre2,ts2,0,pdf->id[2],trans->out_id,0,pdf->weight[2]*selfloop_scale);//0-2
//					wtk_fst_net2_link_state3(net,pre2,ts3,0,pdf->id[3],trans->out_id,0,pdf->weight[3]*selfloop_scale);//0-3
//					wtk_fst_net2_link_state3(net,ts,ts,0,pdf->id[0],0,0,pdf->weight[0]*selfloop_scale);//0-0
//					wtk_fst_net2_link_state3(net,ts2,ts2,0,pdf->id[0],0,0,pdf->weight[0]*selfloop_scale);//0-0
//					wtk_fst_net2_link_state3(net,ts3,ts3,0,pdf->id[0],0,0,pdf->weight[0]*selfloop_scale);//0-0
//
//					ts4=wtk_e2fst_pop_state2(e);
//					ts5=wtk_e2fst_pop_state2(e);
//					ts6=wtk_e2fst_pop_state2(e);
//					wtk_fst_net2_link_state3(net,ts,ts4,0,0,0,0,0.45731);//eps 1
//					wtk_fst_net2_link_state3(net,ts2,ts5,0,0,0,0,0.45731);//eps 2
//					wtk_fst_net2_link_state3(net,ts3,ts6,0,0,0,0,0.45731);//eps 3
//
//					//1
//					ts1=wtk_e2fst_pop_state2(e);
//					ts12=wtk_e2fst_pop_state2(e);
//					ts13=wtk_e2fst_pop_state2(e);
//					wtk_fst_net2_link_state3(net,ts4,ts1,0,pdf->id[5],0,0,pdf->weight[5]*selfloop_scale);//1-2
//					wtk_fst_net2_link_state3(net,ts4,ts12,0,pdf->id[6],0,0,pdf->weight[6]*selfloop_scale);//1-3
//					wtk_fst_net2_link_state3(net,ts4,ts13,0,pdf->id[7],0,0,pdf->weight[7]*selfloop_scale);//1-4
//					wtk_fst_net2_link_state3(net,ts1,ts1,0,pdf->id[4],0,0,pdf->weight[4]*selfloop_scale);//1-1
//					wtk_fst_net2_link_state3(net,ts12,ts12,0,pdf->id[4],0,0,pdf->weight[4]*selfloop_scale);//1-1
//					wtk_fst_net2_link_state3(net,ts13,ts13,0,pdf->id[4],0,0,pdf->weight[4]*selfloop_scale);//1-1
//
//					ts16=wtk_e2fst_pop_state2(e);
//					wtk_fst_net2_link_state3(net,ts1,ts5,0,0,0,0,0.88541);//eps 2
//					wtk_fst_net2_link_state3(net,ts12,ts6,0,0,0,0,0.88541);//eps 3
//					wtk_fst_net2_link_state3(net,ts13,ts16,0,0,0,0,0.88541);//eps 4
//
//					//2
//					ts21=wtk_e2fst_pop_state2(e);
//					ts22=wtk_e2fst_pop_state2(e);
//					ts23=wtk_e2fst_pop_state2(e);
//					wtk_fst_net2_link_state3(net,ts5,ts21,0,pdf->id[8],0,0,pdf->weight[8]*selfloop_scale);//2-1
//					wtk_fst_net2_link_state3(net,ts5,ts22,0,pdf->id[10],0,0,pdf->weight[10]*selfloop_scale);//2-3
//					wtk_fst_net2_link_state3(net,ts5,ts23,0,pdf->id[11],0,0,pdf->weight[11]*selfloop_scale);//2-4
//					wtk_fst_net2_link_state3(net,ts21,ts21,0,pdf->id[9],0,0,pdf->weight[9]*selfloop_scale);//2-2
//					wtk_fst_net2_link_state3(net,ts22,ts22,0,pdf->id[9],0,0,pdf->weight[9]*selfloop_scale);//2-2
//					wtk_fst_net2_link_state3(net,ts23,ts23,0,pdf->id[9],0,0,pdf->weight[9]*selfloop_scale);//2-2
//
//					wtk_fst_net2_link_state3(net,ts21,ts4,0,0,0,0,0.90232);//eps 1
//					wtk_fst_net2_link_state3(net,ts22,ts6,0,0,0,0,0.90232);//eps 3
//					wtk_fst_net2_link_state3(net,ts23,ts16,0,0,0,0,0.90232);//eps 4
//
//					//3
//					ts31=wtk_e2fst_pop_state2(e);
//					ts32=wtk_e2fst_pop_state2(e);
//					ts33=wtk_e2fst_pop_state2(e);
//					wtk_fst_net2_link_state3(net,ts6,ts31,0,pdf->id[12],0,0,pdf->weight[12]*selfloop_scale);//3-1
//					wtk_fst_net2_link_state3(net,ts6,ts32,0,pdf->id[13],0,0,pdf->weight[13]*selfloop_scale);//3-2
//					wtk_fst_net2_link_state3(net,ts6,ts33,0,pdf->id[15],0,0,pdf->weight[15]*selfloop_scale);//3-4
//					wtk_fst_net2_link_state3(net,ts31,ts31,0,pdf->id[14],0,0,pdf->weight[14]*selfloop_scale);//3-3
//					wtk_fst_net2_link_state3(net,ts32,ts32,0,pdf->id[14],0,0,pdf->weight[14]*selfloop_scale);//3-3
//					wtk_fst_net2_link_state3(net,ts33,ts33,0,pdf->id[14],0,0,pdf->weight[14]*selfloop_scale);//3-3
//
//					wtk_fst_net2_link_state3(net,ts31,ts4,0,0,0,0,0.58425);//eps 1
//					wtk_fst_net2_link_state3(net,ts32,ts5,0,0,0,0,0.58425);//eps 2
//					wtk_fst_net2_link_state3(net,ts33,ts16,0,0,0,0,0.58425);//eps 4
//
//					//4
//					ts41=wtk_e2fst_pop_state2(e);
//					wtk_fst_net2_link_state3(net,ts16,ts41,0,pdf->id[17],0,0,pdf->weight[17]*selfloop_scale);//4-5
//					wtk_fst_net2_link_state3(net,ts41,ts41,0,pdf->id[16],0,0,pdf->weight[16]*selfloop_scale);//5-5
//					pre=ts41;
//				}else
//				{
//					pre=pre2;
//					for(i=0;i<expand->num_pdfs;i++)
//					{
//						pdf=expand->pdf+i;
//						ts=wtk_e2fst_pop_state2(e);
//						in_id=pdf->forward_id;
//						if(i==0)
//							out_id=trans->out_id;
//						else
//							out_id=0;
//						wtk_fst_net2_link_state3(net,pre,ts,0,in_id,out_id,0,pdf->forward_weight*selfloop_scale);
//						//wtk_debug("add trans %d %d %d\n",pre->id,ts->id,out_id);
//						in_id=pdf->selfloop_id;
//						wtk_fst_net2_link_state3(net,ts,ts,0,in_id,0,0,pdf->selfloop_weight*selfloop_scale);
//						pre=ts;
//					}
//				}
//	//			ts=wtk_e2fst_pop_state2(e);
//				//trans->to_state->hook=ts;
//				//in_id=inmap[trans->in_id-1];
//	//			in_id=e->cfg->hmm_maps[trans->in_id-1]->pdf->forward_id;
//				//wtk_debug("%d %d %d\n",trans->in_id,in_id,e->cfg->hmm_maps[trans->in_id-1]->pdf->selfloop_id);
//
//	//			wtk_fst_net2_link_state3(net,pre,ts,0,in_id,trans->out_id,0,0.20794);
//				//tt=wtk_fst_net2_link_state3(net,pre,ts,0,in_id,trans->out_id,0,0.69336);
//
//				//wtk_debug("add trans from %d to %d    %d %d\n",pre->id,ts->id,in_id,trans->out_id);
//				//printf("%d %d %d %d 0.69336\n",pre->id,ts->id,in_id,trans->out_id);
//
//				//wtk_e2fst_link_trans(e,trans,NULL,tt);
//	//			in_id=e->cfg->hmm_maps[trans->in_id-1]->pdf->selfloop_id;
//
//				//tt=wtk_fst_net2_link_state3(net,ts,ts,0,in_id,0,0,0.086305);
//	//			wtk_fst_net2_link_state3(net,ts,ts,0,in_id,0,0,0.20794);
//
//				//wtk_debug("add trans from %d to %d    %d 0\n",ts->id,ts->id,in_id);
//				//printf("%d %d  %d 0 0.086305\n",ts->id,ts->id,in_id);
//
//				//wtk_e2fst_link_trans(e,trans,NULL,tt);
//				ts2=wtk_e2fst_pop_state2(e);
//
//				wtk_fst_net2_link_state3(net,pre,ts2,0,0,0,0,0.69336);
//
//				//wtk_debug("add trans from %d to %d    0 0\n",ts->id,ts2->id);
//				//printf("%d %d 0 0 0.41589\n",ts->id,ts2->id);
//
//				//wtk_e2fst_link_trans(e,trans,NULL,tt);
//				trans->to_state->hook=ts2;
//				wtk_e2fst_expand_hmm_net_node2(e,ts2,trans,(wtk_fst_state2_t*)trans->to_state);
//			}else
//			{
//				//TODO
//				expand=e->cfg->hmm_maps[trans->in_id-1];
//
//				if(!e->cfg->use_chain && trans->in_id==e->sil_in_id)
//				{
//					pdf=expand->pdf;
//					//0
//					ts=wtk_e2fst_pop_state2(e);
//					ts2=wtk_e2fst_pop_state2(e);
//					ts3=wtk_e2fst_pop_state2(e);
//
//					wtk_fst_net2_link_state3(net,pre2,ts,0,pdf->id[1],trans->out_id,0,pdf->weight[1]*selfloop_scale);//0-1
//					wtk_fst_net2_link_state3(net,pre2,ts2,0,pdf->id[2],trans->out_id,0,pdf->weight[2]*selfloop_scale);//0-2
//					wtk_fst_net2_link_state3(net,pre2,ts3,0,pdf->id[3],trans->out_id,0,pdf->weight[3]*selfloop_scale);//0-3
//					wtk_fst_net2_link_state3(net,ts,ts,0,pdf->id[0],0,0,pdf->weight[0]*selfloop_scale);//0-0
//					wtk_fst_net2_link_state3(net,ts2,ts2,0,pdf->id[0],0,0,pdf->weight[0]*selfloop_scale);//0-0
//					wtk_fst_net2_link_state3(net,ts3,ts3,0,pdf->id[0],0,0,pdf->weight[0]*selfloop_scale);//0-0
//
//					ts4=wtk_e2fst_pop_state2(e);
//					ts5=wtk_e2fst_pop_state2(e);
//					ts6=wtk_e2fst_pop_state2(e);
//					wtk_fst_net2_link_state3(net,ts,ts4,0,0,0,0,0.45731);//eps 1
//					wtk_fst_net2_link_state3(net,ts2,ts5,0,0,0,0,0.45731);//eps 2
//					wtk_fst_net2_link_state3(net,ts3,ts6,0,0,0,0,0.45731);//eps 3
//
//					//1
//					ts1=wtk_e2fst_pop_state2(e);
//					ts12=wtk_e2fst_pop_state2(e);
//					ts13=wtk_e2fst_pop_state2(e);
//					wtk_fst_net2_link_state3(net,ts4,ts1,0,pdf->id[5],0,0,pdf->weight[5]*selfloop_scale);//1-2
//					wtk_fst_net2_link_state3(net,ts4,ts12,0,pdf->id[6],0,0,pdf->weight[6]*selfloop_scale);//1-3
//					wtk_fst_net2_link_state3(net,ts4,ts13,0,pdf->id[7],0,0,pdf->weight[7]*selfloop_scale);//1-4
//					wtk_fst_net2_link_state3(net,ts1,ts1,0,pdf->id[4],0,0,pdf->weight[4]*selfloop_scale);//1-1
//					wtk_fst_net2_link_state3(net,ts12,ts12,0,pdf->id[4],0,0,pdf->weight[4]*selfloop_scale);//1-1
//					wtk_fst_net2_link_state3(net,ts13,ts13,0,pdf->id[4],0,0,pdf->weight[4]*selfloop_scale);//1-1
//
//					ts16=wtk_e2fst_pop_state2(e);
//					wtk_fst_net2_link_state3(net,ts1,ts5,0,0,0,0,0.88541);//eps 2
//					wtk_fst_net2_link_state3(net,ts12,ts6,0,0,0,0,0.88541);//eps 3
//					wtk_fst_net2_link_state3(net,ts13,ts16,0,0,0,0,0.88541);//eps 4
//
//					//2
//					ts21=wtk_e2fst_pop_state2(e);
//					ts22=wtk_e2fst_pop_state2(e);
//					ts23=wtk_e2fst_pop_state2(e);
//					wtk_fst_net2_link_state3(net,ts5,ts21,0,pdf->id[8],0,0,pdf->weight[8]*selfloop_scale);//2-1
//					wtk_fst_net2_link_state3(net,ts5,ts22,0,pdf->id[10],0,0,pdf->weight[10]*selfloop_scale);//2-3
//					wtk_fst_net2_link_state3(net,ts5,ts23,0,pdf->id[11],0,0,pdf->weight[11]*selfloop_scale);//2-4
//					wtk_fst_net2_link_state3(net,ts21,ts21,0,pdf->id[9],0,0,pdf->weight[9]*selfloop_scale);//2-2
//					wtk_fst_net2_link_state3(net,ts22,ts22,0,pdf->id[9],0,0,pdf->weight[9]*selfloop_scale);//2-2
//					wtk_fst_net2_link_state3(net,ts23,ts23,0,pdf->id[9],0,0,pdf->weight[9]*selfloop_scale);//2-2
//
//					wtk_fst_net2_link_state3(net,ts21,ts4,0,0,0,0,0.90232);//eps 1
//					wtk_fst_net2_link_state3(net,ts22,ts6,0,0,0,0,0.90232);//eps 3
//					wtk_fst_net2_link_state3(net,ts23,ts16,0,0,0,0,0.90232);//eps 4
//
//					//3
//					ts31=wtk_e2fst_pop_state2(e);
//					ts32=wtk_e2fst_pop_state2(e);
//					ts33=wtk_e2fst_pop_state2(e);
//					wtk_fst_net2_link_state3(net,ts6,ts31,0,pdf->id[12],0,0,pdf->weight[12]*selfloop_scale);//3-1
//					wtk_fst_net2_link_state3(net,ts6,ts32,0,pdf->id[13],0,0,pdf->weight[13]*selfloop_scale);//3-2
//					wtk_fst_net2_link_state3(net,ts6,ts33,0,pdf->id[15],0,0,pdf->weight[15]*selfloop_scale);//3-4
//					wtk_fst_net2_link_state3(net,ts31,ts31,0,pdf->id[14],0,0,pdf->weight[14]*selfloop_scale);//3-3
//					wtk_fst_net2_link_state3(net,ts32,ts32,0,pdf->id[14],0,0,pdf->weight[14]*selfloop_scale);//3-3
//					wtk_fst_net2_link_state3(net,ts33,ts33,0,pdf->id[14],0,0,pdf->weight[14]*selfloop_scale);//3-3
//
//					wtk_fst_net2_link_state3(net,ts31,ts4,0,0,0,0,0.58425);//eps 1
//					wtk_fst_net2_link_state3(net,ts32,ts5,0,0,0,0,0.58425);//eps 2
//					wtk_fst_net2_link_state3(net,ts33,ts16,0,0,0,0,0.58425);//eps 4
//
//					//4
//					ts41=wtk_e2fst_pop_state2(e);
//					wtk_fst_net2_link_state3(net,ts16,ts41,0,pdf->id[17],0,0,pdf->weight[17]*selfloop_scale);//4-5
//					wtk_fst_net2_link_state3(net,ts41,ts41,0,pdf->id[16],0,0,pdf->weight[16]*selfloop_scale);//5-5
//					pre=ts41;
//				}else
//				{
//					pre=pre2;
//					for(i=0;i<expand->num_pdfs;i++)
//					{
//						pdf=expand->pdf+i;
//						ts=wtk_e2fst_pop_state2(e);
//						in_id=pdf->forward_id;
//						if(i==0)
//							out_id=trans->out_id;
//						else
//							out_id=0;
//						wtk_fst_net2_link_state3(net,pre,ts,0,in_id,out_id,0,pdf->forward_weight*selfloop_scale);
//						//wtk_debug("add trans %d %d %d\n",pre->id,ts->id,out_id);
//
//						in_id=pdf->selfloop_id;
//						wtk_fst_net2_link_state3(net,ts,ts,0,in_id,0,0,pdf->selfloop_weight*selfloop_scale);
//						pre=ts;
//					}
//				}
//
//				wtk_fst_net2_link_state3(net,pre,trans->to_state->hook,0,0,0,0,0.69336);
////				ts=wtk_e2fst_pop_state2(e);
//				//trans->to_state->hook=ts;
//				//in_id=inmap[trans->in_id-1];
////				in_id=e->cfg->hmm_maps[trans->in_id-1]->pdf->forward_id;
//				//wtk_debug("%d %d\n",in_id,trans->in_id);
//				//wtk_debug("%d %d %d\n",trans->in_id,in_id,e->cfg->hmm_maps[trans->in_id-1]->pdf->selfloop_id);
////				wtk_fst_net2_link_state3(net,pre,ts,0,in_id,trans->out_id,0,0.20794);
//				//tt=wtk_fst_net2_link_state3(net,pre,ts,0,in_id,trans->out_id,0,0.69336);
//
//				//wtk_debug("add trans from %d to %d    %d %d\n",pre->id,ts->id,in_id,trans->out_id);
//				//printf("%d %d %d %d 0.69336\n",pre->id,ts->id,in_id,trans->out_id);
//
//				//wtk_e2fst_link_trans(e,trans,NULL,tt);
////				in_id=e->cfg->hmm_maps[trans->in_id-1]->pdf->selfloop_id;
////				wtk_fst_net2_link_state3(net,ts,ts,0,in_id,0,0,0.20794);
//				//tt=wtk_fst_net2_link_state3(net,ts,ts,0,in_id,0,0,0.086305);
//
//				//wtk_debug("add trans from %d to %d    %d 0\n",ts->id,ts->id,in_id);
//				//printf("%d %d %d 0 0.086305\n",ts->id,ts->id,in_id);
//
//				//wtk_e2fst_link_trans(e,trans,NULL,tt);
//
////				wtk_fst_net2_link_state3(net,pre,trans->to_state->hook,0,0,0,0,0.69336);
//
//				//wtk_debug("add trans from %d to %d    0 0\n",ts->id,tt->to_state->id);
//				//printf("%d %d 0 0 0.41589\n",ts->id,tt->to_state->id);
//
//				//wtk_e2fst_link_trans(e,trans,NULL,tt);
//			}
//		}else
//		{
//			ts2=wtk_e2fst_pop_state2(e);
//
//			wtk_fst_net2_link_state3(net,pre2,ts2,0,0,0,0,0.41589);
//			wtk_e2fst_expand_hmm_net_node2(e,ts2,trans,(wtk_fst_state2_t*)trans->to_state);
//		}
//	}
//}

void wtk_e2fst_expand_hmm_net_node2(wtk_e2fst_t *e,wtk_fst_state2_t* pre2,
		wtk_fst_trans2_t *a,wtk_fst_state2_t *nxt_state)
{
	wtk_fst_net2_t *net=e->hmm_net;
	wtk_fst_trans2_t *trans;
	wtk_fst_state2_t *pre=0;
	wtk_fst_state2_t *ts=0,*ts2=0,*ts3=0,*ts4=0,*ts5=0,*ts6=0;
	wtk_fst_state2_t *ts1=0,*ts12=0,*ts13=0,*ts16=0;
	wtk_fst_state2_t *ts21=0,*ts22=0,*ts23=0;
	wtk_fst_state2_t *ts31=0,*ts32=0,*ts33=0;
	wtk_fst_state2_t *ts41=0;
	int in_id,out_id=0;
	int i;
	float selfloop_scale=e->cfg->selfloop_scale;
	float transition_scale=e->cfg->transition_scale;
	wtk_e2fst_hmm_expand_t *expand;
	wtk_e2fst_hmm_expand_pdf_t *pdf;
	int plus=1;  //current correct: 1.
	//wtk_debug("%d\n",nxt_state->id);
	if(nxt_state->type == WTK_FST_FINAL_STATE)
	{
		//in_id=inmap[a->in_id-1];
		if(a->in_id==0 ||a->in_id == e->sil_in_id || (e->cfg->use_posi && a->in_id == e->sil_S_in_id))
			in_id=0;
		else
			in_id=e->cfg->hmm_maps[a->in_id-1]->pdf->forward_id;
		//wtk_debug("%d %d\n",in_id,a->in_id);

		//wtk_fst_net2_link_state3(net,pre2,net->end,0,in_id,a->out_id,0,plus*log(1.0));
		wtk_fst_net2_link_state3(net,pre2,net->end,0,in_id,0,0,plus*SLOG(1.0));

		//wtk_debug("add trans from %d to %d\n",pre->id,net->end->id);
		//printf("%d %d %d %d 0.41589\n",pre2->id,net->end->id,in_id,a->out_id);

	}
	for(trans=(wtk_fst_trans2_t*)nxt_state->v.trans;trans;trans=(wtk_fst_trans2_t*)trans->hook.next)
	{
		//wtk_debug("process trans: from %d to %d %d %d\n",trans->from_state->id,trans->to_state->id,trans->in_id,trans->out_id);
		if(trans->in_id!=0)
		{
			if(!trans->to_state->hook)
			{
				expand=e->cfg->hmm_maps[trans->in_id-1];
				//if(!e->cfg->use_chain && trans->in_id==e->sil_in_id)
				if(!e->cfg->use_chain && (trans->in_id==e->sil_in_id || (e->cfg->use_posi && trans->in_id == e->sil_S_in_id)))
				{
					pdf=expand->pdf;
					//0
					ts=wtk_e2fst_pop_state2(e);
					ts2=wtk_e2fst_pop_state2(e);
					ts3=wtk_e2fst_pop_state2(e);

					wtk_fst_net2_link_state3(net,pre2,ts,0,pdf->id[1],trans->out_id,trans->lm_like,plus*(trans->lm_like+SLOG(1.0-pdf->weight[0])*selfloop_scale+transition_scale*(SLOG(pdf->weight[1])-SLOG(1-pdf->weight[0]))));//0-1
					wtk_fst_net2_link_state3(net,pre2,ts2,0,pdf->id[2],trans->out_id,trans->lm_like,plus*(trans->lm_like+SLOG(1.0-pdf->weight[0])*selfloop_scale+transition_scale*(SLOG(pdf->weight[2])-SLOG(1-pdf->weight[0]))));//0-2
					wtk_fst_net2_link_state3(net,pre2,ts3,0,pdf->id[3],trans->out_id,trans->lm_like,plus*(trans->lm_like+SLOG(1.0-pdf->weight[0])*selfloop_scale+transition_scale*(SLOG(pdf->weight[3])-SLOG(1-pdf->weight[0]))));//0-3
					wtk_fst_net2_link_state3(net,ts,ts,0,pdf->id[0],0,0,plus*(SLOG(pdf->weight[0])*selfloop_scale));//0-0
					wtk_fst_net2_link_state3(net,ts2,ts2,0,pdf->id[0],0,0,plus*(SLOG(pdf->weight[0])*selfloop_scale));//0-0
					wtk_fst_net2_link_state3(net,ts3,ts3,0,pdf->id[0],0,0,plus*(SLOG(pdf->weight[0])*selfloop_scale));//0-0

					ts4=wtk_e2fst_pop_state2(e);
					ts5=wtk_e2fst_pop_state2(e);
					ts6=wtk_e2fst_pop_state2(e);
					wtk_fst_net2_link_state3(net,ts,ts4,0,0,0,0,plus*SLOG(1.0));//eps 1
					wtk_fst_net2_link_state3(net,ts2,ts5,0,0,0,0,plus*SLOG(1.0));//eps 2
					wtk_fst_net2_link_state3(net,ts3,ts6,0,0,0,0,plus*SLOG(1.0));//eps 3

					//1
					ts1=wtk_e2fst_pop_state2(e);
					ts12=wtk_e2fst_pop_state2(e);
					ts13=wtk_e2fst_pop_state2(e);
					wtk_fst_net2_link_state3(net,ts4,ts1,0,pdf->id[5],0,0,plus*(SLOG(1.0-pdf->weight[4])*selfloop_scale+transition_scale*(SLOG(pdf->weight[5])-SLOG(1-pdf->weight[4]))));//1-2
					wtk_fst_net2_link_state3(net,ts4,ts12,0,pdf->id[6],0,0,plus*(SLOG(1.0-pdf->weight[4])*selfloop_scale+transition_scale*(SLOG(pdf->weight[6])-SLOG(1-pdf->weight[4]))));//1-3
					wtk_fst_net2_link_state3(net,ts4,ts13,0,pdf->id[7],0,0,plus*(SLOG(1.0-pdf->weight[4])*selfloop_scale+transition_scale*(SLOG(pdf->weight[7])-SLOG(1-pdf->weight[4]))));//1-4
					wtk_fst_net2_link_state3(net,ts1,ts1,0,pdf->id[4],0,0,plus*(SLOG(pdf->weight[4])*selfloop_scale));//1-1
					wtk_fst_net2_link_state3(net,ts12,ts12,0,pdf->id[4],0,0,plus*(SLOG(pdf->weight[4])*selfloop_scale));//1-1
					wtk_fst_net2_link_state3(net,ts13,ts13,0,pdf->id[4],0,0,plus*(SLOG(pdf->weight[4])*selfloop_scale));//1-1

					ts16=wtk_e2fst_pop_state2(e);
					wtk_fst_net2_link_state3(net,ts1,ts5,0,0,0,0,plus*SLOG(1.0));//eps 2
					wtk_fst_net2_link_state3(net,ts12,ts6,0,0,0,0,plus*SLOG(1.0));//eps 3
					wtk_fst_net2_link_state3(net,ts13,ts16,0,0,0,0,plus*SLOG(1.0));//eps 4

					//2
					ts21=wtk_e2fst_pop_state2(e);
					ts22=wtk_e2fst_pop_state2(e);
					ts23=wtk_e2fst_pop_state2(e);
					wtk_fst_net2_link_state3(net,ts5,ts21,0,pdf->id[8],0,0,plus*(SLOG(1.0-pdf->weight[9])*selfloop_scale+transition_scale*(SLOG(pdf->weight[8])-SLOG(1-pdf->weight[9]))));//2-1
					wtk_fst_net2_link_state3(net,ts5,ts22,0,pdf->id[10],0,0,plus*(SLOG(1.0-pdf->weight[9])*selfloop_scale+transition_scale*(SLOG(pdf->weight[10])-SLOG(1-pdf->weight[9]))));//2-3
					wtk_fst_net2_link_state3(net,ts5,ts23,0,pdf->id[11],0,0,plus*(SLOG(1.0-pdf->weight[9])*selfloop_scale+transition_scale*(SLOG(pdf->weight[11])-SLOG(1-pdf->weight[9]))));//2-4
					wtk_fst_net2_link_state3(net,ts21,ts21,0,pdf->id[9],0,0,plus*(SLOG(pdf->weight[9])*selfloop_scale));//2-2
					wtk_fst_net2_link_state3(net,ts22,ts22,0,pdf->id[9],0,0,plus*(SLOG(pdf->weight[9])*selfloop_scale));//2-2
					wtk_fst_net2_link_state3(net,ts23,ts23,0,pdf->id[9],0,0,plus*(SLOG(pdf->weight[9])*selfloop_scale));//2-2

					wtk_fst_net2_link_state3(net,ts21,ts4,0,0,0,0,plus*SLOG(1.0));//eps 1
					wtk_fst_net2_link_state3(net,ts22,ts6,0,0,0,0,plus*SLOG(1.0));//eps 3
					wtk_fst_net2_link_state3(net,ts23,ts16,0,0,0,0,plus*SLOG(1.0));//eps 4

					//3
					ts31=wtk_e2fst_pop_state2(e);
					ts32=wtk_e2fst_pop_state2(e);
					ts33=wtk_e2fst_pop_state2(e);
					wtk_fst_net2_link_state3(net,ts6,ts31,0,pdf->id[12],0,0,plus*(SLOG(1.0-pdf->weight[14])*selfloop_scale+transition_scale*(SLOG(pdf->weight[12])-SLOG(1-pdf->weight[14]))));//3-1
					wtk_fst_net2_link_state3(net,ts6,ts32,0,pdf->id[13],0,0,plus*(SLOG(1.0-pdf->weight[14])*selfloop_scale+transition_scale*(SLOG(pdf->weight[13])-SLOG(1-pdf->weight[14]))));//3-2
					wtk_fst_net2_link_state3(net,ts6,ts33,0,pdf->id[15],0,0,plus*(SLOG(1.0-pdf->weight[14])*selfloop_scale+transition_scale*(SLOG(pdf->weight[15])-SLOG(1-pdf->weight[14]))));//3-4
					wtk_fst_net2_link_state3(net,ts31,ts31,0,pdf->id[14],0,0,plus*(SLOG(pdf->weight[14])*selfloop_scale));//3-3
					wtk_fst_net2_link_state3(net,ts32,ts32,0,pdf->id[14],0,0,plus*(SLOG(pdf->weight[14])*selfloop_scale));//3-3
					wtk_fst_net2_link_state3(net,ts33,ts33,0,pdf->id[14],0,0,plus*(SLOG(pdf->weight[14])*selfloop_scale));//3-3

					wtk_fst_net2_link_state3(net,ts31,ts4,0,0,0,0,plus*SLOG(1.0));//eps 1
					wtk_fst_net2_link_state3(net,ts32,ts5,0,0,0,0,plus*SLOG(1.0));//eps 2
					wtk_fst_net2_link_state3(net,ts33,ts16,0,0,0,0,plus*SLOG(1.0));//eps 4

					//4
					ts41=wtk_e2fst_pop_state2(e);
					wtk_fst_net2_link_state3(net,ts16,ts41,0,pdf->id[17],0,0,plus*(SLOG(1.0-pdf->weight[16])*selfloop_scale+transition_scale*(SLOG(pdf->weight[17])-SLOG(1-pdf->weight[16]))));//4-5
					wtk_fst_net2_link_state3(net,ts41,ts41,0,pdf->id[16],0,0,plus*(SLOG(pdf->weight[16])*selfloop_scale));//5-5
					pre=ts41;

				}else
				{
					pre=pre2;
					for(i=0;i<expand->num_pdfs;i++)
					{
						pdf=expand->pdf+i;
						ts=wtk_e2fst_pop_state2(e);
						in_id=pdf->forward_id;
						if(i==0)
							out_id=trans->out_id;
						else
							out_id=0;
						//wtk_fst_net2_link_state3(net,pre,ts,0,in_id,out_id,0,plus*(log(pdf->forward_weight)*selfloop_scale+transition_scale*(log(pdf->forward_weight)-log(1.0-pdf->selfloop_weight))));
						if (i==0)
							wtk_fst_net2_link_state3(net,pre,ts,0,in_id,out_id,trans->lm_like,plus*(trans->lm_like+SLOG(pdf->forward_weight)*selfloop_scale+transition_scale*(SLOG(pdf->forward_weight)-SLOG(1.0-pdf->selfloop_weight))));
						else
							wtk_fst_net2_link_state3(net,pre,ts,0,in_id,out_id,0,plus*(SLOG(pdf->forward_weight)*selfloop_scale+transition_scale*(SLOG(pdf->forward_weight)-SLOG(1.0-pdf->selfloop_weight))));
						//wtk_debug("add trans %d %d %d\n",pre->id,ts->id,out_id);
						in_id=pdf->selfloop_id;
						wtk_fst_net2_link_state3(net,ts,ts,0,in_id,0,0,plus*SLOG(pdf->selfloop_weight)*selfloop_scale);
						pre=ts;
					}
				}
	//			ts=wtk_e2fst_pop_state2(e);
				//trans->to_state->hook=ts;
				//in_id=inmap[trans->in_id-1];
	//			in_id=e->cfg->hmm_maps[trans->in_id-1]->pdf->forward_id;
				//wtk_debug("%d %d %d\n",trans->in_id,in_id,e->cfg->hmm_maps[trans->in_id-1]->pdf->selfloop_id);

	//			wtk_fst_net2_link_state3(net,pre,ts,0,in_id,trans->out_id,0,0.20794);
				//tt=wtk_fst_net2_link_state3(net,pre,ts,0,in_id,trans->out_id,0,0.69336);

				//wtk_debug("add trans from %d to %d    %d %d\n",pre->id,ts->id,in_id,trans->out_id);
				//printf("%d %d %d %d 0.69336\n",pre->id,ts->id,in_id,trans->out_id);

				//wtk_e2fst_link_trans(e,trans,NULL,tt);
	//			in_id=e->cfg->hmm_maps[trans->in_id-1]->pdf->selfloop_id;

				//tt=wtk_fst_net2_link_state3(net,ts,ts,0,in_id,0,0,0.086305);
	//			wtk_fst_net2_link_state3(net,ts,ts,0,in_id,0,0,0.20794);

				//wtk_debug("add trans from %d to %d    %d 0\n",ts->id,ts->id,in_id);
				//printf("%d %d  %d 0 0.086305\n",ts->id,ts->id,in_id);

				//wtk_e2fst_link_trans(e,trans,NULL,tt);
				ts2=wtk_e2fst_pop_state2(e);

//				if (trans->in_id==e->sil_in_id && 0==e->cfg->use_pre_wrd)
//					wtk_fst_net2_link_state3(net,pre,ts2,0,trans->out_id,0,0,plus*log(1.0));
//				else
					wtk_fst_net2_link_state3(net,pre,ts2,0,0,0,0,plus*SLOG(1.0));

				//jump and selfloop??
				//wtk_fst_net2_link_state3(net,pre2,ts2,0,0,0,0,plus*log(1.0));
				//wtk_fst_net2_link_state3(net,ts2,pre2,0,0,0,0,plus*log(1.0));

				//wtk_debug("add trans from %d to %d    0 0\n",ts->id,ts2->id);
				//printf("%d %d 0 0 0.41589\n",ts->id,ts2->id);

				//wtk_e2fst_link_trans(e,trans,NULL,tt);
				trans->to_state->hook=ts2;
				wtk_e2fst_expand_hmm_net_node2(e,ts2,trans,(wtk_fst_state2_t*)trans->to_state);
			}else
			{
				//TODO
				expand=e->cfg->hmm_maps[trans->in_id-1];

				if(!e->cfg->use_chain && (trans->in_id==e->sil_in_id || trans->in_id == e->sil_S_in_id))
				{
					pdf=expand->pdf;
					//0
					ts=wtk_e2fst_pop_state2(e);
					ts2=wtk_e2fst_pop_state2(e);
					ts3=wtk_e2fst_pop_state2(e);

					wtk_fst_net2_link_state3(net,pre2,ts,0,pdf->id[1],trans->out_id,trans->lm_like,plus*(trans->lm_like+SLOG(1.0-pdf->weight[0])*selfloop_scale+transition_scale*(SLOG(pdf->weight[1])-SLOG(1-pdf->weight[0]))));//0-1
					wtk_fst_net2_link_state3(net,pre2,ts2,0,pdf->id[2],trans->out_id,trans->lm_like,plus*(trans->lm_like+SLOG(1.0-pdf->weight[0])*selfloop_scale+transition_scale*(SLOG(pdf->weight[2])-SLOG(1-pdf->weight[0]))));//0-2
					wtk_fst_net2_link_state3(net,pre2,ts3,0,pdf->id[3],trans->out_id,trans->lm_like,plus*(trans->lm_like+SLOG(1.0-pdf->weight[0])*selfloop_scale+transition_scale*(SLOG(pdf->weight[3])-SLOG(1-pdf->weight[0]))));//0-3
					wtk_fst_net2_link_state3(net,ts,ts,0,pdf->id[0],0,0,plus*(SLOG(pdf->weight[0])*selfloop_scale));//0-0
					wtk_fst_net2_link_state3(net,ts2,ts2,0,pdf->id[0],0,0,plus*(SLOG(pdf->weight[0])*selfloop_scale));//0-0
					wtk_fst_net2_link_state3(net,ts3,ts3,0,pdf->id[0],0,0,plus*(SLOG(pdf->weight[0])*selfloop_scale));//0-0

					ts4=wtk_e2fst_pop_state2(e);
					ts5=wtk_e2fst_pop_state2(e);
					ts6=wtk_e2fst_pop_state2(e);
					wtk_fst_net2_link_state3(net,ts,ts4,0,0,0,0,plus*SLOG(1.0));//eps 1
					wtk_fst_net2_link_state3(net,ts2,ts5,0,0,0,0,plus*SLOG(1.0));//eps 2
					wtk_fst_net2_link_state3(net,ts3,ts6,0,0,0,0,plus*SLOG(1.0));//eps 3

					//1
					ts1=wtk_e2fst_pop_state2(e);
					ts12=wtk_e2fst_pop_state2(e);
					ts13=wtk_e2fst_pop_state2(e);
					wtk_fst_net2_link_state3(net,ts4,ts1,0,pdf->id[5],0,0,plus*(SLOG(1.0-pdf->weight[4])*selfloop_scale+transition_scale*(SLOG(pdf->weight[5])-SLOG(1-pdf->weight[4]))));//1-2
					wtk_fst_net2_link_state3(net,ts4,ts12,0,pdf->id[6],0,0,plus*(SLOG(1.0-pdf->weight[4])*selfloop_scale+transition_scale*(SLOG(pdf->weight[6])-SLOG(1-pdf->weight[4]))));//1-3
					wtk_fst_net2_link_state3(net,ts4,ts13,0,pdf->id[7],0,0,plus*(SLOG(1.0-pdf->weight[4])*selfloop_scale+transition_scale*(SLOG(pdf->weight[7])-SLOG(1-pdf->weight[4]))));//1-4
					wtk_fst_net2_link_state3(net,ts1,ts1,0,pdf->id[4],0,0,plus*(SLOG(pdf->weight[4])*selfloop_scale));//1-1
					wtk_fst_net2_link_state3(net,ts12,ts12,0,pdf->id[4],0,0,plus*(SLOG(pdf->weight[4])*selfloop_scale));//1-1
					wtk_fst_net2_link_state3(net,ts13,ts13,0,pdf->id[4],0,0,plus*(SLOG(pdf->weight[4])*selfloop_scale));//1-1

					ts16=wtk_e2fst_pop_state2(e);
					wtk_fst_net2_link_state3(net,ts1,ts5,0,0,0,0,plus*SLOG(1.0));//eps 2
					wtk_fst_net2_link_state3(net,ts12,ts6,0,0,0,0,plus*SLOG(1.0));//eps 3
					wtk_fst_net2_link_state3(net,ts13,ts16,0,0,0,0,plus*SLOG(1.0));//eps 4

					//2
					ts21=wtk_e2fst_pop_state2(e);
					ts22=wtk_e2fst_pop_state2(e);
					ts23=wtk_e2fst_pop_state2(e);
					wtk_fst_net2_link_state3(net,ts5,ts21,0,pdf->id[8],0,0,plus*(SLOG(1.0-pdf->weight[9])*selfloop_scale+transition_scale*(SLOG(pdf->weight[8])-SLOG(1-pdf->weight[9]))));//2-1
					wtk_fst_net2_link_state3(net,ts5,ts22,0,pdf->id[10],0,0,plus*(SLOG(1.0-pdf->weight[9])*selfloop_scale+transition_scale*(SLOG(pdf->weight[10])-SLOG(1-pdf->weight[9]))));//2-3
					wtk_fst_net2_link_state3(net,ts5,ts23,0,pdf->id[11],0,0,plus*(SLOG(1.0-pdf->weight[9])*selfloop_scale+transition_scale*(SLOG(pdf->weight[11])-SLOG(1-pdf->weight[9]))));//2-4
					wtk_fst_net2_link_state3(net,ts21,ts21,0,pdf->id[9],0,0,plus*(SLOG(pdf->weight[9])*selfloop_scale));//2-2
					wtk_fst_net2_link_state3(net,ts22,ts22,0,pdf->id[9],0,0,plus*(SLOG(pdf->weight[9])*selfloop_scale));//2-2
					wtk_fst_net2_link_state3(net,ts23,ts23,0,pdf->id[9],0,0,plus*(SLOG(pdf->weight[9])*selfloop_scale));//2-2

					wtk_fst_net2_link_state3(net,ts21,ts4,0,0,0,0,plus*SLOG(1.0));//eps 1
					wtk_fst_net2_link_state3(net,ts22,ts6,0,0,0,0,plus*SLOG(1.0));//eps 3
					wtk_fst_net2_link_state3(net,ts23,ts16,0,0,0,0,plus*SLOG(1.0));//eps 4

					//3
					ts31=wtk_e2fst_pop_state2(e);
					ts32=wtk_e2fst_pop_state2(e);
					ts33=wtk_e2fst_pop_state2(e);
					wtk_fst_net2_link_state3(net,ts6,ts31,0,pdf->id[12],0,0,plus*(SLOG(1.0-pdf->weight[14])*selfloop_scale+transition_scale*(SLOG(pdf->weight[12])-SLOG(1-pdf->weight[14]))));//3-1
					wtk_fst_net2_link_state3(net,ts6,ts32,0,pdf->id[13],0,0,plus*(SLOG(1.0-pdf->weight[14])*selfloop_scale+transition_scale*(SLOG(pdf->weight[13])-SLOG(1-pdf->weight[14]))));//3-2
					wtk_fst_net2_link_state3(net,ts6,ts33,0,pdf->id[15],0,0,plus*(SLOG(1.0-pdf->weight[14])*selfloop_scale+transition_scale*(SLOG(pdf->weight[15])-SLOG(1-pdf->weight[14]))));//3-4
					wtk_fst_net2_link_state3(net,ts31,ts31,0,pdf->id[14],0,0,plus*(SLOG(pdf->weight[14])*selfloop_scale));//3-3
					wtk_fst_net2_link_state3(net,ts32,ts32,0,pdf->id[14],0,0,plus*(SLOG(pdf->weight[14])*selfloop_scale));//3-3
					wtk_fst_net2_link_state3(net,ts33,ts33,0,pdf->id[14],0,0,plus*(SLOG(pdf->weight[14])*selfloop_scale));//3-3

					wtk_fst_net2_link_state3(net,ts31,ts4,0,0,0,0,plus*SLOG(1.0));//eps 1
					wtk_fst_net2_link_state3(net,ts32,ts5,0,0,0,0,plus*SLOG(1.0));//eps 2
					wtk_fst_net2_link_state3(net,ts33,ts16,0,0,0,0,plus*SLOG(1.0));//eps 4

					//4
					ts41=wtk_e2fst_pop_state2(e);
					wtk_fst_net2_link_state3(net,ts16,ts41,0,pdf->id[17],0,0,plus*(SLOG(1.0-pdf->weight[16])*selfloop_scale+transition_scale*(SLOG(pdf->weight[17])-SLOG(1-pdf->weight[16]))));//4-5
					wtk_fst_net2_link_state3(net,ts41,ts41,0,pdf->id[16],0,0,plus*(SLOG(pdf->weight[16])*selfloop_scale));//5-5
					pre=ts41;
				}else
				{
					pre=pre2;
					for(i=0;i<expand->num_pdfs;i++)
					{
						pdf=expand->pdf+i;
						ts=wtk_e2fst_pop_state2(e);
						in_id=pdf->forward_id;
						if(i==0)
							out_id=trans->out_id;
						else
							out_id=0;
						//wtk_debug("add trans %d %d %d\n",pre->id,ts->id,out_id);
						if (i==0)
							wtk_fst_net2_link_state3(net,pre,ts,0,in_id,out_id,trans->lm_like,plus*(trans->lm_like+SLOG(pdf->forward_weight)*selfloop_scale+transition_scale*(SLOG(pdf->forward_weight)-SLOG(1.0-pdf->selfloop_weight))));
						else
							wtk_fst_net2_link_state3(net,pre,ts,0,in_id,out_id,0,plus*(SLOG(pdf->forward_weight)*selfloop_scale+transition_scale*(SLOG(pdf->forward_weight)-SLOG(1.0-pdf->selfloop_weight))));

						in_id=pdf->selfloop_id;
						wtk_fst_net2_link_state3(net,ts,ts,0,in_id,0,0,plus*SLOG(pdf->selfloop_weight)*selfloop_scale);
						pre=ts;
					}
				}

				wtk_fst_net2_link_state3(net,pre,trans->to_state->hook,0,0,0,0,plus*SLOG(1.0));
//				ts=wtk_e2fst_pop_state2(e);
				//trans->to_state->hook=ts;
				//in_id=inmap[trans->in_id-1];
//				in_id=e->cfg->hmm_maps[trans->in_id-1]->pdf->forward_id;
				//wtk_debug("%d %d\n",in_id,trans->in_id);
				//wtk_debug("%d %d %d\n",trans->in_id,in_id,e->cfg->hmm_maps[trans->in_id-1]->pdf->selfloop_id);
//				wtk_fst_net2_link_state3(net,pre,ts,0,in_id,trans->out_id,0,0.20794);
				//tt=wtk_fst_net2_link_state3(net,pre,ts,0,in_id,trans->out_id,0,0.69336);

				//wtk_debug("add trans from %d to %d    %d %d\n",pre->id,ts->id,in_id,trans->out_id);
				//printf("%d %d %d %d 0.69336\n",pre->id,ts->id,in_id,trans->out_id);

				//wtk_e2fst_link_trans(e,trans,NULL,tt);
//				in_id=e->cfg->hmm_maps[trans->in_id-1]->pdf->selfloop_id;
//				wtk_fst_net2_link_state3(net,ts,ts,0,in_id,0,0,0.20794);
				//tt=wtk_fst_net2_link_state3(net,ts,ts,0,in_id,0,0,0.086305);

				//wtk_debug("add trans from %d to %d    %d 0\n",ts->id,ts->id,in_id);
				//printf("%d %d %d 0 0.086305\n",ts->id,ts->id,in_id);

				//wtk_e2fst_link_trans(e,trans,NULL,tt);

//				wtk_fst_net2_link_state3(net,pre,trans->to_state->hook,0,0,0,0,0.69336);

				//wtk_debug("add trans from %d to %d    0 0\n",ts->id,tt->to_state->id);
				//printf("%d %d 0 0 0.41589\n",ts->id,tt->to_state->id);

				//wtk_e2fst_link_trans(e,trans,NULL,tt);
			}
		}else
		{
			ts2=wtk_e2fst_pop_state2(e);

			wtk_fst_net2_link_state3(net,pre2,ts2,0,0,0,0,plus*SLOG(1.0));
			wtk_e2fst_expand_hmm_net_node2(e,ts2,trans,(wtk_fst_state2_t*)trans->to_state);
		}
	}
}


void wtk_e2fst_expand_hmm_net_node(wtk_e2fst_t *e,wtk_fst_state2_t* pre,
		wtk_fst_trans2_t *a,wtk_fst_state2_t *nxt_state)
{
	wtk_fst_net2_t *net=e->hmm_net;
	wtk_fst_trans2_t *trans;
	wtk_fst_state2_t *ts=0,*ts2=0;
	int in_id;

	//wtk_debug("%d\n",nxt_state->id);
	if(nxt_state->type == WTK_FST_FINAL_STATE)
	{
		//in_id=inmap[a->in_id-1];
		in_id=e->cfg->hmm_maps[a->in_id-1]->pdf->forward_id;
		//wtk_debug("%d %d\n",in_id,a->in_id);

		wtk_fst_net2_link_state3(net,pre,net->end,0,in_id,a->out_id,0,0.41589);

		//wtk_debug("add trans from %d to %d\n",pre->id,net->end->id);
		//printf("%d %d %d %d 0.41589\n",pre->id,net->end->id,in_id,a->out_id);

	}
	for(trans=(wtk_fst_trans2_t*)nxt_state->v.trans;trans;trans=(wtk_fst_trans2_t*)trans->hook.next)
	{
		//wtk_debug("process trans: from %d to %d %d %d\n",trans->from_state->id,trans->to_state->id,trans->in_id,trans->out_id);
		if(trans->to_state->v.trans == NULL)
		{
			if(trans->to_state->type != WTK_FST_FINAL_STATE)
			{
				continue;
			}
		}
		if(trans->in_id!=0)
		{
			if(!trans->to_state->hook)
			{
				ts=wtk_e2fst_pop_state2(e);
				//trans->to_state->hook=ts;
				//in_id=inmap[trans->in_id-1];
				in_id=e->cfg->hmm_maps[trans->in_id-1]->pdf->forward_id;
				//wtk_debug("%d %d %d\n",trans->in_id,in_id,e->cfg->hmm_maps[trans->in_id-1]->pdf->selfloop_id);

				wtk_fst_net2_link_state3(net,pre,ts,0,in_id,trans->out_id,0,0.20794);
				//tt=wtk_fst_net2_link_state3(net,pre,ts,0,in_id,trans->out_id,0,0.69336);

				//wtk_debug("add trans from %d to %d    %d %d\n",pre->id,ts->id,in_id,trans->out_id);
				//printf("%d %d %d %d 0.69336\n",pre->id,ts->id,in_id,trans->out_id);

				//wtk_e2fst_link_trans(e,trans,NULL,tt);
				in_id=e->cfg->hmm_maps[trans->in_id-1]->pdf->selfloop_id;

				//tt=wtk_fst_net2_link_state3(net,ts,ts,0,in_id,0,0,0.086305);
				wtk_fst_net2_link_state3(net,ts,ts,0,in_id,0,0,0.20794);

				//wtk_debug("add trans from %d to %d    %d 0\n",ts->id,ts->id,in_id);
				//printf("%d %d  %d 0 0.086305\n",ts->id,ts->id,in_id);

				//wtk_e2fst_link_trans(e,trans,NULL,tt);
				ts2=wtk_e2fst_pop_state2(e);

				wtk_fst_net2_link_state3(net,ts,ts2,0,0,0,0,0.69336);

				//wtk_debug("add trans from %d to %d    0 0\n",ts->id,ts2->id);
				//printf("%d %d 0 0 0.41589\n",ts->id,ts2->id);

				//wtk_e2fst_link_trans(e,trans,NULL,tt);
				trans->to_state->hook=ts2;
				wtk_e2fst_expand_hmm_net_node(e,ts2,trans,(wtk_fst_state2_t*)trans->to_state);
			}else
			{
				//TODO
				ts=wtk_e2fst_pop_state2(e);
				//trans->to_state->hook=ts;
				//in_id=inmap[trans->in_id-1];
				in_id=e->cfg->hmm_maps[trans->in_id-1]->pdf->forward_id;
				//wtk_debug("%d %d\n",in_id,trans->in_id);
				//wtk_debug("%d %d %d\n",trans->in_id,in_id,e->cfg->hmm_maps[trans->in_id-1]->pdf->selfloop_id);
				wtk_fst_net2_link_state3(net,pre,ts,0,in_id,trans->out_id,0,0.20794);
				//tt=wtk_fst_net2_link_state3(net,pre,ts,0,in_id,trans->out_id,0,0.69336);

				//wtk_debug("add trans from %d to %d    %d %d\n",pre->id,ts->id,in_id,trans->out_id);
				//printf("%d %d %d %d 0.69336\n",pre->id,ts->id,in_id,trans->out_id);

				//wtk_e2fst_link_trans(e,trans,NULL,tt);
				in_id=e->cfg->hmm_maps[trans->in_id-1]->pdf->selfloop_id;
				wtk_fst_net2_link_state3(net,ts,ts,0,in_id,0,0,0.20794);
				//tt=wtk_fst_net2_link_state3(net,ts,ts,0,in_id,0,0,0.086305);

				//wtk_debug("add trans from %d to %d    %d 0\n",ts->id,ts->id,in_id);
				//printf("%d %d %d 0 0.086305\n",ts->id,ts->id,in_id);

				//wtk_e2fst_link_trans(e,trans,NULL,tt);

				wtk_fst_net2_link_state3(net,ts,trans->to_state->hook,0,0,0,0,0.69336);

				//wtk_debug("add trans from %d to %d    0 0\n",ts->id,tt->to_state->id);
				//printf("%d %d 0 0 0.41589\n",ts->id,tt->to_state->id);

				//wtk_e2fst_link_trans(e,trans,NULL,tt);
			}
		}else
		{
			ts2=wtk_e2fst_pop_state2(e);

			wtk_fst_net2_link_state3(net,pre,ts2,0,0,0,0,0.41589);

			//wtk_debug("add trans from %d to %d    0 0\n",pre->id,tt->to_state->id);
			//printf("%d %d 0 0 0.41589\n",pre->id,tt->to_state->id);

			//wtk_e2fst_link_trans(e,trans,NULL,tt);
			wtk_e2fst_expand_hmm_net_node(e,ts2,trans,(wtk_fst_state2_t*)trans->to_state);
		}
	}
	//wtk_debug("-------\n");
}

void wtk_e2fst_expand_hmm_net_node_eval(wtk_e2fst_t *e,wtk_fst_state2_t* pre,
		wtk_fst_trans2_t *a,wtk_fst_state2_t *nxt_state)
{
	wtk_fst_net2_t *net=e->hmm_net;
	wtk_fst_trans2_t *trans;
	wtk_fst_state2_t *ts=0,*ts2=0;
	int in_id;
	float selfloop_scale=e->cfg->selfloop_scale;
	float transition_scale=e->cfg->transition_scale;
	wtk_e2fst_hmm_expand_t *expand;
	wtk_e2fst_hmm_expand_pdf_t *pdf;
	int plus=1;

	//wtk_debug("%d\n",nxt_state->id);
	if(nxt_state->type == WTK_FST_FINAL_STATE)
	{
		//in_id=inmap[a->in_id-1];
		if(a->in_id== 0 || a->in_id == e->sil_in_id || (e->cfg->use_posi && a->in_id == e->sil_S_in_id))
			in_id=0;
		else
			in_id=e->cfg->hmm_maps[a->in_id-1]->pdf->forward_id;
		//wtk_debug("a->in_id=%d in_id=%d %d\n",a->in_id,in_id,a->in_id);

		//wtk_fst_net2_link_state3(net,pre,net->end,0,in_id,a->out_id,0,0.41589);
		wtk_fst_net2_link_state3(net,pre,net->end,0,in_id,0,0,plus*SLOG(1.0));
	}
	for(trans=(wtk_fst_trans2_t*)nxt_state->v.trans;trans;trans=(wtk_fst_trans2_t*)trans->hook.next)
	{
		//wtk_debug("process trans: from %d to %d %d %d\n",trans->from_state->id,trans->to_state->id,trans->in_id,trans->out_id);
		if(trans->to_state->v.trans == NULL)
		{
			if(trans->to_state->type != WTK_FST_FINAL_STATE)
			{
				continue;
			}
		}
		if(trans->in_id!=0)
		{
			expand=e->cfg->hmm_maps[trans->in_id-1];
			pdf=expand->pdf;
			if(!trans->to_state->hook)
			{
				ts=wtk_e2fst_pop_state2(e);
				//trans->to_state->hook=ts;
				//in_id=inmap[trans->in_id-1];
				in_id=pdf->forward_id;
				//wtk_debug("%d %d %d\n",trans->in_id,in_id,e->cfg->hmm_maps[trans->in_id-1]->pdf->selfloop_id);

				//wtk_fst_net2_link_state3(net,pre,ts,0,in_id,trans->out_id,0,0.20794);
				//wtk_fst_net2_link_state3(net,pre,ts,0,in_id,trans->out_id,0,plus*(SLOG(pdf->forward_weight)*selfloop_scale+transition_scale*(SLOG(pdf->forward_weight)-SLOG(1.0-pdf->selfloop_weight))));
				//wtk_fst_net2_link_state3(net,pre,ts,0,in_id,trans->out_id,0,plus*(SLOG(pdf->forward_weight)*1-transition_scale*(SLOG(pdf->forward_weight)-SLOG(1.0-pdf->selfloop_weight))));
				wtk_fst_net2_link_state3(net,pre,ts,0,in_id,trans->out_id,0,plus*(trans->lm_like+transition_scale*(SLOG(pdf->forward_weight)-SLOG(1.0-pdf->selfloop_weight))));
				//tt=wtk_fst_net2_link_state3(net,pre,ts,0,in_id,trans->out_id,0,0.69336);

				//wtk_debug("add trans from %d to %d    %d %d\n",pre->id,ts->id,in_id,trans->out_id);
				//printf("%d %d %d %d 0.69336\n",pre->id,ts->id,in_id,trans->out_id);

				//wtk_e2fst_link_trans(e,trans,NULL,tt);
				in_id=pdf->selfloop_id;

				//tt=wtk_fst_net2_link_state3(net,ts,ts,0,in_id,0,0,0.086305);
				//wtk_fst_net2_link_state3(net,ts,ts,0,in_id,0,0,0.20794);
				wtk_fst_net2_link_state3(net,ts,ts,0,in_id,0,0,plus*(SLOG(pdf->selfloop_weight))*selfloop_scale);

				//wtk_debug("add trans from %d to %d    %d 0\n",ts->id,ts->id,in_id);
				//printf("%d %d  %d 0 0.086305\n",ts->id,ts->id,in_id);

				//wtk_e2fst_link_trans(e,trans,NULL,tt);
				ts2=wtk_e2fst_pop_state2(e);

				//wtk_fst_net2_link_state3(net,ts,ts2,0,0,0,0,0.69336);
				//wtk_fst_net2_link_state3(net,ts,ts2,0,0,0,0,plus*SLOG(1.0));
				wtk_fst_net2_link_state3(net,ts,ts2,0,0,0,0,plus*(SLOG(1.0-pdf->selfloop_weight))*selfloop_scale);

				//wtk_debug("add trans from %d to %d    0 0\n",ts->id,ts2->id);
				//printf("%d %d 0 0 0.41589\n",ts->id,ts2->id);

				//wtk_e2fst_link_trans(e,trans,NULL,tt);
				trans->to_state->hook=ts2;
				wtk_e2fst_expand_hmm_net_node_eval(e,ts2,trans,(wtk_fst_state2_t*)trans->to_state);
			}else
			{
				//TODO
				ts=wtk_e2fst_pop_state2(e);
				//trans->to_state->hook=ts;
				//in_id=inmap[trans->in_id-1];
				in_id=pdf->forward_id;
				//wtk_debug("%d %d\n",in_id,trans->in_id);
				//wtk_debug("%d %d %d\n",trans->in_id,in_id,e->cfg->hmm_maps[trans->in_id-1]->pdf->selfloop_id);
				//wtk_fst_net2_link_state3(net,pre,ts,0,in_id,trans->out_id,0,0.20794);
				wtk_fst_net2_link_state3(net,pre,ts,0,in_id,trans->out_id,0,plus*(trans->lm_like+SLOG(pdf->forward_weight)*selfloop_scale+transition_scale*(SLOG(pdf->forward_weight)-SLOG(1.0-pdf->selfloop_weight))));
				//tt=wtk_fst_net2_link_state3(net,pre,ts,0,in_id,trans->out_id,0,0.69336);

				//wtk_debug("add trans from %d to %d    %d %d\n",pre->id,ts->id,in_id,trans->out_id);
				//printf("%d %d %d %d 0.69336\n",pre->id,ts->id,in_id,trans->out_id);

				//wtk_e2fst_link_trans(e,trans,NULL,tt);
				in_id=pdf->selfloop_id;
				//wtk_fst_net2_link_state3(net,ts,ts,0,in_id,0,0,0.20794);
				wtk_fst_net2_link_state3(net,ts,ts,0,in_id,0,0,plus*(SLOG(pdf->selfloop_weight)*selfloop_scale));
				//tt=wtk_fst_net2_link_state3(net,ts,ts,0,in_id,0,0,0.086305);

				//wtk_debug("add trans from %d to %d    %d 0\n",ts->id,ts->id,in_id);
				//printf("%d %d %d 0 0.086305\n",ts->id,ts->id,in_id);

				//wtk_e2fst_link_trans(e,trans,NULL,tt);

				//wtk_fst_net2_link_state3(net,ts,trans->to_state->hook,0,0,0,0,0.69336);
				wtk_fst_net2_link_state3(net,ts,trans->to_state->hook,0,0,0,0,plus*SLOG(1.0));

				//wtk_debug("add trans from %d to %d    0 0\n",ts->id,tt->to_state->id);
				//printf("%d %d 0 0 0.41589\n",ts->id,tt->to_state->id);

				//wtk_e2fst_link_trans(e,trans,NULL,tt);
			}
		}else
		{
			ts2=wtk_e2fst_pop_state2(e);

			wtk_fst_net2_link_state3(net,pre,ts2,0,0,0,0,plus*(trans->lm_like+SLOG(1.0)));

			//wtk_debug("add trans from %d to %d    0 0\n",pre->id,tt->to_state->id);
			//printf("%d %d 0 0 0.41589\n",pre->id,tt->to_state->id);

			//wtk_e2fst_link_trans(e,trans,NULL,tt);
			wtk_e2fst_expand_hmm_net_node_eval(e,ts2,trans,(wtk_fst_state2_t*)trans->to_state);
		}
	}
	//wtk_debug("-------\n");
}

void wtk_e2fst_reset_state_hook(wtk_e2fst_t *e)
{
	wtk_slist_node_t *sn;
	wtk_fst_state2_t *s;

	for(sn=e->state_l.prev;sn;sn=sn->prev)
	{
		s=data_offset(sn,wtk_fst_state2_t,q_n);
		s->hook=NULL;
		//wtk_debug("%d\n",s->id);
	}
}

int wtk_hmm_expand(wtk_e2fst_t *e)
{
	//wtk_debug("-------------------------------------------\n");
	wtk_fst_state2_t *state=NULL;
	wtk_fst_net2_t *net=e->hmm_net;

	wtk_e2fst_reset_state_hook(e);
	state=wtk_e2fst_pop_state2(e);
	//state->hook=(void*)(long)-1;
	net->start=state;
	net->end=wtk_e2fst_pop_state2(e);
	net->end->type=WTK_FST_FINAL_STATE;
	//wtk_strbuf_t* buf = wtk_strbuf_new(1024,1);
	//wtk_fst_net2_print_fsm(mono_net,buf);
	//printf("%.*s\n",buf->pos,buf->data);
	if(e->cfg->use_chain)
	{
		if (e->cfg->use_eval)
			wtk_e2fst_expand_hmm_net_node_eval(e,state,NULL,(wtk_fst_state2_t*)e->net->start);
		else
			wtk_e2fst_expand_hmm_net_node(e,state,NULL,(wtk_fst_state2_t*)e->net->start);
	}
	else
		wtk_e2fst_expand_hmm_net_node2(e,state,NULL,(wtk_fst_state2_t*)e->net->start);
	return 0;
}

void wtk_e2fst_mono_net_process(wtk_e2fst_t *e, wtk_fst_net2_t *output,
                                wtk_fst_state2_t *wrd_s, wtk_fst_state2_t *from,
                                wtk_fst_state2_t *contact_state) {
    wtk_fst_trans2_t *trans;
    wtk_fst_state2_t *wrd_e;
    // wtk_debug("process state:%d\n",from->id);
    for (trans = (wtk_fst_trans2_t *)from->v.trans; trans;
         trans = (wtk_fst_trans2_t *)trans->hook.next) {

        // printf("%d %d %d
        // %d\n",wrd_s->id,wrd_e->id,trans->in_id,trans->out_id);
        if (trans->to_state->type == WTK_FST_FINAL_STATE) {
            wtk_fst_net2_link_state(output, wrd_s, contact_state, 0, 0, 0, 0,
                                    trans->weight);
            // printf("%d %d %d
            // %d\n",from->id,trans->to_state->id,trans->in_id,trans->out_id);
            // printf("%d %d %d
            // %d\n",wrd_s->id,contact_state->id,trans->in_id,trans->out_id);
        } else {
            if (trans->to_state->hook) {
                wrd_e = (wtk_fst_state2_t *)trans->to_state->hook;
            } else {
                wrd_e = wtk_e2fst_pop_state_filler(
                    e); // wtk_fst_net2_pop_state(output);
            }
            wtk_fst_net2_link_state(output, wrd_s, wrd_e, 0, trans->in_id,
                                    trans->out_id, 0, trans->weight);

            if (!trans->to_state->hook) {
                trans->to_state->hook = wrd_e;
                wtk_e2fst_mono_net_process(e, output, wrd_e,
                                           (wtk_fst_state2_t *)trans->to_state,
                                           contact_state);
            }
        }
    }
}

void wtk_e2fst_add_filler(wtk_e2fst_t *e, wtk_fst_net2_t *input,
                          wtk_fst_net2_t *output) {
    wtk_fst_state2_t *state[126];
    wtk_fst_state2_t *start_state;
    wtk_fst_trans2_t *start_trans;
    wtk_fst_state_t *fstate;
    wtk_fst_trans_t *ftrans;
    wtk_fst_net_t *filler_net = e->filler_net;

    int i, j, ntrans;
    for (i = 0; i <= 125; i++) // state[130] final state
    {
        state[i] =
            wtk_e2fst_pop_state_filler(e); // wtk_fst_net2_pop_state(output);
    }
    output->start = state[0];
    state[125]->type = WTK_FST_FINAL_STATE;
    // wtk_debug("xxxxx %p %d
    // %d\n",(state[130]),state[130]->id,state[130]->type);
    for (i = 0; i <= 123; i++) {
        fstate = wtk_fst_net_get_load_state(filler_net, i);

        if (i != 0) {
            // printf("%d %d %d %d\n",state[i]->id,state[129]->id,0,0);
            wtk_fst_net2_link_state(output, state[i], state[124], 0, 0, 0, 0,
                                    0);
        }

        for (ntrans = fstate->ntrans, ftrans = fstate->v.trans, j = ntrans;
             j > 0; j--, ++ftrans) {
            // printf("%d %d %d
            // %d\n",state[i]->id,state[ftrans->to_state->id]->id,ftrans->in_id,ftrans->out_id);
            wtk_fst_net2_link_state(
                output, state[i], state[ftrans->to_state->id], 0, ftrans->in_id,
                ftrans->out_id, 0, ftrans->weight);
        }
        if (fstate->type == WTK_FST_FINAL_STATE) {
            wtk_fst_net2_link_state(output, state[i], state[125], 0, 0, 0, 0,
                                    fstate->weight);
            // printf("%d %d %d %d\n",state[i]->id,state[130]->id,0,0);
            // wtk_debug("%d %d\n",i,fstate->type);
        }
    }
    start_trans = (wtk_fst_trans2_t *)input->start->v.trans;
    start_state = (wtk_fst_state2_t *)start_trans->to_state;
    // wtk_debug("xxxxx %d\n",start_state->id);
    // wtk_debug("===========================\n");
    wtk_e2fst_mono_net_process(e, output, state[124], start_state, state[123]);
}

int wtk_e2fst_expand(wtk_e2fst_t *e,wtk_fst_net2_t *wrd_net)
{
	//wtk_debug("N=%d L=%d cross=%d\n",wrd_net->state_id,wrd_net->trans_id,e->cfg->use_cross_wrd);
	if(e->cfg->use_cross_wrd)
	{
		/* 将字的网络扩展成发音网络
		 *
		 * (0)  一  (1)  扩展成  (0) y:一 (1) i1 (2)
		 */
		e->wdec_cnt = e->egram->xbnf->xbnf->wdec_cnt;
		// wtk_debug("%d\n",e->wdec_cnt);
		wtk_e2fst_expand_pron_mono(e,wrd_net,e->mono_net);

		if (e->cfg->add_filler) {
			wtk_e2fst_add_filler(e, e->mono_net, e->full_mono_net);
		}
		// wtk_e2fst_write_mono(e);
		// wtk_egram_write_txtmono(e->egram,"egram.mono2.txt","out.mono.fsm2");
		// //by dmd exit(0);

		/**
		 * 根据发音网络扩展成cross-word网络
		 */
		if (e->cfg->use_biphone) {
			if (e->cfg->add_filler) {
				wtk_e2fst_expand_bi_net(e, e->full_mono_net);
			} else {
				wtk_e2fst_expand_bi_net(e, e->mono_net);
			}
		} else {
			if (e->cfg->add_filler) {
				wtk_e2fst_expand_tri_net(e, e->full_mono_net);
			} else {
				wtk_e2fst_expand_tri_net(e, e->mono_net);
			}
		}
		// wtk_egram_write_txtbi(e->egram,"egram.bi.txt","out.bi.fsm");
		// //by dmd wtk_e2fst_remove_dup_out(e);
		if (e->egram->cfg->use_bin) {
			if (e->cfg->type == 0) {
				wtk_e2fst_convert_id(e);
			} else {
				wtk_e2fst_convert_id_ger(e);
			}
		}
		// wtk_e2fst_remove_dup_out(e);

		if (e->cfg->remove_dup) {
			wtk_e2fst_remove_dup(e);
		}
	}else
	{
		wtk_e2fst_expand_pron_mono(e,wrd_net,e->net);
		//wtk_e2fst_convert_id2(e);
	}
	//wtk_e2fst_find_dup(e);
	return 0;
}

void wtk_e2fst_print_fsm_state(wtk_e2fst_t *e,wtk_strbuf_t *buf,wtk_slist_node_t *sn)
{
	wtk_fst_trans_t *trans;
	wtk_fst_trans2_t *t;
	wtk_fst_state2_t *s;
	wtk_string_t *v;
	char phn[3];
	int n;

	if(sn->prev)
	{
		wtk_e2fst_print_fsm_state(e,buf,sn->prev);
	}
	s=data_offset(sn,wtk_fst_state2_t,q_n);
	if(s->type==WTK_FST_FINAL_STATE)
	{
		//printf("%d\n",s->id);
		wtk_strbuf_push_f(buf,"%d\n",s->id);
	}else
	{
		for(trans=s->v.trans;trans;trans=trans->hook.next)
		{
			//wtk_debug("to=%p\n",trans->to_state->hook);
			if(e->cfg->use_txt)
			{
				if(trans->weight!=0)
				{
					//printf("%d %d %d %d %f\n",s->id,trans->to_state->id,trans->in_id,trans->out_id,trans->weight);
					wtk_strbuf_push_f(buf,"%d %d %.*s %d %f\n",s->id,trans->to_state->id,
							e->net->cfg->sym_in->ids[trans->in_id]->str->len,
							e->net->cfg->sym_in->ids[trans->in_id]->str->data,
							trans->out_id,trans->weight);
				}else
				{
					v=wtk_e2fst_get_outsym(e,trans->out_id);
					t=(wtk_fst_trans2_t*)trans;
					//print_hex((char*)&(t->frame),4);
					n=(t->frame>>24);
					phn[0]=(t->frame>>16)&0x00FF;
					phn[1]=(t->frame>>8)&0x00FF;
					phn[2]=(t->frame)&0x00FF;
					/*
					printf("[%.*s]\n",e->cfg->phn_ids[(int)(phn[0])]->len,e->cfg->phn_ids[(int)(phn[0])]->data);
					printf("[%.*s]\n",e->cfg->phn_ids[(int)(phn[1])]->len,e->cfg->phn_ids[(int)(phn[1])]->data);
					printf("[%.*s]\n",e->cfg->phn_ids[(int)(phn[2])]->len,e->cfg->phn_ids[(int)(phn[2])]->data);
					exit(0);*/
					//printf("%d %d %d %d\n",s->id,trans->to_state->id,trans->in_id,trans->out_id);
					wtk_strbuf_push_f(buf,"%d %d %.*s %.*s %d-%d",s->id,trans->to_state->id,
							e->net->cfg->sym_in->ids[trans->in_id]->str->len,
							e->net->cfg->sym_in->ids[trans->in_id]->str->data,
							v->len,v->data,trans->in_id,trans->out_id);
					//wtk_debug("n=%d\n",n);
					if(n>0)
					{
						wtk_strbuf_push_f(buf," %.*s",e->cfg->phn_ids[(int)(phn[0])]->len,e->cfg->phn_ids[(int)(phn[0])]->data);
					}
					if(n>1)
					{
						wtk_strbuf_push_f(buf,"-%.*s",e->cfg->phn_ids[(int)(phn[1])]->len,e->cfg->phn_ids[(int)(phn[1])]->data);
					}
					if(n>2)
					{
						wtk_strbuf_push_f(buf,"+%.*s",e->cfg->phn_ids[(int)(phn[2])]->len,e->cfg->phn_ids[(int)(phn[2])]->data);
					}
					wtk_strbuf_push_s(buf,"\n");
				}
			}else
			{
				if(trans->weight!=0)
				{
					//printf("%d %d %d %d %f\n",s->id,trans->to_state->id,trans->in_id,trans->out_id,trans->weight);
					wtk_strbuf_push_f(buf,"%d %d %d %d %f\n",s->id,trans->to_state->id,trans->in_id,
							trans->out_id,trans->weight);
				}else
				{
					//printf("%d %d %d %d\n",s->id,trans->to_state->id,trans->in_id,trans->out_id);
					wtk_strbuf_push_f(buf,"%d %d %d %d\n",s->id,trans->to_state->id,trans->in_id,
							trans->out_id);
				}
			}
		}
	}
}

void wtk_e2fst_print_fsm2(wtk_e2fst_t *e,wtk_strbuf_t *buf)
{
	wtk_debug("N=%d L=%d\n",e->net->state_id,e->net->trans_id);
	wtk_strbuf_reset(buf);
	wtk_e2fst_print_fsm_state(e,buf,e->state_l.prev);
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	//wtk_debug("[%d]\n",wtk_slist_len(&(e->state_l)));
	//exit(0);
}

void wtk_e2fst_print_fsm_hmmexp(wtk_e2fst_t *e,wtk_strbuf_t *buf)
{
	wtk_slist_node_t *sn;
	wtk_fst_state2_t *s;
	int i,cnt=0,out_id;
	wtk_fst_trans_t *trans;

	wtk_strbuf_reset(buf);
	for(sn=e->hmm_l.prev;sn;sn=sn->prev,cnt++)
	{
		s=data_offset(sn,wtk_fst_state2_t,q_n);

		if(s->ntrans==0)
		{
			wtk_strbuf_push_f(buf,"%d\n",s->id);
			continue;
		}
		for (i = 0, trans = s->v.trans; i < s->ntrans; ++i, ++trans)
		{
                    if (trans->out_id != 0 && e->cfg->add_filler != 1) {
                        out_id = trans->out_id - 1; // TODO ???
                    } else {
                        out_id = trans->out_id;
                    }
                        wtk_strbuf_push_f(buf,"%d %d %d %d %f\n",s->id,trans->to_state->id,trans->in_id,out_id,trans->weight);
		}
		//wtk_free(s->v.trans);
	}
}

void wtk_e2fst_print_fsm(wtk_e2fst_t *e,wtk_strbuf_t *buf)
{
	wtk_fst_trans_t *trans;
	wtk_fst_state2_t *s;
	wtk_fst_trans2_t *t;
	wtk_slist_node_t *sn;
	wtk_string_t *v;
	char phn[3];
	int n;
	float weight;

	//wtk_debug("N=%d L=%d\n",e->net->state_id,e->net->trans_id);
	wtk_strbuf_reset(buf);
	//wtk_debug("id=%d\n",e->net->state_id);
	//wtk_debug("%d\n",vi);
	//wtk_e2fst_print_fsm_state_bin(e,buf,e->state_l.prev);
	for(sn=e->state_l.prev;sn;sn=sn->prev)
	{
		s=data_offset(sn,wtk_fst_state2_t,q_n);
		if(s->type==WTK_FST_FINAL_STATE)
		{
			//printf("%d\n",s->id);
			wtk_strbuf_push_f(buf,"%d\n",s->id);
		}else
		{
			for(trans=s->v.trans;trans;trans=trans->hook.next)
			{
				//wtk_debug("to=%p\n",trans->to_state->hook);
				if(e->cfg->use_txt)
				{
					if(e->egram->cfg->use_bin)
					{
						if(trans->weight!=0)
						{
							//printf("%d %d %d %d %f\n",s->id,trans->to_state->id,trans->in_id,trans->out_id,trans->weight);
							wtk_strbuf_push_f(buf,"%d %d %.*s %d %f\n",s->id,trans->to_state->id,
									e->net->cfg->sym_in->ids[trans->in_id]->str->len,
									e->net->cfg->sym_in->ids[trans->in_id]->str->data,
									trans->out_id,trans->weight);
						}else
						{
							v=wtk_e2fst_get_outsym(e,trans->out_id);
							t=(wtk_fst_trans2_t*)trans;
							//print_hex((char*)&(t->frame),4);
							//t->frame=t->in_id;
							n=(t->frame>>24);
							phn[0]=(t->frame>>16)&0x00FF;
							phn[1]=(t->frame>>8)&0x00FF;
							phn[2]=(t->frame)&0x00FF;
							/*
							printf("[%.*s]\n",e->cfg->phn_ids[(int)(phn[0])]->len,e->cfg->phn_ids[(int)(phn[0])]->data);
							printf("[%.*s]\n",e->cfg->phn_ids[(int)(phn[1])]->len,e->cfg->phn_ids[(int)(phn[1])]->data);
							printf("[%.*s]\n",e->cfg->phn_ids[(int)(phn[2])]->len,e->cfg->phn_ids[(int)(phn[2])]->data);
							exit(0);*/
							//printf("%d %d %d %d\n",s->id,trans->to_state->id,trans->in_id,trans->out_id);
							/*
							wtk_strbuf_push_f(buf,"%d %d %.*s %.*s %d-%d",s->id,trans->to_state->id,
									e->net->cfg->sym_in->ids[trans->in_id]->str->len,
									e->net->cfg->sym_in->ids[trans->in_id]->str->data,
									v->len,v->data,trans->in_id,trans->out_id);
							*/
							wtk_strbuf_push_f(buf,"%d %d xxx %.*s %d-%d",s->id,trans->to_state->id,
									v->len,v->data,trans->in_id,trans->out_id);
							//wtk_debug("n=%d\n",n);
							if(n>0)
							{
								wtk_strbuf_push_f(buf," %.*s",e->cfg->phn_ids[(int)(phn[0])]->len,e->cfg->phn_ids[(int)(phn[0])]->data);
							}
							if(n>1)
							{
								wtk_strbuf_push_f(buf,"-%.*s",e->cfg->phn_ids[(int)(phn[1])]->len,e->cfg->phn_ids[(int)(phn[1])]->data);
							}
							if(n>2)
							{
								wtk_strbuf_push_f(buf,"+%.*s",e->cfg->phn_ids[(int)(phn[2])]->len,e->cfg->phn_ids[(int)(phn[2])]->data);
							}
							wtk_strbuf_push_s(buf,"\n");
						}
					}else
					{
						wtk_strbuf_push_f(buf,"%d %d %d=>%d %.*s:%.*s\n",s->id,trans->to_state->id,
								trans->in_id,trans->out_id,
								e->net->cfg->sym_in->ids[trans->in_id]->str->len,
								e->net->cfg->sym_in->ids[trans->in_id]->str->data,
								e->net->cfg->sym_out->strs[trans->out_id]->len,
								e->net->cfg->sym_out->strs[trans->out_id]->data);
					}
				}else
				{
					//vi=trans->in_id;
					t=(wtk_fst_trans2_t*)trans;
					weight=t->weight + t->lm_like*e->net->cfg->lmscale+e->net->cfg->wordpen;
					//wtk_debug("t=%p weight=%f lm_like=%f\n", t, t->weight, t->lm_like);
					//if(trans->weight!=0)
					if(weight!=0)
					{
						//printf("%d %d %d %d %f\n",s->id,trans->to_state->id,trans->in_id,trans->out_id,trans->weight);
						wtk_strbuf_push_f(buf,"%d %d %d %d %f\n",s->id,t->to_state->id,t->in_id,
								t->out_id,weight);
					}else
					{
						//printf("%d %d %d %d\n",s->id,trans->to_state->id,trans->in_id,trans->out_id);
						wtk_strbuf_push_f(buf,"%d %d %d %d\n",s->id,trans->to_state->id,trans->in_id,
								t->out_id);
					}
				}
			}
		}
	}
}


void wtk_e2fst_print_fsm_state_bin(wtk_e2fst_t *e,wtk_strbuf_t *buf,wtk_slist_node_t *sn)
{
	wtk_fst_trans_t *trans;
	wtk_fst_state2_t *s;
	int vi;
	int cnt;

	if(sn->prev)
	{
		wtk_e2fst_print_fsm_state_bin(e,buf,sn->prev);
	}
	s=data_offset(sn,wtk_fst_state2_t,q_n);
	if(s->type==WTK_FST_FINAL_STATE)
	{
		//printf("%d\n",s->id);
		cnt=-1;
		wtk_strbuf_push(buf,(char*)&cnt,4);
		//wtk_strbuf_push_c(buf,WTK_FST_RNET_FINAL);
		vi=s->id;
		wtk_strbuf_push(buf,(char*)&vi,4);
	}else
	{
		for(cnt=0,trans=s->v.trans;trans;trans=trans->hook.next,++cnt);
		wtk_strbuf_push(buf,(char*)&cnt,4);
		vi=s->id;
		wtk_strbuf_push(buf,(char*)&vi,4);
		for(trans=s->v.trans;trans;trans=trans->hook.next)
		{
			//wtk_debug("to=%p\n",trans->to_state->hook);
			if(trans->in_id==0)
			{
				if(trans->out_id==0)
				{
					wtk_strbuf_push_c(buf,WTK_FST_RNET_NIL);
				}else
				{
					wtk_strbuf_push_c(buf,WTK_FST_RNET_IN_NIL);
				}
			}else if(trans->out_id==0)
			{
				wtk_strbuf_push_c(buf,WTK_FST_RNET_OUT_NIL);
			}else
			{
				wtk_strbuf_push_c(buf,WTK_FST_RNET_IN_OUT);
			}
			vi=trans->to_state->id;
			wtk_strbuf_push(buf,(char*)&vi,4);
			if(trans->in_id!=0)
			{
				vi=trans->in_id;
				wtk_strbuf_push(buf,(char*)&vi,4);
			}
			if(trans->out_id!=0)
			{
				vi=trans->out_id;
				wtk_strbuf_push(buf,(char*)&vi,4);
			}
		}
	}
}


void wtk_e2fst_print_fsm_bin(wtk_e2fst_t *e,wtk_strbuf_t *buf)
{
	wtk_fst_trans_t *trans;
	wtk_fst_state2_t *s;
	wtk_slist_node_t *sn;
	int vi;
	int cnt;

	//wtk_e2fst_remove_null(e);
	wtk_strbuf_reset(buf);
	//wtk_debug("id=%d\n",e->net->state_id);
	vi=e->net->state_id;
	wtk_strbuf_push(buf,(char*)&vi,4);
	//wtk_debug("%d\n",vi);
	//wtk_e2fst_print_fsm_state_bin(e,buf,e->state_l.prev);
	for(sn=e->state_l.prev;sn;sn=sn->prev)
	{
		s=data_offset(sn,wtk_fst_state2_t,q_n);
		if(s->type==WTK_FST_FINAL_STATE)
		{
			//printf("%d\n",s->id);
			cnt=-1;
			wtk_strbuf_push(buf,(char*)&cnt,4);
			//wtk_strbuf_push_c(buf,WTK_FST_RNET_FINAL);
			vi=s->id;
			wtk_strbuf_push(buf,(char*)&vi,4);
		}else
		{
			for(cnt=0,trans=s->v.trans;trans;trans=trans->hook.next,++cnt);
//			if(cnt==0)
//			{
//				wtk_debug("%d=%d trans=%p\n",s->id,cnt,s->v.trans);
//			}
			wtk_strbuf_push(buf,(char*)&cnt,4);
			vi=s->id;
			wtk_strbuf_push(buf,(char*)&vi,4);
			for(trans=s->v.trans;trans;trans=trans->hook.next)
			{
				//wtk_debug("to=%p\n",trans->to_state->hook);
				//{
					//wtk_string_t *v;

					//v=wtk_e2fst_get_outsym(e,trans->out_id);
					//wtk_debug("in=%d|%d %.*s\n",trans->in_id,trans->out_id,v->len,v->data);
				//}
				if(trans->in_id==0)
				{
					if(trans->out_id==0)
					{
						wtk_strbuf_push_c(buf,WTK_FST_RNET_NIL);
					}else
					{
						wtk_strbuf_push_c(buf,WTK_FST_RNET_IN_NIL);
					}
				}else if(trans->out_id==0)
				{
					wtk_strbuf_push_c(buf,WTK_FST_RNET_OUT_NIL);
				}else
				{
					wtk_strbuf_push_c(buf,WTK_FST_RNET_IN_OUT);
				}
				vi=trans->to_state->id;
				wtk_strbuf_push(buf,(char*)&vi,4);
				if(trans->in_id!=0)
				{
					//int wtk_e2fst_get_phnid2(wtk_e2fst_t *e,int id)
					vi=trans->in_id;
					//vi=wtk_e2fst_get_phnid2(e,trans->in_id);
					wtk_strbuf_push(buf,(char*)&vi,4);
					/*
					if(vi<0)
					{
						wtk_debug("vi=%d\n",vi);
					}*/
				}
				if(trans->out_id!=0)
				{
					vi=trans->out_id;
					wtk_strbuf_push(buf,(char*)&vi,4);
				}
			}
		}
	}
}

int wtk_e2fst_dump(wtk_e2fst_t *e,wtk_fst_net_t *net)
{
	wtk_fst_trans_t *trans,*tt;
	wtk_fst_state2_t *s2;
	wtk_fst_state_t *s;
	wtk_slist_node_t *sn;
	int i,vi;
	int cnt;
	int e_id;

	vi=e->net->state_id;
	net->nrbin_states=vi;
	net->use_rbin=1;
	if(net->rbin_states)
	{
		wtk_free(net->rbin_states);
		net->rbin_states=NULL;
	}
	net->rbin_states=(wtk_fst_state_t*)wtk_calloc(vi,sizeof(wtk_fst_state_t));
	//wtk_debug("n=%d\n",vi);
	for(i=0;i<vi;++i)
	{
		wtk_fst_state_init(net->rbin_states+i,i);
	}
	for(sn=e->state_l.prev;sn;sn=sn->prev)
	{
		s2=data_offset(sn,wtk_fst_state2_t,q_n);
		s=net->rbin_states+s2->id;
		if(s2->type==WTK_FST_FINAL_STATE)
		{
			s->type=WTK_FST_FINAL_STATE;
			s->ntrans=0;
		}else
		{
			s->type=WTK_FST_NORM_STATE;
			for(cnt=0,trans=s2->v.trans;trans;trans=trans->hook.next,++cnt);
			if(cnt>0)
			{
				s->ntrans=cnt;
				s->v.trans=(wtk_fst_trans_t*)wtk_heap_malloc(net->heap,cnt*sizeof(wtk_fst_trans_t));
				for(i=0,trans=s2->v.trans;trans;trans=trans->hook.next,++i)
				{
					e_id=trans->to_state->id;
					tt=s->v.trans+i;
					tt->in_id=trans->in_id;
					tt->out_id=trans->out_id;
					//wtk_debug("%d/%d\n",tt->in_id,tt->out_id);
//					{
//						wtk_string_t *v2;
//
//						//v1=net->print->get_insym(net->print->ths,tt->in_id);
//						v2=net->print->get_outsym(net->print->ths,tt->out_id);
//						//wtk_debug("[%.*s/%.*s]\n",v1->len,v1->data,v2->len,v2->data);
//						wtk_debug("%d/%d [%.*s]\n",tt->in_id,tt->out_id,v2->len,v2->data);
//					}
					tt->weight=trans->weight;
					tt->hook.inst=NULL;
					tt->to_state=net->rbin_states+e_id;
				}
			}else
			{
				s->ntrans=0;
			}
		}
	}
	net->init_state=net->rbin_states+e->net->start->id;
	//wtk_debug("state=%d\n",e->hmm_net->start->id);
	return 0;
}

int wtk_e2fst_dump2(wtk_e2fst_t *e,wtk_fst_net_t *net)
{
	wtk_fst_trans_t *trans,*tt;
	wtk_fst_state2_t *s2;
	wtk_fst_state_t *s;
	wtk_slist_node_t *sn;
	wtk_fst_net_t *filler_net = e->filler_net;
	wtk_fst_state_t *fstate;
	wtk_fst_trans_t *ftrans;
	int i,vi,ntrans,j,final_num=0,base_cnt;
	int cnt;
	int e_id;
	float weight = -0.693360;

	vi=e->hmm_net->state_id;
	base_cnt = e->hmm_net->state_id - 1;
	if(filler_net != NULL && e->cfg->filler){
		vi += 137;
	}
	//wtk_debug("%d\n",net->rbin_states);
	//wtk_debug("%d\n",vi);
	net->nrbin_states=vi;
	net->use_rbin=1;
	if(net->rbin_states)
	{
		wtk_free(net->rbin_states);
		net->rbin_states=NULL;
	}
	net->rbin_states=(wtk_fst_state_t*)wtk_calloc(vi,sizeof(wtk_fst_state_t));
	//wtk_debug("%d %d\n",net->nrbin_states,sizeof(wtk_fst_state_t)*vi);
	//wtk_debug("n=%d\n",vi);
	for(i=0;i<vi;++i)
	{
		wtk_fst_state_init(net->rbin_states+i,i);
	}

	for(sn=e->hmm_l.prev;sn;sn=sn->prev,cnt++)
	{
		s2=data_offset(sn,wtk_fst_state2_t,q_n);
		s=net->rbin_states+s2->id;
		s->ntrans = s2->ntrans;
		s->frame = -1;

		if(s2->ntrans==0)
		{
			s->type=WTK_FST_FINAL_STATE;
			final_num = s2->id;
			//wtk_debug("%d\n",s2->id);
			continue;
		}else
		{
			s->type=WTK_FST_NORM_STATE;
			ntrans = s->ntrans;
			if(s->id ==0 && filler_net!=NULL  && e->cfg->filler){
				ntrans++;
			}
			s->v.trans=(wtk_fst_trans_t*)wtk_heap_malloc(net->heap,ntrans*sizeof(wtk_fst_trans_t));
		}
		for (i = 0, trans = s2->v.trans; i < s2->ntrans; ++i, ++trans)
		{
			//wtk_debug("%d %d %d %d %f\n",s2->id,trans->to_state->id,trans->in_id,trans->out_id,trans->weight);
			e_id=trans->to_state->id;
			tt=s->v.trans+i;
			tt->in_id=trans->in_id;
			tt->out_id=trans->out_id;
			tt->weight = -trans->weight;
			tt->hook.inst = NULL;
			tt->to_state=net->rbin_states+e_id;
			//printf("%d %d %d %d %f\n",s2->id,tt->to_state->id,tt->in_id,tt->out_id,tt->weight);
		}
		//wtk_free(s->v.trans);
	}

	if(filler_net != NULL  && e->cfg->filler){
		for (i = 0; i < 138; i++) {
			if(i > 0){
				s = net->rbin_states + i + base_cnt;
			}else{
				s = net->rbin_states;
			}
			fstate = wtk_fst_net_get_load_state(filler_net, i);
			if(i==4){
				s->v.trans=(wtk_fst_trans_t*)wtk_heap_malloc(net->heap,(fstate->ntrans+1)*sizeof(wtk_fst_trans_t));
				s->ntrans = fstate->ntrans + 1;
			}else if(i > 0){
				s->v.trans=(wtk_fst_trans_t*)wtk_heap_malloc(net->heap,fstate->ntrans*sizeof(wtk_fst_trans_t));
				s->ntrans = fstate->ntrans;
			}
			for (j = 0, ftrans = fstate->v.trans; j < fstate->ntrans; ++j, ++ftrans) {
				if(i == 0){
					tt=s->v.trans+s->ntrans;
					s->ntrans++;
				}else{
					tt=s->v.trans+j;
				}
				if(ftrans->in_id != 0){
					weight = -0.207940;
				}else{
					weight = -0.693360;
				}
				tt->weight = weight;
				tt->in_id = ftrans->in_id;
				tt->out_id = ftrans->out_id;
				tt->hook.inst = NULL;
				tt->to_state = net->rbin_states + base_cnt + ftrans->to_state->id;
				//wtk_debug(" %d %d %d %d %f\n",s->id,tt->to_state->id,tt->in_id,tt->out_id,tt->weight);
			}
			if(i == 4){
				tt=s->v.trans+1;
				tt->weight = 0.0;
				tt->in_id = 0;
				tt->out_id = 0;
				tt->hook.inst = NULL;
				tt->to_state = net->rbin_states + final_num;
				//wtk_debug(" %d %d %d %d %f\n",s->id,tt->to_state->id,tt->in_id,tt->out_id,tt->weight);
			}
    	}
	}

	net->init_state=net->rbin_states+e->hmm_net->start->id;
	//wtk_debug("state=%d\n",e->hmm_net->start->id);
	return 0;
}

int wtk_e2fst_dump3(wtk_e2fst_t *e,wtk_fst_net_t *net,wtk_fst_sym_t *sym,wtk_slist_t *state_l)
{
	wtk_fst_trans2_t *trans;
	wtk_fst_trans_t *tt;
	wtk_fst_state2_t *s2;
	wtk_fst_state_t *s;
	wtk_slist_node_t *sn;
	int i,vi,index;
	int cnt=0;
	int e_id;
	wtk_string_t *v;

	vi=e->wrd_net->state_id;
	//wtk_debug("%d\n",net->rbin_states);
	//wtk_debug("%d\n",vi);
	net->nrbin_states=vi;
	net->use_rbin=1;
	if(net->rbin_states)
	{
		wtk_free(net->rbin_states);
		net->rbin_states=NULL;
	}
	net->rbin_states=(wtk_fst_state_t*)wtk_calloc(vi,sizeof(wtk_fst_state_t));
	//wtk_debug("%d %d\n",net->nrbin_states,sizeof(wtk_fst_state_t)*vi);
	//wtk_debug("n=%d\n",vi);
	for(i=0;i<vi;++i)
	{
		wtk_fst_state_init(net->rbin_states+i,i);
	}
        for (sn = state_l->prev; sn; sn = sn->prev, cnt++) {
            s2 = data_offset(sn, wtk_fst_state2_t, q_n);
            s = net->rbin_states + s2->id;
            s->ntrans = s2->ntrans;
            s->frame = -1;

            if (s2->ntrans == 0) {
                s->type = WTK_FST_FINAL_STATE;
                // wtk_debug("%d\n",s2->id);
                continue;
            } else {
                s->type = WTK_FST_NORM_STATE;
                s->v.trans = (wtk_fst_trans_t *)wtk_heap_malloc(
                    net->heap, s->ntrans * sizeof(wtk_fst_trans_t));
            }
            for (i = 0, trans = (wtk_fst_trans2_t *)s2->v.trans; i < s2->ntrans;
                 ++i, trans = (wtk_fst_trans2_t *)trans->hook.next) {
                // wtk_debug("%d %d %d %d
                // %f\n",s2->id,trans->to_state->id,trans->in_id,trans->out_id,trans->weight);
                v = net->print->get_outsym(net->print->ths, trans->out_id);
                index = wtk_fst_sym_get_index(sym, v);
                e_id = trans->to_state->id;
                tt = s->v.trans + i;
                tt->in_id = index;  // trans->in_id;
                tt->out_id = index; // trans->out_id;
                tt->weight = trans->weight;
                tt->hook.inst = NULL;
                tt->to_state = net->rbin_states + e_id;
                // wtk_debug(" %d %d %d %d
                // %f\n",s2->id,tt->to_state->id,tt->in_id,tt->out_id,tt->weight);
            }
            // wtk_free(s->v.trans);
        }

        net->init_state = net->rbin_states + e->wrd_net->start->id;
        //wtk_debug("state=%d\n",e->hmm_net->start->id);
	return 0;
}
