#include "qtk/nnrt/qtk_nnrt.h"
#include "qtk/nnrt/qtk_nnrt_value.h"
#ifdef QTK_NNRT_LIBTORCH
#include "qtk/nnrt/qtk_nnrt_libtorch.h"
#endif

static unsigned char nnrt_value_elem_sz_[] = {sizeof(float), sizeof(uint16_t),
                                              sizeof(uint8_t), sizeof(int64_t)};

#ifdef QTK_NNRT_SS_IPU
static qtk_nnrt_value_elem_type_t
convert_ss_ipu_elem_type_to_nnrt_(MI_IPU_ELEMENT_FORMAT fmt) {
    return fmt == MI_IPU_FORMAT_FP32 ? QTK_NNRT_VALUE_ELEM_F32 : -1;
}
#endif

#ifdef QTK_NNRT_ONNXRUNTIME

#include "qtk/os/qtk_atomic.h"

static OrtEnv *g_ort_env = NULL;
static OrtThreadingOptions *g_tp_options = NULL;
static int g_ort_env_ref = 0;

#define ONNXRUNTIME_CALL(api, f, err_handler, ...)                             \
    do {                                                                       \
        OrtStatus *status = NULL;                                              \
        status = (api)->f(__VA_ARGS__);                                        \
        if (status) {                                                          \
            printf("Err: %s\n", (api)->GetErrorMessage(status));            \
            err_handler                                                        \
        }                                                                      \
    } while (0);

#define QTK_ONNXRUNTIME_CALL(rt, f, err_handler, ...)                          \
    ONNXRUNTIME_CALL(rt->ort_api, f, err_handler, __VA_ARGS__)

static void nnrt_onnx_clean_up_(qtk_nnrt_t *rt) {
    int i;
    if (rt->ort_allocator) {
        rt->ort_api->ReleaseAllocator(rt->ort_allocator);
    }
    if (g_ort_env) {
        if (QTK_ATOM_DEC(&g_ort_env_ref) == 0) {
            rt->ort_api->ReleaseEnv(g_ort_env);
        }
    } else {
        if (rt->ort_env)
            rt->ort_api->ReleaseEnv(rt->ort_env);
    }
    if (rt->ort_session)
        rt->ort_api->ReleaseSession(rt->ort_session);
    if (rt->ort_options)
        rt->ort_api->ReleaseSessionOptions(rt->ort_options);
    if (rt->ort_meminfo)
        rt->ort_api->ReleaseMemoryInfo(rt->ort_meminfo);
    if (rt->ort_in) {
        wtk_free(rt->ort_in);
    }
    if (rt->ort_input_name) {
        wtk_free(rt->ort_input_name);
    }
    if (rt->ort_output_name) {
        wtk_free(rt->ort_output_name);
    }
    for (i = 0; i < rt->num_out; i++) {
        if (rt->ort_out[i]) {
            rt->ort_api->ReleaseValue(rt->ort_out[i]);
            rt->ort_out[i] = NULL;
        }
    }
    if (rt->ort_out) {
        wtk_free(rt->ort_out);
    }
}

static int nnrt_set_onnx_(qtk_nnrt_t *rt) {
    int i;
    const OrtApiBase *base = OrtGetApiBase();
    rt->ort_api = base->GetApi(ORT_API_VERSION);

    QTK_ONNXRUNTIME_CALL(rt, CreateSessionOptions, goto err;, &rt->ort_options);
    if (g_ort_env) {
        QTK_ATOM_INC(&g_ort_env_ref);
        rt->ort_env = g_ort_env;
        QTK_ONNXRUNTIME_CALL(rt, DisablePerSessionThreads, goto err;
                             , rt->ort_options);
    } else {
        QTK_ONNXRUNTIME_CALL(rt, CreateEnv, goto err;,
                                                     ORT_LOGGING_LEVEL_WARNING,
                                                     "qdreamer", &rt->ort_env);
        QTK_ONNXRUNTIME_CALL(rt, SetInterOpNumThreads, goto err;
                             , rt->ort_options, rt->cfg->num_threads);
        QTK_ONNXRUNTIME_CALL(rt, SetIntraOpNumThreads, goto err;
                             , rt->ort_options, rt->cfg->num_threads);
    }

    if (rt->cfg->enable_profiling) {
        QTK_ONNXRUNTIME_CALL(rt, EnableProfiling, goto err;
                             , rt->ort_options, "qdreamer_nnrt")
    }
    if (rt->cfg->bin_data) {
        QTK_ONNXRUNTIME_CALL(rt, CreateSessionFromArray, goto err;
                             , rt->ort_env, rt->cfg->bin_data->data,
                             rt->cfg->bin_data->len, rt->ort_options,
                             &rt->ort_session);
    } else {
        QTK_ONNXRUNTIME_CALL(rt, CreateSession, goto err;
                             , rt->ort_env, rt->cfg->onnx_fn, rt->ort_options,
                             &rt->ort_session);
    }
    QTK_ONNXRUNTIME_CALL(rt, CreateCpuMemoryInfo, goto err;, OrtArenaAllocator,
                                                           OrtMemTypeDefault,
                                                           &rt->ort_meminfo);
    QTK_ONNXRUNTIME_CALL(rt, CreateAllocator, goto err;, rt->ort_session,
                                                       rt->ort_meminfo,
                                                       &rt->ort_allocator);
    rt->num_in = qtk_nnrt_get_num_in(rt);
    rt->num_out = qtk_nnrt_get_num_out(rt);
    if (rt->num_in < 0 || rt->num_out < 0) {
        goto err;
    }

    rt->ort_in = wtk_calloc(sizeof(void *), rt->num_in);
    rt->ort_out = wtk_calloc(sizeof(void *), rt->num_out);
    rt->ort_input_name = wtk_calloc(sizeof(void *), rt->num_in);
    rt->ort_output_name = wtk_calloc(sizeof(void *), rt->num_out);

    for (i = 0; i < rt->num_in; i++) {
        QTK_ONNXRUNTIME_CALL(rt, SessionGetInputName, goto err;
                             , rt->ort_session, i, rt->ort_allocator,
                             rt->ort_input_name + i);
    }
    for (i = 0; i < rt->num_out; i++) {
        QTK_ONNXRUNTIME_CALL(rt, SessionGetOutputName, goto err;
                             , rt->ort_session, i, rt->ort_allocator,
                             rt->ort_output_name + i);
    }
    return 0;
err:
    nnrt_onnx_clean_up_(rt);
    return -1;
}

