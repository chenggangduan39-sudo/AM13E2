#include "engine.h"
//JNI层引擎接口相关代码，主要围绕qtk_jni_on_notify函数服务，目前该函数主要适应康佳需求。
static qtk_jni_msg_t* qtk_jni_msg_new(qtk_jni_engine_t *engine)
{
	qtk_jni_msg_t *msg;

	msg = (qtk_jni_msg_t*)wtk_malloc(sizeof(qtk_jni_msg_t));
	msg->buf = wtk_strbuf_new(engine->step+16,1);
	return msg;
}

static int qtk_jni_msg_delete(qtk_jni_msg_t *msg)
{
	wtk_strbuf_delete(msg->buf);
	wtk_free(msg);
	return 0;
}

static qtk_jni_msg_t* qtk_jni_pop_msg(qtk_jni_engine_t *engine,qtk_jni_type_t type,char *data,int bytes)
{
	qtk_jni_msg_t *msg;

	msg = (qtk_jni_msg_t*)wtk_lockhoard_pop(&(engine->msg_hoard));
	wtk_strbuf_reset(msg->buf);
	wtk_strbuf_push_c(msg->buf,type);
	wtk_strbuf_push(msg->buf,data,bytes);
	return msg;
}

static void qtk_jni_push_msg(qtk_jni_engine_t *engine,qtk_jni_msg_t *msg)
{
	wtk_lockhoard_push(&(engine->msg_hoard),msg);
}

void qtk_jni_on_step_notify(qtk_jni_engine_t *engine,qtk_jni_type_t type,char *data,int bytes)
{
	qtk_jni_msg_t *msg;
	int pos;
	int n;

	pos = 0;
	while(pos < bytes) {
		n = min(engine->step,bytes - pos);
		msg = qtk_jni_pop_msg(engine,type,data+pos,n);
		wtk_blockqueue_push(&(engine->rlt_q),&(msg->q_n));
		pos += n;
	}
}

