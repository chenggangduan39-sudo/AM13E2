/*
 * this is wrapper for libsvm
 * 'norm' is short for 'normalize', used for normalize 'nodes'
 */
#ifndef WTK_LIBSVM_H_
#define WTK_LIBSVM_H_

#include "wtk_svm.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef wtk_svm_t wtk_libsvm_model_t;
typedef struct svm_node  wtk_libsvm_node_t;

typedef struct
{
    int index;  /* start: 1, end: -1 */
    float min;
    float max;
} wtk_libsvm_norm_t;

/* load svm model from source/file/string */
wtk_libsvm_model_t* wtk_libsvm_model_new(wtk_source_t *source, int dim);
wtk_libsvm_model_t* wtk_libsvm_model_new2(const char *fn,      int dim);
wtk_libsvm_model_t* wtk_libsvm_model_new3(const char *data,    int dim);
wtk_libsvm_model_t* wtk_libsvm_model_new4(wtk_string_t *str, int dim, wtk_source_loader_t *sl);

int wtk_libsvm_model_delete(wtk_libsvm_model_t *model);

/* load static norlization parameters from source/file/string */
wtk_libsvm_norm_t*  wtk_libsvm_norm_new(wtk_source_t *source, int dim);
wtk_libsvm_norm_t*  wtk_libsvm_norm_new2(const char *fn,      int dim);
wtk_libsvm_norm_t*  wtk_libsvm_norm_new3(const char *data,    int dim);
wtk_libsvm_norm_t* wtk_libsvm_norm_new4(wtk_string_t *str, int dim, wtk_source_loader_t *sl);

int wtk_libsvm_norm_delete(wtk_libsvm_norm_t *norm);

void wtk_libsvm_nodes_init(wtk_libsvm_node_t *nodes, int dim);

/* normalize value of nodes by static normalization parameters */
void wtk_libsvm_nodes_norm(wtk_libsvm_node_t *nodes, const wtk_libsvm_norm_t *norm);

/* svm predict */
float wtk_libsvm_predict(wtk_libsvm_model_t *model, const wtk_libsvm_node_t *nodes);

#ifdef __cplusplus
}
#endif
#endif
