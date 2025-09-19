#ifndef G_1AE6378DD67E445BAA1704B863D2F15D
#define G_1AE6378DD67E445BAA1704B863D2F15D

#include "wtk/core/wtk_complex.h"
#include "wtk/signal/qtk_fft.h"
#include "wtk/signal/stft/qtk_stft.h"
#include "wtk/signal/stft/qtk_stft_librosa_cfg.h"

typedef struct qtk_stft_librosa qtk_stft_librosa_t;

struct qtk_stft_librosa {
    qtk_stft_librosa_cfg_t *cfg;
    qtk_fft_t *fft;
    float *frame;
    float *synthesis_sample;
    float *x;
    float *X;
    wtk_complex_t *F;
    int frame_pos;
    qtk_stft_notifier_t notifier;
    void *upval;
    int skip_samples;
    uint32_t backward_frame_idx;
};

qtk_stft_librosa_t *qtk_stft_librosa_new(qtk_stft_librosa_cfg_t *cfg,
                                         void *upval,
                                         qtk_stft_notifier_t notifier);
void qtk_stft_librosa_delete(qtk_stft_librosa_t *lr);
int qtk_stft_librosa_forward(qtk_stft_librosa_t *lr, float *wav, int len);
int qtk_stft_librosa_forward_end(qtk_stft_librosa_t *lr);
int qtk_stft_librosa_backward(qtk_stft_librosa_t *lr, wtk_complex_t *X);
int qtk_stft_librosa_backward_end(qtk_stft_librosa_t *lr);
int qtk_stft_librosa_reset(qtk_stft_librosa_t *lr);
void qtk_stft_librosa_set_notifier(qtk_stft_librosa_t *lr, void *upval, qtk_stft_notifier_t notifier);

#endif
