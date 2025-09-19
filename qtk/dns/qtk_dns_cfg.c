#include "qtk_dns_cfg.h" 

int qtk_dns_cfg_init(qtk_dns_cfg_t *cfg)
{
	cfg->timeout = 2000;
	cfg->use_dnsc = 1;
	return 0;
}

int qtk_dns_cfg_clean(qtk_dns_cfg_t *cfg)
{
	return 0;
}

int qtk_dns_cfg_update_local(qtk_dns_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;

	lc = main;
	wtk_local_cfg_update_cfg_i(lc,cfg,timeout,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_dnsc,v);
	return 0;
}

int qtk_dns_cfg_update(qtk_dns_cfg_t *cfg)
{
	return 0;
}
