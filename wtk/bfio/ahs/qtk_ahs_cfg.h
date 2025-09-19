#ifndef WTK_BFIO_AHS_AHS_CFG_H
#define WTK_BFIO_AHS_AHS_CFG_H
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "qtk/nnrt/qtk_nnrt_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/bfio/qform/wtk_qmmse_cfg.h"
#include "wtk/bfio/qform/wtk_bf.h"
#include "wtk/bfio/qform/wtk_covm.h"
#include "wtk/bfio/eq/wtk_equalizer.h"
#include "wtk/bfio/ahs/qtk_freq_shift_cfg.h"
#include "wtk/bfio/ahs/qtk_kalman_cfg.h"
#include "wtk/bfio/ahs/qtk_erb_cfg.h"
#include "wtk/bfio/agc/qtk_gain_controller_cfg.h"
#include "wtk/bfio/masknet/wtk_bbonenet_cfg.h"
#include "wtk/asr/vad/kvad/wtk_kvad_cfg.h"
typedef struct qtk_ahs_cfg qtk_ahs_cfg_t;

struct qtk_ahs_cfg {
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    qtk_nnrt_cfg_t nnrt;

    wtk_qmmse_cfg_t qmmse;
    wtk_qmmse_cfg_t qmmse2;
    wtk_covm_cfg_t covm;
    wtk_bf_cfg_t bf;
    wtk_equalizer_cfg_t eq;
    qtk_ahs_freq_shift_cfg_t freq_shift;
    qtk_ahs_kalman_cfg_t km;
    qtk_ahs_kalman_cfg_t km2;
    qtk_ahs_erb_cfg_t erb;
    wtk_bbonenet_cfg_t gainnet;
    qtk_ahs_gain_controller_cfg_t gain_controller;
    wtk_kvad_cfg_t kvad;

    int rate;
    int hop_sz;
    int chunk_sz;
    int window_sz;
    int pad_sz;
    int feat_dim;
    int nbin;
    int type;
    float *window;
    float *synthesis_window;
    float n_delay;
    float loop_delay;
    float pre_scale;
    float scale;
    float gain;
    float rir_gain;
    float wiener_thresh;
    int kalman_type;

    char *eq_gain_fn;

    int c0_idx;
    int h0_idx;

    int clip_s;
    int clip_e;

    unsigned use_out_qmmse:1;
    unsigned use_in_qmmse:1;
    unsigned use_loop:1;
    unsigned use_nnrt:1;
    unsigned use_nnrt2:1;
    unsigned use_lstm:1;
    unsigned use_clip:1;
    unsigned use_mask:1;
    unsigned use_mask_sp:1;
    unsigned use_fftsbf:1;
    unsigned use_sp_check:1;
    unsigned use_eq:1;
    unsigned use_eq2:1;
    unsigned use_mt:1;
    unsigned use_freq_shift:1;
    unsigned use_gainnet:1;
    unsigned use_nnvad:1;
    unsigned is_eq2_kalmanout:1;
};

int qtk_ahs_cfg_init(qtk_ahs_cfg_t *cfg);
int qtk_ahs_cfg_clean(qtk_ahs_cfg_t *cfg);
int qtk_ahs_cfg_update(qtk_ahs_cfg_t *cfg);
int qtk_ahs_cfg_update_local(qtk_ahs_cfg_t *cfg, wtk_local_cfg_t *lc);
int qtk_ahs_cfg_update2(qtk_ahs_cfg_t *cfg, wtk_source_loader_t *sl);

qtk_ahs_cfg_t* qtk_ahs_cfg_new(char *fn);
void qtk_ahs_cfg_delete(qtk_ahs_cfg_t *cfg);
qtk_ahs_cfg_t* qtk_ahs_cfg_new_bin(char *fn);
void qtk_ahs_cfg_delete_bin(qtk_ahs_cfg_t *cfg);

void qtk_ahs_cfg_set(qtk_ahs_cfg_t *cfg, float pre_scale, float n_delay);
#endif
