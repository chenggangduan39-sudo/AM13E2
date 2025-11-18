#include "qtk/sci/clustering/qtk_clustering_spectral.h"
#include "wtk/core/wtk_alloc.h"
#include "qtk/core/qtk_min_heap.h"
#include "qtk/linalg/qtk_gemm.h"
#include "qtk/math/qtk_math.h"
#include "qtk/math/qtk_matrix.h"
#include "qtk/math/qtk_vector.h"
#include "qtk/sci/clustering/kmeans.h"
#include "qtk/sci/clustering/qtk_kmeans_sink.h"

extern int ssyevd_(char *jobz, char *uplo, int *n, float *a, int *lda, float *w,
                   float *work, int *lwork, int *iwork, int *liwork, int *info);

qtk_clustering_spectral_t *qtk_clustering_spectral_new(int N, int emb_len,
                                                       int ncluster) {
    qtk_clustering_spectral_t *spec;
    spec = wtk_malloc(sizeof(qtk_clustering_spectral_t));
    qtk_clustering_spectral_init(spec, N, emb_len, ncluster);

    return spec;
}

void qtk_clustering_spectral_delete(qtk_clustering_spectral_t *spec) {
    qtk_clustering_spectral_clean(spec);
    wtk_free(spec);
}

int qtk_clustering_spectral_init(qtk_clustering_spectral_t *spec, int N,
                                 int emb_len, int ncluster) {
    int mat_sz;
    int emb_sz;

    if (N < 0) {
        N = 32;
    }

    mat_sz = (N - 1) * (N) / 2;
    emb_sz = emb_len * N;
    spec->w = wtk_strbuf_new(sizeof(float) * mat_sz, 1);
    spec->buf = wtk_strbuf_new(sizeof(float) * emb_sz, 1);
    spec->emb_len = emb_len;
    spec->pval = 0.1;
    spec->N = 0;
    spec->ncluster = ncluster;
    spec->ncluster_min = 1;
    spec->ncluster_max = 100;
    spec->workspace = wtk_heap_new(4096);

    return 0;
}

void qtk_clustering_spectral_clean(qtk_clustering_spectral_t *spec) {
    wtk_strbuf_delete(spec->w);
    wtk_strbuf_delete(spec->buf);
    wtk_heap_delete(spec->workspace);
}

int qtk_clustering_spectral_add(qtk_clustering_spectral_t *spec, float *emb) {
    int emb_len_c;

    emb_len_c = sizeof(float) * spec->emb_len;
    wtk_strbuf_push(spec->w, cast(const char *, emb), emb_len_c);
    spec->N++;
    return 0;
}

static void _construct_sim_mat(qtk_clustering_spectral_t *spec) {
    int i; //,j;
    float *emb_cursor;//*sim_mat, 
    int M, N, K;

    wtk_strbuf_reset(spec->buf);
    wtk_strbuf_expand(spec->buf, spec->N * spec->N * sizeof(float));
    for (i = 0, emb_cursor = cast(float *, spec->w->data); i < spec->N;
         i++, emb_cursor += spec->emb_len) {
        float normf = qtk_vector_normf(emb_cursor, spec->emb_len);
        qtk_vector_scale(emb_cursor, emb_cursor, spec->emb_len, 1.0 / normf);
    }
    M = spec->N;
    K = spec->emb_len;
    N = spec->N;

    memset(spec->buf->data, 0, sizeof(float) * M * N);

    qtk_sgemm(0, 1, M, N, K, 1.0, cast(float *, spec->w->data), K,
              cast(float *, spec->w->data), K, 0.0,
              cast(float *, spec->buf->data), N);

    // sim_mat = cast(float *, spec->buf->data);
    // for (i = 0; i < spec->N; i++) {
    //     for (j = 0; j < spec->N; j++, sim_mat++) {
    //         *sim_mat = *sim_mat;//0.5 * (1.0 + *sim_mat);
    //         //printf("[%d][%d] = %f\n",i,j,*sim_mat);
    //     }
    // }
}

typedef struct {
    float sim;
    int idx;
} _sim_elem_t;

