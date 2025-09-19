#include "wtk_tts_segwrd.h" 
#include <ctype.h>

wtk_tts_segwrd_t* wtk_tts_segwrd_new(wtk_tts_segwrd_cfg_t *cfg,wtk_rbin2_t *rbin)
{
	wtk_tts_segwrd_t *wrd;

	wrd=(wtk_tts_segwrd_t*)wtk_malloc(sizeof(wtk_tts_segwrd_t));
	wtk_tts_segwrd_init(wrd,cfg,rbin);
	return wrd;
}

void wtk_tts_segwrd_delete(wtk_tts_segwrd_t *wrd)
{
	wtk_tts_segwrd_clean(wrd);
	wtk_free(wrd);
}

void wtk_tts_segwrd_init(wtk_tts_segwrd_t *wrd,wtk_tts_segwrd_cfg_t *cfg,wtk_rbin2_t *rbin)
{
	wrd->cfg=cfg;
	if(cfg->use_bin)
	{
		if(rbin)
		{
			wrd->kv=wtk_fkv_new4(rbin,cfg->dict_fn,1703);
		}else
		{
			wrd->kv=wtk_fkv_new3(cfg->dict_fn);
		}
	}else
	{
		wrd->kv=NULL;
	}
	wrd->segmenter=NULL;
	if(cfg->use_maxseg)
	{
		wtk_maxseg_init(&(wrd->seg),NULL,NULL,NULL,NULL);
	}else
	{
		wrd->segmenter=wtk_segmenter_new(&(cfg->seg),rbin);
	}
	wrd->hash=wtk_str_hash_new(133);
}

void wtk_tts_segwrd_clean(wtk_tts_segwrd_t *wrd)
{
	if(wrd->segmenter)
	{
		wtk_segmenter_delete(wrd->segmenter);
	}
	wtk_str_hash_delete(wrd->hash);
	if(wrd->kv)
	{
		wtk_fkv_delete(wrd->kv);
	}
	wtk_maxseg_clean(&(wrd->seg));
}

void wtk_tts_segwrd_reset(wtk_tts_segwrd_t *wrd)
{
	if(wrd->kv)
	{
		wtk_fkv_reset(wrd->kv);
	}
}

wtk_tts_wrd_pron_t* wtk_tts_segwrd_get_wrd(wtk_tts_segwrd_t *wrd,char *k,int k_bytes)
{
	wtk_tts_wrd_pron_t *pron;
	wtk_string_t *v;

	//wtk_debug("[%.*s]\n",k_bytes,k);
	pron=wtk_str_hash_find(wrd->hash,k,k_bytes);
	if(pron){goto end;}
	v=wtk_fkv_get_str(wrd->kv,k,k_bytes);
	if(!v){goto end;}
	//wtk_debug("[%.*s]\n",v->len,v->data);
	pron=wtk_tts_wrd_pron_parse(NULL,wrd->hash->heap,v->data,v->len);
	v=wtk_heap_dup_string(wrd->hash->heap,k,k_bytes);
	wtk_str_hash_add(wrd->hash,v->data,v->len,pron);
	//exit(0);
end:
	return pron;
}

