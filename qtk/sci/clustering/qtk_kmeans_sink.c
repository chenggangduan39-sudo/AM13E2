#include "qtk/sci/clustering/qtk_kmeans_sink.h"

double qtk_kmeans_vecd_distance(int *len, const double *a, const double *b) {
    double dist = 0;
    int i;

    for (i = 0; i < *len; i++) {
        dist += (a[i] - b[i]) * (a[i] - b[i]);
    }

    return dist;
}

void qtk_kmeans_vecd_centroid(int *len, const double **objs,
                              const int *clusters, size_t num_objs, int cluster,
                              double *centers) {
    int i, j;
    int nmemb = 0;
    memset(centers, 0, sizeof(double) * *len);

    if (num_objs <= 0) {
        return;
    }

    for (i = 0; i < num_objs; i++) {
        if (clusters[i] == cluster) {
            for (j = 0; j < *len; j++) {
                centers[j] += objs[i][j];
            }
            nmemb++;
        }
    }

    if (nmemb) {
        for (i = 0; i < *len; i++) {
            centers[i] /= nmemb;
        }
    }
}

double qtk_kmeans_vecf_distance(int *len, const float *a, const float *b) {
    double dist = 0;
    int i;

    for (i = 0; i < *len; i++) {
        dist += (a[i] - b[i]) * (a[i] - b[i]);
    }

    return dist;
}

void qtk_kmeans_vecf_centroid(int *len, const float **objs, const int *clusters,
                              size_t num_objs, int cluster, float *centers) {
    int i, j;
    int nmemb = 0;
    memset(centers, 0, sizeof(float) * *len);

    if (num_objs <= 0) {
        return;
    }

    for (i = 0; i < num_objs; i++) {
        if (clusters[i] == cluster) {
            for (j = 0; j < *len; j++) {
                centers[j] += objs[i][j];
            }
            nmemb++;
        }
    }

    if (nmemb) {
        for (i = 0; i < *len; i++) {
            centers[i] /= nmemb;
        }
    }
}
