#include <jni.h>

#include "qtk/camera/qtk_camera.h"
#include "wtk/core/wtk_alloc.h"

typedef struct {
    qtk_camera_t cam;
} impl_t;

/*
 * Class:     com_qdreamer_os_Camera
 * Method:    create
 * Signature: (Ljava/lang/String;ZI)J
 */
JNIEXPORT jlong JNICALL Java_com_qdreamer_os_Camera_create(JNIEnv *env,
                                                           jobject ths,
                                                           jstring device,
                                                           jboolean blocking,
                                                           jint num_buffers) {
    impl_t *m = wtk_malloc(sizeof(impl_t));
    char *dev = (char *)(*env)->GetStringUTFChars(env, device, NULL);
    m->cam = qtk_camera_new(dev, blocking, num_buffers);
    return (jlong)m;
}

/*
 * Class:     com_qdreamer_os_Camera
 * Method:    destroy
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_qdreamer_os_Camera_destroy(JNIEnv *env,
                                                           jobject ths,
                                                           jlong mod) {
    impl_t *m = (impl_t *)mod;
    qtk_camera_delete(m->cam);
}

/*
 * Class:     com_qdreamer_os_Camera
 * Method:    setResolution
 * Signature: (JII)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_os_Camera_setResolution(
    JNIEnv *env, jobject ths, jlong mod, jint W, jint H) {
    impl_t *m = (impl_t *)mod;
    return qtk_camera_set_resolution(m->cam, W, H);
}

/*
 * Class:     com_qdreamer_os_Camera
 * Method:    setFPS
 * Signature: (JI)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_os_Camera_setFPS(JNIEnv *env,
                                                          jobject ths,
                                                          jlong mod, jint fps) {
    impl_t *m = (impl_t *)mod;
    return qtk_camera_set_fps(m->cam, fps);
}

/*
 * Class:     com_qdreamer_os_Camera
 * Method:    setFmt
 * Signature: (JLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_os_Camera_setFmt(JNIEnv *env,
                                                          jobject ths,
                                                          jlong mod,
                                                          jstring fmt) {
    impl_t *m = (impl_t *)mod;
    char *fmt_str = (char *)(*env)->GetStringUTFChars(env, fmt, NULL);
    qtk_image_fmt_t fmt_type;
    if (strcmp(fmt_str, "NV12") == 0) {
        fmt_type = QBL_IMAGE_NV12;
    } else if (strcmp(fmt_str, "RGB888") == 0) {
        fmt_type = QBL_IMAGE_RGB24;
    } else if (strcmp(fmt_str, "MJPEG") == 0) {
        fmt_type = QBL_IMAGE_MJPEG;
    } else {
        wtk_debug("%s not support\n", fmt_str);
        return -1;
    }
    return qtk_camera_set_fmt(m->cam, fmt_type);
}

/*
 * Class:     com_qdreamer_os_Camera
 * Method:    capFrame
 * Signature: (J)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_qdreamer_os_Camera_capFrame(JNIEnv *env,
                                                                  jobject ths,
                                                                  jlong mod) {
    impl_t *m = (impl_t *)mod;
    jbyteArray result = NULL;
    int idx;
    uint8_t *frame;
    size_t len;
    qtk_camera_cap_frame(m->cam, &frame, &len, &idx);
    result = (*env)->NewByteArray(env, len);
    (*env)->SetByteArrayRegion(env, result, 0, len, (jbyte *)frame);
    qtk_camera_release_frame(m->cam, idx);
    return result;
}

/*
 * Class:     com_qdreamer_os_Camera
 * Method:    start
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_os_Camera_start(JNIEnv *env,
                                                         jobject ths,
                                                         jlong mod) {
    impl_t *m = (impl_t *)mod;
    return qtk_camera_start(m->cam);
}

/*
 * Class:     com_qdreamer_os_Camera
 * Method:    stop
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_os_Camera_stop(JNIEnv *env,
                                                        jobject ths,
                                                        jlong mod) {
    impl_t *m = (impl_t *)mod;
    return qtk_camera_stop(m->cam);
}
