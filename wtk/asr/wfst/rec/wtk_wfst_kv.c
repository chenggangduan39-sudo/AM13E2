#include "wtk_wfst_kv.h"
#include <math.h>

wtk_wfst_kv_t* wtk_wfst_kv_new(wtk_wfst_kv_cfg_t *cfg)
{
	wtk_wfst_kv_t *kv;

	kv=(wtk_wfst_kv_t*)wtk_malloc(sizeof(wtk_wfst_kv_t));
	kv->cfg=cfg;
	kv->buf=wtk_strbuf_new(128,1);
	kv->hash=wtk_str_hash_new(cfg->nslot);
	if(cfg->use_rnn)
	{
		kv->rnn=wtk_rnn_dec_new(&(cfg->rnn));
	}else
	{
		kv->rnn=NULL;
	}
	if(cfg->use_ngram)
	{
		kv->ngram=wtk_nglm_new(&(cfg->nglm),&(cfg->dict));
		if(kv->rnn)
		{
			kv->rnn->start.ngram=kv->ngram->s_node;
		}
	}else
	{
		kv->ngram=NULL;
	}
	return kv;
}

void wtk_wfst_kv_delete(wtk_wfst_kv_t *kv)
{
	if(kv->cfg->use_rnn)
	{
		if(kv->rnn)
		{
			wtk_rnn_dec_delete(kv->rnn);
		}
	}
	if(kv->ngram)
	{
		wtk_nglm_delete(kv->ngram);
	}
	wtk_strbuf_delete(kv->buf);
	wtk_str_hash_delete(kv->hash);
	wtk_free(kv);
}

void wtk_wfst_kv_reset(wtk_wfst_kv_t *kv)
{
//	if(kv->ngram && kv->ngram->cfg->use_dynamic_reset)
//	{
//		wtk_debug("==============> bytes=%d/%f\n",kv->ngram->bytes,kv->ngram->cfg->reset_max_bytes);
//		if(kv->ngram->cfg->reset_max_bytes<0 || kv->ngram->bytes<kv->ngram->cfg->reset_max_bytes)
//		{
//			return;
//		}
//	}
	wtk_str_hash_reset(kv->hash);
	if(kv->cfg->use_rnn)
	{
		if(kv->rnn)
		{
			wtk_rnn_dec_reset(kv->rnn);
		}
	}
	if(kv->ngram)
	{
		wtk_nglm_reset(kv->ngram);
	}
}

wtk_wfst_kv_env_t* wtk_wfst_kv_get_root2(wtk_wfst_kv_t *kv,int *idx,int nid)
{
	if(kv->cfg->use_rnn)
	{
		wtk_rnn_dec_env_t *env;

		env=&(kv->rnn->start);
		if(kv->ngram)
		{
			if(nid>0)
			{
				env->ngram=wtk_nglm_get_node_by_id(kv->ngram,idx,nid);
			}else
			{
				env->ngram=kv->ngram->s_node;
			}
		}
		return (wtk_wfst_kv_env_t*)env;
	}else
	{
		return (wtk_wfst_kv_env_t*)(kv->ngram->s_node);
	}
}


wtk_wfst_kv_env_t* wtk_wfst_kv_get_root(wtk_wfst_kv_t *kv)
{
	if(kv->cfg->use_rnn)
	{
		return (wtk_wfst_kv_env_t*)(&(kv->rnn->start));
	}else
	{
		return (wtk_wfst_kv_env_t*)(kv->ngram->s_node);
	}
}