void qtk_jni_on_notify(qtk_jni_engine_t *engine,qtk_var_t *var)
{
	qtk_jni_msg_t *msg = NULL;
	char tmp[10240];
	int len;

	switch(var->type) {
	case QTK_SPEECH_START:
		len = sprintf(tmp,"{\"start\":%f,\"count\":%d}",var->v.fi.theta,var->v.fi.on);
		msg = qtk_jni_pop_msg(engine,QTK_JNI_SPEECH_START,tmp,len);
		break;
	case QTK_SPEECH_DATA_OGG:
		qtk_jni_on_step_notify(engine,QTK_JNI_SPEECH_DATA_OGG,var->v.str.data,var->v.str.len);
		break;
	case QTK_SPEECH_DATA_PCM:
		qtk_jni_on_step_notify(engine,QTK_JNI_SPEECH_DATA_PCM,var->v.str.data,var->v.str.len);
		break;
	case QTK_SPEECH_END://3
		len = sprintf(tmp,"{\"end\":%f,\"count\":%d}",var->v.fi.theta,var->v.fi.on);
		msg = qtk_jni_pop_msg(engine,QTK_JNI_SPEECH_END,tmp,len);
		break;
	case QTK_AEC_WAKE://4
#if 0
		len = sprintf(tmp,"{\"wake_energy\":%f,\"wake_prob\":%f,\"set_wake_prob\":%f,\"counter\":%ld}",
					var->v.ff.energy,var->v.ff.ff,var->v.ff.fff,var->v.ff.counter);
		msg = qtk_jni_pop_msg(engine,QTK_JNI_AEC_WAKE,tmp,len);
#else
		msg = qtk_jni_pop_msg(engine,QTK_JNI_AEC_WAKE,0,0);
#endif
		break;
    case QTK_AEC_DIRECTION:
		len = sprintf(tmp,"{\"wake_theta\":%d}",var->v.ii.theta);
		msg = qtk_jni_pop_msg(engine,QTK_JNI_AEC_DIRECTION,tmp,len);
		break;
	case QTK_AEC_WAKE_INFO:
		msg = qtk_jni_pop_msg(engine,QTK_JNI_AEC_WAKE_INFO,var->v.str.data,var->v.str.len);
		break;
	case QTK_AEC_SLEEP:
		msg = qtk_jni_pop_msg(engine,QTK_JNI_AEC_SLEEP,0,0);
		break;
	case QTK_AEC_CANCEL:
		msg = qtk_jni_pop_msg(engine,QTK_JNI_AEC_CANCEL,0,0);
		break;
	case QTK_AEC_CANCEL_DATA:
		len = sprintf(tmp,"{\"cancelData\":%d}",var->v.i);
		msg = qtk_jni_pop_msg(engine,QTK_JNI_AEC_CANCEL_DATA,tmp,len);
		break;
	case QTK_AEC_WAKE_ONESHOT:
		msg = qtk_jni_pop_msg(engine,QTK_JNI_AEC_WAKE_ONESHOT,0,0);
		break;
	case QTK_AEC_WAKE_NORMAL:
		msg = qtk_jni_pop_msg(engine,QTK_JNI_AEC_WAKE_NORMAL,0,0);
		break;
	case QTK_AEC_THETA_HINT:
		len = sprintf(tmp,"{\"theta\":%f,\"on\":%d}",var->v.fi.theta,var->v.fi.on);
		msg = qtk_jni_pop_msg(engine,QTK_JNI_AEC_THETA_HINT,tmp,len);
		break;
	case QTK_AEC_THETA_BF_BG:
		len = sprintf(tmp,"{\"theta\":%f,\"on\":%d}",var->v.fi.theta,var->v.fi.on);
		msg = qtk_jni_pop_msg(engine,QTK_JNI_AEC_THETA_BF_BG,tmp,len);
		break;
	case QTK_AEC_THETA_BG:
		len = sprintf(tmp,"{\"theta\":%f,\"on\":%d}",var->v.fi.theta,var->v.fi.on);
		msg = qtk_jni_pop_msg(engine,QTK_JNI_AEC_THETA_BG,tmp,len);
		break;
	case QTK_TTS_START:
		msg = qtk_jni_pop_msg(engine,QTK_JNI_TTS_START,0,0);
		break;
	case QTK_TTS_DATA:
		qtk_jni_on_step_notify(engine,QTK_JNI_TTS_DATA,var->v.str.data,var->v.str.len);
		break;
	case QTK_TTS_END:
		msg = qtk_jni_pop_msg(engine,QTK_JNI_TTS_END,0,0);
		break;
	case QTK_ASR_TEXT://20
        len = sprintf(tmp, "{\"result\":%.*s}", var->v.str.len,
                              var->v.str.data);
                //wtk_log_log(engine->e->session->log,"native json==%.*s",len,tmp);
		//wtk_debug("native json==%.*s",len,tmp);
		msg = qtk_jni_pop_msg(engine,QTK_JNI_ASR_DATA,tmp,len);
		break;
	case QTK_ASR_HINT://21
        len = sprintf(tmp, "{\"result\":%.*s}", var->v.str.len,
                              var->v.str.data);
                //wtk_log_log(engine->e->session->log,"native json==%.*s",len,tmp);
		//wtk_debug("native json==%.*s",len,tmp);
		msg = qtk_jni_pop_msg(engine,QTK_JNI_ASR_HINT,tmp,len);
		break;
	case QTK_ASR_HOTWORD:
		msg = qtk_jni_pop_msg(engine,QTK_JNI_ASR_HOTWORD,var->v.str.data,var->v.str.len);
		break;
	case QTK_EVAL_TEXT:
		msg = qtk_jni_pop_msg(engine,QTK_JNI_EVAL_DATA,var->v.str.data,var->v.str.len);
		break;
	case QTK_SEMDLG_TEXT:
		msg = qtk_jni_pop_msg(engine,QTK_JNI_SEMDLG_DATA,var->v.str.data,var->v.str.len);
		break;
	case QTK_CONSIST_MICERR_NIL:
		len = sprintf(tmp,"{\"channel\":%d}",var->v.i);
		msg = qtk_jni_pop_msg(engine,QTK_JNI_CONSIST_MICERR_NIL,tmp,len);
		break;
    case QTK_CONSIST_MICERR_ALIGN:
		len = sprintf(tmp,"{\"channel\":%d}",var->v.i);
		msg = qtk_jni_pop_msg(engine,QTK_JNI_CONSIST_MICERR_ALIGN,tmp,len);
		break;
    case QTK_CONSIST_MICERR_MAX:
		len = sprintf(tmp,"{\"channel\":%d}",var->v.i);
		msg = qtk_jni_pop_msg(engine,QTK_JNI_CONSIST_MICERR_MAX,tmp,len);
		break;
    case QTK_CONSIST_MICERR_CORR:
		len = sprintf(tmp,"{\"channel\":%d}",var->v.i);
		msg = qtk_jni_pop_msg(engine,QTK_JNI_CONSIST_MICERR_CORR,tmp,len);
		break;
    case QTK_CONSIST_MICERR_ENERGY:
		len = sprintf(tmp,"{\"channel\":%d}",var->v.i);
		msg = qtk_jni_pop_msg(engine,QTK_JNI_CONSIST_MICERR_ENERGY,tmp,len);
		break;
    case QTK_CONSIST_SPKERR_NIL:
		len = sprintf(tmp,"{\"channel\":%d}",var->v.i);
		msg = qtk_jni_pop_msg(engine,QTK_JNI_CONSIST_SPKERR_NIL,tmp,len);
		break;
    case QTK_CONSIST_SPKERR_ALIGN:
		len = sprintf(tmp,"{\"channel\":%d}",var->v.i);
		msg = qtk_jni_pop_msg(engine,QTK_JNI_CONSIST_SPKERR_ALIGN,tmp,len);
		break;
    case QTK_CONSIST_SPKERR_MAX:
		len = sprintf(tmp,"{\"channel\":%d}",var->v.i);
		msg = qtk_jni_pop_msg(engine,QTK_JNI_CONSIST_SPKERR_MAX,tmp,len);
		break;
    case QTK_CONSIST_SPKERR_CORR:
		len = sprintf(tmp,"{\"channel\":%d}",var->v.i);
		msg = qtk_jni_pop_msg(engine,QTK_JNI_CONSIST_SPKERR_CORR,tmp,len);
		break;
    case QTK_CONSIST_SPKERR_ENERGY:
		len = sprintf(tmp,"{\"channel\":%d}",var->v.i);
		msg = qtk_jni_pop_msg(engine,QTK_JNI_CONSIST_SPKERR_ENERGY,tmp,len);
		break;
	case QTK_ULTGESTURE_TYPE:
		len = sprintf(tmp,"{\"type\":%d}",var->v.i);
		msg = qtk_jni_pop_msg(engine,QTK_JNI_ULTGESTURE_TYPE,tmp,len);
		break;
	case QTK_ULTEVM_TYPE:
		len = sprintf(tmp,"{\"type\":%d}",var->v.i);
		msg = qtk_jni_pop_msg(engine,QTK_JNI_ULTEVM_TYPE,tmp,len);
		break;
	case QTK_AUDIO_RANGE_IDX:
		len = sprintf(tmp,"{\"type\":%d}",var->v.i);
		msg = qtk_jni_pop_msg(engine,QTK_JNI_AUDIO_RANGE_IDX,tmp,len);
		break;
	case QTK_VAR_ERR:
		len = sprintf(tmp,"{\"errcode\":%d}",var->v.i);
		msg = qtk_jni_pop_msg(engine,QTK_JNI_NORESPONSE,tmp,len);
		break;
	default:
		break;
	}

	if(msg) {
		wtk_blockqueue_push(&(engine->rlt_q),&(msg->q_n));
	}
}

