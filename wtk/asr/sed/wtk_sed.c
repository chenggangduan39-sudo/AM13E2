#include "wtk/asr/sed/wtk_sed.h"

/*
 * func:
 *     set state machine para
 *
 * input para:
 *     sed: moudle point
 *     cry_smooth: sed->cry->high_n_smooth
 *     cry_salt: sed->cry->n_salt
 *     cry_h: sed->cry->high_threshold
 *     cry_l: sed->cry->low_threshold
 *     shout_smooth: sed->shout->high_n_smooth
 *     shout_salt: sed->shout->n_salt
 *     shout_h: sed->shout->high_threshold
 *     shout_l: sed->shout->low_threshold
 */
void wtk_sed_set_para(wtk_sed_t *sed, int cry_smooth, int cry_salt, float cry_h,
                      float cry_l, int shout_smooth, int shout_salt,
                      float shout_h, float shout_l) {
    sed->cry->high_n_smooth = cry_smooth;
    sed->cry->n_salt = cry_salt;
    sed->cry->high_threshold = cry_h;
    sed->cry->low_threshold = cry_l;

    sed->shout->high_n_smooth = shout_smooth;
    sed->shout->n_salt = shout_salt;
    sed->shout->high_threshold = shout_h;
    sed->shout->low_threshold = shout_l;
}

/*
 * func:
 *     reset state machine struct
 *
 * input para:
 *     sed: moudle point
 *     sn: the current state machine node
 */
void wtk_sed_state_node_reset(wtk_sed_t *sed, wtk_sed_state_node_t *sn) {
    sn->begin = 0;
    sn->end = 0;
    sn->len = 0;
    sn->wait_len = 0;
    sn->is_high = 0;
    sn->state = WTK_SED_NO_EVENT;
}

wtk_sed_t *wtk_sed_new(wtk_sed_cfg_t *cfg) {
    int i = 0;
    wtk_sed_t *sed = NULL;

    sed = (wtk_sed_t *)wtk_malloc(sizeof(wtk_sed_t));
    sed->cfg = cfg;

    /*
     * win_node to save onnx output prob and to compute averge is struct that
     * is the length of every wav_step
     */
    sed->win_node = (wtk_sed_win_node_t **)wtk_malloc(
        sizeof(wtk_sed_win_node_t *) * cfg->sample_duration);
    for (i = 0; i < cfg->sample_duration; i++) {
        sed->win_node[i] =
            (wtk_sed_win_node_t *)wtk_malloc(sizeof(wtk_sed_win_node_t));
        sed->win_node[i]->prob = NULL;
    }

    /*
     * cry and shout that is same in the data structure and processing method
     * is sound event
     */
    sed->cry = wtk_malloc(sizeof(wtk_sed_state_node_t));
    sed->cry->high_threshold = cfg->cry_params.high_threshold;
    sed->cry->low_threshold = cfg->cry_params.low_threshold;
    sed->cry->high_n_smooth = cfg->cry_params.high_n_smooth;
    sed->cry->low_n_smooth = cfg->cry_params.low_n_smooth;
    sed->cry->n_salt = cfg->cry_params.n_salt;
    sed->cry->etype = WTK_SED_CRY;
    sed->cry->prediction_index = 0;
    wtk_sed_state_node_reset(sed, sed->cry);

    sed->shout = wtk_malloc(sizeof(wtk_sed_state_node_t));
    sed->shout->high_threshold = cfg->shout_params.high_threshold;
    sed->shout->low_threshold = cfg->shout_params.low_threshold;
    sed->shout->high_n_smooth = cfg->shout_params.high_n_smooth;
    sed->shout->low_n_smooth = cfg->shout_params.low_n_smooth;
    sed->shout->n_salt = cfg->shout_params.n_salt;
    sed->shout->etype = WTK_SED_SHOUT;
    sed->shout->prediction_index = 0;
    wtk_sed_state_node_reset(sed, sed->shout);

    /*
     * mean is used to compute the mean of the current probability queue
     */
    sed->mean = (float *)wtk_malloc(sizeof(float) * 2);

    /*
     * robin is used to manage memory of the win_node
     */
    sed->robin = wtk_robin_new(cfg->sample_duration);
    /*
     * wav is used to save pcm sample data when pcm data is transform from short
     * to float format
     */
    sed->wav = wtk_strbuf_new(
        cfg->sample_duration * cfg->sample_rate * sizeof(float), 1);
#ifdef ONNX_DEC
    sed->onnx = qtk_onnxruntime_new(&(cfg->onnx));
#endif
    return sed;
}