static int _sim_elem_cmp(_sim_elem_t *e1, _sim_elem_t *e2) {
    if (e1->sim > e2->sim) {
        return 1;
    }
    if (e1->sim < e2->sim) {
        return -1;
    }
    return 0;
}

static void p_pruning_(qtk_clustering_spectral_t *spec) {
    float *sim_mat_row, *sim_mat_elem;
    int stride = spec->N;
    int prune_elems;
    int i, j;
    int min_heap_dlen = spec->w->length;
    qtk_min_heap_t *mheap;
    _sim_elem_t min_sim;

    prune_elems =
        spec->N >= 1000 ? (1 - spec->pval) * spec->N : max(spec->N - 10, 2);
    sim_mat_row = cast(float *, spec->buf->data);

    while ((mheap = qtk_min_heap_init(
                spec->w->data, &min_heap_dlen, sizeof(_sim_elem_t), spec->N,
                cast(qtk_min_heap_cmp_f, _sim_elem_cmp))) == NULL) {
        wtk_strbuf_expand(spec->w, min_heap_dlen);
        min_heap_dlen = spec->w->length;
    }

    for (i = 0; i < spec->N; i++, sim_mat_row += stride) {
        for (j = 0; j < spec->N; j++) {
            sim_mat_elem = sim_mat_row + j;
            qtk_min_heap_push(mheap, &(_sim_elem_t){*sim_mat_elem, j});
        }
        for (j = 0; j < prune_elems; j++) {
            qtk_min_heap_pop(mheap, &min_sim);
            sim_mat_row[min_sim.idx] = 0;
        }
        // for (j = prune_elems; j < spec->N; j++) {
        //     qtk_min_heap_pop(mheap, &min_sim);
        //     //sim_mat_row[min_sim.idx] = 1.0;
        // }
        qtk_min_heap_reset(mheap);
    }

    sim_mat_row = cast(float *, spec->buf->data);
    for (i = 0; i < spec->N; i++) {
        for (j = 0; j < spec->N; j++) {
            if (j > i) {
                sim_mat_row[i * spec->N + j] += sim_mat_row[j * spec->N + i];
                sim_mat_row[i * spec->N + j] *= 0.5;
            } else {
                sim_mat_row[i * spec->N + j] = sim_mat_row[j * spec->N + i];
            }
        }
    }
}

static float *_laplacian(qtk_clustering_spectral_t *spec) {
    float *D, *sim_mat;
    int i, j;

    wtk_strbuf_expand(spec->w, sizeof(float) * spec->N);
    D = cast(float *, spec->w->data);
    sim_mat = cast(float *, spec->buf->data);
    for (i = 0; i < spec->N; i++, sim_mat += spec->N) {
        D[i] = 0;
        for (j = 0; j < spec->N; j++) {
            if (i == j) {
                sim_mat[j] = 0;
            }
            D[i] += qtk_fabs(sim_mat[j]);
        }
    }

    sim_mat = cast(float *, spec->buf->data);

    for (i = 0; i < spec->N; i++, sim_mat += spec->N) {
        for (j = 0; j < spec->N; j++) {
            if (i == j) {
                sim_mat[j] = D[i] - sim_mat[j];
            } else {
                sim_mat[j] = -sim_mat[j];
            }
        }
    }

    return cast(float *, spec->buf->data);
}

static void _init_kmeans(kmeans_config *config) {
    int i, j;
    int dim = *cast(int *, config->upval);
    int nk = config->k;
    int sinked = 0;

    int candidate = 0;
    float min_dist_per_obj = FLT_MAX;
    float max_dist = 0;
    double dist;

    memcpy(config->centers[0], config->objs[0], sizeof(float) * dim);
    sinked++;

    while (sinked < nk) {
        max_dist = 0;
        for (i = 0; i < config->num_objs; i++) {
            min_dist_per_obj = FLT_MAX;
            for (j = 0; j < sinked; j++) {
                dist = config->distance_method(config->upval, config->objs[i],
                                               config->centers[j]);
                if (dist < min_dist_per_obj) {
                    min_dist_per_obj = dist;
                }
            }
            if (min_dist_per_obj > max_dist) {
                max_dist = min_dist_per_obj;
                candidate = i;
            }
        }
        memcpy(config->centers[sinked], config->objs[candidate],
               sizeof(float) * dim);
        sinked++;
    }
}

