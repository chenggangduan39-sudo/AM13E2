#ifndef _Included_com_qdreamer_qvoice_QMulsv
#define _Included_com_qdreamer_qvoice_QMulsv
#include "qvoice_init.h"
#include "sdk/mulsv/qtk_mulsv_api.h"
#ifdef __cplusplus
extern "C" {

/*
 * Class:     com_qdreamer_qvoice_QMulsv
 * Method:    newMulsv
 * Signature: (Ljava/lang/String;II)J
 */
JNIEXPORT jlong JNICALL Java_com_qdreamer_qvoice_QMulsv_newMulsv
  (JNIEnv *, jclass, jstring, jint, jint);

/*
 * Class:     com_qdreamer_qvoice_QMulsv
 * Method:    startMulsv
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_qdreamer_qvoice_QMulsv_startMulsv
  (JNIEnv *, jclass, jlong);

/*
 * Class:     com_qdreamer_qvoice_QMulsv
 * Method:    registerMulsv_start
 * Signature: (JLjava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_com_qdreamer_qvoice_QMulsv_registerMulsv_1start
  (JNIEnv *, jclass, jlong, jstring);

/*
 * Class:     com_qdreamer_qvoice_QMulsv
 * Method:    feedMulsv
 * Signature: (J[BI)V
 */
JNIEXPORT jlong JNICALL Java_com_qdreamer_qvoice_QMulsv_feedMulsv
  (JNIEnv *, jclass, jlong, jbyteArray, jint);

/*
 * Class:     com_qdreamer_qvoice_QMulsv
 * Method:    registerMulsv_endl
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_com_qdreamer_qvoice_QMulsv_registerMulsv_1endl
  (JNIEnv *, jclass, jlong);

/*
 * Class:     com_qdreamer_qvoice_QMulsv
 * Method:    resetMulsv
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_com_qdreamer_qvoice_QMulsv_resetMulsv
  (JNIEnv *, jclass, jlong);

/*
 * Class:     com_qdreamer_qvoice_QMulsv
 * Method:    deleteMulsv
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_qdreamer_qvoice_QMulsv_deleteMulsv
  (JNIEnv *, jclass, jlong);

/*
 * Class:     com_qdreamer_qvoice_QMulsv
 * Method:    readMulsv
 * Signature: (J)V
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_QMulsv_readMulsv
  (JNIEnv *, jclass);

JNIEXPORT void JNICALL Java_com_qdreamer_qvoice_QMulsv_reset
(JNIEnv *, jclass);

#ifdef __cplusplus
}
#endif
#endif /* D5ECABF8_76BC_48FB_AEC5_57675A69EFE9 */
#endif
