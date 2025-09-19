/*
 * this is a wrapper for svmlight
 */
#ifndef WTK_SVMLIGHT_H_
#define WTK_SVMLIGHT_H_

#include "svm_common.h"
#include "wtk/core/cfg/wtk_source.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef MODEL wtk_svmlight_model_t;

/* some as struct WORD in svm_common.h */
typedef struct
{
    FNUM index; /* start: 1, end: 0 */
    FVAL value;
} wtk_svmlight_node_t;

/* load svm model from source/file/string */
wtk_svmlight_model_t* wtk_svmlight_model_new(wtk_source_t *source, int dim);
wtk_svmlight_model_t* wtk_svmlight_model_new2(const char *fn,      int dim);
wtk_svmlight_model_t* wtk_svmlight_model_new3(const char *data,    int dim);

int wtk_svmlight_model_delete(wtk_svmlight_model_t *model);

void wtk_svmlight_nodes_init(wtk_svmlight_node_t *nodes, int dim);

/* svm predict */
float wtk_svmlight_predict(wtk_svmlight_model_t *model, const wtk_svmlight_node_t *nodes);

#ifdef __cplusplus
}
#endif
#endif
