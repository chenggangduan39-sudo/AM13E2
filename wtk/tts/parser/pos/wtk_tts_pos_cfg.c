#include "wtk_tts_pos_cfg.h" 

int wtk_tts_pos_cfg_init(wtk_tts_pos_cfg_t *cfg)
{
	cfg->hmm_fn=NULL;
	cfg->voc_fn=NULL;
	cfg->nwrd=0;
	cfg->attr=NULL;
	cfg->hmm=NULL;
	cfg->use_voc_bin=0;
	return 0;
}

int wtk_tts_pos_cfg_clean(wtk_tts_pos_cfg_t *cfg)
{
	if(cfg->hmm)
	{
		wtk_tts_poshmm_delete(cfg->hmm);
	}
	return 0;
}

int wtk_tts_pos_cfg_bytes(wtk_tts_pos_cfg_t *cfg)
{
	if( cfg->hmm)
		return wtk_tts_poshmm_bytes(cfg->hmm);
	else
		return 0;
}

int wtk_tts_pos_cfg_update_local(wtk_tts_pos_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	cfg->attr=wtk_local_cfg_find_array_s(lc,"attr");
	wtk_local_cfg_update_cfg_str(lc,cfg,hmm_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,voc_fn,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,nwrd,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_voc_bin,v);
	return 0;
}

int wtk_tts_pos_cfg_update(wtk_tts_pos_cfg_t *cfg,wtk_strpool_t *pool)
{
	wtk_source_loader_t sl;

	wtk_source_loader_init_file(&(sl));
	return wtk_tts_pos_cfg_update2(cfg,&(sl),pool);
}

int wtk_tts_pos_cfg_update2(wtk_tts_pos_cfg_t *cfg,wtk_source_loader_t *sl,wtk_strpool_t *pool)
{
	int ret;

	cfg->hmm=wtk_tts_poshmm_new(pool,cfg->nwrd);
	if(cfg->hmm_fn)
	{
		ret=wtk_source_loader_load(sl,cfg->hmm,(wtk_source_load_handler_t)wtk_tts_poshmm_load,cfg->hmm_fn);
		if(ret!=0)
		{
			wtk_debug("load hmm %s failed\n",cfg->hmm_fn);
			goto end;
		}
	}
	if(cfg->voc_fn)
	{
		if(cfg->use_voc_bin)
		{

		}else
		{
			ret=wtk_source_loader_load(sl,cfg->hmm,(wtk_source_load_handler_t)wtk_tts_poshmm_load_voc,cfg->voc_fn);
			if(ret!=0)
			{
				wtk_debug("load voc %s failed\n",cfg->voc_fn);
				goto end;
			}
		}
	}
	ret=0;
end:
	return ret;
}
