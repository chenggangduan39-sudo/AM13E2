#include "qtk/avspeech/qtk_avspeech_lip.h"
#include "qtk/avspeech/qtk_avspeech_lip_cfg.h"
#include "qtk/cv/utils/qtk_lip_cropper.h"
#include "qtk/image/qtk_image.h"
#include "wtk/core/wtk_sort.h"
#include <float.h>

typedef struct {
    wtk_queue_node_t q_n;
    float *landmark;
    qtk_cv_bbox_t roi;
    float conf;
    uint8_t *face_patch;
    int face_H;
    int face_W;
} tracklet_hist_t;

typedef struct {
    int pre_id;
    float pre_theta;
    int switch_enter_trap;
    int candidate_id;
    qtk_avspeech_lip_t *lip;
} tracklet_central_selector_t;

typedef struct {
    qtk_avspeech_lip_t *lip;
    int *cur_slot;
    qtk_mot_sort_tracklet_t **candidate;
} tracklet_at_most_selecotr_t;

typedef struct {
    qtk_avspeech_lip_t *lip;
    int max_area_id;
    int pre_id;
    int candidate_id;
} tracklet_max_area_selector_t;

typedef struct {
    void *(*new_f)(qtk_avspeech_lip_t *l);
    void (*delete_f)(void *sel);
    int (*process_f)(void *sel, qtk_mot_sort_tracklet_t **candidate_tl,
                     int ntracklet, int H, int W);
} lip_selector_interface_t;

static void *empty_selector_new_f_(qtk_avspeech_lip_t *l) { return NULL; }
static void empty_selector_delete_f_(void *sel) {}
static int empty_selector_process_f_(void *sel,
                                     qtk_mot_sort_tracklet_t **candidate_tl,
                                     int ntracklet, int H, int W) {
    return ntracklet;
}
static void raise_tracklet_start_(qtk_avspeech_lip_t *lip, int id) {
    qtk_avspeech_lip_result_t result;
    result.id = id;
    result.frame_idx = lip->frame_idx;
    result.lip_image = NULL;
    result.state = QTK_AVSPEECH_LIP_START;

    lip->notifier(lip->upval, &result);

}

static void raise_tracklet_end_(qtk_avspeech_lip_t *lip, int id) {
    qtk_avspeech_lip_result_t result;
    result.id = id;
    result.frame_idx = lip->frame_idx;
    result.lip_image = NULL;
    result.state = QTK_AVSPEECH_LIP_END;
    lip->notifier(lip->upval, &result);
}

static void *tracklet_max_area_selector_new_(qtk_avspeech_lip_t *lip) {
    tracklet_max_area_selector_t *sel = wtk_malloc(sizeof(tracklet_max_area_selector_t));
    sel->lip = lip;
    sel->candidate_id = -1;
    sel->max_area_id = -1;
    sel->pre_id = -1;
    return sel;
}
static void tracklet_max_area_selector_delete_(void *sel) {wtk_free(sel);}

