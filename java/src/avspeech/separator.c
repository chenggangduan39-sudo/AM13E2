#ifdef QTK_USE_RGA
#include "RgaUtils.h"
#include "im2d.h"
#include "rga.h"
#endif

#include "qtk/avspeech/qtk_avspeech_separator.h"
#include "qtk/serde/qtk_serde_msgpack.h"
#include "wtk/bfio/resample/wtk_resample.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include <jni.h>

typedef struct {
    wtk_main_cfg_t *main_cfg;
    qtk_avspeech_separator_t *sep;
    wtk_strbuf_t *buf;
    uint8_t *avator;
    wtk_strbuf_t *audio_48k_buf;
    wtk_strbuf_t *audio_16k_buf;
    wtk_resample_t *resample;
    wtk_lock_t buf_guard;
    char *dst_buf;
} mod_t;

static int on_sep_(mod_t *m, int id, uint32_t frame_idx,
                   qtk_avspeech_separator_result_t *result) {
    int payload_start;
    uint32_t payload_len;
    wtk_strbuf_t *buf = m->buf;
    uint8_t *rgba_buf = NULL;


    if (result->type == QTK_AVSPEECH_SEPARATOR_AVATOR) {
	    rgba_buf = wtk_malloc(result->avator.width * result->avator.height*4);
        memset(rgba_buf, 255, result->avator.width * result->avator.height*4);
        int i;
        int npixel = result->avator.width * result->avator.height;
        // for (i = 0; i < npixel; i++) {
        //     memcpy(m->avator + i * 4, result->avator.I + i * 3, 3);
        // }
        for (i = 0; i < npixel; i++) {
            memcpy(rgba_buf + i * 4, result->avator.I + i * 3, 3);
        }
    }

    wtk_lock_lock(&m->buf_guard);
    wtk_strbuf_expand(m->buf, sizeof(uint32_t));
    m->buf->pos += sizeof(uint32_t);
    payload_start = m->buf->pos;

    switch (result->type) {
    case QTK_AVSPEECH_SEPARATOR_AUDIO:
        qtk_serde_msgpack_pack_map(3, buf, (qtk_io_writer)wtk_strbuf_push);
        qtk_serde_msgpack_pack_s("id", buf, (qtk_io_writer)wtk_strbuf_push);
        qtk_serde_msgpack_pack_int32(id, buf, (qtk_io_writer)wtk_strbuf_push);
        qtk_serde_msgpack_pack_s("type", buf, (qtk_io_writer)wtk_strbuf_push);
        qtk_serde_msgpack_pack_s("audio", buf, (qtk_io_writer)wtk_strbuf_push);
        qtk_serde_msgpack_pack_s("data", buf, (qtk_io_writer)wtk_strbuf_push);
        qtk_serde_msgpack_pack_bin(result->audio.wav, result->audio.len * 2 * 2,
                                   buf, (qtk_io_writer)wtk_strbuf_push);
        break;
    case QTK_AVSPEECH_SEPARATOR_AVATOR:
        qtk_serde_msgpack_pack_map(5, buf, (qtk_io_writer)wtk_strbuf_push);
        qtk_serde_msgpack_pack_s("id", buf, (qtk_io_writer)wtk_strbuf_push);
        qtk_serde_msgpack_pack_int32(id, buf, (qtk_io_writer)wtk_strbuf_push);
        qtk_serde_msgpack_pack_s("type", buf, (qtk_io_writer)wtk_strbuf_push);
        qtk_serde_msgpack_pack_str("avator", 6, buf,
                                   (qtk_io_writer)wtk_strbuf_push);
        qtk_serde_msgpack_pack_s("width", buf, (qtk_io_writer)wtk_strbuf_push);
        qtk_serde_msgpack_pack_int32(result->avator.width, buf,
                                     (qtk_io_writer)wtk_strbuf_push);
        qtk_serde_msgpack_pack_s("height", buf, (qtk_io_writer)wtk_strbuf_push);
        qtk_serde_msgpack_pack_int32(result->avator.height, buf,
                                     (qtk_io_writer)wtk_strbuf_push);
        qtk_serde_msgpack_pack_s("data", buf, (qtk_io_writer)wtk_strbuf_push);
        qtk_serde_msgpack_pack_bin(
            rgba_buf, result->avator.width * result->avator.height * 4,
            buf, (qtk_io_writer)wtk_strbuf_push);
	        wtk_free(rgba_buf);
        break;
    case QTK_AVSPEECH_SEPARATOR_FACE_ROI:

        qtk_serde_msgpack_pack_map(3, buf, (qtk_io_writer)wtk_strbuf_push);
        qtk_serde_msgpack_pack_s("id", buf, (qtk_io_writer)wtk_strbuf_push);
        qtk_serde_msgpack_pack_int32(id, buf, (qtk_io_writer)wtk_strbuf_push);
        qtk_serde_msgpack_pack_s("type", buf, (qtk_io_writer)wtk_strbuf_push);
        qtk_serde_msgpack_pack_s("face_roi", buf,
                                 (qtk_io_writer)wtk_strbuf_push);
        qtk_serde_msgpack_pack_s("roi", buf, (qtk_io_writer)wtk_strbuf_push);
        qtk_serde_msgpack_pack_array(4, buf, (qtk_io_writer)wtk_strbuf_push);
        qtk_serde_msgpack_pack_float(result->face_roi.x1, buf,
                                     (qtk_io_writer)wtk_strbuf_push);
        qtk_serde_msgpack_pack_float(result->face_roi.y1, buf,
                                     (qtk_io_writer)wtk_strbuf_push);
        qtk_serde_msgpack_pack_float(result->face_roi.x2, buf,
                                     (qtk_io_writer)wtk_strbuf_push);
        qtk_serde_msgpack_pack_float(result->face_roi.y2, buf,
                                     (qtk_io_writer)wtk_strbuf_push);

        break;
    case QTK_AVSPEECH_SEPARATOR_NOPERSON:
        qtk_serde_msgpack_pack_map(2, buf, (qtk_io_writer)wtk_strbuf_push);
        qtk_serde_msgpack_pack_s("id", buf, (qtk_io_writer)wtk_strbuf_push);
        qtk_serde_msgpack_pack_int32(id, buf, (qtk_io_writer)wtk_strbuf_push);
        qtk_serde_msgpack_pack_s("type", buf, (qtk_io_writer)wtk_strbuf_push);
        qtk_serde_msgpack_pack_s("noperson", buf,
                                 (qtk_io_writer)wtk_strbuf_push);
        break;
    }

    payload_len = m->buf->pos - payload_start;
    memcpy(m->buf->data + payload_start - sizeof(payload_len), &payload_len,
           sizeof(payload_len));

    wtk_lock_unlock(&m->buf_guard);

    return 0;
}

