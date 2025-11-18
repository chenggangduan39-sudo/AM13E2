#include "qtk/cv/tracking/qtk_objtrack_KCF.h"
#include "qtk/core/qtk_cyclearray.h"
#include "qtk/image/qtk_image_color.h"
#include "qtk/image/qtk_image_feature.h"
#include "qtk/math/qtk_math.h"
#include "qtk/math/qtk_vector.h"
#include "qtk/sci/qtk_window.h"
#include "wtk/core/wtk_alloc.h"

#include "qdm_kiss_fftndr.h"

extern int sgemm_(char *, char *, int *, int *, int *, float *, float *, int *,
                  float *, int *, float *, float *, int *);

extern int sgesdd_(char *jobz, int *m, int *n, float *a, int *lda, float *s,
                   float *u, int *ldu, float *vt, int *ldvt, float *work,
                   int *lwork, int *iwork, int *info);

typedef struct objtrack_KCF_impl_ objtrack_KCF_impl_t;

struct objtrack_KCF_impl_ {
    qtk_objtrack_KCF_cfg_t *cfg;
    qtk_image_roi_f_t roi;
    float output_sigma;
    float *win;
    float *y;
    int nfreq;

    int npca_dim;
    int pca_dim;

    float *k;
    qdm_kiss_fft_cpx *kf;
    qdm_kiss_fft_cpx *spec;
    qdm_kiss_fft_cpx *spec2;
    qdm_kiss_fft_cpx *kf_lambda;
    qdm_kiss_fft_cpx *new_alphaf;
    qdm_kiss_fft_cpx *new_alphaf_den;
    qdm_kiss_fft_cpx *alphaf;
    qdm_kiss_fft_cpx *alphaf_den;
    qdm_kiss_fft_cpx **vxf;
    qdm_kiss_fft_cpx **vyf;
    qdm_kiss_fft_cpx **vxyf;
    qdm_kiss_fft_cpx *xyf_data;
    float *xy_data;
    float *response;

    qtk_cyclearray_t *pn_hist;
    float peak_hist_sum;
    float neighbor_hist_sum;

    qtk_cyclearray_t *motion_vector_hist;
    float motion_vector_sum[2];
    float motion_vector[2];

    float *x;
    float *z;

    float *feature_pca; // storage for the extracted feature
    float *feature_npca;

    float *Z1; // storage for KRLS model
    float *Z0;

    float *Zc1; // storage for KRLS compress model
    float *Zc0;

    float *old_cov;
    float *new_cov;
    float *proj_matrix;

    // for SVD
    float *U;
    float *SIGMA;
    float *VT;
    float *WORK;
    int LWORK;
    int *IWORK;

    int nfeatures_pca;
    int nfeatures_npca;
    qdm_kiss_fft_cpx *yf;
    qdm_kiss_fftndr_cfg fft2_cfg;
    qdm_kiss_fftndr_cfg ifft2_cfg;
    void *tmp_buf;
    void *img_patch;
    int frame_idx;
    qtk_image_desc_t img_desc;

    unsigned resize : 1;
};

static int _KCF_init_tmpbuf(objtrack_KCF_impl_t *kcf) {
    int max_tmp_sz = sizeof(float) * kcf->roi.width * kcf->roi.height * 10;

    kcf->tmp_buf = wtk_malloc(max_tmp_sz);
    return 0;
}

static int _KCF_create_gaussian_response(objtrack_KCF_impl_t *kcf) {
    int row = kcf->roi.height;
    int col = kcf->roi.width;
    int dims[2] = {row, col};
    float *dst;

    kcf->fft2_cfg = qdm_kiss_fftndr_alloc(dims, 2, 0, NULL, NULL);
    kcf->ifft2_cfg = qdm_kiss_fftndr_alloc(dims, 2, 1, NULL, NULL);
    kcf->y = wtk_malloc(sizeof(float) * row * col);
    kcf->yf = wtk_malloc(sizeof(qdm_kiss_fft_cpx) * row * (col / 2 + 1));
    dst = kcf->y;
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < col; j++) {
            float x = i - kcf->roi.height / 2 + 1;
            float y = j - kcf->roi.width / 2 + 1;
            *dst++ = qtk_expf((x * x + y * y) * kcf->output_sigma);
        }
    }
    qdm_kiss_fftndr(kcf->fft2_cfg, kcf->y, kcf->yf);

    return 0;
}

static int _KCF_init_features(objtrack_KCF_impl_t *kcf, int height, int width) {
    kcf->nfeatures_npca = 0;
    kcf->nfeatures_pca = 0;

    if (kcf->cfg->desc_npca & TRACKING_GRAY) {
        kcf->nfeatures_npca++;
        kcf->npca_dim = width * height;
    }
    if (kcf->cfg->desc_npca & TRACKING_CN) {
        kcf->nfeatures_npca++;
        kcf->npca_dim = width * height * 10;
    }

    kcf->feature_npca = wtk_malloc(sizeof(float) * kcf->npca_dim);
    kcf->Z1 = wtk_malloc(sizeof(float) * kcf->npca_dim);
    kcf->Zc1 = wtk_malloc(sizeof(float) * kcf->npca_dim);

    // TODO: support custom feature extractor

    if (kcf->cfg->desc_pca & TRACKING_GRAY) {
        kcf->nfeatures_pca++;
        kcf->pca_dim = width * height;
    }
    if (kcf->cfg->desc_pca & TRACKING_CN) {
        kcf->nfeatures_pca++;
        kcf->pca_dim = width * height * 10;
    }

    kcf->feature_pca = wtk_malloc(sizeof(float) * kcf->pca_dim);
    kcf->Z0 = wtk_malloc(sizeof(float) * kcf->pca_dim);
    kcf->Zc0 =
        wtk_malloc(sizeof(float) * width * height * kcf->cfg->compress_size);

    kcf->x = wtk_malloc(sizeof(float) * height * width *
                        (1 + kcf->cfg->compress_size));
    kcf->z = wtk_malloc(sizeof(float) * height * width *
                        (1 + kcf->cfg->compress_size));

    // TODO: support feature compose
    qtk_assert(kcf->nfeatures_npca == 1 && kcf->nfeatures_pca == 1);

    return 0;
}

