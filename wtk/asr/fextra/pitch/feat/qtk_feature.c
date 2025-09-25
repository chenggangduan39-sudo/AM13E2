#include "wtk/asr/fextra/pitch/feat/qtk_feature.h"
#include "wtk/asr/fextra/pitch/core/qtk_base.h"
#include "wtk/asr/fextra/pitch/math/qtk_math.h"
#include "wtk/core/wtk_alloc.h"

qtk_feature_delta_t *qtk_feature_delta_new(qtk_feature_delta_cfg_t *cfg) {
    int idx;
    qtk_feature_delta_t *delta = wtk_malloc(sizeof(qtk_feature_delta_t));
    delta->cfg = cfg;
    delta->scales = wtk_malloc(sizeof(float *) * (cfg->order + 1));
    delta->scales_dim = wtk_malloc(sizeof(int) * (cfg->order + 1));

    delta->scales_dim[0] = 1;
    delta->scales[0] = wtk_malloc(sizeof(float) * 1);
    delta->scales[0][0] = 1.0;

    for (idx = 1; idx <= cfg->order; idx++) {
        int j;
        int prev_scales_dim = delta->scales_dim[idx - 1];
        int window = cfg->window;
        int cur_scales_dim = prev_scales_dim + 2 * window;
        int prev_offfset = (prev_scales_dim - 1) / 2,
            cur_offset = prev_offfset + window;

        delta->scales[idx] = wtk_calloc(sizeof(float), cur_scales_dim);
        delta->scales_dim[idx] = cur_scales_dim;

        float *prev_scales = delta->scales[idx - 1];
        float *cur_scales = delta->scales[idx];

        float normalizer = 0.0;
        for (j = -window; j <= window; j++) {
            int k;
            normalizer += j * j;
            for (k = -prev_offfset; k <= prev_offfset; k++) {
                cur_scales[j + k + cur_offset] +=
                    j * prev_scales[k + prev_offfset];
            }
        }
        qtk_vec_scale(cur_scales, cur_scales_dim, 1.0 / normalizer);
    }

    return delta;
}

void qtk_feature_delta_delete(qtk_feature_delta_t *delta) {
    int idx;
    int num = delta->cfg->order + 1;

    for (idx = 0; idx < num; idx++) {
        wtk_free(delta->scales[idx]);
    }
    wtk_free(delta->scales);
    wtk_free(delta->scales_dim);
    wtk_free(delta);
}

int qtk_feature_delta_process(qtk_feature_delta_t *delta, float *input,
                              float *output, int irow, int icol, int frame) {
    int idx;
    QTK_ASSERT(frame < irow, "found bug\n");
    int num_frames = irow, feat_dim = icol;
    memset(output, 0, (icol * (delta->cfg->order + 1)) * sizeof(float));

    for (idx = 0; idx <= delta->cfg->order; idx++) {
        int j;
        float *scales = delta->scales[idx];
        int max_offset = (delta->scales_dim[idx] - 1) / 2;
        float *output_p = output + idx * feat_dim;
        for (j = -max_offset; j <= max_offset; j++) {
            int offset_frame = frame + j;
            if (offset_frame < 0) {
                offset_frame = 0;
            } else if (offset_frame >= num_frames) {
                offset_frame = num_frames - 1;
            }
            float scale = scales[j + max_offset];
            if (scale != 0.0) {
                qtk_vec_add_vec(output_p, feat_dim,
                                MAT_GET_ROW(input, icol, offset_frame), scale);
            }
        }
    }
    return 0;
}