void wtk_sed_delete(wtk_sed_t *sed) {
    int i = 0;
    if (sed->mean) {
        wtk_free(sed->mean);
    }
    wtk_robin_delete(sed->robin);
    if (sed->win_node) {
        for (i = 0; i < sed->cfg->sample_duration; i++) {
            if (sed->win_node[i]) {
                if (sed->win_node[i]->prob) {
                    wtk_free(sed->win_node[i]->prob);
                }
                wtk_free(sed->win_node[i]);
            }
        }
        wtk_free(sed->win_node);
    }
    if (sed->cry) {
        wtk_free(sed->cry);
    }
    if (sed->shout) {
        wtk_free(sed->shout);
    }
#ifdef ONNX_DEC
    if (sed->onnx) {
        qtk_onnxruntime_delete(sed->onnx);
    }
    if (sed->wav) {
        wtk_strbuf_delete(sed->wav);
    }
#endif
    wtk_free(sed);
}

void wtk_sed_reset(wtk_sed_t *sed) {
    /*
     * sound event time index need to be reset when reset and it is not used in
     * wtk_sed_state_node_reset because it is also called when using notify
     */
    sed->cry->prediction_index = 0;
    sed->shout->prediction_index = 0;

    wtk_sed_state_node_reset(sed, sed->cry);
    wtk_sed_state_node_reset(sed, sed->shout);

    wtk_robin_reset(sed->robin);
#ifdef ONNX_DEC
    wtk_strbuf_reset(sed->wav);
    qtk_onnxruntime_reset(sed->onnx);
#endif
}

void wtk_sed_set_notify(wtk_sed_t *sed, wtk_sed_notify_f notify, void *ths) {
    sed->notify = notify;
    sed->notify_ths = ths;
}

/*
 * func:
 *     the default sample rate of input wav is 16k and its data is saved in
 *     sed->wav after every sampel point is converted from short to float.
 *
 * input para:
 *     sed: moudle point
 *     data: the point of input data
 *     len: the char len of input data
 */
void wtk_sed_save_wav(wtk_sed_t *sed, char *data, int len) {
    int
        // traverse sample point to convert
        i = 0,
        // sample_num is the number of wav sample point
        sample_num = 0;
    short
        // sample_data is the wav data of short-type point
        *sample_data = NULL;
    float
        // sample_point is used to temporarily save the floating-point value of
        // each sample point of wav audio.
        sample_point = 0.0;

    sample_num = len / sed->cfg->byte_per_sample;
    sample_data = (short *)data;
    for (i = 0; i < sample_num; i++) {
        sample_point = ((float)sample_data[i]) / 32768;
        wtk_strbuf_push(sed->wav, (char *)(&sample_point), sizeof(float));
    }
}

/*
 * func:
 *     judge the before and after of prob queue which include high_threshold
 *     if is_high is 0, the before and after of prob queue all not include
 *     high_threshold
 *     if is_high is 1, the before of prob queue include high_threshold and the
 *     after of prob queue not include high_threshold
 *     if is_high is 2, the before of prob queue not include high_threshold and
 *     the after of prob queue include high_threshold
 *     if is_high is 3, the before and after of prob queue all include
 *     high_threshold
 *
 * input para:
 *     sn: the current state machine node
 *     prob: the single prob value
 */