/*
 * Class:     com_qdreamer_avspeech_Separator
 * Method:    create
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_com_qdreamer_avspeech_Separator_create(
    JNIEnv *env, jobject this, jstring cfg, jstring custom_str) {
    mod_t *m = wtk_malloc(sizeof(mod_t));
    char *cfg_fn = (char *)(*env)->GetStringUTFChars(env, cfg, NULL);
    char *custom = (char *)(*env)->GetStringUTFChars(env, custom_str, NULL);
    // m->main_cfg = wtk_main_cfg_new_type(qtk_avspeech_separator_cfg, cfg_fn);
    wtk_debug("%s:%d=========>>>>>>>>>>>>>>>\n",__FUNCTION__,__LINE__);
    m->main_cfg = wtk_main_cfg_new_type_with_custom_str(qtk_avspeech_separator_cfg, cfg_fn, custom);
    wtk_debug("%s:%d=========>>>>>>>>>>>>>>>\n",__FUNCTION__,__LINE__);
    m->sep = qtk_avspeech_separator_new(
        (qtk_avspeech_separator_cfg_t *)m->main_cfg->cfg, m,
        (qtk_avspeech_separator_notifier_t)on_sep_);
    wtk_debug("%s:%d=========>>>>>>>>>>>>>>>\n",__FUNCTION__,__LINE__);
    m->buf = wtk_strbuf_new(1024, 1);
    m->audio_16k_buf = wtk_strbuf_new(1024, 1);
     m->audio_48k_buf = wtk_strbuf_new(1024 * 3, 1);
    m->resample = wtk_resample_new(1024);
    wtk_resample_set_notify(m->resample, m->audio_16k_buf,
                            (wtk_resample_notify_f)wtk_strbuf_push);
    wtk_resample_start(m->resample, 48000, 16000);
    m->dst_buf = wtk_malloc(3 * m->sep->cfg->width * m->sep->cfg->height);
#if 0
    m->avator = wtk_malloc(sizeof(uint8_t) * 4 *
                           m->sep->cfg->lip.face_alignment.dst_width *
                           m->sep->cfg->lip.face_alignment.dst_height);
    memset(m->avator, 255,
           4 * m->sep->cfg->lip.face_alignment.dst_width *
               m->sep->cfg->lip.face_alignment.dst_height);
#endif
    wtk_lock_init(&m->buf_guard);
    (*env)->ReleaseStringUTFChars(env, cfg, cfg_fn);
    (*env)->ReleaseStringUTFChars(env, custom_str, custom);
    return (jlong)m;
}

/*
 * Class:     com_qdreamer_avspeech_Separator
 * Method:    destory
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_qdreamer_avspeech_Separator_destory(
    JNIEnv *env, jobject this, jlong mod_ptr) {
    mod_t *m = (mod_t *)mod_ptr;
    qtk_avspeech_separator_delete(m->sep);
    wtk_main_cfg_delete(m->main_cfg);
    wtk_strbuf_delete(m->buf);
    wtk_lock_clean(&m->buf_guard);
    wtk_strbuf_delete(m->audio_48k_buf);
    wtk_strbuf_delete(m->audio_16k_buf);
    wtk_resample_delete(m->resample);
    //wtk_free(m->avator);
    wtk_free(m->dst_buf);
    wtk_free(m);
}
#ifdef QTK_USE_RGA
static void nv21_to_rgb(int W, int H, uint8_t *src, uint8_t *dst) {
    im_handle_param_t src_param, dst_param;
    im_rect src_rect, dst_rect;
    rga_buffer_t src_img, dst_img;
    rga_buffer_handle_t src_handle, dst_handle;
    int ret = -1;

    memset(&src_img, 0, sizeof(src_img));
    memset(&dst_img, 0, sizeof(dst_img));
    memset(&src_rect, 0, sizeof(src_rect));
    memset(&dst_rect, 0, sizeof(dst_rect));

    src_param.width = W;
    src_param.height = H;
    src_param.format = RK_FORMAT_YCrCb_420_SP; // NV21

    dst_param.width = W;
    dst_param.height = H;
    dst_param.format = RK_FORMAT_RGB_888;

    src_handle = importbuffer_virtualaddr(src, &src_param);
    dst_handle = importbuffer_virtualaddr(dst, &dst_param);
    if (src_handle == 0 || dst_handle == 0) {
        wtk_debug("importbuffer failed! %u %u\n", src_handle, dst_handle);
        if (src_handle)
            releasebuffer_handle(src_handle);
        if (dst_handle)
            releasebuffer_handle(dst_handle);
        return;
    }

    src_img = wrapbuffer_handle(src_handle, src_param.width, src_param.height,
                                src_param.format);
    dst_img = wrapbuffer_handle(dst_handle, dst_param.width, dst_param.height,
                                dst_param.format);

    ret = imcheck(src_img, dst_img, src_rect, dst_rect);
    if (IM_STATUS_NOERROR != ret) {
        wtk_debug("%d, check error! %s\n", __LINE__,
                  imStrError((IM_STATUS)ret));
        return;
    }

    ret = imcvtcolor(src_img, dst_img, src_param.format, dst_param.format);
    if (ret != IM_STATUS_SUCCESS) {
        wtk_debug("imcvtcolo rrunning failed, %s\n",
                  imStrError((IM_STATUS)ret));
        if (src_handle)
            releasebuffer_handle(src_handle);
        if (dst_handle)
            releasebuffer_handle(dst_handle);
        return;
    }
    if (src_handle) releasebuffer_handle(src_handle);
    if (dst_handle) releasebuffer_handle(dst_handle);
}

/*
 * Class:     com_qdreamer_avspeech_Separator
 * Method:    feedImageNV21
 * Signature: (J[BI)V
 */
