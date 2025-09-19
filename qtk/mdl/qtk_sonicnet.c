#include "qtk/mdl/qtk_sonicnet.h"
#include "qtk/nnrt/qtk_nnrt.h"
#include "wtk/core/wtk_alloc.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/wtk_heap.h"
#include "wtk/core/wtk_robin.h"
#include "wtk/core/wtk_type.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_lock.h"
#include "wtk/os/wtk_thread.h"
#include "wtk/core/fft/wtk_cfft.h"
#include "qtk/math/qtk_matrix.h"
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#define ASSIGN_SHAPE(shape, B, C, T, D)                                        \
    (shape)[0] = B;                                                            \
    (shape)[1] = C;                                                            \
    (shape)[2] = T;                                                            \
    (shape)[3] = D
#define TENSOR_NELEM(shape) ((shape)[0] * (shape)[1] * (shape)[2] * (shape)[3])

typedef struct {
    wtk_queue_node_t node;
    int T;
    float *data;
} vad_in_msg_t;

static vad_in_msg_t *vad_in_msg_new(qtk_sonicnet_t *sn, int T) {
    vad_in_msg_t *msg =
        wtk_malloc(sizeof(vad_in_msg_t) +
                   sizeof(float) * T * sn->vad_attn_dim * sn->vad_feat_dim);
    msg->T = T;
    msg->data = (float *)(msg + 1);
    return msg;
}

static void vad_in_msg_delete(qtk_sonicnet_t *sn, vad_in_msg_t *msg) {
    wtk_free(msg);
}

static int vad_nn_run_(qtk_sonicnet_t *sn, int T, int C, int D, float *input) {
    int64_t shape[4];
    float *out, prob;
    ASSIGN_SHAPE(shape, 1, C, T, D);
    qtk_nnrt_value_t input_val, output_val;

    input_val = qtk_nnrt_value_create_external(
        sn->vad_rt, QTK_NNRT_VALUE_ELEM_F32, shape, 4, input);
    qtk_nnrt_feed(sn->vad_rt, input_val, 0);
    qtk_nnrt_run(sn->vad_rt);
    qtk_nnrt_get_output(sn->vad_rt, &output_val, 0);
    qtk_nnrt_value_get_shape(sn->vad_rt, output_val, shape, 2);
    out = qtk_nnrt_value_get_data(sn->vad_rt, output_val);
    prob = exp(out[1]);

    qtk_nnrt_value_release(sn->vad_rt, input_val);
    qtk_nnrt_value_release(sn->vad_rt, output_val);

    if (sn->vad_rt->cfg->use_ncnn) {
        qtk_nnrt_reset(sn->vad_rt);
    }

    if (sn->cfg->vad_use_thread) {
        wtk_lock_lock(&sn->vad_result_guard);
    }
    sn->vad_exist_prob = prob;
    sn->vad_utt_frame[1] += T;
    sn->vad_utt_frame[0] = sn->vad_utt_frame[1] - T;
    if (sn->cfg->vad_use_thread) {
        wtk_lock_unlock(&sn->vad_result_guard);
    }
    return 0;
}

int qtk_sonicnet_get_vad_result(qtk_sonicnet_t *sn, uint32_t *chunk_s,
                                uint32_t *chunk_e, float *prob) {
    int ret = -1;
    if (sn->cfg->vad_use_thread) {
        wtk_lock_lock(&sn->vad_result_guard);
    }
    if (sn->vad_utt_frame[1] > 0) {
        *chunk_s = sn->vad_utt_frame[0];
        *chunk_e = sn->vad_utt_frame[1];
        *prob = sn->vad_exist_prob;
        ret = 0;
    }
    if (sn->cfg->vad_use_thread) {
        wtk_lock_unlock(&sn->vad_result_guard);
    }
    return ret;
}