void wtk_sed_get_is_high(wtk_sed_state_node_t *sn, float prob) {
    /*
     * judge whether the before and after of prob queue according to wait_len
     */
    if (prob > sn->high_threshold && 0 == sn->is_high) {
        sn->is_high = 1;
    } else if (prob > sn->high_threshold && 0 == sn->is_high &&
               sn->wait_len > 0) {
        sn->is_high = 2;
    } else if (prob > sn->high_threshold && 1 == sn->is_high &&
               sn->wait_len > 0) {
        sn->is_high = 3;
    }
}

/*
 * func:
 *     judge to smooth prob queue or to call notify according to which state
 *     at now
 *
 * input para:
 *     sed: moudle point
 *     sn: the current state machine node
 *     state: the state machine of state
 *     is_end: is_end == 1 means call feed_end func
 */
void wtk_sed_smooth(wtk_sed_t *sed, wtk_sed_state_node_t *sn,
                    wtk_sed_state_e state, int is_end) {
    switch (state) {
        /*
         * there is not smoothing or calling notify at the NO_EVENT state
         */
    case WTK_SED_NO_EVENT:
        break;
    case WTK_SED_IS_EVENT:
        switch (sn->is_high) {
            /*
             * when sn->is_high is 0, just to judge whether prob queue is
             * smoothed
             */
        case 0:
            if (sn->wait_len > sn->low_n_smooth) {
                sn->len -= sn->end - sn->begin;
                sn->begin = sn->end + sn->wait_len;
                sn->wait_len = 0;
            } else {
                sn->len += sn->wait_len;
                sn->wait_len = 0;
            }
            break;
        case 1:
            /*
             * when sn->is_high is 0, calling notify if prob queue is not
             * smoothed and smoothing prob queue if it can be smoothed
             * assignment of is_high is 0 because the after of prob queue not
             * include high_threshold
             */
            if (sn->wait_len > sn->low_n_smooth) {
                if ((sn->end - sn->begin) > sn->n_salt && sn->begin >= 0 &&
                    sn->is_high > 0) {
                    sed->notify(sed, sn->etype,
                                sn->begin * sed->cfg->seconds_per_index,
                                sn->end * sed->cfg->seconds_per_index);
                }
                sn->len -= sn->end - sn->begin;
                sn->is_high = 0;
                sn->begin = sn->end + sn->wait_len;
                sn->wait_len = 0;
            } else {
                sn->len += sn->wait_len;
                sn->wait_len = 0;
                sn->is_high = 1;
            }
            /*
             * when is_end is 1, calling notify if prob queue satisty the notify
             * condition
             */
            if (is_end) {
                if (sn->len > sn->n_salt && sn->begin >= 0 && sn->is_high > 0) {
                    sed->notify(
                        sed, sn->etype, sn->begin * sed->cfg->seconds_per_index,
                        (sn->begin + sn->len) * sed->cfg->seconds_per_index);
                }
            }
            break;
        case 2:
            /*
             * when sn->is_high is 2, smoothing prob queue if it can be smoothed
             * assignment of is_high is 1 because the after of prob queue
             * include high_threshold
             */
            if (sn->wait_len > sn->low_n_smooth) {
                sn->len -= sn->end - sn->begin;
                sn->begin = sn->end + sn->wait_len;
                sn->wait_len = 0;
                sn->is_high = 1;
            } else {
                sn->len += sn->wait_len;
                sn->wait_len = 0;
                sn->is_high = 1;
            }
            /*
             * when is_end is 1, calling notify if prob queue satisty the notify
             * condition
             */
            if (is_end) {
                if (sn->len > sn->n_salt && sn->begin >= 0 && sn->is_high > 0) {
                    sed->notify(
                        sed, sn->etype, sn->begin * sed->cfg->seconds_per_index,
                        (sn->begin + sn->len) * sed->cfg->seconds_per_index);
                }
            }
            break;
        case 3:
            /*
             * when sn->is_high is 3, calling notify if prob queue is not
             * smoothed and smoothing prob queue if it can be smoothed
             * assignment of is_high is 1 because the after of prob queue
             * include high_threshold
             */
            if (sn->wait_len > sn->high_n_smooth) {
                if ((sn->end - sn->begin) > sn->n_salt && sn->begin >= 0 &&
                    sn->is_high > 0) {
                    sed->notify(sed, sn->etype,
                                sn->begin * sed->cfg->seconds_per_index,
                                sn->end * sed->cfg->seconds_per_index);
                }
                sn->len -= sn->end - sn->begin;
                sn->is_high = 1;
                sn->begin = sn->end + sn->wait_len;
                sn->wait_len = 0;
            } else {
                sn->len += sn->wait_len;
                sn->wait_len = 0;
                sn->is_high = 1;
            }
            /*
             * when is_end is 1, calling notify if prob queue satisty the notify
             * condition
             */
            if (is_end) {
                if (sn->len > sn->n_salt && sn->begin >= 0 && sn->is_high > 0) {
                    sed->notify(
                        sed, sn->etype, sn->begin * sed->cfg->seconds_per_index,
                        (sn->begin + sn->len) * sed->cfg->seconds_per_index);
                }
            }
            break;
        default:
            break;
        }
        /*
         * when is_end is 1, reset the struct of state machine
         */
        if (is_end) {
            wtk_sed_state_node_reset(sed, sn);
        }
        break;
    case WTK_SED_WAIT_EVENT:
        break;
    default:
        break;
    }
}