static qtk_image_roi_t _KCF_get_img_patch(objtrack_KCF_impl_t *kcf,
                                          uint8_t *image,
                                          qtk_image_desc_t *desc) {
    qtk_image_roi_f_t region = kcf->roi;
    int top_pad, bottom_pad, left_pad, right_pad;
    qtk_image_roi_f_t *_roi = &kcf->roi;

    if (_roi->x < 0) {
        region.x = 0;
        region.width += _roi->x;
    }
    if (_roi->y < 0) {
        region.y = 0;
        region.height += _roi->y;
    }
    if (_roi->x + _roi->width > kcf->img_desc.width) {
        region.width = kcf->img_desc.width - _roi->x;
    }
    if (_roi->y + _roi->height > kcf->img_desc.height) {
        region.height = kcf->img_desc.height - _roi->y;
    }
    if (region.width > kcf->img_desc.width) {
        region.width = kcf->img_desc.width;
    }
    if (region.height > kcf->img_desc.height) {
        region.height = kcf->img_desc.height;
    }

    top_pad = region.y - _roi->y;
    bottom_pad = _roi->height + _roi->y > kcf->img_desc.height
                     ? _roi->height + _roi->y - kcf->img_desc.height
                     : 0;
    left_pad = region.x - _roi->x;
    right_pad = _roi->width + _roi->x > kcf->img_desc.width
                    ? _roi->width + _roi->x - kcf->img_desc.width
                    : 0;

    qtk_image_roi_t region_i = {
        .x = qtk_roundf(region.x),
        .y = qtk_roundf(region.y),
        .width = qtk_roundf(region.width),
        .height = qtk_roundf(region.height),
    };

    qtk_image_sub_u8_with_border(desc, &region_i, kcf->img_patch, image,
                                 top_pad, bottom_pad, left_pad, right_pad,
                                 QBL_IMAGE_BORDER_ZERO);

    return region_i;
}

static int _kcf_get_LWORK(objtrack_KCF_impl_t *kcf) {
    int M;
    int N;

    M = N = 10;
    return 3 * min(M, N) * min(M, N) +
           max(max(M, N), 4 * min(M, N) * min(M, N) + 4 * min(M, N));
}

void qtk_objtrack_KCF_set_roi(qtk_objtrack_KCF_t kcf_wrapper,
                              qtk_image_roi_t *roi) {
    objtrack_KCF_impl_t *kcf = cast(objtrack_KCF_impl_t *, kcf_wrapper);
    kcf->output_sigma =
        qtk_sqrtf(cast(float, roi->width) * cast(float, roi->height)) *
        kcf->cfg->output_sigma_fact;
    kcf->output_sigma = -0.5 / (kcf->output_sigma * kcf->output_sigma);

    kcf->roi.x = roi->x;
    kcf->roi.y = roi->y;
    kcf->roi.width = roi->width;
    kcf->roi.height = roi->height;

    if (kcf->cfg->resize &&
        roi->width * roi->height > kcf->cfg->max_patch_size) {
        kcf->resize = 1;
        kcf->roi.x /= 2.0;
        kcf->roi.y /= 2.0;
        kcf->roi.width /= 2.0;
        kcf->roi.height /= 2.0;
    } else {
        kcf->resize = 0;
    }

    // TODO support resize
    qtk_assert(kcf->resize == 0);

    kcf->roi.x -= kcf->roi.width * kcf->cfg->expand_fact;
    kcf->roi.y -= kcf->roi.height * kcf->cfg->expand_fact;
    kcf->roi.height += 2 * kcf->roi.height * kcf->cfg->expand_fact;
    kcf->roi.width += 2 * kcf->roi.width * kcf->cfg->expand_fact;

    // make even
    kcf->roi.height = qtk_roundf(kcf->roi.height) & 0xFFFFFFFE;
    kcf->roi.width = qtk_roundf(kcf->roi.width) & 0xFFFFFFFE;
}