static int vad_route_(qtk_sonicnet_t *sn, wtk_thread_t *th) {
    wtk_queue_node_t *node;

    int C = sn->vad_attn_dim;
    int D = sn->vad_feat_dim;

    while (1) {
        node = wtk_blockqueue_pop(&sn->vad_in, -1, NULL);
        vad_in_msg_t *msg = data_offset2(node, vad_in_msg_t, node);
        if (msg->T == 0) {
            vad_in_msg_delete(sn, msg);
            break;
        }
        vad_nn_run_(sn, msg->T, C, D, msg->data);
        vad_in_msg_delete(sn, msg);
    }
    return 0;
}

static int init_vad_(qtk_sonicnet_t *sn) {
    char buf[32];
    sn->vad_rt = qtk_nnrt_new(&sn->cfg->vad_rt);
    sn->vad_chunk_pos = 0;
    if (qtk_nnrt_metadata_lookup(sn->vad_rt, "attn_dim", buf, sizeof(buf))) {
        qtk_nnrt_metadata_lookup(sn->vad_rt, "in_channels", buf, sizeof(buf));
    }
    sn->vad_attn_dim = atoi(buf);
    if (sn->vad_attn_dim == 0) {
        qtk_nnrt_delete(sn->vad_rt);
        sn->vad_rt = NULL;
        return -1;
    }
    qtk_nnrt_metadata_lookup(sn->vad_rt, "feat_dim", buf, sizeof(buf));
    sn->vad_feat_dim = atoi(buf);
    sn->vad_chunk = wtk_malloc(sizeof(float) * sn->cfg->vad_chunk_sz *
                               sn->vad_attn_dim * sn->vad_feat_dim);
    sn->vad_utt_frame[0] = sn->vad_utt_frame[1] = 0;
    if (sn->cfg->vad_use_thread) {
        wtk_blockqueue_init(&sn->vad_in);
        wtk_lock_init(&sn->vad_result_guard);
        wtk_thread_init(&sn->vad_thread, (thread_route_handler)vad_route_, sn);
        wtk_thread_start(&sn->vad_thread);
    }
    return 0;
}

static void cleanup_vad_(qtk_sonicnet_t *sn) {
    if (sn->cfg->vad_use_thread) {
        vad_in_msg_t *msg = vad_in_msg_new(sn, 0);
        wtk_blockqueue_push(&sn->vad_in, &msg->node);
        wtk_thread_join(&sn->vad_thread);
        wtk_blockqueue_clean(&sn->vad_in);
        wtk_lock_init(&sn->vad_result_guard);
    }
    qtk_nnrt_delete(sn->vad_rt);
    wtk_free(sn->vad_chunk);
}

