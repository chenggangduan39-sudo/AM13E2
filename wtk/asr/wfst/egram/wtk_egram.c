#include "wtk_egram.h"
#include <ctype.h>
wtk_string_t* wtk_egram_get_output_str(wtk_egram_t *e,int id);
int wtk_egram_get_symid(wtk_egram_t *e,char *data,int len);


wtk_egram_t* wtk_egram_new2(wtk_mbin_cfg_t *b_cfg)
{
	wtk_egram_cfg_t *cfg;

	cfg=(wtk_egram_cfg_t*)b_cfg->cfg;
	return wtk_egram_new(cfg,b_cfg->rbin);
}

wtk_egram_t* wtk_egram_new(wtk_egram_cfg_t *cfg,wtk_rbin2_t *rbin)
{
	wtk_egram_t *e;

	e=(wtk_egram_t*)wtk_malloc(sizeof(wtk_egram_t));
	e->cfg=cfg;
	e->label=wtk_label_new(25007);
	if(cfg->use_bin)
	{
		if(rbin)
		{
			wtk_rbin2_item_t *item;

			item=wtk_rbin2_get(rbin,cfg->dict_fn,strlen(cfg->dict_fn));
			if(!item)
			{
				wtk_debug("[%s] not found\n",cfg->dict_fn);
				return NULL;
			}
			e->dict.kv=wtk_kvdict_new2(e->label,cfg->wrd_hash_hint,cfg->phn_hash_hint,cfg->wrd_hash_hint,
				rbin->f,item->pos,item->len);
		}else
		{
			e->dict.kv=wtk_kvdict_new(e->label,cfg->dict_fn,cfg->wrd_hash_hint,cfg->phn_hash_hint,cfg->wrd_hash_hint);
		}
	}
	e->buf=wtk_strbuf_new(10240,1);
	e->e2fst=wtk_e2fst_new(&(cfg->e2fst),e,rbin);

	e->sym.ths=e;
	e->sym.get_word=(wtk_egram_sym_get_word_f)wtk_egram_get_dict_word2;
	e->sym.get_word2=(wtk_egram_sym_get_word_f2)wtk_egram_get_dict_word3;
	e->sym.get_str=(wtk_egram_sym_get_str_f)wtk_egram_get_output_str;
	e->sym.get_id=(wtk_egram_sym_get_id_f)wtk_egram_get_symid;
	if(cfg->use_ebnf)
	{
		e->v.ebnf=wtk_ebnfnet_new(cfg,&(e->sym));
	}else
	{
		e->v.xbnf=wtk_xbnfnet_new(&(cfg->xbnfnet),&(e->sym));
		e->v.xbnf->use_wrd=cfg->use_wordnet;
		e->xbnf=e->v.xbnf;
	}
	e->ebnf=NULL;
	e->gwh=NULL;
	e->gwh2=NULL;
	e->gwh_data=NULL;
	if(cfg->use_kwdec)
	{
		e->heap = wtk_heap_new(4096);
		e->kwdec_tmp = wtk_strbuf_new(256,1);
	}else{
		e->heap = NULL;
		e->kwdec_tmp = NULL;
	}
	return e;
}

void wtk_egram_delete(wtk_egram_t *e)
{
	if(e->cfg->use_ebnf)
	{
		wtk_ebnfnet_delete(e->v.ebnf);
	}else
	{
		wtk_xbnfnet_delete(e->v.xbnf);
	}
	if(e->cfg->use_bin)
	{
		wtk_kvdict_delete(e->dict.kv);
	}
	wtk_e2fst_delete(e->e2fst);
	wtk_strbuf_delete(e->buf);
	wtk_label_delete(e->label);
	if(e->heap){
		wtk_heap_delete(e->heap);
		wtk_strbuf_delete(e->kwdec_tmp);
	}
	wtk_free(e);
}

void wtk_egram_reset(wtk_egram_t *e)
{
	e->ebnf=NULL;
	if(e->cfg->use_ebnf)
	{
		wtk_ebnfnet_reset(e->v.ebnf);
	}else
	{
		wtk_xbnfnet_reset(e->v.xbnf);
	}
	if(e->cfg->use_bin)
	{
		wtk_kvdict_reset(e->dict.kv);
	}else
	{
		wtk_dict_reset_aux(e->cfg->dict);
	}
	if(e->heap){
		wtk_heap_reset(e->heap);
		wtk_strbuf_reset(e->kwdec_tmp);
	}
	wtk_e2fst_reset(e->e2fst);
}

wtk_fst_net2_t* wtk_egram_get_net(wtk_egram_t *e)
{
	return e->e2fst->net;
}

void wtk_egram_set_word_notify(wtk_egram_t *e, wtk_dict_word_find_f gwh, void *data)
{
	e->gwh=gwh;
	e->gwh_data=data;
}

void wtk_egram_set_word_notify2(wtk_egram_t *e, wtk_dict_word_find_f2 gwh2, void *data)
{
	e->gwh2=gwh2;
	e->gwh_data=data;
}

wtk_dict_t* wtk_egram_get_dict(wtk_egram_t *e)
{
	if(e->cfg->use_bin)
		return e->dict.kv->dict;
	else
		return e->cfg->dict;
}

wtk_dict_word_t* wtk_egram_get_dict_word3(wtk_egram_t *e,char *w,int bytes, void* info)
{
	wtk_dict_word_t *dw;

	if (e->gwh2){
		dw = e->gwh2(e->gwh_data, w, bytes, info);
	}else
	{
		dw=wtk_egram_get_dict_word(e, w, bytes);
	}

	return dw;
}
/**
 * for generate pron for multi-words with "_=" connecting.
 * add by dmd
 */
wtk_dict_word_t* wtk_egram_get_dict_word2(wtk_egram_t *e,char *w,int bytes)
{
	wtk_dict_word_t *dw;

	if (e->gwh){
		dw = e->gwh(e->gwh_data, w, bytes);
	}else
	{
		dw=wtk_egram_get_dict_word(e, w, bytes);
	}

	return dw;
}

wtk_dict_word_t* wtk_egram_get_dict_word(wtk_egram_t *e,char *data,int len)
{
	//return  wtk_dict_find_word(e->cfg->dict,wrd->data,wrd->len);
	wtk_dict_word_t *dw;

	if(e->cfg->use_bin)
	{
		dw=wtk_kvdict_get_word(e->dict.kv,data,len);
	}else
	{
		dw=wtk_dict_find_word(e->cfg->dict,data,len);
	}
	if(dw && !dw->aux)
	{
		dw->aux=(void*)((long)wtk_e2fst_get_symid(e->e2fst,data,len));
		//wtk_debug("dw=%p  %d\n",dw,(int)(long)dw->aux);
	}
	//wtk_debug("[%.*s]=%p\n",len,data,dw);
	//wtk_dict_word_print(dw);
	return dw;
}

int wtk_egram_get_symid(wtk_egram_t *e,char *data,int len)
{
	return  wtk_e2fst_get_symid(e->e2fst,data,len);
}

wtk_string_t* wtk_egram_get_output_str(wtk_egram_t *e,int id)
{
	return wtk_e2fst_get_outsym(e->e2fst,id);
}

int wtk_egram_symbol_bytes(int v)
{
	if (v<=0)
	{
		return 0;
	}else if(v<=255)
	{
		return 1;
	}else if(v<=65535)
	{
		return 2;
	}else if(v<=16777215)
	{
		return 3;
	}else
	{
		printf("found bug\n");
		exit(0);
	}
}