/*
 * Class:     com_qdreamer_qvoice_QEngine
 * Method:    newEngine
 * Signature: (JLjava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_com_qdreamer_qvoice_QEngine_newEngine
  (JNIEnv *env, jobject obj, jlong jsession, jstring jparams)
{
	qtk_jni_session_t *session;
	qtk_jni_engine_t *engine;
	char *params;
	int ret=-1;

	engine = (qtk_jni_engine_t*)wtk_malloc(sizeof(qtk_jni_engine_t));

	session = (qtk_jni_session_t*)jsession;
	params = (char*) (*env)->GetStringUTFChars(env,jparams,NULL);
	engine->e = qtk_engine_new(session->session,params);
	(*env)->ReleaseStringUTFChars(env,jparams,params);
	if(!engine->e) {
		ret = -1;
		goto end;
	}
	engine->step = 8192;
	engine->cache = 20;

	wtk_lockhoard_init(&(engine->msg_hoard),offsetof(qtk_jni_msg_t,hoard_n),engine->cache,
			(wtk_new_handler_t)qtk_jni_msg_new,
			(wtk_delete_handler_t)qtk_jni_msg_delete,
			engine
			);
	wtk_blockqueue_init(&(engine->rlt_q));
	qtk_engine_set_notify(engine->e,engine,(qtk_engine_notify_f)qtk_jni_on_notify);

	ret = 0;
end:
	if(ret != 0) {
		wtk_free(engine);
		engine = NULL;
	}
	return (jlong)engine;
}

/*
 * Class:     com_qdreamer_qvoice_QEngine
 * Method:    startEngine
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_QEngine_startEngine
  (JNIEnv *env, jobject obj, jlong jengine)
{
	qtk_jni_engine_t *engine;
	int ret;

	engine = (qtk_jni_engine_t*)jengine;
	ret = qtk_engine_start(engine->e);

	return (jint)ret;
}

JNIEXPORT jbyteArray JNICALL Java_com_qdreamer_qvoice_QEngine_extractVector
  (JNIEnv *env, jobject obj, jlong jengine, jbyteArray jdata, jint jbytes, jint jis_end)
{
    /*qtk_jni_engine_t *engine=(qtk_jni_engine_t*)jengine;
    char *data=NULL;
    char vector_data[1024]={0};
    jbyteArray ret_data = NULL;

    if(jbytes>0){
            data = (char*)(*env)->GetByteArrayElements(env,jdata,0);
            if(!data) return ret_data;
    }
    qtk_engine_extract_vector(engine->e,data,jbytes,jis_end,(float*)vector_data);

    if(jbytes>0)(*env)->ReleaseByteArrayElements(env,jdata, data, JNI_ABORT);
    if(jis_end){
            ret_data = (*env)->NewByteArray(env,1024);
            (*env)->SetByteArrayRegion(env,ret_data,0,1024,(jbyte*)vector_data);
    }

    return ret_data;*/
}

