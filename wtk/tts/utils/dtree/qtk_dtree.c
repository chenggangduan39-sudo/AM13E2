/*
 * qtk_dtree.c
 *
 *  Created on: Feb 26, 2022
 *      Author: dm
 */
#include "wtk/core/wtk_alloc.h"
#include "qtk_dtree.h"

qtk_dtree_t* qtk_dtree_new()
{
	qtk_dtree_t* dt;

	dt=wtk_malloc(sizeof(*dt));
	dt->nodes=NULL;
	dt->nnode=0;
	dt->ncls = 0;
	dt->values = NULL;

	return dt;
}

int qtk_dtree_node_isleaf(qtk_dtree_node_t* node)
{
  return node->feat_idx==-2? 1: 0;
}

void qtk_dtree_build_preorder(qtk_dtree_t* dt, short* feats, float* thres, int* idx)
{
	qtk_dtree_node_t* n;

	n=&(dt->nodes[*idx]);
	if (!qtk_dtree_node_isleaf(n))
	{
		//left node
		(*idx)++;
		n->l = &(dt->nodes[*idx]);
		n->l->idx = *idx;
		n->l->feat_idx = feats[*idx];
		n->l->thre = thres[*idx];
		n->l->value = &(dt->values[*idx * dt->ncls]);
		n->l->cls=n->l->value[0] > n->l->value[1]?0:1;
		n->l->l = n->l->r=NULL;
		qtk_dtree_build_preorder(dt, feats, thres, idx);
		//right node
		(*idx)++;
		n->r = &(dt->nodes[*idx]);
		n->r->idx = *idx;
		n->r->feat_idx = feats[*idx];
		n->r->thre = thres[*idx];
		n->r->value = &(dt->values[*idx * dt->ncls]);
		n->r->l = n->r->r=NULL;
		n->r->cls=n->r->value[0]>n->r->value[1]?0:1;
		qtk_dtree_build_preorder(dt, feats, thres, idx);
	}
}

/**
 * 默认是先序存储
 */
qtk_dtree_t* qtk_dtree_build(qtk_dtree_t* dt, short* feats, unsigned short* samples, float* thres, float *values, int ncls, int nnode)
{
	int idx;

	dt->nnode=nnode;
	dt->ncls=ncls;
	dt->nodes=wtk_malloc(sizeof(qtk_dtree_node_t) * dt->nnode);
	dt->values=wtk_malloc(sizeof(float)* dt->ncls * dt->nnode);
	memcpy(dt->values, values, sizeof(float)* dt->ncls * dt->nnode);
	idx=0;
	dt->root = &(dt->nodes[idx]);
	dt->root->idx = idx;
	dt->root->feat_idx = feats[idx];
	dt->root->thre = thres[idx];
	dt->root->value = &(dt->values[idx*dt->ncls]);
	dt->root->cls = dt->root->value[0]>dt->root->value[1]?0:1;
	dt->root->l = dt->root->r=NULL;
	qtk_dtree_build_preorder(dt, feats, thres, &idx);

	return dt;
}

int qtk_dtree_node_valid(float v)
{
	return 1;
}
/**
 * Note: if current node feat_idx value for input is invalid, it return current node.
 */
qtk_dtree_node_t* qtk_dtree_findnode(qtk_dtree_node_t* n, float* input)
{
	if (qtk_dtree_node_isleaf(n))
	{
		//printf("input[%d]=leaf thre=%f\n", n->feat_idx, n->thre);
		return n;
	}else
	{
		if (qtk_dtree_node_valid(input[n->feat_idx]))
		{
			//printf("input[%d]=%f thre=%f\n", n->feat_idx,input[n->feat_idx], n->thre);
			if (input[n->feat_idx] < n->thre)
				return qtk_dtree_findnode(n->l, input);
			else
				return qtk_dtree_findnode(n->r, input);
		}else
			return n;
	}
}

int qtk_dtree_getclass(qtk_dtree_t* dt, float* input)
{
	qtk_dtree_node_t *n;

	n = qtk_dtree_findnode(dt->root, input);
	//printf("cls=%d values=[%f, %f]\n", n->cls, n->value[0], n->value[1]);
	return n->cls;
}
void qtk_dtree_delete(qtk_dtree_t* dt)
{
	wtk_free(dt->nodes);
	wtk_free(dt->values);
	wtk_free(dt);
}

void qtk_dtree_print_layer(qtk_dtree_t *dt)
{
	wtk_queue_t q;
	wtk_queue_node_t *qn;
	qtk_dtree_node_t *n;

	wtk_queue_init(&q);
	wtk_queue_push(&q, &(dt->root->q_n));

	printf("feats: [");
	while(1)
	{
		qn=wtk_queue_pop(&q);
		if(!qn){break;}
		n=data_offset2(qn,qtk_dtree_node_t,q_n);
		if (n)
			printf(" %d/%f/%d", n->feat_idx, n->thre, n->idx);
		if (n->l)
			wtk_queue_push(&q, &(n->l->q_n));
		if (n->r)
			wtk_queue_push(&q, &(n->r->q_n));
	}
	printf("]\n");
}
void qtk_dtree_print_preorder(qtk_dtree_node_t *n)
{
	printf(" %d/%f/%d", n->feat_idx, n->thre, n->idx);
	if (n->l)
		qtk_dtree_print_preorder(n->l);
	if(n->r)
		qtk_dtree_print_preorder(n->r);
}

void qtk_dtree_print(qtk_dtree_t *dt)
{
	printf("feats: [");
	qtk_dtree_print_preorder(dt->root);
	printf("]\n");
}