int wtk_tts_segwrd_process_max_snt(wtk_tts_segwrd_t *wrd,wtk_tts_info_t *info,wtk_tts_snt_t *s)
{
	wtk_queue_t q;
	wtk_queue_node_t *qn;
	wtk_maxseg_wrd_path_t *pth;
	wtk_tts_wrd_t *tw;
	wtk_tts_wrd_xpron_t *xpron;
	//wtk_tts_xsyl_t *xsyl;
	int wrd_pos;

	if(wrd->cfg->use_bin)
	{
		wtk_maxseg_init(&(wrd->seg),info->heap,info->buf,wrd,(wtk_maxseg_is_wrd_f)wtk_tts_segwrd_get_wrd);
	}else
	{
		wtk_maxseg_init(&(wrd->seg),info->heap,info->buf,wrd->cfg->dict,(wtk_maxseg_is_wrd_f)wtk_kdict_get);
	}
	q=wtk_maxseg_seg(&(wrd->seg),s->snt->data,s->snt->len);
	s->wrds=wtk_array_new_h(info->heap,q.length,sizeof(void*));
	s->syls=NULL;
	wrd_pos=1;
	for(qn=q.pop;qn;qn=qn->next)
	{
		pth=data_offset2(qn,wtk_maxseg_wrd_path_t,out_n);
		// wtk_debug("[%.*s]\n",pth->v->len,pth->v->data);
		tw=wtk_tts_wrd_new(info->heap,pth->v);
		tw->snt=s;
		tw->index=s->wrds->nslot+1;
		tw->valid_pos=wrd_pos;
		xpron=(wtk_tts_wrd_xpron_t*)wtk_heap_malloc(info->heap,sizeof(wtk_tts_wrd_xpron_t));
		tw->pron=xpron;
		if(wrd->cfg->use_bin)
		{
			xpron->pron=wtk_tts_segwrd_get_wrd(wrd,pth->v->data,pth->v->len);
		}else
		{
			xpron->pron=wtk_kdict_get(wrd->cfg->dict,pth->v->data,pth->v->len);
		}

//		if (!xpron->pron){  // no meaning
//			wtk_debug("Warning: doesn't find pron for [%.*s]\n", pth->v->len,pth->v->data);
//		}
		if(xpron->pron && xpron->pron->npron==1 && xpron->pron->nsyl==1 &&( wtk_string_cmp_s(xpron->pron->syls[0].v,"pau")==0))
		{
			//wtk_debug("set sil\n");
			tw->sil=1;
		}else
		{
			++wrd_pos;
		}
		//wtk_tts_wrd_print(tw);
		if(xpron->pron)
		{
			tw->bound=qn->next?WTK_TTS_BOUND_RYTHYME:WTK_TTS_BOUND_SEG;
			//wtk_debug("bound=[%.*s] %d next=%p\n",tw->v->len,tw->v->data,tw->bound,qn->next);
			wtk_array_push2(s->wrds,&tw);
		}
		xpron->xsyl=NULL;
		//else{
		//	wtk_debug("[%.*s]\n",pth->v->len,pth->v->data);
		//}
		//wtk_tts_wrd_print(tw);
	}
	return 0;
}

int wtk_tts_segwrd_process_seg_snt(wtk_tts_segwrd_t *wrd,wtk_tts_info_t *info,wtk_tts_snt_t *s)
{
	wtk_string_t *v;
	wtk_tts_wrd_t *tw;
	wtk_tts_wrd_xpron_t *xpron;
	//wtk_tts_xsyl_t *xsyl;
	int wrd_pos;
	int ret;
	int i;

	ret=wtk_segmenter_parse2(wrd->segmenter,s->snt->data,s->snt->len);
	if(ret!=0){goto end;}
	s->wrds=wtk_array_new_h(info->heap,wrd->segmenter->wrd_array_n,sizeof(void*));
	s->syls=NULL;
	wrd_pos=1;
	for(i=0;i<wrd->segmenter->wrd_array_n;++i)
	{
		v=wrd->segmenter->wrd_array[i];
		//wtk_debug("[%.*s]\n",pth->v->len,pth->v->data);
		tw=wtk_tts_wrd_new(info->heap,v);
		tw->snt=s;
		tw->index=s->wrds->nslot+1;
		tw->valid_pos=wrd_pos;
		xpron=(wtk_tts_wrd_xpron_t*)wtk_heap_malloc(info->heap,sizeof(wtk_tts_wrd_xpron_t));
		tw->pron=xpron;
		if(wrd->cfg->use_bin)
		{
			xpron->pron=wtk_tts_segwrd_get_wrd(wrd,v->data,v->len);
		}else
		{
			xpron->pron=wtk_kdict_get(wrd->cfg->dict,v->data,v->len);
		}
		if(xpron->pron && xpron->pron->npron==1 && xpron->pron->nsyl==1 &&( wtk_string_cmp_s(xpron->pron->syls[0].v,"pau")==0))
		{
			//wtk_debug("set sil\n");
			tw->sil=1;
		}else
		{
			++wrd_pos;
		}
		//wtk_tts_wrd_print(tw);
		if(xpron->pron)
		{
			tw->bound=i==(wrd->segmenter->wrd_array_n-1)?WTK_TTS_BOUND_SEG:WTK_TTS_BOUND_RYTHYME;
			//wtk_debug("bound=[%.*s] %d\n",tw->v->len,tw->v->data,tw->bound);
			wtk_array_push2(s->wrds,&tw);
		}
		//wtk_tts_wrd_print(tw);
	}
	//s->n_valid_wrd=wrd_pos;
	//exit(0);
end:
	return ret;
}

