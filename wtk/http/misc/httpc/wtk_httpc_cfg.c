#include "wtk_httpc_cfg.h"

wtk_httpc_cfg_t* wtk_httpc_cfg_new()
{
	wtk_httpc_cfg_t *cfg;

	cfg=(wtk_httpc_cfg_t*)wtk_malloc(sizeof(*cfg));
	wtk_httpc_cfg_init(cfg);
	return cfg;
}

int wtk_httpc_cfg_delete(wtk_httpc_cfg_t *cfg)
{
	wtk_free(cfg);
	return 0;
}

int wtk_httpc_cfg_init(wtk_httpc_cfg_t *cfg)
{
	wtk_dns_cfg_init(&(cfg->dns));
	wtk_cookie_cfg_init(&(cfg->cookie));
	wtk_string_set(&(cfg->ip),0,0);
	wtk_string_set_s(&(cfg->url),"/");
	wtk_string_set_s(&(cfg->port),"80");
	cfg->use_1_1=1;
	cfg->timeout=-1;
	return 0;
}

int wtk_httpc_cfg_clean(wtk_httpc_cfg_t *cfg)
{
	wtk_dns_cfg_clean(&(cfg->dns));
	wtk_cookie_cfg_clean(&(cfg->cookie));
	return 0;
}

int wtk_httpc_cfg_update(wtk_httpc_cfg_t *cfg)
{
	int ret;

	wtk_dns_cfg_update(&(cfg->dns));
	ret=wtk_cookie_cfg_update(&(cfg->cookie));
	return ret;
}

int wtk_httpc_cfg_update_local(wtk_httpc_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret;

	lc=main;
	wtk_local_cfg_update_cfg_b(lc,cfg,use_1_1,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,timeout,v);
	wtk_local_cfg_update_cfg_string_v(lc,cfg,ip,v);
	wtk_local_cfg_update_cfg_string_v(lc,cfg,url,v);
	wtk_local_cfg_update_cfg_string_v(lc,cfg,port,v);
	if(cfg->ip.len<=0 || cfg->url.len<=0)
	{
		ret=-1;
		goto end;
	}
	lc=wtk_local_cfg_find_lc_s(main,"cookie");
	if(lc)
	{
		ret=wtk_cookie_cfg_update_local(&(cfg->cookie),lc);
		if(ret!=0){goto end;}
	}
	lc = wtk_local_cfg_find_lc_s(main,"dns");
	if(lc) {
		ret = wtk_dns_cfg_update_local(&(cfg->dns),lc);
	}
	ret=0;
end:
	return ret;
}

int wtk_httpc_cfg_update_srv(wtk_httpc_cfg_t *cfg,wtk_string_t *host,wtk_string_t *port,wtk_string_t *url,wtk_string_t *cache_path,int timeout)
{
	cfg->timeout=timeout;
	if(host)
	{
		wtk_string_set(&(cfg->ip),host->data,host->len);
	}
	if(port)
	{
		wtk_string_set(&(cfg->port),port->data,port->len);
	}else
	{
		wtk_string_set_s(&(cfg->port),"80");
	}
	if(url)
	{
		wtk_string_set(&(cfg->url),url->data,url->len);
	}
	wtk_dns_cfg_update_srv(&(cfg->dns),cache_path);
	return 0;
}