int wtk_egram_symbol_bytes2(int v)
{
	if (v<=0)
	{
		return 0;
	}else if(v<=255)
	{
		return 1;
	}else if(v<=65535)
	{
		return 2;
	}else if(v<=4294967296)
	{
		return 3;
	}else
	{
		printf("found bug\n");
		exit(0);
	}
}

void wtk_egram_append_value(wtk_strbuf_t* buf,int v)
{
	unsigned char tmpc;
	unsigned short tmps;
	unsigned int tmpi;
	//printf("%d ",v);
	if(v<=255)
	{
		tmpc=v;
		wtk_strbuf_push(buf,(char*)&tmpc,sizeof(unsigned char));
	}else if(v<=65535)
	{
		tmps=v;
		wtk_strbuf_push(buf,(char*)&tmps,sizeof(unsigned short));
	}else if(v<=16777215)
	{
		tmpc=(v&0xFF0000)>>16;
		wtk_strbuf_push(buf,(char*)&tmpc,sizeof(unsigned char));
		tmps=(v&0x00FFFF);
		wtk_strbuf_push(buf,(char*)&tmps,sizeof(unsigned short));
	}else
	{
		tmpi=v;
		wtk_strbuf_push(buf,(char*)&tmpi,sizeof(unsigned int));
	}
}

void wtk_egram_append_symbol(wtk_strbuf_t* buf,int v)
{
	unsigned char tmpc;
	unsigned short tmps;
	unsigned int tmpi;
	//printf("%d ",v);
	if(v<=255)
	{
		tmpc=v;
		wtk_strbuf_push(buf,(char*)&tmpc,sizeof(unsigned char));
	}else if(v<=65535)
	{
		tmps=v;
		wtk_strbuf_push(buf,(char*)&tmps,sizeof(unsigned short));
	}else if(v<=4294967296)
	{
		tmpi=v;
		wtk_strbuf_push(buf,(char*)&tmpi,sizeof(unsigned int));
	}else
	{
		tmpi=v;
		wtk_strbuf_push(buf,(char*)&tmpi,sizeof(unsigned int));
	}
}

void wtk_egram_ebnf_2bin(wtk_egram_t *e)
{
	wtk_slist_node_t *sn;
	wtk_fst_state2_t *s;
	int i,v1,v2,cnt=1;
	unsigned char ty=0;
	unsigned short tmp1;
	int* idx= (int*)wtk_malloc(sizeof(int)*(e->e2fst->hmms_cnt+1));
	wtk_fst_trans_t *trans;
	wtk_strbuf_t *wfst,*id;
	wfst=wtk_strbuf_new(1024,1);
	id=wtk_strbuf_new(1024,1);
	*(idx)=0;
	for(sn=e->e2fst->hmm_l.prev;sn;sn=sn->prev,cnt++)
	{
		s=data_offset(sn,wtk_fst_state2_t,q_n);

		if(s->ntrans==0)
		{
			//printf("%d\n",s->id);
			tmp1=1;
			wtk_strbuf_push(wfst,(char*)&tmp1,sizeof(unsigned short));
			ty=0;
			wtk_strbuf_push(wfst,(char*)&ty,sizeof(unsigned char));
			//wtk_debug("item:%d %d\n",tmp1,ty);
			*(idx+cnt)=wfst->pos;
			//wtk_debug("idx:%d %d\n",cnt,wfst->pos);
			continue;
		}
		//wtk_debug("%d %d\n",s->id,s->ntrans);
		tmp1=s->ntrans;
		wtk_strbuf_push(wfst,(char*)&tmp1,sizeof(unsigned short)); //append ntrans
		//wtk_debug("item:%d ",tmp1);
		for (i = 0, trans = s->v.trans; i < s->ntrans; ++i, ++trans)
		{
                    // printf("%d %d %d %d %f\n", s->id, trans->to_state->id,
                    //       trans->in_id, trans->out_id, trans->weight);
                    ty = 3;
                    ty = ty << 6;
                    // wtk_debug("%d\n",ty);

                    if (trans->to_state->id <= 255) {
                        v1 = 0;
			}else if(trans->to_state->id<=65535)
			{
				v1=1;
			}else if(trans->to_state->id<=16777215)
			{
				v1=2;
			}else
			{
				v1=3;
			}
			ty+=(v1<<4);

                        if (trans->in_id == trans->out_id) {
                            v1 = 3;
                        } else {
                            v1 = wtk_egram_symbol_bytes(trans->in_id);
                        }
                        v2=wtk_egram_symbol_bytes(trans->out_id);
			v1=v1<<2;
                        v2 = v1 + v2;
                        ty+=v2;
                        // printf("%d ",ty);
                        wtk_strbuf_push(
                            wfst, (char *)&ty,
                            sizeof(unsigned char)); // append agreement

                        if(trans->in_id!=trans->out_id && trans->in_id>0)//append outid
			{
				wtk_egram_append_value(wfst,trans->in_id);
			}
			if(trans->out_id>0)//append inid
			{
				wtk_egram_append_value(wfst,trans->out_id);
			}
			wtk_egram_append_value(wfst,trans->to_state->id);//append to state id
			wtk_strbuf_push(wfst,(char*)&(trans->weight),sizeof(float));//append weight
			//printf("%f ",trans->weight);
		}
		*(idx+cnt)=wfst->pos;
	}

	int n;
	n=(e->e2fst->hmms_cnt+1)*sizeof(int);
	wtk_strbuf_push_string(id,"X==A");
	wtk_strbuf_push(id,(char*)&n,sizeof(int));
	wtk_strbuf_push(id,(char*)idx,sizeof(int)*(e->e2fst->hmms_cnt+1));//idf
        wtk_free(idx);
        //idf+wfst=wtk_fsm_res
	FILE *fsmf;
	fsmf=fopen("./fsm.bin","wb");
	fwrite(id->data,id->pos,1,fsmf);
	fwrite(wfst->data,wfst->pos,1,fsmf);
	fflush(fsmf);
	fclose(fsmf);

	wtk_egram_get_outsym_txt(e,e->buf);
	//e->buf= wtk_outsym_res

	wtk_strbuf_delete(wfst);
	wtk_strbuf_delete(id);
}

