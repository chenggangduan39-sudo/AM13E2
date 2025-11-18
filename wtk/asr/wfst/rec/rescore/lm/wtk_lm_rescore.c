#include "wtk_lm_rescore.h"

wtk_lm_rescore_t*  wtk_lm_rescore_new(wtk_lm_rescore_cfg_t *cfg,
		wtk_wfstenv_cfg_t *env)//,wtk_fst_net_cfg_t *input_net_cfg)
{
	wtk_lm_rescore_t *r;
	int i;

	r=(wtk_lm_rescore_t*)wtk_malloc(sizeof(wtk_lm_rescore_t));
	r->cfg=cfg;
	r->main_lm=wtk_lmlat_new(&(cfg->main_lm),&(cfg->dict));
	if(cfg->n_custom>0)
	{
		r->custom_lm=(wtk_relm_t**)wtk_calloc(cfg->n_custom,sizeof(wtk_relm_t*));
		for(i=0;i<cfg->n_custom;++i)
		{
			r->custom_lm[i]=wtk_relm_new(&(cfg->custom_lm[i]),&(cfg->dict));
		}
	}else
	{
		r->custom_lm=NULL;
	}
	if(cfg->use_post)
	{
		r->post_lm=wtk_lmlat_new(&(cfg->post_lm),&(cfg->dict));
	}else
	{
		r->post_lm=NULL;
	}
	r->env=env;
	r->cur_custom_lm=NULL;
	r->output_net=NULL;
	r->use_custom=0;
	return r;
}

void wtk_lm_rescore_delete(wtk_lm_rescore_t *r)
{
	int i;

	if(r->post_lm)
	{
		wtk_lmlat_delete(r->post_lm);
	}
	if(r->custom_lm)
	{
		for(i=0;i<r->cfg->n_custom;++i)
		{
			wtk_relm_delete(r->custom_lm[i]);
		}
		wtk_free(r->custom_lm);
	}
	wtk_lmlat_delete(r->main_lm);
	wtk_free(r);
}

void wtk_lm_rescore_reset(wtk_lm_rescore_t *r)
{
	r->use_custom=0;
	r->output_net=NULL;
	if(r->post_lm)
	{
		wtk_lmlat_reset(r->post_lm);
	}
	if(r->cur_custom_lm)
	{
		wtk_relm_reset(r->cur_custom_lm);
		r->cur_custom_lm=NULL;
	}
	wtk_lmlat_reset(r->main_lm);
}

int wtk_lm_rescore_update_history(wtk_lm_rescore_t *r,int *idx,int nid)
{
	wtk_lmlat_update_history(r->main_lm,idx,nid);
	return 0;
}

void wtk_lm_rescore_clean_hist(wtk_lm_rescore_t *r)
{
	wtk_lmlat_update_clean_hist(r->main_lm);
}

int wtk_lm_rescore_bytes(wtk_lm_rescore_t *r)
{
	int bytes;
	int i;

	bytes=0;
	if(r->custom_lm)
	{
		for(i=0;i<r->cfg->n_custom;++i)
		{
			bytes+=wtk_relm_bytes(r->custom_lm[i]);
		}
	}
	bytes+=wtk_lmlat_bytes(r->main_lm);
	return bytes;
}

wtk_relm_t* wtk_lm_rescore_get_custom(wtk_lm_rescore_t *r,wtk_string_t *custom)
{
	wtk_relm_t *relm;
	int i;

	for(i=0;i<r->cfg->n_custom;++i)
	{
		relm=(r->custom_lm[i]);
		if(wtk_string_cmp(&(relm->cfg->name),custom->data,custom->len)==0)
		{
			return relm;
		}
	}
	return NULL;
}

int wtk_lm_rescore_process(wtk_lm_rescore_t *r,wtk_fst_net2_t *input)
{
	wtk_fst_net2_t *output;
	wtk_relm_t *relm;
	int ret;
	//wtk_debug("fuck type:%d %d %p\n",input->end->type,input->end->fuck,input->end);
	//wtk_fst_net2_write_lat(input,"x.lat");
	if(r->env && r->custom_lm && r->env->custom.len>0)
	{
		relm=wtk_lm_rescore_get_custom(r,&(r->env->custom));
		if(relm)
		{
			r->cur_custom_lm=relm;
			output=wtk_relm_process(relm,input);
			//wtk_debug("want process %p\n",output);
			if(output)
			{
				r->use_custom=1;
				r->output_net=output;
				ret=0;
				goto end;
			}
		}
	}
	//exit(0);
	output=wtk_lmlat_process(r->main_lm,input);
	if(!output)
	{
		ret=-1;
		goto end;
	}
	if(r->post_lm)
	{
		output=wtk_lmlat_process(r->post_lm,output);
		if(!output)
		{
			ret=-1;
			goto end;
		}
	}
	r->output_net=output;
	ret=0;
end:
	return ret;
}

void wtk_lm_rescore_print(wtk_lm_rescore_t *r)
{
	if(r->output_net && r->output_net->end)
	{
#ifndef DEBUG_N
		wtk_fst_net2_nbest_path(r->output_net,10,100000);
		wtk_fst_net3_print_nbest(r->output_net);
#else
		wtk_fst_net2_print_shortest_path(r->output_net);
#endif
	}else
	{
		wtk_debug("nil\n");
	}
}