JNIEXPORT void JNICALL Java_com_qdreamer_avspeech_Separator_feedImageNV21(
    JNIEnv *env, jobject this, jlong mod_ptr, jbyteArray image, jint len) {

    mod_t *m = (mod_t *)mod_ptr;
    int H = m->sep->cfg->height;
    int W = m->sep->cfg->width;
    uint8_t *data = (uint8_t *)(*env)->GetByteArrayElements(
        env, image, 0); // hypothesis data NV21
    nv21_to_rgb(W, H, data, (uint8_t *)m->dst_buf);
    qtk_avspeech_separator_feed_image(m->sep, (uint8_t *)m->dst_buf);
    (*env)->ReleaseByteArrayElements(env, image, (jbyte *)data, 0);

    
}
#else
/*
 * Class:     com_qdreamer_avspeech_Separator
 * Method:    feedImageNV21
 * Signature: (J[BI)V
 */
JNIEXPORT void JNICALL Java_com_qdreamer_avspeech_Separator_feedImageNV21(
    JNIEnv *env, jobject this, jlong mod_ptr, jbyteArray image, jint len) {

    mod_t *m = (mod_t *)mod_ptr;
    int H = m->sep->cfg->height;
    int W = m->sep->cfg->width;
    uint8_t *data = (uint8_t *)(*env)->GetByteArrayElements(
        env, image, 0); // hypothesis data NV21
    qtk_avspeech_separator_feed_image(m->sep, data);
    (*env)->ReleaseByteArrayElements(env, image, (jbyte *)data, 0);
}
#endif
#if 1

