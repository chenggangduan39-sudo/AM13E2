#include "qtk_httpc_cfg.h" 

int qtk_httpc_cfg_init(qtk_httpc_cfg_t *cfg)
{
	qtk_dns_cfg_init(&cfg->dns);
	wtk_string_set(&cfg->host,0,0);
	wtk_string_set(&cfg->port,0,0);
	wtk_string_set(&cfg->url,0,0);
	cfg->hearbeat_time = 5000;
	cfg->timeout = -1;
	cfg->http_1_1 = 1;
	cfg->use_stage = 0;
	cfg->log_http = 0;
	return 0;
}

int qtk_httpc_cfg_clean(qtk_httpc_cfg_t *cfg)
{
	qtk_dns_cfg_clean(&cfg->dns);
	return 0;
}

int qtk_httpc_cfg_update_local(qtk_httpc_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;

	lc = main;
	wtk_local_cfg_update_cfg_string_v(lc,cfg,host,v);
	wtk_local_cfg_update_cfg_string_v(lc,cfg,port,v);
	wtk_local_cfg_update_cfg_string_v(lc,cfg,url,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,hearbeat_time,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,timeout,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,http_1_1,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_stage,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,log_http,v);

	lc = wtk_local_cfg_find_lc_s(lc,"dns");
	if(lc) {
		qtk_dns_cfg_update_local(&cfg->dns,lc);
	}
	return 0;
}

int qtk_httpc_cfg_update(qtk_httpc_cfg_t *cfg)
{
	qtk_dns_cfg_update(&cfg->dns);
	return 0;
}

void qtk_httpc_cfg_update_opt(qtk_httpc_cfg_t *cfg,
							  wtk_string_t *host,
							  wtk_string_t *port,
							  wtk_string_t *url,
							  wtk_string_t *dns_fn,
							  int timeout)
{
	if(host) {
		wtk_string_set(&cfg->host,host->data,host->len);
	}
	if(port) {
		wtk_string_set(&cfg->port,port->data,port->len);
	}
	if(url) {
		wtk_string_set(&cfg->url,url->data,url->len);
	}
	cfg->timeout = timeout;
}
