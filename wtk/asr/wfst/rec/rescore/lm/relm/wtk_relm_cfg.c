#include "wtk_relm_cfg.h"


int wtk_relm_cfg_init(wtk_relm_cfg_t *cfg)
{
	wtk_string_set(&(cfg->name),0,0);
	wtk_lmlat_cfg_init(&(cfg->lmlat));
	return 0;
}

int wtk_relm_cfg_clean(wtk_relm_cfg_t *cfg)
{
	wtk_lmlat_cfg_clean(&(cfg->lmlat));
	return 0;
}

int wtk_relm_cfg_update_local(wtk_relm_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret;

	//wtk_local_cfg_print(main);
	lc=main;
	wtk_local_cfg_update_cfg_string_v(lc,cfg,name,v);
	lc=wtk_local_cfg_find_lc_s(main,"lmlat");
	if(lc)
	{
		ret=wtk_lmlat_cfg_update_local(&(cfg->lmlat),lc);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return ret;
}

int wtk_relm_cfg_update(wtk_relm_cfg_t *cfg,wtk_lm_dict_cfg_t *dict)
{
	int ret;

	ret=wtk_lmlat_cfg_update(&(cfg->lmlat));
	if(ret!=0){goto end;}
	ret=0;
end:
	return ret;
}

void wtk_relm_cfg_set_sym_out(wtk_relm_cfg_t *cfg,wtk_fst_sym_t *sym_out)
{
	cfg->lmlat.output_net.sym_out=sym_out;
}

