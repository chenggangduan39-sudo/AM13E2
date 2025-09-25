#include "module.h"

static qtk_jni_msg_t* qtk_jni_msg_new(qtk_jni_module_t *module)
{
	qtk_jni_msg_t *msg;

	msg = (qtk_jni_msg_t*)wtk_malloc(sizeof(qtk_jni_msg_t));
	msg->buf = wtk_strbuf_new(module->step+16,1);
	return msg;
}

static int qtk_jni_msg_delete(qtk_jni_msg_t *msg)
{
	wtk_strbuf_delete(msg->buf);
	wtk_free(msg);
	return 0;
}

static qtk_jni_msg_t* qtk_jni_pop_msg(qtk_jni_module_t *module,qtk_jni_type_t type,char *data,int bytes)
{
	qtk_jni_msg_t *msg;

	msg = (qtk_jni_msg_t*)wtk_lockhoard_pop(&(module->msg_hoard));
	wtk_strbuf_reset(msg->buf);
	wtk_strbuf_push_c(msg->buf,type);
	wtk_strbuf_push(msg->buf,data,bytes);
	return msg;
}

static void qtk_jni_push_msg(qtk_jni_module_t *module,qtk_jni_msg_t *msg)
{
	wtk_lockhoard_push(&(module->msg_hoard),msg);
}

void qtk_jni_module_on_step_notify(qtk_jni_module_t *module,qtk_jni_type_t type,char *data,int bytes)
{
	qtk_jni_msg_t *msg = NULL;
	int pos;
	int n;

	pos = 0;
	while(pos < bytes) {
		n = min(module->step,bytes - pos);
		msg = qtk_jni_pop_msg(module,type,data+pos,n);
		wtk_blockqueue_push(&(module->rlt_q),&(msg->q_n));
		pos += n;
	}
}

