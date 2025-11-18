#include "wtk/asr/ced/wtk_ced_cfg.h"

int wtk_ced_cfg_init(wtk_ced_cfg_t *cfg) {
    cfg->hop_size = 2;
    cfg->chunk_size = 2;
    cfg->sample_rate = 16000;
    cfg->byte_per_sample = 2;
    cfg->onnx_output_index = 0;
    cfg->classes_num = 3;
    cfg->start_nframe_threshold = 1;
    cfg->end_nframe_threshold = 2;
    cfg->energy_threshold = 300.0;
    cfg->use_energy = 1;
#ifdef ONNX_DEC
    qtk_onnxruntime_cfg_init(&(cfg->onnx));
#endif

    return 0;
}

int wtk_ced_cfg_clean(wtk_ced_cfg_t *cfg) {
#ifdef ONNX_DEC
    qtk_onnxruntime_cfg_clean(&(cfg->onnx));
#endif

    return 0;
}

int wtk_ced_cfg_update(wtk_ced_cfg_t *cfg) { return 0; }

int wtk_ced_cfg_update2(wtk_ced_cfg_t *cfg, wtk_source_loader_t *sl) {
    int ret = 0;
#ifdef ONNX_DEC
    ret = qtk_onnxruntime_cfg_update2(&(cfg->onnx), sl->hook);
    if (ret != 0) {
        wtk_debug("update onnx failed\n");
        goto end;
    }

end:
#endif
    return ret;
}

int wtk_ced_cfg_update_local(wtk_ced_cfg_t *cfg, wtk_local_cfg_t *main) {
    int ret = 0;
    wtk_string_t *v = NULL;
    wtk_local_cfg_t *lc = main;

    wtk_local_cfg_update_cfg_i(lc, cfg, hop_size, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, chunk_size, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, sample_rate, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, byte_per_sample, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, onnx_output_index, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, classes_num, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, start_nframe_threshold, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, end_nframe_threshold, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, energy_threshold, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_energy, v);
#ifdef ONNX_DEC
    lc = wtk_local_cfg_find_lc_s(lc, "onnx");
    if (lc) {
        ret = qtk_onnxruntime_cfg_update_local(&(cfg->onnx), lc);
        if (ret != 0) {
            wtk_debug("update local onnx failed\n");
            goto end;
        }
    }

end:
#endif
    return ret;
}
