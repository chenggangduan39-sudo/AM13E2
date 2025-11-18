#include "qtk/ult/qtk_ult_msc2d.h"
#include "qtk/ult/qtk_ult_msc2d_cfg.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/wtk_sort.h"
#include "wtk/signal/wtk_umean.h"
#include "lapack/f2c.h"

qtk_ult_msc2d_t *qtk_ult_msc2d_new(qtk_ult_msc2d_cfg_t *cfg) {
    qtk_ult_msc2d_t *s = wtk_malloc(sizeof(qtk_ult_msc2d_t));
    s->cfg = cfg;
    s->eig = wtk_eig_new(s->cfg->nsensor);
    s->xcorr = wtk_malloc(sizeof(wtk_complex_t) * cfg->nsensor * cfg->nsensor);
    s->xcorr_smoothed =
        wtk_malloc(sizeof(wtk_complex_t) * cfg->nsensor * cfg->nsensor);
    s->neigm_smoothed =
        wtk_malloc(sizeof(wtk_complex_t) * cfg->nsensor * cfg->nsensor);
    if (cfg->xcorr_use_alpha) {
        s->mean = NULL;
    } else {
        s->mean = wtk_umean_new(0, cfg->lag, cfg->nsensor * cfg->nsensor);
    }
    s->rk_v = wtk_malloc(sizeof(float) * cfg->nsensor);
    s->rk_v2 = wtk_malloc(sizeof(float) * cfg->nsensor);
    s->rk_vi = wtk_malloc(sizeof(float) * cfg->nsensor);
    s->eig_v = wtk_malloc(sizeof(wtk_complex_t) * cfg->nsensor * cfg->nsensor);
    s->steer = NULL;
    s->nframe = 0;

    if (cfg->use_fft) {
        s->fft = qtk_cfft_new(cfg->nzc);
        s->frame = wtk_malloc(sizeof(wtk_complex_t) * cfg->nzc * cfg->nmic);
    } else {
        s->fft = NULL;
        s->frame = NULL;
    }

    return s;
}

void qtk_ult_msc2d_delete(qtk_ult_msc2d_t *s) {
    wtk_eig_delete(s->eig);
    if (s->mean) {
        wtk_umean_delete(s->mean);
    }
    wtk_free(s->xcorr);
    wtk_free(s->xcorr_smoothed);
    wtk_free(s->rk_v2);
    wtk_free(s->rk_v);
    wtk_free(s->rk_vi);
    wtk_free(s->eig_v);
    wtk_free(s->neigm_smoothed);
    if (s->steer) {
        wtk_free(s->steer);
    }
    if (s->cfg->use_fft) {
        qtk_cfft_delete(s->fft);
        wtk_free(s->frame);
    }
    wtk_free(s);
}

static void msc2d_update_xcorr_(qtk_ult_msc2d_t *s, wtk_complex_t *frame) {
    int nsensor = s->cfg->nsensor;
    wtk_complex_t *xcorr = s->xcorr;
    wtk_complex_t *p, *p2;
    wtk_complex_t *col;
    wtk_complex_t *row;
    int i, j, k;
    float a, b;
    int channel = s->cfg->nmic;
    int nsensor2 = nsensor / channel;
    int nzc = s->cfg->nzc;
    int len = s->cfg->nzc - nsensor2 + 1;
    // float f=1.0/len;//1.0/(nsensor*len);
    int ci, cj;
    wtk_complex_t *cur, *cur2;
    int rowi, coli;
    float f = 1.0 / (len);

    p = xcorr;
    for (ci = 0, rowi = 0; ci < channel; ++ci) {
        cur = frame + ci * nzc;
        for (i = 0; i < nsensor2; ++i, ++rowi) {
            col = cur + i;
            // p=xcorr[idx1];
            //++idx1;
            p = xcorr + nsensor * rowi;
            // p2=xcorr2+rowi;
            /// p2=xcorr2+rowi*nsensor+rowi;
            for (cj = 0, coli = 0; cj < channel; ++cj) {
                cur2 = frame + cj * nzc;
                for (j = 0; j < nsensor2; ++j, ++coli) {
                    if (coli > rowi) {
                        break;
                    }
                    row = cur2 + j;
                    a = b = 0;
                    for (k = 0; k < len; ++k) {
                        //(a+bi)*(c-di)=(a*c+b*d)+(-a*d+b*c)i;
                        a += col[k].a * row[k].a + col[k].b * row[k].b;
                        b += -col[k].a * row[k].b + col[k].b * row[k].a;
                    }
                    p->a = a * f;
                    p->b = b * f;
                    // wtk_debug("v[%d/%d]=%f+%fi\n",i,cj*nsensor2+j,a,b);
                    if (rowi != coli) {
                        p2 = xcorr + coli * nsensor + rowi;
                        p2->a = p->a;
                        p2->b = -p->b;
                    }
                    ++p;
                }
                if (coli > rowi) {
                    break;
                }
            }
            // exit(0);
        }
    }
}

