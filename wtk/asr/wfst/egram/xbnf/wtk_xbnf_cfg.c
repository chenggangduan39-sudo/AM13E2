#include "wtk_xbnf_cfg.h"

int wtk_xbnf_cfg_init(wtk_xbnf_cfg_t *cfg)
{
	wtk_segmenter_cfg_init(&(cfg->segmenter));
	cfg->hash_hint=25007;
	cfg->expr_hint=137;
	cfg->use_merge=0;
	cfg->use_seg=0;
	cfg->en_pre=NULL;
	cfg->en_mid=NULL;
	cfg->en_pst=NULL;
	return 0;
}

int wtk_xbnf_cfg_clean(wtk_xbnf_cfg_t *cfg)
{
	wtk_segmenter_cfg_clean(&(cfg->segmenter));
	return 0;
}

int wtk_xbnf_cfg_update_local(wtk_xbnf_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;
	wtk_local_cfg_t *m=lc;
	int ret=0;
	wtk_local_cfg_update_cfg_i(lc,cfg,hash_hint,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,expr_hint,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_merge,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_seg,v);

	if(cfg->use_seg)
	{
		lc=wtk_local_cfg_find_lc_s(m,"seg");
		if(lc)
		{
			ret=wtk_segmenter_cfg_update_local(&(cfg->segmenter),lc);
			if(ret!=0){goto end;}
		}
	}

	cfg->en_pre=wtk_local_cfg_find_array_s(lc,"en_pre");
	cfg->en_mid=wtk_local_cfg_find_array_s(lc,"en_mid");
	cfg->en_pst=wtk_local_cfg_find_array_s(lc,"en_pst");

end:
	return ret;
}

int wtk_xbnf_cfg_update(wtk_xbnf_cfg_t *cfg)
{
	if(cfg->use_seg)
	{
		wtk_segmenter_cfg_update(&(cfg->segmenter));
	}
	return 0;
}

int wtk_xbnf_cfg_update2(wtk_xbnf_cfg_t *cfg, wtk_source_loader_t *sl) {
    if (cfg->use_seg) {
        wtk_segmenter_cfg_update2(&(cfg->segmenter), sl);
    }
    return 0;
}
