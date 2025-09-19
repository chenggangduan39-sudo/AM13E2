#include "wtk/signal/stft/qtk_stft_librosa_cfg.h"
#include "qtk/sci/qtk_window.h"

int qtk_stft_librosa_cfg_init(qtk_stft_librosa_cfg_t *cfg) {
    static wtk_string_t default_window_name = wtk_string("hann");
    cfg->window = NULL;
    cfg->pad_mode = QTK_STFT_LIBROSA_PAD_CONSTANT;
    cfg->hop_length = 0;
    cfg->win_length = 0;
    cfg->n_fft = 2048;
    cfg->window_name = &default_window_name;
    cfg->window_sq = NULL;
    cfg->window_envelop = NULL;
    return 0;
}

int qtk_stft_librosa_cfg_clean(qtk_stft_librosa_cfg_t *cfg) {
    if (cfg->window) {
        wtk_free(cfg->window);
    }
    if (cfg->window_sq) {
        wtk_free(cfg->window_sq);
    }
    if (cfg->window_envelop) {
        wtk_free(cfg->window_envelop);
    }
    return 0;
}

static void update_envelop_(qtk_stft_librosa_cfg_t *cfg, int nshift,
                            int nblock) {
    int i, j;
    float *prev_block, *cur_block, *cur_win_sq;
    int last_valid_data_len = cfg->n_fft % cfg->hop_length;
    if (last_valid_data_len == 0) {
        last_valid_data_len = cfg->hop_length;
    }
    float *last_block = cfg->window_envelop + (nblock - 1) * cfg->hop_length;
    memset(last_block, 0, sizeof(float) * cfg->hop_length);
    memcpy(last_block, cfg->window_sq + (nshift - 1) * cfg->hop_length,
           sizeof(float) * last_valid_data_len);
    if (nblock == 1) {
        return;
    }
    prev_block = cfg->window_envelop;
    memcpy(prev_block, cfg->window_sq, sizeof(float) * cfg->hop_length);
    for (i = 1; i < nshift - 1; i++, prev_block += cfg->hop_length) {
        cur_block = prev_block + cfg->hop_length;
        cur_win_sq = cfg->window_sq + i * cfg->hop_length;
        for (j = 0; j < cfg->hop_length; j++) {
            cur_block[j] = prev_block[j] + cur_win_sq[j];
        }
    }
    cur_block = prev_block + cfg->hop_length;
    for (j = 0; j < cfg->hop_length; j++) {
        cur_block[j] = prev_block[j] + last_block[j];
    }
    cur_win_sq = cfg->window_sq;
    for (i++, prev_block += cfg->hop_length; i < nblock;
         i++, prev_block += cfg->hop_length) {
        cur_block = prev_block + cfg->hop_length;
        for (j = 0; j < cfg->hop_length; j++) {
            cur_block[j] = prev_block[j] - cur_win_sq[j];
        }
        cur_win_sq += cfg->hop_length;
    }
}

int qtk_stft_librosa_cfg_update(qtk_stft_librosa_cfg_t *cfg) {
    int i;
    int nshift, nblock;
    if (cfg->win_length == 0) {
        cfg->win_length = cfg->n_fft;
    }
    if (cfg->hop_length == 0) {
        cfg->hop_length = cfg->win_length / 4;
    }
    cfg->window = wtk_calloc(cfg->n_fft, sizeof(float));
    cfg->window_sq = wtk_calloc(cfg->n_fft, sizeof(float));
    if (wtk_string_equal_s(cfg->window_name, "hann")) {
        int pad = (cfg->n_fft - cfg->win_length) / 2;
        qtk_window_hann(cfg->win_length, cfg->window + pad);
    } else {
        wtk_debug("%s Not Supported\n", cfg->window_name->data);
        goto err;
    }
    for (i = 0; i < cfg->n_fft; i++) {
        cfg->window_sq[i] = cfg->window[i] * cfg->window[i];
    }
    nshift = ceil((float)cfg->n_fft / cfg->hop_length);
    nblock = 2 * nshift - 1;
    cfg->window_envelop = wtk_malloc(sizeof(float) * cfg->hop_length * nblock);
    cfg->nshift = nshift;
    update_envelop_(cfg, nshift, nblock);
    return 0;
err:
    return -1;
}

int qtk_stft_librosa_cfg_update_local(qtk_stft_librosa_cfg_t *cfg,
                                      wtk_local_cfg_t *lc) {
    wtk_string_t *v;

    wtk_local_cfg_update_cfg_string(lc, cfg, window_name, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, hop_length, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, win_length, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, n_fft, v);
    v = wtk_local_cfg_find_string_s(lc, "pad_mode");
    if (v) {
        if (wtk_string_equal_s(v, "constant")) {
            cfg->pad_mode = QTK_STFT_LIBROSA_PAD_CONSTANT;
        } else {
            wtk_debug("%s Not Supported\n", v->data);
            goto err;
        }
    }
    return 0;
err:
    return -1;
}

int qtk_stft_librosa_cfg_update2(qtk_stft_librosa_cfg_t *cfg,
                                 wtk_source_loader_t *sl) {
    return qtk_stft_librosa_cfg_update(cfg);
}
