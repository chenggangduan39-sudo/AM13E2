#ifndef C26F8FEE_AE24_7B24_B09F_7CCCC239DF55
#define C26F8FEE_AE24_7B24_B09F_7CCCC239DF55

#include "wtk/core/fft/pffft.h"
#include "wtk/core/wtk_complex.h"

typedef struct qtk_cfft qtk_cfft_t;
typedef struct qtk_fft qtk_fft_t;

struct qtk_cfft {
    PFFFT_Setup *pf;
    void *work;
    float *input;
    float *output;
    int N;
};

struct qtk_fft {
    PFFFT_Setup *pf;
    void *work;
    float *input;
    float *output;
    void *kissfft;
    void *kissifft;
    void *kiss_F;
    int N;
};

qtk_cfft_t *qtk_cfft_new(int n);
void qtk_cfft_delete(qtk_cfft_t *cfft);
void qtk_cfft_fft(qtk_cfft_t *cfft, wtk_complex_t *x, wtk_complex_t *res);
void qtk_cfft_ifft(qtk_cfft_t *cfft, wtk_complex_t *x, wtk_complex_t *res);

qtk_fft_t *qtk_fft_new(int n);
void qtk_fft_delete(qtk_fft_t *fft);
void qtk_fft_fft(qtk_fft_t *fft, float *x, float *res);
void qtk_fft_ifft(qtk_fft_t *fft, float *x, float *res);

#endif /* C26F8FEE_AE24_7B24_B09F_7CCCC239DF55 */
