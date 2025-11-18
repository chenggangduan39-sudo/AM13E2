#include "wtk/asr/kws/clu/wtk_clu.h"

wtk_clu_t *wtk_clu_new(wtk_clu_cfg_t *cfg) {
    wtk_clu_t *clu = NULL;
    clu = (wtk_clu_t *)wtk_malloc(sizeof(wtk_clu_t));
    clu->cfg = cfg;
    /*
     * to save audio wav
     */
    clu->wav = wtk_strbuf_new(
        cfg->sample_rate * cfg->byte_per_sample * cfg->vprint_num_byte, 2);
    /*
     * save length and data point which is sended to onnx compute
     */
    clu->win_l = clu->wav->pos;
    /*
     * compute vprint vector
     */
    clu->x = wtk_nnet3_xvector_compute_new2(&cfg->xvector);
    /*
     * compute eigen value decomposition
     */
    clu->spec = qtk_clustering_spectral_new(-1, cfg->vprint_size, -1);
    clu->spec->pval = 0.022;
    /*
     * to save vprint data
     */
    clu->vprint_idx = NULL;
    clu->vprint_data = NULL;
    clu->two_s_buffer = wtk_strbuf_new(
        cfg->chunk_dur * cfg->sample_rate * cfg->byte_per_sample, 1);

    clu->vprint_idx_num = 0;
    clu->result = NULL;
    return clu;
}

void wtk_clu_set_oracle_num(wtk_clu_t *clu, int oracle_num) {
    clu->spec->ncluster = oracle_num;
}

void wtk_clu_delete(wtk_clu_t *clu) {
    if (clu->two_s_buffer) {
        wtk_strbuf_delete(clu->two_s_buffer);
    }
    if (clu->spec) {
        qtk_clustering_spectral_delete(clu->spec);
    }
    if (clu->x) {
        wtk_nnet3_xvector_compute_delete(clu->x);
    }
    if (clu->wav) {
        wtk_strbuf_delete(clu->wav);
    }
    if (clu->result) {
        wtk_strbufs_delete(clu->result,clu->vprint_idx_num);
    }
    wtk_free(clu);
}

void wtk_clu_reset(wtk_clu_t *clu) {
    if (clu->result) {
        wtk_strbufs_delete(clu->result,clu->vprint_idx_num);
        clu->result = NULL;
    }
    clu->win_l = 0;
    clu->vprint_idx_num = 0;
    wtk_strbuf_reset(clu->two_s_buffer);
    wtk_strbuf_reset(clu->wav);
    qtk_clustering_spectral_reset(clu->spec);
}

/*
 * func:
 *     compute the every chunk of voiceprint vector
 *
 * input para:
 *     clu: moudle point
 */
//int cntx=0;
void wtk_clu_compute_chunk_vprint(wtk_clu_t *clu) {
    wtk_vecf_t
        // temporary preservation of voiceprint vector
        *vprint_vec = NULL;
    wtk_nnet3_xvector_compute_feed(
        clu->x, (short *)(clu->wav->data + clu->wav->pos - clu->win_l),
        clu->cfg->chunk_num_byte / clu->cfg->byte_per_sample, 0);
    /*
     * subtract the calculated audio length
     */
    clu->win_l -= clu->cfg->hop_num_byte;
    wtk_nnet3_xvector_compute_feed(clu->x, NULL, 0, 1);
    vprint_vec = wtk_nnet3_xvector_compute(clu->x);
    //printf("======%d\n",++cntx);
    //print_float(vprint_vec->p,256);
    /*
     * send voiceprint vector to the clustering
     */
    qtk_clustering_spectral_add(clu->spec, vprint_vec->p);
    wtk_nnet3_xvector_compute_reset(clu->x);
}

float wtk_clu_round_n(float a, int n) {
    return 1.0 * (int)(a * pow(10, n) + 0.5) / pow(10, n);
}

/*
 * func:
 *     compute the every chunk of voiceprint vector
 *
 * input para:
 *     clu: moudle point
 *     data: the result of clustering
 *     len: the len of data
 */
