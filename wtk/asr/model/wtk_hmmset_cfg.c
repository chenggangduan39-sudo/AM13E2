#include "wtk_hmmset_cfg.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk_hmmset.h"

int wtk_hmmset_cfg_init(wtk_hmmset_cfg_t *cfg)
{
	cfg->hmmlist_fn=0;
	cfg->hmm_fn=0;
	cfg->hmmset=0;
	cfg->hmmlist_hint=30641;
	cfg->use_le=0;
	cfg->use_list_bin=0;
	cfg->use_fix=0;
	cfg->max_mean=0;
	cfg->max_var=0;
	cfg->max_mean_i=127;
	cfg->max_var_i=65535;
	return 0;
}

int wtk_hmmset_cfg_clean(wtk_hmmset_cfg_t *cfg)
{
	if(cfg->hmmset)
	{
		wtk_hmmset_delete(cfg->hmmset);
	}
	return 0;
}

int wtk_hmmset_cfg_bytes(wtk_hmmset_cfg_t *cfg)
{
	return wtk_hmmset_bytes(cfg->hmmset);
}

int wtk_hmmset_cfg_update_local(wtk_hmmset_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_str(lc,cfg,hmmlist_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,hmm_fn,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,hmmlist_hint,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_le,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_list_bin,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_fix,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,max_mean,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,max_var,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,max_var_i,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,max_mean_i,v);

	//wtk_local_cfg_print(lc);
	return 0;
}

int wtk_hmmset_cfg_update(wtk_hmmset_cfg_t *cfg,wtk_label_t *label)
{
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;
	return wtk_hmmset_cfg_update2(cfg,label,&sl);
}

int wtk_hmmset_cfg_update2(wtk_hmmset_cfg_t *cfg,wtk_label_t *label,wtk_source_loader_t *sl)
{
	int ret=-1;

	if(cfg->use_fix)
	{
		cfg->mean_scale=cfg->max_mean_i/cfg->max_mean;
		cfg->var_scale=cfg->max_var_i/cfg->max_var;
	}
	if(!cfg->hmmlist_fn || !cfg->hmm_fn)
	{
		wtk_debug("lost name\n");
		goto end;
	}
	cfg->hmmset=wtk_hmmset_new2(label,cfg->hmmlist_hint);
	cfg->hmmset->cfg=cfg;
	cfg->hmmset->use_le=cfg->use_le;
	//wtk_debug("load list: %d\n",cfg->use_list_bin);
	if(cfg->use_list_bin)
	{
		//wtk_debug("%s\n",cfg->hmmlist_fn);
		ret=wtk_source_loader_load(sl,cfg->hmmset,(wtk_source_load_handler_t)wtk_hmmset_load_list2,cfg->hmmlist_fn);
	}else
	{
		ret=wtk_source_loader_load(sl,cfg->hmmset,(wtk_source_load_handler_t)wtk_hmmset_load_list,cfg->hmmlist_fn);
	}
	//ret=wtk_source_load_file(cfg->hmmset,(wtk_source_load_handler_t)wtk_hmmset_load_list,cfg->hmmlist_fn);
	if(ret!=0)
	{
		wtk_debug("load list failed\n");
		goto end;
	}
	//wtk_debug("nhmm=%d\n",cfg->hmmset->hmm_array->nslot);
	ret=wtk_source_loader_load(sl,cfg->hmmset,(wtk_source_load_handler_t)wtk_hmmset_load_model,cfg->hmm_fn);
	//ret=wtk_source_load_file(cfg->hmmset,(wtk_source_load_handler_t)wtk_hmmset_load_model,cfg->hmm_fn);
	if(ret!=0)
	{
		wtk_debug("load mmf failed\n");
		goto end;
	}
end:
	return ret;
}
