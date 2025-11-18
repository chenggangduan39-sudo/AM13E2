#ifndef G_FEEBD151C1474D26B815D5CAD01680F4
#define G_FEEBD151C1474D26B815D5CAD01680F4
#include "wtk/core/cfg/wtk_local_cfg.h"

typedef struct qtk_stft_librosa_cfg qtk_stft_librosa_cfg_t;

typedef enum {
    QTK_STFT_LIBROSA_PAD_CONSTANT,
} qtk_stft_librosa_pad_mode_t;

struct qtk_stft_librosa_cfg {
    qtk_stft_librosa_pad_mode_t pad_mode;
    int hop_length;
    int win_length;
    int n_fft;
    wtk_string_t *window_name;

    float *window;
    float *window_sq;
    float *window_envelop;
    int nshift;
    unsigned center : 1;
};

int qtk_stft_librosa_cfg_init(qtk_stft_librosa_cfg_t *cfg);
int qtk_stft_librosa_cfg_clean(qtk_stft_librosa_cfg_t *cfg);
int qtk_stft_librosa_cfg_update(qtk_stft_librosa_cfg_t *cfg);
int qtk_stft_librosa_cfg_update_local(qtk_stft_librosa_cfg_t *cfg,
                                      wtk_local_cfg_t *lc);
int qtk_stft_librosa_cfg_update2(qtk_stft_librosa_cfg_t *cfg,
                                 wtk_source_loader_t *sl);

#endif
