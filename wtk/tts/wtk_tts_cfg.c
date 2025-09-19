#include "wtk_tts_cfg.h"

int wtk_tts_cfg_init(wtk_tts_cfg_t *cfg)
{
	wtk_tts_parser_cfg_init(&(cfg->parser));
	wtk_syn_cfg_init(&(cfg->syn));
	wtk_pitch_cfg_init(&(cfg->pitch));
	cfg->pool=NULL;
	cfg->bin_cfg=NULL;
	cfg->main_cfg=NULL;
	cfg->use_thread=1;
	cfg->buf_size=3200;
	cfg->volume_scale=1.0;
	cfg->min_sil_time=-1;
	cfg->snt_sil_time=0;
	cfg->max_sil_value=10;
	return 0;
}

int wtk_tts_cfg_clean(wtk_tts_cfg_t *cfg)
{
	wtk_pitch_cfg_clean(&(cfg->pitch));
	wtk_tts_parser_cfg_clean(&(cfg->parser));
	wtk_syn_cfg_clean(&(cfg->syn));
	if(cfg->pool)
	{
		wtk_strpool_delete(cfg->pool);
	}
	return 0;
}

int wtk_tts_cfg_update_local(wtk_tts_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret;

	lc=main;
	wtk_local_cfg_update_cfg_f(lc,cfg,volume_scale,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,max_sil_value,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,buf_size,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,min_sil_time,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,snt_sil_time,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_thread,v);
	lc=wtk_local_cfg_find_lc_s(main,"pitch");
	if(lc)
	{
		ret=wtk_pitch_cfg_update_local(&(cfg->pitch),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"parser");
	if(lc)
	{
		ret=wtk_tts_parser_cfg_update_local(&(cfg->parser),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"syn");
	if(lc)
	{
		ret=wtk_syn_cfg_update_local(&(cfg->syn),lc);
		if(ret!=0){goto end;}
	}
	if(cfg->min_sil_time>0)
	{
		cfg->min_sil_time=cfg->min_sil_time*16;
	}
	ret=0;
end:
	return ret;
}

int wtk_tts_cfg_update3(wtk_tts_cfg_t *cfg)
{
	int ret;

	if(cfg->parser.segwrd.use_bin)
	{
		cfg->pool=wtk_strpool_new(1507);
	}else
	{
		cfg->pool=wtk_strpool_new(15007);
	}
	ret=wtk_tts_parser_cfg_update2(&(cfg->parser),cfg->pool);
	if(ret!=0){goto end;}
	ret=wtk_syn_cfg_update2(&(cfg->syn),cfg->pool);
	if(ret!=0){goto end;}
end:
	return ret;
}

int wtk_tts_cfg_update(wtk_tts_cfg_t *cfg)
{
	wtk_source_loader_t sl;

	wtk_source_loader_init_file(&(sl));
	return wtk_tts_cfg_update2(cfg,&(sl));
}

//#include "wtk/os/wtk_proc.h"

int wtk_tts_cfg_bytes(wtk_tts_cfg_t *cfg)
{
	int bytes;

	bytes=wtk_strpool_bytes(cfg->pool);
	wtk_debug("bytes=%f M\n",bytes*1.0/(1024*1024));
	bytes+=wtk_tts_parser_cfg_bytes(&(cfg->parser));
	wtk_debug("bytes=%f M\n",bytes*1.0/(1024*1024));
	bytes+=wtk_syn_cfg_bytes(&(cfg->syn));
	wtk_debug("bytes=%f M\n",bytes*1.0/(1024*1024));
//	{
//		double m;
//
//		m=wtk_proc_mem();
//		wtk_debug("m=%f\n",m);
//	}
	//getchar();
	//exit(0);
	return bytes;
}

int wtk_tts_cfg_update2(wtk_tts_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	if(cfg->parser.segwrd.use_bin)
	{
		cfg->pool=wtk_strpool_new(1507);
	}else
	{
		cfg->pool=wtk_strpool_new(15007);
	}
	//wtk_debug("bytes=%f M\n",wtk_strpool_bytes(cfg->pool)*1.0/(1024*1024));

	ret=wtk_tts_parser_cfg_update3(&(cfg->parser),sl,cfg->pool);

	//wtk_debug("bytes=%f M\n",wtk_strpool_bytes(cfg->pool)*1.0/(1024*1024));

	if(ret!=0){goto end;}
	ret=wtk_syn_cfg_update3(&(cfg->syn),sl,cfg->pool);

	//wtk_debug("bytes=%f M\n",wtk_strpool_bytes(cfg->pool)*1.0/(1024*1024));
	if(ret!=0){goto end;}
	ret=wtk_pitch_cfg_update(&(cfg->pitch));
	//wtk_debug("bytes=%f M\n",wtk_strpool_bytes(cfg->pool)*1.0/(1024*1024));
	if(ret!=0){goto end;}
end:
	return ret;
}

wtk_tts_cfg_t* wtk_tts_cfg_new(char *cfg_fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_tts_cfg_t *tts=NULL;

	main_cfg=wtk_main_cfg_new_type(wtk_tts_cfg,cfg_fn);
	if(!main_cfg){goto end;}
	tts=(wtk_tts_cfg_t*)(main_cfg->cfg);
	tts->main_cfg=main_cfg;
end:
	return  tts;
}

void wtk_tts_cfg_delete(wtk_tts_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}


wtk_tts_cfg_t* wtk_tts_cfg_new_bin2(char *cfg_fn,int seek_pos)
{
	wtk_mbin_cfg_t *cfg;
	wtk_tts_cfg_t *tc;

	cfg=wtk_mbin_cfg_new_type2(seek_pos,wtk_tts_cfg,cfg_fn,"./cfg");
	tc=(wtk_tts_cfg_t*)(cfg->cfg);
	tc->bin_cfg=cfg;
	return tc;
}

wtk_tts_cfg_t* wtk_tts_cfg_new_bin(char *cfg_fn)
{
	wtk_mbin_cfg_t *cfg;
	wtk_tts_cfg_t *tc=0;

	cfg=wtk_mbin_cfg_new_type(wtk_tts_cfg,cfg_fn,"./cfg");
	if (cfg){
		tc=(wtk_tts_cfg_t*)(cfg->cfg);
		tc->bin_cfg=cfg;
	}

	return tc;
}

void wtk_tts_cfg_delete_bin(wtk_tts_cfg_t *cfg)
{
	if(cfg->bin_cfg)
	{
		wtk_mbin_cfg_delete(cfg->bin_cfg);
	}
}
