#include "qtk/nnrt/qtk_nnrt.h"
#include <jni.h>
#include <string.h>

JNIEXPORT void JNICALL Java_com_qdreamer_Env_setNNRTNumberThreads(
    JNIEnv *env, jobject this, jint numThreads, jstring affinity) {
#ifdef QTK_NNRT_ONNXRUNTIME
    char *affinity_str = (char *)(*env)->GetStringUTFChars(env, affinity, NULL);
    if (affinity_str == NULL || strlen(affinity_str) == 0) {
        affinity_str = NULL;
    }
    qtk_nnrt_set_global_thread_pool(numThreads, affinity_str);
#else
    (*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/Exception"),
                     "Not Implementation");
#endif
}