int wtk_tts_segwrd_process_seg_snt_sp(wtk_tts_segwrd_t *wrd,wtk_tts_info_t *info,wtk_tts_snt_t *s)
{
	wtk_string_t *v;
	wtk_tts_wrd_t *tw;
	wtk_tts_wrd_xpron_t *xpron;
	//wtk_tts_xsyl_t *xsyl;
	int wrd_pos;
	int ret;
	int i;
	char *ts,*te;
	int state=0;
	wtk_array_t *a;
	wtk_string_t k;

	//wtk_debug("[%.*s]\n",s->snt->len,s->snt->data);
	a=wtk_array_new_h(info->heap,s->snt->len/2,sizeof(void*));
	ts=s->snt->data;
	te=ts+s->snt->len;
	k.len=0;
	k.data=NULL;
	while(ts<te)
	{
		switch(state)
		{
		case 0:
			if(!isspace(*ts))
			{
				k.data=ts;
				state=1;
			}
			break;
		case 1:
			if(isspace(*ts))
			{
				k.len=ts-k.data;
				//wtk_debug("[%.*s]\n",k.len,k.data);
				v=wtk_heap_dup_string(info->heap,k.data,k.len);
				wtk_array_push2(a,&v);
				k.data=NULL;
				state=0;
			}
			break;
		}
		++ts;
	}
	if(k.data)
	{
		k.len=ts-k.data;
		//wtk_debug("[%.*s]\n",k.len,k.data);
		v=wtk_heap_dup_string(info->heap,k.data,k.len);
		wtk_array_push2(a,&v);
	}
	//exit(0);
	s->wrds=wtk_array_new_h(info->heap,a->nslot,sizeof(void*));
	s->syls=NULL;
	wrd_pos=1;
	for(i=0;i<a->nslot;++i)
	{
		v=((wtk_string_t**)a->slot)[i];
		//wtk_debug("[%.*s]\n",pth->v->len,pth->v->data);
		tw=wtk_tts_wrd_new(info->heap,v);
		tw->snt=s;
		tw->index=s->wrds->nslot+1;
		tw->valid_pos=wrd_pos;
		xpron=(wtk_tts_wrd_xpron_t*)wtk_heap_malloc(info->heap,sizeof(wtk_tts_wrd_xpron_t));
		tw->pron=xpron;
		if(wrd->cfg->use_bin)
		{
			xpron->pron=wtk_tts_segwrd_get_wrd(wrd,v->data,v->len);
		}else
		{
			xpron->pron=wtk_kdict_get(wrd->cfg->dict,v->data,v->len);
		}
		if(!xpron->pron)
		{
			wtk_debug("found bug null[%.*s]\n",v->len,v->data);
			exit(0);
		}
		if(xpron->pron && xpron->pron->npron==1 && xpron->pron->nsyl==1 &&( wtk_string_cmp_s(xpron->pron->syls[0].v,"pau")==0))
		{
			//wtk_debug("set sil\n");
			tw->sil=1;
		}else
		{
			++wrd_pos;
		}
		//wtk_tts_wrd_print(tw);
		if(xpron->pron)
		{
			tw->bound=i==(a->nslot-1)?WTK_TTS_BOUND_SEG:WTK_TTS_BOUND_RYTHYME;
			//wtk_debug("bound=[%.*s] %d\n",tw->v->len,tw->v->data,tw->bound);
			wtk_array_push2(s->wrds,&tw);
		}
		//wtk_tts_wrd_print(tw);
	}
	ret=0;
	return ret;
}

