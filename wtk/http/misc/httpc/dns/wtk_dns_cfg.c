#include "wtk_dns_cfg.h" 

int wtk_dns_cfg_init(wtk_dns_cfg_t *cfg)
{
	wtk_string_set(&cfg->cache_path,0,0);
	cfg->dns_timeout = 3000;
	cfg->cache_day = 2.0L;
	cfg->use_cache = 0;
	cfg->use_dnsc = 1;
	return 0;
}

int wtk_dns_cfg_clean(wtk_dns_cfg_t *cfg)
{
	return 0;
}

int wtk_dns_cfg_update_local(wtk_dns_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_string_v(lc,cfg,cache_path,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,cache_day,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,dns_timeout,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_cache,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_dnsc,v);
	return 0;
}

int wtk_dns_cfg_update(wtk_dns_cfg_t *cfg)
{
	return 0;
}

void wtk_dns_cfg_update_srv(wtk_dns_cfg_t *cfg,wtk_string_t *cache_path)
{
	if(cache_path) {
		wtk_string_set(&(cfg->cache_path),cache_path->data,cache_path->len);
		cfg->use_cache = 1;
	}
}
