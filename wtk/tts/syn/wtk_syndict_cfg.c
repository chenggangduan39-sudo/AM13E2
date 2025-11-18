#include "wtk_syndict_cfg.h" 

int wtk_syndict_cfg_init(wtk_syndict_cfg_t *cfg)
{
	cfg->dict_fn=NULL;
	cfg->dict_hash_hint=15007;
	return 0;
}

int wtk_syndict_cfg_clean(wtk_syndict_cfg_t *cfg)
{
	return 0;
}

int wtk_syndict_cfg_update_local(wtk_syndict_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_str(lc,cfg,dict_fn,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,dict_hash_hint,v);
	return 0;
}

int wtk_syndict_cfg_update(wtk_syndict_cfg_t *cfg)
{
	return 0;
}
