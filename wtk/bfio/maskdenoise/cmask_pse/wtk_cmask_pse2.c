#include "wtk/bfio/maskdenoise/cmask_pse/wtk_cmask_pse2.h"
#include "qtk/math/qtk_vector.h"
#include "qtk/nnrt/qtk_nnrt.h"
#include "qtk/nnrt/qtk_nnrt_value.h"
#include "qtk/numeric/qtk_numeric_type.h"
#include "wtk/core/wtk_alloc.h"
#ifndef WTK_WAV_SHORT_TO_FLOAT
#define WTK_WAV_SHORT_TO_FLOAT(f) ((f) > 0 ? (f / 32767.0) : (f / 32768.0))
#endif
#ifndef WTK_WAV_FLOAT_TO_SHORT
#define WTK_WAV_FLOAT_TO_SHORT(f)                                              \
    ((f) > 0 ? floorf(f * 32767.0 + 0.5) : floorf(f * 32768.0 + 0.5))
#endif

void wtk_cmask_pse2_print_type_info(wtk_cmask_pse2_t *cmask_pse2) {
#ifdef ONNX_DEC
    qtk_onnxruntime_t *onnx = cmask_pse2->emb;
    wtk_strbuf_t *fbank_buf = cmask_pse2->fbank_buf;
    wtk_strbuf_t *vp_buf = cmask_pse2->vp_buf;
    OrtStatus *status;
    OrtTypeInfo **type_info =
        (OrtTypeInfo **)wtk_malloc(sizeof(OrtTypeInfo *) * onnx->num_out);
    OrtTypeInfo **type_info2 =
        (OrtTypeInfo **)wtk_malloc(sizeof(OrtTypeInfo *) * onnx->num_in);
    OrtTensorTypeAndShapeInfo *tensor_info2 =
        NULL; //(OrtTensorTypeAndShapeInfo**)wtk_malloc(sizeof(OrtTensorTypeAndShapeInfo*)*onnx->num_in);

    int i, j;
    int len;
    int outer_in_num = onnx->cfg->outer_in_num;
    onnx->stream_len = 0;

    onnx->in_items =
        (qtk_onnx_item_t *)wtk_calloc(onnx->num_in, sizeof(qtk_onnx_item_t));
    for (i = 0; i < onnx->num_in; ++i) {
        qtk_onnx_item_t *item = onnx->in_items + i;
        if (i < outer_in_num) {
            len = 1;
            // wtk_debug("%d %d\n",onnx->num_in,i);
            status = onnx->api->SessionGetInputTypeInfo(onnx->session, i,
                                                        type_info2);
            status = onnx->api->CastTypeInfoToTensorInfo(
                *type_info2,
                cast(const OrtTensorTypeAndShapeInfo **, &tensor_info2));

            // wtk_debug("%p\n",tensor_info2);
            // item->name = (char*)wtk_malloc(sizeof(char*));
            status = onnx->api->SessionGetInputName(
                onnx->session, i, onnx->allocator, &(item->name));

            status = onnx->api->GetDimensionsCount(
                tensor_info2, cast(size_t *, &(item->shape_len)));
            item->shape =
                (int64_t *)wtk_calloc(item->shape_len, sizeof(int64_t));
            status = onnx->api->GetDimensions(tensor_info2, item->shape,
                                              item->shape_len);
            if (i == 0) {
                len = fbank_buf->pos / sizeof(float);
                for (j = 0; j < item->shape_len; j++) {
                    if (item->shape[j] != -1) {
                        len /= item->shape[j];
                    }
                }
                for (j = 0; j < item->shape_len; j++) {
                    if (item->shape[j] == -1) {
                        item->shape[j] = len;
                    }
                }
                len = fbank_buf->pos / sizeof(float);
            } else {
                len = vp_buf->pos / sizeof(float);
                for (j = 0; j < item->shape_len; j++) {
                    if (item->shape[j] != -1) {
                        len /= item->shape[j];
                    }
                }
                for (j = 0; j < item->shape_len; j++) {
                    if (item->shape[j] == -1) {
                        item->shape[j] = len;
                    }
                }
                len = vp_buf->pos / sizeof(float);
            }

            status =
                onnx->api->GetTensorElementType(tensor_info2, &(item->type));
            onnx->api->ReleaseTypeInfo(*type_info2);
            item->in_dim = len;
        } else {
            len = 1;
            // wtk_debug("%d %d\n",onnx->num_in,i);
            status = onnx->api->SessionGetInputTypeInfo(onnx->session, i,
                                                        type_info2);
            status = onnx->api->CastTypeInfoToTensorInfo(
                *type_info2,
                cast(const OrtTensorTypeAndShapeInfo **, &tensor_info2));

            // wtk_debug("%p\n",tensor_info2);
            // item->name = (char*)wtk_malloc(sizeof(char*));
            status = onnx->api->SessionGetInputName(
                onnx->session, i, onnx->allocator, &(item->name));

            status = onnx->api->GetDimensionsCount(
                tensor_info2, cast(size_t *, &(item->shape_len)));
            item->shape =
                (int64_t *)wtk_calloc(item->shape_len, sizeof(int64_t));
            status = onnx->api->GetDimensions(tensor_info2, item->shape,
                                              item->shape_len);
            for (j = 0; j < item->shape_len; j++) {
                if (item->shape[j] == -1) {
                    item->shape[j] = 1;
                }
                len *= item->shape[j];
            }
            status =
                onnx->api->GetTensorElementType(tensor_info2, &(item->type));
            onnx->api->ReleaseTypeInfo(*type_info2);
            item->in_dim = len;
        }
        switch (item->type) {
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT:
            item->bytes = sizeof(float);
            break;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32:
            item->bytes = sizeof(int);
            break;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64:
            item->bytes = sizeof(int64_t);
            break;
        default:
            break;
        }
        if (i < outer_in_num) {
            item->val =
                (char *)wtk_calloc(item->bytes * item->in_dim, sizeof(char));
        } else {
            onnx->stream_len += item->bytes * item->in_dim;
        }
    }

    if (onnx->stream_len > 0) {
        long unsigned int p_len = 0;
        onnx->stream_val = (char *)wtk_calloc(onnx->stream_len, sizeof(char));
        for (i = outer_in_num; i < onnx->num_in; ++i) {
            qtk_onnx_item_t *item = onnx->in_items + i;
            item->val = onnx->stream_val + p_len;
            p_len += item->bytes * item->in_dim;
        }
    }

    // for(i=0; i<onnx->num_out; ++i)
    // {
    // 	ONNXType type;
    // 	status =
    // onnx->api->SessionGetOutputTypeInfo(onnx->session,i,type_info+i);
    // status = onnx->api->GetOnnxTypeFromTypeInfo(type_info[i],&type);
    // 	//wtk_debug("output:%d type:%d\n",i,type);
    // }
    (void)status;

    wtk_free(type_info);
    wtk_free(type_info2);
#endif
}