qtk_sonicnet_t *qtk_sonicnet_new(qtk_sonicnet_cfg_t *cfg) {
    char buf[128];
    int max_chunk_sz = 0;
    qtk_sonicnet_t *sn = wtk_malloc(sizeof(qtk_sonicnet_t));
    sn->cfg = cfg;
    sn->rt = qtk_nnrt_new(&cfg->rt);
    qtk_nnrt_metadata_lookup(sn->rt, "right_context", buf, sizeof(buf));
    sn->right_context = atoi(buf);
    sn->subsamples = NULL;
    if (sn->rt->cfg->use_ncnn) {
        int i = 0;
        sn->subsamples = wtk_malloc(sizeof(int) * sn->right_context);
        if (!qtk_nnrt_metadata_lookup(sn->rt, "subsamples", buf, sizeof(buf))) {
            char *cursor, *sep;
            cursor = buf;
            while ((sep = strchr(cursor, ','))) {
                *sep = '\0';
                sn->subsamples[i++] = atoi(cursor);
                cursor = sep + 1;
            }
            sn->subsamples[i] = atoi(cursor);
        } else {
            for (i = 0; i < sn->right_context; i++) {
                sn->subsamples[i] = 1;
            }
        }
        sn->in_channels = NULL;
        if (!qtk_nnrt_metadata_lookup(sn->rt, "in_channels", buf,
                                      sizeof(buf))) {
            char *cursor, *sep;
            cursor = buf;
            sn->in_channels = wtk_malloc(sizeof(int) * sn->right_context);
            i = 0;
            while ((sep = strchr(cursor, ','))) {
                *sep = '\0';
                sn->in_channels[i++] = atoi(cursor);
                cursor = sep + 1;
            }
            sn->in_channels[i] = atoi(cursor);
        }
    }
    sn->nout = qtk_nnrt_get_num_out(sn->rt) - sn->right_context;
    if (sn->nout == 3 || (sn->nout == 2 && cfg->channel == 1)) {
        init_vad_(sn);
    }
    sn->heap = wtk_heap_new(4096);
    sn->cir_tmp = wtk_malloc(sizeof(wtk_complex_t) * cfg->channel * cfg->nzc);
    sn->dyn_cir = wtk_malloc(sizeof(wtk_complex_t) * cfg->channel * cfg->nzc);
    sn->nframe = 0;
    sn->left_padding = 0;
    sn->fft = wtk_cfft_new(cfg->nzc);
    if (cfg->left_context + cfg->right_context > 0) {
        sn->ctx = wtk_robin_new(1 + cfg->left_context + cfg->right_context);
        for (int i = 0; i < sn->ctx->nslot; i++) {
            sn->ctx->r[i] =
                wtk_malloc(sizeof(wtk_complex_t) * cfg->nzc * cfg->channel);
        }
    } else {
        sn->ctx = NULL;
    }
    for (int i = 0; i < cfg->nchunk_sz; i++) {
        if (max_chunk_sz < cfg->chunk_sz[i]) {
            max_chunk_sz = cfg->chunk_sz[i];
        }
    }
    if (max_chunk_sz > 0) {
        sn->chunks = wtk_robin_new(max_chunk_sz);
        for (int i = 0; i < max_chunk_sz; i++) {
            sn->chunks->r[i] =
                wtk_malloc(sizeof(wtk_complex_t) * cfg->channel * cfg->nzc * 2);
        }
    }
    sn->cache_value = wtk_calloc(sizeof(void *), sn->right_context);
    return sn;
}

void qtk_sonicnet_delete(qtk_sonicnet_t *sn) {
    if (sn->ctx) {
        for (int i = 0; i < sn->ctx->nslot; i++) {
            wtk_free(sn->ctx->r[i]);
        }
        wtk_robin_delete(sn->ctx);
    }
    if (sn->chunks) {
        for (int i = 0; i < sn->chunks->nslot; i++) {
            wtk_free(sn->chunks->r[i]);
        }
        wtk_free(sn->chunks);
    }
    wtk_cfft_delete(sn->fft);
    wtk_heap_delete(sn->heap);
    wtk_free(sn->cir_tmp);
    wtk_free(sn->dyn_cir);

    for (int i = 0; i < sn->right_context; i++) {
        if (sn->cache_value[i]) {
            qtk_nnrt_value_release(sn->rt, sn->cache_value[i]);
        }
    }
    wtk_free(sn->cache_value);
    qtk_nnrt_delete(sn->rt);
    if (sn->nout == 3 || (sn->nout == 2 && sn->cfg->channel == 1)) {
        cleanup_vad_(sn);
    }
    if (sn->subsamples) {
        wtk_free(sn->subsamples);
    }
    if (sn->in_channels) {
        wtk_free(sn->in_channels);
    }
    wtk_free(sn);
}

static void softmax_(float *val, int n, float *dst) {
    float *val1 = val + n;
    for (int i = 0; i < n; i++) {
        float f1 = exp(val[i]);
        float f2 = exp(val1[i]);
        dst[i] = f2 / (f1 + f2);
    }
}

static float *nn_post_(qtk_sonicnet_t *sn, float *range_prob, float *angle_prob,
                       int K, int T, int D) {
    float *prob = wtk_heap_malloc(sn->heap, sizeof(float) * (K + 1) * T * D);
    if (angle_prob) {
        memcpy(prob, range_prob + T * D, T * D * sizeof(float));
        memcpy(prob + T * D, angle_prob + K * T * D, K * T * D * sizeof(float));
        for (int i = 0; i < (K + 1) * T * D; i++) {
            prob[i] = exp(prob[i]);
        }
    } else {
        softmax_(range_prob, T * D, prob);
    }
    return prob; // TD + KTD
}