void qtk_jni_module_on_notify(qtk_jni_module_t *module,qtk_var_t *var)
{
	qtk_jni_msg_t *msg = NULL;
	char tmp[64];
	int len;

//	wtk_debug("var type = %d  engine type = %d\n",var->type,engine->e->type);

	switch(var->type) {
	case QTK_SPEECH_START:
		msg = qtk_jni_pop_msg(module,QTK_JNI_SPEECH_START,0,0);
		break;
	case QTK_SPEECH_DATA_OGG:
		qtk_jni_module_on_step_notify(module,QTK_JNI_SPEECH_DATA_OGG,var->v.str.data,var->v.str.len);
		break;
	case QTK_SPEECH_DATA_PCM:
		qtk_jni_module_on_step_notify(module,QTK_JNI_SPEECH_DATA_PCM,var->v.str.data,var->v.str.len);
		break;
	case QTK_SPEECH_END:
		msg = qtk_jni_pop_msg(module,QTK_JNI_SPEECH_END,0,0);
		break;
	case QTK_AEC_WAKE:
		msg = qtk_jni_pop_msg(module,QTK_JNI_AEC_WAKE,0,0);
		break;
	case QTK_AEC_DIRECTION:
		len = sprintf(tmp,"{\"theta\":%d,\"phi\":%d}",var->v.ii.theta,var->v.ii.phi);
		msg = qtk_jni_pop_msg(module,QTK_JNI_AEC_DIRECTION,tmp,len);
		break;
	case QTK_AEC_WAKE_INFO:
		msg = qtk_jni_pop_msg(module,QTK_JNI_AEC_WAKE_INFO,var->v.str.data,var->v.str.len);
		break;
	case QTK_AEC_SLEEP:
		msg = qtk_jni_pop_msg(module,QTK_JNI_AEC_SLEEP,0,0);
		break;
	case QTK_AEC_CANCEL:
		msg = qtk_jni_pop_msg(module,QTK_JNI_AEC_CANCEL,0,0);
		break;
	case QTK_AEC_CANCEL_DATA:
		len = sprintf(tmp,"{\"cancelData\":%d}",var->v.i);
		msg = qtk_jni_pop_msg(module,QTK_JNI_AEC_CANCEL_DATA,tmp,len);
		break;
	case QTK_AEC_WAKE_ONESHOT:
		msg = qtk_jni_pop_msg(module,QTK_JNI_AEC_WAKE_ONESHOT,0,0);
		break;
	case QTK_AEC_WAKE_NORMAL:
		msg = qtk_jni_pop_msg(module,QTK_JNI_AEC_WAKE_NORMAL,0,0);
		break;
	case QTK_AEC_THETA_HINT:
		len = sprintf(tmp,"{\"theta\":%f,\"on\":%d}",var->v.fi.theta,var->v.fi.on);
		msg = qtk_jni_pop_msg(module,QTK_JNI_AEC_THETA_HINT,tmp,len);
		break;
	case QTK_AEC_THETA_BF_BG:
		len = sprintf(tmp,"{\"theta\":%f,\"on\":%d}",var->v.fi.theta,var->v.fi.on);
		msg = qtk_jni_pop_msg(module,QTK_JNI_AEC_THETA_BF_BG,tmp,len);
		break;
	case QTK_AEC_THETA_BG:
		len = sprintf(tmp,"{\"theta\":%f,\"on\":%d}",var->v.fi.theta,var->v.fi.on);
		msg = qtk_jni_pop_msg(module,QTK_JNI_AEC_THETA_BG,tmp,len);
		break;
	case QTK_TTS_START:
		msg = qtk_jni_pop_msg(module,QTK_JNI_TTS_START,0,0);
		break;
	case QTK_TTS_DATA:
		qtk_jni_module_on_step_notify(module,QTK_JNI_TTS_DATA,var->v.str.data,var->v.str.len);
		break;
	case QTK_TTS_END:
		msg = qtk_jni_pop_msg(module,QTK_JNI_TTS_END,0,0);
		break;
	case QTK_ASR_TEXT:
		msg = qtk_jni_pop_msg(module,QTK_JNI_ASR_DATA,var->v.str.data,var->v.str.len);
		break;
	case QTK_ASR_HINT:
		msg = qtk_jni_pop_msg(module,QTK_JNI_ASR_HINT,var->v.str.data,var->v.str.len);
		break;
	case QTK_ASR_HOTWORD:
		msg = qtk_jni_pop_msg(module,QTK_JNI_ASR_HOTWORD,var->v.str.data,var->v.str.len);
		break;
	case QTK_EVAL_TEXT:
		msg = qtk_jni_pop_msg(module,QTK_JNI_EVAL_DATA,var->v.str.data,var->v.str.len);
		break;
	case QTK_SEMDLG_TEXT:
		msg = qtk_jni_pop_msg(module,QTK_JNI_SEMDLG_DATA,var->v.str.data,var->v.str.len);
		break;
	case QTK_VAR_ERR:
		len = sprintf(tmp,"{\"errcode\":%d}",var->v.i);
		msg = qtk_jni_pop_msg(module,QTK_JNI_NORESPONSE,tmp,len);
		break;
	case QTK_ULTEVM_TYPE:
		len = sprintf(tmp,"{\"type\":%d}",var->v.i);
		msg = qtk_jni_pop_msg(module,QTK_JNI_ULTEVM_TYPE,tmp,len);		
		break;
	case QTK_VAR_SOURCE_AUDIO:
		qtk_jni_module_on_step_notify(module,QTK_JNI_SOURCE_AUDIO,var->v.str.data,var->v.str.len);
		break;
	case QTK_AUDIO_LEFT:
		msg = qtk_jni_pop_msg(module,QTK_AUDIO_LEFT,0,0);
		break;
	case QTK_AUDIO_ARRIVE:
		msg = qtk_jni_pop_msg(module,QTK_AUDIO_ARRIVE,0,0);
		break;
	case QTK_AUDIO_ERROR:
		msg = qtk_jni_pop_msg(module,QTK_AUDIO_ERROR,0,0);
		break;
	}

	if(msg) {
		wtk_blockqueue_push(&(module->rlt_q),&(msg->q_n));
	}
}