int qtk_nnrt_set_global_thread_pool(int num_threads, char *affinity) {
    const OrtApiBase *base = OrtGetApiBase();
    const OrtApi *api = base->GetApi(ORT_API_VERSION);
    if (g_ort_env == NULL) {
        ONNXRUNTIME_CALL(api, CreateThreadingOptions, goto err;, &g_tp_options);
        ONNXRUNTIME_CALL(api, SetGlobalIntraOpNumThreads, goto err;
                         , g_tp_options, num_threads);
        ONNXRUNTIME_CALL(api, SetGlobalInterOpNumThreads, goto err;
                         , g_tp_options, num_threads);
        if (affinity) {
#if ORT_API_VERSION >= 14
            ONNXRUNTIME_CALL(api, SetGlobalIntraOpThreadAffinity, goto err;
                             , g_tp_options, affinity);
#endif
        }
        ONNXRUNTIME_CALL(api, CreateEnvWithGlobalThreadPools, goto err;
                         , ORT_LOGGING_LEVEL_WARNING, "glb_qdreamer",
                         g_tp_options, &g_ort_env);
    }
    return 0;
err:
    return -1;
}
#endif

qtk_nnrt_t *qtk_nnrt_new(qtk_nnrt_cfg_t *cfg) {

    qtk_nnrt_t *rt = wtk_calloc(sizeof(qtk_nnrt_t), 1);
    rt->cfg = cfg;
#ifdef QTK_NNRT_NCNN
    if (cfg->use_ncnn) {
        rt->ncnn_option = ncnn_option_create();
        ncnn_option_set_num_threads(rt->ncnn_option, cfg->num_threads);
        if (!cfg->ncnn_use_winograd_conv) {
            ncnn_option_disable_field(rt->ncnn_option,
                                      "use_winograd_convolution");
        } else {
            ncnn_option_enable_field(rt->ncnn_option,
                                     "use_winograd_convolution");
        }
        if (cfg->enable_profiling) {
            wtk_debug("Warning: enable profiling does not support ncnn\n");
        }
        rt->net = ncnn_net_create();
        ncnn_net_set_option(rt->net, rt->ncnn_option);
        rt->allocator = ncnn_allocator_create_pool_allocator(); // thread unsafe
        if (cfg->bin_data) {
            ncnn_net_load_param_memory(rt->net, cfg->param_data->data);
            ncnn_net_load_model_memory(
                rt->net, (const unsigned char *)cfg->bin_data->data);
        } else {
            ncnn_net_load_param(rt->net, cfg->ncnn_param_fn);
            ncnn_net_load_model(rt->net, cfg->ncnn_model_fn);
        }
        rt->extractor = ncnn_extractor_create(rt->net);
        // ncnn_extractor_set_option(rt->extractor, rt->ncnn_option);
        return rt;
    }
#endif

#ifdef QTK_NNRT_RKNPU
    if(cfg->use_rknpu){
        int ret;
        int i;
        rknn_input_output_num io_num;
        ret = rknn_init(&rt->ctx, cfg->model, 0, 0, NULL);
        if (ret < 0) {
            printf("rknn init failed\n");
            return NULL;
        }

        ret =
            rknn_query(rt->ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
        if (ret < 0) {
            printf("query io num failed\n");
            return NULL;
        }

        rt->num_in = io_num.n_input;
        rt->num_out = io_num.n_output;

        if (rt->num_in > RKNN_MAX_INPUT || rt->num_out > RKNN_MAX_OUTPUT) {
            printf("i/o num exceed limit\n");
            return NULL;
        }

        for (i = 0; i < rt->num_in; i++) {
            rt->input_attrs[i].index = i;
            ret = rknn_query(rt->ctx, RKNN_QUERY_INPUT_ATTR,
                             &(rt->input_attrs[i]), sizeof(rknn_tensor_attr));
            if (ret < 0) {
                printf("Query input attr failed\n");
                return NULL;
            }
            rt->input[i].index = i;
            rt->input[i].buf = NULL;
            rt->input[i].size = rt->input_attrs[i].size;
            rt->input[i].type = rt->input_attrs[i].type;
            rt->input[i].fmt = rt->input_attrs[i].fmt;
        }

        for (i = 0; i < rt->num_out; i++) {
            rt->output_attrs[i].index = i;
            ret = rknn_query(rt->ctx, RKNN_QUERY_OUTPUT_ATTR, &(rt->output_attrs[i]), sizeof(rknn_tensor_attr));
            if (ret < 0) {
                printf("Query input attr failed\n");
                return NULL;
            }
            rt->outputs[i].index = i;
            rt->outputs[i].size = rt->output_attrs[i].size;
            rt->outputs[i].buf = NULL;
            rt->outputs[i].want_float = 1;
            rt->outputs[i].is_prealloc = 0;
        }
        return rt;
    }
#endif

#ifdef QTK_NNRT_ONNXRUNTIME
    if (cfg->use_onnxruntime) {
        if (nnrt_set_onnx_(rt)) {
            goto err;
        }
        return rt;
    }
err:
#endif

#ifdef QTK_NNRT_SS_IPU
    if (cfg->use_ss_ipu) {
        int ret;
        MI_IPUChnAttr_t stChnAttr;
        memset(&stChnAttr, 0, sizeof(stChnAttr));
        stChnAttr.u32InputBufDepth = 1;
        stChnAttr.u32OutputBufDepth = 1;
        ret =
            MI_IPU_CreateCHN(&rt->u32ChannelID, &stChnAttr, NULL, cfg->mdl_fn);
        if (ret != MI_SUCCESS) {
            wtk_debug("MI_IPU_CreateCHN failed: %d\n", ret);
            return NULL;
        }
        ret = MI_IPU_GetInputTensors(rt->u32ChannelID, &rt->InputTensorVector);
        if (ret != MI_SUCCESS) {
            wtk_debug("MI_IPU_GetInputTensors failed: %d\n", ret);
            return NULL;
        }
        ret = MI_IPU_GetOutputTensors(rt->u32ChannelID, &rt->OutputTensorVector);
        if (ret != MI_SUCCESS) {
            wtk_debug("MI_IPU_GetOutputTensors failed: %d\n", ret);
            return NULL;
        }
        ret = MI_IPU_GetInOutTensorDesc(rt->u32ChannelID, &rt->desc);
        if (ret != MI_SUCCESS) {
            wtk_debug("MI_IPU_GetInOutTensorDesc failed: %d\n", ret);
            return NULL;
        }
        rt->num_in = rt->desc.u32InputTensorCount;
        rt->num_out = rt->desc.u32OutputTensorCount;
        return rt;
    }
#endif
#ifdef QTK_NNRT_LIBTORCH
    if (rt->cfg->use_libtorch) {
        rt->libtorch = qtk_nnrt_libtorch_new(rt->cfg);
        return rt;
    }
#endif
    wtk_free(rt);
    return NULL;
}

void qtk_nnrt_delete(qtk_nnrt_t *rt) {
#ifdef QTK_NNRT_NCNN
    if (rt->cfg->use_ncnn) {
        ncnn_extractor_destroy(rt->extractor);
        ncnn_net_destroy(rt->net);
        ncnn_allocator_destroy(rt->allocator);
        ncnn_option_destroy(rt->ncnn_option);
        goto end;
    }
#endif

#ifdef QTK_NNRT_ONNXRUNTIME
    if (rt->cfg->use_onnxruntime) {
        nnrt_onnx_clean_up_(rt);
        goto end;
    }
#endif

#ifdef QTK_NNRT_RKNPU
    if(rt->cfg->use_rknpu){
        // int i;
        // for (i = 0; i < rt->io_num.n_output; i++){
        //     wtk_free(rt->outputs[i].buf);
        // }
        rknn_destroy(rt->ctx);

        goto end;
    }
#endif

#ifdef QTK_NNRT_SS_IPU
    if (rt->cfg->use_ss_ipu) {
        MI_IPU_PutInputTensors(rt->u32ChannelID, &rt->InputTensorVector);
        MI_IPU_PutOutputTensors(rt->u32ChannelID, &rt->OutputTensorVector);
        MI_IPU_DestroyCHN(rt->u32ChannelID);
        goto end;
    }
#endif
#ifdef QTK_NNRT_LIBTORCH
    if (rt->cfg->use_libtorch) {
        qtk_nnrt_libtorch_delete(rt->libtorch);
        goto end;
    }
#endif
end:
    wtk_free(rt);
}

int qtk_nnrt_run(qtk_nnrt_t *rt) {
#ifdef QTK_NNRT_NCNN
    if (rt->cfg->use_ncnn) {
        return 0;
    }
#endif

#ifdef QTK_NNRT_ONNXRUNTIME
    if (rt->cfg->use_onnxruntime) {
        int i;
        for (i = 0; i < rt->num_out; i++) {
            if (!rt->ort_out[i]) {
                continue;
            }
            rt->ort_api->ReleaseValue(rt->ort_out[i]);
            rt->ort_out[i] = NULL;
        }

        QTK_ONNXRUNTIME_CALL(
            rt, Run, goto err;
            , rt->ort_session, NULL, (const char *const *)rt->ort_input_name,
            (const OrtValue *const *)rt->ort_in, rt->num_in,
            (const char *const *)rt->ort_output_name, rt->num_out, rt->ort_out);

        return 0;
    }
err:
#endif


#ifdef QTK_NNRT_RKNPU
    if(rt->cfg->use_rknpu){
        rknn_inputs_set(rt->ctx, rt->num_in, rt->input);
        int ret = rknn_run(rt->ctx, NULL);
        if(ret < 0){
            wtk_debug("rknn_run failed !!!  ret = %d\n", ret);
        }
        rknn_outputs_get(rt->ctx, rt->num_out, rt->outputs, NULL);
    }

#endif

#ifdef QTK_NNRT_SS_IPU
    if (rt->cfg->use_ss_ipu) {
        int i;
        for (i = 0; i < rt->num_in; i++) {
            MI_SYS_FlushInvCache(
                rt->InputTensorVector.astArrayTensors[i].ptTensorData[0],
                rt->desc.astMI_InputTensorDescs[i].s32AlignedBufSize);
        }
        return MI_SUCCESS != MI_IPU_Invoke(rt->u32ChannelID, &rt->InputTensorVector,
                &rt->OutputTensorVector) ? -1 : 0;
    }
#endif

#ifdef QTK_NNRT_LIBTORCH
    if (rt->cfg->use_libtorch) {
        return qtk_nnrt_libtorch_run(rt->libtorch);
    }
#endif
    return -1;
}

#ifdef QTK_NNRT_RKNPU
int qtk_nnrt_feed_image(qtk_nnrt_t *rt, qtk_nnrt_value_t value, int idx) {
    if (rt->cfg->use_rknpu) {
        rt->input[idx].buf = value;
        rt->input[idx].type = RKNN_TENSOR_UINT8;
        return 0;
    }
    return -1;
}
#endif

int qtk_nnrt_feed(qtk_nnrt_t *rt, qtk_nnrt_value_t value, int idx) {
#ifdef QTK_NNRT_NCNN
    if (rt->cfg->use_ncnn) {
        int ncnn_idx = ncnn_net_get_input_index(rt->net, idx);
        return ncnn_extractor_input_index(rt->extractor, ncnn_idx,
                                          (ncnn_mat_t)value);
    }
#endif

#ifdef QTK_NNRT_ONNXRUNTIME
    if (rt->cfg->use_onnxruntime) {
        rt->ort_in[idx] = value;
        return 0;
    }
#endif

#ifdef QTK_NNRT_RKNPU
    if(rt->cfg->use_rknpu){
        rt->input[idx].buf = value;
        return 0;
    }
#endif

#ifdef QTK_NNRT_SS_IPU
    if (rt->cfg->use_ss_ipu) {
        if (value ==
            rt->InputTensorVector.astArrayTensors[idx].ptTensorData[0]) {
            return 0;
        }
        wtk_debug("Not Impl\n");
        return -1;
    }
#endif

#ifdef QTK_NNRT_LIBTORCH
    if (rt->cfg->use_libtorch) {
        return qtk_nnrt_libtorch_feed(rt->libtorch, value, idx);
    }
#endif
    return -1;
}

int qtk_nnrt_feed_s(qtk_nnrt_t *rt, qtk_nnrt_value_t value, const char *name) {
#ifdef QTK_NNRT_NCNN
    if (rt->cfg->use_ncnn) {
        return ncnn_extractor_input(rt->extractor, name, value);
    }
#endif
#ifdef QTK_NNRT_ONNXRUNTIME
    if (rt->cfg->use_onnxruntime) {
        int i;
        for (i = 0; i < rt->num_in; i++) {
            if (!strcmp(rt->ort_input_name[i], name)) {
                rt->ort_in[i] = value;
                return 0;
            }
        }
    }
#endif
    return -1;
}

int qtk_nnrt_get_output(qtk_nnrt_t *rt, qtk_nnrt_value_t *out, int idx) {
#ifdef QTK_NNRT_NCNN
    if (rt->cfg->use_ncnn) {
        int ncnn_idx = ncnn_net_get_output_index(rt->net, idx);
        return ncnn_extractor_extract_index(rt->extractor, ncnn_idx,
                                            (ncnn_mat_t *)out);
    }
#endif

#ifdef QTK_NNRT_ONNXRUNTIME
    if (rt->cfg->use_onnxruntime) {
        if (idx >= rt->num_out) {
            goto err;
        }
        *out = rt->ort_out[idx];
        rt->ort_out[idx] = NULL;
        return 0;
    }
err:
#endif


#ifdef QTK_NNRT_RKNPU
    if (rt->cfg->use_rknpu) {
        *out = rt->outputs[idx].buf;
    }
#endif

#ifdef QTK_NNRT_SS_IPU
    if (rt->cfg->use_ss_ipu) {
        *out = rt->OutputTensorVector.astArrayTensors[idx].ptTensorData[0];
        return 0;
    }
#endif

#ifdef QTK_NNRT_LIBTORCH
    if (rt->cfg->use_libtorch) {
        return qtk_nnrt_libtorch_get_output(rt->libtorch, out, idx);
    }
#endif
    return -1;
}

int qtk_nnrt_get_output_s(qtk_nnrt_t *rt, qtk_nnrt_value_t *out,
                          const char *name) {
#ifdef QTK_NNRT_NCNN
    if (rt->cfg->use_ncnn) {
        return ncnn_extractor_extract(rt->extractor, name, (ncnn_mat_t *)out);
    }
#endif
#ifdef QTK_NNRT_ONNXRUNTIME
    if (rt->cfg->use_onnxruntime) {
        int i;
        for (i = 0; i < rt->num_out; i++) {
            if (strcmp(name, rt->ort_output_name[i]) == 0) {
                *out = rt->ort_out[i];
                rt->ort_out[i] = NULL;
                return 0;
            }
        }
    }
#endif
    return -1;
}

int qtk_nnrt_get_num_in(qtk_nnrt_t *rt) {
#ifdef QTK_NNRT_NCNN
    if (rt->cfg->use_ncnn) {
        return ncnn_net_get_input_count(rt->net);
    }
#endif

#ifdef QTK_NNRT_ONNXRUNTIME
    if (rt->cfg->use_onnxruntime) {
        size_t num_in;
        QTK_ONNXRUNTIME_CALL(rt, SessionGetInputCount, return -1;
                             , rt->ort_session, &num_in);
        return num_in;
    }
#endif

#ifdef QTK_NNRT_SS_IPU
    if (rt->cfg->use_ss_ipu) {
        return rt->num_in;
    }
#endif
    return -1;
}

int qtk_nnrt_get_num_out(qtk_nnrt_t *rt) {
#ifdef QTK_NNRT_NCNN
    if (rt->cfg->use_ncnn) {
        return ncnn_net_get_output_count(rt->net);
    }
#endif

#ifdef QTK_NNRT_ONNXRUNTIME
    if (rt->cfg->use_onnxruntime) {
        size_t num_out;
        QTK_ONNXRUNTIME_CALL(rt, SessionGetOutputCount, return -1;
                             , rt->ort_session, &num_out);
        return num_out;
    }
#endif

#ifdef QTK_NNRT_SS_IPU
    if (rt->cfg->use_ss_ipu) {
        return rt->num_out;
    }
#endif
    return -1;
}

#ifdef QTK_NNRT_NCNN
static size_t alignSize(size_t sz, int n) {
    return (sz + n - 1) & -n;
}
#endif

qtk_nnrt_value_t qtk_nnrt_value_create(qtk_nnrt_t *rt,
                                       qtk_nnrt_value_elem_type_t t,
                                       int64_t *shape, int shape_len) {
#ifdef QTK_NNRT_ONNXRUNTIME
    if (rt->cfg->use_onnxruntime) {
        OrtValue *val;
        switch (t) {
        case QTK_NNRT_VALUE_ELEM_F32:
            QTK_ONNXRUNTIME_CALL(rt, CreateTensorAsOrtValue, return NULL;
                                 , rt->ort_allocator, shape, shape_len,
                                 ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, &val);
            return val;
        case QTK_NNRT_VALUE_ELEM_F16:
            QTK_ONNXRUNTIME_CALL(rt, CreateTensorAsOrtValue, return NULL;
                                 , rt->ort_allocator, shape, shape_len,
                                 ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16, &val);
            return val;
        case QTK_NNRT_VALUE_ELEM_I64:
            QTK_ONNXRUNTIME_CALL(
                rt, CreateTensorAsOrtValue, return NULL;
                , rt->ort_allocator, shape,shape_len,
                ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64, &val);
            return val;
        default:
            return NULL;
        }
    }
#endif

#ifdef QTK_NNRT_RKNPU
    if(rt->cfg->use_rknpu){
        return NULL; // use qtk_nnrt_value_create_external instead
    }
#endif

    return NULL;
}

qtk_nnrt_value_t qtk_nnrt_value_create_external(qtk_nnrt_t *rt,
                                                qtk_nnrt_value_elem_type_t t,
                                                int64_t *shape, int shape_len,
                                                void *data) {
#ifdef QTK_NNRT_NCNN
    if (rt->cfg->use_ncnn) {
        size_t cstep, step;
        int elemsize = nnrt_value_elem_sz_[t];
        ncnn_mat_t val;
        int i;
        if (shape_len <= 1 || shape_len > 5 || shape[0] != 1) {
            return NULL;
        }
        shape_len--; /* skip batch dim */
        shape++;
        switch (shape_len) {
        case 1:
            val = ncnn_mat_create_external_1d_elem(shape[0], data, elemsize, 1,
                                                   rt->allocator);
            break;
        case 2:
            val = ncnn_mat_create_external_2d_elem(shape[1], shape[0], data, elemsize, 1, rt->allocator);
            break;
        case 3:
            step = shape[2] * shape[1];
            cstep = alignSize(step * elemsize, 16) / elemsize;
            if (cstep != step) {
                val = ncnn_mat_create_3d_elem(
                        shape[2], shape[1], shape[0],  elemsize, 1, rt->allocator);;
                for (i = 0; i < shape[0]; i ++) {
                    memcpy(ncnn_mat_get_channel_data(val, i), (char *)data + step * i * elemsize, step * elemsize);
                }
            } else {
                val = ncnn_mat_create_external_3d_elem(shape[2], shape[1], shape[0], data, elemsize, 1, rt->allocator);
            }
            break;
        case 4:
            step = shape[3] * shape[2] * shape[1];
            cstep = alignSize(step * elemsize, 16) / elemsize;
            if (cstep != step) {
                val = ncnn_mat_create_4d_elem(shape[3], shape[2], shape[1], shape[0], elemsize, 1, rt->allocator);
                for (i = 0; i < shape[0]; i ++) {
                    memcpy(ncnn_mat_get_channel_data(val, i), (char *)data + step * i * elemsize, step * elemsize);
                }
            } else {
                val = ncnn_mat_create_external_4d_elem(shape[3], shape[2], shape[1],
                        shape[0], data, elemsize, 1,
                        rt->allocator);
            }
            break;
        default:
            return NULL;
        }
        return val;
    }
#endif

#ifdef QTK_NNRT_ONNXRUNTIME
    if (rt->cfg->use_onnxruntime) {
        OrtValue *val;
        int i;
        int64_t nelem = 1;
        for (i = 0; i < shape_len; i++) {
            nelem *= shape[i];
        }
        switch (t) {
        case QTK_NNRT_VALUE_ELEM_F32:
            QTK_ONNXRUNTIME_CALL(
                rt, CreateTensorWithDataAsOrtValue, return NULL;
                , rt->ort_meminfo, data, nelem * sizeof(float), shape,
                shape_len, ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, &val);
            return val;
        case QTK_NNRT_VALUE_ELEM_F16:
            QTK_ONNXRUNTIME_CALL(
                rt, CreateTensorWithDataAsOrtValue, return NULL;
                , rt->ort_meminfo, data, nelem * 2, shape, shape_len,
                ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16, &val);
            return val;
        case QTK_NNRT_VALUE_ELEM_I64:
            QTK_ONNXRUNTIME_CALL(
                rt, CreateTensorWithDataAsOrtValue, return NULL;
                , rt->ort_meminfo, data, nelem * sizeof(int64_t), shape,
                shape_len, ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64, &val);
            return val;
        default:
            return NULL;
        }
    }
#endif

#ifdef QTK_NNRT_RKNPU
    if(rt->cfg->use_rknpu){
        return data;
    }
#endif

#ifdef QTK_NNRT_SS_IPU
    if (rt->cfg->use_ss_ipu) {
        return data;
    }
#endif

#ifdef QTK_NNRT_LIBTORCH
    if (rt->cfg->use_libtorch) {
        return qtk_nnrt_libtorch_value_create_external(rt->libtorch, t, shape,
                                                       shape_len, data);
    }
#endif
    return NULL;
}

qtk_nnrt_value_t qtk_nnrt_value_create_external2(qtk_nnrt_t *rt,
                                                 qtk_nnrt_value_elem_type_t t,
                                                 int64_t *shape, int shape_len,
                                                 void *data, int64_t len) {
#ifdef QTK_NNRT_ONNXRUNTIME
    OrtValue *val;
    QTK_ONNXRUNTIME_CALL(rt, CreateTensorWithDataAsOrtValue, return NULL;
                         , rt->ort_meminfo, data, len, shape, shape_len,
                         ONNX_TENSOR_ELEMENT_DATA_TYPE_BOOL, &val);
    return val;
#endif
    return NULL;
}

void qtk_nnrt_value_release(qtk_nnrt_t *rt, qtk_nnrt_value_t value) {
#ifdef QTK_NNRT_NCNN
    if (rt->cfg->use_ncnn) {
        ncnn_mat_destroy((ncnn_mat_t)value);
        return;
    }
#endif

#ifdef QTK_NNRT_RKNPU
    if (rt->cfg->use_rknpu) {
        return;
    }
#endif

#ifdef QTK_NNRT_ONNXRUNTIME
    if (rt->cfg->use_onnxruntime) {
        rt->ort_api->ReleaseValue(value);
        return;
    }
#endif

#ifdef QTK_NNRT_LIBTORCH
    if (rt->cfg->use_libtorch) {
        qtk_nnrt_libtorch_value_release(rt->libtorch, value);
        return;
    }
#endif
}

int qtk_nnrt_value_get_shape(qtk_nnrt_t *rt, qtk_nnrt_value_t value,
                             int64_t *shape, int shape_cap) {
#ifdef QTK_NNRT_NCNN
    if (rt->cfg->use_ncnn) {
        ncnn_mat_t mat = (ncnn_mat_t)value;
        int dims = ncnn_mat_get_dims(mat);
        if (dims + 1 > shape_cap) {
            return -1;
        }
        *shape++ = 1;
        switch (dims) {
        case 1:
            shape[0] = ncnn_mat_get_w(mat);
            break;
        case 2:
            shape[1] = ncnn_mat_get_w(mat);
            shape[0] = ncnn_mat_get_h(mat);
            break;
        case 3:
            shape[2] = ncnn_mat_get_w(mat);
            shape[1] = ncnn_mat_get_h(mat);
            shape[0] = ncnn_mat_get_c(mat);
            break;
        case 4:
            shape[3] = ncnn_mat_get_w(mat);
            shape[2] = ncnn_mat_get_h(mat);
            shape[1] = ncnn_mat_get_d(mat);
            shape[0] = ncnn_mat_get_c(mat);
            break;
        default:
            return -1;
        }
        return dims + 1;
    }
#endif

#ifdef QTK_NNRT_ONNXRUNTIME
    if (rt->cfg->use_onnxruntime) {
        OrtValue *ov = (qtk_nnrt_value_t)value;
        OrtTensorTypeAndShapeInfo *info = NULL;
        size_t shape_len;
        QTK_ONNXRUNTIME_CALL(rt, GetTensorTypeAndShape, goto err;, ov, &info);
        QTK_ONNXRUNTIME_CALL(rt, GetDimensionsCount, goto err;
                             , info, &shape_len);
        if (shape_len > shape_cap) {
            goto err;
        }
        QTK_ONNXRUNTIME_CALL(rt, GetDimensions, goto err;
                             , info, shape, shape_len);
        rt->ort_api->ReleaseTensorTypeAndShapeInfo(info);
        return shape_len;
    err:
        if (info) {
            rt->ort_api->ReleaseTensorTypeAndShapeInfo(info);
        }
        return -1;
    }
#endif

#ifdef QTK_NNRT_LIBTORCH
    if (rt->cfg->use_libtorch) {
        return qtk_nnrt_libtorch_value_get_shape(rt->libtorch, value, shape,
                                                 shape_cap);
    }
#endif
    return -1;
}

void *qtk_nnrt_value_get_data(qtk_nnrt_t *rt, qtk_nnrt_value_t value) {
#ifdef QTK_NNRT_NCNN
    if (rt->cfg->use_ncnn) {
        ncnn_mat_t mat = (ncnn_mat_t)value;
        return ncnn_mat_get_data(mat);
    }
#endif

#ifdef QTK_NNRT_ONNXRUNTIME
    if (rt->cfg->use_onnxruntime) {
        void *data;
        QTK_ONNXRUNTIME_CALL(rt, GetTensorMutableData, return NULL;
                             , value, &data);
        return data;
    }
#endif

#ifdef  QTK_NNRT_RKNPU
    if(rt->cfg->use_rknpu){
        return value;
    }
#endif

#ifdef QTK_NNRT_SS_IPU
    if (rt->cfg->use_ss_ipu) {
        return value;
    }
#endif

#ifdef QTK_NNRT_LIBTORCH
    if (rt->cfg->use_libtorch) {
        return qtk_nnrt_libtorch_value_get_data(rt->libtorch, value);
    }
#endif

    return NULL;
}

int qtk_nnrt_metadata_lookup(qtk_nnrt_t *rt, char *key, char *data, int len) {
#ifdef QTK_NNRT_NCNN
    if (rt->cfg->use_ncnn) {
        wtk_string_t *v;
        if (rt->cfg->metadata == NULL ||
            !(v = wtk_local_cfg_find_string(rt->cfg->metadata, key,
                                            strlen(key))) ||
            v->len > len - 1) {
            return -1;
        }
        memcpy(data, v->data, v->len);
        data[v->len] = '\0';
        return 0;
    }
#endif

#ifdef QTK_NNRT_ONNXRUNTIME
    if (rt->cfg->use_onnxruntime) {
        int ret = -1;
        OrtModelMetadata *metadata = NULL;
        char *val = NULL;
        QTK_ONNXRUNTIME_CALL(rt, SessionGetModelMetadata, goto ort_end;
                             , rt->ort_session, &metadata);
        QTK_ONNXRUNTIME_CALL(rt, ModelMetadataLookupCustomMetadataMap,
                             goto ort_end;
                             , metadata, rt->ort_allocator, key, &val);
        if (!val) {
            goto ort_end;
        }
        snprintf(data, len, "%s", val);
        ret = 0;
    ort_end:
        if (metadata)
            rt->ort_api->ReleaseModelMetadata(metadata);
        if (val)
            rt->ort_allocator->Free(rt->ort_allocator, val);
        return ret;
    }
#endif
    return -1;
}

int qtk_nnrt_get_input_shape(qtk_nnrt_t *rt, int idx, int64_t *shape,
                             int shape_cap) {
#ifdef QTK_NNRT_NCNN
    if (rt->cfg->use_ncnn) {
        return -1;
    }
#endif

#ifdef QTK_NNRT_ONNXRUNTIME
    if (rt->cfg->use_onnxruntime) {
        int ret = -1;
        OrtTypeInfo *info = NULL;
        size_t shape_len;
        const OrtTensorTypeAndShapeInfo *tensor_info;
        QTK_ONNXRUNTIME_CALL(rt, SessionGetInputTypeInfo, goto ort_end;
                             , rt->ort_session, idx, &info);
        QTK_ONNXRUNTIME_CALL(rt, CastTypeInfoToTensorInfo, goto ort_end;
                             , info, &tensor_info);
        QTK_ONNXRUNTIME_CALL(rt, GetDimensionsCount, goto ort_end;
                             , tensor_info, &shape_len);
        if (shape_len > shape_cap) {
            goto ort_end;
        }
        QTK_ONNXRUNTIME_CALL(rt, GetDimensions, goto ort_end;
                             , tensor_info, shape, shape_len);
        ret = shape_len;
    ort_end:
        if (info) {
            rt->ort_api->ReleaseTypeInfo(info);
        }
        return ret;
    }
#endif


#ifdef QTK_NNRT_RKNPU
    if(rt->cfg->use_rknpu){
        int i;
        if (rt->input_attrs[idx].n_dims > shape_cap) {
            return -1;
        }
        for (i = 0; i < rt->input_attrs[idx].n_dims; i++) {
            shape[i] = rt->input_attrs[idx].dims[i];
        }
        return rt->input_attrs[idx].n_dims;
    }
#endif

#ifdef QTK_NNRT_SS_IPU
    if (rt->cfg->use_ss_ipu) {
        int i;
        int shape_dim = rt->desc.astMI_InputTensorDescs[idx].u32TensorDim;
        if (shape_dim > shape_cap) {
            return -1;
        }
        for (i = 0; i < shape_dim; i++) {
            shape[i] = rt->desc.astMI_InputTensorDescs[idx].u32TensorShape[i];
        }
        return shape_dim;
    }
#endif
    return -1;
}

int qtk_nnrt_reset(qtk_nnrt_t *rt) {
#ifdef QTK_NNRT_NCNN
    if(rt->cfg->use_ncnn){
        ncnn_extractor_destroy(rt->extractor);
        rt->extractor = ncnn_extractor_create(rt->net);
        // ncnn_extractor_set_option(rt->extractor, rt->ncnn_option);
        return 0;
    }
#endif
#ifdef QTK_NNRT_LIBTORCH
    if (rt->cfg->use_libtorch) {
        return qtk_nnrt_libtorch_reset(rt->libtorch);
    }
#endif
#ifdef QTK_NNRT_RKNPU
    if (rt->cfg->use_rknpu) {
        rknn_outputs_release(rt->ctx, rt->num_out, rt->outputs);
        return 0;
    }
#endif
#ifdef QTK_NNRT_ONNXRUNTIME
    if (rt->cfg->use_onnxruntime) {
        int i;
        for (i = 0; i < rt->num_out; i++) {
            if (rt->ort_out[i] == NULL) {
                continue;
            }
            rt->ort_api->ReleaseValue(rt->ort_out[i]);
            rt->ort_out[i] = NULL;
        }
        return 0;
    }
#endif
    return -1;
}

char *qtk_nnrt_net_get_input_name(qtk_nnrt_t *rt, int idx)
{
#ifdef QTK_NNRT_NCNN
    if(rt->cfg->use_ncnn){
        char *name;
        name = ncnn_net_get_input_name(rt->net, idx);
        return name;
    }
#endif
#ifdef QTK_NNRT_ONNXRUNTIME
    wtk_debug("qtk_nnrt_net_get_input_name not implemented for onnxruntime\n");
#endif
    return NULL;
}


char *qtk_nnrt_net_get_output_name(qtk_nnrt_t *rt, int idx)
{
#ifdef QTK_NNRT_NCNN
    if(rt->cfg->use_ncnn){
        char *name;
        name = ncnn_net_get_output_name(rt->net, idx);
        return name;
    }
#endif
#ifdef QTK_NNRT_ONNXRUNTIME
    wtk_debug("qtk_nnrt_net_get_output_name not implemented for onnxruntime\n");
#endif
    return NULL;
}

void *qtk_nnrt_value_get_channel_data(qtk_nnrt_t *rt, qtk_nnrt_value_t value, int idx) {
#ifdef QTK_NNRT_NCNN
    if (rt->cfg->use_ncnn) {
        ncnn_mat_t mat = (ncnn_mat_t)value;
        return ncnn_mat_get_channel_data(mat, idx);
    }
#endif
    return NULL;
}

int qtk_nnrt_disable_output_dequant(qtk_nnrt_t *rt, int idx) {
#ifdef QTK_NNRT_RKNPU
    if (rt->cfg->use_rknpu) {
        rt->outputs[idx].want_float = 0;
        return 0;
    }
#endif
    return -1;
}

qtk_nnrt_value_t qtk_nnrt_create_input(qtk_nnrt_t *rt, int idx) {
#ifdef QTK_NNRT_ONNXRUNTIME
    if (rt->cfg->use_onnxruntime) {
        int64_t shape[16];
        int rank = qtk_nnrt_get_input_shape(rt, idx, shape,
                                            sizeof(shape) / sizeof(shape[0]));
        return qtk_nnrt_value_create(rt, QTK_NNRT_VALUE_ELEM_F32, shape, rank);
    }
#endif
#ifdef QTK_NNRT_SS_IPU
    if (rt->cfg->use_ss_ipu) {
        return rt->InputTensorVector.astArrayTensors[idx].ptTensorData[0];
    }
#endif
    return NULL;
}

int qtk_nnrt_get_output_shape(qtk_nnrt_t *rt, int idx, int64_t *shape,
                              int shape_cap) {
#ifdef QTK_NNRT_ONNXRUNTIME
    if (rt->cfg->use_onnxruntime) {
        return qtk_nnrt_value_get_shape(rt, rt->ort_out[idx], shape, shape_cap);
    }
#endif
    return -1;
}

size_t qtk_nnrt_get_output_nbytes(qtk_nnrt_t *rt, int idx) {
    int nelems = qtk_nnrt_get_output_nelems(rt, idx);
    return nelems * nnrt_value_elem_sz_[qtk_nnrt_get_output_elem_type(rt, idx)];
}

size_t qtk_nnrt_get_output_nelems(qtk_nnrt_t *rt, int idx) {
    int i;
    int nelem = 1;
    int64_t shape[16];
    int rank = qtk_nnrt_get_output_shape(rt, idx, shape,
                                         sizeof(shape) / sizeof(shape[0]));
    for (i = 0; i < rank; i++) {
        nelem *= shape[i];
    }
    return nelem;
}

size_t qtk_nnrt_get_input_nbytes(qtk_nnrt_t *rt, int idx) {
#ifdef QTK_NNRT_SS_IPU
    if (rt->cfg->use_ss_ipu) {
        return rt->desc.astMI_InputTensorDescs[idx].s32AlignedBufSize;
    }
#endif
    int nelems = qtk_nnrt_get_input_nelems(rt, idx);
    return nelems * nnrt_value_elem_sz_[qtk_nnrt_get_input_elem_type(rt, idx)];
}

size_t qtk_nnrt_get_input_nelems(qtk_nnrt_t *rt, int idx) {
    int i;
    int nelem = 1;
    int64_t shape[16];
    int rank = qtk_nnrt_get_input_shape(rt, idx, shape,
                                        sizeof(shape) / sizeof(shape[0]));
    for (i = 0; i < rank; i++) {
        nelem *= shape[i];
    }
    return nelem;
}

qtk_nnrt_value_elem_type_t qtk_nnrt_get_input_elem_type(qtk_nnrt_t *rt,
                                                        int idx) {
#ifdef QTK_NNRT_ONNXRUNTIME
    if (rt->cfg->use_onnxruntime) {
        return QTK_NNRT_VALUE_ELEM_F32;
    }
#endif
#ifdef QTK_NNRT_SS_IPU
    if (rt->cfg->use_ss_ipu) {
        return convert_ss_ipu_elem_type_to_nnrt_(
            rt->desc.astMI_InputTensorDescs[idx].eElmFormat);
    }
#endif
    return QTK_NNRT_VALUE_ELEM_F32;
}

qtk_nnrt_value_elem_type_t qtk_nnrt_get_output_elem_type(qtk_nnrt_t *rt,
                                                         int idx) {
#ifdef QTK_NNRT_ONNXRUNTIME
    if (rt->cfg->use_onnxruntime) {
        return QTK_NNRT_VALUE_ELEM_F32;
    }
#endif
#ifdef QTK_NNRT_SS_IPU
    if (rt->cfg->use_ss_ipu) {
        return convert_ss_ipu_elem_type_to_nnrt_(
            rt->desc.astMI_OutputTensorDescs[idx].eElmFormat);
    }
#endif
    return QTK_NNRT_VALUE_ELEM_F32;
}
