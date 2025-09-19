#include "vprintWake.h"


static vw_jni_msg_t* vw_jni_msg_new(vw_jni_t *vw)
{
	vw_jni_msg_t *msg;

	msg = (vw_jni_msg_t*)wtk_malloc(sizeof(vw_jni_msg_t));
	msg->buf = wtk_strbuf_new(3200,1);
	return msg;
}

static int vw_jni_msg_delete(vw_jni_msg_t *msg)
{
	wtk_strbuf_delete(msg->buf);
	wtk_free(msg);
	return 0;
}

static vw_jni_msg_t* vw_jni_pop_msg(vw_jni_t *vw, vw_jni_type_t type, const char *data, int bytes)
{
	vw_jni_msg_t *msg;

	msg = (vw_jni_msg_t*)wtk_lockhoard_pop(&(vw->msg_hoard));
	wtk_strbuf_reset(msg->buf);
	wtk_strbuf_push_c(msg->buf,type);
	wtk_strbuf_push(msg->buf,data,bytes);
	return msg;
}

static void vw_jni_push_msg(vw_jni_t *vw, vw_jni_msg_t *msg)
{
	wtk_lockhoard_push(&(vw->msg_hoard),msg);
}


int vw_jni_on_notify(vw_jni_t *vw, qtk_vprint_wake_type_t type, char *data, int len)
{
	vw_jni_msg_t *msg = NULL;

	switch (type) {
	case QTK_VPRINT_WAKE_WAKE:
		msg = vw_jni_pop_msg(vw, QTK_JNI_WAKE, data, len);
		break;
	case QTK_VPRINT_WAKE_WAKE_PRE:
		msg = vw_jni_pop_msg(vw, QTK_JNI_WAKE_PRE, data, len);
		break;
	default:
		break;
	}
	
	if(msg) {
		wtk_blockqueue_push(&(vw->rlt_q),&(msg->q_n));
	}
	return 0;
}


/*
 * Class:     com_qdreamer_vprintWake_vprintWakeHelper
 * Method:    init
 * Signature: (Ljava/lang/String;)J
 */
	JNIEXPORT jlong JNICALL Java_com_qdreamer_vprintWake_vprintWakeHelper_init
(JNIEnv *env, jobject obj, jstring Jfn)
{
	vw_jni_t *vw;
	char *cfg_fn;
	int ret;

	vw = (vw_jni_t*)wtk_malloc(sizeof(vw_jni_t));
	cfg_fn = (char*) (*env)->GetStringUTFChars(env, Jfn, NULL);
	vw->vw = qtk_vprint_wake_new(cfg_fn);
	(*env)->ReleaseStringUTFChars(env, Jfn, cfg_fn);
	if(!vw->vw){
		ret = -1;
		goto end;
	}
	qtk_vprint_wake_set_notify(vw->vw, vw, (qtk_vprint_wake_notify_f)vw_jni_on_notify);
	wtk_lockhoard_init(&(vw->msg_hoard),offsetof(vw_jni_msg_t,hoard_n),20,
					   (wtk_new_handler_t)vw_jni_msg_new,
					   (wtk_delete_handler_t)vw_jni_msg_delete,
					   vw
					  );
	wtk_blockqueue_init(&(vw->rlt_q));
	ret = 0;
end:
	if(ret != 0){
		wtk_free(vw);
		vw = NULL;
	}
	printf(">>>>>>>>>>>>.. New %p, %p\n", vw, vw->vw);
	return (jlong)vw;
}

