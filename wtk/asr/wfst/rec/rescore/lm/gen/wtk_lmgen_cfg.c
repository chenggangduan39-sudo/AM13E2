#include "wtk_lmgen_cfg.h" 
#include "wtk/core/cfg/wtk_source.h"


int wtk_lmgen_cfg_init(wtk_lmgen_cfg_t *cfg)
{
	wtk_lm_dict_cfg_init(&(cfg->dict));
	wtk_lmgen_rec_cfg_init(&(cfg->backward));
	wtk_lmgen_rec_cfg_init(&(cfg->forward));
	cfg->stop_wrd_fn=NULL;
	cfg->stop_wrd=NULL;
	return 0;
}

int wtk_lmgen_cfg_clean(wtk_lmgen_cfg_t *cfg)
{
	if(cfg->stop_wrd)
	{
		wtk_str_hash_delete(cfg->stop_wrd);
	}
	wtk_lmgen_rec_cfg_clean(&(cfg->backward));
	wtk_lmgen_rec_cfg_clean(&(cfg->forward));
	wtk_lm_dict_cfg_clean(&(cfg->dict));
	return 0;
}

int wtk_lmgen_cfg_update_stop(wtk_lmgen_cfg_t *cfg,wtk_source_t *src)
{
	wtk_strbuf_t *buf;
	wtk_string_t *v;
	int ret;

	cfg->stop_wrd=wtk_str_hash_new(1037);
	buf=wtk_strbuf_new(256,1);
	while(1)
	{
		ret=wtk_source_read_normal_string(src,buf);
		if(ret!=0){ret=0;goto end;}
		v=wtk_heap_dup_string(cfg->stop_wrd->heap,buf->data,buf->pos);
		//wtk_debug("[%.*s]\n",v->len,v->data);
		wtk_str_hash_add(cfg->stop_wrd,v->data,v->len,v);
		//exit(0);
	}
	ret=0;
end:
	wtk_strbuf_delete(buf);
	return ret;
}

int wtk_lmgen_cfg_update_local(wtk_lmgen_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	int ret;

	lc=main;
	//wtk_local_cfg_print(lc);
	wtk_local_cfg_update_cfg_str(lc,cfg,stop_wrd_fn,v);
	if(cfg->stop_wrd_fn)
	{
		ret=wtk_source_load_file(cfg,(wtk_source_load_handler_t)wtk_lmgen_cfg_update_stop,cfg->stop_wrd_fn);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"forward");
	if(lc)
	{
		ret=wtk_lmgen_rec_cfg_update_local(&(cfg->forward),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"backward");
	if(lc)
	{
		ret=wtk_lmgen_rec_cfg_update_local(&(cfg->backward),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"dict");
	if(lc)
	{
		ret=wtk_lm_dict_cfg_update_local(&(cfg->dict),lc);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return ret;
}

int wtk_lmgen_cfg_update(wtk_lmgen_cfg_t *cfg)
{
	int ret;

	ret=wtk_lmgen_rec_cfg_update(&(cfg->forward));
	if(ret!=0){goto end;}
	ret=wtk_lmgen_rec_cfg_update(&(cfg->backward));
	if(ret!=0){goto end;}
	ret=wtk_lm_dict_cfg_update(&(cfg->dict));
	if(ret!=0){goto end;}
end:
	return ret;
}