static int tracklet_max_area_selector_process_(void *sel_ptr,
                                               qtk_mot_sort_tracklet_t **tl_list,
                                               int ntracklet, int H, int W) {
    tracklet_max_area_selector_t *sel = sel_ptr;
    qtk_avspeech_lip_t *lip = sel->lip;
    int max_area = 0;
    int pre_id = sel->pre_id;
    int result_ntracklet = 0;
         qtk_mot_sort_tracklet_t *candidata_tl = NULL;
    for (int i = 0; i < ntracklet; i++) {
        qtk_mot_sort_tracklet_t *tl = tl_list[i];
        tracklet_hist_t *hist = tl->upval;
        float area = (hist->roi.x2 - hist->roi.x1) * (hist->roi.y2 - hist->roi.y1);
        if (area > max_area) {
            max_area = area;
            candidata_tl = tl;
        }
    } 
    if (candidata_tl != NULL) {
        tracklet_hist_t *hist = candidata_tl->upval;
    
        float deep = W / (2 * tan(lip->cfg->camera_angle * M_PI / 360));
        float central_w = (hist->roi.x1 + hist->roi.x2) / 2;
        // float theta = atan2(deep, W / 2.0 - central_w) * 180 / M_PI;
        float theta = atan2(deep, central_w - W / 2.0) * 180 / M_PI;
        float theta_dist = fabs(theta - 90);
        
        if(theta_dist < lip->cfg->theta_tolerance){
            if (sel->pre_id == candidata_tl->id) {
                result_ntracklet = 1;
                tl_list[0] = candidata_tl;
            } else {
                if (sel->candidate_id == candidata_tl->id) {
                        sel->pre_id = candidata_tl->id;
                        result_ntracklet = 1;
                        tl_list[0] = candidata_tl;
                } else {
                    sel->candidate_id = candidata_tl->id;
                }
            }
        }
    }
    if (result_ntracklet == 0) {
        if (pre_id >= 0) {
            raise_tracklet_end_(lip, pre_id);
            sel->pre_id = -1;
        }
    } else {
        if (pre_id != sel->pre_id) {
            if (pre_id >= 0) {
                raise_tracklet_end_(lip, pre_id);
            }
            raise_tracklet_start_(lip, sel->pre_id);
        }
    }
    return result_ntracklet;

    #if 0
    for (int i = 0; i < ntracklet; i++) {
        qtk_mot_sort_tracklet_t *tl = candidate_tl[i];
        tracklet_hist_t *hist = tl->upval;

        // if(hist->roi.x1 <= 0 || hist->roi.y1 <= 0 || hist->roi.x2 >= W || hist->roi.y2 >= H) {
        //     continue;
        // }
        
        int area = (hist->roi.x2 - hist->roi.x1) * (hist->roi.y2 - hist->roi.y1);
        // if(area < 100 || area > 1000000) {
        //     continue;
        // }
        
        if (area > max_area) {
            max_area = area;
            max_area_index = i;
        }
    }

    // 如果找到了最大面积的轨迹
    if (max_area_index >= 0) {
        qtk_mot_sort_tracklet_t *selected_tl = candidate_tl[max_area_index];
        candidate_tl[0] = selected_tl;
        sel->max_area_id = selected_tl->id;

        // 如果当前最大面积的轨迹与上一次不同，通知轨迹开始
        if (sel->max_area_id != sel->pre_id) {
            if (sel->pre_id != -1) {
                raise_tracklet_end_(lip, sel->pre_id);
            }
            raise_tracklet_start_(lip, sel->max_area_id);
            sel->pre_id = sel->max_area_id;
        }

        return 1;
    } else {
        // 如果没有找到任何轨迹，通知上一次的轨迹结束
        if (sel->pre_id != -1) {
            raise_tracklet_end_(lip, sel->pre_id);
            sel->pre_id = -1;
        }
        return 0;
    }

    #endif
}

static void *tracklet_at_most_selector_new_(qtk_avspeech_lip_t *lip) {
    int i;
    tracklet_at_most_selecotr_t *sel =
        wtk_malloc(sizeof(tracklet_at_most_selecotr_t));
    sel->lip = lip;
    sel->cur_slot = wtk_malloc(sizeof(int) * lip->cfg->policy_n);
    sel->candidate =
        wtk_malloc(sizeof(qtk_mot_sort_tracklet_t *) * lip->cfg->policy_n);
    for (i = 0; i < lip->cfg->policy_n; i++) {
        sel->cur_slot[i] = -1;
    }
    return sel;
}

static void tracklet_at_most_selector_delete_(void *sel_ptr) {
    tracklet_at_most_selecotr_t *sel = sel_ptr;
    wtk_free(sel->cur_slot);
    wtk_free(sel->candidate);
    wtk_free(sel);
}

static float tracklet_cmp_(void *args, void *arga, void *argb) {
    qtk_mot_sort_tracklet_t **a = arga;
    qtk_mot_sort_tracklet_t **b = argb;
    tracklet_hist_t *hista = a[0]->upval;
    tracklet_hist_t *histb = b[0]->upval;
    return hista->conf > histb->conf ? -1 : 1;
}

