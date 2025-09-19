#include "mulsv.h"
#include "wtk/core/wtk_alloc.h"
static qtk_jni_msg_t* qtk_jni_msg_new(qtk_jni_mul_t *mul)
{
	qtk_jni_msg_t *msg;

	msg = (qtk_jni_msg_t*)wtk_malloc(sizeof(qtk_jni_msg_t));
	msg->buf = wtk_strbuf_new(mul->step+16,1);
	return msg;
}

static int qtk_jni_msg_delete(qtk_jni_msg_t *msg)
{
	wtk_strbuf_delete(msg->buf);
	wtk_free(msg);
	return 0;
}

static int num=0;
static void print(void *ths, float conf,char *data,int len){
    if (conf>=0.4){
      ++num;
      //wtk_debug(">>>>>>>>>>>>>>>>.print%d\n",num);
    }
      
}

JNIEXPORT jlong JNICALL Java_com_qdreamer_qvoice_QMulsv_newMulsv
  (JNIEnv *env, jobject this, jstring jparams ,jint step ,jint cache)
{
    qtk_jni_mul_t *mul=(qtk_jni_mul_t*)wtk_malloc(sizeof(qtk_jni_mul_t));
    char *params = (char*) (*env)->GetStringUTFChars(env,jparams,NULL);
    mul->m=qtk_mulsv_api_new_bin(params);
    (*env)->ReleaseStringUTFChars(env,jparams,params);
    mul->step=(int)step;
    mul->cache=(int)cache;
    if (!mul->m){
      wtk_debug("mulsv new error!\n");
      wtk_free(mul);
      exit(0);
    }
    wtk_lockhoard_init(&mul->msg_hoard,offsetof(qtk_jni_msg_t,hoard_n),mul->step,
                      (wtk_new_handler_t)qtk_jni_msg_new,
                      (wtk_delete_handler_t)qtk_jni_msg_delete,
                      mul);
    qtk_mulsv_api_set_notify(mul->m,mul->m,print);
    return (jlong)mul;
}

JNIEXPORT void JNICALL Java_com_qdreamer_qvoice_QMulsv_startMulsv
  (JNIEnv *env, jobject this, jlong mulsv)
{
    qtk_jni_mul_t *mul=(qtk_jni_mul_t *)mulsv;
    qtk_mulsv_api_start(mul->m);
}

static char* jstring2string(JNIEnv* env, jstring jstr)
{
    char* rtn = NULL;
    jclass clsstring = (*env)->FindClass(env,"java/lang/String");
    jstring strencode = (*env)->NewStringUTF(env,"utf-8");
    jmethodID mid = (*env)->GetMethodID(env,clsstring, "getBytes", "(Ljava/lang/String;)[B");
    jbyteArray barr= (jbyteArray)(*env)->CallObjectMethod(env,jstr, mid, strencode);
    jsize alen = (*env)->GetArrayLength(env,barr);
    jbyte* ba = (*env)->GetByteArrayElements(env,barr, JNI_FALSE);
 
    if (alen > 0)
    {
        rtn = (char*)malloc(alen + 1);
        memcpy(rtn, ba, alen);
        rtn[alen] = 0;
    }
    (*env)->ReleaseByteArrayElements(env,barr, ba, 0);
    return rtn;
}

JNIEXPORT jlong JNICALL Java_com_qdreamer_qvoice_QMulsv_registerMulsv_1start
  (JNIEnv *env, jobject this, jlong mulsv,jstring name)
{
    qtk_jni_mul_t *mul=(qtk_jni_mul_t *)mulsv;
    char *s_name=jstring2string(env,name);
    qtk_mulsv_api_feed(mul->m,s_name,strlen(s_name),QTK_MULSV_API_DATA_TYPE_ENROLL_START,0);
    if (s_name!=NULL){
      free(s_name);
      s_name=NULL;
    }
    return (jlong)mul;
}

JNIEXPORT void JNICALL Java_com_qdreamer_qvoice_QMulsv_feedMulsv
  (JNIEnv *env, jobject this, jlong mulsv,jbyteArray jdata , jint jbytes)
{
  qtk_jni_mul_t *mul=(qtk_jni_mul_t *)mulsv;
  if ((int)jbytes>0){
      jbyte *sz =(jbyte*)(*env)->GetByteArrayElements(env,jdata,NULL);
      char *data=(char*)sz;
      if (data==NULL){
        return ;
      }
      qtk_mulsv_api_feed(mul->m,data,(int)jbytes,QTK_MULSV_API_DATA_TYPE_PCM,0);
      (*env)->ReleaseByteArrayElements(env,jdata, sz, JNI_ABORT);
  }
  else{
     qtk_mulsv_api_feed(mul->m,NULL,0,QTK_MULSV_API_DATA_TYPE_PCM,1);
  }
}

JNIEXPORT jlong JNICALL Java_com_qdreamer_qvoice_QMulsv_registerMulsv_1endl
  (JNIEnv *env, jobject this , jlong mulsv)
{
    qtk_jni_mul_t *mul=(qtk_jni_mul_t *)mulsv;
    qtk_mulsv_api_feed(mul->m,NULL,0,QTK_MULSV_API_DATA_TYPE_ENROLL_END,0);
    return (jlong)mul;
}

JNIEXPORT jlong JNICALL Java_com_qdreamer_qvoice_QMulsv_resetMulsv
  (JNIEnv *env, jobject this, jlong mulsv)
{
    qtk_jni_mul_t *mul=(qtk_jni_mul_t *)mulsv;
    qtk_mulsv_api_reset(mul->m);
    return (jlong)mul;
}

JNIEXPORT void JNICALL Java_com_qdreamer_qvoice_QMulsv_deleteMulsv
  (JNIEnv *env, jobject this, jlong mulsv)
{
    qtk_jni_mul_t *mul=(qtk_jni_mul_t *)mulsv;
    qtk_mulsv_api_delete_bin(mul->m);
    wtk_lockhoard_clean(&mul->msg_hoard);
    wtk_free(mul);
    mul=NULL;
}

JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_QMulsv_readMulsv
  (JNIEnv *env, jobject this)
{
    return num;
}

JNIEXPORT void JNICALL Java_com_qdreamer_qvoice_QMulsv_reset
(JNIEnv *env, jclass this)
{
    num=0;
}