static void* qtk_jni_module_rcd_start(qtk_jni_module_t *module,qtk_session_t *session,char *name,int rate,int channel,int bytes_per_sample,int buf_time)
{
	int buf_size;
	int ret;

	if(!module->rcd_env) {
		(*(module->jvm))->AttachCurrentThread(module->jvm,&(module->rcd_env),NULL);
		if(!module->rcd_env) {
			ret = -1;
			goto end;
		}

		module->rcd_cls = module->cls;
		if(!module->rcd_cls) {
			ret = -1;
			goto end;
		}

		module->rcd_start = (*(module->rcd_env))->GetStaticMethodID(
				module->rcd_env,module->rcd_cls,"RecorderStart","(IIII)I");
		if(!module->rcd_start) {
			ret = -1;
			goto end;
		}

		module->rcd_stop = (*(module->rcd_env))->GetStaticMethodID(
				module->rcd_env,module->rcd_cls,"RecorderStop","()I");
		if(!module->rcd_stop) {
			ret = -1;
			goto end;
		}

		module->rcd_read = (*(module->rcd_env))->GetStaticMethodID(
				module->rcd_env,module->rcd_cls,"RecorderRead","([B)I");
		if(!module->rcd_read) {
			ret = -1;
			goto end;
		}

		if(!module->rcd_byte) {
			buf_size = rate * channel *bytes_per_sample * buf_time / 1000;
			module->rcd_byte = (*(module->rcd_env))->NewByteArray(
					module->rcd_env,buf_size);
		}
	}

	ret = (*(module->rcd_env))->CallStaticIntMethod(module->rcd_env,
			module->rcd_cls,module->rcd_start,rate,channel,bytes_per_sample,buf_time);
	if(ret != 0) {
		goto end;
	}

	ret = 0;
end:
	return ret==0 ? (void*)1 : NULL;
}


static int qtk_jni_module_rcd_read(qtk_jni_module_t *module,void *ths,char *buf,int bytes)
{
	int len;

	len = (*(module->rcd_env))->CallStaticIntMethod(module->rcd_env,
			module->rcd_cls,module->rcd_read,module->rcd_byte);
	if(len > 0) {
		(*(module->rcd_env))->GetByteArrayRegion(module->rcd_env,
				module->rcd_byte,0,len,(jbyte*)buf);
	} else {
		len = 0;
	}

	return len;
}

static int qtk_jni_module_rcd_stop(qtk_jni_module_t *module,void *ths)
{
	(*(module->rcd_env))->CallStaticIntMethod(module->rcd_env,
			module->rcd_cls,module->rcd_stop);
	return 0;
}

static int qtk_jni_module_rcd_clean(qtk_jni_module_t *module,void *ths)
{
	if(module->rcd_env) {
		if(module->rcd_byte) {
			(*(module->rcd_env))->DeleteLocalRef(module->rcd_env,module->rcd_byte);
			module->rcd_byte = NULL;
		}

		(*(module->jvm))->DetachCurrentThread(module->jvm);
		module->rcd_env = NULL;
	}

	return 0;
}

static void* qtk_jni_module_ply_start(qtk_jni_module_t *module,char *name,int rate,int channel,int bytes_per_sample,int buf_time)
{
	int buf_size;
	int ret;

	if(!module->ply_env) {
		(*(module->jvm))->AttachCurrentThread(module->jvm,&module->ply_env,NULL);
		if(!module->ply_env) {
			ret = -1;
			goto end;
		}

		module->ply_cls = module->cls;
		if(!module->ply_cls) {
			ret = -1;
			goto end;
		}

		module->ply_start = (*(module->ply_env))->GetStaticMethodID(
				module->ply_env,module->ply_cls,"PlayerStart","(IIII)I");
		if(!module->ply_start) {
			ret = -1;
			goto end;
		}

		module->ply_stop = (*(module->ply_env))->GetStaticMethodID(
				module->ply_env,module->ply_cls,"PlayerStop","()I");
		if(!module->ply_stop) {
			ret = -1;
			goto end;
		}

		module->ply_write = (*(module->ply_env))->GetStaticMethodID(
				module->ply_env,module->ply_cls,"PlayerWrite","([BI)I");
		if(!module->ply_write) {
			ret = -1;
			goto end;
		}

		if(!module->ply_byte) {
			buf_size = rate * channel * bytes_per_sample * buf_time / 1000;
			module->ply_byte = (*(module->ply_env))->NewByteArray(module->ply_env,buf_size);
		}
	}

	ret = (*(module->ply_env))->CallStaticIntMethod(module->ply_env,module->ply_cls,
			module->ply_start,rate,channel,bytes_per_sample,buf_time);
	if(ret != 0) {
		goto end;
	}

	ret = 0;
end:
	return ret==0 ? (void*)1 : NULL;
}

