#ifndef __QTK_SVPRINTC_CFG_H__
#define __QTK_SVPRINTC_CFG_H__

#include "wtk/asr/fextra/nnet3/wtk_nnet3_xvector_compute_cfg.h"
#include "wtk/asr/vad/kvad/wtk_kvad_cfg.h"
#include "wtk/asr/fextra/ivector/wtk_ivector_plda_scoring_cfg.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_svprintc_cfg{
    wtk_nnet3_xvector_compute_cfg_t xvector;
    wtk_ivector_plda_scoring_cfg_t plda;
    wtk_kvad_cfg_t kvad;

    char *svprint_nn_fn;
    char *pool_fn;
    unsigned use_plda:1;
}qtk_svprintc_cfg_t;

int qtk_svprintc_cfg_init(qtk_svprintc_cfg_t *cfg);
int qtk_svprintc_cfg_update_local(qtk_svprintc_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_svprintc_cfg_update(qtk_svprintc_cfg_t *cfg);
int qtk_svprintc_cfg_clean(qtk_svprintc_cfg_t *cfg);

#ifdef __cplusplus
};
#endif


#endif