void wtk_cmask_pse2_fbank_on(wtk_cmask_pse2_t *cmask_pse2, float *data,
                             int len) {
    wtk_strbuf_push(cmask_pse2->fbank_buf, (char *)data, len * sizeof(float));
    int i;
    for (i = 0; i < len; ++i) {
        cmask_pse2->fbank_mean[i] += data[i];
    }
    ++cmask_pse2->fbank_frame;
}

static void cmask_prepare_pse_nnrt_(wtk_cmask_pse2_t *cmask_pse2) {
    int64_t shape[16];
    int i, j, ndim;
    size_t nelem;
    qtk_nnrt_value_elem_type_t elem_typ = QTK_NNRT_VALUE_ELEM_F32;
    int elem_sz = sizeof(float);
    cmask_pse2->ninput_elems =
        wtk_malloc(sizeof(size_t) * cmask_pse2->pse_rt->num_in);
    if (cmask_pse2->pse_rt->cfg->use_rknpu) {
        elem_sz = sizeof(uint16_t);
        elem_typ = QTK_NNRT_VALUE_ELEM_F16;
    }
    cmask_pse2->pse_rt_input_data =
        wtk_malloc(sizeof(void *) * cmask_pse2->pse_rt->num_in);
    cmask_pse2->pse_rt_input =
        wtk_malloc(sizeof(qtk_nnrt_value_t) * cmask_pse2->pse_rt->num_in);
    for (i = 0; i < cmask_pse2->pse_rt->num_in; i++) {
        nelem = 1;
        ndim = qtk_nnrt_get_input_shape(cmask_pse2->pse_rt, i, shape,
                                        sizeof(shape) / sizeof(shape[0]));
        for (j = 0; j < ndim; j++) {
            nelem *= shape[j];
        }
        cmask_pse2->ninput_elems[i] = nelem;
        cmask_pse2->pse_rt_input_data[i] = wtk_malloc(elem_sz * nelem);
        cmask_pse2->pse_rt_input[i] = qtk_nnrt_value_create_external(
            cmask_pse2->pse_rt, elem_typ, shape, ndim,
            cmask_pse2->pse_rt_input_data[i]);
    }
}

static void cmask_reset_pse_nnrt_(wtk_cmask_pse2_t *cmask_pse2) {
    int i;
    int elem_sz =
        cmask_pse2->pse_rt->cfg->use_rknpu ? sizeof(uint16_t) : sizeof(float);
    for (i = 0; i < cmask_pse2->pse_rt->num_in; i++) {
        memset(cmask_pse2->pse_rt_input_data[i], 0,
               elem_sz * cmask_pse2->ninput_elems[i]);
    }
}

static void cmask_cleanup_pse_nnrt_(wtk_cmask_pse2_t *cmask_pse2) {
    int i;
    for (i = 0; i < cmask_pse2->pse_rt->num_in; i++) {
        qtk_nnrt_value_release(cmask_pse2->pse_rt, cmask_pse2->pse_rt_input[i]);
        wtk_free(cmask_pse2->pse_rt_input_data[i]);
    }
    wtk_free(cmask_pse2->pse_rt_input_data);
    wtk_free(cmask_pse2->ninput_elems);
    wtk_free(cmask_pse2->pse_rt_input);
}