void wtk_egram_ebnf_2bin_kdec(wtk_egram_t *e,wtk_strbuf_t *res)
{
	wtk_slist_node_t *sn;
	wtk_fst_state2_t *s;
	int i,v1,v2,cnt=1;
	unsigned char ty=0;
	unsigned int tmp2;
	uint64_t* idx= (uint64_t*)wtk_malloc(sizeof(uint64_t)*(e->e2fst->hmms_cnt+1));
	wtk_fst_trans_t *trans;
	wtk_strbuf_t *wfst,*id;
	wfst=wtk_strbuf_new(1024,1);
	id=wtk_strbuf_new(1024,1);
	*(idx)=0;
	for(sn=e->e2fst->hmm_l.prev;sn;sn=sn->prev,cnt++)
	{
		s=data_offset(sn,wtk_fst_state2_t,q_n);

		if(s->ntrans==0)
		{
			//printf("%d\n",s->id);
			tmp2=1;
			wtk_strbuf_push(wfst,(char*)&tmp2,sizeof(unsigned int));
			ty=0;
			wtk_strbuf_push(wfst,(char*)&ty,sizeof(unsigned char));
			//wtk_debug("item:%d %d\n",tmp1,ty);
			*(idx+cnt)=wfst->pos;
			//wtk_debug("idx:%d %d\n",cnt,wfst->pos);
			continue;
		}
		//wtk_debug("%d %d\n",s->id,s->ntrans);
		tmp2=s->ntrans;
		wtk_strbuf_push(wfst,(char*)&tmp2,sizeof(unsigned int)); //append ntrans
		//wtk_debug("item:%d ",tmp1);
		for (i = 0, trans = s->v.trans; i < s->ntrans; ++i, ++trans)
		{
			//printf("%d %d %d %d %f\n",s->id,trans->to_state->id,trans->in_id,trans->out_id,trans->weight);
			ty=3;
			ty=ty<<6;

			if(trans->to_state->id<=255)
			{
				v1=0;
			}else if(trans->to_state->id<=65535)
			{
				v1=1;
			}else if(trans->to_state->id<=16777215)
			{
				v1=2;
			}else
			{
				v1=3;
			}
			ty+=(v1<<4);

			v1=wtk_egram_symbol_bytes2(trans->in_id);
			v2=wtk_egram_symbol_bytes2(trans->out_id);
			v1=v1<<2;
			v2=v1+v2;
			ty+=v2;
			//printf("%d ",ty);
			wtk_strbuf_push(wfst,(char*)&ty,sizeof(unsigned char));   //append agreement

			if(trans->in_id>0)//append outid
			{
				wtk_egram_append_symbol(wfst,trans->in_id);
			}
			if(trans->out_id>0)//append inid
			{
				wtk_egram_append_symbol(wfst,trans->out_id);
			}
			wtk_egram_append_value(wfst,trans->to_state->id);//append to state id
			wtk_strbuf_push(wfst,(char*)&(trans->weight),sizeof(float));//append weight
			//printf("%f ",trans->weight);
		}
		*(idx+cnt)=wfst->pos;
		//wtk_debug("%d %d\n",cnt,wfst->pos);
	}

	uint64_t n;
	n=(e->e2fst->hmms_cnt+1)*sizeof(uint64_t);
	wtk_strbuf_push_string(id,"X==A");
	wtk_strbuf_push(id,(char*)&n,sizeof(uint64_t));
	wtk_strbuf_push(id,(char*)idx,sizeof(uint64_t)*(e->e2fst->hmms_cnt+1));//idf
	wtk_free(idx);
	//idf+wfst=wtk_fsm_res
/*	FILE *fsmf;
	fsmf=fopen("./fsm.bin","wb");
	fwrite(id->data,id->pos,1,fsmf);
	fwrite(wfst->data,wfst->pos,1,fsmf);
	fflush(fsmf);
	fclose(fsmf);*/

	wtk_egram_get_outsym_txt(e,e->buf);
        // e->buf= wtk_outsym_res
        wtk_strbuf_reset(res);
        wtk_strbuf_push(res, id->data, id->pos);
        wtk_strbuf_push(res, wfst->data, wfst->pos);

        wtk_strbuf_delete(wfst);
        wtk_strbuf_delete(id);
}

short wtk_egram_float2fix(float a, int r)
{
	int fix;
	if(a  < 0.0)
	{
		fix = (int)(a * (1<<r) - 0.5);
	}else
	{
		fix = (int)(a * (1<<r) + 0.5);
	}

    if(fix > 32767)
    {
        fix = 32767;
    }
    else if(fix < -32768)
    {
    	fix = -32768;
    }

	return fix;
}

void wtk_egram_print_res(wtk_strbuf_t* buf,char *fn)
{
	FILE *fsmf;
	fsmf=fopen(fn,"wb");
	int i;
	unsigned char c;

	for(i = 1;i <= buf->pos;i++)
	{
		c = buf->data[i-1];
		fprintf(fsmf,"0x%02x, ",c);
		if(i%20==0)
		{
			fprintf(fsmf,"\n");
		}
	}
}

void wtk_egram_mdl_remake(unsigned char *input,unsigned char *out,int len, int x)
{
	unsigned char *tmp = out;
	int i,j,m,n,row,col,col2=x,col3;

	row = len/3;
	col = x/4;

	for(i=0;i<row;i++)
	{
		for(j=0;j<col;j++)
		{
			for(m=0;m<3;m++)
			{
				*tmp++ = *(input + (3*i+m)*x + 4*j);
				*tmp++ = *(input + (3*i+m)*x + 4*j+2);
				*tmp++ = *(input + (3*i+m)*x + 4*j+1);
				*tmp++ = *(input + (3*i+m)*x + 4*j+3);
			}
		}
		j = col;
		col3 = col2 & 3;
		for(m=0;m<3;m++)
		{
			for(n=0;n<col3;n++)
			{
				*tmp = *(input + (3*i+m)*x + 4*j+n);
				tmp++;
			}
		}
	}

	i = len/3;
	row = len%3;
	col = x/4;
	for(m=0;m<row;m++)
	{
		for(j=0;j<col;j++)
		{
			*tmp++ = *(input + (3*i+m)*x + 4*j);
			*tmp++ = *(input + (3*i+m)*x + 4*j+2);
			*tmp++ = *(input + (3*i+m)*x + 4*j+1);
			*tmp++ = *(input + (3*i+m)*x + 4*j+3);
		}
		j = col;
		col3 = col2 & 3;
		for(n=0;n<col3;n++)
		{
			*tmp = *(input + (3*i+m)*x + 4*j+n);
			tmp++;
		}
	}

//	tmp = l;
//	for(i = 0; i < len; i++)
//	{
//		for(j = 0; j < 40; j++,tmp++)
//		{
//			printf("%d\n",*tmp);
//		}
//	}
}
//wtk_ecut_cfg_t chn_ecut;
void wtk_egram_mdl_repack(wtk_egram_t *e,short *map, int len)
{
	int i,j;
	int col = e->ecut->num_col;
	unsigned char *tmp;
	short *lmin = (short*)wtk_malloc(sizeof(short)*len);//TODO fbank num_chn
	unsigned char *lshift = (unsigned char*)wtk_malloc(sizeof(char)*len);
	unsigned char *l = (unsigned char*)wtk_malloc(sizeof(char)*len*col);
	//wtk_debug("%d %d\n",len,col);
	unsigned char *b = (unsigned char*)wtk_malloc(sizeof(char)*len);
	unsigned char *l_re = (unsigned char*)wtk_malloc(sizeof(char)*len*col);
	wtk_strbuf_t *mdl = wtk_strbuf_new(1024,1);

	tmp = l;
	for(i = 0; i < len; i++)
	{
		lmin[i] = *(e->ecut->lmin + map[i]);
		lshift[i] = *(e->ecut->lshift + map[i]);
		b[i] = *(e->ecut->b + map[i]);
		for(j = 0; j < col; j++,tmp++)
		{
			*tmp = *(e->ecut->l + map[i]*col +j);
		}
	}
	wtk_egram_mdl_remake(l,l_re,len,col);
	//wtk_debug("%d\n",len);
	i = col;j=len;

	wtk_strbuf_push(mdl,(char*)(e->ecut->mdl_info),sizeof(char)*9);
	//row col
	wtk_strbuf_push(mdl,(char*)&j,sizeof(unsigned int));
	wtk_strbuf_push(mdl,(char*)&i,sizeof(unsigned int));
	//linear vmin,shift,v
	wtk_strbuf_push(mdl,(char*)lmin,sizeof(short)*len);
	wtk_strbuf_push(mdl,(char*)lshift,sizeof(char)*len);
	wtk_strbuf_push(mdl,(char*)l_re,sizeof(char)*len*col);
	//bias vmin,shift,v
	wtk_strbuf_push(mdl,(char*)&(e->ecut->min),sizeof(short));
	wtk_strbuf_push(mdl,(char*)&(e->ecut->shift),sizeof(char));
	wtk_strbuf_push(mdl,(char*)b,sizeof(char)*len);

	//wtk_egram_print_res(mdl,"mdl");
//	tmp = l;
//	for(i = 0; i < len; i++)
//	{
//		for(j = 0; j < 40; j++,tmp++)
//		{
//			printf("%d\n",*tmp);
//		}
//	}s
	FILE *fsmf;
	fsmf=fopen(e->cfg->mdl_fn,"wb");
	fwrite(mdl->data,1,mdl->pos,fsmf);
	fflush(fsmf);
	fclose(fsmf);

	wtk_free(lmin);
	wtk_free(lshift);
	wtk_free(l);
	wtk_free(b);
	wtk_free(l_re);
	wtk_strbuf_delete(mdl);
}