static int
tracklet_at_most_selector_process_(void *sel_ptr,
                                   qtk_mot_sort_tracklet_t **candidate_tl,
                                   int ntracklet, int H, int W) {
    int i, j;
    qtk_mot_sort_tracklet_t *candidate;
    int ntracklet_result = 0;
    tracklet_at_most_selecotr_t *sel = sel_ptr;
    int N = sel->lip->cfg->policy_n;
    for (i = 0; i < N; i++) {
        if (sel->cur_slot[i] < 0) {
            continue;
        }
        candidate = NULL;
        for (j = 0; j < ntracklet; j++) {
            if (candidate_tl[j] && candidate_tl[j]->id == sel->cur_slot[i]) {
                candidate = candidate_tl[j];
                candidate_tl[j] = NULL;
                break;
            }
        }
        if (candidate) {
            sel->candidate[ntracklet_result++] = candidate;
        } else {
            // stale tracklet
            raise_tracklet_end_(sel->lip, sel->cur_slot[i]);
            sel->cur_slot[i] = -1;
        }
    }
    if (ntracklet_result < N) {
        i = 0;
        for (j = 0; j < ntracklet; j++) {
            if (candidate_tl[j] != NULL) {
                candidate_tl[i++] = candidate_tl[j];
            }
        }
        if (i > 0) {
            qtk_mot_sort_tracklet_t *tmp_tl;
            wtk_qsort(candidate_tl, candidate_tl + i - 1,
                      sizeof(qtk_mot_sort_tracklet_t *), tracklet_cmp_, sel,
                      &tmp_tl);
        }
        j = min(N - ntracklet_result, i);
        memcpy(sel->candidate + ntracklet_result, candidate_tl,
               j * sizeof(qtk_mot_sort_tracklet_t *));
        ntracklet_result += j;
        // new added
        for (i = 0; i < j; i++) {
            raise_tracklet_start_(sel->lip, candidate_tl[i]->id);
        }
    }
    if (ntracklet_result > 0) {
        memcpy(candidate_tl, sel->candidate,
               sizeof(qtk_mot_sort_tracklet_t *) * ntracklet_result);
        for (i = 0; i < ntracklet_result; i++) {
            sel->cur_slot[i] = candidate_tl[i]->id;
        }
    }
    return ntracklet_result;
}

static void *tracklet_central_selector_new_(qtk_avspeech_lip_t *lip) {
    tracklet_central_selector_t *sel =
        wtk_malloc(sizeof(tracklet_central_selector_t));
    sel->pre_id = -1;
    sel->pre_theta = -1;
    sel->candidate_id = -1;
    sel->switch_enter_trap = 0;
    sel->lip = lip;
    return sel;
}

static void tracklet_central_selector_delete_(void *sel) { wtk_free(sel); }

#if  1
static int avspeech_lip_selector_central_(void *sel_ptr,
                                          qtk_mot_sort_tracklet_t **tl_list,
                                          int ntracklet, int H, int W) {
    int i;
    tracklet_central_selector_t *sel = sel_ptr;
    qtk_avspeech_lip_t *lip = sel->lip;
    int pre_id = sel->pre_id;
    float min_theta_dist = 360;
    qtk_mot_sort_tracklet_t *candidata_tl;
    int result_ntracklet = 0;
    for (i = 0; i < ntracklet; i++) {
        qtk_mot_sort_tracklet_t *tl = tl_list[i];
        tracklet_hist_t *hist = tl->upval;
        float deep = W / (2 * tan(lip->cfg->camera_angle * M_PI / 360));
        float central_w = (hist->roi.x1 + hist->roi.x2) / 2;
        float theta = atan2(deep, W / 2.0 - central_w) * 180 / M_PI;
        float theta_dist = fabs(theta - 90);
        if (theta_dist < min_theta_dist) {
            min_theta_dist = theta_dist;
            candidata_tl = tl;
    
        }
        if (tl->id == sel->pre_id) {
            sel->pre_theta = theta;
        }
    }

    if (min_theta_dist < lip->cfg->theta_tolerance) {
        if (sel->pre_id == candidata_tl->id) {
            result_ntracklet = 1;
            tl_list[0] = candidata_tl;
            
        } else {
            if (sel->candidate_id == candidata_tl->id) {
                if (++sel->switch_enter_trap > lip->cfg->switch_enter_trap) {
                    sel->pre_id = candidata_tl->id;
                    result_ntracklet = 1;
                    tl_list[0] = candidata_tl;
        
                }
            } else {
                sel->candidate_id = candidata_tl->id;
                sel->switch_enter_trap = 0;
            }
        }
    }
    if (result_ntracklet == 0) {
        if (pre_id >= 0) {
            raise_tracklet_end_(lip, pre_id);
            sel->pre_id = -1;
        }
    } else {
        if (pre_id != sel->pre_id) {
            if (pre_id >= 0) {
                raise_tracklet_end_(lip, pre_id);
            }
            raise_tracklet_start_(lip, sel->pre_id);
        }
    }
    return result_ntracklet;
}
#endif
static lip_selector_interface_t selectors[] = {
    {empty_selector_new_f_, empty_selector_delete_f_,
     empty_selector_process_f_},
    {tracklet_central_selector_new_, tracklet_central_selector_delete_,
     avspeech_lip_selector_central_},
    {tracklet_at_most_selector_new_, tracklet_at_most_selector_delete_,
     tracklet_at_most_selector_process_},
    {tracklet_max_area_selector_new_, tracklet_max_area_selector_delete_, 
     tracklet_max_area_selector_process_}
     };