static int qtk_objtrack_KCF_init(objtrack_KCF_impl_t *kcf,
                                 qtk_objtrack_KCF_cfg_t *cfg, uint8_t *image,
                                 qtk_image_desc_t *img_desc,
                                 qtk_image_roi_t *roi) {
    kcf->cfg = cfg;

    qtk_objtrack_KCF_set_roi(kcf, roi);

    kcf->frame_idx = 0;
    kcf->img_desc = *img_desc;
    kcf->img_patch = wtk_malloc(sizeof(uint8_t) * kcf->roi.height *
                                kcf->roi.width * img_desc->channel);

    kcf->nfreq = kcf->roi.height * (kcf->roi.width / 2 + 1);

    _KCF_init_tmpbuf(kcf);

    kcf->win = wtk_malloc(sizeof(float) * kcf->roi.width * kcf->roi.height);
    qtk_window_hanning_2d(kcf->win, kcf->roi.height, kcf->roi.width,
                          kcf->tmp_buf);

    _KCF_create_gaussian_response(kcf);

    if (img_desc->channel == 1) { // disable CN for grayscale image
        cfg->desc_pca &= ~(TRACKING_CN);
        cfg->desc_npca &= ~(TRACKING_CN);
    }

    _KCF_init_features(kcf, kcf->roi.width, kcf->roi.height);

    // FIXME
    kcf->new_cov = wtk_calloc(sizeof(float), 10 * 10);
    kcf->old_cov = wtk_calloc(sizeof(float), 10 * 10);
    kcf->proj_matrix = wtk_malloc(sizeof(float) * 10 * cfg->compress_size);

    kcf->U = wtk_malloc(sizeof(float) * 10 * 10);
    kcf->SIGMA = wtk_malloc(sizeof(float) * 10);
    kcf->VT = wtk_malloc(sizeof(float) * 10 * 10);
    kcf->LWORK = _kcf_get_LWORK(kcf);
    kcf->WORK = wtk_malloc(sizeof(float) * kcf->LWORK);
    kcf->IWORK = wtk_malloc(sizeof(int) * 8 * 10);

    {
        int i;
        int width = kcf->roi.width;
        int height = kcf->roi.height;
        int nfeat_chan = 1 + kcf->cfg->compress_size;
        kcf->vxf = wtk_malloc(sizeof(qdm_kiss_fft_cpx *) * nfeat_chan);
        kcf->vyf = wtk_malloc(sizeof(qdm_kiss_fft_cpx *) * nfeat_chan);
        kcf->vxyf = wtk_malloc(sizeof(qdm_kiss_fft_cpx *) * nfeat_chan);
        for (i = 0; i < nfeat_chan; i++) {
            kcf->vxf[i] = wtk_malloc(sizeof(qdm_kiss_fft_cpx) * kcf->nfreq);
            kcf->vyf[i] = wtk_malloc(sizeof(qdm_kiss_fft_cpx) * kcf->nfreq);
            kcf->vxyf[i] = wtk_malloc(sizeof(qdm_kiss_fft_cpx) * kcf->nfreq);
        }
        kcf->xyf_data = wtk_malloc(sizeof(qdm_kiss_fft_cpx) * kcf->nfreq);
        kcf->xy_data = wtk_malloc(sizeof(float) * width * height);
        kcf->k = wtk_malloc(sizeof(float) * width * height);
        kcf->kf = wtk_malloc(sizeof(qdm_kiss_fft_cpx) * kcf->nfreq);
        kcf->kf_lambda = wtk_malloc(sizeof(qdm_kiss_fft_cpx) * kcf->nfreq);
        kcf->new_alphaf = wtk_malloc(sizeof(qdm_kiss_fft_cpx) * kcf->nfreq);
        kcf->new_alphaf_den = wtk_malloc(sizeof(qdm_kiss_fft_cpx) * kcf->nfreq);
        kcf->alphaf = wtk_malloc(sizeof(qdm_kiss_fft_cpx) * kcf->nfreq);
        kcf->alphaf_den = wtk_malloc(sizeof(qdm_kiss_fft_cpx) * kcf->nfreq);
        kcf->spec = wtk_malloc(sizeof(qdm_kiss_fft_cpx) * kcf->nfreq);
        kcf->spec2 = wtk_malloc(sizeof(qdm_kiss_fft_cpx) * kcf->nfreq);
        kcf->response = wtk_malloc(sizeof(float) * width * height);

        if (cfg->post_hist > 0) {
            kcf->pn_hist = qtk_cyclearray_new(cfg->post_hist, sizeof(float[2]));
            kcf->peak_hist_sum = kcf->neighbor_hist_sum = 0;
        } else {
            kcf->pn_hist = NULL;
        }

        if (cfg->motion_vector_hist > 0) {
            kcf->motion_vector_hist =
                qtk_cyclearray_new(cfg->motion_vector_hist, sizeof(float[2]));
            kcf->motion_vector_sum[0] = 0;
            kcf->motion_vector_sum[1] = 0;
        } else {
            kcf->motion_vector_hist = NULL;
        }
    }

    return 0;
}

static void qtk_objtrack_KCF_clean(objtrack_KCF_impl_t *kcf) {
    int feat_channels = 1 + kcf->cfg->compress_size;

    wtk_free(kcf->win);
    wtk_free(kcf->y);
    wtk_free(kcf->k);
    wtk_free(kcf->kf);
    wtk_free(kcf->spec);
    wtk_free(kcf->spec2);
    wtk_free(kcf->kf_lambda);
    wtk_free(kcf->new_alphaf);
    wtk_free(kcf->new_alphaf_den);
    wtk_free(kcf->alphaf);
    wtk_free(kcf->alphaf_den);

    for (int i = 0; i < feat_channels; i++) {
        wtk_free(kcf->vxf[i]);
        wtk_free(kcf->vyf[i]);
        wtk_free(kcf->vxyf[i]);
    }

    wtk_free(kcf->vxf);
    wtk_free(kcf->vyf);
    wtk_free(kcf->vxyf);

    wtk_free(kcf->xyf_data);
    wtk_free(kcf->xy_data);
    wtk_free(kcf->response);
    wtk_free(kcf->x);
    wtk_free(kcf->z);

    if (kcf->pn_hist) {
        qtk_cyclearray_delete(kcf->pn_hist);
    }

    if (kcf->motion_vector_hist) {
        qtk_cyclearray_delete(kcf->motion_vector_hist);
    }

    wtk_free(kcf->feature_pca);
    wtk_free(kcf->feature_npca);

    wtk_free(kcf->Z1);
    wtk_free(kcf->Z0);

    wtk_free(kcf->Zc1);
    wtk_free(kcf->Zc0);

    wtk_free(kcf->old_cov);
    wtk_free(kcf->new_cov);
    wtk_free(kcf->proj_matrix);

    wtk_free(kcf->U);
    wtk_free(kcf->SIGMA);
    wtk_free(kcf->VT);
    wtk_free(kcf->WORK);
    wtk_free(kcf->IWORK);

    wtk_free(kcf->yf);

    qdm_kiss_fftndr_free(kcf->fft2_cfg);
    qdm_kiss_fftndr_free(kcf->ifft2_cfg);

    wtk_free(kcf->tmp_buf);

    wtk_free(kcf->img_patch);
}