static void feed_input_(qtk_sonicnet_t *sn, qtk_nnrt_value_t *in) {
    int64_t shape[4];
    int chunk_sz = sn->chunks->used;
    float *feat =
        wtk_heap_malloc(sn->heap, sizeof(float) * 1 * 4 * chunk_sz *
                                      sn->cfg->nzc * sn->cfg->channel);
    float *p = feat;
    int C = 4 * sn->cfg->channel;
    for (int c = 0; c < C; c++) {
        for (int i = 0; i < chunk_sz; i++) {
            for (int j = 0; j < sn->cfg->nzc; j++) {
                wtk_complex_t *cir = wtk_robin_at(sn->chunks, i);
                wtk_complex_t *dyn_cir = cir + sn->cfg->nzc * sn->cfg->channel;
                float f[128];
                for (int m = 0; m < 4; m++) {
                    for (int _c = 0; _c < sn->cfg->channel; _c++) {
                        float ff[4] = {cir[_c * sn->cfg->nzc + j].a,
                                       cir[_c * sn->cfg->nzc + j].b,
                                       dyn_cir[_c * sn->cfg->nzc + j].a,
                                       dyn_cir[_c * sn->cfg->nzc + j].b};
                        f[m * sn->cfg->channel + _c] = ff[m];
                    }
                }
                *p++ = f[c];
            }
        }
    }

    ASSIGN_SHAPE(shape, 1, C, chunk_sz, sn->cfg->nzc);
    *in = qtk_nnrt_value_create_external(sn->rt, QTK_NNRT_VALUE_ELEM_F32, shape,
                                         4, feat);
    if (sn->cfg->rt.use_ncnn) {
        qtk_nnrt_feed_s(sn->rt, *in, "in0");
    } else {
        qtk_nnrt_feed(sn->rt, *in, 0);
    }
}

static int feed_vad_(qtk_sonicnet_t *sn) {
    vad_in_msg_t *msg;
    int T = sn->vad_chunk_pos;
    int C = sn->vad_attn_dim;
    int D = sn->vad_feat_dim;
    msg = vad_in_msg_new(sn, T);
    qtk_matrix_transposef(sn->vad_chunk, msg->data, T * D, C);
    if (sn->cfg->vad_use_thread) {
        wtk_blockqueue_push(&sn->vad_in, &msg->node);
    } else {
        vad_nn_run_(sn, T, C, D, msg->data);
        vad_in_msg_delete(sn, msg);
    }
    return 0;
}

static void process_encoder_out_(qtk_sonicnet_t *sn, qtk_nnrt_value_t val,
                                 int is_end) {
    int64_t shape[5];
    if (sn->rt->cfg->use_onnxruntime || sn->nframe >= sn->right_context) {
        float *encoder_out = qtk_nnrt_value_get_data(sn->rt, val);
        int L = sn->vad_attn_dim * sn->vad_feat_dim;
        int TL = sn->vad_chunk_pos * L;
        int newly_T;
        qtk_nnrt_value_get_shape(sn->rt, val, shape, 4);
        newly_T = min(shape[2], sn->cfg->vad_chunk_sz - sn->vad_chunk_pos);
        qtk_matrix_transposef2(encoder_out, sn->vad_chunk + TL,
                               sn->vad_attn_dim, shape[2] * sn->vad_feat_dim,
                               newly_T * sn->vad_feat_dim);
        sn->vad_chunk_pos += newly_T;

        if (sn->vad_chunk_pos >= sn->cfg->vad_chunk_sz || is_end) {
            int preserved = sn->vad_chunk_pos * sn->cfg->vad_chunk_overlap;
            feed_vad_(sn);
            memmove(sn->vad_chunk,
                    sn->vad_chunk + (sn->vad_chunk_pos - preserved) * L,
                    sizeof(float) * L * preserved);
            sn->vad_chunk_pos = preserved;
        }
    }
}