tracklet_hist_t *tracklet_hist_new(qtk_avspeech_lip_t *lip) {
    tracklet_hist_t *hist = wtk_malloc(
        sizeof(tracklet_hist_t) + sizeof(float[2]) * lip->cfg->num_landmarks);
    hist->landmark = (float *)(hist + 1);
    hist->face_patch = NULL;
    return hist;
}

int tracklet_hist_delete(tracklet_hist_t *hist) {
    if (hist->face_patch) {
        wtk_free(hist->face_patch);
    }
    wtk_free(hist);
    return 0;
}

static int on_sort_(qtk_avspeech_lip_t *lip, qtk_mot_sort_tracklet_t *kt,
                    int add) {
    qtk_avspeech_lip_result_t result;
    result.id = kt->id;
    result.frame_idx = lip->frame_idx;
    result.lip_image = NULL;
    if (add) {
        tracklet_hist_t *hist = wtk_hoard_pop(&lip->tracklet_hist);
	    if (hist->face_patch) {
            wtk_free(hist->face_patch);
            hist->face_patch = NULL;
        }
        memcpy(hist->landmark, lip->cur_faces[kt->det_id].keypoints,
               sizeof(float[2]) * lip->cfg->num_landmarks);
        kt->upval = hist;
        hist->conf = FLT_MIN;
        result.state = QTK_AVSPEECH_LIP_START;
    } else {
        wtk_hoard_push(&lip->tracklet_hist, kt->upval);
        kt->upval = NULL;
        result.state = QTK_AVSPEECH_LIP_END;
    }
    if (lip->cfg->selector_policy == QTK_AVSPEECH_LIP_SELECTOR_POLICY_ALL) {
        lip->notifier(lip->upval, &result);
    }
    return 0;
}

qtk_avspeech_lip_t *qtk_avspeech_lip_new(qtk_avspeech_lip_cfg_t *cfg,
                                         void *upval,
                                         qtk_avspeech_lip_notifier_t notifier) {
    qtk_avspeech_lip_t *lip = wtk_malloc(sizeof(qtk_avspeech_lip_t));
    lip->cfg = cfg;
    lip->face_det = qtk_cv_detection_new(&cfg->face_det);
    lip->notifier = notifier;
    lip->upval = upval;
    lip->heap = wtk_heap_new(4096);
    lip->lip_patch = wtk_malloc(sizeof(uint8_t) * cfg->H * cfg->W);
    lip->detect_patch = wtk_malloc(sizeof(uint8_t) * cfg->face_det.width *
                                   cfg->face_det.height * 3);
    lip->frame_idx = 0;
    if (cfg->use_landmark) {
        lip->landmark = qtk_nnrt_new(&cfg->landmark);
        lip->landmark_patch =
            wtk_malloc(sizeof(uint8_t) * 192 * 192 * 3); // FIXME
    } else {
        lip->landmark_patch = NULL;
        lip->landmark = NULL;
    }
    qtk_mot_sort_init(&lip->mot, &cfg->mot);
    wtk_hoard_init(&lip->tracklet_hist, offsetof(tracklet_hist_t, q_n), 10,
                   (wtk_new_handler_t)tracklet_hist_new,
                   (wtk_delete_handler_t)tracklet_hist_delete, lip);
    qtk_mot_sort_set_notifier(&lip->mot, lip,
                              (qtk_mot_sort_notifier_t)on_sort_);
    lip->selector = selectors[cfg->selector_policy].new_f(lip);
    return lip;
}

