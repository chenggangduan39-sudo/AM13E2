#ifndef __QTK_DEVICETTS_CFG_H__
#define __QTK_DEVICETTS_CFG_H__

#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/wtk_array.h"
#include "parse/qtk_tts_parse_cfg.h"
#include "dfsmn/qtk_tts_dfsmn_cfg.h"
#include "acoustic/devicetts/qtk_devicetts_duration_predictor_cfg.h"
#include "qtk_devicetts_decoder_cfg.h"
#include "qtk_devicetts_postnet_cfg.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct{
    wtk_array_t *embedding_dim_num; 
    char *embedding_fn;

    qtk_tts_parse_cfg_t parse_cfg;
    qtk_tts_dfsmn_cfg_t encoder;    //dfsmn encoder
    qtk_devicetts_duration_predictor_cfg_t dur;
    qtk_devicetts_decoder_cfg_t dec;
    qtk_devicetts_postnet_cfg_t postnet;
    char *mean_fn;
    char *std_fn;
}qtk_devicetts_cfg_t;


int qtk_devicetts_cfg_init(qtk_devicetts_cfg_t *cfg);
int qtk_devicetts_cfg_clean(qtk_devicetts_cfg_t *cfg);
int qtk_devicetts_cfg_update_local(qtk_devicetts_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_devicetts_cfg_update(qtk_devicetts_cfg_t *cfg);
int qtk_devicetts_cfg_update2(qtk_devicetts_cfg_t *cfg, wtk_source_loader_t *sl);


#ifdef __cplusplus
};
#endif

#endif
