#include "wtk_tts_norm_cfg.h" 

int wtk_tts_norm_cfg_init(wtk_tts_norm_cfg_t *cfg)
{
	wtk_lex_cfg_init(&(cfg->lex));
	cfg->lex_fn=NULL;
	return 0;
}

int wtk_tts_norm_cfg_clean(wtk_tts_norm_cfg_t *cfg)
{
	wtk_lex_cfg_clean(&(cfg->lex));
	return 0;
}

int wtk_tts_norm_cfg_update_local(wtk_tts_norm_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;

	lc=main;
	wtk_local_cfg_update_cfg_str(lc,cfg,lex_fn,v);
	lc=wtk_local_cfg_find_lc_s(main,"lex");
	if(lc)
	{
		wtk_lex_cfg_update_local(&(cfg->lex),lc);
	}
	return 0;
}

int wtk_tts_norm_cfg_update(wtk_tts_norm_cfg_t *cfg)
{
	wtk_lex_cfg_update(&(cfg->lex));
	return 0;
}

int wtk_tts_norm_cfg_update2(wtk_tts_norm_cfg_t *cfg, wtk_source_loader_t *sl)
{
	wtk_lex_cfg_update2(&(cfg->lex), sl);
	return 0;
}
