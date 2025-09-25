
#include <string>
#include <iostream>

extern "C" {
#include "qtk/cv/stand_sit/qtk_cv_stand_sit.h"
#include "qtk/cv/detection/qtk_cv_detection_onnx.h"
}

using namespace std;
// using namespace cv;

static void _copyMakeBorder(unsigned char *src, unsigned char *dst, int src_height, int src_width, int src_channel, int top, int bottom, int left, int right, unsigned char border_value[3]) {
   int dst_height = src_height + top + bottom;
   int dst_width = src_width + left + right;

   for (int c = 0; c < src_channel; ++c) {
      for (int i = 0; i < dst_height; ++i) {
            for (int j = 0; j < dst_width; ++j) {
               int di = i * dst_width * src_channel + j * src_channel + c;
               if (i < top || i >= top + src_height || j < left || j >= left + src_width) {
                  dst[di] = border_value[c];
               } else {
                  int si = (i - top) * src_width * src_channel + (j - left) * src_channel + c;
                  dst[di] = src[si];
               }
            }
      }
   }
}

static void _letterbox(qtk_cv_stand_sit_t *st ,uint8_t *src_data, uint8_t* dest_data,int src_width, int src_height, int input_width, int input_height){
   
   double r = min((double)input_height/src_height, (double)input_width/src_width);
   int new_unpad_w = (int)(round(src_width * r));
   int new_unpad_h = (int)(round(src_height * r));
   float dw = input_width - new_unpad_w;
   float dh = input_height - new_unpad_h;
   dw /= 2;
   dh /= 2;
   st->person_detection->rat[0] = dw;
   st->person_detection->rat[1] = dh;

   uint8_t *tmp = (uint8_t *)malloc(new_unpad_w*new_unpad_h*3);

   qtk_image_desc_t src_desc;
   src_desc.fmt = QBL_IMAGE_RGB24;
   src_desc.height = src_height;
   src_desc.width = src_width;
   src_desc.channel = 3;
   qtk_image_resize(&src_desc, src_data, new_unpad_h, new_unpad_w, tmp);

   int top = int(round(dh - 0.1));
   int bottom = int(round(dh + 0.1));
   int left = int(round(dw - 0.1));
   int right = int(round(dw + 0.1));

   unsigned char border_value[3] = {128, 128, 128};
   _copyMakeBorder(tmp, dest_data, new_unpad_h, new_unpad_w, 3, top, bottom, left, right, border_value);

   free(tmp);

}

#if 0
static void letterbox(qtk_cv_stand_sit_t *st ,Mat &RGB_data, Mat &desc_data,int src_width, int src_height, int input_width, int input_height){
   double r = min((double)input_height/src_height, (double)input_width/src_width);
   int new_unpad_w = (int)(round(src_width * r));
   int new_unpad_h = (int)(round(src_height * r));
   float dw = input_width - new_unpad_w;
   float dh = input_height - new_unpad_h;

   dw /= 2;
   dh /= 2;
   st->person_detection->rat[0] = dw;
   st->person_detection->rat[1] = dh;
   Mat resize_data ;
   resize(RGB_data, resize_data, Size(new_unpad_w, new_unpad_h), 0, 0, INTER_AREA);

   int top = int(round(dh - 0.1));
   int bottom = int(round(dh + 0.1));
   int left = int(round(dw - 0.1));
   int right = int(round(dw + 0.1));
   copyMakeBorder(resize_data, desc_data, top, bottom, left, right, BORDER_CONSTANT, Scalar(128, 128, 128));

   resize_data.release();
}
#endif
qtk_cv_stand_sit_t *qtk_cv_stand_sit_new(qtk_cv_stand_sit_cfg_t *cfg){
   qtk_cv_stand_sit_t *st = (qtk_cv_stand_sit_t *)wtk_malloc(sizeof(qtk_cv_stand_sit_t));
   st->cfg = cfg;
   st->person_detection = qtk_cv_detection_new(&cfg->person_detection);
   st->classify = qtk_cv_classify_new(&cfg->classify);
   st->result = (wtk_string_t *)wtk_malloc(sizeof(*st->result));
   st->result->data = (char *)wtk_malloc(32);
   st->result->len = 0;
   return st;

}

void qtk_cv_stand_sit_delete(qtk_cv_stand_sit_t *st){
   qtk_cv_detection_delete(st->person_detection);
   qtk_cv_classify_delete(st->classify);
   wtk_free(st->result->data);
   wtk_free(st->result);
   wtk_free(st);
}

qtk_cv_bbox_t convert_to_square(int xmin, int ymin, int xmax, int ymax,int width, int height){
   qtk_cv_bbox_t bbox;
   int center_x = (xmin + xmax) / 2;
   int center_y = (ymin + ymax) / 2;
   int square_length = max((xmax - xmin),(ymax - ymin));
   square_length *= 1.1;
   xmin = max(0,int(center_x - square_length/2));
   ymin = max(0,int(center_y - square_length/2));
   xmax = min(width,int(center_x + square_length/2));
   ymax = min(height,int(center_y + square_length/2));

   bbox.x1 = xmin;
   bbox.y1 = ymin;
   bbox.x2 = xmax;
   bbox.y2 = ymax;
   return bbox;
}