//英文断句
//how are you --> *how@are@you~
//@ 一级韵律
//# 二级韵律
//$ 三级韵律
//暂时把连字符认为成一级韵律
int wtk_tts_segwrd_process_seg_snt_sp2(wtk_tts_segwrd_t *wrd,wtk_tts_info_t *info,wtk_tts_snt_t *s)
{
	wtk_string_t *v;
	wtk_tts_wrd_t *tw;
	wtk_tts_wrd_xpron_t *xpron;
	//wtk_tts_xsyl_t *xsyl;
	int wrd_pos;
	int ret;
	int i;
	char *ts,*te;
	int state=0;
	wtk_array_t *a;
	wtk_string_t k;

	//wtk_debug("[%.*s]\n",s->snt->len,s->snt->data);
	a=wtk_array_new_h(info->heap,s->snt->len/2,sizeof(void*));
	ts=s->snt->data;
	te=ts+s->snt->len;
	k.len=0;
	k.data=NULL;
	v = wtk_heap_dup_string(info->heap,"*",1);	//用*开始
	wtk_array_push2(a,&v);
	while(ts<te)
	{
		switch(state)
		{
		case 0:
			if(!isspace(*ts))
			{
				k.data=ts;
				state=1;
			}
			break;
		case 1:
			if(isspace(*ts))
			{
				k.len=ts-k.data;
				// wtk_debug("[%.*s]\n",k.len,k.data);
				//最后的字符不能是标点符号,要分割存放
				if(isalpha(k.data[k.len-1]) || k.len == 1){
					v=wtk_heap_dup_string(info->heap,k.data,k.len);
					wtk_array_push2(a,&v);
					v = wtk_heap_dup_string(info->heap,"@",1);	//单独的单词要用@结尾
					wtk_array_push2(a,&v);
				}else{
					v = wtk_heap_dup_string(info->heap,k.data,k.len-1);
					wtk_array_push2(a,&v);
					v = wtk_heap_dup_string(info->heap,k.data+k.len-1,1);
					wtk_array_push2(a,&v);
				}
				k.data=NULL;
				state=0;
			}
			break;
		}
		++ts;
	}
	if(k.data)
	{
		k.len=ts-k.data;
		//wtk_debug("[%.*s]\n",k.len,k.data);
		if(isalpha(k.data[k.len-1]) || k.len == 1){	//去掉尾部标点符号
			v=wtk_heap_dup_string(info->heap,k.data,k.len);
			wtk_array_push2(a,&v);
			v = wtk_heap_dup_string(info->heap,"@",1);	//单独的单词要用@结尾
			wtk_array_push2(a,&v);
		}else{
			v = wtk_heap_dup_string(info->heap,k.data,k.len-1);
			wtk_array_push2(a,&v);
			v = wtk_heap_dup_string(info->heap,k.data+k.len-1,1);
			wtk_array_push2(a,&v);
		}
	}
	//每一句后边用~结束
	v = wtk_heap_dup_string(info->heap,"~",1);
	wtk_array_push2(a,&v);

	s->wrds=wtk_array_new_h(info->heap,a->nslot,sizeof(void*));
	s->syls=NULL;
	wrd_pos=1;
	for(i=0;i<a->nslot;++i)
	{
		v=((wtk_string_t**)a->slot)[i];
		tw=wtk_tts_wrd_new(info->heap,v);
		tw->snt=s;
		tw->index=s->wrds->nslot+1;
		tw->valid_pos=wrd_pos;
		xpron=(wtk_tts_wrd_xpron_t*)wtk_heap_malloc(info->heap,sizeof(wtk_tts_wrd_xpron_t));
		tw->pron=xpron;
		// printf("%d %.*s\n",v->len,v->len,v->data);
		if(wrd->cfg->use_upper){
			wtk_utf8_toupper(v->data,v->len);
		}else if(wrd->cfg->use_lower){
			wtk_utf8_tolower(v->data,v->len);
		}
		if(wrd->cfg->use_bin)
		{
			xpron->pron=wtk_tts_segwrd_get_wrd(wrd,v->data,v->len);
		}else
		{
			xpron->pron=wtk_kdict_get(wrd->cfg->dict,v->data,v->len);
		}
		if(!xpron->pron)
		{
			// wtk_debug("found bug null[%.*s]\n",v->len,v->data);
			// exit(0);
			xpron->pron  = wtk_kdict_get(wrd->cfg->dict,"unknown",strlen("unknown"));
		}
		if(xpron->pron && xpron->pron->npron==1 && xpron->pron->nsyl==1 &&( wtk_string_cmp_s(xpron->pron->syls[0].v,"pau")==0))
		{
			//wtk_debug("set sil\n");
			tw->sil=1;
		}else
		{
			++wrd_pos;
		}
		//wtk_tts_wrd_print(tw);
		if(xpron->pron)
		{
			tw->bound=i==(a->nslot-1)?WTK_TTS_BOUND_SEG:WTK_TTS_BOUND_RYTHYME;
			//wtk_debug("bound=[%.*s] %d\n",tw->v->len,tw->v->data,tw->bound);
			wtk_array_push2(s->wrds,&tw);
		}
		//wtk_tts_wrd_print(tw);
	}
	ret=0;
	return ret;
}

