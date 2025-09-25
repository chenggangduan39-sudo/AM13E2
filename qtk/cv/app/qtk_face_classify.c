#include "qtk/cv/app/qtk_face_classify.h"
#include "qtk/image/qtk_image.h"

qtk_face_classify_t *qtk_face_classify_new(qtk_face_classify_cfg_t *cfg) {
    qtk_face_classify_t *cls = wtk_malloc(sizeof(qtk_face_classify_t));
    cls->cfg = cfg;
    cls->face_detection = qtk_cv_detection_new(&cfg->face_detection);
    cls->classify = qtk_cv_classify_new(&cfg->classify);
    cls->heap = wtk_heap_new(1024);
    cls->cls_img = wtk_malloc(sizeof(uint8_t) * cls->classify->H * cls->classify->W * 3);
    return cls;
}

void qtk_face_classify_delete(qtk_face_classify_t *cls) {
    qtk_cv_detection_delete(cls->face_detection);
    qtk_cv_classify_delete(cls->classify);
    wtk_heap_delete(cls->heap);
    wtk_free(cls->cls_img);
    wtk_free(cls);
}

int qtk_face_classify_feed(qtk_face_classify_t *cls, uint8_t *img, qtk_face_classify_result_t **res) {
    int i, ndet;
    qtk_cv_detection_result_t *det_res;
    qtk_image_desc_t desc;
    wtk_heap_reset(cls->heap);
    ndet = qtk_cv_detection_detect(cls->face_detection, img, &det_res);
    desc.fmt = QBL_IMAGE_RGB24;
    desc.channel = 3;
    desc.width = cls->face_detection->cfg->width;
    desc.height = cls->face_detection->cfg->height;
    if (ndet == 0) {
        return 0;
    }
    *res = wtk_heap_malloc(cls->heap, sizeof(qtk_face_classify_result_t) * ndet);
    for (i = 0; i < ndet; i++) {
        uint8_t *crop_img;
        qtk_cv_bbox_t fix_bbox;
        qtk_image_desc_t crop_desc = desc;
        qtk_image_roi_t roi;
        qtk_cv_bbox_fix_ratio(&det_res[i].box, &fix_bbox, (float)cls->classify->W / cls->classify->H, desc.width, desc.height);
        roi.x = fix_bbox.x1;
        roi.y = fix_bbox.y1;
        roi.width = fix_bbox.x2 - fix_bbox.x1;
        roi.height = fix_bbox.y2 - fix_bbox.y1;
        wtk_debug("%d %d %d %d %f %f %f %f\n", roi.x, roi.y, roi.width, roi.height, det_res[i].box.x1, det_res[i].box.y1, det_res[i].box.x2, det_res[i].box.y2);
        crop_img = wtk_heap_malloc(cls->heap, roi.width * roi.height * 3 * sizeof(uint8_t));
        qtk_image_sub_u8(&desc, &roi, crop_img, img);
        crop_desc.width = roi.width;
        crop_desc.height = roi.height;
        qtk_image_resize(&crop_desc, crop_img, cls->classify->H, cls->classify->W, cls->cls_img);
        (*res)[i].labels_id = qtk_cv_classify_feed(cls->classify, cls->cls_img);
        (*res)[i].det = det_res + i;
    }
    return ndet;
}

wtk_string_t *qtk_face_classify_get_label(qtk_face_classify_t *cls, int id) {
    return qtk_cv_classify_get_label(cls->classify, id);
}