void wtk_clu_smooth(wtk_clu_t *clu, int *data, int len) {
    int
        // when flags is one, the last clustering is smoothed
        flags = 0,
        // the para of loop
        i = 0,
        // the clustering result id of start and end
        start_id = 0, end_id = 0,
        // the byte position of start and end and its calculate in order to
        // avoid floating point multiplying by fixed point error
        start_byte = 0, end_byte = 0;
    float
        // the time of start and end
        start_s = 0.0,
        end_s = 0.0;
    /*
     * smooth the first chunk
     */
    if (data[0] != data[1]) {
        data[0] = data[1];
    }
    /*
     * smooth the last chunk
     */
    if ((data[len - 1] != data[len - 2]) &&
        (wtk_clu_round_n((float)(clu->wav->pos) / (clu->cfg->sample_rate *
                                                   clu->cfg->byte_per_sample),
                         2) -
         wtk_clu_round_n((len - 1) * clu->cfg->hop_s + clu->cfg->hop_s / 2,
                         2)) < 1) {
        data[len - 1] = data[len - 2];
    } else {
        flags = 1;
    }

    for (i = 1; i < len; i++) {
        if (i == len - 1) {
            if (data[i] == data[i - 1]) {
                end_id = i;
                start_s = wtk_clu_round_n(
                    start_id * clu->cfg->hop_s + clu->cfg->hop_s / 2, 2);
                start_byte = (int)round((start_id * clu->cfg->hop_s +
                                         clu->cfg->hop_s / 2) *
                                        100) * 320;
                end_s =
                    wtk_clu_round_n(end_id * clu->cfg->hop_s +
                                        clu->cfg->chunk_s - clu->cfg->hop_s / 2, 2);
                                    //wtk_debug("%f %f %d\n",start_s,end_s,data[i - 1]);
                end_byte =
                    (int)round((end_id * clu->cfg->hop_s + clu->cfg->chunk_s -
                                clu->cfg->hop_s / 2) *
                               100) * 320;
                wtk_strbuf_push_int(clu->vprint_idx[data[i - 1]], &start_byte,
                                    1);
                wtk_strbuf_push_int(clu->vprint_idx[data[i - 1]], &end_byte, 1);
            //printf("[%.2f,%.2f,%d]\n",start_s,end_s,data[i-1]);
                wtk_strbuf_push_float(clu->result[data[i-1]],&start_s,1);
                wtk_strbuf_push_float(clu->result[data[i-1]],&end_s,1);
            } else {
                start_s = wtk_clu_round_n(
                    start_id * clu->cfg->hop_s + clu->cfg->hop_s / 2, 2);
                start_byte = (int)round((start_id * clu->cfg->hop_s +
                                         clu->cfg->hop_s / 2) *
                                        100) * 320;
                end_s =
                    wtk_clu_round_n(end_id * clu->cfg->hop_s +
                                        clu->cfg->chunk_s - clu->cfg->hop_s / 2,
                                    2);
                                    //wtk_debug("%f %f %d\n",start_s,end_s,data[i - 1]);
                end_byte =
                    (int)round((end_id * clu->cfg->hop_s + clu->cfg->chunk_s -
                                clu->cfg->hop_s / 2) *
                               100) * 320;
                wtk_strbuf_push_int(clu->vprint_idx[data[i - 1]], &start_byte,
                                    1);
                wtk_strbuf_push_int(clu->vprint_idx[data[i - 1]], &end_byte, 1);
                wtk_strbuf_push_float(clu->result[data[i-1]],&start_s,1);
                wtk_strbuf_push_float(clu->result[data[i-1]],&end_s,1);
            //printf("[%.2f,%.2f,%d]\n",start_s,end_s,data[i-1]);
                if (flags) {
                    start_s = wtk_clu_round_n(
                        i * clu->cfg->hop_s + clu->cfg->hop_s / 2, 2);
                    start_byte =
                        (int)round((i * clu->cfg->hop_s + clu->cfg->hop_s / 2) *
                                   100) * 320;
                    end_s = wtk_clu_round_n(
                        (float)clu->wav->pos /
                            (clu->cfg->sample_rate * clu->cfg->byte_per_sample),
                        2);
                    //wtk_debug("%f %f\n",start_s,end_s);
                    end_byte = (int)round(((float)clu->wav->pos /
                                           (clu->cfg->sample_rate *
                                            clu->cfg->byte_per_sample)) *
                                          100) * 320;
                    wtk_strbuf_push_int(clu->vprint_idx[data[i]], &start_byte,
                                        1);
                    wtk_strbuf_push_int(clu->vprint_idx[data[i]], &end_byte, 1);
                    wtk_strbuf_push_float(clu->result[data[i-1]],&start_s,1);
                    wtk_strbuf_push_float(clu->result[data[i-1]],&end_s,1);
            //printf("[%.2f,%.2f,%d]\n",start_s,end_s,data[i-1]);
                }
            }
        } else if ((data[i] == data[start_id]) ||
                   (data[i] != data[i - 1] && data[i] != data[i + 1])) {
            end_id = i;
        } else {
            if (0 == start_id) {
                start_s = 0.0;
                start_byte = 0;
            } else {
                start_s = wtk_clu_round_n(
                    start_id * clu->cfg->hop_s + clu->cfg->hop_s / 2, 2);
                start_byte = (int)round((start_id * clu->cfg->hop_s +
                                         clu->cfg->hop_s / 2) *
                                        100) * 320;
            }
            if (end_id == len - 1) {
                end_s = wtk_clu_round_n(
                    (float)clu->wav->pos /
                        (clu->cfg->sample_rate * clu->cfg->byte_per_sample),
                    2);
                end_byte = (int)round(((float)clu->wav->pos /
                                       (clu->cfg->sample_rate *
                                        clu->cfg->byte_per_sample)) *
                                      100) * 320;
            } else {
                end_s =
                    wtk_clu_round_n(end_id * clu->cfg->hop_s +
                                        clu->cfg->chunk_s - clu->cfg->hop_s / 2,
                                    2);
                end_byte =
                    (int)round((end_id * clu->cfg->hop_s + clu->cfg->chunk_s -
                                clu->cfg->hop_s / 2) *
                               100) * 320;
            }
            wtk_strbuf_push_int(clu->vprint_idx[data[i - 1]], &start_byte, 1);
            wtk_strbuf_push_int(clu->vprint_idx[data[i - 1]], &end_byte, 1);
            wtk_strbuf_push_float(clu->result[data[i-1]],&start_s,1);
            wtk_strbuf_push_float(clu->result[data[i-1]],&end_s,1);
            //printf("[%.2f,%.2f,%d]\n",start_s,end_s,data[i-1]);
            start_id = i;
            end_id = -1;
        }
    }
    (void)end_s;
    (void)start_s;
}

