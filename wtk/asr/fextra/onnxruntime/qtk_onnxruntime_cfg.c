#include "qtk_onnxruntime_cfg.h"
#include <math.h>

int qtk_onnxruntime_cfg_init(qtk_onnxruntime_cfg_t *cfg)
{
	cfg->onnx_fn = NULL;
	cfg->outer_in_num = 1;
	cfg->outer_out_num = 1;
	cfg->num_inter_threads = 1;
	cfg->num_intra_threads = 1;
	cfg->use_mem = 0;
	cfg->use_reduce_mem = 0;
	cfg->use_inner_item = 0;
	cfg->pre_malloc = 1;
	return 0;
}

int qtk_onnxruntime_cfg_clean(qtk_onnxruntime_cfg_t *cfg)
{
	return 0;
}

int qtk_onnxruntime_cfg_update(qtk_onnxruntime_cfg_t *cfg)
{
	return 0;
}

int qtk_onnxruntime_cfg_update2(qtk_onnxruntime_cfg_t *cfg,wtk_rbin2_t *rb)
{
	cfg->rb = rb;
	if(rb){
		cfg->use_mem = 1;
	}
	return 0;
}

int qtk_onnxruntime_cfg_update_local(qtk_onnxruntime_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;
    wtk_local_cfg_update_cfg_str(lc, cfg, onnx_fn, v);
    wtk_local_cfg_update_cfg_i(lc,cfg,outer_in_num,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,outer_out_num,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,num_inter_threads,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,num_intra_threads,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_mem,v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_reduce_mem, v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_inner_item,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,pre_malloc,v);

	return 0;
}
