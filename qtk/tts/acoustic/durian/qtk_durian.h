#ifndef __QTK_DURIAN_H__
#define __QTK_DURIAN_H__

//mode of durian

#include "qtk_durian_cfg.h"
#include "wtk/core/math/wtk_mat.h"
#include "qtk/layer/qtk_nn_embedding.h"
#include "qtk_durian_encoder.h"
#include "qtk_durian_duration_predictor.h"
#include "qtk_durian_decoder.h"
#include "qtk_durian_postnet.h"
#ifdef __cplusplus
extern "C"{
#endif

typedef int (*qtk_durian_notify_f)(void *user_data,wtk_matf_t *mel,int is_end);

typedef struct{
    qtk_durian_cfg_t *cfg;
    wtk_heap_t *heap;

    qtk_nn_embedding_t *embedding;
    qtk_durian_encoder_t *encoder;
    qtk_durian_duration_predictor_t *dp;
    qtk_durian_decoder_t *decoder;
    qtk_durian_postnet_t *postnet;
    
    void *user_data;
    qtk_durian_notify_f notify;
}qtk_durian_t;

qtk_durian_t* qtk_durian_new(qtk_durian_cfg_t *cfg);
int qtk_durian_delete(qtk_durian_t *durian);
int qtk_durian_process(qtk_durian_t *durian,wtk_veci_t *tokens,int is_end);
int qtk_durian_reset(qtk_durian_t *dev);
int qtk_durian_set_notify(qtk_durian_t *dev, qtk_durian_notify_f, void*);


#ifdef __cplusplus
};
#endif

#endif