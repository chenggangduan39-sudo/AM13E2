#include "wtk/asr/kws/qtk_audio2vec_cfg.h"

int qtk_audio2vec_cfg_init(qtk_audio2vec_cfg_t *cfg) {
    cfg->use_vad = 0;
    cfg->use_window_pad = 0;
    wtk_svprint_cfg_init(&cfg->svprint);
    wtk_kvad_cfg_init(&(cfg->vad));
    return 0;
}

int qtk_audio2vec_cfg_clean(qtk_audio2vec_cfg_t *cfg) {
    wtk_svprint_cfg_clean(&cfg->svprint);
    wtk_kvad_cfg_clean(&(cfg->vad));
    return 0;
}

int qtk_audio2vec_cfg_update(qtk_audio2vec_cfg_t *cfg) {
    wtk_source_loader_t sl;
    sl.hook = 0;
    sl.vf = wtk_source_load_file_v;
    qtk_audio2vec_cfg_update2(cfg, &sl);
    return 0;
}

int qtk_audio2vec_cfg_update2(qtk_audio2vec_cfg_t *cfg,
                              wtk_source_loader_t *sl) {
    wtk_svprint_cfg_update2(&cfg->svprint, sl);

    if (cfg->use_vad) {
        wtk_kvad_cfg_update2(&(cfg->vad), sl);
    }

    return 0;
}

int qtk_audio2vec_cfg_update_local(qtk_audio2vec_cfg_t *cfg, wtk_local_cfg_t *main) {
    wtk_local_cfg_t *lc;
    wtk_string_t *v;

    wtk_local_cfg_update_cfg_b(main, cfg, use_vad, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_window_pad, v);

    lc = wtk_local_cfg_find_lc_s(main, "svprint");
    if (lc) {
        wtk_svprint_cfg_update_local(&cfg->svprint, lc);
    }

    lc = wtk_local_cfg_find_lc_s(main, "kvad");
    if (lc) {
        wtk_kvad_cfg_update_local(&(cfg->vad), lc);
    }

    return 0;
}
