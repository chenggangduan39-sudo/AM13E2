#include "wtk_xbnfnet_cfg.h"

int wtk_xbnfnet_cfg_init(wtk_xbnfnet_cfg_t *cfg)
{
	wtk_xbnf_cfg_init(&(cfg->xbnf));
	cfg->lower=0;
	cfg->use_leak=0;
	cfg->use_selfloop=0;
	cfg->use_addre=0;
	cfg->use_ctx=0;
	cfg->type=0;
	qtk_xbnf_post_cfg_init(&cfg->xbnf_post);

	return 0;
}

int wtk_xbnfnet_cfg_clean(wtk_xbnfnet_cfg_t *cfg)
{
	wtk_xbnf_cfg_clean(&(cfg->xbnf));
	qtk_xbnf_post_cfg_clean(&(cfg->xbnf_post));

	return 0;
}

int wtk_xbnfnet_cfg_update_local(wtk_xbnfnet_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;

	lc=main;
	wtk_local_cfg_update_cfg_b(lc,cfg,lower,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_leak,v);
	wtk_local_cfg_update_cfg_b_local(lc,cfg,use_selfloop,v);
	wtk_local_cfg_update_cfg_b_local(lc,cfg,use_addre,v);
	wtk_local_cfg_update_cfg_b_local(lc,cfg,use_ctx,v);
	lc=wtk_local_cfg_find_lc_s(main,"xbnf");
	if(lc)
	{
		wtk_xbnf_cfg_update_local(&(cfg->xbnf),lc);
	}

	lc=wtk_local_cfg_find_lc_s(main,"xbnf_post");
	if(lc)
	{
		qtk_xbnf_post_cfg_update_local(&(cfg->xbnf_post),lc);
	}

	return 0;
}

int wtk_xbnfnet_cfg_update(wtk_xbnfnet_cfg_t *cfg)
{
	wtk_xbnf_cfg_update(&(cfg->xbnf));
	qtk_xbnf_post_cfg_update(&(cfg->xbnf_post));

	return 0;
}

int wtk_xbnfnet_cfg_update2(wtk_xbnfnet_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret=0;

        wtk_xbnf_cfg_update2(&(cfg->xbnf), sl);
        qtk_xbnf_post_cfg_update(&(cfg->xbnf_post));

        return ret;
}
