#include "audio.h"

typedef struct {
	wtk_queue_node_t q_n;
	wtk_strbuf_t *buf;
}qtk_jni_audio_rcd_t;

static void* qtk_jni_audio_rcd_start(qtk_jni_audio_t *audio,char *name,int rate,int channel,int bytes_per_sample,int buf_time)
{
	int buf_size;
	int ret;
	if(!audio->rcd_env) {
		(*(audio->jvm))->AttachCurrentThread(audio->jvm,&(audio->rcd_env),NULL);
		if(!audio->rcd_env) {
			ret = -1;
			goto end;
		}

		audio->rcd_cls = audio->cls;
		if(!audio->rcd_cls) {
			ret = -1;
			goto end;
		}

		audio->rcd_start = (*(audio->rcd_env))->GetStaticMethodID(
				audio->rcd_env,audio->rcd_cls,"RecorderStart","(IIII)I");
		if(!audio->rcd_start) {
			ret = -1;
			goto end;
		}

		audio->rcd_stop = (*(audio->rcd_env))->GetStaticMethodID(
				audio->rcd_env,audio->rcd_cls,"RecorderStop","()I");
		if(!audio->rcd_stop) {
			ret = -1;
			goto end;
		}

		audio->rcd_read = (*(audio->rcd_env))->GetStaticMethodID(
				audio->rcd_env,audio->rcd_cls,"RecorderRead","([B)I");
		if(!audio->rcd_read) {
			ret = -1;
			goto end;
		}

		if(!audio->rcd_byte) {
			buf_size = rate * 2 * buf_time / 1000;
			audio->rcd_byte = (*(audio->rcd_env))->NewByteArray(
					audio->rcd_env,buf_size);
		}
	}

	ret = (*(audio->rcd_env))->CallStaticIntMethod(audio->rcd_env,
			audio->rcd_cls,audio->rcd_start,rate,channel,bytes_per_sample,buf_time);

	ret = 0;
end:
	return ret == 0?(void*)1:NULL;
}

static int qtk_jni_audio_rcd_read(qtk_jni_audio_t *audio,void *ths,char *buf,int bytes)
{
	int len;

	len = (*(audio->rcd_env))->CallStaticIntMethod(audio->rcd_env,
			audio->rcd_cls,audio->rcd_read,audio->rcd_byte);
	if(len > 0) {
		(*(audio->rcd_env))->GetByteArrayRegion(audio->rcd_env,
				audio->rcd_byte,0,len,(jbyte*)buf);
	} else {
		len = 0;
	}

	return len;
}

static int qtk_jni_audio_rcd_stop(qtk_jni_audio_t *audio,void *ths)
{
	(*(audio->rcd_env))->CallStaticIntMethod(audio->rcd_env,
			audio->rcd_cls,audio->rcd_stop);
	return 0;
}

static int qtk_jni_audio_rcd_clean(qtk_jni_audio_t *audio,void *ths)
{
	if(audio->rcd_env) {
		if(audio->rcd_byte) {
			(*(audio->rcd_env))->DeleteLocalRef(audio->rcd_env,audio->rcd_byte);
			audio->rcd_byte = NULL;
		}

		(*(audio->jvm))->DetachCurrentThread(audio->jvm);
		audio->rcd_env = NULL;
	}

	return 0;
}