/*
 * func:
 *     deal with the single class of prob in the state machine
 *
 * input para:
 *     sed: moudle point
 *     sn: the current state machine node
 *     prob: the single prob value
 *     is_end: is_end == 1 means call feed_end func
 */
void wtk_sed_state(wtk_sed_t *sed, wtk_sed_state_node_t *sn, float prob,
                   int is_end) {
    switch (sn->state) {
    case WTK_SED_NO_EVENT:
        /*
         * enter IS_EVENT state when prob is biger than low_threshold
         */
        if (prob > sn->low_threshold) {
            /*
             * save index as begin index
             */
            sn->begin = sn->prediction_index;
            sn->state = WTK_SED_IS_EVENT;
            /*
             * the len of prob queue whose value is biger than low_threshold
             * plus one
             */
            sn->len++;
            /*
             * estimate current smooth state
             */
            wtk_sed_get_is_high(sn, prob);
        }
        break;
    case WTK_SED_IS_EVENT:
        /*
         * keep IS_EVENT state when prob is biger than low_threshold
         * enter WAIT_EVENT state when prob is smaller than low_threshold
         */
        if (prob > sn->low_threshold) {
            sn->len++;
            wtk_sed_get_is_high(sn, prob);
        } else {
            /*
             * judge whether the prob queue are merged before and after before
             * entering the WAIT_EVENT state for the second time
             */
            if (sn->wait_len > 0) {
                wtk_sed_smooth(sed, sn, sn->state, 0);
            }
            sn->state = WTK_SED_WAIT_EVENT;
            sn->wait_len++;
            sn->end = sn->prediction_index;
        }
        break;
    case WTK_SED_WAIT_EVENT:
        if (prob > sn->low_threshold) {
            sn->len++;
            /*
             * estimate current smooth state
             */
            wtk_sed_get_is_high(sn, prob);
        } else {
            sn->wait_len++;
        }

        /*
         * wtk_sed_smooth need to know current state
         */
        if (prob > sn->low_threshold) {
            sn->state = WTK_SED_IS_EVENT;
        }

        /*
         * judge whether the prob queue are discarded after
         * entering the NO_EVENT state
         */
        if (1 == sn->is_high) {
            if (sn->wait_len > sn->high_n_smooth || is_end) {
                if (sn->len > sn->n_salt && sn->begin >= 0 && sn->is_high > 0) {
                    sed->notify(sed, sn->etype,
                                sn->begin * sed->cfg->seconds_per_index,
                                sn->end * sed->cfg->seconds_per_index);
                    wtk_sed_state_node_reset(sed, sn);
                }
            }
        } else if (0 == sn->is_high) {
            if (sn->wait_len > sn->low_n_smooth) {
                wtk_sed_state_node_reset(sed, sn);
            }
        }
        break;
    default:
        break;
    }

    /*
     * record prediction index and every index is 10ms
     */
    sn->prediction_index++;

    /*
     * merging all prob and judge whether calling notify when feed_end
     */
    if (is_end) {
        wtk_sed_smooth(sed, sn, sn->state, 1);
    }
}

