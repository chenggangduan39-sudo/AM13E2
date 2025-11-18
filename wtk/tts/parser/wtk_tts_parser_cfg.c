#include "wtk_tts_parser_cfg.h" 

int wtk_tts_parser_cfg_init(wtk_tts_parser_cfg_t *cfg)
{
	wtk_tts_norm_cfg_init(&(cfg->norm));
	wtk_tts_segsnt_cfg_init(&(cfg->segsnt));
	wtk_tts_segwrd_cfg_init(&(cfg->segwrd));
	wtk_tts_pos_cfg_init(&(cfg->pos));
	wtk_polyphn_cfg_init(&(cfg->polyphn));
	wtk_tts_phn_cfg_init(&(cfg->phn));
	cfg->pool=NULL;
	cfg->use_hi=1;
	cfg->use_pos=1;
	return 0;
}

int wtk_tts_parser_cfg_clean(wtk_tts_parser_cfg_t *cfg)
{
	wtk_tts_norm_cfg_clean(&(cfg->norm));
	wtk_tts_segsnt_cfg_clean(&(cfg->segsnt));
	wtk_tts_segwrd_cfg_clean(&(cfg->segwrd));
	wtk_tts_pos_cfg_clean(&(cfg->pos));
	wtk_polyphn_cfg_clean(&(cfg->polyphn));
	wtk_tts_phn_cfg_clean(&(cfg->phn));
	if(cfg->pool)
	{
		wtk_strpool_delete(cfg->pool);
	}
	return 0;
}

int wtk_tts_parser_cfg_bytes(wtk_tts_parser_cfg_t *cfg)
{
	int bytes;

	bytes=sizeof(wtk_tts_parser_cfg_t);
	bytes+=wtk_tts_segwrd_cfg_bytes(&(cfg->segwrd));
	//wtk_debug("bytes=%f m\n",bytes*1.0/(1024*1024));
	bytes+=wtk_tts_pos_cfg_bytes(&(cfg->pos));
	//wtk_debug("bytes=%f m\n",bytes*1.0/(1024*1024));
	bytes+=wtk_tts_phn_cfg_bytes(&(cfg->phn));
	//wtk_debug("bytes=%f m\n",bytes*1.0/(1024*1024));
	return bytes;
}

int wtk_tts_parser_cfg_update_local(wtk_tts_parser_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret;

	lc=main;
	wtk_local_cfg_update_cfg_b(lc,cfg,use_hi,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_pos,v);
	lc=wtk_local_cfg_find_lc_s(main,"norm");
	if(lc)
	{
		//wtk_local_cfg_print(lc);
		ret=wtk_tts_norm_cfg_update_local(&(cfg->norm),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"segsnt");
	if(lc)
	{
		ret=wtk_tts_segsnt_cfg_update_local(&(cfg->segsnt),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"segwrd");
	if(lc)
	{
		ret=wtk_tts_segwrd_cfg_update_local(&(cfg->segwrd),lc);
		if(ret!=0){goto end;}
	}
	if (cfg->use_pos)
	{
		lc=wtk_local_cfg_find_lc_s(main,"pos");
		if(lc)
		{
			ret=wtk_tts_pos_cfg_update_local(&(cfg->pos),lc);
			if(ret!=0){goto end;}
		}
	}

	lc=wtk_local_cfg_find_lc_s(main,"polyphn");
	if(lc)
	{
		ret=wtk_polyphn_cfg_update_local(&(cfg->polyphn),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"phn");
	if(lc)
	{
		ret=wtk_tts_phn_cfg_update_local(&(cfg->phn),lc);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return ret;
}

int wtk_tts_parser_cfg_update(wtk_tts_parser_cfg_t *cfg)
{
	return wtk_tts_parser_cfg_update2(cfg,NULL);
}

int wtk_tts_parser_cfg_update2(wtk_tts_parser_cfg_t *cfg,wtk_strpool_t *pool)
{
	int ret;

	if(!pool)
	{
		pool=wtk_strpool_new(25007);
		cfg->pool=pool;
	}
	ret=wtk_tts_norm_cfg_update(&(cfg->norm));
	if(ret!=0){goto end;}
	ret=wtk_tts_segsnt_cfg_update(&(cfg->segsnt));
	if(ret!=0){goto end;}
	ret=wtk_tts_segwrd_cfg_update3(&(cfg->segwrd),pool);
	if(ret!=0){goto end;}
	if (cfg->use_pos)
	{
		ret=wtk_tts_pos_cfg_update(&(cfg->pos),pool);
		if(ret!=0){goto end;}
	}
	ret=wtk_polyphn_cfg_update(&(cfg->polyphn));
	if(ret!=0){goto end;}
	ret=wtk_tts_phn_cfg_update(&(cfg->phn),pool);
	if(ret!=0){goto end;}
	ret=0;
end:
	return ret;
}

int wtk_tts_parser_cfg_update3(wtk_tts_parser_cfg_t *cfg,wtk_source_loader_t *sl,wtk_strpool_t *pool)
{
	int ret;

	if(!pool)
	{
		pool=wtk_strpool_new(25007);
		cfg->pool=pool;
	}
	ret=wtk_tts_segsnt_cfg_update2(&(cfg->segsnt),sl);
	//wtk_debug("bytes=%f M\n",wtk_strpool_bytes(pool)*1.0/(1024*1024));
	if(ret!=0){goto end;}
	ret=wtk_tts_segwrd_cfg_update2(&(cfg->segwrd),sl,pool);
	//wtk_debug("bytes=%f M\n",wtk_strpool_bytes(pool)*1.0/(1024*1024));
	if(ret!=0){goto end;}
	if (cfg->use_pos)
	{
		ret=wtk_tts_pos_cfg_update2(&(cfg->pos),sl,pool);
		//wtk_debug("bytes=%f M\n",wtk_strpool_bytes(pool)*1.0/(1024*1024));
		if(ret!=0){goto end;}
	}
	ret=wtk_polyphn_cfg_update2(&(cfg->polyphn),sl);
	//wtk_debug("bytes=%f M\n",wtk_strpool_bytes(pool)*1.0/(1024*1024));
	if(ret!=0){goto end;}
	ret=wtk_tts_phn_cfg_update2(&(cfg->phn),sl,pool);
	//wtk_debug("bytes=%f M\n",wtk_strpool_bytes(pool)*1.0/(1024*1024));
	if(ret!=0){goto end;}
	ret=0;
end:
	return ret;
}
