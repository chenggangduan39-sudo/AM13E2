#include "qtk/ult/qtk_ult_ofdm.h"
#include "wtk/core/fft/wtk_rfft.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/fft/wtk_cfft.h"

qtk_ult_ofdm_t *qtk_ult_ofdm_new(qtk_ult_ofdm_cfg_t *cfg) {
    qtk_ult_ofdm_t *ofdm;

    ofdm = wtk_malloc(sizeof(qtk_ult_ofdm_t));
    ofdm->cfg = cfg;
    ofdm->L = cfg->gap_frame * cfg->period;
    ofdm->F = wtk_calloc(sizeof(float), ofdm->L);
    ofdm->fft = wtk_rfft_new(ofdm->L / 2);
    ofdm->cfft = wtk_cfft_new(cfg->nsymbols * cfg->gap_frame);
    ofdm->cfr =
        wtk_malloc(sizeof(wtk_complex_t) * cfg->nsymbols * cfg->gap_frame);
    ofdm->cir =
        wtk_calloc(sizeof(wtk_complex_t), cfg->nsymbols * cfg->gap_frame);
    return ofdm;
}

static void mask_cfr_(qtk_ult_ofdm_t *ofdm, float dis_start, float dis_end) {
    float sr = 48000;
    wtk_complex_t *cir, *cfr;
    wtk_complex_t *frame = ofdm->cfr;
    int N = ofdm->cfg->nsymbols * ofdm->cfg->gap_frame;

    float period_gap = N;
    float freq_unit = (sr / ofdm->cfg->period) / ofdm->cfg->gap_frame;

    int start_idx = (int)(dis_start * freq_unit * period_gap / 343.0) + 1;
    int end_idx = (int)(dis_end * freq_unit * period_gap / 343.0);

    cir = wtk_cfft_ifft(ofdm->cfft, frame);

    memcpy(ofdm->cir + start_idx, cir + start_idx,
           sizeof(wtk_complex_t) * (end_idx - start_idx));

    cfr = wtk_cfft_fft(ofdm->cfft, ofdm->cir);
    memcpy(frame, cfr, sizeof(wtk_complex_t) * N);
}

int qtk_ult_ofdm_estimate_cfr2(qtk_ult_ofdm_t *ofdm, float *wav,
                               wtk_complex_t **cfr) {
    wtk_rfft_process_fft(ofdm->fft, ofdm->F, wav);
    for (int i = ofdm->cfg->st; i < ofdm->cfg->et; i++) {
        int j = i - ofdm->cfg->st;
        float fa = ofdm->F[i];
        float fb = -ofdm->F[i + ofdm->fft->win];
        ofdm->cfr[j].a =
            (fa * ofdm->cfg->wav_freq[j].a + fb * ofdm->cfg->wav_freq[j].b) /
            ofdm->cfg->power_spec[j];
        ofdm->cfr[j].b =
            (-fa * ofdm->cfg->wav_freq[j].b + fb * ofdm->cfg->wav_freq[j].a) /
            ofdm->cfg->power_spec[j];
    }
    *cfr = ofdm->cfr;
    return 0;
}

int qtk_ult_ofdm_estimate_cfr(qtk_ult_ofdm_t *ofdm, float *wav, float dis_start,
                              float dis_end, wtk_complex_t **cfr,
                              wtk_complex_t **cir) {
    qtk_ult_ofdm_estimate_cfr2(ofdm, wav, cfr);
    mask_cfr_(ofdm, dis_start, dis_end);
    *cfr = ofdm->cfr;
    *cir = ofdm->cir;
    return 0;
}

void qtk_ult_ofdm_delete(qtk_ult_ofdm_t *ofdm) {
    wtk_free(ofdm->F);
    wtk_free(ofdm->cfr);
    wtk_free(ofdm->cir);
    wtk_rfft_delete(ofdm->fft);
    wtk_cfft_delete(ofdm->cfft);
    wtk_free(ofdm);
}
