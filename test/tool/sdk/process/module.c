#include "sdk/qtk_api.h"
#include "wtk/core/cfg/wtk_main_cfg.h"

FILE *of=NULL;

void test_mwakeup_on_process(void *ths,qtk_var_t *var)
{
	switch(var->type){
	case QTK_SPEECH_DATA_PCM:
		if(of == NULL)
		{
			of = fopen("./out.pcm","wb+");
		}
		if(var->v.str.len > 0)
		{
			fwrite(var->v.str.data, var->v.str.len, 1, of);
			fflush(of);
		}
		//var->v.str.data;
		//var->v.str.len;
		// wtk_debug("+===============>>>>>>>>>>>>>>>len=%d\n",var->v.str.len);
		break;
	case QTK_VAR_SOURCE_AUDIO:
		wtk_debug("+===============>>>>>>>>>>>>>>>len=%d\n",var->v.str.len);
		break;
	case QTK_AEC_DIRECTION:
		wtk_debug("theta=%d\n",var->v.ii.theta);
		break;
	case QTK_ULTEVM_TYPE:
		wtk_debug("=======>>>>active=%d\n",var->v.i);
		break;
	default:
		break;
	}
}


void test_module(qtk_session_t * session,char *infn, int channel)
{
	qtk_module_t * m;
	int ret;

	// m = qtk_module_new(session,"ult","./res-ult/module.cfg");
	// m = qtk_module_new(session,"mqform","./res-module/mqform.cfg");
	 m = qtk_module_new(session,"mqform","./res-module/module_caec.cfg");
	// m = qtk_module_new(session,"mqform","./res-module/module_vboxebf.cfg");
	// m = qtk_module_new(session,"qform","./res-vboxebf/module.cfg");
	if(!m){
		wtk_debug("new module failed\n");
		exit(1);
	}

	qtk_module_set_notify(m,NULL,(qtk_engine_notify_f)test_mwakeup_on_process);

	ret = qtk_module_start(m);
	if(ret != 0){
		wtk_debug("module start failed\n");
		exit(1);
	}
#ifdef OFFLINE_TEST
	char *data;
	int len = 0;
	data = file_read_buf(infn, &len);
	int step = 32* 32 * channel;
	int flen;
	int pos = 44;
	while(pos < len){
			flen = min(step, len - pos);
			qtk_module_feed(m, data+pos, flen);
			// wtk_msleep(20);
			usleep(20 * 1000);
			pos += flen;
	}
	free(data);
#else
	getchar();
#endif
	qtk_module_stop(m);
	qtk_module_delete(m);

	if(of)
	{
		fclose(of);
	}
}

static void test_module_on_errcode(qtk_session_t *session, int errcode,char *errstr)
{
        wtk_debug("====> errcode :%d errstr: %s\n",errcode,errstr);
}

int main(int argc,char *argv[])
{
	qtk_session_t *session;
	char *params;

	params = "appid=f4b7c86a-ef57-11e6-8aa7-00e04c12c2c7;secretkey=0063909a-ef58-11e6-8a33-00e04c12c2c7;"
						"cache_path=./qvoice;log_wav=0;use_timer=1;";

	session = qtk_session_init(params,QTK_WARN,NULL,(qtk_errcode_handler)test_module_on_errcode);
	if(!session) {
			wtk_debug("session init failed.\n");
			exit(1);
	}

	test_module(session,argv[1],atoi(argv[2]));

	qtk_session_exit(session);
	return 0;
}