void qtk_avspeech_lip_delete(qtk_avspeech_lip_t *lip) {
    qtk_cv_detection_delete(lip->face_det);
    qtk_mot_sort_clean(&lip->mot);
    if (lip->landmark) {
        qtk_nnrt_delete(lip->landmark);
    }
    if (lip->landmark_patch) {
        wtk_free(lip->landmark_patch);
    }
    wtk_heap_delete(lip->heap);
    wtk_hoard_clean(&lip->tracklet_hist);
    wtk_free(lip->lip_patch);
    wtk_free(lip->detect_patch);
    selectors[lip->cfg->selector_policy].delete_f(lip->selector);
    wtk_free(lip);
}

static void process_single_(qtk_avspeech_lip_t *lip, uint8_t *image, int H,
                            int W, float *landmark,
                            qtk_mot_sort_tracklet_t *tl) {
    qtk_avspeech_lip_result_t result;
    qtk_lip_cropper_process(image, lip->lip_patch, H, W, landmark, lip->cfg->border);
    result.frame_idx = lip->frame_idx;
    result.id = tl->id;
    result.lip_image = lip->lip_patch;
    result.state = QTK_AVSPEECH_LIP_DATA;
    {
        float x_ratio = (float)W / lip->face_det->cfg->width;
        float y_ratio = (float)H / lip->face_det->cfg->height;
        result.roi = tl->predict;

        result.roi.x1 *= x_ratio;
        result.roi.x2 *= x_ratio;
        result.roi.y1 *= y_ratio;
        result.roi.y2 *= y_ratio;
    }
    {
        tracklet_hist_t *hist = tl->upval;
        if (!hist->face_patch) {
            qtk_image_desc_t desc;
            qtk_cv_bbox_t roi;
            qtk_image_roi_t iroi;
            qtk_cv_bbox_fix_ratio(&tl->predict, &roi, 1.0,
                                  lip->face_det->cfg->width,
                                  lip->face_det->cfg->height);
            if (roi.x2 <= roi.x1 || roi.y2 <= roi.y1) {
                return; 
            }                      
            desc.fmt = QBL_IMAGE_RGB24;
            desc.channel = 3;
            desc.height = lip->face_det->cfg->height;
            desc.width = lip->face_det->cfg->width;

            iroi.x = roi.x1;
            iroi.y = roi.y1;
            iroi.width = roi.x2 - roi.x1;
            iroi.height = roi.y2 - roi.y1;

            hist->face_patch =
                wtk_malloc(sizeof(uint8_t) * iroi.width * iroi.height * 3);
            hist->face_H = iroi.height;
            hist->face_W = iroi.width;

            if(qtk_image_sub_u8(&desc, &iroi, hist->face_patch, lip->detect_patch) != 0){
                wtk_debug("qtk_image_sub_u8 failed\n");
                return;
            }
        }

        result.face_patch = hist->face_patch;
        result.face_H = hist->face_H;
        result.face_W = hist->face_W;
    }

    lip->notifier(lip->upval, &result);
}