wtk_cmask_pse2_t *wtk_cmask_pse2_new(wtk_cmask_pse2_cfg_t *cfg) {
    wtk_cmask_pse2_t *cmask_pse2;

    cmask_pse2 = (wtk_cmask_pse2_t *)wtk_malloc(sizeof(wtk_cmask_pse2_t));
    cmask_pse2->cfg = cfg;
    cmask_pse2->ths = NULL;
    cmask_pse2->notify = NULL;
    cmask_pse2->ths2 = NULL;
    cmask_pse2->notify2 = NULL;
    cmask_pse2->mic = wtk_strbufs_new(cmask_pse2->cfg->nmicchannel);
    cmask_pse2->sp = wtk_strbufs_new(cmask_pse2->cfg->nspchannel);

    cmask_pse2->nbin = cfg->wins / 2 + 1;
    cmask_pse2->analysis_window = wtk_malloc(sizeof(float) * cfg->wins); /// 2);
    cmask_pse2->synthesis_window =
        wtk_malloc(sizeof(float) * cfg->wins); /// 2);
    cmask_pse2->analysis_mem =
        wtk_float_new_p2(cfg->nmicchannel, cmask_pse2->nbin - 1);
    cmask_pse2->analysis_mem_sp =
        wtk_float_new_p2(cfg->nspchannel, cmask_pse2->nbin - 1);
    cmask_pse2->synthesis_mem =
        wtk_malloc(sizeof(float) * (cmask_pse2->nbin - 1));
    cmask_pse2->rfft = wtk_drft_new(cfg->wins);
    cmask_pse2->rfft_in = (float *)wtk_malloc(sizeof(float) * (cfg->wins));

    cmask_pse2->fft =
        wtk_complex_new_p2(cfg->nmicchannel, cmask_pse2->nbin * cfg->num_frame);
    cmask_pse2->fft_sp =
        wtk_complex_new_p2(cfg->nspchannel, cmask_pse2->nbin * cfg->num_frame);
    cmask_pse2->fftx = (wtk_complex_t *)wtk_malloc(
        sizeof(wtk_complex_t) * cmask_pse2->nbin * cfg->num_frame);
    cmask_pse2->ffty = (wtk_complex_t *)wtk_malloc(
        sizeof(wtk_complex_t) * cmask_pse2->nbin * cfg->num_frame);

    cmask_pse2->out = wtk_malloc(sizeof(float) * (cmask_pse2->nbin - 1));

    cmask_pse2->emb_feat = NULL;
    cmask_pse2->gb_feat = NULL;
    cmask_pse2->feat = NULL;

    cmask_pse2->pse_in =
        (float *)wtk_malloc(sizeof(float) * cmask_pse2->nbin * cfg->num_frame);
    cmask_pse2->mask =
        (float *)wtk_malloc(sizeof(float) * cmask_pse2->nbin * cfg->num_frame);

    cmask_pse2->pse_rt = qtk_nnrt_new(&(cmask_pse2->cfg->pse_rt));
    cmask_prepare_pse_nnrt_(cmask_pse2);
    cmask_pse2->mask_bf = NULL;
    if (cfg->use_bf) {
        cmask_pse2->mask_bf = wtk_mask_bf_new(&(cfg->mask_bf));
    }
    wtk_cmask_pse2_reset(cmask_pse2);

    return cmask_pse2;
}

void wtk_cmask_pse2_reset(wtk_cmask_pse2_t *cmask_pse2) {
    int wins = cmask_pse2->cfg->wins;
    int nbin = cmask_pse2->nbin;
    int num_frame = cmask_pse2->cfg->num_frame;
    int nmicchannel = cmask_pse2->cfg->nmicchannel;
    int nspchannel = cmask_pse2->cfg->nspchannel;
    int i;

    wtk_strbufs_reset(cmask_pse2->mic, nmicchannel);
    wtk_strbufs_reset(cmask_pse2->sp, nspchannel);

    for (i = 0; i < wins; ++i) {
        cmask_pse2->analysis_window[i] = sin((0.5 + i) * PI / (wins));
    }
    wtk_drft_init_synthesis_window(cmask_pse2->synthesis_window,
                                   cmask_pse2->analysis_window, wins);

    wtk_float_zero_p2(cmask_pse2->analysis_mem, nmicchannel, nbin - 1);
    wtk_float_zero_p2(cmask_pse2->analysis_mem_sp, nspchannel, nbin - 1);
    memset(cmask_pse2->synthesis_mem, 0, sizeof(float) * (nbin - 1));

    wtk_complex_zero_p2(cmask_pse2->fft, nmicchannel, nbin * num_frame);
    wtk_complex_zero_p2(cmask_pse2->fft_sp, nspchannel, nbin * num_frame);
    memset(cmask_pse2->fftx, 0, sizeof(wtk_complex_t) * nbin * num_frame);
    memset(cmask_pse2->ffty, 0, sizeof(wtk_complex_t) * nbin * num_frame);
    cmask_reset_pse_nnrt_(cmask_pse2);

    memset(cmask_pse2->pse_in, 0, sizeof(float) * nbin * num_frame);
    memset(cmask_pse2->mask, 0, sizeof(float) * nbin * num_frame);

    if (cmask_pse2->mask_bf) {
        wtk_mask_bf_reset(cmask_pse2->mask_bf);
    }

    cmask_pse2->sp_silcnt = 0;
    cmask_pse2->sp_sil = 1;
    cmask_pse2->mic_silcnt = 0;
    cmask_pse2->mic_sil = 1;

    cmask_pse2->bs_scale = 1.0;
    cmask_pse2->bs_last_scale = 1.0;
    cmask_pse2->bs_max_cnt = 0;

    cmask_pse2->feat_len = cmask_pse2->cfg->emb_len + cmask_pse2->cfg->gb_len;
    cmask_pse2->nframe = 0;
    cmask_pse2->feed_frame = 0;
    cmask_pse2->frame_pos = 0;
}

