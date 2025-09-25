#include "qtk_kcmn_cfg.h"

int qtk_kcmn_cfg_init(qtk_kcmn_cfg_t *cfg)
{
	cfg->cmn_def=0;
	cfg->cmn_fn=0;
	cfg->cmn_window=400;
	cfg->speaker_frames=600;
	cfg->global_frames=50;
	cfg->modulus=20;
	cfg->ring_buffer_size=20;
	return 0;
}

int qtk_kcmn_cfg_clean(qtk_kcmn_cfg_t *cfg)
{
	if(cfg->cmn_def)
	{
		wtk_vector_delete(cfg->cmn_def);
	}
	return 0;
}

int qtk_kcmn_cfg_update_local(qtk_kcmn_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_str(lc,cfg,cmn_fn,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,cmn_window,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,speaker_frames,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,global_frames,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,modulus,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,ring_buffer_size,v);
	return 0;
}

int qtk_zmean_cfg_load_cmn(qtk_kcmn_cfg_t *cfg,wtk_source_t *s)
{
	wtk_strbuf_t *buf;
	int ret,n;

	buf=wtk_strbuf_new(32,1);
	ret=wtk_source_read_string(s,buf);
	if(ret!=0){goto end;}
	if(!wtk_str_equal_s(buf->data,buf->pos,"<MEAN>"))
	{
		ret=-1;goto end;
	}
	ret=wtk_source_read_int(s,&n,1,0);
	if(ret!=0){goto end;}
	cfg->cmn_def=wtk_vector_new(n);
	ret=wtk_source_read_vector(s,cfg->cmn_def,0);
end:
	wtk_strbuf_delete(buf);
	return ret;
}

/*
int wtk_zmean_cfg_update(wtk_zmean_cfg_t *cfg)
{
	return wtk_zmean_cfg_update2(cfg,0);
}
*/

int qtk_kcmn_cfg_update(qtk_kcmn_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	if(!cfg->cmn_fn)
	{
		ret=0;
		goto end;
	}
	ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)qtk_zmean_cfg_load_cmn,cfg->cmn_fn);
	/*
	if(sl)
	{
		ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)wtk_zmean_cfg_load_cmn,cfg->cmn_fn);
	}else
	{
		ret=wtk_source_load_file(cfg,(wtk_source_load_handler_t)wtk_zmean_cfg_load_cmn,cfg->cmn_fn);
	}*/
	if(ret!=0){goto end;}
end:
	return ret;
}