static int _KCF_feature_extract(objtrack_KCF_impl_t *kcf, uint8_t *patch,
                                qtk_objtrack_mode_t mode, float *feature,
                                int channel) {
    uint8_t *gray = cast(uint8_t *, kcf->tmp_buf);
    qtk_image_desc_t patch_desc = {
        .height = kcf->roi.height,
        .width = kcf->roi.width,
        .channel = channel,
    };
    int npixel = patch_desc.height * patch_desc.width;
    if (mode & TRACKING_GRAY) {
        if (channel == 3) {
            qtk_image_color_bgr2gray_u8(patch, gray, &patch_desc);
        }
        for (int i = 0; i < npixel; i++) {
            feature[i] = gray[i] * 1.0 / 255.0 - 0.5;
        }
        qtk_vector_multipy_elewise(feature, kcf->win, feature, npixel);
    }
    if (mode & TRACKING_CN) {
        qtk_assert(channel == 3);
        qtk_image_feature_cn(patch, &patch_desc, feature);
        qtk_vector_multipy_elewise_depth(feature, kcf->win, feature, npixel,
                                         10);
    }
    return 0;
}

static int _KCF_roi_feature_extract(objtrack_KCF_impl_t *kcf, uint8_t *image,
                                    qtk_image_desc_t *desc) {
    _KCF_get_img_patch(kcf, image, desc);
    _KCF_feature_extract(kcf, kcf->img_patch, kcf->cfg->desc_npca,
                         kcf->feature_npca, desc->channel);
    _KCF_feature_extract(kcf, kcf->img_patch, kcf->cfg->desc_pca,
                         kcf->feature_pca, desc->channel);

    return 0;
}

// FIXME: hard-coded feature_channels eq 10
static int _KCF_update_projectionMatrix(objtrack_KCF_impl_t *kcf,
                                        float *feature, int height, int width) {
    float *feature_tmp;
    float *pca_data = cast(float *, kcf->tmp_buf), *pca_data_tmp;
    float *svd_data = cast(float *, kcf->tmp_buf);
    float *proj_matrix_vars = cast(float *, kcf->tmp_buf);
    int npixel = height * width;
    int m, n, k;
    char T = 'T';
    char N = 'N';
    char A = 'A';
    int info;
    float alpha, beta;
    int feat_channel = 10;
    double avg[10] = {0};

    feature_tmp = feature;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            for (int k = 0; k < feat_channel; k++) {
                avg[k] += *feature_tmp++;
            }
        }
    }

    for (int i = 0; i < feat_channel; i++) {
        avg[i] /= npixel;
    }

    feature_tmp = feature;
    pca_data_tmp = pca_data;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            for (int k = 0; k < feat_channel; k++) {
                *pca_data_tmp++ = *feature_tmp++ - avg[k];
            }
        }
    }

    m = 10;
    k = npixel;
    n = 10;
    alpha = 1.0 / cast(float, npixel - 1);
    beta = 0;

    // do not need to transpose result as result is symmetric
    sgemm_(&N, &T, &m, &n, &k, &alpha, pca_data, &m, pca_data, &n, &beta,
           kcf->new_cov, &m);

    if (kcf->frame_idx == 0) {
        memcpy(kcf->old_cov, kcf->new_cov,
               feat_channel * feat_channel * sizeof(float));
    }

    for (int i = 0, data_idx = 0; i < feat_channel; i++) {
        for (int j = 0; j < feat_channel; j++, data_idx++) {
            // adjust for sgesdd_ matrix order
            svd_data[data_idx] = (1.0 - kcf->cfg->pca_learning_rate) *
                                     kcf->old_cov[j * feat_channel + i] +
                                 kcf->cfg->pca_learning_rate *
                                     kcf->new_cov[j * feat_channel + i];
        }
    }

    sgesdd_(&A, &feat_channel, &feat_channel, svd_data, &feat_channel,
            kcf->SIGMA, kcf->U, &feat_channel, kcf->VT, &feat_channel,
            kcf->WORK, &kcf->LWORK, kcf->IWORK, &info);

    qtk_assert(info == 0);

    for (int i = 0; i < feat_channel; i++) {
        for (int j = 0; j < kcf->cfg->compress_size; j++) {
            kcf->proj_matrix[i * kcf->cfg->compress_size + j] =
                kcf->U[j * feat_channel + i];
        }
    }

    alpha = kcf->cfg->pca_learning_rate;
    beta = 1 - alpha;

    for (int i = 0, data_idx = 0; i < feat_channel; i++) {
        for (int j = 0; j < kcf->cfg->compress_size; j++, data_idx++) {
            proj_matrix_vars[data_idx] =
                kcf->SIGMA[j] * kcf->proj_matrix[data_idx];
        }
    }

    m = 10;
    k = kcf->cfg->compress_size;
    n = 10;
    sgemm_(&T, &N, &m, &n, &k, &alpha, proj_matrix_vars, &k, kcf->proj_matrix,
           &k, &beta, kcf->old_cov, &feat_channel);

    return 0;
}

