#ifndef WTK_BFIO_SOUNDFIELD_SYNTHEIS_CFG
#define WTK_BFIO_SOUNDFIELD_SYNTHEIS_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/math/wtk_mat.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_soundfield_syntheis_cfg qtk_soundfield_syntheis_cfg_t;
struct qtk_soundfield_syntheis_cfg
{
    float **positions;//测量点
    int n_pos;

    float *reference_position;
    float *source_position;
    float *lambda_vector;
    float *freq_weight;

    char *lp_fn;
    char *hp_fn;
    wtk_vecf_t *lp;
    wtk_vecf_t *hp;
    wtk_vecf_t *tmp;

    char *weight_fn;
    wtk_strbuf_t *weight_buf;

    int rir_channel;
    int cutoff_freq;
    int hop_size;
    int fs;
    int N;
    int nref;
    int neval;
    int n_tap;
    int ref_spk_index;
    int nbin;
    float rt60;
    float lambda;
    float scale;
    unsigned bandsplit_on:1;
};

int qtk_soundfield_syntheis_cfg_init(qtk_soundfield_syntheis_cfg_t *cfg);
int qtk_soundfield_syntheis_cfg_clean(qtk_soundfield_syntheis_cfg_t *cfg);
int qtk_soundfield_syntheis_cfg_update_local(qtk_soundfield_syntheis_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_soundfield_syntheis_cfg_update(qtk_soundfield_syntheis_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif