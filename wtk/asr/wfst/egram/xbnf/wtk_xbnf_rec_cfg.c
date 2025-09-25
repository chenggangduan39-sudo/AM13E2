#include "wtk_xbnf_rec_cfg.h"

int wtk_xbnf_rec_cfg_init(wtk_xbnf_rec_cfg_t *cfg)
{
	wtk_xbnf_cfg_init(&(cfg->xbnf));
	cfg->xbnf_fn=NULL;
	cfg->xb=NULL;
	cfg->inst_cache=100;
	return 0;
}

int wtk_xbnf_rec_cfg_clean(wtk_xbnf_rec_cfg_t *cfg)
{
	if(cfg->xb)
	{
		wtk_xbnf_delete(cfg->xb);
	}
	wtk_xbnf_cfg_clean(&(cfg->xbnf));
	return 0;
}

int wtk_xbnf_rec_cfg_update_local(wtk_xbnf_rec_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret;

	lc=main;
	wtk_local_cfg_update_cfg_str(lc,cfg,xbnf_fn,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,inst_cache,v);
	lc=wtk_local_cfg_find_lc_s(main,"xbnf");
	if(lc)
	{
		ret=wtk_xbnf_cfg_update_local(&(cfg->xbnf),lc);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return ret;
}

int wtk_xbnf_rec_cfg_load_xbnf(wtk_xbnf_rec_cfg_t *cfg,wtk_source_t *src)
{
	wtk_strbuf_t *buf;
	int ret;

	buf=wtk_strbuf_new(1024,1);
	wtk_source_read_file2(src,buf);
	ret=wtk_xbnf_compile(cfg->xb,buf->data,buf->pos);
	wtk_strbuf_delete(buf);
	return ret;
}

int wtk_xbnf_rec_cfg_update2(wtk_xbnf_rec_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

        wtk_xbnf_cfg_update2(&(cfg->xbnf), sl);
        // wtk_debug("xbnf_fn=%s\n",cfg->xbnf_fn);
        if(cfg->xbnf_fn)
	{
		cfg->xb=wtk_xbnf_new(&(cfg->xbnf));
		ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)wtk_xbnf_rec_cfg_load_xbnf,cfg->xbnf_fn);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return ret;
}

int wtk_xbnf_rec_cfg_update(wtk_xbnf_rec_cfg_t *cfg)
{
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;
	return wtk_xbnf_rec_cfg_update2(cfg,&sl);
}

int wtk_xbnf_rec_cfg_update3(wtk_xbnf_rec_cfg_t *cfg)
{
	wtk_xbnf_cfg_update(&(cfg->xbnf));
	//wtk_debug("xbnf_fn=%s\n",cfg->xbnf_fn);
	if(cfg->xbnf_fn)
	{
		cfg->xb=wtk_xbnf_new(&(cfg->xbnf));
		wtk_xbnf_compile_file(cfg->xb,cfg->xbnf_fn);
	}
	return 0;
}