/*
 * Class:     com_qdreamer_vprintWake_vprintWakeHelper
 * Method:    delete
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_vprintWake_vprintWakeHelper_delete
  (JNIEnv *env, jobject obj, jlong jvw)
{
  vw_jni_t *vw;
  
  vw = (vw_jni_t *)jvw;
  printf(">>>>>>>>>>>>.. Delete %p, %p\n", vw, vw->vw);
  qtk_vprint_wake_delete(vw->vw);
  wtk_lockhoard_clean(&(vw->msg_hoard));
  wtk_blockqueue_clean(&(vw->rlt_q));

  return (jint)0;
}

/*
 * Class:     com_qdreamer_vprintWake_vprintWakeHelper
 * Method:    read
 * Signature: (J)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_qdreamer_vprintWake_vprintWakeHelper_read
(JNIEnv *env, jobject obj, jlong jvw)
{
	vw_jni_t *vw;
	wtk_queue_node_t *qn;
	vw_jni_msg_t *msg;
	jbyteArray jdata = NULL;

	vw = (vw_jni_t *)jvw;

	qn = wtk_blockqueue_pop(&(vw->rlt_q),-1,NULL);
	if(!qn) {
		goto end;
	}
	msg = data_offset2(qn,vw_jni_msg_t,q_n);
	jdata = (*env)->NewByteArray(env,msg->buf->pos);
	(*env)->SetByteArrayRegion(env,jdata,0,msg->buf->pos,(jbyte*)msg->buf->data);

	vw_jni_push_msg(vw,msg);

end:
	return jdata;
}

/*
 * Class:     com_qdreamer_vprintWake_vprintWakeHelper
 * Method:    enrollSetFn
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_vprintWake_vprintWakeHelper_enrollSetFn
(JNIEnv *env, jobject obj, jlong jvw, jstring Jfn)
{
	vw_jni_t *vw;
	char *fn;

	vw = (vw_jni_t *)jvw;
	fn = (char*) (*env)->GetStringUTFChars(env,Jfn,NULL);
	qtk_vprint_wake_enroll_set_fn(vw->vw, fn, strlen(fn));
	(*env)->ReleaseStringUTFChars(env,Jfn,fn);

	return (jint)0;
}

/*
 * Class:     com_qdreamer_vprintWake_vprintWakeHelper
 * Method:    enrollSetName
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_vprintWake_vprintWakeHelper_enrollSetName
(JNIEnv *env, jobject obj, jlong jvw, jstring Jfn)
{
	vw_jni_t *vw;
	char *fn;

	vw = (vw_jni_t *)jvw;
	fn = (char*) (*env)->GetStringUTFChars(env,Jfn,NULL);
	qtk_vprint_wake_enroll_set_name(vw->vw, fn, strlen(fn));
	(*env)->ReleaseStringUTFChars(env, Jfn, fn);

	return (jint)0;
}

/*
 * Class:     com_qdreamer_vprintWake_vprintWakeHelper
 * Method:    enrollStart
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_vprintWake_vprintWakeHelper_enrollStart
(JNIEnv *env, jobject obj, jlong jvw)
{
	vw_jni_t *vw;
	int ret;

	vw = (vw_jni_t *)jvw;
	ret = qtk_vprint_wake_enroll_start(vw->vw);

	return (jint)ret;
}

/*
 * Class:     com_qdreamer_vprintWake_vprintWakeHelper
 * Method:    enrollFeed
 * Signature: (J[BII)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_vprintWake_vprintWakeHelper_enrollFeed
(JNIEnv *env, jobject obj , jlong jvw, jbyteArray jdata, jint jbytes)
{

	vw_jni_t *vw;
	char *data;
	int ret;

	vw = (vw_jni_t *)jvw;
	if( ((int)jbytes) > 0) {
		data = (char*)(*env)->GetByteArrayElements(env,jdata,0);
		if(!data) {
			ret = -1;
			goto end;
		}

		ret = qtk_vprint_wake_enroll_feed(vw->vw, data, (int)jbytes);
		(*env)->ReleaseByteArrayElements(env,jdata, data, JNI_ABORT);
		if(ret != 0) {
			goto end;
		}
	}

	ret = 0;
end:
	return (jint)ret;
}

/*
 * Class:     com_qdreamer_vprintWake_vprintWakeHelper
 * Method:    enrollReset
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_vprintWake_vprintWakeHelper_enrollReset
(JNIEnv *env, jobject obj, jlong jvw)
{
	vw_jni_t *vw;
	int ret;

	vw = (vw_jni_t *)jvw;
	ret = qtk_vprint_wake_enroll_reset(vw->vw);

	return (jint)ret;
}



/*
 * Class:     com_qdreamer_vprintWake_vprintWakeHelper
 * Method:    enrollEnd
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_vprintWake_vprintWakeHelper_enrollEnd
(JNIEnv *env, jobject obj, jlong jvw)
{
	vw_jni_t *vw;

	vw = (vw_jni_t *)jvw;
	qtk_vprint_wake_enroll_end(vw->vw);

	return (jint)0;
}


/*
 * Class:     com_qdreamer_vprintWake_vprintWakeHelper
 * Method:    enrollClean
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_vprintWake_vprintWakeHelper_enrollClean
(JNIEnv *env, jobject obj, jlong jvw)
{
	vw_jni_t *vw;

	vw = (vw_jni_t *)jvw;
	qtk_vprint_wake_enroll_clean(vw->vw);

	return (jint)0;
}


/*
 * Class:     com_qdreamer_vprintWake_vprintWakeHelper
 * Method:    recognitionStart
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_vprintWake_vprintWakeHelper_recognitionStart
(JNIEnv *env, jobject obj, jlong jvw)
{
	vw_jni_t *vw;
	int ret;

	vw = (vw_jni_t *)jvw;
	ret = qtk_vprint_wake_recognition_start(vw->vw);

	return (jint)ret;
}

/*
 * Class:     com_qdreamer_vprintWake_vprintWakeHelper
 * Method:    enrollGetFn
 * Signature: (J)I
 */