/*
 * func:
 *     separate the class of prob
 *
 * input para:
 *     sed: moudle point
 *     prob: the single prob value
 *     event_type: the class of sound event eg: 0 is crying and 1 is shout
 *     is_end: is_end == 1 means call feed_end func
 */
void wtk_sed_classes_choice(wtk_sed_t *sed, float prob, int event_type,
                            int is_end) {
    wtk_sed_state_node_t
        // current state node
        *cur_sn = NULL;

    /*
     * select current node point according to event type
     */
    switch (event_type) {
    case WTK_SED_CRY:
        cur_sn = sed->cry;
        break;
    case WTK_SED_SHOUT:
        cur_sn = sed->shout;
        break;
    default:
        wtk_debug("evevt type error\n");
        exit(0);
    }
    /*
     * deal with the single prob every event type
     */
    wtk_sed_state(sed, cur_sn, prob, 0);

    if (is_end) {
        wtk_sed_state(sed, cur_sn, 0.0, 1);
    }
}

/*
 * func:
 *     separate the dim of prob
 *
 * input para:
 *     sed: moudle point
 *     prob: the point of input prob
 *     frames_num: the number of frame which is 10ms every frame at now
 *     classes_num: the number of classes which is 2 at now
 *     is_end: is_end == 1 means call feed_end func
 */
void wtk_sed_prediction(wtk_sed_t *sed, float *prob, int frames_num,
                        int classes_num, int is_end) {
    int
        // for loop
        i = 0,
        j = 0,
        // the max prob classes of index
        index = 0;
    float
        // the current mean biger class of threshold
        threshold = 100.0;

    /*
     * get mean of the number of frames_num
     */
    for (i = 0; i < classes_num; i++) {
        sed->mean[i] = 0.0;
    }

    for (i = 0; i < frames_num; i++) {
        for (j = 0; j < classes_num; j++) {
            sed->mean[j] =
                (sed->mean[j] * (i) + *(prob + i * classes_num + j)) / (i + 1);
        }
    }

    /*
     * get the event type whose mean is biger
     */
    index = wtk_float_argmax(sed->mean, classes_num);

    /*
     * select threshold of event whose mean is biger
     */
    if (0 == index) {
        threshold = sed->cry->low_threshold;
    } else if (1 == index) {
        threshold = sed->shout->low_threshold;
    }

    /*
     * when the biger mean is biger than threshold,the probability of another
     * event is set to zero
     */
    if (sed->mean[index] > threshold) {
        for (i = 0; i < frames_num; i++) {
            for (j = 0; j < classes_num; j++) {
                /*
                 * separate the number of frames and classes
                 */
                if (j == index) {
                    wtk_sed_classes_choice(sed, *(prob + i * classes_num + j),
                                           j, 0);
                } else {
                    wtk_sed_classes_choice(sed, 0.0, j, 0);
                }
            }
        }
    } else {
        for (i = 0; i < frames_num; i++) {
            for (j = 0; j < classes_num; j++) {
                /*
                 * separate the number of frames and classes
                 */
                wtk_sed_classes_choice(sed, *(prob + i * classes_num + j), j,
                                       0);
            }
        }
    }

    if (is_end) {
        for (i = 0; i < classes_num; i++) {
            sed->mean[i] = 0.0;
            wtk_sed_classes_choice(sed, 0.0, i, 1);
        }
    }
}

/*
 * func:
 *     wtk_sed_onnx_compute is used to compute onnx output
 *
 * input para:
 *     sed: moudle point
 *     onnx_input_data: the point of onnx model input prob
 *     onnx_input_len: the number of onnx model input
 *     out_shape: the onnx model of output shape point
 *
 * output para:
 *     the output point of onnx model
 */
