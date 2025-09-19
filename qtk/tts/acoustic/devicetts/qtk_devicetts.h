#ifndef __QTK_DEVICETTS_H__
#define __QTK_DEVICETTS_H__

#include "qtk_devicetts_cfg.h"
#include "wtk/core/math/wtk_mat.h"
#include "parse/qtk_tts_parse.h"
#include "dfsmn/qtk_tts_dfsmn.h"
#include "acoustic/devicetts/qtk_devicetts_duration_predictor.h"
#include "qtk_devicetts_decoder.h"
#include "qtk_devicetts_postnet.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef int (*qtk_devicetts_notify_f)(void *user_data,wtk_matf_t *mel, int is_end);


typedef struct{
	float* mean_vector;
	float* std_vector;
	int mgc_order;
	int lf0_order;
	int bap_order;
	int feature_dim;
	float* mgc_mean_vector;
	float* lf0_mean_vector;
	float* bap_mean_vector;
	float* mgc_std_vector;
	float* lf0_std_vector;
	float* bap_std_vector;
}qtk_devicetts_normalizer_t;

qtk_devicetts_normalizer_t* qtk_devicetts_normalizer_new(char* mean_path, char* std_path, int mgc_dim, int lf0_dim, int bap_dim);
int qtk_devicetts_normalizer_denormal(float* mean, float* std, wtk_matf_t* feat);
void qtk_devicetts_normalizer_delete(qtk_devicetts_normalizer_t* normalizer);

typedef struct{
    qtk_devicetts_cfg_t *cfg;
    wtk_heap_t *heap;
    
    wtk_matf_t *embedding_table;
    qtk_tts_dfsmn_t *encoder;
    qtk_devicetts_duration_predictor_t *dur;
    qtk_devicetts_decoder_t *dec;
    qtk_devicetts_postnet_t *postnet;
    qtk_devicetts_normalizer_t *normalizer;
    void *user_data;
    qtk_devicetts_notify_f notify;
}qtk_devicetts_t;

qtk_devicetts_t* qtk_devicetts_new_lpcnet(qtk_devicetts_cfg_t *cfg);
qtk_devicetts_t* qtk_devicetts_new_world(qtk_devicetts_cfg_t *cfg);
int qtk_devicetts_process_lpcnet(qtk_devicetts_t *dev,wtk_veci_t *token, int is_end);
int qtk_devicetts_process_world(qtk_devicetts_t *dev,wtk_veci_t *token, int is_end);
int qtk_devicetts_delete(qtk_devicetts_t *);
int qtk_devicetts_reset(qtk_devicetts_t *dev);
int qtk_devicetts_set_notify(qtk_devicetts_t *dev, qtk_devicetts_notify_f, void*);

#ifdef __cplusplus
};
#endif

#endif
