#include "wtk_kcmn_cfg.h"

int wtk_kcmn_cfg_init(wtk_kcmn_cfg_t *cfg)
{
	cfg->cmn_def=0;
	cfg->cmn_fn=0;
	cfg->cmn_window=200;
	cfg->speaker_frames=600;
	cfg->global_frames=100;
	cfg->modulus=20;
	cfg->ring_buffer_size=20;
	return 0;
}

int wtk_kcmn_cfg_clean(wtk_kcmn_cfg_t *cfg)
{
	if(cfg->cmn_def)
	{
		wtk_vector_delete(cfg->cmn_def);
	}
	return 0;
}

int wtk_kcmn_cfg_update_local(wtk_kcmn_cfg_t *cfg,wtk_local_cfg_t *lc)
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

int wtk_kcmn_zmean_cfg_load_cmn(wtk_kcmn_cfg_t *cfg,wtk_source_t *s)
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

int wtk_kcmn_zmean_cfg_load_cmn2(wtk_kcmn_cfg_t *cfg,wtk_source_t *s)
{
	wtk_strbuf_t *buf;
	int ret,n;

	s->swap=0;
	buf=wtk_strbuf_new(32,1);
	ret=wtk_source_read_string(s,buf);
	if(ret!=0){goto end;}
	if(!wtk_str_equal_s(buf->data,buf->pos,"<MEAN>"))
	{
		wtk_debug("mean string error %.*s\n",buf->pos,buf->data);
		ret=-1;goto end;
	}
    n=0;
	wtk_source_skip_sp(s,&n);
	ret=wtk_source_read_int(s,&n,1,1);
	if(ret!=0)
	{
		wtk_debug("read int data error\n");
		goto end;
	}

	cfg->cmn_def=wtk_vector_new(n+1);
	ret=wtk_source_read_vector(s,cfg->cmn_def,1);
	if(ret!=0)
	{
		wtk_debug("read cmvn vector error\n");
		goto end;
	}
//	int i;
//	for(i=1;i<=n+1;++i)
//	{
//		printf("%f\n",cfg->cmn_def[i]);
//	}
//	exit(0);

	ret=0;
end:
	wtk_strbuf_delete(buf);
	return ret;
}


int wtk_kcmn_cfg_update(wtk_kcmn_cfg_t *cfg)
{
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;
	return wtk_kcmn_cfg_update2(cfg,&sl);
}


int wtk_kcmn_cfg_update2(wtk_kcmn_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;
	wtk_source_loader_t sl2;

	if(cfg->cmn_fn && wtk_file_exist(cfg->cmn_fn)==0)
	{
		sl2.hook=0;
		sl2.vf=wtk_source_load_file_v;
		ret=wtk_source_loader_load(&sl2,cfg,(wtk_source_load_handler_t)wtk_kcmn_zmean_cfg_load_cmn2,cfg->cmn_fn);
		if(ret!=0){goto end;}
	}

	ret=0;
end:
	return ret;
}
