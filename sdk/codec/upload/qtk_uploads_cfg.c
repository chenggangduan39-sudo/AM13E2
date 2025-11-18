#include "qtk_uploads_cfg.h" 

int qtk_uploads_cfg_init(qtk_uploads_cfg_t *cfg)
{
	wtk_string_set_s(&cfg->dn,".");
	wtk_string_set_s(&cfg->port,"8001");
	return 0;
}

int qtk_uploads_cfg_clean(qtk_uploads_cfg_t *cfg)
{
	return 0;
}

int qtk_uploads_cfg_update_local(qtk_uploads_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_string_v(lc,cfg,dn,v);
	wtk_local_cfg_update_cfg_string_v(lc,cfg,port,v);
	return 0;
}

int qtk_uploads_cfg_update(qtk_uploads_cfg_t *cfg)
{
	return 0;
}
