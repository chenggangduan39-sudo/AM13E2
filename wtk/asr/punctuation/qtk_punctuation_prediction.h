#ifndef QTK_PUNCTUATION_PREDICTION_H_
#define QTK_PUNCTUATION_PREDICTION_H_

#include "qtk_punctuation_prediction_cfg.h"
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/math/wtk_math.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_punctuation_prediction qtk_punctuation_prediction_t;

typedef enum{
    QTK_PP_NORM,
    QTK_PP_COMMA,
    QTK_PP_PERIOD,
    QTK_PP_QMARK,
    QTK_PP_PAUSE
}qtk_punctuation_type_t;

struct qtk_punctuation_prediction{
    qtk_punctuation_prediction_cfg_t *cfg;
#ifdef ONNX_DEC
    qtk_onnxruntime_t *model;
#endif
    wtk_strbuf_t *buf;
    wtk_strbuf_t *res_buf;
    wtk_strbuf_t *id;//int64
    wtk_strbuf_t *token_type;//int64
};

qtk_punctuation_prediction_t* qtk_punctuation_prediction_new(qtk_punctuation_prediction_cfg_t* cfg);
int qtk_punctuation_prediction_start(qtk_punctuation_prediction_t* wrapper);
int qtk_punctuation_prediction_feed(qtk_punctuation_prediction_t* wrapper,char *data,int bytes);
void qtk_punctuation_prediction_reset(qtk_punctuation_prediction_t* wrapper);
void qtk_punctuation_prediction_delete(qtk_punctuation_prediction_t* wrapper);
void qtk_punctuation_prediction_get_result(qtk_punctuation_prediction_t *wrapper,wtk_string_t *v);

#ifdef __cplusplus
};
#endif
#endif

