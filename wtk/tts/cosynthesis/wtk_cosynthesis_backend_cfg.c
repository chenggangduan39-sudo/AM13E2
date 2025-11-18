#include "wtk_cosynthesis_backend_cfg.h"

int wtk_cosynthesis_backend_cfg_init(wtk_cosynthesis_backend_cfg_t *cfg)
{
    wtk_cosynthesis_dtree_cfg_init(&(cfg->tree_cfg));
    wtk_cosynthesis_hmm_cfg_init(&(cfg->hmm_cfg));
	cfg->tree=NULL;
	cfg->hmm=NULL;
    cfg->spec_weight = 0.1;
    cfg->dur_weight = 1;
    cfg->pitch_weight = 0.2;
    cfg->conca_pitch_weight = 10.0;
    cfg->conca_spec_weight = 0.1;

    return 0;
}

int wtk_cosynthesis_backend_cfg_clean(wtk_cosynthesis_backend_cfg_t *cfg)
{
	wtk_cosynthesis_dtree_cfg_clean(&(cfg->tree_cfg));
	wtk_cosynthesis_hmm_cfg_clean(&(cfg->hmm_cfg));
	if(cfg->tree)
	{
		wtk_cosynthesis_dtree_delete(cfg->tree);
	}
	if(cfg->hmm)
	{
		wtk_cosynthesis_hmm_delete(cfg->hmm);
	}
	return 0;  
}

int wtk_cosynthesis_backend_cfg_update_local(wtk_cosynthesis_backend_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret=-1;
	lc=main;
    wtk_local_cfg_update_cfg_f(lc, cfg, spec_weight, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, dur_weight, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, pitch_weight, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, conca_pitch_weight, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, conca_spec_weight, v);
	lc=wtk_local_cfg_find_lc_s(main,"tree");
	if(lc)
	{
		ret=wtk_cosynthesis_dtree_cfg_update_local(&(cfg->tree_cfg),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"hmm");
	if(lc)
	{
		ret=wtk_cosynthesis_hmm_cfg_update_local(&(cfg->hmm_cfg),lc);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return ret;
}

int wtk_cosynthesis_backend_cfg_update2(wtk_cosynthesis_backend_cfg_t *cfg,wtk_source_loader_t *sl,wtk_strpool_t *pool)
{
	int ret;

	ret=wtk_cosynthesis_dtree_cfg_update2(&(cfg->tree_cfg),sl);
	if(ret!=0){goto end;}
	ret=wtk_cosynthesis_hmm_cfg_update2(&(cfg->hmm_cfg),sl);
	if(ret!=0){goto end;}
    ret=-1;
    cfg->tree=wtk_cosynthesis_dtree_new(&(cfg->tree_cfg),sl,pool);
    if(!cfg->tree){goto end;}
    cfg->hmm=wtk_cosynthesis_hmm_new(&(cfg->hmm_cfg),sl,pool,cfg->tree);
    if(!cfg->hmm){goto end;}
	ret=0;

end:
	return ret;
}