static int _KCF_compress(objtrack_KCF_impl_t *kcf, float *src, float *dst,
                         int width, int height) {
    int npixel = width * height;
    float *tmp_dst = cast(float *, kcf->tmp_buf);
    int m, n, k;
    char T = 'T';
    float alpha, beta;

    m = npixel;
    n = kcf->cfg->compress_size;
    k = 10;
    beta = 0;
    alpha = 1.0;
    sgemm_(&T, &T, &m, &n, &k, &alpha, src, &k, kcf->proj_matrix, &n, &beta,
           tmp_dst, &m);

    for (int i = 0; i < npixel; i++) {
        for (int j = 0; j < kcf->cfg->compress_size; j++) {
            dst[i * kcf->cfg->compress_size + j] = tmp_dst[j * npixel + i];
        }
    }

    return 0;
}

static void _mat_merge_2(float *mat1, float *mat2, int width, int height,
                         int channel1, int channel2, float *dst) {
    int npixel = width * height;
    int dst_channel = channel1 + channel2;

    for (int i = 0; i < npixel;
         i++, mat1 += channel1, mat2 += channel2, dst += dst_channel) {
        memcpy(dst, mat1, sizeof(float) * channel1);
        memcpy(dst + channel1, mat2, sizeof(float) * channel2);
    }
}

static void mulSpectrums_conjB_(qdm_kiss_fft_cpx *f1, qdm_kiss_fft_cpx *f2,
                                qdm_kiss_fft_cpx *dst, int nfreq) {
    for (int k = 0; k < nfreq; k++) {
        dst[k].r = f1[k].r * f2[k].r + f1[k].i * f2[k].i;
        dst[k].i = f1[k].i * f2[k].r - f1[k].r * f2[k].i;
    }
}

static void mulSpectrums_(qdm_kiss_fft_cpx *f1, qdm_kiss_fft_cpx *f2,
                          qdm_kiss_fft_cpx *dst, int nfreq) {
    for (int k = 0; k < nfreq; k++) {
        dst[k].r = f1[k].r * f2[k].r - f1[k].i * f2[k].i;
        dst[k].i = f1[k].i * f2[k].r + f1[k].r * f2[k].i;
    }
}

static void pixelWiseMult_conjB_(qdm_kiss_fft_cpx **xf_data,
                                 qdm_kiss_fft_cpx **yf_data, int nchan,
                                 int nfreq, qdm_kiss_fft_cpx **xyf_data) {
    for (int i = 0; i < nchan; i++) {
        mulSpectrums_conjB_(xf_data[i], yf_data[i], xyf_data[i], nfreq);
    }
}

static void sum_channel_(qdm_kiss_fft_cpx **v_data, int nchan, int nfreq,
                         qdm_kiss_fft_cpx *dst) {
    memcpy(dst, v_data[0], sizeof(qdm_kiss_fft_cpx) * nfreq);
    for (int i = 1; i < nchan; i++) {
        for (int j = 0; j < nfreq; j++) {
            dst[j].r += v_data[i][j].r;
            dst[j].i += v_data[i][j].i;
        }
    }
}

static int _KCF_dense_gauss_kernel(objtrack_KCF_impl_t *kcf, float *x_data,
                                   float *y_data, int width, int height) {
    int feat_channels = kcf->cfg->compress_size + 1;
    float normX, normY, coff;
    const float sigma = kcf->cfg->sigma;
    float sig;
    int npixel = kcf->roi.width * kcf->roi.height;

    qdm_kiss_fft_cpx **xf_data = kcf->vxf;
    qdm_kiss_fft_cpx **yf_data = kcf->vyf;
    qdm_kiss_fft_cpx **xyf_v = kcf->vxyf;
    qdm_kiss_fft_cpx *xyf = kcf->xyf_data;
    float *xy = kcf->xy_data;
    float *xyt = cast(float *, kcf->tmp_buf);
    float *xy_trans = cast(float *, kcf->tmp_buf);
    float *k_data = kcf->k;

    for (int i = 0; i < npixel; i++) {
        for (int j = 0; j < feat_channels; j++) {
            xy_trans[j * npixel + i] = x_data[i * feat_channels + j];
        }
    }

    for (int i = 0; i < feat_channels; i++) {
        qdm_kiss_fftndr(kcf->fft2_cfg, xy_trans + i * npixel, xf_data[i]);
    }

    for (int i = 0; i < npixel; i++) {
        for (int j = 0; j < feat_channels; j++) {
            xy_trans[j * npixel + i] = y_data[i * feat_channels + j];
        }
    }

    for (int i = 0; i < feat_channels; i++) {
        qdm_kiss_fftndr(kcf->fft2_cfg, xy_trans + i * npixel, yf_data[i]);
    }

    normX = qtk_vector_dotf(x_data, x_data, npixel * feat_channels);
    normY = qtk_vector_dotf(y_data, y_data, npixel * feat_channels);

    pixelWiseMult_conjB_(xf_data, yf_data, feat_channels, kcf->nfreq, xyf_v);
    sum_channel_(xyf_v, feat_channels, kcf->nfreq, xyf);

    qdm_kiss_fftndri(kcf->ifft2_cfg, xyf, xyt);
    qtk_vector_scale(xyt, xyt, npixel, 1.0 / npixel);

    // FIXME
    qtk_assert(kcf->cfg->wrap_kernel == 0);
    coff = 1.0 / (npixel * feat_channels);

    for (int i = 0; i < npixel; i++) {
        xy[i] = (normX + normY - 2 * xyt[i]) * coff;
        if (xy[i] < 0) {
            xy[i] = 0;
        }
    }

    sig = -1.0 / (sigma * sigma);

    for (int i = 0; i < npixel; i++) {
        k_data[i] = qtk_expf(xy[i] * sig);
    }

    return 0;
}