float *wtk_sed_onnx_compute(wtk_sed_t *sed, float *onnx_input_data,
                            int onnx_input_len, int64_t **out_shape) {
    float
        // the point of onnx model output
        *output_point = NULL;
#ifdef ONNX_DEC
    int64_t
        // the dim of in and out shape
        in_shape_dim = 0,
        out_shape_dim = 0,
        // the index of onnx model output that is frame and clip
        onnx_output_index = 0,
        // the shape of input
            *in_shape = NULL;

    /*
     * it is need to get onnx output index
     */
    onnx_output_index = sed->cfg->onnx_output_index;
    /*
     * get input shape
     */
    in_shape = qtk_onnxruntime_get_inshape(sed->onnx, 0, &in_shape_dim);
    /*
     * default value is replaced to input len
     */
    in_shape[1] = onnx_input_len;

    /*
     * get onnx input
     */
    qtk_onnxruntime_feed(sed->onnx, onnx_input_data, onnx_input_len,
                         cast(int64_t *, in_shape), in_shape_dim, 0, 0);
    qtk_onnxruntime_run(sed->onnx);
    /*
     * get output shape
     */
    *out_shape = qtk_onnxruntime_get_outshape(sed->onnx, onnx_output_index,
                                              &out_shape_dim);
    /*
     * get output point
     */
    output_point = qtk_onnxruntime_getout(sed->onnx, onnx_output_index);
    /*
     * in_shape is freed every time
     */
    if (in_shape) {
        wtk_free(in_shape);
    }
#endif
    return output_point;
}

/*
 * func:
 *     wtk_sed_merge is used to compute average which is the coincident part of
 * the two onnx outputs
 *
 * input para:
 *     sed: moudle point
 *     prob: the point of onnx model output prob
 *     frames_num: the frame num of onnx model output
 *     classes_num: the class num of onnx model output
 *
 * output para:
 *     the point of wtk_sed_win_node_t is used to send prediction
 */
wtk_sed_win_node_t *wtk_sed_merge(wtk_sed_t *sed, float *prob, int frames_num,
                                  int classes_num, int is_end) {
    int
        // for lopp
        i = 0,
        j = 0;
    wtk_sed_win_node_t
        // the point of current window node
        *wn = NULL;

    if (0 == sed->robin->used) {
        /*
         * create window node when robin is empty and save input prob to every
         * node
         */
        for (i = 0; i < sed->cfg->sample_duration; i++) {
            sed->win_node[i]->avr_count = 1;
            sed->win_node[i]->num = sed->cfg->index_per_second;
            sed->win_node[i]->classes = classes_num;
            if (!(sed->win_node[i]->prob)) {
                sed->win_node[i]->prob =
                    (float *)wtk_malloc(sizeof(float) * sed->win_node[i]->num *
                                        sed->win_node[i]->classes);
            }
            memmove(sed->win_node[i]->prob,
                    prob + i * sed->cfg->index_per_second * classes_num,
                    sed->cfg->index_per_second * classes_num * sizeof(float));
            wtk_robin_push(sed->robin, sed->win_node[i]);
        }
        /*
         * return the first node because it is no change even if new data
         * is sended
         */
        return wtk_robin_pop(sed->robin);
    } else {
        for (i = 0; i < sed->cfg->sample_duration; i++) {
            /*
             * pop every node and compute average with input prob except for the
             * node at the end that is used to save the last part of input prob
             */
            wn = wtk_robin_pop(sed->robin);
            if (i != (sed->cfg->sample_duration - 1)) {
                for (j = 0; j < wn->num * wn->classes; j++) {
                    wn->prob[j] =
                        (wn->prob[j] * wn->avr_count +
                         *(prob + i * sed->cfg->index_per_second * wn->classes +
                           j)) /
                        (wn->avr_count + 1);
                }
                wn->avr_count++;
                wtk_robin_push(sed->robin, wn);
            } else {
                wn->avr_count = 1;
                wn->num = frames_num / sed->cfg->sample_duration;
                wn->classes = classes_num;
                memmove(wn->prob,
                        prob + i * sed->cfg->index_per_second * classes_num,
                        sed->cfg->index_per_second * classes_num *
                            sizeof(float));
                wtk_robin_push(sed->robin, wn);
            }
        }
        /*
         * return the first node because it is no change even if new data
         * is sended
         */
        return wtk_robin_pop(sed->robin);
    }
}