static void update_landmark_via_nnrt_(qtk_avspeech_lip_t *lip,
                                      tracklet_hist_t *hist, uint8_t *image,
                                      int H, int W) {
    int i;
    qtk_nnrt_value_t lm_input, lm_output;
    float *lm_input_data, *lm_output_data;
    float x_ratio, y_ratio;
    int64_t shape[4] = {1, 3, 192, 192};
    uint8_t *crop;
    int N = 192 * 192;
    qtk_image_roi_t roi = {
        .x = hist->roi.x1,
        .y = hist->roi.y1,
        .width = hist->roi.x2 - hist->roi.x1,
        .height = hist->roi.y2 - hist->roi.y1,
    };
    qtk_image_desc_t desc = {
        .fmt = QBL_IMAGE_RGB24, .channel = 3, .width = W, .height = H};
    crop = wtk_heap_malloc(lip->heap,
                           sizeof(uint8_t) * roi.width * roi.height * 3);
    qtk_image_sub_u8(&desc, &roi, crop, image);
    desc.height = roi.height;
    desc.width = roi.width;
    qtk_image_resize(&desc, crop, 192, 192, lip->landmark_patch);
    lm_input =
        qtk_nnrt_value_create(lip->landmark, QTK_NNRT_VALUE_ELEM_F32, shape, 4);
    lm_input_data = qtk_nnrt_value_get_data(lip->landmark, lm_input);
    for (i = 0; i < N; i++) {
        lm_input_data[i] = lip->landmark_patch[i * 3 + 0];
        lm_input_data[i + N] = lip->landmark_patch[i * 3 + 1];
        lm_input_data[i + N * 2] = lip->landmark_patch[i * 3 + 2];
    }
    qtk_nnrt_feed(lip->landmark, lm_input, 0);
    qtk_nnrt_run(lip->landmark);
    qtk_nnrt_get_output(lip->landmark, &lm_output, 0);
    lm_output_data = qtk_nnrt_value_get_data(lip->landmark, lm_output);
    memcpy(hist->landmark, lm_output_data + 38 * 2, sizeof(float[2]));
    memcpy(hist->landmark + 2, lm_output_data + 88 * 2, sizeof(float[2]));
    memcpy(hist->landmark + 4, lm_output_data + 86 * 2, sizeof(float[2]));
    memcpy(hist->landmark + 6, lm_output_data + 52 * 2, sizeof(float[2]));
    memcpy(hist->landmark + 8, lm_output_data + 61 * 2, sizeof(float[2]));
    x_ratio = roi.width / 192.0;
    y_ratio = roi.height / 192.0;
    for (i = 0; i < lip->cfg->num_landmarks; i++) {
        float x = hist->landmark[i * 2 + 0];
        float y = hist->landmark[i * 2 + 1];
        x = (x + 1) * (192.0 / 2);
        y = (y + 1) * (192.0 / 2);
        hist->landmark[i * 2 + 0] = x * x_ratio + roi.x;
        hist->landmark[i * 2 + 1] = y * y_ratio + roi.y;
    }
    qtk_nnrt_value_release(lip->landmark, lm_input);
    qtk_nnrt_value_release(lip->landmark, lm_output);
}

static void update_tl_roi_(qtk_avspeech_lip_t *lip, tracklet_hist_t *hist,
                           int id, int H, int W) {
    // TODO
    qtk_cv_bbox_t roi = lip->cur_faces[id].box;
    qtk_cv_bbox_t fix_roi;
    float ratio_x = (float)W / lip->cfg->face_det.width;
    float ratio_y = (float)H / lip->cfg->face_det.height;
    roi.x1 *= ratio_x;
    roi.x2 *= ratio_x;
    roi.y1 *= ratio_y;
    roi.y2 *= ratio_y;

    qtk_cv_bbox_fix_ratio(&roi, &fix_roi, 1.0, W, H);
    qtk_cv_bbox_clip2int(&fix_roi, &hist->roi, W, H);
}

static void update_landmark_(qtk_avspeech_lip_t *lip,
                             qtk_mot_sort_tracklet_t *tl, uint8_t *image, int H,
                             int W) {
    tracklet_hist_t *hist;
    float ratio_x = (float)W / lip->cfg->face_det.width;
    float ratio_y = (float)H / lip->cfg->face_det.height;
    int i;
    hist = tl->upval;
    if (lip->cfg->use_landmark) {
        update_landmark_via_nnrt_(lip, hist, image, H, W);
    } else {
        if (lip->cur_faces && tl->det_id >= 0) { // detected frame
            for (i = 0; i < lip->cfg->num_landmarks; i++) {
                hist->landmark[i * 2 + 0] =
                    lip->cur_faces[tl->det_id].keypoints[i * 2 + 0] * ratio_x;
                hist->landmark[i * 2 + 1] =
                    lip->cur_faces[tl->det_id].keypoints[i * 2 + 1] * ratio_y;
            }
        }
    }
}

