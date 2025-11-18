#include "wtk/signal/qtk_fft.h"
#include "kissfft/qdm_kiss_fftr.h"
#include "wtk/core/wtk_alloc.h"

qtk_cfft_t *qtk_cfft_new(int n) {
    qtk_cfft_t *cfft = wtk_malloc(sizeof(qtk_cfft_t));
    cfft->pf = pffft_new_setup(n, PFFFT_COMPLEX);
    cfft->work = pffft_aligned_malloc(sizeof(wtk_complex_t) * n);
    cfft->input = pffft_aligned_malloc(sizeof(wtk_complex_t) * n);
    cfft->output = pffft_aligned_malloc(sizeof(wtk_complex_t) * n);
    cfft->N = n;
    return cfft;
}

void qtk_cfft_delete(qtk_cfft_t *cfft) {
    pffft_destroy_setup(cfft->pf);
    pffft_aligned_free(cfft->work);
    pffft_aligned_free(cfft->input);
    pffft_aligned_free(cfft->output);
    wtk_free(cfft);
}

void qtk_cfft_fft(qtk_cfft_t *cfft, wtk_complex_t *x, wtk_complex_t *res) {
    memcpy(cfft->input, x, sizeof(wtk_complex_t) * cfft->N);
    pffft_transform_ordered(cfft->pf, cfft->input, cfft->output,
                            cfft->work, PFFFT_FORWARD);
    memcpy(res, cfft->output, sizeof(wtk_complex_t) * cfft->N);
}

void qtk_cfft_ifft(qtk_cfft_t *cfft, wtk_complex_t *x, wtk_complex_t *res) {
    memcpy(cfft->input, x, sizeof(wtk_complex_t) * cfft->N);
    pffft_transform_ordered(cfft->pf, cfft->input, cfft->output,
                            cfft->work, PFFFT_BACKWARD);
    memcpy(res, cfft->output, sizeof(wtk_complex_t) * cfft->N);
}

qtk_fft_t *qtk_fft_new(int n) {
    qtk_fft_t *fft = wtk_malloc(sizeof(qtk_fft_t));
    if (n % 32 == 0) {
        fft->pf = pffft_new_setup(n, PFFFT_REAL);
        fft->work = pffft_aligned_malloc(sizeof(float) * n);
        fft->input = pffft_aligned_malloc(sizeof(float) * n);
        fft->output = pffft_aligned_malloc(sizeof(float) * n);
    } else if (n % 2 == 0) {
        fft->kissfft = qdm_kiss_fftr_alloc(n, 0, NULL, NULL);
        fft->kissifft = qdm_kiss_fftr_alloc(n, 1, NULL, NULL);
        fft->kiss_F = wtk_malloc(sizeof(qdm_kiss_fft_cpx) * n);
    } else {
        wtk_free(fft);
        return NULL;
    }
    fft->N = n;
    return fft;
}

void qtk_fft_delete(qtk_fft_t *fft) {
    if (fft->N % 32 == 0) {
        pffft_destroy_setup(fft->pf);
        pffft_aligned_free(fft->work);
        pffft_aligned_free(fft->input);
        pffft_aligned_free(fft->output);
    } else {
        qdm_kiss_fftr_free(fft->kissfft);
        qdm_kiss_fftr_free(fft->kissifft);
        wtk_free(fft->kiss_F);
    }
    wtk_free(fft);
}

void qtk_fft_fft(qtk_fft_t *fft, float *x, float *res) {
    if (fft->N % 32 == 0) {
        memcpy(fft->input, x, sizeof(float) * fft->N);
        pffft_transform_ordered(fft->pf, fft->input, fft->output, fft->work,
                                PFFFT_FORWARD);
        memcpy(res, fft->output, sizeof(float) * fft->N);
    } else {
        int nbin = fft->N / 2 + 1;
        qdm_kiss_fftr(fft->kissfft, x, fft->kiss_F);
        res[0] = ((qdm_kiss_fft_cpx *)fft->kiss_F)[0].r;
        res[1] = ((qdm_kiss_fft_cpx *)fft->kiss_F)[nbin - 1].r;
        memcpy(res + 2, ((qdm_kiss_fft_cpx *)fft->kiss_F) + 1,
               sizeof(qdm_kiss_fft_cpx) * (nbin - 2));
    }
}

void qtk_fft_ifft(qtk_fft_t *fft, float *x, float *res) {
    if (fft->N % 32 == 0) {
        memcpy(fft->input, x, sizeof(float) * fft->N);
        pffft_transform_ordered(fft->pf, fft->input, fft->output, fft->work,
                                PFFFT_BACKWARD);
        memcpy(res, fft->output, sizeof(float) * fft->N);
    } else {
        int nbin = fft->N / 2 + 1;
        ((qdm_kiss_fft_cpx *)fft->kiss_F)[0].r = x[0];
        ((qdm_kiss_fft_cpx *)fft->kiss_F)[0].i = 0;
        ((qdm_kiss_fft_cpx *)fft->kiss_F)[nbin - 1].r = x[1];
        ((qdm_kiss_fft_cpx *)fft->kiss_F)[nbin - 1].i = 0;
        memcpy(((qdm_kiss_fft_cpx *)fft->kiss_F) + 1, x + 2,
               sizeof(qdm_kiss_fft_cpx) * (nbin - 2));
        qdm_kiss_fftr(fft->kissifft, res, fft->kiss_F);
    }
}
