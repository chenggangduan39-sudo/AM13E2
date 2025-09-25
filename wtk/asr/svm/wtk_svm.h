#ifndef WTK_EVAL_SVM_WTK_SVM_H_
#define WTK_EVAL_SVM_WTK_SVM_H_
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/wtk_str.h"
#include "wtk/core/wtk_heap.h"
#include "svm.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_svm wtk_svm_t;
struct wtk_svm
{
	struct svm_model* model;
	wtk_heap_t *heap;
};

wtk_svm_t* wtk_svm_new();
int wtk_svm_delete(wtk_svm_t *s);
int wtk_svm_load(wtk_svm_t *svm,wtk_source_t *s);
#ifdef __cplusplus
};
#endif
#endif
