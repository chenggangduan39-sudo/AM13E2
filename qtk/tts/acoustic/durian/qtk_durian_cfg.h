#ifndef __QTK_DURIAN_CFG_H__
#define __QTK_DURIAN_CFG_H__

#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/wtk_array.h"
#include "qtk_durian_encoder_cfg.h"
#include "qtk_durian_duration_predictor_cfg.h"
#include "qtk_durian_decoder_cfg.h"
#include "qtk_durian_postnet_cfg.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_durian_cfg{
    char *embedding_fn;
    wtk_array_t *embedding_dim_num;
    qtk_durian_encoder_cfg_t encoder_cfg;
    qtk_durian_duration_predictor_cfg_t dp_cfg;
    qtk_durian_decoder_cfg_t decoder_cfg;
    qtk_durian_postnet_cfg_t postnet_cfg;
}qtk_durian_cfg_t;

int qtk_durian_cfg_init(qtk_durian_cfg_t *cfg);
int qtk_durian_cfg_clean(qtk_durian_cfg_t *cfg);
int qtk_durian_cfg_update_local(qtk_durian_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_durian_cfg_update(qtk_durian_cfg_t *cfg);

#ifdef __cplusplus
};
#endif

#endif