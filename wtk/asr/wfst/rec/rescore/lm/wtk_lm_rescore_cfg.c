#include "wtk_lm_rescore_cfg.h"
#include <math.h>

int wtk_lm_rescore_cfg_init(wtk_lm_rescore_cfg_t *cfg)
{
	wtk_lm_dict_cfg_init(&(cfg->dict));
	wtk_lmlat_cfg_init(&(cfg->main_lm));
	wtk_lmlat_cfg_init(&(cfg->post_lm));
	cfg->n_custom=0;
	cfg->custom_lm=0;
	cfg->use_post=0;
	return 0;
}

int wtk_lm_rescore_cfg_clean(wtk_lm_rescore_cfg_t *cfg)
{
	int i;

	if(cfg->custom_lm)
	{
		for(i=0;i<cfg->n_custom;++i)
		{
			wtk_relm_cfg_clean(&(cfg->custom_lm[i]));
		}
		wtk_free(cfg->custom_lm);
	}
	wtk_lm_dict_cfg_clean(&(cfg->dict));
	wtk_lmlat_cfg_clean(&(cfg->main_lm));
	return 0;
}

int wtk_lm_rescore_cfg_update_custom_lc(wtk_lm_rescore_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_queue_node_t *qn;
	wtk_cfg_item_t *item;
	wtk_relm_cfg_t *relm;
	int ret=-1;

	cfg->n_custom=lc->cfg->queue.length;
	cfg->custom_lm=(wtk_relm_cfg_t*)wtk_calloc(cfg->n_custom,sizeof(wtk_relm_cfg_t));
	for(cfg->n_custom=0,qn=lc->cfg->queue.pop;qn;qn=qn->next)
	{
		item=data_offset(qn,wtk_cfg_item_t,n);
		if(item->type!=WTK_CFG_LC){continue;}
		relm=&(cfg->custom_lm[cfg->n_custom]);
		++cfg->n_custom;
		wtk_relm_cfg_init(relm);
		ret=wtk_relm_cfg_update_local(relm,item->value.cfg);
		if(ret!=0){goto end;}
		//ret=wtk_relm_cfg_update(relm,cfg->dict);
		//if(ret!=0){goto end;}
		if(relm->name.len==0)
		{
			relm->name=*item->key;
		}
	}
	ret=0;
end:
	return ret;
}

int wtk_lm_rescore_cfg_update_custom(wtk_lm_rescore_cfg_t *cfg)
{
	wtk_relm_cfg_t *relm;
	int i;
	int ret=-1;

	for(i=0;i<cfg->n_custom;++i)
	{
		relm=&(cfg->custom_lm[i]);
		ret=wtk_relm_cfg_update(relm,&(cfg->dict));
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return ret;
}

int wtk_lm_rescore_cfg_update_local(wtk_lm_rescore_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret;

	lc=main;
	wtk_local_cfg_update_cfg_b(lc,cfg,use_post,v);
	lc=wtk_local_cfg_find_lc_s(main,"post_lm");
	if(lc)
	{
		ret=wtk_lmlat_cfg_update_local(&(cfg->post_lm),lc);
		if(ret!=0){goto end;}
	}
	//wtk_local_cfg_print(main);
	lc=wtk_local_cfg_find_lc_s(main,"dict");
	if(lc)
	{
		ret=wtk_lm_dict_cfg_update_local(&(cfg->dict),lc);
		if(ret!=0){goto end;}
	}else
	{
		lc=wtk_local_cfg_find_lc_s(main,"nglm");
		if(lc)
		{
			ret=wtk_lm_dict_cfg_update_local(&(cfg->dict),lc);
			if(ret!=0){goto end;}
		}else
		{
			ret=wtk_lm_dict_cfg_update_local(&(cfg->dict),main);
			if(ret!=0){goto end;}
		}
	}
	lc=wtk_local_cfg_find_lc_s(main,"main_lm");
	if(lc)
	{
		ret=wtk_lmlat_cfg_update_local(&(cfg->main_lm),lc);
		if(ret!=0){goto end;}
	}else
	{
		ret=wtk_lmlat_cfg_update_local(&(cfg->main_lm),main);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"custom_lm");
	if(lc)
	{
		ret=wtk_lm_rescore_cfg_update_custom_lc(cfg,lc);
		if(ret!=0){goto end;}
	}
	ret=0;
	//wtk_debug("%d/%f\n",cfg->state_ntok,cfg->state_beam);
end:
	return ret;
}

int wtk_lm_rescore_cfg_update2(wtk_lm_rescore_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	if(cfg->use_post)
	{
		ret=wtk_lmlat_cfg_update(&(cfg->post_lm));
		if(ret!=0){goto end;}
	}
	ret=wtk_lm_dict_cfg_update2(&(cfg->dict),sl);
	if(ret!=0)
	{
		goto end;
	}
	ret=wtk_lm_rescore_cfg_update_custom(cfg);
	if(ret!=0)
	{
		goto end;
	}
	ret=wtk_lmlat_cfg_update2(&(cfg->main_lm),sl);
	if(ret!=0)
	{
		wtk_debug("load nglm failed.\n");
		goto end;
	}
	ret=0;
end:
	return ret;
}


int wtk_lm_rescore_cfg_update(wtk_lm_rescore_cfg_t *cfg)
{
	wtk_source_loader_t sl;
	int ret;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;
	ret=wtk_lm_rescore_cfg_update2(cfg,&(sl));
	return ret;
}

void wtk_lm_rescore_cfg_set_sym_out(wtk_lm_rescore_cfg_t *cfg,wtk_fst_sym_t *sym_out)
{
	int i;

	cfg->main_lm.output_net.sym_out=sym_out;
	if(cfg->use_post)
	{
		cfg->post_lm.output_net.sym_out=sym_out;
	}
	for(i=0;i<cfg->n_custom;++i)
	{
		wtk_relm_cfg_set_sym_out(&(cfg->custom_lm[i]),sym_out);
	}
}
