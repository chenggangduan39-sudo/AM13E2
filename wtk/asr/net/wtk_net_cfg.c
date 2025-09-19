#include "wtk_net_cfg.h"

int wtk_net_cfg_init(wtk_net_cfg_t *cfg)
{
	//cfg->frame_dur=0;
	cfg->factor_lm=0;
	cfg->allow_xwrd_exp=0;
	cfg->allow_ctx_exp=1;
	cfg->force_ctx_exp=0;
	cfg->force_left_biphones=0;
	cfg->force_right_biphones=0;
	cfg->sp_word_boundary=1;
	return 0;
}

int wtk_net_cfg_update_local(wtk_net_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_b(lc,cfg,factor_lm,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,allow_xwrd_exp,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,allow_ctx_exp,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,force_ctx_exp,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,force_left_biphones,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,force_right_biphones,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,sp_word_boundary,v);
	//wtk_net_cfg_print(cfg);
	return 0;
}

int wtk_net_cfg_update(wtk_net_cfg_t *cfg)
{
	return 0;
}

int wtk_net_cfg_clean(wtk_net_cfg_t *cfg)
{
	return 0;
}

void wtk_net_cfg_print(wtk_net_cfg_t *cfg)
{
	wtk_debug("============= net configure ============\n");
	wtk_debug_item(cfg,"%d",factor_lm);
	wtk_debug_item(cfg,"%d",allow_xwrd_exp);
	wtk_debug_item(cfg,"%d",allow_ctx_exp);
	wtk_debug_item(cfg,"%d",force_ctx_exp);
	wtk_debug_item(cfg,"%d",force_left_biphones);
	wtk_debug_item(cfg,"%d",force_right_biphones);
}
