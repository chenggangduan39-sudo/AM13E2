#ifndef C66A835C_1D29_4C7E_8C04_633A64B6F206
#define C66A835C_1D29_4C7E_8C04_633A64B6F206

#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/wtk_complex.h"

typedef struct qtk_ult_ofdm_cfg qtk_ult_ofdm_cfg_t;

struct qtk_ult_ofdm_cfg {
    int nsymbols;
    int sampling_rate;
    float central_freq;
    int period;
    int gap_frame;
    int hann_L;

    int st;
    int et;
    float *wav;
    wtk_complex_t *wav_freq;
    float *power_spec;

    wtk_complex_t *zc_seq;
};

int qtk_ult_ofdm_cfg_init(qtk_ult_ofdm_cfg_t *cfg);
int qtk_ult_ofdm_cfg_clean(qtk_ult_ofdm_cfg_t *cfg);
int qtk_ult_ofdm_cfg_update(qtk_ult_ofdm_cfg_t *cfg);
int qtk_ult_ofdm_cfg_update_local(qtk_ult_ofdm_cfg_t *cfg, wtk_local_cfg_t *lc);

#endif /* C66A835C_1D29_4C7E_8C04_633A64B6F206 */