static float *process_angle_out_(qtk_sonicnet_t *sn, qtk_nnrt_value_t val,
                                 int D, int *K) {
    float *angle_prob;
    int64_t shape[5];
    qtk_nnrt_value_get_shape(sn->rt, val, shape, 5);
    if (sn->cfg->rt.use_ncnn) {
        shape[3] = shape[3] / D;
        shape[4] = D;
    }
    angle_prob = qtk_nnrt_value_get_data(sn->rt, val);
    *K = shape[2];
    return angle_prob;
}

static int feed_nn_chunk_(qtk_sonicnet_t *sn, int *nframe, float **prob,
                          int is_end) {
    int64_t shape[5];
    int nout = sn->nout;
    int i;
    int K = 0, T, D;
    float *range_prob, *angel_prob = NULL;
    int num_in = qtk_nnrt_get_num_in(sn->rt);
    qtk_nnrt_value_t input_val[64];
    qtk_nnrt_value_t output_val[8];
    char name[16];
    int dim = sn->cfg->D;
    if (num_in >= sizeof(input_val) / sizeof(input_val[0])) {
        return -1;
    }
    feed_input_(sn, input_val + 0);

    for (int i = 1; i < sn->right_context + 1; i++) {
        if (sn->nframe > 0) {
            input_val[i] = sn->cache_value[i - 1];
            if (sn->rt->cfg->use_onnxruntime) {
                qtk_nnrt_feed(sn->rt, input_val[i], i);
            } else {
                char name[16];
                snprintf(name, sizeof(name), "in%d", i);
                qtk_nnrt_feed_s(sn->rt, input_val[i], name);
            }
            qtk_nnrt_value_get_shape(sn->rt, input_val[i], shape, 4);
            sn->cache_value[i - 1] = NULL;
        } else {
            int64_t nelem;
            float *input_data;
            if (sn->rt->cfg->use_onnxruntime) {
                qtk_nnrt_get_input_shape(sn->rt, i, shape, 4);
            } else {
                shape[0] = 1;
                shape[3] = dim;
                shape[1] = sn->in_channels[i - 1];
                dim = dim / sn->subsamples[i - 1];
            }
            shape[2] = sn->rt->cfg->use_onnxruntime ? 1 : 2;
            nelem = TENSOR_NELEM(shape);
            input_data = wtk_heap_zalloc(sn->heap, sizeof(float) * nelem);
            input_val[i] = qtk_nnrt_value_create_external(
                sn->rt, QTK_NNRT_VALUE_ELEM_F32, shape, 4, input_data);
            if (sn->rt->cfg->use_ncnn) {
                sprintf(name, "in%d", i);
                qtk_nnrt_feed_s(sn->rt, input_val[i], name);
            } else {
                qtk_nnrt_feed(sn->rt, input_val[i], i);
            }
        }
    }

    if (sn->rt->cfg->use_onnxruntime) {
        for (int i = sn->right_context + 1; i < num_in; i++) {
            qtk_nnrt_get_input_shape(sn->rt, i, shape, 4);
            if (is_end) {
                int64_t nelem;
                float *fake_input;
                shape[2] = 1;
                nelem = TENSOR_NELEM(shape);
                fake_input = wtk_heap_zalloc(sn->heap, sizeof(float) * nelem);
                input_val[i] = qtk_nnrt_value_create_external(
                    sn->rt, QTK_NNRT_VALUE_ELEM_F32, shape, 4, fake_input);
                qtk_nnrt_feed(sn->rt, input_val[i], i);
            } else {
                shape[2] = 0;
                input_val[i] = qtk_nnrt_value_create_external(
                    sn->rt, QTK_NNRT_VALUE_ELEM_F32, shape, 4, NULL);
                qtk_nnrt_feed(sn->rt, input_val[i], i);
            }
        }
    }

    qtk_nnrt_run(sn->rt);

    if (sn->rt->cfg->use_onnxruntime) {
        for (int i = 0; i < sn->right_context; i++) {
            qtk_nnrt_get_output(sn->rt, sn->cache_value + i, i + nout);
        }
    } else {
        for (int i = 0; i < sn->right_context; i++) {
            snprintf(name, sizeof(name), "out%d", i + nout);
            qtk_nnrt_get_output_s(sn->rt, sn->cache_value + i, name);
        }
    }

    for (i = 0; i < num_in; i++) {
        qtk_nnrt_value_release(sn->rt, input_val[i]);
    }

    if (sn->rt->cfg->use_onnxruntime) {
        qtk_nnrt_get_output(sn->rt, output_val + 0, 0);
    } else {
        qtk_nnrt_get_output_s(sn->rt, output_val + 0, "out0");
    }
    qtk_nnrt_value_get_shape(sn->rt, output_val[0], shape, 4);
    T = *nframe = shape[2];
    D = shape[3];
    range_prob = qtk_nnrt_value_get_data(sn->rt, output_val[0]);

    if (nout == 2) {
        if (sn->cfg->rt.use_onnxruntime) {
            qtk_nnrt_get_output(sn->rt, output_val + 1, 1);
        } else {
            qtk_nnrt_get_output_s(sn->rt, output_val + 1, "out1");
        }
        if (sn->cfg->channel == 1) {
            process_encoder_out_(sn, output_val[1], is_end);
        } else {
            angel_prob = process_angle_out_(sn, output_val[1], D, &K);
        }
    } else if (nout == 3) {
        if (sn->cfg->rt.use_onnxruntime) {
            qtk_nnrt_get_output(sn->rt, output_val + 1, 1);
            qtk_nnrt_get_output(sn->rt, output_val + 2, 2);
        } else {
            qtk_nnrt_get_output_s(sn->rt, output_val + 1, "out1");
            qtk_nnrt_get_output_s(sn->rt, output_val + 2, "out2");
        }
        angel_prob = process_angle_out_(sn, output_val[1], D, &K);
        process_encoder_out_(sn, output_val[2], is_end);
    } else {
        wtk_debug("Never should be here\n");
    }

    *prob = nn_post_(sn, range_prob, angel_prob, K, T, D);

    for (i = 0; i < sn->nout; i++) {
        qtk_nnrt_value_release(sn->rt, output_val[i]);
    }

    if (sn->rt->cfg->use_ncnn) {
        if (sn->nframe < sn->right_context) {
            *nframe = 0;
        }
        qtk_nnrt_reset(sn->rt);
    }

    return 0;
}