static void update_tracklet_(qtk_avspeech_lip_t *lip, uint8_t *image, int H,
                             int W) {
    wtk_queue_node_t *node;
    int ntracklet = 0;
    int i;
    qtk_mot_sort_tracklet_t **candidate_tl =
        wtk_heap_malloc(lip->heap, sizeof(qtk_mot_sort_tracklet_t *) *
                                       lip->mot.trackers.length);
    for (node = lip->mot.trackers.pop; node; node = node->next) {
        tracklet_hist_t *hist;
        qtk_mot_sort_tracklet_t *tl =
            data_offset2(node, qtk_mot_sort_tracklet_t, q_n);
        if (!tl->ongoing) {
            continue;
        }
        hist = tl->upval;
        if (lip->cur_faces && tl->det_id >= 0) { // detected frame
            update_tl_roi_(lip, hist, tl->det_id, H, W);
            if (hist->conf < 0) {
                hist->conf = lip->cur_faces[tl->det_id].conf;
            } else {
                hist->conf =
                    hist->conf * 0.8 + 0.2 * lip->cur_faces[tl->det_id].conf;
            }
        } // else use old roi
        candidate_tl[ntracklet++] = tl;
    }
    ntracklet = selectors[lip->cfg->selector_policy].process_f(
        lip->selector, candidate_tl, ntracklet, H, W);
    for (i = 0; i < ntracklet; i++) {
        qtk_mot_sort_tracklet_t *tl = candidate_tl[i];
        tracklet_hist_t *hist = tl->upval;

        update_landmark_(lip, tl, image, H, W);
        // double start_time = time_get_ms();
        process_single_(lip, image, H, W, hist->landmark, tl);
        // double end_time = time_get_ms() - start_time;
        // printf("process_single_ time: %f\n", end_time);
    }
    if (ntracklet == 0) {
        qtk_avspeech_lip_result_t result;
        result.frame_idx = lip->frame_idx;
        result.state = QTK_AVSPEECH_LIP_EMPTY;
        lip->notifier(lip->upval, &result);
    }
}

int qtk_avspeech_lip_feed(qtk_avspeech_lip_t *lip, uint8_t *image, int H,
                          int W) {
    int i;
    int nface;
    qtk_cv_bbox_t *dets;
    uint8_t *detect_patch = image;
    wtk_heap_reset(lip->heap);
    lip->cur_faces = NULL;
    if (lip->frame_idx % lip->cfg->nskip != 0) {
        update_tracklet_(lip, image, H, W);
        lip->frame_idx++;
        return 0;
    }
    if (H != lip->cfg->face_det.height || W != lip->cfg->face_det.width) {
        qtk_image_desc_t desc;
        detect_patch = lip->detect_patch;
        desc.fmt = QBL_IMAGE_RGB24;
        desc.width = W;
        desc.height = H;
        desc.channel = 3;
        qtk_image_resize(&desc, image, lip->cfg->face_det.height,
                         lip->cfg->face_det.width, detect_patch);
    }
    nface =
        qtk_cv_detection_detect(lip->face_det, detect_patch, &lip->cur_faces);
    dets = wtk_heap_malloc(lip->heap, sizeof(qtk_cv_bbox_t) * nface);
    for (i = 0; i < nface; i++) {
        dets[i] = lip->cur_faces[i].box;
    }
    qtk_mot_sort_update(&lip->mot, nface, dets);
    update_tracklet_(lip, image, H, W);
    lip->frame_idx++;
    return 0;
}

int qtk_avspeech_lip_reset(qtk_avspeech_lip_t *lip) {
    qtk_mot_sort_clean(&lip->mot);
    lip->frame_idx = 0;
    wtk_hoard_clean(&lip->tracklet_hist);
    wtk_hoard_init(&lip->tracklet_hist, offsetof(tracklet_hist_t, q_n), 10,
                   (wtk_new_handler_t)tracklet_hist_new,
                   (wtk_delete_handler_t)tracklet_hist_delete, lip);
    qtk_mot_sort_init(&lip->mot, &lip->cfg->mot);
    qtk_mot_sort_set_notifier(&lip->mot, lip,
                              (qtk_mot_sort_notifier_t)on_sort_);
    selectors[lip->cfg->selector_policy].delete_f(lip->selector);
    lip->selector = selectors[lip->cfg->selector_policy].new_f(lip);
    return 0;
}