void wtk_cmask_pse2_delete(wtk_cmask_pse2_t *cmask_pse2) {
    int nmicchannel = cmask_pse2->cfg->nmicchannel;
    int nspchannel = cmask_pse2->cfg->nspchannel;

    wtk_strbufs_delete(cmask_pse2->mic, nmicchannel);
    wtk_strbufs_delete(cmask_pse2->sp, nspchannel);

    wtk_free(cmask_pse2->analysis_window);
    wtk_free(cmask_pse2->synthesis_window);
    wtk_float_delete_p2(cmask_pse2->analysis_mem, nmicchannel);
    wtk_float_delete_p2(cmask_pse2->analysis_mem_sp, nspchannel);
    wtk_free(cmask_pse2->synthesis_mem);
    wtk_free(cmask_pse2->rfft_in);
    wtk_drft_delete(cmask_pse2->rfft);
    wtk_complex_delete_p2(cmask_pse2->fft, nmicchannel);
    wtk_complex_delete_p2(cmask_pse2->fft_sp, nspchannel);

    wtk_free(cmask_pse2->fftx);
    wtk_free(cmask_pse2->ffty);

    wtk_free(cmask_pse2->out);
    cmask_cleanup_pse_nnrt_(cmask_pse2);
    qtk_nnrt_delete(cmask_pse2->pse_rt);
    if (cmask_pse2->emb_feat) {
        wtk_free(cmask_pse2->emb_feat);
    }
    if (cmask_pse2->gb_feat) {
        wtk_free(cmask_pse2->gb_feat);
    }
    if (cmask_pse2->feat) {
        wtk_free(cmask_pse2->feat);
    }

    wtk_free(cmask_pse2->pse_in);
    wtk_free(cmask_pse2->mask);

    if (cmask_pse2->mask_bf) {
        wtk_mask_bf_delete(cmask_pse2->mask_bf);
    }

    wtk_free(cmask_pse2);
}

void wtk_cmask_pse2_start(wtk_cmask_pse2_t *cmask_pse2) {}

void wtk_cmask_pse2_set_notify(wtk_cmask_pse2_t *cmask_pse2, void *ths,
                               wtk_cmask_pse2_notify_f notify) {
    cmask_pse2->notify = notify;
    cmask_pse2->ths = ths;
}

void wtk_cmask_pse2_new_vp(wtk_cmask_pse2_t *cmask_pse2) {
#ifdef ONNX_DEC
    cmask_pse2->emb = NULL;
    cmask_pse2->emb_caches = NULL;
    cmask_pse2->emb_out_len = NULL;
    if (cmask_pse2->cfg->use_onnx) {
        cmask_pse2->emb = qtk_onnxruntime_new(&(cmask_pse2->cfg->emb));
        // cmask_pse2->emb_caches = wtk_calloc(sizeof(OrtValue *),
        // cmask_pse2->emb->num_in - cmask_pse2->cfg->emb.outer_in_num);
        if (cmask_pse2->emb->num_in - cmask_pse2->cfg->emb.outer_in_num !=
            cmask_pse2->emb->num_out - cmask_pse2->cfg->emb.outer_out_num) {
            wtk_debug("err inner_item\n");
            exit(0);
        }
        cmask_pse2->emb_out_len = (int *)wtk_malloc(
            sizeof(int) * (cmask_pse2->cfg->emb.outer_out_num));
    }
#endif

    cmask_pse2->fbank = wtk_fbank_new(&(cmask_pse2->cfg->fbank));
    cmask_pse2->fbank_len = cmask_pse2->cfg->fbank.num_fbank;
    cmask_pse2->fbank_mean =
        (float *)wtk_malloc(sizeof(float) * cmask_pse2->fbank_len);
    cmask_pse2->fbank_buf = wtk_strbuf_new(80, 1);
    cmask_pse2->vp_buf = wtk_strbuf_new(1024, 1);
    wtk_fbank_set_notify(cmask_pse2->fbank, cmask_pse2,
                         (wtk_fbank_notify_f)wtk_cmask_pse2_fbank_on);

    wtk_fbank_reset(cmask_pse2->fbank);
    memset(cmask_pse2->fbank_mean, 0, sizeof(float) * cmask_pse2->fbank_len);
    wtk_strbuf_reset(cmask_pse2->fbank_buf);
    wtk_strbuf_reset(cmask_pse2->vp_buf);
    cmask_pse2->fbank_frame = 0;
}

void wtk_cmask_pse2_reset_vp(wtk_cmask_pse2_t *cmask_pse2) {
#ifdef ONNX_DEC
    if (cmask_pse2->cfg->use_onnx) {
        qtk_onnxruntime_reset(cmask_pse2->emb);
        // {
        //     int n = cmask_pse2->emb->num_in -
        //     cmask_pse2->emb->cfg->outer_in_num; if
        //     (cmask_pse2->emb_caches[0]) {
        //         int i;
        //         for (i = 0; i < n; i++) {
        //             cmask_pse2->emb->api->ReleaseValue(cmask_pse2->emb_caches[i]);
        //         }
        //         memset(cmask_pse2->emb_caches, 0, sizeof(OrtValue *) * n);
        //     }
        // }
        memset(cmask_pse2->emb_out_len, 0,
               sizeof(int) * (cmask_pse2->emb->cfg->outer_out_num));
    }
#endif
}

void wtk_cmask_pse2_info_clean(wtk_cmask_pse2_t *cmask_pse2) {
#ifdef ONNX_DEC
    int i;
    qtk_onnxruntime_t *onnx = cmask_pse2->emb;
    qtk_onnx_item_t *item;

    for (i = 0; i < onnx->num_in; ++i) {
        item = onnx->in_items + i;
        if (i < onnx->cfg->outer_in_num) {
            wtk_free(item->val);
        }
        if (item->name != NULL) {
            wtk_free(item->name);
        }
        wtk_free(item->shape);
    }
    wtk_free(onnx->in_items);
#endif
}