void wtk_clu_L2_normalization(float *data, int len, int mean) {
    int i = 0;
    float L2 = 0.0;
    float eps = 1e-12;

    for (i = 0; i < len; i++) {
        L2 += data[i] * data[i];
    }
    if (L2 <= 0.0) {
        L2 = eps;
    }else{
        L2 = pow(L2, 0.5);
    }

    for (i = 0; i < len; i++) {
        data[i] /= L2;
    }
}

/*
 * func:
 *    create vprint_data to save voiceprint information
 *
 * input para:
 *     size: the size of vector to voiceprint
 *     num: the number of speaker
 */
wtk_clu_vprint_data_t *wtk_clu_create_vprint_data(int size, int num) {
    int i = 0;
    wtk_clu_vprint_data_t *vprint_data = NULL;
    vprint_data = (wtk_clu_vprint_data_t *)wtk_malloc(
        sizeof(wtk_clu_vprint_data_t) +
        num * (sizeof(int) + sizeof(float) * size));
    vprint_data->num = num;
    vprint_data->size = size;
    /*
     * save the number of vector to compute mean
     */
    vprint_data->mean = (int *)((char *)vprint_data + sizeof(wtk_clu_vprint_data_t));
    /*
     * save the voiceprint vector of data
     */
    vprint_data->data = (float *)((char *)vprint_data->mean + num * sizeof(int));
    memset(vprint_data->data,0,sizeof(float)* size *num);
    /*
     * set zero for mean
     */
    for (i = 0; i < num; i++) {
        vprint_data->mean[i] = 0;
    }

    return vprint_data;
}

