#include "wtk/asr/ced/wtk_ced.h"

wtk_ced_t *wtk_ced_new(wtk_ced_cfg_t *cfg) {
    wtk_ced_t *ced = NULL;
    ced = (wtk_ced_t *)wtk_malloc(sizeof(wtk_ced_t));
    ced->cfg = cfg;
    /*
     * the value of prediction value
     */
    ced->crying_start = -1;
    ced->crying_end = -1;
    ced->crying_len = 0;
    ced->crying_flag = 0;
    ced->shout_start = -1;
    ced->shout_end = -1;
    ced->shout_len = 0;
    ced->shout_flag = 0;
    /*
     * prediction_index is input len divided by hop_size
     */
    ced->prediction_index = 0;
    /*
     * wav is uced to save pcm sample data when pcm data is transform from short
     * to float format
     */
    ced->wav =
        wtk_strbuf_new(cfg->hop_size * cfg->sample_rate * sizeof(float), 1);
#ifdef ONNX_DEC
    ced->onnx = qtk_onnxruntime_new(&(cfg->onnx));
#endif
    return ced;
}

void wtk_ced_delete(wtk_ced_t *ced) {
    if (ced->wav) {
        wtk_strbuf_delete(ced->wav);
    }
#ifdef ONNX_DEC
    if (ced->onnx) {
        qtk_onnxruntime_delete(ced->onnx);
    }

#endif
    wtk_free(ced);
}

void wtk_ced_reset(wtk_ced_t *ced) {
    wtk_strbuf_reset(ced->wav);
#ifdef ONNX_DEC
    qtk_onnxruntime_reset(ced->onnx);
#endif
}

void wtk_ced_set_notify(wtk_ced_t *ced, wtk_ced_notify_f notify, void *ths) {
    ced->notify = notify;
    ced->notify_ths = ths;
}

/*
 * func:
 *     the default sample rate of input wav is 16k and its data is saved in
 *     ced->wav after every sampel point is converted from short to float.
 *
 * input para:
 *     ced: moudle point
 *     data: the point of input data
 *     len: the char len of input data
 */
void wtk_ced_save_wav(wtk_ced_t *ced, char *data, int len) {
    int
        // traverse sample point to convert
        i = 0,
        // sample_num is the number of wav sample point
        sample_num = 0;
    short
        // sample_data is the wav data of short-type point
        *sample_data = NULL;
    float
        // sample_point is uced to temporarily save the floating-point value of
        // each sample point of wav audio.
        sample_point = 0.0;

    sample_num = len / ced->cfg->byte_per_sample;
    sample_data = (short *)data;
    for (i = 0; i < sample_num; i++) {
        sample_point = ((float)sample_data[i]) / 32768;
        wtk_strbuf_push(ced->wav, (char *)(&sample_point), sizeof(float));
    }
}

/*
 * func:
 *     wtk_ced_onnx_compute is uced to compute onnx output
 *
 * input para:
 *     ced: moudle point
 *     onnx_input_data: the point of onnx model input prob
 *     onnx_input_len: the number of onnx model input
 *
 * output para:
 *     the output point of onnx model
 */
float *wtk_ced_onnx_compute(wtk_ced_t *ced, float *onnx_input_data,
                            int onnx_input_len) {
    float
        // the point of onnx model output
        *output_point = NULL;
#ifdef ONNX_DEC
    int64_t
        // the dim of in and out shape
        in_shape_dim = 0,
        // the index of onnx model output that is frame and clip
        onnx_output_index = 0,
        // the shape of input
            *in_shape = NULL;

    /*
     * it is need to get onnx output index
     */
    onnx_output_index = ced->cfg->onnx_output_index;
    /*
     * get input shape
     */
    in_shape = qtk_onnxruntime_get_inshape(ced->onnx, 0, &in_shape_dim);
    /*
     * default value is replaced to input len
     */
    in_shape[1] = onnx_input_len;

    /*
     * get onnx input
     */
    qtk_onnxruntime_feed(ced->onnx, onnx_input_data, onnx_input_len,
                         cast(int64_t *, in_shape), in_shape_dim, 0, 0);
    qtk_onnxruntime_run(ced->onnx);
    /*
     * get output point
     */
    output_point = qtk_onnxruntime_getout(ced->onnx, onnx_output_index);
    /*
     * in_shape is freed every time
     */
    wtk_free(in_shape);
#endif
    return output_point;
}

