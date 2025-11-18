#include "wtk_tts_norm.h" 

wtk_tts_norm_t* wtk_tts_norm_new(wtk_tts_norm_cfg_t *cfg,wtk_rbin2_t *rbin)
{
	wtk_tts_norm_t *n;

	if (NULL==cfg->lex_fn)
	{
		return NULL;
	}
	n=(wtk_tts_norm_t*)wtk_malloc(sizeof(wtk_tts_norm_t));
	n->cfg=cfg;
	n->lex=wtk_lex_new(&(cfg->lex));
	if(rbin)
	{
		wtk_rbin2_load_file(rbin,n->lex,(wtk_source_load_handler_t)wtk_lex_compile3,cfg->lex_fn);
	}else
	{
		wtk_lex_compile(n->lex,cfg->lex_fn);
	}
	return n;
}

int wtk_tts_norm_bytes(wtk_tts_norm_t* n)
{
	return 0;
}

void wtk_tts_norm_delete(wtk_tts_norm_t *n)
{
	wtk_lex_delete(n->lex);
	wtk_free(n);
}

void wtk_tts_norm_reset(wtk_tts_norm_t *n)
{
	wtk_lex_reset(n->lex);
}

wtk_string_t wtk_tts_norm_process(wtk_tts_norm_t *n,char *data,int bytes)
{
	return wtk_lex_process(n->lex,data,bytes);
}
