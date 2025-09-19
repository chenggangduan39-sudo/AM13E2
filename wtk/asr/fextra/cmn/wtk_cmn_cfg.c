#include "wtk_cmn_cfg.h"

int wtk_cmn_cfg_init(wtk_cmn_cfg_t *cfg)
{
	cfg->cmn_def=0;
	cfg->cmn_fn=0;
	cfg->start_min_frame=12;
	cfg->post_update_frame=18;
	cfg->post_update_frame2=-1;
	cfg->smooth=1;
	cfg->left_seek_frame=-1;
	cfg->min_flush_frame=-1;
	cfg->save_cmn=0;
	cfg->max_cmn_frame=-1;
	cfg->post_left_frame=30;
	cfg->use_hist=1;
	cfg->alpha=1;
	cfg->use_whole=0;
	return 0;
}

int wtk_cmn_cfg_clean(wtk_cmn_cfg_t *cfg)
{
	if(cfg->cmn_def)
	{
		wtk_vector_delete(cfg->cmn_def);
	}
	return 0;
}

int wtk_cmn_cfg_update_local(wtk_cmn_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_f(lc,cfg,alpha,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,cmn_fn,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,post_left_frame,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_hist,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,start_min_frame,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,post_update_frame,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,post_update_frame2,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,left_seek_frame,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,smooth,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_whole,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,min_flush_frame,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,max_cmn_frame,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,save_cmn,v);
	return 0;
}

int wtk_zmean_cfg_load_cmn(wtk_cmn_cfg_t *cfg,wtk_source_t *s)
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

int wtk_cmn_cfg_update(wtk_cmn_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	if(!cfg->cmn_fn)
	{
		ret=0;
		goto end;
	}
	ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)wtk_zmean_cfg_load_cmn,cfg->cmn_fn);
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