void wtk_sed_feed(wtk_sed_t *sed, char *data, int len, int is_end) {
#ifdef ONNX_DEC
    int
        // onnx model input length is fixed
        onnx_input_len = 0,
        // the wav_step length of data need to be pop after data is send to
        // onnx model compute
        pop_bytes = 0,
        // the input length of prediction processing and the classes of
        // the sound event
        predict_input_len = 0, predict_input_classes = 0,
        // the input length of merge prob and compute average processing and
        // the classes of the sound event
        merge_input_len = 0, merge_input_classes = 0;
    int64_t
        // the shape of onnx model output array
        *out_shape = NULL;
    float
        // the point of onnx in and out
        *onnx_input_point = NULL,
        *onnx_out_point = NULL,
        // the input prob point of prediction process
            *predict_input_prob = NULL;
    wtk_sed_win_node_t
        // current window node to save prob and other information for prediction
        *cur_wn = NULL;

    /*
     * convert audio data from short to float and save data to sed->wav
     */
    wtk_sed_save_wav(sed, data, len);
    onnx_input_len =
        sed->cfg->sample_duration * sed->cfg->sample_rate * sizeof(float);
    /*
     * data which is the len of sample_duration is sended to onnx model to
     * compute and data which is the len of wav_step is pop when the len of pos
     * is longer than onnx_input_len
     */
    while (sed->wav->pos >= onnx_input_len) {
        onnx_input_point = (float *)sed->wav->data;
        /*
         * the number of onnx_input_len / sizeof(float) sample point data is
         * sended to onnx to compute.
         */
        onnx_out_point = wtk_sed_onnx_compute(
            sed, onnx_input_point, onnx_input_len / sizeof(float), &out_shape);

        /*
         * the wav_step len of data is needed to pop
         */
        pop_bytes = sed->cfg->wav_step * sed->cfg->sample_rate * sizeof(float);
        wtk_strbuf_pop(sed->wav, NULL, pop_bytes);

        /*
         * len and class is the output shape of onnx
         */
        merge_input_len = out_shape[1];
        merge_input_classes = out_shape[2];

        /*
         * get current window node to send prediction
         */
        cur_wn = wtk_sed_merge(sed, onnx_out_point, merge_input_len,
                               merge_input_classes, is_end);
        /*
         * the information of prediction process is need to save in window node
         */
        predict_input_prob = cur_wn->prob;
        predict_input_len = cur_wn->num;
        predict_input_classes = cur_wn->classes;
        /*
         * prediction process
         */
        wtk_sed_prediction(sed, predict_input_prob, predict_input_len,
                           predict_input_classes, 0);
        /*
         * the memory of window node is reuse after it is sended to prediction
         */
        wtk_robin_push(sed->robin, cur_wn);

        /*
         * reset and free the onnx of in and out struct every to run onnx
         */
        qtk_onnxruntime_reset(sed->onnx);
        wtk_free(out_shape);
        out_shape = NULL;
    }

    if (is_end) {
        /*
         * every window node is send to prediction when feed end
         */
        while (sed->robin->used > 1) {
            cur_wn = wtk_robin_pop(sed->robin);
            predict_input_prob = cur_wn->prob;
            predict_input_len = cur_wn->num;
            predict_input_classes = cur_wn->classes;
            wtk_sed_prediction(sed, predict_input_prob, predict_input_len,
                               predict_input_classes, 0);
        }
        /*
         * the classes of 2 is most simple when is_end
         */
        wtk_sed_prediction(sed, NULL, 0, 2, 1);
    }
#endif
}