static void calc_response_split_(objtrack_KCF_impl_t *kcf, int width,
                                 int height, int nfreq) {
    qdm_kiss_fft_cpx *spec_cursor = kcf->spec;
    qdm_kiss_fft_cpx *spec2_cursor = kcf->spec2;
    qdm_kiss_fft_cpx *alphaf_den_cursor = kcf->alphaf_den;
    float den;

    mulSpectrums_(kcf->alphaf, kcf->kf, kcf->spec, nfreq);

    for (int j = 0; j < nfreq;
         j++, spec2_cursor++, alphaf_den_cursor++, spec_cursor++) {
        den = 1.0 / (alphaf_den_cursor->r * alphaf_den_cursor->r +
                     alphaf_den_cursor->i * alphaf_den_cursor->i);
        spec2_cursor->r = (spec_cursor->r * alphaf_den_cursor->r +
                           spec_cursor->i * alphaf_den_cursor->i) *
                          den;
        spec2_cursor->i = (spec_cursor->i * alphaf_den_cursor->r -
                           spec_cursor->r * alphaf_den_cursor->i) *
                          den;
    }

    qdm_kiss_fftndri(kcf->ifft2_cfg, kcf->spec2, kcf->response);
    qtk_vector_scale(kcf->response, kcf->response, width * height,
                     1.0 / (width * height));
}

static void calc_response_(objtrack_KCF_impl_t *kcf, int width, int height,
                           int nfreq) {
    mulSpectrums_(kcf->alphaf, kcf->kf, kcf->spec, nfreq);
    qdm_kiss_fftndri(kcf->ifft2_cfg, kcf->spec, kcf->response);
    qtk_vector_scale(kcf->response, kcf->response, width * height,
                     1.0 / (width * height));
}

static float max_response_(objtrack_KCF_impl_t *kcf, int *x, int *y, int width,
                           int height) {
    float maxVal = FLT_MIN;
    float *response_cursor = kcf->response;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++, response_cursor++) {
            if (*response_cursor > maxVal) {
                maxVal = *response_cursor;
                *x = j;
                *y = i;
            }
        }
    }
    return maxVal;
}

static float calc_neighborhood_(objtrack_KCF_impl_t *kcf, int peak_x,
                                int peak_y, int width, int height) {
    int x_min = max(0, peak_x - 2);
    int x_max = min(width - 1, peak_x + 2);
    int y_min = max(0, peak_y - 2);
    int y_max = min(height - 1, peak_y + 2);
    int cnt = 0;
    float sum = 0;
    float *response_row;
    int i, j;

    for (i = y_min, response_row = kcf->response + i * width; i <= y_max;
         i++, response_row += width) {
        for (j = x_min; j <= x_max; j++, cnt++) {
            sum += response_row[j];
        }
    }

    sum -= kcf->response[peak_y * width + peak_x];
    cnt -= 1;

    return sum / cnt;
}

static void _get_result(objtrack_KCF_impl_t *kcf, qtk_image_roi_t *result) {
    float coff = 1.0 / (1 + 2 * kcf->cfg->expand_fact);
    float widthf = kcf->roi.width * coff;
    float heightf = kcf->roi.height * coff;

    result->width = qtk_roundf(widthf);
    result->height = qtk_roundf(heightf);
    result->x = qtk_roundf(kcf->roi.x + kcf->cfg->expand_fact * widthf);
    result->y = qtk_roundf(kcf->roi.y + kcf->cfg->expand_fact * heightf);
}

static void calc_avg_response_(objtrack_KCF_impl_t *kcf, float *pavg,
                               float *navg, float pcur, float ncur) {
    float sub[2] = {0, 0};
    float cur[2] = {pcur, ncur};

    qtk_cyclearray_push(kcf->pn_hist, cur, sub);
    kcf->peak_hist_sum += pcur - sub[0];
    kcf->neighbor_hist_sum += ncur - sub[1];

    *pavg = kcf->peak_hist_sum / qtk_cyclearray_len(kcf->pn_hist);
    *navg = kcf->neighbor_hist_sum / qtk_cyclearray_len(kcf->pn_hist);
}

static float calc_conf_(objtrack_KCF_impl_t *kcf, int width, int height, int *x,
                        int *y) {
    float pval, nval;
    float pavg, navg;
    float conf;

    pval = max_response_(kcf, x, y, width, height);
    nval = calc_neighborhood_(kcf, *x, *y, width, height);
    if (kcf->cfg->post_hist > 0) {
        calc_avg_response_(kcf, &pavg, &navg, pval, nval);
        conf = pval / navg;
    } else {
        conf = pval;
    }
    return conf;
}

