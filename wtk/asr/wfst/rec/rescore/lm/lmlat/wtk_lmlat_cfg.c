#include "wtk_lmlat_cfg.h"
#include <math.h>

int wtk_lmlat_cfg_init(wtk_lmlat_cfg_t *cfg)
{
	wtk_wfst_kv_cfg_init(&(cfg->kv));
	//wtk_nglm_cfg_init(&(cfg->nglm));
	cfg->lmscale=1.0;
	cfg->wrdpen=0.0;
	/*
	cfg->sym_out=NULL;
	cfg->sym_out_fn=NULL;
	*/
	cfg->beam=-1;
	cfg->ntok=-1;
	cfg->state_ntok=-1;
	cfg->state_beam=-1;

	cfg->dict=NULL;
	cfg->dict_fn=NULL;
	cfg->use_dict=0;
	cfg->unk_pen=0;

	wtk_fst_net_cfg_init(&(cfg->output_net));
	return 0;
}

int wtk_lmlat_cfg_clean(wtk_lmlat_cfg_t *cfg)
{
	if(cfg->dict)
	{
		wtk_lmexpand_dict_delete(cfg->dict);
	}
	/*
	if(cfg->sym_out)
	{
		wtk_fst_insym_delete(cfg->sym_out);
	}*/
	wtk_fst_net_cfg_clean(&(cfg->output_net));
	//wtk_nglm_cfg_clean(&(cfg->nglm));
	wtk_wfst_kv_cfg_clean(&(cfg->kv));
	return 0;
}

int wtk_lmlat_cfg_update_local(wtk_lmlat_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	int ret;

	lc=main;
	//wtk_local_cfg_print(main);
	/*
	wtk_local_cfg_update_cfg_str(lc,cfg,sym_out_fn,v);
	*/
	wtk_local_cfg_update_cfg_f(lc,cfg,lmscale,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,wrdpen,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,state_beam,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,state_ntok,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,beam,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,ntok,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,dict_fn,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_dict,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,unk_pen,v);
//	lc=wtk_local_cfg_find_lc_s(main,"nglm");
//	if(lc)
//	{
//		ret=wtk_nglm_cfg_update_local(&(cfg->nglm),lc);
//		if(ret!=0){goto end;}
//	}
	lc=wtk_local_cfg_find_lc_s(main,"kv");
	if(lc)
	{
		ret=wtk_wfst_kv_cfg_update_local(&(cfg->kv),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"output_net");
	if(lc)
	{
		ret=wtk_fst_net_cfg_update_local(&(cfg->output_net),lc);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return ret;
}

int wtk_lmlat_cfg_update(wtk_lmlat_cfg_t *cfg)
{
	wtk_source_loader_t sl;
	int ret;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;
	ret=wtk_lmlat_cfg_update2(cfg,&(sl));
	return ret;
}


int wtk_lmlat_cfg_update2(wtk_lmlat_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	//wtk_debug("%p\n",cfg->sym_out_fn);
	/*
	if(cfg->sym_out_fn)
	{
		cfg->sym_out=wtk_fst_insym_new(NULL,cfg->sym_out_fn,1);
	}*/
	if(cfg->use_dict && cfg->dict_fn)
	{
		cfg->dict=wtk_lmexpand_dict_new(cfg->dict_fn);
	}
	cfg->output_net.label=wtk_label_new(25007);
	ret=wtk_fst_net_cfg_update3(&(cfg->output_net),cfg->output_net.label,sl);
	if(ret!=0)
	{
		wtk_debug("load output_net failed\n");
		goto end;
	}
	ret=wtk_wfst_kv_cfg_update2(&(cfg->kv),sl);
	//ret=wtk_nglm_cfg_update(&(cfg->nglm));
	if(ret!=0)
	{
		wtk_debug("load nglm failed.\n");
		goto end;
	}
	cfg->lmscale=log(10)*cfg->lmscale;
	ret=0;
end:
	return ret;
}
