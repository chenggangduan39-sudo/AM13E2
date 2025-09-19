#include "wtk_lm_dict_cfg.h"

int wtk_lm_dict_cfg_init(wtk_lm_dict_cfg_t *cfg)
{
	cfg->sym=NULL;
	cfg->sym_out_fn=NULL;
	cfg->snt_end_id=-1;
	cfg->snt_start_id=-1;
	cfg->use_sym_bin=0;
	return 0;
}

int wtk_lm_dict_cfg_clean(wtk_lm_dict_cfg_t *cfg)
{
	if(cfg->sym)
	{
		wtk_fst_insym_delete(cfg->sym);
	}
	return 0;
}

int wtk_lm_dict_cfg_update_local(wtk_lm_dict_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_str(lc,cfg,sym_out_fn,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_sym_bin,v);
	return 0;
}

void wtk_lm_dict_cfg_update_id(wtk_lm_dict_cfg_t *cfg)
{
	wtk_string_t v;

	wtk_string_set_s(&(v),"<s>");
	cfg->snt_start_id=wtk_fst_insym_get_index(cfg->sym,&v);
	//wtk_debug("snt=%d\n",cfg->snt_start_id);
	wtk_string_set_s(&(v),"</s>");
	cfg->snt_end_id=wtk_fst_insym_get_index(cfg->sym,&v);
}

int wtk_lm_dict_cfg_update(wtk_lm_dict_cfg_t *cfg)
{
	wtk_source_loader_t file_sl;

	file_sl.hook=0;
	file_sl.vf=wtk_source_load_file_v;
	wtk_lm_dict_cfg_update2(cfg,&(file_sl));
	return 0;
}

int wtk_lm_dict_cfg_update2(wtk_lm_dict_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	ret=-1;
	if(cfg->sym_out_fn)
	{
		//cfg->sym=wtk_fst_insym_new(NULL,cfg->sym_out_fn,1);
		//wtk_debug("%d\n",cfg->use_sym_bin);
		cfg->sym=wtk_fst_insym_new3(NULL,cfg->sym_out_fn,1,sl,cfg->use_sym_bin);
		//wtk_debug("upload sym=%p\n",cfg->sym);
		if(!cfg->sym)
		{
			goto end;
		}
		wtk_lm_dict_cfg_update_id(cfg);
//		{
//			wtk_string_t *v;
//
//			v=cfg->sym->ids[4848]->str;
//			wtk_debug("%p:%.*s\n",cfg->sym,v->len,v->data);
//		}
	}
	ret=0;
end:
	return ret;
}