static void* qtk_jni_audio_ply_start(qtk_jni_audio_t *audio,char *name,int rate,
		int channel,int bytes_per_sample,int buf_time,int peroid_time)
{
	int buf_size;
	int ret;

	if(!audio->ply_env) {
		(*(audio->jvm))->AttachCurrentThread(audio->jvm,&(audio->ply_env),NULL);
		if(!audio->ply_env) {
			ret = -1;
			goto end;
		}
		audio->ply_cls = audio->cls;
		if(!audio->ply_cls) {
			ret = -1;
			goto end;
		}
		audio->ply_start = (*(audio->ply_env))->GetStaticMethodID(
				audio->ply_env,audio->ply_cls,"PlayerStart","(IIII)I");
		if(!audio->ply_start) {
			ret = -1;
			goto end;
		}
		audio->ply_write = (*(audio->ply_env))->GetStaticMethodID(
				audio->ply_env,audio->ply_cls,"PlayerWrite","([BI)I");
		if(!audio->ply_write) {
			ret = -1;
			goto end;
		}
		audio->ply_stop = (*(audio->ply_env))->GetStaticMethodID(
				audio->ply_env,audio->ply_cls,"PlayerStop","()I");
		if(!audio->ply_stop) {
			ret = -1;
			goto end;
		}
		if(!audio->ply_byte) {
			buf_size = rate * channel * 2 * buf_time / 1000;
			audio->ply_byte = (*(audio->ply_env))->NewByteArray(audio->ply_env,buf_size);
		}
	}
	ret = (*(audio->ply_env))->CallStaticIntMethod(audio->ply_env,audio->ply_cls,
			audio->ply_start,rate,channel,bytes_per_sample,buf_time);
end:
	return ret==0?(void*)1:NULL;
}

static int qtk_jni_audio_ply_write(qtk_jni_audio_t *audio,void *ths,char *data,int bytes)
{
	int ret;
	(*(audio->ply_env))->SetByteArrayRegion(audio->ply_env,audio->ply_byte,
			0,bytes,(jbyte*)data);
	ret = (*(audio->ply_env))->CallStaticIntMethod(audio->ply_env,audio->ply_cls,
			audio->ply_write,audio->ply_byte,bytes);
	return ret;
}

static int qtk_jni_audio_ply_stop(qtk_jni_audio_t *audio,void *ths)
{
	(*(audio->ply_env))->CallStaticIntMethod(audio->ply_env,audio->ply_cls,
			audio->ply_stop);
	return 0;
}

static int qtk_jni_audio_ply_clean(qtk_jni_audio_t *audio,void *ths)
{
	if(audio->ply_env) {
		if(audio->ply_byte) {
			(*(audio->ply_env))->DeleteLocalRef(audio->ply_env,audio->ply_byte);
			audio->ply_byte = NULL;
		}
		(*(audio->jvm))->DetachCurrentThread(audio->jvm);
		audio->ply_env = NULL;
	}

	return  0;
}

static void qtk_jni_audio_on_record(qtk_jni_audio_t *audio,char *data,int bytes)
{
	qtk_jni_audio_rcd_t *rcd;
	wtk_queue_node_t *qn;

	//应当写文档，不要让上层有多余的判断
	if(bytes <= 0) {
		wtk_log_log0(audio->audio->session->log,"jni_notify bytes<=0");
		return;
	}

	if(audio->linux_wav){
		wtk_wavfile_write(audio->linux_wav,data,bytes);
	}
	//先从空闲队列弹出节点存储新数据
	qn = wtk_blockqueue_pop(&audio->cache_q,0,NULL);
	//若后续拿数据(同步)太慢导致没有空闲节点，则直接将数据放在就绪队列中
	//一定会导致音频数据丢帧
	if(!qn) {
		wtk_log_log0(audio->audio->session->log,"no cache node,abandon audio node data");
		qn = wtk_blockqueue_pop(&audio->audio_q,-1,NULL);
	}
	rcd = data_offset2(qn,qtk_jni_audio_rcd_t,q_n);
	wtk_strbuf_reset(rcd->buf);
	wtk_strbuf_push(rcd->buf,data,bytes);

	wtk_blockqueue_push(&audio->audio_q,&rcd->q_n);
}

static qtk_jni_audio_rcd_t* qtk_jni_audio_rcd_new(int buf_size)
{
	qtk_jni_audio_rcd_t *rcd;

	rcd = (qtk_jni_audio_rcd_t*)wtk_malloc(sizeof(qtk_jni_audio_rcd_t));
	rcd->buf = wtk_strbuf_new(buf_size,1);
	return rcd;
}

static void qtk_jni_audio_rcd_delete(qtk_jni_audio_rcd_t *rcd)
{
	wtk_strbuf_delete(rcd->buf);
	wtk_free(rcd);
}


