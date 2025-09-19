#include "qtk/nnrt/qtk_nnrt_cfg.h"
#include "wtk/core/cfg/wtk_local_cfg.h"

int qtk_nnrt_cfg_init(qtk_nnrt_cfg_t *cfg) {
    cfg->use_ncnn = 0;
    cfg->use_onnxruntime = 0;
#ifdef QTK_NNRT_NCNN
    cfg->ncnn_model_fn = NULL;
    cfg->ncnn_param_fn = NULL;
    cfg->metadata = NULL;
    cfg->ncnn_use_winograd_conv = 0;
    cfg->use_ncnn = 1;
    cfg->param_data = NULL;
#endif
#ifdef QTK_NNRT_ONNXRUNTIME
    cfg->onnx_fn = NULL;
    cfg->use_onnxruntime = 1;
#endif
#ifdef QTK_NNRT_SS_IPU
    cfg->use_ss_ipu = 1;
    cfg->mdl_fn = NULL;
#endif
    cfg->num_threads = 1;
    cfg->enable_profiling = 0;
    cfg->bin_data = NULL;
    cfg->use_libtorch = 0;
    cfg->model = NULL;
    cfg->use_rknpu = 0;
    return 0;
}

int qtk_nnrt_cfg_clean(qtk_nnrt_cfg_t *cfg) {
    if (cfg->bin_data) {
        wtk_string_delete(cfg->bin_data);
    }
#ifdef QTK_NNRT_NCNN
    if (cfg->param_data) {
        wtk_string_delete(cfg->param_data);
    }
#endif
    return 0;
}

int qtk_nnrt_cfg_update(qtk_nnrt_cfg_t *cfg) { return 0; }

int qtk_nnrt_cfg_update_local(qtk_nnrt_cfg_t *cfg, wtk_local_cfg_t *main) {
    wtk_string_t *v;
#ifdef QTK_NNRT_NCNN
    wtk_local_cfg_update_cfg_str_local(main, cfg, ncnn_model_fn, v);
    wtk_local_cfg_update_cfg_str_local(main, cfg, ncnn_param_fn, v);
    cfg->metadata = wtk_local_cfg_find_lc_s(main, "metadata");
    wtk_local_cfg_update_cfg_b(main, cfg, ncnn_use_winograd_conv, v);
#endif

#ifdef QTK_NNRT_ONNXRUNTIME
    wtk_local_cfg_update_cfg_str_local(main, cfg, onnx_fn, v);
#endif

    wtk_local_cfg_update_cfg_b_local(main, cfg, use_rknpu, v);

#ifdef QTK_NNRT_SS_IPU
    wtk_local_cfg_update_cfg_b_local(main, cfg, use_ss_ipu, v);
    wtk_local_cfg_update_cfg_str_local(main, cfg, mdl_fn, v);
#endif
    wtk_local_cfg_update_cfg_b_local(main, cfg, use_ncnn, v);
    wtk_local_cfg_update_cfg_b_local(main, cfg, use_onnxruntime, v);
    wtk_local_cfg_update_cfg_b_local(main, cfg, use_libtorch, v);
    wtk_local_cfg_update_cfg_b_local(main, cfg, enable_profiling, v);
    wtk_local_cfg_update_cfg_i(main, cfg, num_threads, v);
    wtk_local_cfg_update_cfg_str_local(main, cfg, model, v);
    return 0;
}

#if defined(QTK_NNRT_NCNN) || defined(QTK_NNRT_ONNXRUNTIME)
int load_model_(qtk_nnrt_cfg_t *cfg, wtk_source_t *src) {
    cfg->bin_data = wtk_source_read_file(src);
    return 0;
}
#endif

#ifdef QTK_NNRT_NCNN
int load_param_(qtk_nnrt_cfg_t *cfg, wtk_source_t *src) {
    cfg->param_data = wtk_source_read_file(src);
    return 0;
}
#endif

int qtk_nnrt_cfg_update2(qtk_nnrt_cfg_t *cfg, wtk_source_loader_t *sl) {
#ifdef QTK_NNRT_ONNXRUNTIME
    if (cfg->use_onnxruntime) {
        wtk_source_loader_load(sl, cfg, (wtk_source_load_handler_t)load_model_,
                               cfg->onnx_fn);
    }
    return 0;
#endif
#ifdef QTK_NNRT_NCNN
    if (cfg->use_ncnn) {
        wtk_source_loader_load(sl, cfg, (wtk_source_load_handler_t)load_model_,
                               cfg->ncnn_model_fn);
        wtk_source_loader_load(sl, cfg, (wtk_source_load_handler_t)load_param_,
                               cfg->ncnn_param_fn);
    }
    return 0;
#endif
    return 0;
}
