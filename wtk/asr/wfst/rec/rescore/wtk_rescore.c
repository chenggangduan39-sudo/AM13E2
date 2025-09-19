#include "wtk_rescore.h"

wtk_rescore_t* wtk_rescore_new(wtk_rescore_cfg_t *cfg,wtk_wfstenv_cfg_t *env)
{
	wtk_rescore_t *r;

	r=(wtk_rescore_t*)wtk_malloc(sizeof(wtk_rescore_t));
	r->cfg=cfg;
	r->lm=wtk_lm_rescore_new(&(cfg->lm),env);
	r->buf=wtk_strbuf_new(256,1);
	r->output_net=NULL;
	return r;
}

void wtk_rescore_delete(wtk_rescore_t *r)
{
	wtk_strbuf_delete(r->buf);
	wtk_lm_rescore_delete(r->lm);
	wtk_free(r);
}

void wtk_rescore_reset(wtk_rescore_t *r)
{
	wtk_lm_rescore_reset(r->lm);
	r->output_net=NULL;
}

int wtk_rescore_bytes(wtk_rescore_t *r)
{
	return wtk_lm_rescore_bytes(r->lm);
}

void wtk_rescore_clean_hist(wtk_rescore_t *r)
{
	return wtk_lm_rescore_clean_hist(r->lm);
}

int wtk_rescore_process(wtk_rescore_t *r,wtk_fst_net2_t *input_net)
{
	int ret;

	ret=wtk_lm_rescore_process(r->lm,input_net);
	if(ret==0)
	{
		r->output_net=r->lm->output_net;
	}
	ret=r->output_net?0:-1;
	return ret;
}

void wtk_rescore_get_result2(wtk_rescore_t *r,wtk_strbuf_t *buf,char *sep,int sep_bytes)
{
	int ret;

	if(r->output_net)
	{
		ret=wtk_fst_net2_shortest_path(r->output_net);
		//wtk_debug("ret=%d\n",ret);
		if(ret==0)
		{
			//wtk_fst_net2_print_best_path(r->output_net);
			wtk_fst_net2_get_short_one_best_path(r->output_net,buf,sep,sep_bytes);
			if(r->cfg->use_hist)
			{
				int idx[10];
				int cnt;

				cnt=wtk_fst_net2_get_history(r->output_net,idx,r->lm->cfg->main_lm.kv.nglm.max_order-1);
				if(cnt>0)
				{
					wtk_lm_rescore_update_history(r->lm,idx,cnt);
				}
			}
		}
	}
}

void wtk_rescore_get_result(wtk_rescore_t *r,wtk_string_t *v,char *sep,int sep_bytes)
{
	wtk_strbuf_t *buf;

	buf=r->buf;
	wtk_strbuf_reset(buf);
	//wtk_debug("get empty %p\n",r->output_net);
	if(r->output_net)
	{
		wtk_rescore_get_result2(r,buf,sep,sep_bytes);
	}
	wtk_string_set(v,buf->data,buf->pos);
}


wtk_fst_rec_trans_t* wtk_rescore_get_nbest(wtk_rescore_t *r,wtk_heap_t *heap,int nbest,char *sep,int bytes)
{
	wtk_fst_rec_trans_t *trans=NULL;
	int ret;

	if(r->output_net)
	{
		ret=wtk_fst_net2_nbest_path(r->output_net,nbest,r->cfg->nbest_max_search);
		if(ret==0)
		{
			trans=wtk_fst_net2_get_nbest(r->output_net,heap,sep,bytes);
		}
	}
	return trans;
}
