#include "session.h"
#ifdef USE_GPROF
#include "third/android-ndk-profiler-master/jni/prof.h"
#endif


static qtk_jni_msg_t* qtk_jni_msg_new(qtk_jni_session_t *session)
{
	qtk_jni_msg_t *msg;

	msg = (qtk_jni_msg_t*)wtk_malloc(sizeof(qtk_jni_msg_t));
	msg->buf = wtk_strbuf_new(512,1);
	return msg;
}

static int qtk_jni_msg_delete(qtk_jni_msg_t *msg)
{
	wtk_strbuf_delete(msg->buf);
	wtk_free(msg);
	return 0;
}

static qtk_jni_msg_t* qtk_jni_pop_msg(qtk_jni_session_t *session,qtk_jni_type_t type,char *data,int bytes)
{
	qtk_jni_msg_t *msg;

	msg = (qtk_jni_msg_t*)wtk_lockhoard_pop(&(session->msg_hoard));
	wtk_strbuf_reset(msg->buf);
	wtk_strbuf_push_c(msg->buf,type);
	wtk_strbuf_push(msg->buf,data,bytes);
	return msg;
}

static void qtk_jni_push_msg(qtk_jni_session_t *session,qtk_jni_msg_t *msg)
{
	wtk_lockhoard_push(&(session->msg_hoard),msg);
}

void qtk_jni_session_on_errcodenotify(qtk_jni_session_t *session,int errcode,char* errstr )
{
	qtk_jni_msg_t *msg = NULL;
	char tmp[512];
	int len;

	if(errcode == _QTK_SERVER_ERR) {
		len = snprintf(tmp,512,"{\"errID\":%d,\"errStr\":%s}}",errcode,errstr);
	} else {
		len = snprintf(tmp,512,"{\"errID\":%d,\"errStr\":\"%s\"}",errcode,errstr);
	}
	msg = qtk_jni_pop_msg(session,QTK_JNI_ERRCODE,tmp,len);
	if(msg) {
		wtk_blockqueue_push(&(session->rlt_q),&(msg->q_n));
	}
}

/*
 * Class:     com_qdreamer_qvoice_QSession
 * Method:    init
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_com_qdreamer_qvoice_QSession_init
  (JNIEnv *env, jclass cls, jstring jparams)
{
	qtk_jni_session_t *session;
	char *params;
	jmethodID usrid_handler = NULL;
	jstring jusrid;
	char *usrid;
	int ret;

#ifdef USE_GPROF
	 setenv("CPUPROFILE_FREQUENCY", "500", 1);
	 monstartup("libQvoice.so");
#endif

	session = (qtk_jni_session_t*)wtk_malloc(sizeof(qtk_jni_session_t));

	wtk_lockhoard_init(&(session->msg_hoard),offsetof(qtk_jni_msg_t,hoard_n),10,
			(wtk_new_handler_t)qtk_jni_msg_new,
			(wtk_delete_handler_t)qtk_jni_msg_delete,
			session
			);
	wtk_blockqueue_init(&(session->rlt_q));

	params = (char*)(*env)->GetStringUTFChars(env,jparams,NULL);
	session->session = qtk_session_new(params,QTK_WARN,session,(qtk_errcode_handler)qtk_jni_session_on_errcodenotify);
	(*env)->ReleaseStringUTFChars(env,jparams,params);

	(*env)->GetJavaVM(env,&session->jvm);
	session->cls = session_cls;

	usrid_handler = (*env)->GetStaticMethodID(
			env,session->cls,"UserIDGET","()Ljava/lang/String;");
	if(usrid_handler) {
		jusrid = (jstring)(*env)->CallStaticObjectMethod(env,session->cls,
				usrid_handler);
		if(jusrid) {
			usrid = (char*)(*env)->GetStringUTFChars(env,jusrid,NULL);
			wtk_log_log(session->session->log,"session usrid = %s\n",usrid);
			qtk_session_set_usrid(session->session,usrid,strlen(usrid));
			(*env)->ReleaseStringUTFChars(env,jusrid,usrid);
		} else {
			wtk_log_warn0(session->session->log,"get usrid failed");
		}
	} else {
		wtk_log_warn0(session->session->log,"get UsrIDGET method failed");
	}
	ret =qtk_session_check_option(session->session);
	if(ret != 0){
		qtk_session_delete(session->session);
		session = NULL;
		goto end;
	}
	qtk_session_start(session->session);
end:
	return (jlong)session;
}
JNIEXPORT jbyteArray JNICALL Java_com_qdreamer_qvoice_QSession_sessionRead
  (JNIEnv *env, jclass cls, jlong jsession)
{
	qtk_jni_session_t *session;
	wtk_queue_node_t *qn;
	qtk_jni_msg_t *msg;
	jbyteArray jdata = NULL;

	session = (qtk_jni_session_t*)jsession;

	qn = wtk_blockqueue_pop(&(session->rlt_q),-1,NULL);
	if(!qn) {
		goto end;
	}
	msg = data_offset2(qn,qtk_jni_msg_t,q_n);

	jdata = (*env)->NewByteArray(env,msg->buf->pos);
	(*env)->SetByteArrayRegion(env,jdata,0,msg->buf->pos,(jbyte*)msg->buf->data);

	qtk_jni_push_msg(session,msg);

end:
	return jdata;
}

/*
 * Class:     com_qdreamer_qvoice_QSession
 * Method:    exit
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_qdreamer_qvoice_QSession_exit
  (JNIEnv *env, jclass cls, jlong jsession)
{
	qtk_jni_session_t *session;
#ifdef USE_GPROF
	char path[256];
#endif

	session = (qtk_jni_session_t*)jsession;
#ifdef USE_GPROF
	snprintf(path,256,"%.*s/gmon.out",session->session->opt.cache_path->len,session->session->opt.cache_path->data);
#endif
	qtk_session_exit(session->session);
	wtk_blockqueue_wake(&(session->rlt_q));
	wtk_msleep(100);
	wtk_blockqueue_clean(&(session->rlt_q));
	wtk_lockhoard_clean(&(session->msg_hoard));
	wtk_free(session);

#ifdef USE_GPROF
	setenv("CPUPROFILE", path, 1);
	moncleanup();
#endif
}