static int feed_nn_(qtk_sonicnet_t *sn, wtk_complex_t *cur,
                    wtk_complex_t *dyn_cir, int *nframe, float **prob,
                    int is_end) {
    wtk_complex_t *nxt;
    int chunk_sz = sn->nframe == 0 ? sn->cfg->chunk_sz[0]
                                   : sn->cfg->chunk_sz[sn->cfg->nchunk_sz - 1];
    int N = sn->cfg->channel * sn->cfg->nzc;

    nxt = wtk_robin_next(sn->chunks);
    memcpy(nxt, cur, N * sizeof(wtk_complex_t));
    memcpy(nxt + N, dyn_cir, N * sizeof(wtk_complex_t));

    if (sn->chunks->used == chunk_sz || is_end) {
        feed_nn_chunk_(sn, nframe, prob, is_end);
        wtk_robin_reset(sn->chunks);
        sn->nframe++;
    }
    return sn->cur_act_hint;
}

int qtk_sonicnet_feed(qtk_sonicnet_t *sn, wtk_complex_t *cfr, int *nframe,
                      float **prob) {
    wtk_complex_t *nxt, *cur;
    int N = sn->cfg->channel * sn->cfg->nzc;
    *nframe = 0;
    *prob = NULL;

    if (!sn->ctx) {
        wtk_debug("Not Impl\n");
        exit(0);
        return -1;
    }

    wtk_heap_reset(sn->heap);
    nxt = wtk_robin_next(sn->ctx);

    if (sn->cfg->use_ifft) {
        for (int i = 0; i < sn->cfg->channel; i++) {
            wtk_complex_t *cir = wtk_cfft_ifft(sn->fft, cfr + i * sn->cfg->nzc);
            memcpy(nxt + i * sn->cfg->nzc, cir,
                   sizeof(wtk_complex_t) * sn->cfg->nzc);
        }
    } else {
        memcpy(nxt, cfr, sizeof(wtk_complex_t) * N);
    }

    if (sn->ctx->used < sn->ctx->nslot) {
        return 0;
    }

    memset(sn->cir_tmp, 0, sizeof(wtk_complex_t) * N);
    for (int i = 0; i < sn->ctx->used; i++) {
        cur = wtk_robin_at(sn->ctx, i);
        for (int j = 0; j < N; j++) {
            sn->cir_tmp[j].a += cur[j].a;
            sn->cir_tmp[j].b += cur[j].b;
        }
    }
    cur = wtk_robin_at(sn->ctx, sn->cfg->left_context);
    for (int i = 0; i < N; i++) {
        sn->dyn_cir[i].a = (cur[i].a - sn->cir_tmp[i].a / sn->ctx->used) *
                           sn->cfg->dynamic_scaler;
        sn->dyn_cir[i].b = (cur[i].b - sn->cir_tmp[i].b / sn->ctx->used) *
                           sn->cfg->dynamic_scaler;
    }

    if (sn->left_padding == 0) {
        for (int i = 0; i < sn->cfg->left_context; i++) {
            feed_nn_(sn, cur, sn->dyn_cir, nframe, prob, 0);
        }
        sn->left_padding = 1;
    }
    return feed_nn_(sn, cur, sn->dyn_cir, nframe, prob, 0);
}