JNIEXPORT jstring JNICALL Java_com_qdreamer_vprintWake_vprintWakeHelper_enrollGetFn
(JNIEnv *env, jobject obj, jlong jvw)
{
	vw_jni_t *vw;
	char *fn;

	fn = qtk_vprint_wake_enroll_get_fn(vw->vw);
	return (*env)->NewStringUTF(env, fn);
}

/*
 * Class:     com_qdreamer_vprintWake_vprintWakeHelper
 * Method:	  recognitionFeed
 * Signature: (J[BII)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_vprintWake_vprintWakeHelper_erecognitionFeed
(JNIEnv *env, jobject obj , jlong jvw, jbyteArray jdata, jint jbytes)
{

	vw_jni_t *vw;
	char *data;
	int ret;

	vw = (vw_jni_t *)jvw;
	if( ((int)jbytes) > 0) {
		data = (char*)(*env)->GetByteArrayElements(env,jdata,0);
		if(!data) {
			ret = -1;
			goto end;
		}

		ret = qtk_vprint_wake_recognition_feed(vw->vw, data, (int)jbytes);
		(*env)->ReleaseByteArrayElements(env,jdata, data, JNI_ABORT);
		if(ret != 0) {
			goto end;
		}
	}

	ret = 0;
end:
	return (jint)ret;
}

/*
 * Class:     com_qdreamer_vprintWake_vprintWakeHelper
 * Method:    recognitionReset
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_vprintWake_vprintWakeHelper_recognitionReset
(JNIEnv *env, jobject obj, jlong jvw)
{
	vw_jni_t *vw;
	int ret;

	vw = (vw_jni_t *)jvw;
	ret = qtk_vprint_wake_recognition_reset(vw->vw);

	return (jint)ret;
}

/*
 * Class:     com_qdreamer_vprintWake_vprintWakeHelper
 * Method:    recognitionSetConf
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_vprintWake_vprintWakeHelper_recognitionSetConf
(JNIEnv *env, jobject obj, jlong jvw, jfloat jconf)
{
	vw_jni_t *vw;

	vw = (vw_jni_t *)jvw;
	qtk_vprint_wake_recognition_set_conf(vw->vw, (float)jconf);

	return (jint)0;
}

/*
 * Class:     com_qdreamer_vprintWake_vprintWakeHelper
 * Method:    recognitionGetProb
 * Signature: (J)I
 */
JNIEXPORT jfloat JNICALL Java_com_qdreamer_vprintWake_vprintWakeHelper_recognitionGetProb
(JNIEnv *env, jobject obj, jlong jvw)
{
	vw_jni_t *vw;
	float prob;

	vw = (vw_jni_t *)jvw;
	printf(">>>>>>>>>>>>.. Prob %p, %p\n", vw, vw->vw);
	prob = qtk_vprint_wake_recognition_get_prob(vw->vw);

	return (jfloat)prob;
}