/*
 * Class:     com_qdreamer_avspeech_Separator
 * Method:    feedImage
 * Signature: (J[BIII)V
 */
JNIEXPORT void JNICALL Java_com_qdreamer_avspeech_Separator_feedImage(
    JNIEnv *env, jobject this, jlong mod_ptr, jbyteArray image, jint len,
    jint H, jint W) {
    mod_t *m = (mod_t *)mod_ptr;
    uint8_t *data = (uint8_t *)(*env)->GetByteArrayElements(env, image, 0); // hypothesis data NV12
#ifdef QTK_USE_RGA
    im_handle_param_t src_param, dst_param;
    W = m->sep->cfg->width;
    H = m->sep->cfg->height;

    im_rect src_rect, dst_rect;
    rga_buffer_t src_img, dst_img;
    rga_buffer_handle_t src_handle, dst_handle;
    int ret = -1;

    memset(&src_img, 0, sizeof(src_img));
    memset(&dst_img, 0, sizeof(dst_img));
    memset(&src_rect, 0, sizeof(src_rect));
    memset(&dst_rect, 0, sizeof(dst_rect));

    src_param.width = W;
    src_param.height = H;
    // src_param.format = RK_FORMAT_YCbCr_420_SP; // NV12
    src_param.format = RK_FORMAT_RGBA_8888; // RGBA 

    dst_param.width = W;
    dst_param.height = H;
    dst_param.format = RK_FORMAT_RGB_888;

    src_handle = importbuffer_virtualaddr(data, &src_param);
    dst_handle = importbuffer_virtualaddr(m->dst_buf, &dst_param);
    if (src_handle == 0 || dst_handle == 0)
    {
        wtk_debug("importbuffer failed! %u %u\n", src_handle, dst_handle);
        if (src_handle) releasebuffer_handle(src_handle);
        if (dst_handle) releasebuffer_handle(dst_handle);
	(*env)->ReleaseByteArrayElements(env, image, (uint8_t *)data, JNI_ABORT);
        return;
    }

    src_img = wrapbuffer_handle(src_handle, src_param.width, src_param.height, src_param.format);
    dst_img = wrapbuffer_handle(dst_handle, dst_param.width, dst_param.height, dst_param.format);

    ret = imcheck(src_img, dst_img, src_rect, dst_rect);
    if (IM_STATUS_NOERROR != ret)
    {
        wtk_debug("%d, check error! %s\n", __LINE__, imStrError((IM_STATUS)ret));
        if (src_handle) releasebuffer_handle(src_handle);
        if (dst_handle) releasebuffer_handle(dst_handle);
	(*env)->ReleaseByteArrayElements(env, image, (uint8_t *)data, JNI_ABORT);
        return ;
    }

    ret = imcvtcolor(src_img, dst_img, src_param.format, dst_param.format);
    if (ret != IM_STATUS_SUCCESS)
    {
        wtk_debug("imcvtcolo rrunning failed, %s\n", imStrError((IM_STATUS)ret));
        if (src_handle) releasebuffer_handle(src_handle);
        if (dst_handle) releasebuffer_handle(dst_handle);
	(*env)->ReleaseByteArrayElements(env, image, (uint8_t *)data, JNI_ABORT);

        return;
    }

    // qtk_avspeech_separator_feed_image(m->sep, data, H, W);
    qtk_avspeech_separator_feed_image(m->sep, (uint8_t *)m->dst_buf);
    if (src_handle) releasebuffer_handle(src_handle);
    if (dst_handle) releasebuffer_handle(dst_handle);
#else
    qtk_avspeech_separator_feed_image(m->sep, data);
    (*env)->ReleaseByteArrayElements(env, image, (uint8_t *)data, 0);
#endif
}
#endif
/*
 * Class:     com_qdreamer_avspeech_Separator
 * Method:    feedAudio
 * Signature: (J[BI)[B
 */
