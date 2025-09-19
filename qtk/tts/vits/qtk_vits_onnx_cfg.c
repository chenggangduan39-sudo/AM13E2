/*
 * qtk_vits_onnx_cfg.c
 *
 *  Created on: Sep 7, 2022
 *      Author: dm
 */
#include "qtk_vits_onnx_cfg.h"
#include "wtk/core/rbin/wtk_rbin2.h"


int qtk_vits_onnx_cfg_init(qtk_vits_onnx_cfg_t* cfg)
{
	cfg->heap = wtk_heap_new(128);
	cfg->onnx_fn=NULL;
	cfg->onnx_data=NULL;
	cfg->onnx_data_len=0;
	cfg->nthread = 1;
	cfg->use_bin = 0;

	return 0;
}

int qtk_vits_onnx_cfg_clean(qtk_vits_onnx_cfg_t* cfg)
{
	if (cfg->heap)
		wtk_heap_delete(cfg->heap);

	return 0;
}

int qtk_vits_onnx_cfg_update_local(qtk_vits_onnx_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_str(lc, cfg, onnx_fn, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, nthread, v);

	return 0;
}

int qtk_vits_onnx_cfg_update(qtk_vits_onnx_cfg_t *cfg)
{
	return 0;
}

int qtk_vits_onnx_cfg_update2(qtk_vits_onnx_cfg_t *cfg,wtk_source_loader_t *sl)
{
	wtk_rbin2_t* rbin;
	wtk_rbin2_item_t* item;

	rbin = (wtk_rbin2_t*) sl->hook;
	if (rbin && cfg->onnx_fn)
	{
		item=wtk_rbin2_get2(rbin, cfg->onnx_fn, strlen(cfg->onnx_fn));
		cfg->onnx_data = item->data->data;
		cfg->onnx_data_len = item->data->len;
		cfg->use_bin = 1;
	}

	return 0;
}

int qtk_vits_onnx_cfg_update3(qtk_vits_onnx_cfg_t *cfg,wtk_source_loader_t *sl)
{
	wtk_rbin2_t* rbin;
	wtk_string_t *v;
	wtk_strbuf_t* buf;


	buf = wtk_strbuf_new(128,1);
	rbin = (wtk_rbin2_t*) sl->hook;
	if (rbin && cfg->onnx_fn)
	{
		v=wtk_dir_name(rbin->fn,'/');
		wtk_strbuf_push(buf, v->data, v->len);
		wtk_strbuf_push_c(buf, '/');
		wtk_strbuf_push(buf, cfg->onnx_fn, strlen(cfg->onnx_fn));
		cfg->onnx_fn = wtk_heap_dup_str2(cfg->heap, buf->data, buf->pos);
		if (v)
			wtk_string_delete(v);
	}

	return 0;
}


