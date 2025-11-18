#include "qtk/avspeech/qtk_avspeech_visual_voice_cfg.h"

int qtk_avspeech_visual_voice_cfg_init(qtk_avspeech_visual_voice_cfg_t *cfg) {
    qtk_nnrt_cfg_init(&cfg->av_sep);
    qtk_nnrt_cfg_init(&cfg->lip_embedding);
    qtk_stft_librosa_cfg_init(&cfg->stft);
    cfg->fps = 25;
    cfg->num_frames = 64;
    cfg->lip_embedding_dim = 256;
    cfg->audio_segment_duration = 2.55;
    return 0;
}

int qtk_avspeech_visual_voice_cfg_clean(qtk_avspeech_visual_voice_cfg_t *cfg) {
    qtk_nnrt_cfg_clean(&cfg->av_sep);
    qtk_nnrt_cfg_clean(&cfg->lip_embedding);
    qtk_stft_librosa_cfg_clean(&cfg->stft);
    return 0;
}

int qtk_avspeech_visual_voice_cfg_update(qtk_avspeech_visual_voice_cfg_t *cfg) {
    qtk_nnrt_cfg_update(&cfg->av_sep);
    qtk_nnrt_cfg_update(&cfg->lip_embedding);
    qtk_stft_librosa_cfg_update(&cfg->stft);
    return 0;
}

int qtk_avspeech_visual_voice_cfg_update_local(
    qtk_avspeech_visual_voice_cfg_t *cfg, wtk_local_cfg_t *lc) {
    wtk_string_t *v;
    wtk_local_cfg_t *sub;
    sub = wtk_local_cfg_find_lc_s(lc, "stft");
    if (sub) {
        qtk_stft_librosa_cfg_update_local(&cfg->stft, sub);
    }
    sub = wtk_local_cfg_find_lc_s(lc, "lip_embedding");
    if (sub) {
        qtk_nnrt_cfg_update_local(&cfg->lip_embedding, sub);
    }
    sub = wtk_local_cfg_find_lc_s(lc, "av_sep");
    if (sub) {
        qtk_nnrt_cfg_update_local(&cfg->av_sep, sub);
    }
    wtk_local_cfg_update_cfg_i(lc, cfg, fps, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, audio_segment_duration, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, num_frames, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, lip_embedding_dim, v);
    return 0;
}

int qtk_avspeech_visual_voice_cfg_update2(qtk_avspeech_visual_voice_cfg_t *cfg,
                                          wtk_source_loader_t *sl) {
    qtk_nnrt_cfg_update2(&cfg->av_sep, sl);
    qtk_nnrt_cfg_update2(&cfg->lip_embedding, sl);
    qtk_stft_librosa_cfg_update2(&cfg->stft, sl);
    return 0;
}
