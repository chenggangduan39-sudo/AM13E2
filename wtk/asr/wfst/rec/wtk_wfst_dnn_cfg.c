#include "wtk_wfst_dnn_cfg.h"

int wtk_wfst_dnn_cfg_init(wtk_wfst_dnn_cfg_t *cfg)
{
	cfg->conf_min=0;
	cfg->conf_max=0;
	cfg->conf_bias=0;
	cfg->scale=0.5;
	cfg->state_fn=0;
	cfg->hash=0;
	cfg->use_bin=0;
	return 0;
}

int wtk_wfst_dnn_cfg_clean(wtk_wfst_dnn_cfg_t *cfg)
{
	if(cfg->hash)
	{
		wtk_str_hash_delete(cfg->hash);
	}
	return 0;
}

int wtk_wfst_dnn_cfg_bytes(wtk_wfst_dnn_cfg_t *cfg)
{
	if(cfg->hash)
	{
		return wtk_str_hash_bytes(cfg->hash);
	}else
	{
		return 0;
	}
}

int wtk_wfst_dnn_cfg_update_local(wtk_wfst_dnn_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_str(lc,cfg,state_fn,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_bin,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,conf_min,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,conf_max,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,conf_bias,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,scale,v);
	return 0;
}

wtk_dnn_state_t* wtk_wfst_dnn_cfg_find(wtk_wfst_dnn_cfg_t *cfg,wtk_string_t *name)
{
	return (wtk_dnn_state_t*)wtk_str_hash_find(cfg->hash,name->data,name->len);
}

int wtk_rec_dnn_cfg_load_state(wtk_wfst_dnn_cfg_t *cfg,wtk_source_t *src)
{
	wtk_strbuf_t *buf;
	wtk_str_hash_t *hash;
	wtk_heap_t *heap;
	wtk_dnn_state_t *s;
	int ret=0;
	float f;
	int index=0;

	buf=wtk_strbuf_new(256,1);
	cfg->hash=hash=wtk_str_hash_new(1007);
	heap=cfg->hash->heap;
	//行数为state index;
	//state gconst
	//aa_s2_1 -14.469122 -6.28386
	while(1)
	{
		ret=wtk_source_read_string(src,buf);
		if(ret!=0){ret=0;goto end;}
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
		++index;
		ret=wtk_source_read_float(src,&(f),1,0);
		if(ret!=0){goto end;}
		s=(wtk_dnn_state_t*)wtk_heap_malloc(heap,sizeof(wtk_dnn_state_t));
		s->index=index;
		s->gconst=-f*cfg->scale;//1.0f;
		s->name=wtk_heap_dup_string(heap,buf->data,buf->pos);
		wtk_str_hash_add(hash,s->name->data,s->name->len,s);
		ret=wtk_source_read_float(src,&(f),1,0);
		if(ret!=0){goto end;}
	}
end:
	wtk_strbuf_delete(buf);
	return ret;
}

int wtk_rec_dnn_cfg_load_state_bin(wtk_wfst_dnn_cfg_t *cfg,wtk_source_t *src)
{
	wtk_strbuf_t *buf;
	wtk_str_hash_t *hash;
	wtk_heap_t *heap;
	wtk_dnn_state_t *s;
	int ret=0;
	float f;
	int t;
	int i,cnt;

	src->swap=0;
	buf=wtk_strbuf_new(256,1);
	ret=wtk_source_read_int(src,&cnt,1,1);
	if(ret!=0){goto end;}
	cfg->hash=hash=wtk_str_hash_new(cnt);
	heap=cfg->hash->heap;
	for(i=0;i<cnt;++i)
	{
		ret=wtk_source_read_int(src,&t,1,1);
		if(ret!=0){goto end;}
		ret=wtk_source_fill(src,buf->data,t);
		if(ret!=0){goto end;}
		buf->pos=t;
		ret=wtk_source_read_float(src,&(f),1,1);
		if(ret!=0){goto end;}
		s=(wtk_dnn_state_t*)wtk_heap_malloc(heap,sizeof(wtk_dnn_state_t));
		s->index=i+1;
		s->gconst=f;//-f*0.5f;
		s->name=wtk_heap_dup_string(heap,buf->data,buf->pos);
		wtk_str_hash_add(hash,s->name->data,s->name->len,s);
	}
end:
	wtk_strbuf_delete(buf);
	return ret;
}

int wtk_wfst_dnn_cfg_update(wtk_wfst_dnn_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	if(!cfg->state_fn){ret=0;goto end;}
	if(cfg->use_bin)
	{
		ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)wtk_rec_dnn_cfg_load_state_bin,cfg->state_fn);
	}else
	{
		ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)wtk_rec_dnn_cfg_load_state,cfg->state_fn);
	}
end:
	return ret;
}


int wtk_wfst_dnn_cfg_attach_hmmset(wtk_wfst_dnn_cfg_t *cfg,wtk_hmmset_t *set)
{
	wtk_hmm_t **hmms,*h;
	int i,j;

	hmms=(wtk_hmm_t**)(set->hmm_array->slot);
	for(i=0;i<set->hmm_array->nslot;++i)
	{
		h=hmms[i];
		for(j=2;j<h->num_state;++j)
		{
			if(!h->pState[j]->dnn)
			{
				h->pState[j]->dnn=wtk_wfst_dnn_cfg_find(cfg,h->pState[j]->name);
				//wtk_debug("[%.*s]=%p\n",h->pState[j]->name->len,h->pState[j]->name->data,h->pState[j]->dnn);
			}
		}
	}
	//exit(0);
	return 0;
}
