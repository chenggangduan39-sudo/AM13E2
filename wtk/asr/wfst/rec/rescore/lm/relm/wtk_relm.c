#include "wtk_relm.h"

wtk_relm_t* wtk_relm_new(wtk_relm_cfg_t *cfg,wtk_lm_dict_cfg_t *dict)
{
	wtk_relm_t *r;

	r=(wtk_relm_t*)wtk_malloc(sizeof(wtk_relm_t));
	r->cfg=cfg;
	r->lmlat=wtk_lmlat_new(&(cfg->lmlat),dict);
	return r;
}

void wtk_relm_delete(wtk_relm_t *r)
{
	wtk_lmlat_delete(r->lmlat);
	wtk_free(r);
}

void wtk_relm_reset(wtk_relm_t *r)
{
	wtk_lmlat_reset(r->lmlat);
}

int wtk_relm_bytes(wtk_relm_t *r)
{
	return wtk_lmlat_bytes(r->lmlat);
}

wtk_fst_net2_t* wtk_relm_process(wtk_relm_t *r,wtk_fst_net2_t *input)
{
	wtk_fst_net2_t *output;

	output=wtk_lmlat_process(r->lmlat,input);
	if(!output)
	{
		wtk_fst_net2_clean_hook(input);
	}
	return output;
}