void wtk_clu_delete_vprint_data(wtk_clu_vprint_data_t *vprint_data) {
    if (vprint_data) {
        wtk_free(vprint_data);
    }
}

/*
 * func:
 *    compute voiceprint vector and its mean to save to vprint_data
 *
 * input para:
 *     clu: moudle point
 *     id: the current speaker of id
 *     data: the result of voiceprint
 *     len: the len of data
 */
void wtk_clu_push_vprint_data(wtk_clu_vprint_data_t *vprint_data, int id,
                              float *data, int len) { 
    int
        // the para of loop
        i = 0;
    float
        // the current speaker of data point
        *data_p = NULL;

    data_p = vprint_data->data + id * vprint_data->size;

    /*
     * the first data is assignment and compute the mean of data after that
     */
    if (0 == vprint_data->mean[id]) {
        for (i = 0; i < vprint_data->size; i++) {
            data_p[i] = data[i];
        }
    } else {
        for (i = 0; i < vprint_data->size; i++) {
            data_p[i] = (data_p[i] * vprint_data->mean[id] + data[i]) /
                        (vprint_data->mean[id] + 1);
        }
    }
    /*
     * recore the number of voiceprint vector to compute mean
     */
    vprint_data->mean[id]++;
}

/*
 * func:
 *     the mean voiceprint of L2 normalization
 *
 * input para:
 *     vprint_data: voiceprint data information point
 */
void wtk_clu_vprint_data_normalization(wtk_clu_vprint_data_t *vprint_data) {
    int i = 0;
    for (i = 0; i < vprint_data->num; i++) {
        wtk_clu_L2_normalization(vprint_data->data + i * vprint_data->size,
                                 vprint_data->size, vprint_data->mean[i]);
    }
}

/*
 * func:
 *    compute voiceprint vector and its mean to save to vprint_data
 *
 * input para:
 *     clu: moudle point
 *     id: the current speaker of id
 */
void wtk_clu_compute_enroll_vprint(wtk_clu_t *clu, int id) {
    wtk_vecf_t
        // to tmp save vprint vector
        *vprint_vec = NULL;
    wtk_nnet3_xvector_compute_feed(clu->x, (short *)(clu->two_s_buffer->data),
                                   clu->cfg->chunk_dur * clu->cfg->sample_rate,
                                   0);
    wtk_nnet3_xvector_compute_feed(clu->x, NULL, 0, 1);
    vprint_vec = wtk_nnet3_xvector_compute(clu->x);
    /*
     * vprint data to save vprint_data
     */
    wtk_clu_push_vprint_data(clu->vprint_data, id, vprint_vec->p,
                             vprint_vec->len);
    wtk_nnet3_xvector_compute_reset(clu->x);
}

/*
 * func:
 *    get every speaker the mean of voiceprint
 *
 * input para:
 *     clu: moudle point
 *     vprint_idx_num: the clustering of people number
 *     data: the result of clustering
 *     len: the len of data
 */