void wtk_cmask_pse2_delete_vp(wtk_cmask_pse2_t *cmask_pse2) {
#ifdef ONNX_DEC
    if (cmask_pse2->cfg->use_onnx) {
        // {
        //     int n = cmask_pse2->emb->num_in -
        //     cmask_pse2->emb->cfg->outer_in_num; if
        //     (cmask_pse2->emb_caches[0]) {
        //         int i;
        //         for (i = 0; i < n; i++) {
        //             cmask_pse2->emb->api->ReleaseValue(cmask_pse2->emb_caches[i]);
        //         }
        //     }
        // }
        if (cmask_pse2->emb) {
            wtk_cmask_pse2_info_clean(cmask_pse2);
            qtk_onnxruntime_delete(cmask_pse2->emb);
        }
        wtk_free(cmask_pse2->emb_out_len);
        // wtk_free(cmask_pse2->emb_caches);
    }
#endif
    wtk_fbank_delete(cmask_pse2->fbank);
    wtk_free(cmask_pse2->fbank_mean);
    wtk_strbuf_delete(cmask_pse2->fbank_buf);
    wtk_strbuf_delete(cmask_pse2->vp_buf);
}
void wtk_cmask_pse2_start_vp(wtk_cmask_pse2_t *cmask_pse2) {}

void wtk_cmask_pse2_set_notify2(wtk_cmask_pse2_t *cmask_pse2, void *ths,
                                wtk_cmask_pse2_notify_f2 notify) {
    cmask_pse2->notify2 = notify;
    cmask_pse2->ths2 = ths;
}

void wtk_cmask_pse2_feed_emb(wtk_cmask_pse2_t *cmask_pse2) {
#ifdef ONNX_DEC
    wtk_strbuf_t *fbank_buf = cmask_pse2->fbank_buf;
    wtk_strbuf_t *vp_buf = cmask_pse2->vp_buf;
    int i, j;
    const OrtApi *api = cmask_pse2->emb->api;
    OrtMemoryInfo *meminfo = cmask_pse2->emb->meminfo;
    qtk_onnxruntime_t *emb = cmask_pse2->emb;
    OrtStatus *status;
    int num_in = emb->num_in;
    int outer_in_num = emb->cfg->outer_in_num;
    int outer_out_num = emb->cfg->outer_out_num;
    qtk_onnx_item_t *item;
    void *onnx_out;
    int feat_state;
    int emb_state;
    int gb_state;
    int tmp_len;
    // int64_t size = 0, *out_shape;

    for (i = 0; i < outer_in_num; ++i) {
        item = emb->in_items + i;
        if (i == 0) {
            memcpy(item->val, fbank_buf->data, item->bytes * item->in_dim);
        } else if (i == 1) {
            memcpy(item->val, vp_buf->data, item->bytes * item->in_dim);
        }
    }

    // printf("num_in:\n");
    for (i = 0; i < num_in; ++i) {
        item = emb->in_items + i;
        status = api->CreateTensorWithDataAsOrtValue(
            meminfo, item->val, item->bytes * item->in_dim, item->shape,
            item->shape_len, item->type, emb->in + i);
        // printf("%d\n", i);
        // for(j=0;j<item->shape_len;++j){
        // 	printf("%d %ld\n", j, item->shape[j]);
        // }
        // printf("%ld\n", item->bytes*item->in_dim/sizeof(float));
    }

    status = api->Run(
        emb->session, NULL, cast(const char *const *, emb->name_in),
        cast(const OrtValue *const *, emb->in), emb->num_in,
        cast(const char *const *, emb->name_out), emb->num_out, emb->out);

    cmask_pse2->feat_len = 0;
    emb_state = -1;
    gb_state = -1;
    for (j = 0; j < outer_out_num; ++j) {
        feat_state = -1;
        if (cmask_pse2->emb_out_len[j] == 0) {
            int64_t size = 0, *out_shape;
            int d_len;
            d_len = 1;
            out_shape = qtk_onnxruntime_get_outshape(emb, j, &size);
            for (int k = 0; k < size; ++k) {
                d_len *= out_shape[k];
            }
            wtk_free(out_shape);
            cmask_pse2->emb_out_len[j] = d_len;
        }
        if (!cmask_pse2->emb_feat && cmask_pse2->cfg->emb_len > 0 &&
            feat_state == -1) {
            cmask_pse2->emb_feat =
                (float *)wtk_malloc(sizeof(float) * cmask_pse2->emb_out_len[j]);
            feat_state = j;
            // printf("emb_feat_init:\n");
        }
        if (!cmask_pse2->gb_feat && cmask_pse2->cfg->gb_len > 0 &&
            feat_state == -1) {
            cmask_pse2->gb_feat =
                (float *)wtk_malloc(sizeof(float) * cmask_pse2->emb_out_len[j]);
            feat_state = j;
            // printf("gb_init:\n");
        }
        onnx_out = qtk_onnxruntime_getout(emb, j);
        feat_state = -1;
        if (cmask_pse2->cfg->emb_len > 0 && feat_state == -1 &&
            emb_state == -1) {
            memcpy(cmask_pse2->emb_feat, onnx_out,
                   cmask_pse2->emb_out_len[j] * sizeof(float));
            feat_state = j;
            emb_state = j;
            // printf("emb_feat:\n");
            // for(int k=0;k<cmask_pse2->emb_out_len[j];++k){
            //     printf("%d %f\n", k, cmask_pse2->emb_feat[k]);
            // }
        }
        if (cmask_pse2->cfg->gb_len > 0 && feat_state == -1 && gb_state == -1) {
            memcpy(cmask_pse2->gb_feat, onnx_out,
                   cmask_pse2->emb_out_len[j] * sizeof(float));
            feat_state = j;
            gb_state = j;
            // printf("gb_feat:\n");
            // for(int k=0;k<cmask_pse2->emb_out_len[j];++k){
            //     printf("%d %f\n", k, cmask_pse2->gb_feat[k]);
            // }
        }
        cmask_pse2->feat_len += cmask_pse2->emb_out_len[j];
    }
    if (!cmask_pse2->feat) {
        cmask_pse2->feat =
            (float *)wtk_malloc(sizeof(float) * cmask_pse2->feat_len);
    }
    tmp_len = 0;
    if (cmask_pse2->emb_feat) {
        memcpy(cmask_pse2->feat, cmask_pse2->emb_feat,
               cmask_pse2->cfg->emb_len * sizeof(float));
        tmp_len += cmask_pse2->cfg->emb_len;
    }
    if (cmask_pse2->gb_feat) {
        memcpy(cmask_pse2->feat + tmp_len, cmask_pse2->gb_feat,
               cmask_pse2->cfg->gb_len * sizeof(float));
        tmp_len += cmask_pse2->cfg->gb_len;
    }
    for (i = outer_in_num, j = outer_out_num; i < num_in; ++i, ++j) {
        item = emb->in_items + i;
        onnx_out = qtk_onnxruntime_getout(emb, j);
        memcpy(item->val, onnx_out, item->bytes * item->in_dim);
    }
    qtk_onnxruntime_reset(emb);
    (void)status;
#endif
}