void wtk_egram_qlite_bin(wtk_egram_t *e,wtk_strbuf_t *res)
{
	wtk_slist_node_t *sn;
	wtk_fst_state2_t *s;
	int i,j,cnt=1;
	//unsigned char ty=0;
	unsigned int tmp2;
	int* idx= (int*)wtk_calloc((e->e2fst->hmms_cnt+1),sizeof(int));
	int* idx2= (int*)wtk_calloc((e->e2fst->hmms_cnt+1),sizeof(int));

	wtk_fst_trans_t *trans;
	wtk_strbuf_t *wfst;
	wfst=wtk_strbuf_new(1024,1);
	*(idx)=0;
	int *xx;
	xx=idx;
	for(sn=e->e2fst->hmm_l.prev;sn;sn=sn->prev,cnt++)
	{
		xx++;
		s=data_offset(sn,wtk_fst_state2_t,q_n);
		if(s->type==WTK_FST_FINAL_STATE)
		{
			//printf("%d\n",s->id);
			*xx = 3;
			if(s->ntrans)
			{
				wtk_debug("unsupport\n");
				exit(0);
			}
			//wtk_debug("%d\n",*xx);
			continue;
		}
		//wtk_debug("%d %d\n",s->id,s->ntrans);
		tmp2=s->ntrans;
		//wtk_strbuf_push(wfst,(char*)&tmp2,sizeof(unsigned int)); //append ntrans
		//wtk_debug("item:%d ",tmp1);
		*xx += 1;
		for (i = 0, trans = s->v.trans; i < s->ntrans; ++i, ++trans)
		{
			//*xx += 1;
			//printf("%d %d %d %d %f\n",s->id,trans->to_state->id,trans->in_id,trans->out_id,trans->weight);
			if(trans->out_id >2)
			{
				trans->out_id = trans->out_id -2;
			}else
			{
				trans->out_id = 0;
			}

			if(trans->to_state->id != s->id)
			{
				*xx += 2;
			}
			if(trans->in_id!=0)
			{
				*xx += 1;
			}
			if(trans->out_id!=0)
			{
				*xx += 1;
			}
			*xx += 3;
		}
		//wtk_debug("%d\n",*xx);
	}

	idx2[0] = 0;
	for(i=1;i<e->e2fst->hmms_cnt+1;i++)
	{
		idx2[i] = idx2[i-1] + idx[i];
		//wtk_debug("%d %d\n",i,idx2[i]);
	}

	xx=idx;
	unsigned char typ;
	short f_weight;

	f_weight = e->ecut->sil_id1;
	wtk_strbuf_push(wfst,(char*)&f_weight,sizeof(short));
	f_weight = e->ecut->sil_id2;
	wtk_strbuf_push(wfst,(char*)&f_weight,sizeof(short));
	f_weight = e->e2fst->hmms_cnt;
	wtk_strbuf_push(wfst,(char*)&f_weight,sizeof(short));
	//wtk_debug("%d %d %d\n",chn_ecut.sil_id1,chn_ecut.sil_id2,e->e2fst->hmms_cnt);
	int flag=0;
	int start_idx = e->ecut->num_cutdnn;
	short *chn_mapx = (short*)wtk_calloc(e->ecut->num_in,sizeof(short));
	memcpy(chn_mapx,e->ecut->map3,sizeof(short)*e->ecut->num_cutdnn);
	short *chn_map2 = e->ecut->map2;
	short *chn_map = e->ecut->map1;

	for(sn=e->e2fst->hmm_l.prev;sn;sn=sn->prev,cnt++)
	{
		//xx++;
		s=data_offset(sn,wtk_fst_state2_t,q_n);
		tmp2=s->ntrans;
		typ = tmp2 << 1;
		if(s->type==WTK_FST_FINAL_STATE)
		{
			//printf("%d\n",s->id);
			typ += 1;
			wtk_strbuf_push(wfst,(char*)&typ,sizeof(unsigned char));
			f_weight = wtk_egram_float2fix(s->v.weight,12);
			wtk_strbuf_push(wfst,(char*)&f_weight,sizeof(short));
			//wtk_debug("%d %d\n",typ,f_weight);
			continue;
		}
		//wtk_debug("%d %d\n",s->id,s->ntrans);

		wtk_strbuf_push(wfst,(char*)&typ,sizeof(unsigned char));
		//wtk_debug("%d\n",typ);

		for (i = 0, trans = s->v.trans; i < s->ntrans; ++i, ++trans)
		{
			if(trans->in_id > 0)
			{
				//wtk_debug("%d %d %d\n",trans->in_id,chn_map[trans->in_id],chn_map2[trans->in_id]);
				if(chn_map2[trans->in_id] == -1)
				{
					//wtk_debug("%d %d %d\n",trans->in_id,chn_map[trans->in_id],chn_map2[trans->in_id]);
					for(j=0;j<e->ecut->num_in;j++)
					{
						if(chn_mapx[j] == chn_map[trans->in_id])
						{
							//wtk_debug("aihei %d %d\n",chn_mapx[j],j);
							trans->in_id = j+1;
							flag=1;
							break;
						}
					}
					if(flag != 1)
					{
						//wtk_debug("%d %d %d\n",trans->in_id,chn_map[trans->in_id],chn_map2[trans->in_id]);
						chn_mapx[start_idx] = chn_map[trans->in_id];
						//wtk_debug("%d %d\n",start_idx,chn_map[trans->in_id]);
						trans->in_id = start_idx + 1;//chn_map[trans->in_id]+1;
						start_idx++;
					}
					flag = 0;
				}else
				{
					trans->in_id = chn_map2[trans->in_id];
				}
			}
			//printf("%d %d %d %d %f\n",s->id,trans->to_state->id,trans->in_id,trans->out_id,trans->weight);
			typ = (trans->in_id & 0b11111) << 3;
			if(s->id == trans->to_state->id)
			{
				typ += (1<<0);
			}
			if(trans->in_id == 0)
			{
				typ += (1 << 1);
			}
			if(trans->out_id == 0)
			{
				typ += (1 << 2);
			}
			wtk_strbuf_push(wfst,(char*)&typ,sizeof(unsigned char));
			//wtk_debug("%d\n",typ);
			f_weight = wtk_egram_float2fix(trans->weight,12);
			wtk_strbuf_push(wfst,(char*)&f_weight,sizeof(short));
			//wtk_debug("%d\n",f_weight);
			//printf("%d %d %d %d %f\n",s->id,trans->to_state->id,trans->in_id,trans->out_id,trans->weight);
			if(s->id != trans->to_state->id)
			{
				f_weight = *(idx2+trans->to_state->id);
				wtk_strbuf_push(wfst,(char*)&f_weight,sizeof(short));
				//wtk_debug("%d\n",f_weight);
			}
			if(trans->in_id != 0)
			{
				typ = trans->in_id >> 5;
				wtk_strbuf_push(wfst,(char*)&typ,sizeof(unsigned char));
				//wtk_debug("%d\n",typ);
			}
			if(trans->out_id != 0)
			{
				typ = trans->out_id;
				wtk_strbuf_push(wfst,(char*)&typ,sizeof(unsigned char));
				//wtk_debug("%d\n",typ);
			}
		}
	}
//
	FILE *fsmf;
	fsmf=fopen(e->cfg->fst_fn,"wb");
	fwrite(wfst->data,1,wfst->pos,fsmf);
	fflush(fsmf);
	fclose(fsmf);
	//wtk_egram_print_res(wfst,"./fstt");//fst.txt

	wtk_egram_mdl_repack(e,chn_mapx,start_idx);//mdl.txt
	//wtk_egram_mdl_repack(e,e->ecut->map3,e->ecut->num_cutdnn);//mdl.txt

	wtk_e2fst_get_outsym_bin2(e->e2fst,e->buf);//sym.txt
	FILE *fsmf3;
	fsmf3=fopen(e->cfg->sym_fn,"wb");
	fwrite(e->buf->data,1,e->buf->pos,fsmf3);
	fflush(fsmf3);
	fclose(fsmf3);
	//wtk_egram_print_res(e->buf,"./symm");

	wtk_free(chn_mapx);
	wtk_free(idx);
	wtk_free(idx2);
	wtk_strbuf_delete(wfst);
	//print_short(chn_mapx,1500);
}

