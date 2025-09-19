/*
 * qtk_nltkpos.h
 *
 *  Created on: Mar 4, 2022
 *      Author: dm
 */

#ifndef WTK_UTILS_POS_QTK_NLTKPOS_H_
#define WTK_UTILS_POS_QTK_NLTKPOS_H_
#include "wtk/core/wtk_strbuf.h"
#include "wtk/core/wtk_str_hash.h"
#include "wtk/core/wtk_fkv.h"
#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_nltkpos_w{
    char *k;
    float v;
}qtk_nltkpos_w_t;

typedef struct qtk_nltkpos_weight{
    char *k;
    int w_len;
    qtk_nltkpos_w_t weight[40];   //now max 38
}qtk_nltkpos_weight_t;

typedef struct qtk_nltkpos_tagdict{
    char *k;
    char *v;
}qtk_nltkpos_tagdict_t;

typedef struct{
    wtk_string_t *k;
    int v;
}qtk_nltkpos_features_t;

typedef struct qtk_nltkpos{
    wtk_heap_t *heap;
    wtk_str_hash_t *tagdict;
    // wtk_str_hash_t *weights;
    wtk_fkv_t *weight_kv;
    char **context;
    char **pos;
    int context_len;
    qtk_nltkpos_features_t *features; //max 14 features
    int features_len;
}qtk_nltkpos_t;

qtk_nltkpos_t * qtk_nltkpos_new(char *path,wtk_heap_t *heap);
int qtk_nltkpos_delete(qtk_nltkpos_t *pos);
int qtk_nltkpos_process(qtk_nltkpos_t *pos,char **seq_word,int word_len);
int qtk_nltkpos_features_print(qtk_nltkpos_t *pos);

int qtk_nltkpos_getidx(wtk_string_t *cls);
int qtk_nltkpos_getidx2(char *cls, int len);
int qtk_pos_getidx(wtk_string_t *cls);
#ifdef __cplusplus
};
#endif
#endif /* WTK_UTILS_POS_QTK_NLTKPOS_H_ */
