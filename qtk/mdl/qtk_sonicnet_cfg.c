#include "qtk/mdl/qtk_sonicnet_cfg.h"
#include "qtk/nnrt/qtk_nnrt_cfg.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/wtk_alloc.h"

int qtk_sonicnet_cfg_init(qtk_sonicnet_cfg_t *cfg) {
    qtk_nnrt_cfg_init(&cfg->rt);
    qtk_nnrt_cfg_init(&cfg->vad_rt);
    cfg->left_context = 1;
    cfg->right_context = 1;
    cfg->nzc = 64;
    cfg->chunk_sz = NULL;
    cfg->nchunk_sz = 0;
    cfg->act_hint_thresh = 0.1;
    cfg->dis_start = 0.5;
    cfg->dis_end = 12;
    cfg->channel = 1;
    cfg->K = 36;
    cfg->D = 64;
    cfg->vad_chunk_sz = 118;
    cfg->vad_use_thread = 0;
    cfg->vad_chunk_overlap = 0;
    cfg->channel = 1;
    cfg->use_ifft = 1;
    cfg->dynamic_scaler = 10.0;
    return 0;
}

int qtk_sonicnet_cfg_clean(qtk_sonicnet_cfg_t *cfg) {
    if (cfg->chunk_sz) {
        wtk_free(cfg->chunk_sz);
    }
    qtk_nnrt_cfg_clean(&cfg->rt);
    qtk_nnrt_cfg_clean(&cfg->vad_rt);
    return 0;
}

int qtk_sonicnet_cfg_update(qtk_sonicnet_cfg_t *cfg) {
    qtk_nnrt_cfg_update(&cfg->rt);
    qtk_nnrt_cfg_update(&cfg->vad_rt);
    return 0;
}

int qtk_sonicnet_cfg_update_local(qtk_sonicnet_cfg_t *cfg,
                                  wtk_local_cfg_t *main) {
    wtk_string_t *v;
    wtk_local_cfg_t *lc;
    wtk_array_t *chunk_sz;

    wtk_local_cfg_update_cfg_i(main, cfg, left_context, v);
    wtk_local_cfg_update_cfg_i(main, cfg, right_context, v);
    wtk_local_cfg_update_cfg_f(main, cfg, act_hint_thresh, v);
    wtk_local_cfg_update_cfg_i(main, cfg, channel, v);
    wtk_local_cfg_update_cfg_i(main, cfg, K, v);
    wtk_local_cfg_update_cfg_i(main, cfg, D, v);
    wtk_local_cfg_update_cfg_i(main, cfg, vad_chunk_sz, v);
    wtk_local_cfg_update_cfg_b(main, cfg, vad_use_thread, v);
    wtk_local_cfg_update_cfg_f(main, cfg, vad_chunk_overlap, v);
    wtk_local_cfg_update_cfg_f(main, cfg, dis_start, v);
    wtk_local_cfg_update_cfg_f(main, cfg, dis_end, v);
    wtk_local_cfg_update_cfg_f(main, cfg, dynamic_scaler, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_ifft, v);
    wtk_local_cfg_update_cfg_i(main, cfg, nzc, v);

    lc = wtk_local_cfg_find_lc_s(main, "rt");
    if (lc) {
        qtk_nnrt_cfg_update_local(&cfg->rt, lc);
    }
    lc = wtk_local_cfg_find_lc_s(main, "vad_rt");
    if (lc) {
        qtk_nnrt_cfg_update_local(&cfg->vad_rt, lc);
    }
    chunk_sz = wtk_local_cfg_find_array_s(main, "chunk_sz");
    if (chunk_sz) {
        wtk_string_t **data = (wtk_string_t **)chunk_sz->slot;
        cfg->nchunk_sz = chunk_sz->nslot;
        cfg->chunk_sz = wtk_malloc(sizeof(int) * cfg->nchunk_sz);
        for (int i = 0; i < cfg->nchunk_sz; i++) {
            cfg->chunk_sz[i] = atoi(data[i]->data);
        }
    }
    return 0;
}
