/*
 * qtk_dtree.h
 *
 *  Created on: Feb 26, 2022
 *      Author: dm
 */
#include "wtk/core/wtk_queue.h"

#ifndef WTK_UTILS_QTK_DTREE_H_
#define WTK_UTILS_QTK_DTREE_H_
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_dtree qtk_dtree_t;
typedef struct qtk_dtree_node qtk_dtree_node_t;
struct qtk_dtree{
    qtk_dtree_node_t *nodes;
    float *values;
    qtk_dtree_node_t *root;
    int nnode;
    int ncls;
};

struct qtk_dtree_node{
	wtk_queue_node_t q_n;
	short feat_idx;
	float thre;
	//float sample;
	float *value;
	int cls;
	int idx;
	qtk_dtree_node_t *r;
	qtk_dtree_node_t *l;
};
qtk_dtree_t* qtk_dtree_new();
qtk_dtree_t* qtk_dtree_build(qtk_dtree_t* dt, short* feats, unsigned short* samples, float* thres, float *values, int nclass, int nnode);
int qtk_dtree_isleaf(qtk_dtree_node_t* node);
void qtk_dtree_delete(qtk_dtree_t* dt);

int qtk_dtree_getclass(qtk_dtree_t* dt, float* input);
void qtk_dtree_print(qtk_dtree_t *dt);
void qtk_dtree_print_layer(qtk_dtree_t *dt);

#ifdef __cplusplus
};
#endif
#endif /* WTK_UTILS_QTK_DTREE_H_ */
