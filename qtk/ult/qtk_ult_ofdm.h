#ifndef A06C7F5A_5548_459C_9B8E_D102726FFDE7
#define A06C7F5A_5548_459C_9B8E_D102726FFDE7

#include "qtk/ult/qtk_ult_ofdm_cfg.h"
#include "wtk/core/fft/wtk_rfft.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/fft/wtk_cfft.h"

typedef struct qtk_ult_ofdm qtk_ult_ofdm_t;

struct qtk_ult_ofdm {
    qtk_ult_ofdm_cfg_t *cfg;
    wtk_rfft_t *fft;
    wtk_cfft_t *cfft;
    wtk_complex_t *cfr;
    wtk_complex_t *cir;
    float *F;
    int L;
};

qtk_ult_ofdm_t *qtk_ult_ofdm_new(qtk_ult_ofdm_cfg_t *cfg);
int qtk_ult_ofdm_estimate_cfr(qtk_ult_ofdm_t *ofdm, float *wav, float dis_start,
                              float dis_end, wtk_complex_t **cfr,
                              wtk_complex_t **cir);
int qtk_ult_ofdm_estimate_cfr2(qtk_ult_ofdm_t *ofdm, float *wav,
                               wtk_complex_t **cfr);
void qtk_ult_ofdm_delete(qtk_ult_ofdm_t *ofdm);

#endif /* A06C7F5A_5548_459C_9B8E_D102726FFDE7 */
