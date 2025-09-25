#include "oggdec.h"


static qtk_jni_msg_t* qtk_jni_msg_new(qtk_jni_oggdec_t *oggdec)
{
	qtk_jni_msg_t *msg;

	msg = (qtk_jni_msg_t*)wtk_malloc(sizeof(qtk_jni_msg_t));
	msg->buf = wtk_strbuf_new(3200,1);
	return msg;
}

static int qtk_jni_msg_delete(qtk_jni_msg_t *msg)
{
	wtk_strbuf_delete(msg->buf);
	wtk_free(msg);
	return 0;
}

static qtk_jni_msg_t* qtk_jni_pop_msg(qtk_jni_oggdec_t *oggdec,char *data,int bytes)
{
	qtk_jni_msg_t *msg;

	msg = (qtk_jni_msg_t*)wtk_lockhoard_pop(&(oggdec->msg_hoard));
	wtk_strbuf_reset(msg->buf);
	wtk_strbuf_push(msg->buf,data,bytes);
	return msg;
}

static void qtk_jni_push_msg(qtk_jni_oggdec_t *oggdec,qtk_jni_msg_t *msg)
{
	wtk_lockhoard_push(&(oggdec->msg_hoard),msg);
}


int qtk_jni_ogg_on_notify(qtk_jni_oggdec_t *oggdec, char *buf,int size)
{
	qtk_jni_msg_t *msg = NULL;

  msg = qtk_jni_pop_msg(oggdec, buf, size);
  if(msg) {
		wtk_blockqueue_push(&(oggdec->rlt_q),&(msg->q_n));
	}
  return 0;
}


/*
 * Class:     com_qdreamer_qvoice_OggHelper
 * Method:    init
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_com_qdreamer_qvoice_OggHelper_init
  (JNIEnv *env, jobject obj, jstring Jfn)
{
    qtk_jni_oggdec_t *oggdec;
    int ret;

    oggdec = (qtk_jni_oggdec_t*)wtk_malloc(sizeof(qtk_jni_oggdec_t));
    oggdec->oggdec = qtk_oggdec_new();
    if(!oggdec->oggdec){
      ret = -1;
      goto end;
    }
    wtk_lockhoard_init(&(oggdec->msg_hoard),offsetof(qtk_jni_msg_t,hoard_n),20,
			(wtk_new_handler_t)qtk_jni_msg_new,
			(wtk_delete_handler_t)qtk_jni_msg_delete,
			oggdec
			);
	wtk_blockqueue_init(&(oggdec->rlt_q));
  ret = 0;
end:
  if(ret != 0){
    wtk_free(oggdec);
    oggdec = NULL;
  }
  return (jlong)oggdec;
}

/*
 * Class:     com_qdreamer_qvoice_OggHelper
 * Method:    delete
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_OggHelper_delete
  (JNIEnv *env, jobject obj, jlong joggdec)
{
  qtk_jni_oggdec_t *oggdec;
  
  oggdec = (qtk_jni_oggdec_t *)joggdec;
  qtk_oggdec_delete(oggdec->oggdec);

  return (jint)0;
}
/*
 * Class:     com_qdreamer_qvoice_OggHelper
 * Method:    start
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_OggHelper_start
  (JNIEnv *env, jobject obj, jlong joggdec)
{
  qtk_jni_oggdec_t *oggdec;
  
  oggdec = (qtk_jni_oggdec_t *)joggdec;
  qtk_oggdec_start(oggdec->oggdec, (qtk_oggdec_write_f)qtk_jni_ogg_on_notify,oggdec);

  return (jint)0;
}


/*
 * Class:     com_qdreamer_qvoice_OggHelper
 * Method:    stop
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_OggHelper_stop
  (JNIEnv *env, jobject obj, jlong joggdec)
{
  qtk_jni_oggdec_t *oggdec;
  
  oggdec = (qtk_jni_oggdec_t *)joggdec;
  qtk_oggdec_stop(oggdec->oggdec);

  return (jint)0;
}


/*
 * Class:     com_qdreamer_qvoice_OggHelper
 * Method:    feed
 * Signature: (J[BII)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_OggHelper_feed
  (JNIEnv *env, jobject obj , jlong joggdec, jbyteArray jdata, jint jbytes, jint jis_end)
{

  qtk_jni_oggdec_t *oggdec;
  char *data;
  int ret;
  
  oggdec = (qtk_jni_oggdec_t *)joggdec;
	if( ((int)jbytes) > 0) {

		jbyte *sz = (jbyte*)(*env)->GetByteArrayElements(env,jdata,0);
		data=(char*)sz;
		if(!data) {
			ret = -1;
			goto end;
		}
		ret = qtk_oggdec_feed(oggdec->oggdec,data,(int)jbytes);
		(*env)->ReleaseByteArrayElements(env,jdata, sz, JNI_ABORT);
		if(ret != 0) {
			goto end;
		}
	}

	ret = 0;
end:
	return (jint)ret;
}

/*
 * Class:     com_qdreamer_qvoice_OggHelper
 * Method:    read
 * Signature: (J)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_qdreamer_qvoice_OggHelper_read
  (JNIEnv *env, jobject obj, jlong joggdec)
{
	qtk_jni_oggdec_t *oggdec;
	wtk_queue_node_t *qn;
	qtk_jni_msg_t *msg;
	jbyteArray jdata = NULL;

  oggdec = (qtk_jni_oggdec_t *)joggdec;

	qn = wtk_blockqueue_pop(&(oggdec->rlt_q),-1,NULL);
	if(!qn) {
		goto end;
	}
	msg = data_offset2(qn,qtk_jni_msg_t,q_n);

	jdata = (*env)->NewByteArray(env,msg->buf->pos);
	(*env)->SetByteArrayRegion(env,jdata,0,msg->buf->pos,(jbyte*)msg->buf->data);

	qtk_jni_push_msg(oggdec,msg);

end:
	return jdata;
}