static int qtk_jni_module_ply_write(qtk_jni_module_t *module,void *ths,char *data,int bytes)
{
	int ret;

	(*(module->ply_env))->SetByteArrayRegion(module->ply_env,module->ply_byte,
			0,bytes,(jbyte*)data);
	ret = (*(module->ply_env))->CallStaticIntMethod(module->ply_env,module->ply_cls,
			module->ply_write,module->ply_byte,bytes);

	return ret;
}

static int qtk_jni_module_ply_stop(qtk_jni_module_t *module,void *ths)
{
	(*(module->ply_env))->CallStaticIntMethod(module->ply_env,module->ply_cls,
			module->ply_stop);
	return 0;
}

static int qtk_jni_module_ply_clean(qtk_jni_module_t *module,void *ths)
{
	if(module->ply_env) {
		if(module->ply_byte) {
			(*(module->ply_env))->DeleteLocalRef(module->ply_env,module->ply_byte);
			module->ply_byte = NULL;
		}
		(*(module->jvm))->DetachCurrentThread(module->jvm);
		module->ply_env = NULL;
	}
}

/*
 * Class:     com_qdreamer_qvoice_QModule
 * Method:    moduleNew
 * Signature: (JLjava/lang/String;Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_com_qdreamer_qvoice_QModule_moduleNew
  (JNIEnv *env, jobject obj, jlong jsession, jstring jres, jstring jcfg)
{
	qtk_jni_session_t *session;
	qtk_jni_module_t *module;
	char *res;
	char *cfg;
	int ret;

	session = (qtk_jni_session_t*)jsession;
	if(!session) {
		return (jlong)0;
	}

	module = (qtk_jni_module_t*)wtk_malloc(sizeof(qtk_jni_module_t));
	memset(module,0,sizeof(qtk_jni_module_t));

	res = (char*)(*env)->GetStringUTFChars(env,jres,NULL);
	cfg = (char*)(*env)->GetStringUTFChars(env,jcfg,NULL);
	module->module = qtk_module_new(session->session,res,cfg);
	module->log = session->session->log;

	(*env)->ReleaseStringUTFChars(env,jcfg,cfg);
	(*env)->ReleaseStringUTFChars(env,jres,res);

	if(!module->module) {
		ret = -1;
		goto end;
	}
	module->step = 8000;
	module->cache = 10;
	wtk_lockhoard_init(&module->msg_hoard,offsetof(qtk_jni_msg_t,hoard_n),module->cache,
			(wtk_new_handler_t)qtk_jni_msg_new,
			(wtk_delete_handler_t)qtk_jni_msg_delete,
			module);
	wtk_blockqueue_init(&module->rlt_q);
	qtk_module_set_notify(module->module,module,(qtk_engine_notify_f)qtk_jni_module_on_notify);

	(*env)->GetJavaVM(env,&module->jvm);
	module->cls = module_cls;

	// qtk_module_set_audio_callback(module->module,
	// 		module,
	// 		(qtk_recorder_start_func)qtk_jni_module_rcd_start,
	// 		(qtk_recorder_read_func)qtk_jni_module_rcd_read,
	// 		(qtk_recorder_stop_func)qtk_jni_module_rcd_stop,
	// 		(qtk_recorder_clean_func)qtk_jni_module_rcd_clean,
	// 		(qtk_player_start_func)qtk_jni_module_ply_start,
	// 		(qtk_player_write_func)qtk_jni_module_ply_write,
	// 		(qtk_player_stop_func)qtk_jni_module_ply_stop,
	// 		(qtk_player_clean_func)qtk_jni_module_ply_clean
	// 		);

	ret=0;
end:
	if(ret != 0) {
		wtk_free(module);
		module = NULL;
	}
	return (jlong)module;
}

/*
 * Class:     com_qdreamer_qvoice_QModule
 * Method:    moduleDel
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_qdreamer_qvoice_QModule_moduleDel
  (JNIEnv *env, jobject obj, jlong jmodule)
{
	qtk_jni_module_t *module;

	module = (qtk_jni_module_t*)jmodule;
	if(!module) {
		return;
	}
	qtk_module_delete(module->module);

	wtk_blockqueue_clean(&module->rlt_q);
	wtk_lockhoard_clean(&module->msg_hoard);

	wtk_free(module);
}

/*
 * Class:     com_qdreamer_qvoice_QModule
 * Method:    moduleStart
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_QModule_moduleStart
  (JNIEnv *env, jobject obj, jlong jmodule)
{
	qtk_jni_module_t *module;
	int ret;

	module = (qtk_jni_module_t*)jmodule;
	ret = qtk_module_start(module->module);
	return (jint)ret;
}

/*
 * Class:     com_qdreamer_qvoice_QModule
 * Method:    moduleStop
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_QModule_moduleStop
  (JNIEnv *env, jobject obj, jlong jmodule)
{
	qtk_jni_module_t *module;
	qtk_jni_msg_t *msg;
	wtk_queue_node_t *qn;
	int ret;

	module = (qtk_jni_module_t*)jmodule;
	ret = qtk_module_stop(module->module);

	wtk_blockqueue_wake(&module->rlt_q);
	wtk_msleep(50);

	while(1) {
		qn = wtk_blockqueue_pop(&module->rlt_q,0,NULL);
		if(!qn) {
			break;
		}
		msg = data_offset2(qn,qtk_jni_msg_t,q_n);
		qtk_jni_push_msg(module,msg);
	}

	return (jint)ret;
}

/*
 * Class:     com_qdreamer_qvoice_QModule
 * Method:    moduleFeed
 * Signature: (J[BI)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_QModule_moduleFeed
  (JNIEnv *env, jobject obj, jlong jmodule, jbyteArray jdata, jint jbytes)
{
	qtk_jni_module_t *module;
	char *data;
	int ret;

	module = (qtk_jni_module_t*)jmodule;
	if(((int)jbytes) > 0) {
		data = (char*)(*env)->GetByteArrayElements(env,jdata,0);
		if(!data) {
			ret = -1;
			goto end;
		}

		ret = qtk_module_feed(module->module,data,(int)jbytes);
		(*env)->ReleaseByteArrayElements(env,jdata,data,JNI_ABORT);
		if(ret != 0) {
			goto end;
		}
	} else {
		ret = -1;
		goto end;
	}

	ret = 0;
end:
	return (jint)ret;
}

/*
 * Class:     com_qdreamer_qvoice_QModule
 * Method:    moduleSet
 * Signature: (JLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_QModule_moduleSet
  (JNIEnv *env, jobject obj, jlong jmodule, jstring jparam)
{
	qtk_jni_module_t *module;
	char *param;
	int ret;

	module = (qtk_jni_module_t*)jmodule;
	param = (char*) (*env)->GetStringUTFChars(env,jparam,NULL);
	ret = qtk_module_set(module->module,param);
	(*env)->ReleaseStringUTFChars(env,jparam,param);

	return (jint)ret;
}

/*
 * Class:     com_qdreamer_qvoice_QModule
 * Method:    moduleRead
 * Signature: (J)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_qdreamer_qvoice_QModule_moduleRead
  (JNIEnv * env, jobject obj, jlong jmodule)
{
	qtk_jni_module_t *module;
	qtk_jni_msg_t *msg;
	wtk_queue_node_t *qn;
	jbyteArray ret = NULL;

	module = (qtk_jni_module_t*)jmodule;
	qn = wtk_blockqueue_pop(&module->rlt_q,-1,NULL);
	if(!qn) {
		goto end;
	}
	msg = data_offset2(qn,qtk_jni_msg_t,q_n);
	ret = (*env)->NewByteArray(env,msg->buf->pos);
	(*env)->SetByteArrayRegion(env,ret,0,msg->buf->pos,(jbyte*)msg->buf->data);
	qtk_jni_push_msg(module,msg);

end:
	return ret;
}