/*
 * Class:     com_qdreamer_qvoice_QAudio
 * Method:    init
 * Signature: (JLjava/lang/String;)
 */
JNIEXPORT jlong JNICALL Java_com_qdreamer_qvoice_QAudio_init
  (JNIEnv *env, jclass cls, jlong jsession, jstring jcfg_fn)
{
	qtk_jni_audio_t *audio;
	qtk_jni_session_t *session;
	qtk_audio_cfg_t *cfg;
	qtk_jni_audio_rcd_t *rcd;
	char *cfg_fn;
	int buf_time;
	int buf_size;
	int ret;
	int i;

	audio = (qtk_jni_audio_t*)wtk_malloc(sizeof(qtk_jni_audio_t));
	memset(audio,0,sizeof(qtk_jni_audio_t));

	cfg_fn = (char*) (*env)->GetStringUTFChars(env,jcfg_fn,NULL);
	audio->cfg = wtk_main_cfg_new_type(qtk_audio_cfg,cfg_fn);
	(*env)->ReleaseStringUTFChars(env,jcfg_fn,cfg_fn);

	if(!audio->cfg) {
		ret = -1;
		goto end;
	}

	session = (qtk_jni_session_t*)jsession;
	if(!session) {
		ret = -1;
		goto end;
	}

	cfg = (qtk_audio_cfg_t*)audio->cfg->cfg;
	audio->audio = qtk_audio_new(cfg,session->session);
	if(!audio->audio) {
		ret = -1;
		goto end;
	}

	(*env)->GetJavaVM(env, &(audio->jvm));
	audio->cls = audio_cls;
#if 0
	qtk_audio_set_callback(audio->audio,
			audio,
			(qtk_recorder_start_func) qtk_jni_audio_rcd_start,
			(qtk_recorder_read_func)  qtk_jni_audio_rcd_read,
			(qtk_recorder_stop_func)  qtk_jni_audio_rcd_stop,
			(qtk_recorder_clean_func) qtk_jni_audio_rcd_clean,
			(qtk_player_start_func)   qtk_jni_audio_ply_start,
			(qtk_player_write_func)   qtk_jni_audio_ply_write,
			(qtk_player_stop_func)    qtk_jni_audio_ply_stop,
			(qtk_player_clean_func)   qtk_jni_audio_ply_clean
			);
#endif
	wtk_debug("QAudio_init================>>>>>>>>>>>>>>ok\n");
	ret = 0;
end:
	if(ret != 0) {
		if(audio->audio) {
			qtk_audio_delete(audio->audio);
		}
		if(audio->cfg) {
			wtk_main_cfg_delete(audio->cfg);
		}
		wtk_free(audio);
		audio = NULL;
	}
	return (jlong)audio;
}

