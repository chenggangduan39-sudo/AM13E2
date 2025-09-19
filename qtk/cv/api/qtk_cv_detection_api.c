#include <stdint.h>
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "qtk_cv_detection_api.h"
#include "qtk/cv/detection/qtk_cv_detection_head.h"
//#include "qtk/cv/detection/qtk_cv_detection_body.h"
#include "qtk/cv/detection/qtk_cv_detection_face.h"
//#include "qtk/cv/detection/qtk_cv_detection_hand.h"
//#include "qtk/cv/detection/qtk_cv_detection_stand.h"
//#include "qtk/cv/detection/qtk_cv_detection_dynamic.h"
#include "wtk/asr/fextra/onnxruntime/qtk_onnxruntime.h"
#include "wtk/asr/fextra/onnxruntime/qtk_onnxruntime_cfg.h"
#include "qtk/cv/tracking/qtk_mot_sort.h"

#ifdef USE_CNTLIMIT
#define MAX_COUNT_LIMIT 30*60*60*10
static long qtk_count=0;
#endif

/*detection hander*/
typedef struct qtk_cv_detection_api {
	void *h;
	wtk_main_cfg_t *main_cfg;
	wtk_mbin_cfg_t *mbin_cfg;
	qtk_onnxruntime_cfg_t *onnx_cfg;
	void *hand;
	qtk_image_desc_t img_desc;
	qtk_cv_detection_api_box_t box[1000];
	qtk_cv_detection_api_type_t type;

    qtk_mot_sort_cfg_t sort_cfg;
    qtk_mot_sort_t sort;
    wtk_strbuf_t *buf;

} qtk_cv_detection_api_t;

extern void qtk_cv_detection_onnxruntime_invoke_single(
    float *input, float **output, uint8_t *data, float *svd, float *std,
    int64_t *shape, qtk_cv_detection_onnxruntime_t *invo, const int size);

void resize_process(uint8_t *src,qtk_image_desc_t *src_desc,uint8_t *dst,qtk_image_desc_t *dst_desc){
    src_desc->fmt=QBL_IMAGE_RGB24;
    qtk_image_resize(src_desc,src,dst_desc->height,dst_desc->width,dst);
}

void *qtk_cv_detection_api_new(char* cfn, qtk_cv_detection_api_type_t type)
{
	qtk_cv_detection_api_t* e;

	qtk_cv_detection_head_t *head=NULL;
//    qtk_cv_detection_body_t *body=NULL;
    qtk_cv_detection_face_t *face=NULL;
//    qtk_cv_detection_hand_t *hand=NULL;
//    qtk_cv_detection_hand_gesture_t *ges=NULL;
    int use_bin=1;


    e = wtk_calloc(1, sizeof(qtk_cv_detection_api_t));
    e->h = NULL;

    if (use_bin)
    {
    	e->mbin_cfg = wtk_mbin_cfg_new_type(qtk_onnxruntime_cfg, cfn,
                                         (char *)"./main.cfg");
    	if (e->mbin_cfg  && e->mbin_cfg->cfg)
    	{
        	e->onnx_cfg = e->mbin_cfg->cfg;
        	e->onnx_cfg->rb = e->mbin_cfg->rbin;  //because qtk_onnxruntime_cfg interface don't match standard format.
    	}
    }else
    {
        e->main_cfg = wtk_main_cfg_new_type(qtk_onnxruntime_cfg, cfn);
        if (e->main_cfg)
        {
        	e->onnx_cfg = e->main_cfg->cfg;
        }
    }

    //init var
    e->img_desc.channel=0;
    e->img_desc.height=0;
    e->img_desc.width=0;

	switch (type){
	case QTK_CV_DETECTION_HEAD:
		head = qtk_cv_detection_head_new();
	    head->iou = 0.45;
	    head->conf = 0.3;
//	    head->invo = qtk_cv_detection_onnxruntime_new(model);
	    head->invo = qtk_cv_detection_onnxruntime_new2(e->onnx_cfg);
	    head->img_desc.width = 960;
	    head->img_desc.height = 540;

		e->h = (void*) head;
		e->type = QTK_CV_DETECTION_HEAD;
		break;
//	case QTK_CV_DETECTION_BODY:
//		body = qtk_cv_detection_body_new();
//	    body->iou=0.45;
//	    body->conf=0.2;
//	    body->invo=qtk_cv_detection_onnxruntime_new(model);
//	    e->h = (void*) body;
//	    e->type = QTK_CV_DETECTION_BODY;
//		break;
	case QTK_CV_DETECTION_FACE:
		face = qtk_cv_detection_face_new();
		face->iou = 0.45;
		face->conf = 0.3;
//		face->invo=qtk_cv_detection_onnxruntime_new(model);
		face->invo=qtk_cv_detection_onnxruntime_new2(e->onnx_cfg);
		face->img_desc.width = 960;
		face->img_desc.height = 540;
	    e->h = (void*) face;
	    e->type = QTK_CV_DETECTION_FACE;
		break;
//	case QTK_CV_DETECTION_HAND:
//		hand = qtk_cv_detection_hand_new();
//	    hand->iou=0.4;
//	    hand->conf=0.3;
//	    hand->invo=qtk_cv_detection_onnxruntime_new(model);
//	    e->h = (void*) hand;
//	    e->type = QTK_CV_DETECTION_HAND;
//		break;
//	case QTK_CV_DETECTION_GES:
//		ges = qtk_cv_detection_hand_gesture_new();
//		ges->invo=qtk_cv_detection_onnxruntime_new(model);
//		e->h = (void*) ges;
//		e->type = QTK_CV_DETECTION_GES;
//		break;
	default:
		wtk_debug("[Error]: nosupport or unknown type[%d]\n", type);
	}

    qtk_mot_sort_cfg_init(&e->sort_cfg);
    qtk_mot_sort_init(&e->sort, &e->sort_cfg);
    e->buf = wtk_strbuf_new(1024, 1);

	if (NULL==e->h)
	{
		wtk_debug("[Error]: don't build engine\n");
		qtk_cv_detection_api_delete(e);
		free(e);
		e = NULL;
	}

	return (void*)e;
}