void wtk_cmask_pse2_feed_pse(wtk_cmask_pse2_t *cmask_pse2,
                             wtk_complex_t *fftx) {
    int num_frame = cmask_pse2->cfg->num_frame;
    int i;
    int nbin = cmask_pse2->nbin;
    int pos = cmask_pse2->frame_pos;
    float *mask = cmask_pse2->mask;
    int cache_start_idx = 1;

    cmask_pse2->feed_frame++;

    if (cmask_pse2->pse_rt->cfg->use_rknpu) {
        unsigned short *pse_in = cmask_pse2->pse_rt_input_data[0];
        for (i = 0; i < nbin; ++i) {
            pse_in[(pos + i) * 2 + 0] = float_to_half(
                sqrtf(fftx[i].a * fftx[i].a + fftx[i].b * fftx[i].b));
        }
    } else {
        float *pse_in = cmask_pse2->pse_rt_input_data[0];
        for (i = 0; i < nbin; ++i) {
            pse_in[pos + i] =
                sqrtf(fftx[i].a * fftx[i].a + fftx[i].b * fftx[i].b);
        }
    }

    cmask_pse2->frame_pos += nbin;
    if (cmask_pse2->feed_frame >= num_frame) {
        for (i = 0; i < cmask_pse2->pse_rt->num_in; i++) {
            qtk_nnrt_feed(cmask_pse2->pse_rt, cmask_pse2->pse_rt_input[i], i);
        }
        if (cmask_pse2->pse_rt->cfg->use_rknpu) {
            for (i = cache_start_idx; i < cmask_pse2->pse_rt->num_out; i++) {
                qtk_nnrt_disable_output_dequant(cmask_pse2->pse_rt, i);
            }
        }
        qtk_nnrt_run(cmask_pse2->pse_rt);
        qtk_nnrt_value_t mask_val;
        qtk_nnrt_get_output(cmask_pse2->pse_rt, &mask_val, 0);
        float *mask_out = qtk_nnrt_value_get_data(cmask_pse2->pse_rt, mask_val);
        memcpy(mask, mask_out, sizeof(float) * nbin * num_frame);
        qtk_nnrt_value_release(cmask_pse2->pse_rt, mask_val);

        if (cmask_pse2->pse_rt->cfg->use_rknpu) {
            for (i = cache_start_idx; i < cmask_pse2->pse_rt->num_out; i++) {
                int input_idx =
                    1 + cmask_pse2->cfg->nemb_feat + i - cache_start_idx;
                qtk_nnrt_value_t val;
                qtk_nnrt_get_output(cmask_pse2->pse_rt, &val, i);
                float *data = qtk_nnrt_value_get_data(cmask_pse2->pse_rt, val);
                for (int j = 0; j < cmask_pse2->ninput_elems[input_idx]; j++) {
                    ((unsigned short *)
                         cmask_pse2->pse_rt_input_data[input_idx])[j] =
                        float_to_half(data[j]);
                }
            }
        } else {
            for (i = cache_start_idx; i < cmask_pse2->pse_rt->num_out; i++) {
                int input_idx =
                    1 + cmask_pse2->cfg->nemb_feat + i - cache_start_idx;
                qtk_nnrt_value_release(cmask_pse2->pse_rt,
                                       cmask_pse2->pse_rt_input[input_idx]);
                qtk_nnrt_get_output(cmask_pse2->pse_rt,
                                    &cmask_pse2->pse_rt_input[input_idx], i);
            }
        }

        qtk_nnrt_reset(cmask_pse2->pse_rt);
        cmask_pse2->feed_frame = 0;
        cmask_pse2->frame_pos = 0;
    }
}

void wtk_cmask_pse2_start_vp_feat(wtk_cmask_pse2_t *cmask_pse2, float *feat,
                                  int len) {
    if (feat) {
        int i;
        wtk_cmask_pse2_reset(cmask_pse2);
        for (i = 0; i < cmask_pse2->cfg->nemb_feat; i++) {
            if (cmask_pse2->pse_rt->cfg->use_rknpu) {
                int j;
                for (j = 0; j < cmask_pse2->cfg->emb_feat_len[i]; j++) {
                    ((unsigned short *)
                         cmask_pse2->pse_rt_input_data[1 + i])[j] =
                        float_to_half(feat[j]);
                }
            } else {
                memcpy(cmask_pse2->pse_rt_input_data[1 + i], feat,
                       sizeof(float) * cmask_pse2->cfg->emb_feat_len[i]);
            }
            feat += cmask_pse2->cfg->emb_feat_len[i];
        }
    }
}

