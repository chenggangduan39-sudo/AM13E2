/*
 * qtk_tts_syn_cfg.c
 *
 *  Created on: Apr 26, 2023
 *      Author: dm
 */

#include "qtk_tts_syn_cfg.h"

int qtk_tts_syn_cfg_init(qtk_tts_syn_cfg_t *cfg)
{
#ifdef USE_VITS
	qtk_vits_cfg_init(&(cfg->vits));
#endif
#ifdef USE_DEVICE
	qtk_devicetts_cfg_init(&(cfg->device));
#endif

#ifdef USE_LPCNET
	wtk_tac_cfg_syn_lpcnet_init(&cfg->lpcnet);
#endif
	wtk_wsola_cfg_init(&(cfg->wsola));
	cfg->use_vits=0;
	cfg->use_device=0;
	cfg->use_lpcnet=0;
	cfg->use_world=0;
	cfg->use_smooth=0;

	cfg->use_sample_float=0;
	cfg->flow_step = 1;
	cfg->use_flow = 0;

	return 0;
}

int qtk_tts_syn_cfg_clean(qtk_tts_syn_cfg_t *cfg)
{
#ifdef USE_VITS
	qtk_vits_cfg_clean(&(cfg->vits));
#endif
#ifdef USE_DEVICE
	qtk_devicetts_cfg_clean(&(cfg->device));
#endif

#ifdef USE_LPCNET
	wtk_tac_cfg_syn_lpcnet_clean(&cfg->lpcnet);
#endif
	wtk_wsola_cfg_clean(&(cfg->wsola));

	return 0;
}

static int qtk_tts_syn_decode(qtk_tts_syn_cfg_t* cfg)
{
	if (cfg->use_lpcnet == 1 || cfg->use_world == 1)
		return 1;

	return 0;
}

static int qtk_tts_syn_check(qtk_tts_syn_cfg_t* cfg)
{
	if (cfg->use_vits==1)
		return 1;
	if (cfg->use_device == 1 && qtk_tts_syn_decode(cfg))
		return 1;

	return 0;
}
int qtk_tts_syn_cfg_update_local(qtk_tts_syn_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    wtk_local_cfg_t *main_lc;
    wtk_string_t *v;

    main_lc = lc;
    wtk_local_cfg_update_cfg_b(lc, cfg, use_vits, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_device, v);

    wtk_local_cfg_update_cfg_b(lc, cfg, use_lpcnet, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_world, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_smooth, v);

    wtk_local_cfg_update_cfg_b(lc, cfg, use_sample_float, v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_flow,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,flow_step,v);

    if (qtk_tts_syn_check(cfg)==0)
    {
    	wtk_debug("Error: no tts engine set, please check\n");
    	return -1;
    }
#ifdef USE_VITS
    if (cfg->use_vits)
    {
        lc = wtk_local_cfg_find_lc_s(main_lc,"vits");
        if(lc){
        	qtk_vits_cfg_update_local(&cfg->vits,lc);
        }
    }
#endif

#ifdef USE_DEVICE
    if (cfg->use_device)
    {
        lc = wtk_local_cfg_find_lc_s(main_lc,"device");
        if(lc){
        	qtk_devicetts_cfg_update_local(&cfg->device,lc);
        }
    }
#endif

#ifdef USE_LPCNET
    if (cfg->use_lpcnet)
    {
        lc = wtk_local_cfg_find_lc_s(main_lc,"lpcnet");
        if(lc){
            wtk_tac_cfg_syn_lpcnet_update_local(&cfg->lpcnet,lc);
        }
    }
#endif
    if (cfg->use_flow == 0)
    	cfg->use_smooth = 0;
    if (cfg->use_smooth)
    {
        lc = wtk_local_cfg_find_lc_s(main_lc,"wsola");
        if(lc){
            wtk_wsola_cfg_update_local(&cfg->wsola,lc);
        }
    }

	return 0;
}

int qtk_tts_syn_cfg_update(qtk_tts_syn_cfg_t *cfg)
{
#ifdef USE_VITS
	if (cfg->use_vits)
		qtk_vits_cfg_update(&(cfg->vits));
#endif
#ifdef USE_DEVICE
	if (cfg->use_device)
		qtk_devicetts_cfg_update(&(cfg->device));
#endif
#ifdef USE_LPCNET
	if (cfg->use_lpcnet)
		wtk_tac_cfg_syn_lpcnet_update(&cfg->lpcnet);
#endif
	if (cfg->use_smooth)
		wtk_wsola_cfg_update(&(cfg->wsola));

	return 0;
}

int qtk_tts_syn_cfg_update2(qtk_tts_syn_cfg_t *cfg,wtk_source_loader_t *sl)
{
#ifdef USE_VITS
	if(cfg->use_vits)
		qtk_vits_cfg_update2(&(cfg->vits), sl);
#endif
#ifdef USE_DEVICE
	if (cfg->use_device)
		qtk_devicetts_cfg_update2(&(cfg->device), sl);
#endif
#ifdef USE_LPCNET
	if (cfg->use_lpcnet)
		wtk_tac_cfg_syn_lpcnet_update2(&(cfg->lpcnet), sl);
#endif
	if (cfg->use_smooth)
		wtk_wsola_cfg_update2(&(cfg->wsola), sl);
	return 0;
}
