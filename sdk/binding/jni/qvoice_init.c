#include "qvoice_init.h"
#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_os.h"

JavaVM *gJvm = NULL;
jobject gClassLoader = NULL;
jmethodID gFindClassMethod = NULL;
jclass *session_cls = NULL;
jclass *module_cls = NULL;
jclass *engine_cls = NULL;
jclass *audio_cls = NULL;
jclass *ogg_cls = NULL;
// jclass *vw_cls = NULL;

JNIEnv* getEnv()
{
	JNIEnv *env;
	int status = (*gJvm)->GetEnv(gJvm, (void**) &env, JNI_VERSION_1_6);
	if (status < 0)
	{
		status = (*gJvm)->AttachCurrentThread(gJvm, &env, NULL);
		if (status < 0)
		{
			return NULL;
		}
	}
	return env;
}


/*
 * JNI_OnLoad
 */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *pjvm, void *reserved)
{
	char buf[256];
	jclass randomClass;

	wtk_get_build_timestamp(buf);
	wtk_debug("%s", "==================================\n");
	wtk_debug("Qdreamer SDK build at %s\n", buf);
	wtk_debug("%s", "==================================\n");

	gJvm = pjvm;
	JNIEnv* env = getEnv();

	randomClass = (*env)->FindClass(env,"com/qdreamer/qvoice/QAudio");
	audio_cls = (*env)->NewGlobalRef(env,randomClass);
	randomClass = (*env)->FindClass(env,"com/qdreamer/qvoice/QSession");
	session_cls = (*env)->NewGlobalRef(env,randomClass);
	randomClass = (*env)->FindClass(env,"com/qdreamer/qvoice/QModule");
	module_cls = (*env)->NewGlobalRef(env,randomClass);
	randomClass = (*env)->FindClass(env,"com/qdreamer/qvoice/OggHelper");
	ogg_cls = (*env)->NewGlobalRef(env,randomClass);
	// randomClass = (*env)->FindClass(env,"com/qdreamer/vprintWake/vprintWakeHelper");
	// vw_cls = (*env)->NewGlobalRef(env,randomClass);


	return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *pjvm, void *reserved)
{
	char buf[256];

	gJvm = pjvm;
	JNIEnv *env = getEnv();

	(*env)->DeleteGlobalRef(env,audio_cls);
	audio_cls = NULL;
	(*env)->DeleteGlobalRef(env,session_cls);
	session_cls = NULL;
	(*env)->DeleteGlobalRef(env,module_cls);
	module_cls = NULL;
	(*env)->DeleteGlobalRef(env,ogg_cls);
	ogg_cls = NULL;
	// (*env)->DeleteGlobalRef(env,vw_cls);
	// vw_cls = NULL;

	wtk_get_build_timestamp(buf);
	wtk_debug("%s", "==================================\n");
	wtk_debug("Qdreamer SDK destroy at %s\n", buf);
	wtk_debug("%s", "==================================\n");
}
