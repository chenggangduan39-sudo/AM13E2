#include "qtk_uploadc_cfg.h" 

int qtk_uploadc_cfg_init(qtk_uploadc_cfg_t *cfg)
{
	wtk_string_set(&cfg->host,0,0);
	wtk_string_set(&cfg->port,0,0);
	cfg->timeout = -1;
	cfg->send_string = 1;
	cfg->host_buf = NULL;
	cfg->port_buf = NULL;
	return 0;
}

int qtk_uploadc_cfg_clean(qtk_uploadc_cfg_t *cfg)
{
	if(cfg->host_buf){
		wtk_strbuf_delete(cfg->host_buf);
	}
	if(cfg->port_buf){
		wtk_strbuf_delete(cfg->port_buf);
	}
	return 0;
}

int qtk_uploadc_cfg_update_local(qtk_uploadc_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_string_v(lc,cfg,host,v);
	if(v->len > 0){
		cfg->host_buf = wtk_strbuf_new(64, 1);
		wtk_strbuf_push(cfg->host_buf, v->data, v->len);
	}
	wtk_local_cfg_update_cfg_string_v(lc,cfg,port,v);
	if(v->len > 0){
		cfg->port_buf = wtk_strbuf_new(64, 1);
		wtk_strbuf_push(cfg->port_buf, v->data, v->len);
	}
	wtk_local_cfg_update_cfg_i(lc,cfg,timeout,v);
	wtk_local_cfg_update_cfg_b(lc, cfg, send_string, v);
	return 0;
}

int qtk_uploadc_cfg_update(qtk_uploadc_cfg_t *cfg)
{
	return 0;
}