/*
 * func:
 *     wtk_ced_prediction is used to call notify
 *
 * input para:
 *     ced: moudle point
 *     prob: the prob of single chunk_size input
 *     event_type: the type of sound event
 *     is_end: is_end == 1 means call feed_end func
 */
void wtk_ced_prediction(wtk_ced_t *ced, float prob,
                        wtk_ced_event_type_e event_type, int is_end) {
#ifdef CED_MODEL_OUTPUT
    if (!is_end) {
        ced->notify(ced, event_type, ced->prediction_index * ced->cfg->hop_size,
                    (ced->prediction_index + 1) * ced->cfg->hop_size);
        ced->prediction_index++;
    } else {
        ced->prediction_index = 0;
    }
#else
    /*
     * assign value for crying or shout varialbe according to current event type
     */
    switch (event_type) {
    case WTK_CED_CRY:
        if (-1 == ced->crying_start) {
            ced->crying_start = ced->prediction_index;
        }
        ced->crying_end = ced->prediction_index;
        ced->crying_len++;
        break;
    case WTK_CED_SHOUT:
        if (-1 == ced->shout_start) {
            ced->shout_start = ced->prediction_index;
        }
        ced->shout_end = ced->prediction_index;
        ced->shout_len++;
        break;
    case WTK_CED_OTHER:
        break;
    default:
        break;
    }

    /*
     * determine whether the start notify is called and there only is single
     * event at a time
     */
    if (ced->crying_start != -1 && ced->shout_start == -1) {
        if ((ced->prediction_index ==
             ced->crying_start + ced->crying_len - 1) &&
            ced->crying_len == ced->cfg->start_nframe_threshold) {
            ced->notify(ced, WTK_CED_CRY,
                        ced->crying_start * ced->cfg->hop_size, -1);
            ced->crying_flag = 1;
        }
    }

    if (ced->shout_start != -1 && ced->crying_start == -1) {
        if ((ced->prediction_index == ced->shout_start + ced->shout_len - 1) &&
            ced->shout_len == ced->cfg->start_nframe_threshold) {
            ced->notify(ced, WTK_CED_SHOUT,
                        ced->shout_start * ced->cfg->hop_size, -1);
            ced->shout_flag = 1;
        }
    }

    /*
     * when there are two kinds of events with "start", clean up "start" at last
     */
    if (ced->crying_flag && ced->shout_flag) {
        if (ced->crying_start < ced->shout_start) {
            ced->shout_flag = 0;
        } else {
            ced->crying_flag = 0;
        }
    }

    /*
     * determine whether the end notify can be call
     */
    if (ced->crying_flag && ced->crying_start >= 0 &&
        WTK_CED_CRY != event_type) {
        if ((ced->prediction_index - ced->crying_end) >=
            ced->cfg->end_nframe_threshold) {
            ced->notify(ced, WTK_CED_CRY,
                        ced->crying_start * ced->cfg->hop_size,
                        (ced->crying_end + 1) * ced->cfg->hop_size);
            ced->crying_start = -1;
            ced->crying_end = -1;
            ced->crying_len = 0;
            ced->crying_flag = 0;

            ced->shout_start = -1;
            ced->shout_end = -1;
            ced->shout_len = 0;
            ced->shout_flag = 0;
        }
    }

    if (ced->shout_flag && ced->shout_start >= 0 &&
        WTK_CED_SHOUT != event_type) {
        if ((ced->prediction_index - ced->shout_end) >=
            ced->cfg->end_nframe_threshold) {
            ced->notify(ced, WTK_CED_SHOUT,
                        ced->shout_start * ced->cfg->hop_size,
                        (ced->shout_end + 1) * ced->cfg->hop_size);
            ced->crying_start = -1;
            ced->crying_end = -1;
            ced->crying_len = 0;
            ced->crying_flag = 0;

            ced->shout_start = -1;
            ced->shout_end = -1;
            ced->shout_len = 0;
            ced->shout_flag = 0;
        }
    }

    ced->prediction_index++;

    if (is_end) {
        /*
         * when is_end, deal with all existing start
         */
        if (ced->crying_flag && ced->crying_start != -1) {
            ced->notify(ced, WTK_CED_CRY,
                        ced->crying_start * ced->cfg->hop_size,
                        ced->prediction_index * ced->cfg->hop_size);
        }

        if (ced->shout_flag && ced->shout_start != -1) {
            ced->notify(ced, WTK_CED_SHOUT,
                        ced->shout_start * ced->cfg->hop_size,
                        ced->prediction_index * ced->cfg->hop_size);
        }

        ced->crying_start = -1;
        ced->crying_end = -1;
        ced->crying_len = 0;
        ced->crying_flag = 0;
        ced->shout_start = -1;
        ced->shout_end = -1;
        ced->shout_len = 0;
        ced->shout_flag = 0;
        ced->prediction_index = 0;
    }
#endif
}

