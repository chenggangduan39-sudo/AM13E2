/*
 * qtk_vits_cfg.c
 *
 *  Created on: Aug 26, 2022
 *      Author: dm
 */
#include "qtk_vits_cfg.h"

int qtk_vits_cfg_init(qtk_vits_cfg_t *cfg)
{
	qtk_vits_onnx_cfg_init(&(cfg->onnx));
	qtk_vits_onnx_cfg_init(&(cfg->dec_onnx));
	wtk_wsola_cfg_init(&(cfg->wsola));
	cfg->chunk_len = 34;
	cfg->chunk_pad = 3;
	cfg->step = 0;
	cfg->upsample = 256;
	cfg->nt_thd = 1;
	cfg->use_stream = 0;

	return 0;
}

int qtk_vits_cfg_clean(qtk_vits_cfg_t *cfg)
{
	qtk_vits_onnx_cfg_clean(&(cfg->onnx));
	qtk_vits_onnx_cfg_clean(&(cfg->dec_onnx));
	wtk_wsola_cfg_clean(&(cfg->wsola));

	return 0;
}

int qtk_vits_cfg_update_local(qtk_vits_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    wtk_local_cfg_t *main_lc;
    wtk_string_t *v;

    main_lc = lc;
    wtk_local_cfg_update_cfg_i(lc, cfg, chunk_len, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, chunk_pad, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, step, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, upsample, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, nt_thd, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_stream, v);

    lc = wtk_local_cfg_find_lc_s(main_lc,"onnx");
    if(lc){
        qtk_vits_onnx_cfg_update_local(&cfg->onnx,lc);
    }

    lc = wtk_local_cfg_find_lc_s(main_lc,"dec_onnx");
    if(lc){
        qtk_vits_onnx_cfg_update_local(&cfg->dec_onnx,lc);
    }

    lc = wtk_local_cfg_find_lc_s(main_lc,"wsola");
    if(lc){
        wtk_wsola_cfg_update_local(&cfg->wsola,lc);
    }

	return 0;
}

int qtk_vits_cfg_update(qtk_vits_cfg_t *cfg)
{
	qtk_vits_onnx_cfg_update(&(cfg->onnx));
	qtk_vits_onnx_cfg_update(&(cfg->dec_onnx));
	wtk_wsola_cfg_update(&(cfg->wsola));

	return 0;
}

int qtk_vits_cfg_update2(qtk_vits_cfg_t *cfg,wtk_source_loader_t *sl)
{
	qtk_vits_onnx_cfg_update2(&(cfg->onnx), sl);
	qtk_vits_onnx_cfg_update2(&(cfg->dec_onnx), sl);
	wtk_wsola_cfg_update2(&(cfg->wsola), sl);

	return 0;
}