static void update_motion_vector_(objtrack_KCF_impl_t *kcf, float x_motion,
                                  float y_motion) {
    float cur[2] = {x_motion, y_motion};
    float sub[2] = {0, 0};

    qtk_cyclearray_push(kcf->motion_vector_hist, cur, sub);
    kcf->motion_vector_sum[0] += cur[0] - sub[0];
    kcf->motion_vector_sum[1] += cur[1] - sub[1];

    kcf->motion_vector[0] =
        kcf->motion_vector_sum[0] / qtk_cyclearray_len(kcf->motion_vector_hist);
    kcf->motion_vector[1] =
        kcf->motion_vector_sum[1] / qtk_cyclearray_len(kcf->motion_vector_hist);
}

int qtk_objtrack_KCF_update(qtk_objtrack_KCF_t kcf_wrapper, uint8_t *image,
                            qtk_image_desc_t *desc, qtk_image_roi_t *result,
                            float *conf) {
    objtrack_KCF_impl_t *kcf = cast(objtrack_KCF_impl_t *, kcf_wrapper);
    qtk_assert(desc->channel == 1 || desc->channel == 3);
    int width, height;

    // TODO image resize if needed

    if (kcf->frame_idx > 0) {
        int x = 0, y = 0;
        float x_motion, y_motion;
        _KCF_roi_feature_extract(kcf, image, desc);

        width = qtk_roundf(kcf->roi.width);
        height = qtk_roundf(kcf->roi.height);
        if (kcf->cfg->desc_pca != 0) {
            _KCF_compress(kcf, kcf->feature_pca, kcf->feature_pca, width,
                          height);
            _KCF_compress(kcf, kcf->Z0, kcf->Zc0, width, height);
        }

        memcpy(kcf->Zc1, kcf->Z1, sizeof(float) * kcf->npca_dim);

        _mat_merge_2(kcf->feature_pca, kcf->feature_npca, width, height,
                     kcf->cfg->compress_size, 1, kcf->x);
        _mat_merge_2(kcf->Zc0, kcf->Zc1, width, height, kcf->cfg->compress_size,
                     1, kcf->z);

        _KCF_dense_gauss_kernel(kcf, kcf->x, kcf->z, width, height);

        qdm_kiss_fftndr(kcf->fft2_cfg, kcf->k, kcf->kf);

        if (kcf->cfg->split_coeff) {
            calc_response_split_(kcf, width, height, kcf->nfreq);
        } else {
            calc_response_(kcf, width, height, kcf->nfreq);
        }

        *conf = calc_conf_(kcf, width, height, &x, &y);
        if (*conf < kcf->cfg->detect_thresh) {
            return 0;
        }

        x_motion = x - kcf->roi.width / 2 + 1;
        y_motion = y - kcf->roi.height / 2 + 1;
        kcf->roi.x += x_motion;
        kcf->roi.y += y_motion;
        if (kcf->motion_vector_hist) {
            update_motion_vector_(kcf, x_motion, y_motion);
        }
    } else {
        *conf = 1;
    }

    width = qtk_roundf(kcf->roi.width);
    height = qtk_roundf(kcf->roi.height);
    // ROI already update
    _KCF_roi_feature_extract(kcf, image, desc);

    if (kcf->frame_idx == 0) {
        memcpy(kcf->Z1, kcf->feature_npca, sizeof(float) * kcf->npca_dim);
        memcpy(kcf->Z0, kcf->feature_pca, sizeof(float) * kcf->pca_dim);
    } else {
        qtk_vector_update(kcf->Z1, kcf->feature_npca, kcf->npca_dim,
                          kcf->cfg->interp_fact);
        qtk_vector_update(kcf->Z0, kcf->feature_pca, kcf->pca_dim,
                          kcf->cfg->interp_fact);
    }

    _KCF_update_projectionMatrix(kcf, kcf->Z0, width, height);
    _KCF_compress(kcf, kcf->feature_pca, kcf->feature_pca, width, height);

    _mat_merge_2(kcf->feature_pca, kcf->feature_npca, width, height,
                 kcf->cfg->compress_size, 1, kcf->x);
    _KCF_dense_gauss_kernel(kcf, kcf->x, kcf->x, width, height);

    qdm_kiss_fftndr(kcf->fft2_cfg, kcf->k, kcf->kf);

    memcpy(kcf->kf_lambda, kcf->kf, sizeof(qdm_kiss_fft_cpx) * kcf->nfreq);
    for (int i = 0; i < kcf->nfreq; i++) {
        kcf->kf_lambda[i].r += kcf->cfg->lambda;
    }

    if (kcf->cfg->split_coeff) {
        mulSpectrums_(kcf->yf, kcf->kf, kcf->new_alphaf, kcf->nfreq);
        mulSpectrums_(kcf->kf, kcf->kf_lambda, kcf->new_alphaf_den, kcf->nfreq);
    } else {
        // FIXME
        qtk_assert(0);
    }

    // update the RLS model
    if (kcf->frame_idx == 0) {
        memcpy(kcf->alphaf, kcf->new_alphaf,
               sizeof(qdm_kiss_fft_cpx) * kcf->nfreq);
        if (kcf->cfg->split_coeff) {
            memcpy(kcf->alphaf_den, kcf->new_alphaf_den,
                   sizeof(qdm_kiss_fft_cpx) * kcf->nfreq);
        }
    } else {
        qtk_vector_update(cast(float *, kcf->alphaf),
                          cast(float *, kcf->new_alphaf), kcf->nfreq * 2,
                          kcf->cfg->interp_fact);
        if (kcf->cfg->split_coeff) {
            qtk_vector_update(cast(float *, kcf->alphaf_den),
                              cast(float *, kcf->new_alphaf_den),
                              kcf->nfreq * 2, kcf->cfg->interp_fact);
        }
    }

    _get_result(kcf, result);

    kcf->frame_idx++;

    return 1;
}