wtk_wfst_kv_env_t* wtk_wfst_kv_get(wtk_wfst_kv_t *kv,wtk_wfst_kv_env_t *env,unsigned int id,double *plike)
{
	double like;

#ifdef USE_D
	node=wtk_nglm_get_child_prob(kv->ngram,node,(int)id,&like);
	like+=node->prob;
	if(plike)
	{
		*plike=like;
	}
	return node;
#else
	wtk_strbuf_t *buf=kv->buf;
	wtk_wfst_kv_item_t *item;
	wtk_lm_node_t *nodex;
	wtk_lm_node_t *node;
	double lm_like;
	wtk_rnn_dec_env_t *envx;
	wtk_rnn_dec_env_t *env1;
	wtk_string_t *v;
	wtk_hs_tree_wrd_t *wrd;


	wtk_strbuf_reset(buf);
	wtk_strbuf_push(buf,(char*)&env,sizeof(wtk_wfst_kv_env_t*));
	wtk_strbuf_push_c(buf,':');
	wtk_strbuf_push(buf,(char*)&id,sizeof(int));
	item=(wtk_wfst_kv_item_t*)wtk_str_hash_find(kv->hash,buf->data,buf->pos);
	if(item)
	{
//		nodex=wtk_nglm_get_child_prob(kv->ngram,node,(int)id,&like);
//		like+=nodex->prob;
//		if(item->node!=nodex  || fabs(like-item->like)>0.01)
//		{
//			wtk_debug("item=%p node=%p/%p/%p id=%d like=%f like=%f %f\n",item,item->node,node,nodex,id,item->like,like,
//					like-item->like);
//			exit(0);
//		}
		goto end;
	}
	if(kv->cfg->use_rnn)
	{
		env1=(wtk_rnn_dec_env_t*)env;
		//wtk_debug("%p=%d\n",kv->cfg->dict.sym,id);
		v=kv->cfg->dict.sym->ids[id]->str;
		wrd=wtk_hs_tree_get_word(kv->rnn->cfg->tree,v->data,v->len);
		envx=wtk_rnn_dec_pop_env(kv->rnn,wrd);
		if(wrd)
		{
			envx->wrd=wrd;
			like=wtk_rnn_dec_calc(kv->rnn,env1,envx);
			//like=wtk_rnn_rec_compute(kv->v.rec,env1->neu_hid,envx->neu_hid,env1->word,envx->word);
			//like=-like;
		}else
		{
			//wtk_debug("oov=%.*s\n",v->len,v->data);
			//exit(0);
			wtk_rnn_dec_env_cpy(env1,envx);
			like=kv->cfg->oov_pen;
		}
		if(kv->cfg->use_ngram)
		{
			node=(wtk_lm_node_t*)env1->ngram;
			nodex=wtk_nglm_get_child_prob(kv->ngram,node,(int)id,&lm_like);
			envx->ngram=nodex;
			lm_like+=nodex->prob;
			//wtk_debug("%f * %f+ %f * %f =%f\n",kv->cfg->ngram_scale,lm_like,kv->cfg->rnn_scale,like,kv->cfg->ngram_scale*lm_like+kv->cfg->rnn_scale*like);
			//wtk_debug("[%.*s]=%f/%f/%f\n",envx->buf->pos,envx->buf->data,lm_like,like,kv->cfg->ngram_scale*lm_like+kv->cfg->rnn_scale*like);
			//wtk_debug("=%f/%f/%f\n",lm_like,like,kv->cfg->ngram_scale*lm_like+kv->cfg->rnn_scale*like);
			like=kv->cfg->ngram_scale*lm_like+kv->cfg->rnn_scale*like;
			//wtk_debug("%f=%f\n",log10(pow(10,-lm_like)),-lm_like);
			//like=-log10(pow(10,-like)*kv->cfg->rnn_scale+kv->cfg->ngram_scale*pow(10,-lm_like));
			//wtk_debug("like=%f\n",like);
			//exit(0);
		}
		item=(wtk_wfst_kv_item_t*)wtk_heap_malloc(kv->hash->heap,sizeof(wtk_wfst_kv_item_t));
		item->node=(wtk_wfst_kv_env_t*)envx;
		item->like=like;
	}else
	{
		node=(wtk_lm_node_t*)env;
		//wtk_nglm_print_node(kv->ngram,node);
		nodex=wtk_nglm_get_child_prob(kv->ngram,node,(int)id,&like);
		//wtk_nglm_print_node(kv->ngram,nodex);
		like+=nodex->prob;
		if(nodex->ngram==kv->ngram->cfg->max_order)
		{
			nodex=wtk_nglm_get_bow_node(kv->ngram,nodex);
		}
		item=(wtk_wfst_kv_item_t*)wtk_heap_malloc(kv->hash->heap,sizeof(wtk_wfst_kv_item_t));
		item->node=(wtk_wfst_kv_env_t*)nodex;
		item->like=like;
		//wtk_debug("node=%d\n",nodex->ngram);
	}
	wtk_str_hash_add2(kv->hash,buf->data,buf->pos,item);

end:
	if(plike)
	{
		*plike=item->like;
	}
	return item->node;
#endif
}


