/*
 * wtk_asr_api.h
 *
 *  Created on: 2024年3月27日
 *      Author: root
 */

#ifndef WTK_ASR_API_QVOICE_ASR_API_H_
#define WTK_ASR_API_QVOICE_ASR_API_H_
#ifdef __cplusplus
extern "C" {
#endif


/*
 * *********************************************************************
 * API execute process
 * 1. build engine [once]
 *                qvoice_asr_api_new
 *
 * 2. set parameter for engine [once]
 *                qvoice_asr_api_setContext
 *
 * 3. approach the engine [once]
 *                qvoice_asr_api_start
 *
 * 4. process input data [multi-times]
 *                qvoice_asr_api_feed (Note: last times must set isend=1)
 *
 * 5. obtain result [multi-times]
 *                qvoice_asr_api_get_wake
 *
 * 6. reset engine [multi-times]
 *                qvoice_asr_api_reset
 *
 * 7. destroy engine [once]
 *                qvoice_asr_api_delete
 *
 */
/**
 * Function: qvoice_asr_api_new
 * Description: build asr-wakeup engine
 * Parameter:
 *     cfg_fn, asr model file
 *     vcfg_fn, vad model file
   * Return:
 *     engine handler
 */
void* qvoice_asr_api_new(char* cfg_fn, char* vcfg_fn);

/**
 * Function: qtk_cv_detection_api_delete
 * Description: start asr engine
 * Parameter:
 *     e, engine handler
  * Return:
 *     void
 */
int qvoice_asr_api_start(void* h);

/**
 * Function: qvoice_asr_api_feed
 * Description: free data to asr engine
 * Parameter:
 *     h, engine handler
 *     data, audio data
 *     size, audio data size
 *     isend, if audio is end
  * Return:
 *     Error Cod, 0 means success, -1 means failed.
 */
int qvoice_asr_api_feed(void* h, char* data, int size, int isend);

/**
 * Function: qvoice_asr_api_setContext
 * Description: feed hot-words to asr engine
 * Parameter:
 *     h, engine handler
 *     data, hot-words data
 *     bytes, hot-words data size
  * Return:
 *     Error Cod, 0 means success, -1 means failed.
 */
int qvoice_asr_api_setContext(void* h,char *data,int bytes);

/**
 * Function: qvoice_asr_api_setContext_asr
 * Description: feed hot-words to asr engine
 * Parameter:
 *     h, engine handler
 *     data, hot-words data
 *     bytes, hot-words data size
  * Return:
 *     Error Cod, 0 means success, -1 means failed.
 */
int qvoice_asr_api_setContext_asr(void* h,char *data,int bytes);

/**
 * Function: qvoice_asr_api_setContext_wakeup
 * Description: feed hot-words to asr engine
 * Parameter:
 *     h, engine handler
 *     data, hot-words data
 *     bytes, hot-words data size
  * Return:
 *     Error Cod, 0 means success, -1 means failed.
 */
int qvoice_asr_api_setContext_wakeup(void* h,char *data,int bytes);

/**
 * Function: qvoice_asr_api_get_result
 * Description: obtain asr result
 * Parameter:
 *     h, engine handler
 *     data, audio data
 *     len, audio data size
  * Return:
 *     Error Cod, 0 means success, -1 means failed.
 */
int qvoice_asr_api_get_result(void* h, char*data, int len);

/**
 * Function: qvoice_asr_api_get_wakecmd
 * Description: obtain number of wakeup times
 * Parameter:
 *     e, engine handler
  * Return:
 *     1 means wakeup, 0 means no wakeup
 */
int qvoice_asr_api_get_wake(void* h);

/**
 * Function: qvoice_asr_api_reset
 * Description: reset asr engine
 * Parameter:
 *     e, engine handler
  * Return:
 *     Error Cod, 0 means success, -1 means failed.
 */
int qvoice_asr_api_reset(void* h);

/**
 * Function: qvoice_asr_api_delete
 * Description: free asr engine
 * Parameter:
 *     e, engine handler
  * Return:
 *     void
 */
void qvoice_asr_api_delete(void* h);

#ifdef __cplusplus
};
#endif
#endif /* WTK_ASR_API_QVOICE_ASR_API_H_ */