int wtk_egram_ebnf2fst(wtk_egram_t *e,char *ebnf,int ebnf_bytes)
{
	wtk_fst_net2_t *net;
	wtk_string_t v;
	int ret=0;

	//wtk_debug("[%.*s]\n",ebnf_bytes,ebnf);
	e->ebnf=wtk_heap_dup_string(e->e2fst->hash->heap,ebnf,ebnf_bytes);
	net=e->e2fst->wrd_net;
	wtk_string_set(&(v),ebnf,ebnf_bytes);
	if(e->cfg->use_ebnf)
	{
		ret=wtk_ebnfnet_process(e->v.ebnf,&(v),net);
	}else
	{
		ret=wtk_xbnfnet_process(e->v.xbnf,&(v),net);
//		wtk_egram_write_txtwrd(e,"egram.word.txt","out.word.fsm");  //this may be changed structure, cause err. so must comment when submit.
//		exit(0);
	}
	if(ret!=0)
	{
		wtk_debug("procesess ebnf  failed.\n");
		goto end;
	}

	if(!e->cfg->use_wordnet)
	{
		ret=wtk_e2fst_expand(e->e2fst,net);
		if(ret!=0)
		{
			wtk_debug("expand ebnf  failed.\n");
			goto end;
		}

		//test
		//e->cfg->hmm_expand=0;
		//wtk_egram_write_txt(e,"egram.triphn.txt","out.triphn.fsm");
		//e->cfg->hmm_expand=1;
		//exit(0);
		if(e->cfg->hmm_expand)
		{
			ret=wtk_hmm_expand(e->e2fst);
			//wtk_egram_ebnf_2bin(e);
			if(ret!=0)
			{
				wtk_debug("expand ebnf to hmm failed\n");
				goto end;
			}
		}
	}
end:
	//wtk_debug("ret=%d\n",ret);
	return ret;
}

int wtk_egram_ebnf2fst2(wtk_egram_t *e,char *fn)
{
	wtk_fst_net2_t *net;
	int ret=0;

	net=e->e2fst->wrd_net;
	{
		char *data;
		int len;

		data=file_read_buf(fn,&len);
		if(data)
		{
			e->ebnf=wtk_heap_dup_string(e->e2fst->hash->heap,data,len);
			wtk_free(data);
		}
		ret=wtk_xbnfnet_process2(e->v.xbnf,fn,net);
	}
	if(ret!=0)
	{
		wtk_debug("procesess ebnf  failed.\n");
		goto end;
	}
	ret=wtk_e2fst_expand(e->e2fst,net);
	if(ret!=0)
	{
		wtk_debug("expand ebnf  failed.\n");
		goto end;
	}
end:
	//wtk_debug("ret=%d\n",ret);
	return ret;
}

wtk_egram_kwdec_trans_t* wtk_egram_kwdec_trans_new(wtk_egram_t *e,wtk_egram_kwdec_item_t *item){
	wtk_egram_kwdec_trans_t* trans = wtk_heap_malloc(e->heap,sizeof(wtk_egram_kwdec_trans_t));
	item->ntrans++;
	trans->base_state = 0;
	trans->map = NULL;
	return trans;
}

wtk_egram_kwdec_item_t* wtk_egram_kwdec_item_new(wtk_egram_t *e){
	wtk_egram_kwdec_item_t* item = wtk_heap_malloc(e->heap,sizeof(wtk_egram_kwdec_item_t));

	item->trans = (wtk_egram_kwdec_trans_t**)wtk_heap_malloc(e->heap,sizeof(wtk_egram_kwdec_trans_t*)*500);
	item->ntrans = 0;
	item->n_1st_phns = 0;
	return item;
}

void trans_debug(wtk_egram_kwdec_trans_t *trans, int d){
	wtk_debug("%d %d %d %d %d\n",d,trans->from,trans->to,trans->in,trans->out);
}

static int phn_map[]={-1,1,118,121,120,119,117,83,82,81,80,79,78,75,73,72,69,68,67,66,65,64,63
,62,36,35,34,33,32,31,29,28,27,26,24,23,22,21,20,19,18,17,16,13,10,9,8,7,6,5,4,3,2,1,-1,50,116,
115,114,113,112,102,101,100,99,93,92,91,90,89,88,88,85,84,61,59,57,56,55,54,53,52,51,51,48,47,46,
45,44,43,42,41,40,39,38,37,111,108,107,106,103,97,96,94,86,77,76,74,72,25,15,14,12,111,109,105,
11,72,30,104,98,61,58,95};

static int trans_map[18]={
	 134,546,601,613,646,896,954,960,965,1081,1106,1131,1142,1146,1149,1173,1190,1199
};

static int get_index(int id){
	int i;
	for(i = 0;i<18;i++){
		if(id == trans_map[i]){
			return i;
		}
	}
	return -1;
}

