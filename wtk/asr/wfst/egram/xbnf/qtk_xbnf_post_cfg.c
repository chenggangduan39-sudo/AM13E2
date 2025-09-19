/*
 * qtk_xbnf_post_cfg.c
 *
 *  Created on: Jan 6, 2020
 *      Author: dm
 */
#include "qtk_xbnf_post_cfg.h"

int qtk_xbnf_post_cfg_init(qtk_xbnf_post_cfg_t* cfg)
{
	cfg->pct=0.9; //0.5;
	cfg->max_skip=3;
	cfg->min_len=1;
	cfg->wrdpen=-10.0;
	cfg->step_wrdpen=-5.0;
	cfg->lpron_fct=1.0;
	cfg->mpscale=10;
	cfg->min_selfloop=1;
	cfg->max_selfloop=3;
	cfg->use_selfloop_limit=1;

	return 0;
}
int qtk_xbnf_post_cfg_update_local(qtk_xbnf_post_cfg_t* cfg, wtk_local_cfg_t* main)
{
	int ret;
	wtk_string_t* v;

	ret=0;
	wtk_local_cfg_update_cfg_f(main, cfg, pct, v);
	wtk_local_cfg_update_cfg_i(main, cfg, max_skip, v);
	wtk_local_cfg_update_cfg_i(main, cfg, min_len, v);
	wtk_local_cfg_update_cfg_f(main, cfg, mpscale, v);
	wtk_local_cfg_update_cfg_f(main, cfg, wrdpen, v);
	wtk_local_cfg_update_cfg_f2(main, cfg, wrdpen, leak_wrdpen, v);    //support old res
	wtk_local_cfg_update_cfg_f(main, cfg, step_wrdpen, v);
	wtk_local_cfg_update_cfg_f2(main, cfg, step_wrdpen, step_leak_wrdpen, v);   //support old res
	wtk_local_cfg_update_cfg_b(main, cfg, use_selfloop_limit, v);

	return ret;
}

int qtk_xbnf_post_cfg_update(qtk_xbnf_post_cfg_t* cfg)
{
	return 0;
}

int qtk_xbnf_post_cfg_update2(qtk_xbnf_post_cfg_t* cfg, wtk_source_loader_t* sl)
{
	return 0;
}

int qtk_xbnf_post_cfg_clean(qtk_xbnf_post_cfg_t* cfg)
{
	return 0;
}