void wtk_cmask_pse2_feed_vp(wtk_cmask_pse2_t *cmask_pse2, short *data, int len,
                            int is_end) {
    wtk_strbuf_t *mic = cmask_pse2->mic[0];
    wtk_strbuf_t *vp_buf = cmask_pse2->vp_buf;
    wtk_complex_t *fft = cmask_pse2->fft[0];
    wtk_drft_t *rfft = cmask_pse2->rfft;
    float *rfft_in = cmask_pse2->rfft_in;
    float *analysis_mem = cmask_pse2->analysis_mem[0];
    float *analysis_window = cmask_pse2->analysis_window;
    int nbin = cmask_pse2->nbin;
    float fv;
    int i;
    int length;
    int wins = cmask_pse2->cfg->wins;
    int fsize = wins / 2;

    if (is_end) {
        if (cmask_pse2->fbank_frame == 0) {
            wtk_debug("error need feed data first\n");
            exit(0);
        }
        int pos = 0;
        float *fbank;
        for (i = 0; i < cmask_pse2->fbank_len; ++i) {
            cmask_pse2->fbank_mean[i] /= cmask_pse2->fbank_frame;
        }
        length = cmask_pse2->fbank_buf->pos / sizeof(float);
        while (length > pos) {
            fbank =
                (float *)(cmask_pse2->fbank_buf->data + pos * sizeof(float));
            for (i = 0; i < cmask_pse2->fbank_len; ++i) {
                fbank[i] -= cmask_pse2->fbank_mean[i];
            }
            pos += cmask_pse2->fbank_len;
        }
        wtk_cmask_pse2_print_type_info(cmask_pse2);
        wtk_cmask_pse2_reset_vp(cmask_pse2);
        wtk_cmask_pse2_feed_emb(cmask_pse2);

        if (cmask_pse2->notify2) {
            cmask_pse2->notify2(cmask_pse2->ths2, cmask_pse2->feat,
                                cmask_pse2->feat_len);
            wtk_cmask_pse2_start_vp_feat(cmask_pse2, cmask_pse2->feat,
                                         cmask_pse2->feat_len);
        }
        wtk_strbuf_reset(mic);
    } else {
        wtk_fbank_feed(cmask_pse2->fbank, data, len, is_end);

        for (i = 0; i < len; ++i) {
            fv = WTK_WAV_SHORT_TO_FLOAT(data[i]);
            wtk_strbuf_push(mic, (char *)&(fv), sizeof(float));
        }
        length = mic->pos / sizeof(float);
        while (length >= fsize) {
            wtk_drft_stft(rfft, rfft_in, analysis_mem, fft,
                          (float *)(mic->data), wins, analysis_window);
            for (i = 0; i < nbin; ++i) {
                fv = sqrtf(fft[i].a * fft[i].a + fft[i].b * fft[i].b);
                wtk_strbuf_push(vp_buf, (char *)&(fv), sizeof(float));
            }

            wtk_strbuf_pop(mic, NULL, fsize * sizeof(float));
            length = mic->pos / sizeof(float);
        }
    }
}

void wtk_cmask_pse2_feed_cnon(wtk_cmask_pse2_t *cmask_pse2,
                              wtk_complex_t *fft) {
    int nbin = cmask_pse2->nbin;
    float sym = cmask_pse2->cfg->sym;
    static float fx = 2.0f * PI / RAND_MAX;
    float f, f2;
    int i;

    for (i = 1; i < nbin - 1; ++i) {
        f = rand() * fx;
        f2 = 1.f;
        if (f2 > 0) {
            // f2=sqrtf(f2);
            fft[i].a += sym * cosf(f) * f2;
            fft[i].b += sym * sinf(f) * f2;
        }
    }
}

void wtk_cmask_pse2_control_bs(wtk_cmask_pse2_t *cmask_pse2, float *out,
                               int len) {
    float out_max;
    int i;

    if (cmask_pse2->mic_sil == 0) {
        out_max = wtk_float_abs_max(out, len);
        if (out_max > cmask_pse2->cfg->max_bs_out) {
            cmask_pse2->bs_scale = cmask_pse2->cfg->max_bs_out / out_max;
            if (cmask_pse2->bs_scale < cmask_pse2->bs_last_scale) {
                cmask_pse2->bs_last_scale = cmask_pse2->bs_scale;
            } else {
                cmask_pse2->bs_scale = cmask_pse2->bs_last_scale;
            }
            cmask_pse2->bs_max_cnt = 5;
        }
        for (i = 0; i < len; ++i) {
            out[i] *= cmask_pse2->bs_scale;
        }
        if (cmask_pse2->bs_max_cnt > 0) {
            --cmask_pse2->bs_max_cnt;
        }
        if (cmask_pse2->bs_max_cnt <= 0 && cmask_pse2->bs_scale < 1.0) {
            cmask_pse2->bs_scale *= 1.1f;
            cmask_pse2->bs_last_scale = cmask_pse2->bs_scale;
            if (cmask_pse2->bs_scale > 1.0) {
                cmask_pse2->bs_scale = 1.0;
                cmask_pse2->bs_last_scale = 1.0;
            }
        }
    } else {
        cmask_pse2->bs_scale = 1.0;
        cmask_pse2->bs_last_scale = 1.0;
        cmask_pse2->bs_max_cnt = 0;
    }
}