JNIEXPORT jfloat JNICALL Java_com_qdreamer_qvoice_QEngine_vectorCompare
  (JNIEnv *env, jobject obj, jlong jengine, jbyteArray jdata1, jbyteArray jdata2)
{
    /*qtk_jni_engine_t *engine=(qtk_jni_engine_t*)jengine;
    char* vector_data1=NULL;
    char* vector_data2=NULL;

    vector_data1 = (char*)(*env)->GetByteArrayElements(env,jdata1,0);
    vector_data2 = (char*)(*env)->GetByteArrayElements(env,jdata2,0);
    float ret =
    qtk_engine_vector_compare(engine->e,(float*)vector_data1,(float*)vector_data2);
    (*env)->ReleaseByteArrayElements(env,jdata1, vector_data1, JNI_ABORT);
    (*env)->ReleaseByteArrayElements(env,jdata2, vector_data2, JNI_ABORT);

    return ret;*/
}

/*
 * Class:     com_qdreamer_qvoice_QEngine
 * Method:    feedEngine
 * Signature: (J[BII)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_QEngine_feedEngine
  (JNIEnv *env, jobject obj, jlong jengine, jbyteArray jdata, jint jbytes, jint jis_end)
{
	qtk_jni_engine_t *engine;
	char *data;
	int ret;

	engine = (qtk_jni_engine_t*)jengine;
	if( ((int)jbytes) > 0) {
		data = (char*)(*env)->GetByteArrayElements(env,jdata,0);
		if(!data) {
			ret = -1;
			goto end;
		}

		ret = qtk_engine_feed(engine->e,data,(int)jbytes,(int)jis_end);
		(*env)->ReleaseByteArrayElements(env,jdata, data, JNI_ABORT);
		if(ret != 0) {
			goto end;
		}
	} else if( (int)jis_end ) {
		ret = qtk_engine_feed(engine->e,0,0,1);
		if(ret != 0) {
			goto end;
		}
	}

	ret = 0;
end:
	return (jint)ret;
}

/*
 * Class:     com_qdreamer_qvoice_QEngine
 * Method:    cancelEngine
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_QEngine_cancelEngine
  (JNIEnv *env, jobject obj, jlong jengine)
{
	qtk_jni_engine_t *engine;
	int ret;

	engine = (qtk_jni_engine_t*)jengine;
	ret = qtk_engine_cancel(engine->e);

	return (jint)ret;
}

/*
 * Class:     com_qdreamer_qvoice_QEngine
 * Method:    setEngine
 * Signature: (JLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_QEngine_setEngine
  (JNIEnv *env, jobject obj, jlong jengine, jstring jparams)
{
	qtk_jni_engine_t *engine;
	char *params;
	int ret;

	engine = (qtk_jni_engine_t*)jengine;

	params = (char*) (*env)->GetStringUTFChars(env,jparams,NULL);
	ret = qtk_engine_set(engine->e,params);
	(*env)->ReleaseStringUTFChars(env,jparams,params);

	return (jint)ret;
}

#ifdef USE_ASR
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_QEngine_updateCmds
  (JNIEnv *env, jobject obj, jlong jengine, jstring jparams)
{
	qtk_jni_engine_t *engine=(qtk_jni_engine_t*)jengine;
	
	char *params=(char*) (*env)->GetStringUTFChars(env,jparams,NULL);
	int ret = qtk_engine_update_cmds(engine->e,params);
	(*env)->ReleaseStringUTFChars(env,jparams,params);

	return ret;
}
#endif

////////////////////////////////////////////////////////////////////
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_QEngine_setParamsEngine
  (JNIEnv *env, jobject obj, jlong jengine, jint jparams,jfloat jval)
{
    /*qtk_jni_engine_t *engine;
    int ret;

    engine = (qtk_jni_engine_t*)jengine;
    ret = qtk_engine_set_params(engine->e,(int)jparams,(float)jval);

    return (jint)ret;*/
}
////////////////////////////////////////////////////////////////////
/*
 * Class:     com_qdreamer_qvoice_QEngine
 * Method:    resetEngine
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_QEngine_resetEngine
  (JNIEnv *env, jobject obj, jlong jengine)
{
	qtk_jni_engine_t *engine;
	int ret;

	engine = (qtk_jni_engine_t*)jengine;
	ret = qtk_engine_reset(engine->e);

	return (jint)ret;
}

/*
 * Class:     com_qdreamer_qvoice_QEngine
 * Method:    deleteEngine
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_QEngine_deleteEngine
  (JNIEnv *env, jobject obj, jlong jengine)
{
	qtk_jni_engine_t *engine;

	engine = (qtk_jni_engine_t*)jengine;
	qtk_engine_delete(engine->e);

	wtk_blockqueue_wake(&(engine->rlt_q));
	wtk_msleep(100);
	wtk_blockqueue_clean(&(engine->rlt_q));
	wtk_lockhoard_clean(&(engine->msg_hoard));
	wtk_free(engine);

	return (jint)0;
}

/*
 * Class:     com_qdreamer_qvoice_QEngine
 * Method:    readEngine
 * Signature: (J)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_qdreamer_qvoice_QEngine_readEngine
  (JNIEnv *env, jobject obj, jlong jengine)
{
	qtk_jni_engine_t *engine;
	wtk_queue_node_t *qn;
	qtk_jni_msg_t *msg;
	jbyteArray jdata = NULL;

	engine = (qtk_jni_engine_t*)jengine;

	qn = wtk_blockqueue_pop(&(engine->rlt_q),-1,NULL);
	if(!qn) {
		goto end;
	}
	msg = data_offset2(qn,qtk_jni_msg_t,q_n);

	jdata = (*env)->NewByteArray(env,msg->buf->pos);
	(*env)->SetByteArrayRegion(env,jdata,0,msg->buf->pos,(jbyte*)msg->buf->data);

	qtk_jni_push_msg(engine,msg);

end:
	return jdata;
}

/*
 * Class:     com_qdreamer_qvoice_QEngine
 * Method:    getResultEngine
 * Signature: (J)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_qdreamer_qvoice_QEngine_getResultEngine
  (JNIEnv *env, jobject obj, jlong jengine)
{
	qtk_jni_engine_t *engine;
	jbyteArray jdata = NULL;

	engine = (qtk_jni_engine_t*)jengine;
	qtk_var_t vresult;

	qtk_engine_get_result(engine->e, &vresult);
	if(vresult.v.str.len > 0)
	{
		jdata = (*env)->NewByteArray(env,vresult.v.str.len);
		(*env)->SetByteArrayRegion(env,jdata,0,vresult.v.str.len,(jbyte*)(vresult.v.str.data));
	}

	return jdata;
}

/*
 * Class:     com_qdreamer_qvoice_QEngine
 * Method:    getFnEngine
 * Signature: (J)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_qdreamer_qvoice_QEngine_getFnEngine
  (JNIEnv *env, jobject obj, jlong jengine)
{
	qtk_jni_engine_t *engine;
	jbyteArray jdata = NULL;

	engine = (qtk_jni_engine_t*)jengine;

	char *fn=qtk_engine_get_fn(engine->e);
	jdata = (*env)->NewByteArray(env,strlen(fn));
	(*env)->SetByteArrayRegion(env,jdata,0,strlen(fn),(jbyte*)fn);

	return jdata;
}

/*
 * Class:     com_qdreamer_qvoice_QEngine
 * Method:    getProbEngine
 * Signature: (J)[B
 */
JNIEXPORT jfloat JNICALL Java_com_qdreamer_qvoice_QEngine_getProbEngine
  (JNIEnv *env, jobject obj, jlong jengine)
{
	qtk_jni_engine_t *engine;

	engine = (qtk_jni_engine_t*)jengine;

	return qtk_engine_get_prob(engine->e);
}