int wtk_egram_dump_kwdec(wtk_egram_t *e,wtk_fst_net_t *net)
{
	int ret = 0,vi,i,j,m,k=1,nstate = 0,base_state = 1213,arc_plus=0,cnt=3,out=0,flag;
	wtk_fst_net_print_t *print;
	wtk_fst_net_t *filler_net = e->e2fst->filler_net;
	wtk_fst_state_t *fstate;
	wtk_fst_trans_t *ftrans;
	wtk_egram_kwdec_item_t *item;
	wtk_egram_kwdec_trans_t *etrans;
	wtk_fst_state_t *s = NULL;
	wtk_fst_trans_t *tt = NULL;
	wtk_egram_kwdec_trans_t **start_trans;
	char bufx[3];

	bufx[2] = 0;
	
	print=&(e->e2fst->print);
	wtk_fst_net_print_init(print,e->e2fst,(wtk_fst_net_get_sym_f)NULL,
			(wtk_fst_net_get_sym_f)wtk_e2fst_get_outsym);
	net->print=print;

	for(i = 0;i < e->nwords; i++){
		item = e->items[i];
		nstate += (item->nstate);
		//wtk_debug("%d\n",nstate);
		arc_plus += item->n_1st_phns;
	}
	start_trans = (wtk_egram_kwdec_trans_t**)wtk_malloc(sizeof(wtk_egram_kwdec_trans_t*)*arc_plus);
	vi = base_state + nstate + 1;
	//wtk_debug("state num:%d\n",vi);
	net->nrbin_states=vi;
	net->use_rbin=1;

	if(net->rbin_states)
	{
		wtk_free(net->rbin_states);
		net->rbin_states=NULL;
	}
	net->rbin_states=(wtk_fst_state_t*)wtk_calloc(vi,sizeof(wtk_fst_state_t));
	for(i=0;i<vi;++i)
	{
		wtk_fst_state_init(net->rbin_states+i,i);
	}
	int n = 0;
	for(i = 0;i < e->nwords; i++){
		item = e->items[i];
		//wtk_debug("xxxxxx %d %d\n",item->nstate,item->ntrans);
		for(j = 0; j < item->n_1st_phns; j++){
			etrans = item->trans[j];
			etrans->base_state = base_state + etrans->to;
			start_trans[n] = etrans;
			n++;
		}

		for(j = item->n_1st_phns; j < item->ntrans; j++){
			etrans = item->trans[j];
			s = net->rbin_states + base_state + etrans->from;
			if(s->ntrans == 0){
				//wtk_debug("%d %d %d %d\n",etrans->from,etrans->to,etrans->in,etrans->out);
				s->ntrans = *(item->state_ntrans + etrans->from);
				if(j == item->ntrans - 1){
					s->ntrans++;
				}
				s->v.trans=(wtk_fst_trans_t*)wtk_heap_malloc(net->heap,s->ntrans*sizeof(wtk_fst_trans_t));
				tt=s->v.trans;
			}
			//wtk_debug("%d %d %d %d\n",etrans->from,etrans->to,etrans->in,etrans->out);
			tt->weight = -1.03972077;
			tt->in_id = etrans->in;
			tt->out_id = etrans->out;

			if(etrans->out != 0){
				out = etrans->out;
			}
			tt->hook.inst = NULL;
			tt->to_state = net->rbin_states + base_state + etrans->to;
			//wtk_debug("%d %d %d %d %f\n",s->id,tt->to_state->id,tt->in_id,tt->out_id,tt->weight);
			tt = s->v.trans + 1;

			if(j == item->ntrans -1){
				tt->weight = -1.03972077;
				tt->in_id = 0;
				tt->out_id = out;
				tt->hook.inst = NULL;
				tt->to_state = net->rbin_states + 133;
				//wtk_debug("%d %d %d %d %f\n",s->id,tt->to_state->id,tt->in_id,tt->out_id,tt->weight);			
			}
		}
		base_state += (item->nstate);
	}

	for (i = 0; i <= 1213; i++){
		n = 0;
		if(i == 54){
			continue;
		}
		fstate = wtk_fst_net_get_load_state(filler_net, i);

		for (j = 0, ftrans = fstate->v.trans; j < fstate->ntrans; ++j, ++ftrans) {
			
			if(i >= 133){
				if(j == 0){
					flag = 0;
					s = net->rbin_states + i;
					s->ntrans = fstate->ntrans;
					if(ftrans->to_state->id == 124){
						flag = 1;
						s->ntrans = fstate->ntrans + (arc_plus - 1)*2;
					}
					//wtk_debug("xxxxxxxxxx %d %d\n",s->ntrans,arc_plus);
					s->v.trans=(wtk_fst_trans_t*)wtk_heap_malloc(net->heap,s->ntrans*sizeof(wtk_fst_trans_t));
					if(ftrans->to_state->id == 124){
						for(m = 0,tt=s->v.trans; m < arc_plus; m++){
							tt->weight = -1.03972077;
							tt->in_id = 0;
							tt->out_id = 0;
							tt->hook.inst = NULL;
							tt->to_state = net->rbin_states + start_trans[m]->base_state;
							//wtk_debug("%d %d %d %d %f\n",s->id,tt->to_state->id,tt->in_id,tt->out_id,tt->weight);
							
							tt++;
							tt->weight = -1.03972077;
							int index = get_index(s->id);
							tt->in_id = start_trans[m]->map[index] - 1;
							tt->out_id = 0;
							tt->hook.inst = NULL;
							tt->to_state = s;
							//wtk_debug("%d %d %d %d %f\n",s->id,tt->to_state->id,tt->in_id,tt->out_id,tt->weight);
							tt++;
							//wtk_debug("%d\n",tt->in_id);
						}
						continue;
					}
				}
				if(flag != 1){
					tt=s->v.trans+j;
					tt->weight = ftrans->weight;
					tt->in_id = ftrans->in_id;
					tt->out_id = ftrans->out_id;
					tt->hook.inst = NULL;
					tt->to_state = net->rbin_states + ftrans->to_state->id;
					//wtk_debug("%d %d %d %d %f\n",s->id,tt->to_state->id,tt->in_id,tt->out_id,tt->weight);
				}
			}

			if(i <= 123){
				if(j == 0){
					if(ftrans->out_id == 123){
						s = net->rbin_states + i;
						s->ntrans = fstate->ntrans + arc_plus - 1;
						s->v.trans=(wtk_fst_trans_t*)wtk_heap_malloc(net->heap,(s->ntrans)*sizeof(wtk_fst_trans_t));
					}else{
						s = net->rbin_states + i;
						s->v.trans=(wtk_fst_trans_t*)wtk_heap_malloc(net->heap,fstate->ntrans*sizeof(wtk_fst_trans_t));
						s->ntrans = fstate->ntrans;						
					}
				}

				if(j == 0 && ftrans->out_id == 123){
					for(k = 0; k < arc_plus; k++){
						//wtk_debug("%d\n",start_trans[k]->out);
						tt=s->v.trans+k;
						tt->weight = -1.03972077;
						bufx[0] = phn_map[i];
						bufx[1] = start_trans[k]->in;
						int tmp =wtk_e2fst_get_phnid(e->e2fst,bufx,cnt);	
						tt->in_id = e->e2fst->cfg->hmm_maps[tmp-1]->pdf->forward_id;
						tt->out_id = start_trans[k]->out;
						//wtk_debug("======================== %d\n",start_trans[k]->base_state);
						tt->hook.inst = NULL;
						if(start_trans[k]->map == NULL){
							start_trans[k]->map = (int*)wtk_heap_malloc(e->heap,sizeof(int)*18);
						}

						tt->to_state = net->rbin_states + ftrans->to_state->id;
						int index = get_index(tt->to_state->id);
						start_trans[k]->map[index] = tt->in_id;
						//wtk_debug("%d %d %d %d %f\n",s->id,tt->to_state->id,tt->in_id,tt->out_id,tt->weight);
						//wtk_debug("%d %d %p\n",s->ntrans,k,tt);
					}
					n = arc_plus - 1;
				}else{
					tt=s->v.trans+j+n;
					tt->weight = ftrans->weight;
					tt->in_id = ftrans->in_id;
					tt->out_id = ftrans->out_id;
					tt->hook.inst = NULL;
					tt->to_state = net->rbin_states + ftrans->to_state->id;
					//wtk_debug("%d %d %d %d %f\n",s->id,tt->to_state->id,tt->in_id,tt->out_id,tt->weight);
					//wtk_debug("%d %p %p\n",s->ntrans,s->v.trans,tt);
					//wtk_debug("%d %d %d\n",s->ntrans,j,n);
				}
			}
		}

		if(fstate->type == WTK_FST_FINAL_STATE){
			s->type = WTK_FST_FINAL_STATE;
			s->weight = -fstate->weight;
			//wtk_debug("%d %f\n",s->id,s->weight);
		}
	}

	net->init_state=net->rbin_states;
	//wtk_debug("%d %p\n",net->init_state->id,net->init_state->v.trans);
	wtk_free(start_trans);
	return ret;
}

