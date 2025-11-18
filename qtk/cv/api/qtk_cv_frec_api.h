/*
 * qtk_cv_frec_api.h
 *
 *  Created on: 2024年4月7日
 *      Author: root
 */

#ifndef QTK_CV_API_QTK_CV_FREC_API_H_
#define QTK_CV_API_QTK_CV_FREC_API_H_
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

//position coordinate
typedef struct {
    float x1;
    float x2;
    float y1;
    float y2;
    char name[32];
} qtk_cv_frec_api_box_t;


/*
 * *********************************************************************
 * API execute process
 * 1. build engine [once]
 *                qtk_cv_frec_api_new
 *
 * 2. parameter setting [selected]
 *                qtk_cv_frec_api_setThresh
 *
 * 2. face register [multi-times]
 *                qtk_cv_frec_api_register
 *
 * 3. face recognition [multi-times]
 *                qtk_cv_frec_api_rec
 *
 * 4. obtain result [multi-times]
 *                qtk_cv_frec_api_rstValue
 *
 * 5. destroy engine [once]
 *                qtk_cv_frec_api_delete
 *
 ************************************************************************/

/**
 * Function: qtk_cv_frec_api_new
 * Description: build frec engine
 * Parameter:
 *     model, model file
 *     type, frec type
   * Return:
 *     engine handler
 */
void* qtk_cv_frec_api_new(char* cfn);

/**
 * Function: qtk_cv_frec_api_delete
 * Description: free frec engine
 * Parameter:
 *     e, engine handler
  * Return:
 *     void
 */
void qtk_cv_frec_api_delete(void *e);

/**
 * Function: qtk_cv_frec_api_register
 * Description: register face object and update register info.
 * Parameter:
 *     e, engine handler
 *     data, input data, must fit:
 *            face: 960*544*3 BGR
 *     user, user name
 *     user_size, user name length
 */
int qtk_cv_frec_api_register(void *h, uint8_t *data, char* user, int user_size);

/**
 * Function: qtk_cv_frec_api_process
 * Description: process frec engine with input data of special format.
 * Parameter:
 *     e, engine handler
 *     data, input data, must fit:
 *            face: 960*544*3 BGR
 */
int qtk_cv_frec_api_rec(void* e, uint8_t *data);

/**
 * Function: qtk_cv_frec_api_setThresh
 * Description: set recognition threshold value, selected function
 * Parameter:
 *     e, engine handler
 *     thresh, threshold
 * Return
 *     void
 */
void qtk_cv_frec_api_setThresh(void* e, float thresh);

/**
 * Function: qtk_cv_frec_api_rstValue
 * Description: number of result, box save result info.
 * Parameter:
 *      e, engine handler
 *      box, save result info
 *      target, 0 means check result, 1 means face recognition result.
 * Return:
 *      address of values
 */
int qtk_cv_frec_api_rstValue(void* h, qtk_cv_frec_api_box_t **box, int target);
#ifdef __cplusplus
};
#endif
#endif /* QTK_CV_API_QTK_CV_FREC_API_H_ */
