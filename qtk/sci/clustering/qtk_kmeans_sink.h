#ifndef __SCI_CLUSTERING_QBL_KMEANS_SINK_H__
#define __SCI_CLUSTERING_QBL_KMEANS_SINK_H__
#pragma once
#include "qtk/core/qtk_type.h"
#ifdef __cplusplus
extern "C" {
#endif

double qtk_kmeans_vecd_distance(int *len, const double *a, const double *b);
void qtk_kmeans_vecd_centroid(int *len, const double **objs,
                              const int *clusters, size_t num_objs, int cluster,
                              double *centers);
double qtk_kmeans_vecf_distance(int *len, const float *a, const float *b);
void qtk_kmeans_vecf_centroid(int *len, const float **objs, const int *clusters,
                              size_t num_objs, int cluster, float *centers);

#ifdef __cplusplus
};
#endif
#endif