void *qtk_cv_detection_api_newb(char* model_arr, qtk_cv_detection_api_type_t type)
{
	return NULL;
}

void qtk_cv_detection_api_delete(void* e)
{
	qtk_cv_detection_api_t *h = (qtk_cv_detection_api_t*)e;
	if (h)
	{
		switch(h->type){
		case QTK_CV_DETECTION_HEAD:
			qtk_cv_detection_onnxruntime_delete(((qtk_cv_detection_head_t*)h->h)->invo);
			qtk_cv_detection_head_delete(h->h);
			break;
//		case QTK_CV_DETECTION_BODY:
//			qtk_cv_detection_onnxruntime_delete(((qtk_cv_detection_body_t*)h->h)->invo);
//			qtk_cv_detection_body_detele(h->h);
//			break;
		case QTK_CV_DETECTION_FACE:
			qtk_cv_detection_onnxruntime_delete(((qtk_cv_detection_face_t*)h->h)->invo);
			qtk_cv_detection_face_delete(h->h);
			break;
//		case QTK_CV_DETECTION_HAND:
//			qtk_cv_detection_onnxruntime_delete(((qtk_cv_detection_hand_t*)h->h)->invo);
//			qtk_cv_detection_hand_delete(h->h);
//			break;
//		case QTK_CV_DETECTION_GES:
//			qtk_cv_detection_onnxruntime_delete(((qtk_cv_detection_hand_gesture_t*)h->h)->invo);
//			qtk_cv_detection_hand_gesture_new(h->h);
//			break;
//		case QTK_CV_DETECTION_DYNAMIC:
//			qtk_cv_detection_onnxruntime_delete(((qtk_cv_detection_dynamic_t*)h->h)->invo);
//			qtk_cv_detection_dynamic_delete(h->h);
//			break;
		}
	}
	if (h->main_cfg)
	{
		wtk_main_cfg_delete(h->main_cfg);
	}else if(h->mbin_cfg)
	{
		wtk_mbin_cfg_delete(h->mbin_cfg);
	}

	if (h->buf)
		wtk_strbuf_delete(h->buf);
    qtk_mot_sort_clean(&h->sort);

	wtk_free(h);
}

void qtk_cv_detection_api_process(void* e, uint8_t *data)
{
#ifdef USE_CNTLIMIT
	if  (qtk_count++ > MAX_COUNT_LIMIT)
	{
		wtk_debug("[Error]: overcome maximum use times\n");
		return;
	}
#endif
	qtk_cv_detection_api_t *h = (qtk_cv_detection_api_t*)e;
	if (h)
	{
		switch(h->type){
		case QTK_CV_DETECTION_HEAD:
			qtk_cv_detection_head_process(h->h, data, qtk_cv_detection_onnxruntime_invoke_single);
			break;
//		case QTK_CV_DETECTION_BODY:
//			qtk_cv_detection_body_process(h->h, data, qtk_cv_detection_onnxruntime_invoke_single);
//			break;
		case QTK_CV_DETECTION_FACE:
			qtk_cv_detection_face_process(h->h, data, qtk_cv_detection_onnxruntime_invoke_single);
			break;
//		case QTK_CV_DETECTION_HAND:
//			qtk_cv_detection_body_hand_process(h->h,h->hand, data, &(h->img_desc), resize_process,qtk_cv_detection_onnxruntime_invoke_single);
//			break;
//		case QTK_CV_DETECTION_GES:
//			qtk_cv_detection_hand_gesture_process(h->h,h->hand, data, &(h->img_desc), resize_process,qtk_cv_detection_onnxruntime_invoke_single);
//			break;
//		case QTK_CV_DETECTION_DYNAMIC:
//			qtk_cv_detection_dynamic_process(h->h, data, qtk_cv_detection_onnxruntime_invoke_single);
//			break;
		}
	}
}