int qtk_sonicnet_feed_end(qtk_sonicnet_t *sn, int *nframe, float **prob) {
    *nframe = 0;
    *prob = NULL;
    wtk_strbuf_t *prob_buf = wtk_strbuf_new(1024, 1);
    wtk_strbuf_t *angle_prob_buf;
    int nout = sn->nout;
    int K = sn->cfg->K, D = sn->cfg->D;
    if (nout >= 3) {
        angle_prob_buf = wtk_strbuf_new(2048, 1);
    }
    if (sn->nframe == 0) {
        return -1;
    }
    for (int i = 1 + sn->cfg->left_context; i < sn->ctx->used; i++) {
        int nframe_;
        float *prob_;
        wtk_complex_t *cur = wtk_robin_at(sn->ctx, i);
        feed_nn_(sn, cur, sn->dyn_cir, &nframe_, &prob_,
                 i == sn->ctx->used - 1);
        *nframe += nframe_;
        wtk_strbuf_push(prob_buf, (char *)prob_, nframe_ * D * sizeof(float));
        if (nout >= 3) {
            wtk_strbuf_push(angle_prob_buf, (char *)(prob_ + nframe_ * D),
                            nframe_ * K * D * sizeof(float));
        }
    }
    if (nout >= 3) {
        *prob = wtk_heap_malloc(sn->heap, prob_buf->pos + angle_prob_buf->pos);
        memcpy(*prob, prob_buf->data, prob_buf->pos);
        memcpy((char *)*prob + prob_buf->pos, angle_prob_buf->data,
               angle_prob_buf->pos);
        wtk_strbuf_delete(angle_prob_buf);
    } else {
        *prob = wtk_heap_malloc(sn->heap, prob_buf->pos);
        memcpy(*prob, prob_buf->data, prob_buf->pos);
    }
    wtk_strbuf_delete(prob_buf);
    return 0;
}

#undef ASSIGN_SHAPE
#undef TENSOR_NELEM