static int *_perform_kmeans(qtk_clustering_spectral_t *spec, float *eig_vecs,
                            int ncluster) {
    int i,j;
    kmeans_config config;
    kmeans_result result;
    Pointer *objs =
        wtk_heap_malloc(spec->workspace,
                        sizeof(Pointer) * spec->N); // spec->N * sizeof(Pointer)
    Pointer *centers = wtk_heap_malloc(
        spec->workspace,
        sizeof(Pointer) * ncluster); // ncluster * sizeof(Pointer)
    float *centers_val =
        wtk_heap_malloc(spec->workspace, sizeof(float) * ncluster * ncluster);
    float *centers_val_cursor;
    int *clusters = wtk_heap_malloc(
        spec->workspace, sizeof(int) * spec->N); // spec->N * sizeof(int)

    for (i = 0, centers_val_cursor = centers_val; i < ncluster;
         i++, centers_val_cursor += ncluster) {
        centers[i] = centers_val_cursor;
    }
    for (i = 0; i < spec->N; i++) {
        for(j = 0; j < spec->ncluster;j++){
            *(eig_vecs + i * spec->N + j) = - *(eig_vecs + i * spec->N + j);
        }
        objs[i] = eig_vecs + i * spec->N;
    }
    memset(clusters, 0, sizeof(int) * spec->N);

    memset(&config, 0, sizeof(config));
    config.k = ncluster;
    config.num_objs = spec->N;
    config.max_iterations = 300;
    config.objs = objs;
    config.centers = centers;
    config.clusters = clusters;

    config.distance_method =
        cast(kmeans_distance_method, qtk_kmeans_vecf_distance);
    config.centroid_method =
        cast(kmeans_centroid_method, qtk_kmeans_vecf_centroid);

    config.upval = &ncluster;

    _init_kmeans(&config);

    result = kmeans(&config);

    if (result == KMEANS_OK) {
        return clusters;
    }

    return NULL;
}

static int _detect_ncluster(qtk_clustering_spectral_t *spec, float *eig_val) {
    int i;
    float max_gap = FLT_MIN, gap;
    int max_idx = 1;
    int k;

    k = min(spec->N - 1, spec->ncluster_max + 1);
    for (i = 1; i < k; i++) {
        gap = eig_val[i + 1] - eig_val[i];
        if (gap > max_gap) {
            max_gap = gap;
            max_idx = i;
        }
    }

    return max_idx + 1;
}

int *qtk_clustering_spectral_get_result(qtk_clustering_spectral_t *spec) {
    float *L;
    int *lablels;
    int ncluster;
    _construct_sim_mat(spec);
    p_pruning_(spec);
    L = _laplacian(spec);

    {
        int info;
        char jobz = 'V';
        char uplo = 'U';
        int lwork = 1 + 6 * spec->N + 2 * spec->N * spec->N;
        int liwork = 3 + 5 * spec->N;

        float *work;
        int *iwork;
        float *eig_val;

        int n = spec->N;
        int lda = spec->N;

        wtk_strbuf_reset(spec->w);
        wtk_strbuf_expand(spec->w, sizeof(float) * lwork +
                                       sizeof(int) * liwork +
                                       sizeof(float) * spec->N);
        work = cast(float *, spec->w->data);
        iwork = cast(int *, work + lwork);
        eig_val = cast(float *, iwork + liwork);

        ssyevd_(&jobz, &uplo, &n, L, &lda, eig_val, work, &lwork, iwork,
                &liwork, &info);
        qtk_assert(info == 0);

        qtk_matrix_square_transpose_inplace(L, spec->N);

        ncluster = spec->ncluster <= 0 ? _detect_ncluster(spec, eig_val)
                                       : spec->ncluster;
        if (ncluster < spec->ncluster_min) {
            ncluster = spec->ncluster_min;
        }

        lablels = _perform_kmeans(spec, L, ncluster);
    }

    return lablels;
}

void qtk_clustering_spectral_reset(qtk_clustering_spectral_t *spec) {
    spec->N = 0;
    wtk_strbuf_reset(spec->w);
    wtk_strbuf_reset(spec->buf);
    wtk_heap_reset(spec->workspace);
}