int wtk_egram_ebnf2fst_kwdec(wtk_egram_t *e,char *ebnf,int ebnf_bytes)
{
	wtk_string_t **strs;
	wtk_dict_word_t *wrd;
	wtk_dict_pron_t *pron;
	wtk_strbuf_t *buf = e->kwdec_tmp;
	wtk_egram_kwdec_item_t *item;
	char *s,*we;
	int ret=0;
	int i,n,j,nword,nphns,state_id,out_id = 123,pre_phn=-1,ntrans=0;
	int in_id,cnt=3;
	char bufx[3];
	unsigned char* ids;
	//wtk_debug("[%.*s]\n",ebnf_bytes,ebnf);
	bufx[2] = 0;
	wtk_strbuf_reset(buf);
	e->a = wtk_str_to_array(e->heap, ebnf, ebnf_bytes, '|');
	e->items = (wtk_egram_kwdec_item_t**)wtk_heap_malloc(e->heap,sizeof(wtk_egram_kwdec_item_t*)*e->a->nslot);
	strs = (wtk_string_t**)e->a->slot;
	e->nwords = e->a->nslot;
	if(e->a->nslot > 8){
		e->nwords = 8;
		return -1;//max wdec words 8
	}
	for(i=0;i<e->nwords;i++){
		e->items[i] = wtk_egram_kwdec_item_new(e);
		item = e->items[i];
		item->name = strs[i];
		s = strs[i]->data;
		we = s + strs[i]->len;
		if(strs[i]->len < 5 && strs[i]->len > 20){
			return -1;
		}
		nword = 0;
		state_id = 0;
		pre_phn = -1;
		while(s<we)
		{
			n = wtk_utf8_bytes(*s);
			if(n>1)
			{
				//wtk_string_set(&(v),s,n);
				//wtk_debug("[%.*s][len=%d]\n",n,s,n);
				wrd = wtk_egram_get_dict_word(e,s,n);
			}else
			{
				if(isspace(*s))
				{
					wrd = wtk_egram_get_dict_word(e,buf->data,buf->pos);
					wtk_strbuf_reset(buf);
				}else if(s+1==we)
				{
					wtk_strbuf_push(buf,s,n);
					wrd = wtk_egram_get_dict_word(e,buf->data,buf->pos);
					wtk_strbuf_reset(buf);
				}else
				{
					wtk_strbuf_push(buf,s,n);
					s=s+n;
					continue;
				}				
			}
			if(wrd)
			{
				nphns = 0;
				nword++;
				pron = wrd->pron_list;
				if(nword == 1){
					item->n_1st_phns = wrd->npron;
					ntrans = item->n_1st_phns;
					state_id = item->n_1st_phns;
				}

				while(pron)
				{
					ids=(unsigned char*)pron->pPhones;
					for(j=0;j<pron->nPhones;++j)
					{
						//wtk_debug("%d\n",ids[j]);
						if(nword == 1 && j==0){
							item->trans[nphns] = wtk_egram_kwdec_trans_new(e,item);
							item->trans[nphns]->in = ids[j];
							item->trans[nphns]->out = out_id;
							item->trans[nphns]->from = 0;
							item->trans[nphns]->to = nphns + 1;
							//trans_debug(item->trans[nphns],nphns);

							// item->trans[ntrans] = wtk_egram_kwdec_trans_new(e,item);
							// item->trans[ntrans]->in = ids[j];
							// item->trans[ntrans]->out = 0;
							// item->trans[ntrans]->from = nphns + 1;
							// item->trans[ntrans]->to = nphns + 1;
							// trans_debug(item->trans[ntrans],ntrans);
							// ntrans++;					
						}else{
							bufx[0] = pre_phn;
							bufx[1] = ids[j];
							int tmp =wtk_e2fst_get_phnid(e->e2fst,bufx,cnt);	

							in_id=e->e2fst->cfg->hmm_maps[tmp-1]->pdf->forward_id;					
							item->trans[ntrans] = wtk_egram_kwdec_trans_new(e,item);
							item->trans[ntrans]->in = in_id;
							item->trans[ntrans]->out = out_id;
							item->trans[ntrans]->from = state_id;
							item->trans[ntrans]->to = state_id + 1;
							//trans_debug(item->trans[ntrans],ntrans);
							state_id++;
							ntrans++;

							in_id=e->e2fst->cfg->hmm_maps[tmp-1]->pdf->selfloop_id;
							item->trans[ntrans] = wtk_egram_kwdec_trans_new(e,item);
							item->trans[ntrans]->in = in_id;
							item->trans[ntrans]->out = 0;
							item->trans[ntrans]->from = state_id;
							item->trans[ntrans]->to = state_id;
							//trans_debug(item->trans[ntrans],ntrans);						
							ntrans++;
						}
						pre_phn = ids[j];
					}
					pron=pron->next;
					nphns++;
				}
			}else
			{
				wtk_debug("Err [%.*s] word not found.\n",n,s);
				return -1;
			}
			s+=n;
		}
		item->nstate = state_id;
		item->ntrans = ntrans;
		//wtk_debug("%d %d\n",ntrans,state_id);
		out_id++;

		int state_id;
		item->state_ntrans = (int*)wtk_heap_malloc(e->heap,sizeof(int)*(item->nstate+1));
		memset(item->state_ntrans,0,(item->nstate+1)*sizeof(int));
		for(j=0;j<item->ntrans;j++){
			state_id = item->trans[j]->from;
			item->state_ntrans[state_id]++;
		}
	}
	return ret;
}

void wtk_egram_get_outsym(wtk_egram_t *e,wtk_string_t *v)
{
	wtk_strbuf_t *buf=e->buf;
	wtk_str_hash_t *hash=e->e2fst->hash;
	wtk_queue_t *q;
	wtk_queue_node_t *qn;
	hash_str_node_t *hash_n;
	long id;
	int i;

	wtk_strbuf_reset(buf);
	for(i=0;i<hash->nslot;++i)
	{
		q=hash->slot[i];
		if(!q){continue;}
		for(qn=q->pop;qn;qn=qn->next)
		{
			hash_n=data_offset(qn,hash_str_node_t,n);
			wtk_strbuf_push(buf,hash_n->key.data,hash_n->key.len);
			wtk_strbuf_push_c(buf,' ');
			id=(long)hash_n->value;
			wtk_strbuf_push_f(buf,"%d",id);
			wtk_strbuf_push_c(buf,'\n');
		}
	}
	wtk_string_set(v,buf->data,buf->pos);
}


void wtk_egram_get_outsym_txt(wtk_egram_t *e,wtk_strbuf_t *buf)
{
	wtk_e2fst_get_outsym_txt(e->e2fst,buf);
}


void wtk_egram_write_txt(wtk_egram_t *e,char *out_fn,char *fsm_fn)
{
    // wtk_egram_ebnf_2bin(e);
    wtk_egram_get_outsym_txt(e, e->buf);
    file_write_buf(out_fn, e->buf->data, e->buf->pos);
    if (e->cfg->hmm_expand) {
        wtk_e2fst_print_fsm_hmmexp(e->e2fst, e->buf);
    } else {
        wtk_e2fst_print_fsm(e->e2fst, e->buf);
    }
        file_write_buf(fsm_fn,e->buf->data,e->buf->pos);
}