int qtk_cv_stand_sit_process(qtk_cv_stand_sit_t *stand_sit, uint8_t * image, int height, int width){
   stand_sit->person_detection->src_imgheight = height;
   stand_sit->person_detection->src_imgwidth = width;
   qtk_image_desc_t desc;
   desc.width = width;
   desc.height = height;
   desc.fmt = QBL_IMAGE_RGB24;
   desc.channel=3; 

   float x_factor = (float)stand_sit->cfg->person_detection.width / width ;
   float y_factor = (float)stand_sit->cfg->person_detection.height / height;
   float r = min(x_factor, y_factor);
   stand_sit->person_detection->x_factor = r;
   stand_sit->person_detection->y_factor = r;
   
   uint8_t* detect_image = (uint8_t*)malloc(stand_sit->cfg->person_detection.width * stand_sit->cfg->person_detection.height * 3);
   _letterbox(stand_sit, image, detect_image,  width, height, stand_sit->cfg->person_detection.width,  stand_sit->cfg->person_detection.height);

   int ret = qtk_cv_detection_detect(stand_sit->person_detection, detect_image, &(stand_sit->person));
   
   if(detect_image){free(detect_image); detect_image = nullptr;}

   if(stand_sit->cfg->use_phone_cls){
      wtk_debug("======== detected ret  = %d ============\n",ret);
      if(ret > 0){
         for(int i = 0; i < ret; i++){
            qtk_cv_bbox_t roi = convert_to_square(((stand_sit->person)[i]).box.x1, ((stand_sit->person)[i]).box.y1, ((stand_sit->person)[i]).box.x2, ((stand_sit->person)[i]).box.y2, width, height);
            uint8_t *roi_data = (uint8_t *)malloc((roi.x2 - roi.x1) * (roi.y2 - roi.y1) * 3);
            
            qtk_cv_detection_onnx_image_rect(image, &desc, roi_data, &roi);
            qtk_image_desc_t roi_desc;
            roi_desc.channel = 3;
            roi_desc.fmt = QBL_IMAGE_RGB24;
            roi_desc.width = roi.x2 - roi.x1;
            roi_desc.height = roi.y2 - roi.y1;
      
            uint8_t *cls_data = (uint8_t *)malloc(3 * stand_sit->classify->cfg->width * stand_sit->classify->cfg->height);
            qtk_image_resize(&roi_desc, roi_data, stand_sit->classify->cfg->height, stand_sit->classify->cfg->width, cls_data);

            if(roi_data){
               free(roi_data); roi_data = nullptr; 
            }
            ret = qtk_cv_classify_feed(stand_sit->classify, cls_data);

            wtk_string_t *cls_result = qtk_cv_classify_get_label(stand_sit->classify, ret);
            memcpy(stand_sit->result->data, cls_result->data, cls_result->len);
            stand_sit->result->len = cls_result->len;
            if(cls_data){free(cls_data); cls_data = nullptr;}

         }
      }

   }else{
      // person stand or sit
      if(ret == 1){
         wtk_debug("Detected 1 person\n");

            float expand_rate = 0.2;
            float w = stand_sit->person->box.x2 - stand_sit->person->box.x1;
            float h = stand_sit->person->box.y2 - stand_sit->person->box.y1;
            int expand_w = (int)(expand_rate * w / 2);
            int expand_h = (int)(expand_rate * h / 2);

            int new_x1 = max(stand_sit->person->box.x1 - expand_w, 0.0f);
            int new_y1 = max(stand_sit->person->box.y1 - expand_h, 0.0f);
            int new_x2 = min(stand_sit->person->box.x2 + expand_w, (float)width);
            int new_y2 = min(stand_sit->person->box.y2 + expand_h, (float)height);

            qtk_cv_bbox_t roi_box;
            roi_box.x1=new_x1;roi_box.y1=new_y1;roi_box.x2=new_x2;roi_box.y2=new_y2;
            uint8_t *cls_data = (uint8_t *)malloc(3 * stand_sit->classify->cfg->width * stand_sit->classify->cfg->height);
            uint8_t *roi_data = (uint8_t *)malloc((new_x2-new_x1)*(new_y2-new_y1)*3);
            qtk_cv_detection_onnx_image_rect(image, &desc, roi_data, &roi_box);
            _letterbox(stand_sit, roi_data, cls_data, new_x2 - new_x1, new_y2 - new_y1, stand_sit->cfg->classify.width,  stand_sit->cfg->classify.height);

            if(roi_data){
               free(roi_data); roi_data = nullptr; 
            }
            ret = qtk_cv_classify_feed(stand_sit->classify, cls_data);

            wtk_string_t *cls_result = qtk_cv_classify_get_label(stand_sit->classify, ret);
            memcpy(stand_sit->result->data, cls_result->data, cls_result->len);
            stand_sit->result->len = cls_result->len;
            if(cls_data){free(cls_data); cls_data = nullptr;}
      }else if(ret > 1){
         char tmpp[32] = {0};
         snprintf(tmpp, sizeof(tmpp),"%d", ret);
         memset(stand_sit->result->data, 0, 32);
         memcpy(stand_sit->result->data, tmpp, strlen(tmpp));
         stand_sit->result->len = strlen(tmpp);
         wtk_debug("Detected %s people\n", tmpp);

      }else{
         wtk_debug(" No person detected . have %d person\n", ret);
      }
   }

   return ret;
}