void wtk_clu_get_vprint(wtk_clu_t *clu, int vprint_idx_num, int *data,
                        int len) {
    int
        // the para of loop
        i = 0,
        j = 0,
        // the byte position of start and end
        start_byte = 0, end_byte = 0;
    for (i = 0; i < vprint_idx_num; i++) {
        for (j = 0; j < clu->vprint_idx[i]->pos / (sizeof(int) * 2); j++) {
            start_byte =
                *(int *)(clu->vprint_idx[i]->data + j * (sizeof(int) * 2));
            end_byte = *(int *)(clu->vprint_idx[i]->data +
                                j * (sizeof(int) * 2) + sizeof(int));
            /*
             * After each audio is se nt to the two_s_buffer, it is calculated
             * continuously until the data in the buffer is less than 2s.
             */
            wtk_strbuf_push(clu->two_s_buffer, clu->wav->data + start_byte,
                            end_byte - start_byte);
            while (clu->two_s_buffer->pos >= clu->cfg->chunk_dur *
                                                 clu->cfg->byte_per_sample *
                                                 clu->cfg->sample_rate) {
                wtk_clu_compute_enroll_vprint(clu, i);
                wtk_strbuf_pop(clu->two_s_buffer, NULL,
                               clu->cfg->chunk_dur * clu->cfg->byte_per_sample *
                                   clu->cfg->sample_rate);
            }
        }
        wtk_strbuf_reset(clu->two_s_buffer);
    }
}

/*
 * func:
 *     Processing the output voiceprint vector of general clustering results
 *
 * input para:
 *     clu: moudle point
 *     vprint_idx_num: the clustering of people number
 *     data: the result of clustering
 *     len: the len of data
 */
void wtk_clu_post_process(wtk_clu_t *clu, int vprint_idx_num, int *data,
                          int len) {
    /*
     * smooth the result of clustering
     */
    wtk_clu_smooth(clu, data, len);
    /*
     * get the vprint of every speaker
     */
    wtk_clu_get_vprint(clu, vprint_idx_num, data, len);
    /*
     * normalization for vprint data
     */
    wtk_clu_vprint_data_normalization(clu->vprint_data);
}

void wtk_clu_feed(wtk_clu_t *clu, char *data, int len, int is_end) {
    int
        // the number of speaker by the clustering
        vprint_idx_num = 0,
        // the result of clustering
        *labels = NULL;
    /*
     * to save audio and convert from short to float
     */
    wtk_strbuf_push(clu->wav, data, len);
    /*
     * record the current time point at which the voiceprint is calculated
     */
    clu->win_l += len;
    /*
     * to send audio to compute onnx and use the point and len of clu to
     * recorder send audio info to not pop audio which is compute vprint after
     * computing eigen value decomposition and geting audio time info
     */
    while (clu->win_l >= clu->cfg->chunk_num_byte) {
        wtk_clu_compute_chunk_vprint(clu);
    }

    /*
     * computing eigen value decomposition and computing vprint to notify
     */
    if (is_end) {
        if(clu->wav->pos < clu->cfg->chunk_num_byte * clu->spec->ncluster){
            return;
        }
        /*
         * clustering
         */
        labels = qtk_clustering_spectral_get_result(clu->spec);
        clu->vprint_idx_num = clu->spec->ncluster;
        vprint_idx_num = clu->spec->ncluster;
        /*
         * create vprint_idx to save vprint time information when vprint_idx_num
         * is known
         */
        clu->vprint_idx = wtk_strbufs_new(vprint_idx_num);
        clu->result = wtk_strbufs_new(vprint_idx_num);
        /*
         * create vprint_data to save vprint data which is computed to mean and
         * normalization when vprint_idx_num is known
         */
        clu->vprint_data =
            wtk_clu_create_vprint_data(clu->cfg->vprint_size, vprint_idx_num);
        /*
         * post processing
         */
        wtk_clu_post_process(clu, vprint_idx_num, labels, clu->spec->N);
        /*
         * notify voiceprint data
         */
        clu->notify(clu->notify_ths, vprint_idx_num, clu->vprint_data->size,
                    clu->vprint_data->data);
        /*
         * reset related value and delete vprint memory
         */
        wtk_strbufs_delete(clu->vprint_idx, vprint_idx_num);
        wtk_clu_delete_vprint_data(clu->vprint_data);
        //wtk_clu_reset(clu);
    }
}

void wtk_clu_set_notify(wtk_clu_t *clu, wtk_clu_notify_f notify, void *ths) {
    clu->notify = notify;
    clu->notify_ths = ths;
}