void wtk_egram_write_txtmono(wtk_egram_t *e,char *out_fn,char *fsm_fn)
{
	wtk_fst_net2_t *net;
	//wtk_fst_net_print_t print;

	wtk_egram_get_outsym_txt(e,e->buf);
	file_write_buf(out_fn,e->buf->data,e->buf->pos);
	net=e->e2fst->mono_net;
//	wtk_fst_net_print_init(&print,e->xbnf,(wtk_fst_net_get_sym_f)wtk_xbnfnet_get_outsym,
//			(wtk_fst_net_get_sym_f)wtk_xbnfnet_get_outsym);
//	net->print=&(print);
	wtk_fst_net2_print_fsm(net, e->buf);
	file_write_buf(fsm_fn,e->buf->data,e->buf->pos);
}


void wtk_egram_write_txtwrd(wtk_egram_t *e,char *out_fn,char *fsm_fn)
{
	wtk_fst_net2_t *net;
	//wtk_fst_net_print_t print;

	wtk_egram_get_outsym_txt(e,e->buf);
	file_write_buf(out_fn,e->buf->data,e->buf->pos);
	net=e->e2fst->wrd_net;
//	wtk_fst_net_print_init(&print,e->xbnf,(wtk_fst_net_get_sym_f)wtk_xbnfnet_get_outsym,
//			(wtk_fst_net_get_sym_f)wtk_xbnfnet_get_outsym);
//	net->print=&(print);
	wtk_fst_net2_print_fsm(net, e->buf);
	file_write_buf(fsm_fn,e->buf->data,e->buf->pos);
}

void wtk_egram_write_bin(wtk_egram_t *e,char *out_fn,char *fsm_fn)
{
	wtk_e2fst_get_outsym_bin(e->e2fst,e->buf);
	file_write_buf(out_fn,e->buf->data,e->buf->pos);
	if(e->cfg->hmm_expand)
	{
		wtk_e2fst_print_fsm_hmmexp(e->e2fst,e->buf);
	}else
	{
		wtk_e2fst_print_fsm(e->e2fst,e->buf);
	}
	file_write_buf(fsm_fn,e->buf->data,e->buf->pos);
}

void wtk_egram_get_outsym2(wtk_egram_t *e,wtk_strbuf_t *buf)
{
	wtk_str_hash_t *hash=e->e2fst->hash;
	wtk_queue_t *q;
	wtk_queue_node_t *qn;
	hash_str_node_t *hash_n;
	int vi;
	int i;

	wtk_strbuf_reset(buf);
	for(i=0;i<hash->nslot;++i)
	{
		q=hash->slot[i];
		if(!q){continue;}
		for(qn=q->pop;qn;qn=qn->next)
		{
			hash_n=data_offset(qn,hash_str_node_t,n);
			vi=hash_n->key.len;
			wtk_strbuf_push(buf,(char*)&vi,4);
			wtk_strbuf_push(buf,hash_n->key.data,hash_n->key.len);
			vi=(long)hash_n->value;
			wtk_strbuf_push(buf,(char*)&vi,4);
		}
	}
}


void wtk_egram_get_outsym3_node(wtk_egram_t *e,wtk_strbuf_t *buf,wtk_slist_node_t *sn)
{
	wtk_e2fst_id_t *id;
	int vi;
	char bi;

	if(sn->prev)
	{
		wtk_egram_get_outsym3_node(e,buf,sn->prev);
	}
	id=data_offset(sn,wtk_e2fst_id_t,s_n);
	bi=id->v->len;
	wtk_strbuf_push(buf,(char*)&bi,1);
	wtk_strbuf_push(buf,id->v->data,id->v->len);
	vi=id->id;
	//wtk_debug("v[%d]=%d\n",i,vi);
	wtk_strbuf_push(buf,(char*)&vi,4);
}


void wtk_egram_get_outsym3(wtk_egram_t *e,wtk_strbuf_t *buf)
{
	wtk_slist_t *l=&(e->e2fst->sym_l);
	int vi;

	wtk_strbuf_reset(buf);
	vi=e->e2fst->sym_out_id+1;
	wtk_strbuf_push(buf,(char*)&vi,4);
	wtk_egram_get_outsym3_node(e,buf,l->prev);
	//wtk_debug("i=%d/%d\n",i,(int)e->e2fst->sym_out_id);
}

int wtk_egram_write(wtk_egram_t *e,char *fn)
{
	wtk_rbin2_t *rb;
	wtk_strbuf_t *buf=e->buf;
	int ret;
	wtk_string_t *v;

	rb=wtk_rbin2_new();
	wtk_egram_get_outsym3(e,buf);
	//wtk_debug("[%d]\n",buf->pos);
	v=wtk_heap_dup_string(rb->heap,buf->data,buf->pos);
	wtk_rbin2_add2(rb,&(e->cfg->symout_fn),v->data,v->len);
	wtk_e2fst_print_fsm_bin(e->e2fst,buf);
	//wtk_debug("[%d]\n",buf->pos);
	wtk_rbin2_add2(rb,&(e->cfg->fsm_fn),buf->data,buf->pos);
	//wtk_debug("ebnf_fn=%s %p\n",e->cfg->ebnf_fn.data,e->ebnf);
	if(e->ebnf)
	{
		wtk_rbin2_add2(rb,&(e->cfg->ebnf_fn),e->ebnf->data,e->ebnf->len);
	}
	ret=wtk_rbin2_write(rb,fn);
	if(ret!=0){goto end;}
	ret=0;
end:
	//wtk_debug("ret=%d\n",ret);
	wtk_rbin2_delete(rb);
	return ret;
}

int wtk_egram_dump(wtk_egram_t *e,wtk_fst_net_t *net)
{
	wtk_fst_net_print_t *print;
	print=&(e->e2fst->print);
	wtk_fst_net_print_init(print,e->e2fst,(wtk_fst_net_get_sym_f)NULL,
			(wtk_fst_net_get_sym_f)wtk_e2fst_get_outsym);
	net->print=print;
	return wtk_e2fst_dump(e->e2fst,net);
}

int wtk_egram_dump2(wtk_egram_t *e,wtk_fst_net_t *net)
{
	wtk_fst_net_print_t *print;
	print=&(e->e2fst->print);
	wtk_fst_net_print_init(print,e->e2fst,(wtk_fst_net_get_sym_f)NULL,
			(wtk_fst_net_get_sym_f)wtk_e2fst_get_outsym);
	net->print=print;
	return wtk_e2fst_dump2(e->e2fst,net);
}

int wtk_egram_dump3(wtk_egram_t *e,wtk_fst_net_t *net,wtk_fst_sym_t *sym)
{
	wtk_fst_net_print_t *print;
	print=&(e->e2fst->print);
	wtk_fst_net_print_init(print,e->e2fst,(wtk_fst_net_get_sym_f)NULL,
			(wtk_fst_net_get_sym_f)wtk_e2fst_get_outsym);
	net->print=print;
	return wtk_e2fst_dump3(e->e2fst,net,sym,&e->xbnf->state_l);
}

int wtk_egram_update_cmds(wtk_egram_t *e, char *words, int len) {
    int ret = -1;
    wtk_strbuf_t *ebnf_buf;

    wtk_egram_reset(e);
    ebnf_buf = wtk_strbuf_new(1024, 1);
    wtk_strbuf_reset(ebnf_buf);
    wtk_strbuf_push_s(ebnf_buf, "$main=");
    wtk_strbuf_push(ebnf_buf, words, len);
    wtk_strbuf_push_s(ebnf_buf, ";(\\<s\\>($main)\\<\\/s\\>)");
    // wtk_debug("%.*s\n",dec->ebnf_buf->pos,dec->ebnf_buf->data);
    ret = wtk_egram_ebnf2fst(e, ebnf_buf->data, ebnf_buf->pos);
    wtk_strbuf_delete(ebnf_buf);

    return ret;
}
