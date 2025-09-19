/*
 * qtk_tts_module_cfg.c
 *
 *  Created on: Apr 24, 2023
 *      Author: dm
 */
#include "qtk_tts_module_cfg.h"

int qtk_tts_module_cfg_init(qtk_tts_module_cfg_t *cfg)
{
	qtk_tts_parse_cfg_init(&(cfg->parse));
	qtk_tts_syn_cfg_init(&(cfg->syn));
	cfg->pitch=1.0;
	cfg->tempo=1.0;
	cfg->vol=1.0;
	cfg->rate=1.0;

	return 0;
}

int qtk_tts_module_cfg_clean(qtk_tts_module_cfg_t *cfg)
{
	qtk_tts_parse_cfg_clean(&(cfg->parse));
	qtk_tts_syn_cfg_clean(&(cfg->syn));

	return 0;
}

int qtk_tts_module_cfg_update_local(qtk_tts_module_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    wtk_local_cfg_t *main_lc;
    int ret=0;

    main_lc = lc;
    lc = wtk_local_cfg_find_lc_s(main_lc,"parse");
    if(lc){
        ret=qtk_tts_parse_cfg_update_local(&cfg->parse,lc);
        if (ret!=0) goto end;
    }
    lc = wtk_local_cfg_find_lc_s(main_lc,"syn");
    if(lc){
    	ret=qtk_tts_syn_cfg_update_local(&cfg->syn,lc);
    	if (ret!=0) goto end;
    }
end:

	return ret;
}

int qtk_tts_module_cfg_update(qtk_tts_module_cfg_t *cfg)
{
	qtk_tts_parse_cfg_update(&(cfg->parse));
	qtk_tts_syn_cfg_update(&(cfg->syn));

	return 0;
}

int qtk_tts_module_cfg_update2(qtk_tts_module_cfg_t *cfg,wtk_source_loader_t *sl)
{
	qtk_tts_parse_cfg_update2(&(cfg->parse), sl);
	qtk_tts_syn_cfg_update2(&(cfg->syn), sl);

	return 0;
}

#include "wtk/core/cfg/wtk_mbin_cfg.h"
qtk_tts_module_cfg_t* qtk_tts_module_cfg_new_bin(char *cfg_fn,int seek_pos)
{
	wtk_mbin_cfg_t *main_cfg;
	qtk_tts_module_cfg_t *cfg;

	cfg=0;
	main_cfg=wtk_mbin_cfg_new_type2(seek_pos,qtk_tts_module_cfg,cfg_fn,"./cfg");
	if(main_cfg){
		cfg=(qtk_tts_module_cfg_t*)(main_cfg->cfg);
		cfg->mbin_cfg=main_cfg;
	}

	return cfg;
}

void qtk_tts_module_cfg_delete_bin(qtk_tts_module_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}

int qtk_tts_module_cfg_delete(qtk_tts_module_cfg_t *cfg)
{
	if(!cfg){return 0;}
	qtk_tts_module_cfg_clean(cfg);
	wtk_free(cfg);

	return 0;
}