void wtk_cmask_pse2_feed_mask_bf(wtk_cmask_pse2_t *cmask_pse2) {
    int nbin = cmask_pse2->nbin;
    float *mask = cmask_pse2->mask;
    wtk_mask_bf_t *mask_bf = cmask_pse2->mask_bf;
    wtk_complex_t **fft = cmask_pse2->fft;
    wtk_complex_t *fftx = cmask_pse2->fftx;

    wtk_mask_bf_feed(mask_bf, fft, mask, NULL, NULL, 1);
    memcpy(fftx, mask_bf->fftx, sizeof(wtk_complex_t) * nbin);
}

void wtk_cmask_pse2_feed(wtk_cmask_pse2_t *cmask_pse2, short *data, int len,
                         int is_end) {
    int i, j, n;
    int nmicchannel = cmask_pse2->cfg->nmicchannel;
    int nspchannel = cmask_pse2->cfg->nspchannel;
    int channel = cmask_pse2->cfg->channel;
    int *mic_channel = cmask_pse2->cfg->mic_channel;
    int *sp_channel = cmask_pse2->cfg->sp_channel;
    int wins = cmask_pse2->cfg->wins;
    int fsize = wins / 2;
    int nbin = cmask_pse2->nbin;
    int length;
    wtk_drft_t *rfft = cmask_pse2->rfft;
    float *rfft_in = cmask_pse2->rfft_in;
    wtk_complex_t **fft = cmask_pse2->fft;
    wtk_complex_t **fft_sp = cmask_pse2->fft_sp;
    wtk_complex_t *fftx = cmask_pse2->fftx;
    float *mask = cmask_pse2->mask;
    float **analysis_mem = cmask_pse2->analysis_mem,
          **analysis_mem_sp = cmask_pse2->analysis_mem_sp;
    float *synthesis_mem = cmask_pse2->synthesis_mem;
    float *synthesis_window = cmask_pse2->synthesis_window;
    float *analysis_window = cmask_pse2->analysis_window;
    float *out = cmask_pse2->out;
    short *pv = (short *)out;
    wtk_strbuf_t **mic = cmask_pse2->mic;
    wtk_strbuf_t **sp = cmask_pse2->sp;
    float fv;

    for (i = 0; i < len; ++i) {
        for (j = 0; j < nmicchannel; ++j) {
            fv = WTK_WAV_SHORT_TO_FLOAT(data[mic_channel[j]]);
            wtk_strbuf_push(mic[j], (char *)&(fv), sizeof(float));
        }
        for (j = 0; j < nspchannel; ++j) {
            fv = WTK_WAV_SHORT_TO_FLOAT(data[sp_channel[j]]);
            wtk_strbuf_push(sp[j], (char *)&(fv), sizeof(float));
        }
        data += channel;
    }
    length = mic[0]->pos / sizeof(float);
    while (length >= fsize) {
        ++cmask_pse2->nframe;
        for (i = 0; i < nmicchannel; ++i) {
            wtk_drft_stft(rfft, rfft_in, analysis_mem[i], fft[i],
                          (float *)(mic[i]->data), wins, analysis_window);
        }
        for (i = 0; i < nspchannel; ++i) {
            wtk_drft_stft(rfft, rfft_in, analysis_mem_sp[i], fft_sp[i],
                          (float *)(sp[i]->data), wins, analysis_window);
        }

        if (cmask_pse2->cfg->use_onnx) {
            wtk_cmask_pse2_feed_pse(cmask_pse2, fft[0]);
        } else {
            int num_frame = cmask_pse2->cfg->num_frame;
            cmask_pse2->feed_frame++;
            cmask_pse2->frame_pos += nbin;
            if (cmask_pse2->feed_frame >= num_frame) {
                cmask_pse2->feed_frame = 0;
                cmask_pse2->frame_pos = 0;
            }
        }

        if (cmask_pse2->feed_frame == 0) {
            for (n = 0; n < cmask_pse2->cfg->num_frame; ++n) {
                if (cmask_pse2->cfg->use_bf) {
                    wtk_cmask_pse2_feed_mask_bf(cmask_pse2);
                } else {
                    for (i = 0; i < nbin; ++i) {
                        fftx[i].a = fft[0][i + n * nbin].a * mask[i + n * nbin];
                        fftx[i].b = fft[0][i + n * nbin].b * mask[i + n * nbin];
                    }
                }
                if (cmask_pse2->cfg->use_cnon) {
                    wtk_cmask_pse2_feed_cnon(cmask_pse2, fftx);
                }
                wtk_drft_istft(rfft, rfft_in, synthesis_mem, fftx, out, wins,
                               synthesis_window);
                wtk_cmask_pse2_control_bs(cmask_pse2, out, fsize);
                for (i = 0; i < fsize; ++i) {
                    pv[i] = WTK_WAV_FLOAT_TO_SHORT(out[i]);
                }
                if (cmask_pse2->notify) {
                    cmask_pse2->notify(cmask_pse2->ths, pv, fsize);
                }
            }
        }
        wtk_strbufs_pop(mic, nmicchannel, fsize * sizeof(float));
        wtk_strbufs_pop(sp, nspchannel, fsize * sizeof(float));
        length = mic[0]->pos / sizeof(float);
    }
    if (is_end && length > 0) {
        if (cmask_pse2->notify) {
            pv = (short *)mic[0]->data;
            cmask_pse2->notify(cmask_pse2->ths, pv, length);
        }
    }
}