void qtk_objtrack_KCF_cfg_init(qtk_objtrack_KCF_cfg_t *cfg) {
    cfg->detect_thresh = 0.6;
    cfg->sigma = 0.2;
    cfg->lambda = 0.0001;
    cfg->interp_fact = 0.075;
    cfg->output_sigma_fact = 1.0 / 16.0;
    cfg->expand_fact = 0.5;
    cfg->retracking_moving_fact = 0.2;
    cfg->pca_learning_rate = 0.15;
    cfg->max_patch_size = 80 * 80;
    cfg->compress_size = 2;
    cfg->post_hist = 4;
    cfg->motion_vector_hist = 3;

    cfg->desc_npca = TRACKING_GRAY;
    cfg->desc_pca = TRACKING_CN;

    cfg->resize = 0;
    cfg->split_coeff = 1;
    cfg->wrap_kernel = 0;
    cfg->compress_feature = 1;
}

int retracking_search_(objtrack_KCF_impl_t *kcf, int order[8], uint8_t *image,
                       qtk_image_desc_t *desc, qtk_image_roi_t *result,
                       float *conf) {
    for (int i = 0; i < 8; i++) {
        qtk_image_roi_f_t roi_bak = kcf->roi;
        switch (order[i]) {
        case 1:
            kcf->roi.x -= kcf->roi.width * kcf->cfg->retracking_moving_fact;
            kcf->roi.y -= kcf->roi.height * kcf->cfg->retracking_moving_fact;
            break;
        case 2:
            kcf->roi.y -= kcf->roi.height * kcf->cfg->retracking_moving_fact;
            break;
        case 3:
            kcf->roi.x += kcf->roi.width * kcf->cfg->retracking_moving_fact;
            kcf->roi.y -= kcf->roi.height * kcf->cfg->retracking_moving_fact;
            break;
        case 4:
            kcf->roi.x -= kcf->roi.width * kcf->cfg->retracking_moving_fact;
            break;
        case 6:
            kcf->roi.x += kcf->roi.width * kcf->cfg->retracking_moving_fact;
            break;
        case 7:
            kcf->roi.x -= kcf->roi.width * kcf->cfg->retracking_moving_fact;
            kcf->roi.y += kcf->roi.height * kcf->cfg->retracking_moving_fact;
            break;
        case 8:
            kcf->roi.y += kcf->roi.height * kcf->cfg->retracking_moving_fact;
            break;
        case 9:
            kcf->roi.x += kcf->roi.width * kcf->cfg->retracking_moving_fact;
            kcf->roi.y += kcf->roi.height * kcf->cfg->retracking_moving_fact;
            break;
        default:
            qtk_unreachable();
        }
        if (qtk_objtrack_KCF_update(kcf, image, desc, result, conf)) {
            goto found;
        }
        kcf->roi = roi_bak;
    }
    return 0;
found:
    return 1;
}

int qtk_objtrack_KCF_retracking(qtk_objtrack_KCF_t kcf_wrapper, uint8_t *image,
                                qtk_image_desc_t *desc, qtk_image_roi_t *result,
                                float *conf) {
    objtrack_KCF_impl_t *kcf = cast(objtrack_KCF_impl_t *, kcf_wrapper);
    int region = 5;
    int x = kcf->motion_vector[0];
    int y = kcf->motion_vector[1];

    int search_order[][8] = {{1, 4, 2, 7, 3, 8, 6, 9}, {2, 1, 3, 4, 6, 8, 7, 9},
                             {3, 2, 6, 1, 9, 4, 8, 7}, {4, 1, 7, 2, 8, 6, 3, 9},
                             {4, 6, 2, 8, 7, 9, 1, 3}, {6, 9, 3, 8, 2, 4, 1, 7},
                             {7, 4, 8, 1, 9, 2, 6, 3}, {8, 9, 7, 6, 4, 2, 1, 3},
                             {9, 6, 8, 3, 7, 2, 4, 1}};

    if (x < -2) {
        if (y < -2) {
            region = 1;
        } else if (y > 2) {
            region = 7;
        }
        region = 4;
    } else if (x > 2) {
        if (y < -2) {
            region = 3;
        } else if (y > 2) {
            region = 9;
        }
        region = 6;
    } else {
        if (y < -2) {
            region = 2;
        } else if (y > 2) {
            region = 8;
        }
        region = 5;
    }

    return retracking_search_(kcf, search_order[region - 1], image, desc,
                              result, conf);
}

qtk_objtrack_KCF_t qtk_objtrack_KCF_new(qtk_objtrack_KCF_cfg_t *cfg,
                                        uint8_t *image,
                                        qtk_image_desc_t *img_desc,
                                        qtk_image_roi_t *roi) {
    objtrack_KCF_impl_t *impl = wtk_malloc(sizeof(objtrack_KCF_impl_t));
    qtk_objtrack_KCF_init(impl, cfg, image, img_desc, roi);
    return impl;
}

void qtk_objtrack_KCF_delete(qtk_objtrack_KCF_t kcf_wrapper) {
    objtrack_KCF_impl_t *kcf = cast(objtrack_KCF_impl_t *, kcf_wrapper);
    qtk_objtrack_KCF_clean(kcf);
    wtk_free(kcf);
}