/*
 * func:
 *     wtk_ced_compute_energy is used to compute the energy of data of len
 *
 * input para:
 *     data: the point of data whose type is float
 *     len: the len of data
 */
float wtk_ced_compute_energy(float *data, int len) {
    int i = 0;
    float val = 0.0, sum = 0.0;

    for (i = 0; i < len; i++) {
        val = (data[i] > 0) ? data[i] : -(data[i]);
        sum += val;
    }

    return sum;
}

void wtk_ced_feed(wtk_ced_t *ced, char *data, int len, int is_end) {
    int
        // onnx model input length is fixed
        onnx_input_len = 0,
        // the wav_step length of data need to be pop after data is send to
        // onnx model compute
        pop_bytes = 0,
        // the number of sound event class
        predict_input_classes = ced->cfg->classes_num;
    float
        // current energy value
        energy = 0.0,
        // the point of onnx in and out
        *onnx_input_point = NULL, *onnx_out_point = NULL,
        // the input prob point of prediction process
                                      *predict_input_prob = NULL;
    wtk_ced_event_type_e
        // the event type of max prob
        event_type = WTK_CED_OTHER;

    /*
     * convert audio data from short to float and save data to ced->wav
     */
    wtk_ced_save_wav(ced, data, len);
    onnx_input_len =
        ced->cfg->chunk_size * ced->cfg->sample_rate * sizeof(float);
    /*
     * data which is the len of chunk_size is sended to onnx model to compute
     * and data which is the len of chunk_size is pop when the len of pos is
     * longer than onnx_input_len
     */
    while (ced->wav->pos >= onnx_input_len) {
        onnx_input_point = (float *)ced->wav->data;
        /*
         * compute the energy of data of length "chunk_size"
         */
        if (ced->cfg->use_energy) {
            energy = wtk_ced_compute_energy(onnx_input_point,
                                            onnx_input_len / sizeof(float));
        }
        /*
         * if energy is lower than energy_threshold, current event is considered
         * as other event type
         */
        if (energy > ced->cfg->energy_threshold || !ced->cfg->use_energy) {
            /*
             * the number of onnx_input_len / sizeof(float) sample point data is
             * sended to onnx to compute.
             */
            onnx_out_point = wtk_ced_onnx_compute(
                ced, onnx_input_point, onnx_input_len / sizeof(float));
            /*
             * the information of prediction process is need to save in window
             * node
             */
            predict_input_prob = onnx_out_point;
            event_type =
                wtk_float_argmax(predict_input_prob, predict_input_classes);
            /*
             * call notify
             */
            wtk_ced_prediction(ced, *(predict_input_prob + event_type),
                               event_type, 0);
        } else {
            wtk_ced_prediction(ced, 0.0, 2, 0);
        }

        /*
         * the wav_step len of data is needed to pop
         */
        if (ced->cfg->hop_size > 0) {
            pop_bytes =
                ced->cfg->hop_size * ced->cfg->sample_rate * sizeof(float);
            wtk_strbuf_pop(ced->wav, NULL, pop_bytes);
        }
#ifdef ONNX_DEC
        qtk_onnxruntime_reset(ced->onnx);
#endif
    }

    if (is_end) {
        wtk_ced_prediction(ced, 0.0, 2, 1);
    }
}
