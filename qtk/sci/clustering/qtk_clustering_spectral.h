#ifndef __QBL_SCI_CLUSTERING_QBL_CLUSTERING_SPECTRAL_H__
#define __QBL_SCI_CLUSTERING_QBL_CLUSTERING_SPECTRAL_H__
#pragma once
#include "wtk/core/wtk_heap.h"
#include "wtk/core/wtk_strbuf.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_clustering_spectral qtk_clustering_spectral_t;

struct qtk_clustering_spectral {
    wtk_strbuf_t *w;
    wtk_strbuf_t *buf;
    float pval;
    int emb_len;
    int N;
    int ncluster;
    int ncluster_min;
    int ncluster_max;
    wtk_heap_t *workspace;
};

qtk_clustering_spectral_t *qtk_clustering_spectral_new(int N, int emb_len,
                                                       int ncluster);
void qtk_clustering_spectral_delete(qtk_clustering_spectral_t *spec);
int qtk_clustering_spectral_add(qtk_clustering_spectral_t *spec, float *emb);
int *qtk_clustering_spectral_get_result(qtk_clustering_spectral_t *spec);
int qtk_clustering_spectral_init(qtk_clustering_spectral_t *spec, int N,
                                 int emb_len, int ncluster);
void qtk_clustering_spectral_clean(qtk_clustering_spectral_t *spec);
void qtk_clustering_spectral_reset(qtk_clustering_spectral_t *spec);

#ifdef __cplusplus
};
#endif
#endif