static void float_alpha_update_(float *d, float *new, int len, float alpha) {
    float beta = 1 - alpha;
    for (int i = 0; i < len; i++) {
        d[i] = alpha * d[i] + beta * new[i];
    }
}

static void msc2d_update_neigm_(qtk_ult_msc2d_t *m, wtk_complex_t *xcorr2) {
    qtk_ult_msc2d_cfg_t *cfg = m->cfg;
    wtk_complex_t *r, *c;
    int nsensor = cfg->nsensor;
    // wtk_complex_t* xcorr2=m->xcorr;
    int i, j, k;
    // wtk_complex_t *p;
    // int cnt=0;
    float a, b;
    // float *fv;
    float *rkv = m->rk_v2;
    float thresh;

    // print_float(rkv,nsensor);
    memcpy(rkv, m->rk_v, sizeof(float) * nsensor);
    wtk_qsort_float(rkv, nsensor);
    thresh = rkv[nsensor - cfg->num_tgt - 1];
    // thresh=rkv[cfg->max_target];
    // wtk_debug("thresh=%f max=%d\n",thresh,cfg->max_target);
    memset(xcorr2, 0, sizeof(wtk_complex_t) * nsensor * nsensor);
    for (i = 0; i < nsensor; ++i) {
        // wtk_debug("v[%d]: %f\n",i,rkv[i]);
        if (m->rk_v[i] >= thresh) {
            continue;
        }
        // wtk_debug("v[%d]: %f\n",i,rkv[i]);
        // wtk_debug("v[%d]=%.10f\n",i,m->rk_v[i]);
        r = m->eig_v + i * nsensor;
        // wtk_debug("rkv[%d]=%.10f\n",i,m->rk_v[i]);
        // wtk_complex_print(r,nsensor);
        // neig_matrix = Rn * Rn';
        //|n*1|*|1*n|=|n*n|
        c = xcorr2;
        for (j = 0; j < nsensor; ++j) {
            a = r[j].a;
            b = r[j].b;
            for (k = j; k < nsensor; ++k) {
                //(a+bi)*(c-di)=(ac+bd)+(-ad+bc)i;
                c[k].a += a * r[k].a + b * r[k].b;
                c[k].b += -a * r[k].b + b * r[k].a;
            }
            c += nsensor;
        }
    }
    // wtk_complex_print2(xcorr2,nsensor,nsensor);
    for (j = 1; j < nsensor; ++j) {
        c = xcorr2 + j * nsensor;
        for (k = 0, i = j; k < j; ++k, i += nsensor) {
            // c[k]=xcorr2[k*nsensor+j];
            c[k].a = xcorr2[i].a;
            c[k].b = -xcorr2[i].b;
        }
    }
    // wtk_complex_print2(xcorr2,nsensor,nsensor);
}

static float msc2d_update_prob_(qtk_ult_msc2d_t *s, int l, int k) {
    int i, j;
    int nsensor = s->cfg->nsensor;
    wtk_complex_t *neigm = s->neigm_smoothed;
    wtk_complex_t *steer = s->steer + (l * s->K + k) * nsensor;
    wtk_complex_t total = {0, 0};
    for (i = 0; i < nsensor; i++) {
        wtk_complex_t sum = {0, 0};
        wtk_complex_t *c = steer + i;
        for (j = 0; j < nsensor; j++) {
            wtk_complex_t *a = steer + j;
            wtk_complex_t *b = neigm + j * nsensor + i;
            sum.a += a->a * b->a + a->b * b->b;
            sum.b += a->a * b->b - a->b * b->a;
        }
        total.a += sum.a * c->a - sum.b * c->b;
        total.b += sum.a * c->b + sum.b * c->a;
    }
    return 1.0 / total.a;
}