/*
 * Class:     com_qdreamer_qvoice_QAudio
 * Method:    exit
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_QAudio_exit
  (JNIEnv *env, jclass cls, jlong jaudio)
{
	qtk_jni_audio_t *audio;

	audio = (qtk_jni_audio_t*)jaudio;
	if(audio->audio) {
		qtk_audio_delete(audio->audio);
	}
	if(audio->cfg) {
		wtk_main_cfg_delete(audio->cfg);
	}
	wtk_free(audio);
	return (jint)0;
}

/*
 * Class:     com_qdreamer_qvoice_QAudio
 * Method:    recorderStart
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_QAudio_recorderStart
  (JNIEnv *env, jclass cls, jlong jaudio)
{
	qtk_jni_audio_t *audio;
	qtk_jni_audio_rcd_t *rcd;
	wtk_queue_node_t *qn;
	int buf_time,buf_size;
	int ret;
	int i;

	audio = (qtk_jni_audio_t*)jaudio;
	if(!audio) {
		return (jint)-1;
	}

	qtk_audio_rcder_set_notify(audio->audio,audio,(qtk_audio_rcd_notify_f)qtk_jni_audio_on_record);
	wtk_blockqueue_init(&audio->audio_q);
	wtk_blockqueue_init(&audio->cache_q);
	audio->cache_time = 500;
	buf_time = qtk_audio_rcder_get_bufTime(audio->audio);
	audio->cache_times = audio->cache_time / buf_time;
	if(audio->cache_times < 1) {
		audio->cache_times = 1;
	}
	buf_size = qtk_audio_rcder_get_bufSize(audio->audio);
	for(i=0;i<audio->cache_times;++i) {
		rcd = qtk_jni_audio_rcd_new(buf_size);
		wtk_blockqueue_push(&audio->cache_q,&rcd->q_n);
	}
	wtk_log_log(audio->audio->session->log,"audio->cache_times==%d",audio->cache_times);

#if 0
    audio->linux_wav=wtk_wavfile_new(16000);
    audio->linux_wav->max_pend=0;
    wtk_wavfile_open(audio->linux_wav, "/sdcard/Android/data/com.qdreamer.asr/files/linux.wav");

    audio->android_wav=wtk_wavfile_new(16000);
    audio->android_wav->max_pend=0;
    wtk_wavfile_open(audio->android_wav, "/sdcard/Android/data/com.qdreamer.asr/files/android.wav");
#else
	audio->linux_wav=NULL;
	audio->android_wav=NULL;
#endif

	wtk_debug("QAudio_recorderStart===============>>>>>>>>>>>>>>start  0000!\n");
	ret = qtk_audio_rcder_start(audio->audio);

	if(ret != 0) {
		while(1) {
			qn = wtk_blockqueue_pop(&audio->cache_q,0,NULL);
			if(!qn) {
				break;
			}
			rcd = data_offset2(qn,qtk_jni_audio_rcd_t,q_n);
			qtk_jni_audio_rcd_delete(rcd);//回收内存
		}
		wtk_blockqueue_clean(&audio->cache_q);
		wtk_blockqueue_clean(&audio->audio_q);
	}
	wtk_debug("QAudio_recorderStart===============>>>>>>>>>>>>>>start  1111!\n");

	return (jint)ret;
}

/*
 * Class:     com_qdreamer_qvoice_QAudio
 * Method:    recorderStop
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_QAudio_recorderStop
  (JNIEnv *env, jclass cls, jlong jaudio)
{
	qtk_jni_audio_t *audio;
	qtk_jni_audio_rcd_t *rcd;
	wtk_queue_node_t *qn;
	int ret;

	audio = (qtk_jni_audio_t *)jaudio;
	if(!audio) {
		return (jint)-1;
	}

	if(audio->linux_wav){
		wtk_wavfile_close(audio->linux_wav);
		wtk_wavfile_delete(audio->linux_wav);
		audio->linux_wav=NULL;
	}
	if(audio->android_wav){
		wtk_wavfile_close(audio->android_wav);
		wtk_wavfile_delete(audio->android_wav);
		audio->android_wav=NULL;
	}

	ret = qtk_audio_rcder_stop(audio->audio);

	while(1) {
		qn = wtk_blockqueue_pop(&audio->audio_q,0,NULL);
		if(!qn) {
			break;
		}
		rcd = data_offset2(qn,qtk_jni_audio_rcd_t,q_n);
		qtk_jni_audio_rcd_delete(rcd);
	}
	while(1) {
		qn = wtk_blockqueue_pop(&audio->cache_q,0,NULL);
		if(!qn) {
			break;
		}
		rcd = data_offset2(qn,qtk_jni_audio_rcd_t,q_n);
		qtk_jni_audio_rcd_delete(rcd);
	}
	wtk_blockqueue_clean(&audio->cache_q);
	wtk_blockqueue_clean(&audio->audio_q);
	return (jint)ret;
}

/*
 * Class:     com_qdreamer_qvoice_QAudio
 * Method:    recorderRead
 * Signature: (J)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_qdreamer_qvoice_QAudio_recorderRead
  (JNIEnv *env, jclass cls, jlong jaudio)
{
	qtk_jni_audio_t *audio;
	qtk_jni_audio_rcd_t *rcd;
	wtk_queue_node_t *qn;
	jbyteArray jdata = NULL;

	audio = (qtk_jni_audio_t*)jaudio;
	if(!audio) {
		return NULL;
	}

	//会阻塞的唯一情况是调用者处理数据太快
	qn = wtk_blockqueue_pop(&audio->audio_q,audio->cache_time,NULL);
	if(!qn) {
		wtk_log_log0(audio->audio->session->log,"cache_time 500ms no voice data");
		return NULL;
	}

	rcd = data_offset2(qn,qtk_jni_audio_rcd_t,q_n);
	if(audio->android_wav){
		wtk_wavfile_write(audio->android_wav,rcd->buf->data,rcd->buf->pos);
	}
	jdata = (*env)->NewByteArray(env,rcd->buf->pos);
	(*env)->SetByteArrayRegion(env,jdata,0,rcd->buf->pos,(jbyte*)rcd->buf->data);

	wtk_blockqueue_push(&audio->cache_q,&rcd->q_n);//回收空闲节点
	return jdata;
}

/*
 * Class:     com_qdreamer_qvoice_QAudio
 * Method:    recorderGetChannel
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_QAudio_recorderGetChannel
  (JNIEnv *env, jclass cls, jlong jaudio)
{
	qtk_jni_audio_t *audio;
	int channel;

	audio = (qtk_jni_audio_t*)jaudio;
	if(!audio) {
		return (jint)0;
	}

	channel = qtk_audio_rcder_get_channel(audio->audio);
	return (jint)channel;
}

/*
 * Class:     com_qdreamer_qvoice_QAudio
 * Method:    recorderGetRate
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_QAudio_recorderGetRate
  (JNIEnv *env, jclass cls, jlong jaudio)
{
	qtk_jni_audio_t *audio;
	int rate;

	audio = (qtk_jni_audio_t*)jaudio;
	if(!audio) {
		return (jint)0;
	}

	rate = qtk_audio_rcder_get_sampleRate(audio->audio);
	return (jint)rate;
}

/*
 * Class:     com_qdreamer_qvoice_QAudio
 * Method:    recorderGetBytes
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_QAudio_recorderGetBytes
  (JNIEnv *env, jclass cls, jlong jaudio)
{
	qtk_jni_audio_t *audio;
	int bytes_per_sample;

	audio = (qtk_jni_audio_t*)jaudio;
	if(!audio) {
		return (jint)0;
	}

	bytes_per_sample = qtk_audio_rcder_get_bytes(audio->audio);
	return (jint)bytes_per_sample;
}

/*
 * Class:     com_qdreamer_qvoice_QAudio
 * Method:    playerStart
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_QAudio_playerStart
  (JNIEnv *env, jclass cls, jlong jaudio)
{
	qtk_jni_audio_t *audio;
	int ret;

	audio = (qtk_jni_audio_t*)jaudio;
	if(!audio) {
		return (jint)-1;
	}

	ret = qtk_audio_plyer_start(audio->audio);
	return (jint)ret;
}

/*
 * Class:     com_qdreamer_qvoice_QAudio
 * Method:    playerStop
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_QAudio_playerStop
  (JNIEnv *env, jclass cls, jlong jaudio)
{
	qtk_jni_audio_t *audio;
	int ret;

	audio = (qtk_jni_audio_t*)jaudio;
	if(!audio) {
		return (jint)-1;
	}

	ret = qtk_audio_plyer_stop(audio->audio);
	return (jint)ret;
}

/*
 * Class:     com_qdreamer_qvoice_QAudio
 * Method:    playerSetFormat
 * Signature: (JIII)V
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_QAudio_playerSetFormat
  (JNIEnv *env, jclass cls, jlong jaudio, jint jrate, jint jchannel, jint jbytes_per_sample)
{
	qtk_jni_audio_t *audio;

	audio = (qtk_jni_audio_t*)jaudio;
	if(!audio) {
		return (jint)-1;
	}
	qtk_audio_plyer_play_start(audio->audio,(int)jrate,(int)jchannel,(int)jbytes_per_sample);
	return (jint)0;
}

/*
 * Class:     com_qdreamer_qvoice_QAudio
 * Method:    playerPlay
 * Signature: (J[BIII)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_QAudio_playerPlay
  (JNIEnv *env, jclass cls, jlong jaudio, jbyteArray jdata, jint jbytes, jint jis_end, jint jsyn)
{
	qtk_jni_audio_t *audio;
	char *data;

	audio = (qtk_jni_audio_t*)jaudio;
	if(!audio) {
		return (jint)-1;
	}

	data = (char*)(*env)->GetByteArrayElements(env,jdata,0);
	if(!data) {
		return (jint)-1;
	}

	qtk_audio_plyer_play_data(audio->audio,data,(int)jbytes);

	if((int)jis_end) {
		qtk_audio_plyer_play_end(audio->audio,(int)jsyn);
	}
	(*env)->ReleaseByteArrayElements(env,jdata,data,0);
	return (jint)0;
}

/*
 * Class:     com_qdreamer_qvoice_QAudio
 * Method:    playerInterrupt
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_QAudio_playerInterrupt
  (JNIEnv *env, jclass cls, jlong jaudio)
{
	qtk_jni_audio_t *audio;

	audio = (qtk_jni_audio_t*)jaudio;
	if(!audio) {
		return (jint)-1;
	}

	qtk_audio_plyer_stop_play(audio->audio);
	return (jint)0;
}

/*
 * Class:     com_qdreamer_qvoice_QAudio
 * Method:    playerSetVolume
 * Signature: (JF)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_QAudio_playerSetVolume
  (JNIEnv *env, jclass cls, jlong jaudio, jfloat jvolume)
{
	qtk_jni_audio_t *audio;

	audio = (qtk_jni_audio_t*)jaudio;
	if(!audio) {
		return (jint)-1;
	}

	qtk_audio_plyer_set_volume(audio->audio,(float)jvolume);
	return (jint)0;
}

/*
 * Class:     com_qdreamer_qvoice_QAudio
 * Method:    playerIncVolume
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_QAudio_playerIncVolume
  (JNIEnv *env, jclass cls, jlong jaudio)
{
	qtk_jni_audio_t *audio;

	audio = (qtk_jni_audio_t*)jaudio;
	if(!audio) {
		return (jint)-1;
	}

	qtk_audio_plyer_inc_volume(audio->audio);
	return (jint)0;
}

/*
 * Class:     com_qdreamer_qvoice_QAudio
 * Method:    playerDecVolume
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_QAudio_playerDecVolume
  (JNIEnv *env, jclass cls, jlong jaudio)
{
	qtk_jni_audio_t *audio;

	audio = (qtk_jni_audio_t*)jaudio;
	if(!audio) {
		return (jint)-1;
	}

	qtk_audio_plyer_dec_volume(audio->audio);
	return (jint)0;
}


/*
 * Class:     com_qdreamer_qvoice_QAudio
 * Method:    playerSetPitch
 * Signature: (JF)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_QAudio_playerSetPitch
  (JNIEnv *env, jclass cls, jlong jaudio, jfloat jpitch)
{
	qtk_jni_audio_t *audio;

	audio = (qtk_jni_audio_t*)jaudio;
	if(!audio) {
		return (jint)-1;
	}

	qtk_audio_plyer_set_pitch(audio->audio,(float)jpitch);
	return (jint)0;
}

/*
 * Class:     com_qdreamer_qvoice_QAudio
 * Method:    playerIncPitch
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_QAudio_playerIncPitch
  (JNIEnv *env, jclass cls, jlong jaudio)
{
	qtk_jni_audio_t *audio;

	audio = (qtk_jni_audio_t*)jaudio;
	if(!audio) {
		return (jint)-1;
	}

	qtk_audio_plyer_inc_pitch(audio->audio);
	return (jint)0;
}

/*
 * Class:     com_qdreamer_qvoice_QAudio
 * Method:    playerDecPitch
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_qdreamer_qvoice_QAudio_playerDecPitch
  (JNIEnv *env, jclass cls, jlong jaudio)
{
	qtk_jni_audio_t *audio;

	audio = (qtk_jni_audio_t*)jaudio;
	if(!audio) {
		return (jint)-1;
	}

	qtk_audio_plyer_dec_pitch(audio->audio);
	return (jint)0;
}