void qtk_cv_detection_api_commprocess(void* e, uint8_t *data)
{
	qtk_cv_detection_api_t *h = (qtk_cv_detection_api_t*)e;
	if (h)
	{
		switch(h->type){
		case QTK_CV_DETECTION_HEAD:
			//REDO
			break;
//		case QTK_CV_DETECTION_BODY:
//			qtk_cv_detection_body_process_data(h->h, data, &(h->img_desc), resize_process, qtk_cv_detection_onnxruntime_invoke_single);
//			break;
		case QTK_CV_DETECTION_FACE:
			qtk_cv_detection_face_process_data(h->h, data, &(h->img_desc), resize_process, qtk_cv_detection_onnxruntime_invoke_single);
			break;
//		case QTK_CV_DETECTION_HAND:
//			qtk_cv_detection_body_hand_process(h->h, h->hand, data, &(h->img_desc), resize_process, qtk_cv_detection_onnxruntime_invoke_single);
//			break;
//		case QTK_CV_DETECTION_GES:
//			qtk_cv_detection_hand_gesture_process(h->h,h->hand, data, &(h->img_desc), resize_process,qtk_cv_detection_onnxruntime_invoke_single);
//			break;
//		case QTK_CV_DETECTION_DYNAMIC:
//			qtk_cv_detection_dynamic_process_data(h->h, data, resize_process, qtk_cv_detection_onnxruntime_invoke_single);
//			break;
		default:
			break;
		}
	}
}

void qtk_cv_detection_api_setProb(void* e, float iou, float conf)
{
	qtk_cv_detection_api_t *h = (qtk_cv_detection_api_t*)e;

	switch(h->type)
	{
	case QTK_CV_DETECTION_HEAD:
	case QTK_CV_DETECTION_FACE:
		((qtk_cv_detection_head_t*)(h->h))->iou = iou;
		((qtk_cv_detection_head_t*)(h->h))->conf = conf;
		break;
	default:
		break;
	}
}

void qtk_cv_detection_api_setAttrs(void* e, int width, int height, int channel)
{
	qtk_cv_detection_api_t *h = (qtk_cv_detection_api_t*)e;

	switch(h->type)
	{
	case QTK_CV_DETECTION_HEAD:
	case QTK_CV_DETECTION_FACE:
		h->img_desc.width = width;
		h->img_desc.height = height;
		h->img_desc.channel = channel;
		((qtk_cv_detection_head_t*)(h->h))->img_desc.width = width;
		((qtk_cv_detection_head_t*)(h->h))->img_desc.height = height;
		if (channel>0)
			((qtk_cv_detection_head_t*)(h->h))->img_desc.channel = channel;
		break;
	default:
		break;
	}
}

int qtk_cv_detection_api_rstCount(void* e)
{
	qtk_cv_detection_api_t *h = (qtk_cv_detection_api_t*)e;

	return ((qtk_cv_detection_head_t*)(h->h))->res.cnt;
}

qtk_cv_detection_api_box_t* qtk_cv_detection_api_rstValue(void* e)
{
	qtk_cv_detection_api_t *h = (qtk_cv_detection_api_t*)e;
	int i, cnt;

	cnt = ((qtk_cv_detection_head_t*)(h->h))->res.cnt;
	for (i=0; i < cnt; i++)
	{
		h->box[i].x1 = ((qtk_cv_detection_head_t*)(h->h))->res.box[i].roi.x1;
		h->box[i].y1 = ((qtk_cv_detection_head_t*)(h->h))->res.box[i].roi.y1;
		h->box[i].x2 = ((qtk_cv_detection_head_t*)(h->h))->res.box[i].roi.x2;
		h->box[i].y2 = ((qtk_cv_detection_head_t*)(h->h))->res.box[i].roi.y2;
		h->box[i].no = -1;

		wtk_strbuf_push(h->buf, (char *)&((qtk_cv_detection_head_t*)(h->h))->res.box[i].roi, sizeof(qtk_cv_bbox_t));
	}
	qtk_mot_sort_update(&h->sort, cnt, (qtk_cv_bbox_t *)h->buf->data);
	for (i = 0; i < cnt; i++) {
//		printf("%d ", h->sort.result[i]);
		h->box[i].no = h->sort.result[i];
	}
//	    printf("\n");
	    wtk_strbuf_reset(h->buf);

	return h->box;
}