JNIEXPORT void JNICALL Java_com_qdreamer_avspeech_Separator_feedAudio(
    JNIEnv *env, jobject this, jlong mod_ptr, jbyteArray audio, jint len, jint is_48k) {
    int i;
    mod_t *m = (mod_t *)mod_ptr;
    short *data = (short *)(*env)->GetByteArrayElements(env, audio, 0);
    if (is_48k){
        wtk_strbuf_reset(m->audio_48k_buf);
        wtk_strbuf_expand(m->audio_48k_buf, len / 2);
        short *data_1ch = m->audio_48k_buf->data;
        for (i = 0; i < len / 4; i++) {
            data_1ch[i] = data[i * 2];
        }
        wtk_resample_feed(m->resample, data_1ch, len / 2, 0);
        qtk_avspeech_separator_feed_audio(m->sep, m->audio_16k_buf->data,
                                          m->audio_16k_buf->pos / 2);
    }else{
        if (m->sep->cfg->use_qform9) {
            qtk_avspeech_separator_feed_audio(
                m->sep, data, len / (2 * m->sep->cfg->qform9.stft2.channel));
        } else {
            qtk_avspeech_separator_feed_audio(m->sep, data,
                                              len / 2); // 1 channel 16k
        }
    }

    (*env)->ReleaseByteArrayElements(env, audio, (short *)data, 0);	
    wtk_strbuf_reset(m->audio_16k_buf);
}

/*
 * Class:     com_qdreamer_avspeech_Separator
 * Method:    read
 * Signature: (J)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_qdreamer_avspeech_Separator_read(
    JNIEnv *env, jobject this, jlong mod_ptr) {
    jbyteArray result = NULL;
    mod_t *m = (mod_t *)mod_ptr;
    wtk_lock_lock(&m->buf_guard);
    if (m->buf->pos > 0) {
        result = (*env)->NewByteArray(env, m->buf->pos);
        (*env)->SetByteArrayRegion(env, result, 0, m->buf->pos,
                                   (jbyte *)m->buf->data);
        wtk_strbuf_reset(m->buf);
    }
    wtk_lock_unlock(&m->buf_guard);
    return result;
}

/*
 * Class:     com_qdreamer_avspeech_Separator
 * Method:    reset
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_avspeech_Separator_reset(JNIEnv *env, jobject this, jlong mod_ptr){
    mod_t *m = (mod_t *)mod_ptr;
    return qtk_avspeech_separator_reset(m->sep);
}
#if 0
extern void avspeech_process(char *cfn, char *dfn, int n);
JNIEXPORT void JNICALL Java_com_qdreamer_avspeech_Separator_process
  (JNIEnv *env, jobject obj, jstring cfn, jstring dfn, jint n){
    char *fnc = (char *)(*env)->GetStringUTFChars(env, cfn, NULL);
    char *fnd = (char *)(*env)->GetStringUTFChars(env, dfn, NULL);

    avspeech_process(fnc, fnd, n);
    (*env)->ReleaseStringUTFChars(env, cfn, fnc);
    (*env)->ReleaseStringUTFChars(env, dfn, fnd);

  }
#endif
#if 1
void qtk_get_track_roi(int boundary_angle, int camera_angle, int *roi_array, int W) {
    float deep = W / (2 * tan(camera_angle * M_PI / 360));
    float theta_left = -boundary_angle; 
    float theta_right = boundary_angle; 

    float left_offset_x = deep * tan(theta_left * M_PI / 180); 
    float right_offset_x = deep * tan(theta_right * M_PI / 180);

    float x_left = (W / 2) + left_offset_x;
    float x_right = (W / 2) + right_offset_x;

    if (x_left < 0.000001 || x_right > W) {
        wtk_debug("lip->cfg->theta_tolerance Invalid !!!\n");
        roi_array[0] = 0; // x_left
        roi_array[1] = W; // x_right
        return;
    }

    roi_array[0] = (int)(x_left);  
    roi_array[1] = (int)(x_right); 
}


// JNIEXPORT jbyteArray JNICALL Java_QTKNative_getTrackROI(JNIEnv *env, jobject obj, jint boundary_angle, jint camera_angle, jint W);
JNIEXPORT jintArray JNICALL Java_com_qdreamer_avspeech_Separator_getroi
  (JNIEnv *env, jobject obj, jint boundary_angle, jint camera_angle, jint W){
    jintArray roiArray = (*env)->NewIntArray(env, 2); // 长度为2，存储 x_left 和 x_right
    int roi_result[2];
    qtk_get_track_roi(boundary_angle, camera_angle, roi_result, W);
    wtk_debug("roi_result[0] = %d, roi_result[1] = %d\n", roi_result[0], roi_result[1]);
    (*env)->SetIntArrayRegion(env, roiArray, 0, 2, roi_result);
    return roiArray; 
}
#endif
