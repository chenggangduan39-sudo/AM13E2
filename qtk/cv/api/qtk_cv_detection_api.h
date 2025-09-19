#ifndef QTK_CV_DETECTION_API_H
#define QTK_CV_DETECTION_API_H
#ifdef __cplusplus
extern "C" {
#endif

/* detection type */
typedef enum {
	QTK_CV_DETECTION_HEAD,
//	QTK_CV_DETECTION_BODY,
	QTK_CV_DETECTION_FACE,
//	QTK_CV_DETECTION_HAND,
//	QTK_CV_DETECTION_GES,
//	QTK_CV_DETECTION_DYNAMIC
}qtk_cv_detection_api_type_t;

//position coordinate
typedef struct {
    float x1;
    float x2;
    float y1;
    float y2;
    int no;
} qtk_cv_detection_api_box_t;


/*
 * *********************************************************************
 * API execute process
 * 1. build engine [once]
 *                qtk_cv_detection_api_new
 *
 * 2. set parameter for engine [selected]
 *                     qtk_cv_detection_api_setAttrs
 *                     qtk_cv_detection_api_setProb
 *
 * 3. process input data [multi-times]
 *                qtk_cv_detection_api_process
 *
 * 4. obtain result [multi-times]
 *                qtk_cv_detection_api_rstCount/qtk_cv_detection_api_rstValue
 *
 * 5. destroy engine [once]
 *                qtk_cv_detection_api_delete
 *
 */
/**
 * Function: qtk_cv_detection_api_new
 * Description: build detection engine
 * Parameter:
 *     model, cfg file
 *     type, detection type
   * Return:
 *     engine handler
 */
void *qtk_cv_detection_api_new(char* cfn, qtk_cv_detection_api_type_t type);

/**
 * Function: qtk_cv_detection_api_newb
 * Description: build detection engine
 * Parameter:
 *     model, model data stream
 *     type, detection type
 * Return:
 *     engine handler
 * Note:
 *     no support on current
 */
void *qtk_cv_detection_api_newb(char* model_arr, qtk_cv_detection_api_type_t type);

/**
 * Function: qtk_cv_detection_api_delete
 * Description: free detection engine
 * Parameter:
 *     e, engine handler
  * Return:
 *     void
 */

void qtk_cv_detection_api_delete(void* e);

/**
 * Function: qtk_cv_detection_api_process
 * Description: process detection engine with input data of special format.
 * Parameter:
 *     e, engine handler
 *     data, input data, must fit:
 *            head: --
 *            face: 960*544*3 BGR
 *            head: --
 *            gesture: 128*128*3 BGR
 *            dynamic: 224*244*3 RGB
 *            body_hand: <=3840*2160*3 BGR
 *            hand_gesture: --
 *            stand: 960*544*3 BGR
 */
void qtk_cv_detection_api_process(void* e, uint8_t *data);

/**
 * Function: qtk_cv_detection_api_commprocess
 * Description: process detection engine with input data of common format.
 * Parameter:
 *     e, engine handler
 *     data, input data, common format, but must fit
 *            body: <= 3840*2160*3 BGR
 *            face: <= 3840*2160*3 BGR
 *            dynamic: <= 3840*2160*3 RGB
 * Return:
 *     void
 * Note:
 *     no support on current
 */
void qtk_cv_detection_api_commprocess(void* e, uint8_t *data);

/**
 * Function: qtk_cv_detection_api_setAttrs
 * Description: set image attribute info,, selected function
 * Parameter:
 *     e, engine handler
 *     width, width of image
 *     height, height of image
 *     channel, channels of image, don't update when set 0.
 * Return
 *     void
 */
void qtk_cv_detection_api_setAttrs(void* e, int width, int height, int channel);

/**
 * Function: qtk_cv_detection_api_setProb
 * Description: set param value, selected function
 * Parameter:
 *     e, engine handler
 *     iou, Screening probability
 *     conf, overlap probability
 * Return
 *     void
 */
void qtk_cv_detection_api_setProb(void* e, float iou, float conf);

/**
 * Function: qtk_cv_detection_api_rstCount
 * Description: number of result
 * Parameter:
 *      e, engine handler
 * Return:
 *      count of result
 */
int qtk_cv_detection_api_rstCount(void* e);

/**
 * Function: qtk_cv_detection_api_rstValue
 * Description: number of result
 * Parameter:
 *      e, engine handler
 * Return:
 *      address of values
 */
qtk_cv_detection_api_box_t* qtk_cv_detection_api_rstValue(void* e);

#ifdef __cplusplus
};
#endif
#endif