void wtk_tts_segwrd_print(wtk_tts_segwrd_t *wrd,wtk_tts_snt_t *s)
{
	wtk_tts_wrd_t **tws,*tw;
	int i;

	tws=(wtk_tts_wrd_t**)(s->wrds->slot);
	for(i=0;i<s->wrds->nslot;++i)
	{
		tw=tws[i];
		wtk_debug("bound=[%.*s] %d\n",tw->v->len,tw->v->data,tw->bound);
	}
}

int wtk_tts_segwrd_process_snt(wtk_tts_segwrd_t *wrd,wtk_tts_info_t *info,wtk_tts_snt_t *s)
{
	int ret;

	if(wrd->cfg->use_sp)
	{
		ret=wtk_tts_segwrd_process_seg_snt_sp(wrd,info,s);
	}else if(wrd->cfg->use_sp2)
	{
		ret=wtk_tts_segwrd_process_seg_snt_sp2(wrd,info,s);
	}else if(wrd->cfg->use_maxseg)
	{
		ret=wtk_tts_segwrd_process_max_snt(wrd,info,s);
	}else
	{
		ret=wtk_tts_segwrd_process_seg_snt(wrd,info,s);
	}
	//wtk_tts_segwrd_print(wrd,s);
	return ret;
}

int wtk_tts_segwrd_process(wtk_tts_segwrd_t *wrd,wtk_tts_info_t *info,wtk_tts_lab_t *lab)
{
	wtk_tts_snt_t **snt,*s;
	int i;

	snt=(wtk_tts_snt_t**)lab->snts->slot;
	for(i=0;i<lab->snts->nslot;++i)
	{
		//wtk_debug("v[%d]=[%.*s]\n",i,snt[i]->snt->len,snt[i]->snt->data);
		s=snt[i];
		wtk_tts_segwrd_process_snt(wrd,info,s);
		// wtk_tts_segwrd_print(wrd,s);
	}
	//exit(0);
	return 0;
}