int qtk_ult_msc2d_feed(qtk_ult_msc2d_t *s, wtk_complex_t *input,
                       int range_idx_s, int nrange, int angle_idx_s, int nangle,
                       float *prob) {
    wtk_complex_t *xcorr;
    wtk_complex_t *frame;
    int i, j, k, l;
    int xcorr_len = s->cfg->nsensor * s->cfg->nsensor;
    if (!s->steer || !prob) {
        return -1;
    }
    s->nframe++;

    if (s->cfg->use_fft) {
        for (i = 0; i < s->cfg->nmic; i++) {
            qtk_cfft_fft(s->fft, input + i * s->cfg->nzc,
                         s->frame + i * s->cfg->nzc);
        }
        frame = s->frame;
    } else {
        frame = input;
    }

    msc2d_update_xcorr_(s, frame);
    if (s->cfg->xcorr_use_alpha) {
        if (s->nframe == 1) {
            memcpy(s->xcorr_smoothed, s->xcorr,
                   sizeof(wtk_complex_t) * xcorr_len);
        } else {
            float_alpha_update_((float *)s->xcorr_smoothed, (float *)s->xcorr,
                                xcorr_len * 2, s->cfg->xcorr_alpha);
        }
        xcorr = s->xcorr_smoothed;
    } else {
        wtk_umean_feed(s->mean, s->xcorr);
        xcorr = wtk_umean_get_mean(s->mean);
    }
    wtk_eig_process_all_eig3(s->eig, xcorr, s->eig_v, s->rk_v, s->rk_vi);
    msc2d_update_neigm_(s, s->xcorr);
    if (s->nframe == 1) {
        memcpy(s->neigm_smoothed, s->xcorr, sizeof(wtk_complex_t) * xcorr_len);
    } else {
        float_alpha_update_((float *)s->neigm_smoothed, (float *)s->xcorr,
                            xcorr_len * 2, s->cfg->xcorr_alpha);
    }
    for (i = 0, l = range_idx_s; i < nrange; ++i, ++l) {
        for (j = 0, k = angle_idx_s; j < nangle; ++j, ++k) {
            prob[i * nangle + j] = msc2d_update_prob_(s, l, k);
        }
    }
    return 0;
}

extern /* Subroutine */ int cgemm_(char *, char *, integer *, integer *,
                                   integer *, complex *, complex *, integer *,
                                   complex *, integer *, complex *, complex *,
                                   integer *);

static void ult_msc2d_update_steer_(qtk_ult_msc2d_t *s, float fc, float dis,
                                    float theta, float *freq, int subsamples,
                                    wtk_complex_t *steer) {
    wtk_complex_t steer_angle[64];
    char T = 'T';
    int mm, nn, kk;
    int i;
    wtk_complex_t steer_dis[512];
    float wavpath, phase;
    int nmic = s->cfg->nmic;

    for (i = 0; i < nmic; i++) {
        wavpath = s->cfg->mic[i * 2] * sin(theta) +
                  s->cfg->mic[i * 2 + 1] * cos(theta);
        phase = -1 * 2 * PI * wavpath * fc / 340;
        steer_angle[i].a = cos(phase);
        steer_angle[i].b = sin(phase);
    }

    for (i = 0; i < subsamples; i++) {
        phase = -1 * 2 * PI * freq[i] * dis / 340;
        steer_dis[i].a = cos(phase);
        steer_dis[i].b = sin(phase);
    }

    mm = subsamples;
    kk = 1;
    nn = nmic;
    wtk_complex_t alpha = {1, 0};
    wtk_complex_t beta = {0, 0};
    cgemm_(&T, &T, &mm, &nn, &kk, (complex *)&alpha, (complex *)steer_dis, &kk,
           (complex *)steer_angle, &nn, (complex *)&beta, (complex *)steer,
           &mm);
}

int qtk_ult_msc2d_update_steer(qtk_ult_msc2d_t *s, float df, float fc,
                               float angle_s, float angle_e, float angle_unit,
                               float range_s, float range_e, float range_unit) {
    float freq[512];
    int i, j;
    int L, K;
    int subsamples = s->cfg->nsensor / s->cfg->nmic;
    wtk_complex_t *steer;
    if (subsamples >= sizeof(freq) / sizeof(freq[0])) {
        return -1;
    }
    if (s->steer) {
        wtk_free(s->steer);
    }
    K = (angle_e - angle_s) / angle_unit + 1;
    L = (range_e - range_s) / range_unit + 1;
    s->steer = wtk_malloc(sizeof(wtk_complex_t) * K * L * s->cfg->nsensor);
    for (i = 0; i < subsamples; i++) {
        freq[i] = i * df;
    }
    for (i = 0; i < L; i++) {
        for (j = 0; j < K; j++) {
            float dis = range_s + i * range_unit;
            float theta = angle_s + j * angle_unit;
            steer = s->steer + (i * K + j) * subsamples * s->cfg->nmic;
            ult_msc2d_update_steer_(s, fc, dis, theta, freq, subsamples, steer);
        }
    }
    s->K = K;
    s->L = L;
    return 0;
}
