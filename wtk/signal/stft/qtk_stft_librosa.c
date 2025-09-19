#include "wtk/signal/stft/qtk_stft_librosa.h"
#include "qtk/math/qtk_vector.h"

qtk_stft_librosa_t *qtk_stft_librosa_new(qtk_stft_librosa_cfg_t *cfg,
                                         void *upval,
                                         qtk_stft_notifier_t notifier) {
    qtk_stft_librosa_t *lr = wtk_malloc(sizeof(qtk_stft_librosa_t));
    lr->cfg = cfg;
    lr->frame = wtk_malloc(sizeof(float) * cfg->n_fft);
    lr->x = wtk_malloc(sizeof(float) * cfg->n_fft);
    lr->X = wtk_malloc(sizeof(float) * cfg->n_fft);
    lr->F = wtk_malloc(sizeof(wtk_complex_t) * (cfg->n_fft / 2 + 1));
    lr->synthesis_sample = wtk_malloc(sizeof(float) * cfg->n_fft);
    lr->fft = qtk_fft_new(cfg->n_fft);
    qtk_stft_librosa_set_notifier(lr, upval, notifier);
    if (qtk_stft_librosa_reset(lr) != 0) {
        goto err;
    }
    return lr;
err:
    return NULL;
}

void qtk_stft_librosa_set_notifier(qtk_stft_librosa_t *lr, void *upval, qtk_stft_notifier_t notifier) {
    lr->notifier = notifier;
    lr->upval = upval;
}

int qtk_stft_librosa_reset(qtk_stft_librosa_t *lr) {
    int pad_sz = lr->cfg->n_fft / 2;
    memset(lr->synthesis_sample, 0, sizeof(float) * lr->cfg->n_fft);
    if (lr->cfg->pad_mode == QTK_STFT_LIBROSA_PAD_CONSTANT) {
        memset(lr->frame, 0, sizeof(float) * pad_sz);
        lr->frame_pos = pad_sz;
        lr->skip_samples = pad_sz;
    } else {
        wtk_debug("Not Impl\n");
        goto err;
    }
    lr->backward_frame_idx = 0;
    return 0;
err:
    return -1;
}

void qtk_stft_librosa_delete(qtk_stft_librosa_t *lr) {
    wtk_free(lr->frame);
    wtk_free(lr->x);
    wtk_free(lr->X);
    wtk_free(lr->F);
    wtk_free(lr->synthesis_sample);
    qtk_fft_delete(lr->fft);
    wtk_free(lr);
}

static void forward_frame_(qtk_stft_librosa_t *lr) {
    int nbin = lr->cfg->n_fft / 2 + 1;
    qtk_vector_multipy_elewise(lr->frame, lr->cfg->window, lr->x,
                               lr->cfg->n_fft);
    qtk_fft_fft(lr->fft, lr->x, lr->X);
    lr->F[0].a = lr->X[0];
    lr->F[0].b = 0;
    lr->F[nbin - 1].a = lr->X[1];
    lr->F[nbin - 1].b = 0;
    memcpy(lr->F + 1, lr->X + 2, sizeof(wtk_complex_t) * (nbin - 2));
    lr->notifier(lr->upval, lr->F, QTK_STFT_FORWARD);
}

int qtk_stft_librosa_forward(qtk_stft_librosa_t *lr, float *wav, int len) {
    int cpyed = lr->cfg->n_fft - lr->cfg->hop_length;
    while (len > 0) {
        int need_len = lr->cfg->n_fft - lr->frame_pos;
        int cosume_len = min(need_len, len);
        memcpy(lr->frame + lr->frame_pos, wav, sizeof(float) * cosume_len);
        lr->frame_pos += cosume_len;
        len -= cosume_len;
        wav += cosume_len;
        if (lr->frame_pos == lr->cfg->n_fft) {
            forward_frame_(lr);
            memmove(lr->frame, lr->frame + lr->cfg->hop_length,
                    sizeof(float) * cpyed);
            lr->frame_pos = cpyed;
        }
    }
    return 0;
}

int qtk_stft_librosa_forward_end(qtk_stft_librosa_t *lr) {
    int pad_sz = lr->cfg->n_fft / 2;
    float pad[512] = {0};
    while (pad_sz > 0) {
        int feed_len = min(pad_sz, sizeof(pad) / sizeof(pad[0]));
        qtk_stft_librosa_forward(lr, pad, feed_len);
        pad_sz -= feed_len;
    }
    return 0;
}

int qtk_stft_librosa_backward(qtk_stft_librosa_t *lr, wtk_complex_t *X) {
    qtk_stft_synthesis_sample_t synthesis_sample;
    int i;
    float envelope;
    uint32_t full_idx = lr->cfg->hop_length * lr->cfg->nshift;
    int N = lr->cfg->n_fft;
    int nbin = N / 2 + 1;
    lr->X[0] = X[0].a;
    lr->X[1] = X[nbin - 1].a;
    memcpy(lr->X + 2, X + 1, sizeof(wtk_complex_t) * (nbin - 2));
    qtk_fft_ifft(lr->fft, lr->X, lr->x);
    for (i = 0; i < N; i++) {
        lr->x[i] /= N;
    }
    qtk_vector_multipy_elewise(lr->x, lr->cfg->window, lr->x, N);
    for (i = 0; i < N; i++) {
        uint32_t sample_idx = lr->backward_frame_idx * N + i;
        if (sample_idx <= full_idx) {
            envelope = lr->cfg->window_envelop[sample_idx];
        } else {
            envelope =
                lr->cfg->window_envelop[lr->cfg->hop_length *
                                            (lr->cfg->nshift - 1) +
                                        sample_idx % lr->cfg->hop_length];
        }
        lr->x[i] /= envelope + 1e-10;
    }
    qtk_vector_add_elewise(lr->synthesis_sample, lr->x, lr->synthesis_sample,
                           N);
    if (lr->skip_samples > 0) {
        int consumed = min(lr->skip_samples, lr->cfg->hop_length);
        lr->skip_samples -= consumed;
        if (lr->skip_samples == 0 && lr->cfg->hop_length > consumed) {
            synthesis_sample.data = lr->synthesis_sample + consumed;
            synthesis_sample.len = lr->cfg->hop_length - consumed;
            lr->notifier(lr->upval, &synthesis_sample, QTK_STFT_BACKWARD);
        }
    } else {
        synthesis_sample.data = lr->synthesis_sample;
        synthesis_sample.len = lr->cfg->hop_length;
        lr->notifier(lr->upval, &synthesis_sample, QTK_STFT_BACKWARD);
    }
    memmove(lr->synthesis_sample, lr->synthesis_sample + lr->cfg->hop_length,
            sizeof(float) * (N - lr->cfg->hop_length));
    memset(lr->synthesis_sample + N - lr->cfg->hop_length, 0,
           sizeof(float) * lr->cfg->hop_length);
    lr->backward_frame_idx++;
    return 0;
}

int qtk_stft_librosa_backward_end(qtk_stft_librosa_t *lr) {
    int skip_sz = lr->cfg->n_fft / 2;
    int raise_sz = lr->cfg->n_fft - lr->cfg->hop_length - skip_sz;
    if (raise_sz <= 0) {
        return 0;
    }
    qtk_stft_synthesis_sample_t synthesis_sample;
    synthesis_sample.data = lr->synthesis_sample;
    synthesis_sample.len = raise_sz;
    lr->notifier(lr->upval, &synthesis_sample, QTK_STFT_BACKWARD);
    return 0;
}
