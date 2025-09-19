#include "sdk/qtk_api.h"
#include <stdlib.h>
#include "wtk/core/wtk_os.h"

void test_ecsr_on_notify(void *ths,qtk_var_t *var)
{
	switch(var->type) {
	case QTK_SPEECH_START:
		wtk_debug("===>speech start %f count=%d\n",var->v.fi.theta,var->v.fi.on);
		break;
	case QTK_SPEECH_DATA_PCM:
		break;
	case QTK_SPEECH_END:
		wtk_debug("===>speech end %f count=%d\n",var->v.fi.theta,var->v.fi.on);
		break;
	case QTK_ASR_TEXT:
		wtk_debug("asr result = %.*s\n",var->v.str.len,var->v.str.data);
		break;
	case QTK_ASR_HINT:
		wtk_debug("%.*s\n",var->v.str.len,var->v.str.data);
		break;
	case QTK_VAR_ERR:
		wtk_debug("error = %d\n",var->v.i);
		break;
	default:
		//wtk_debug("unexpect type\n");
		break;
	}
}

void test_ecsr(qtk_session_t *session, char *infn)
{
	qtk_engine_t *e;
	char *params;
	char *fn,*data;
	int len;
	int ret;

	// params = "role=csr;cfg=./res/bin/csr.bin;left_margin=10;right_margin=10;rec_min_conf=-10;";
	//params = "role=csr;cfg=./res/bin/csr-1225.bin;use_bin=1;use_thread=0;use_lex=0;use_hint=1;";//left_margin=10;right_margin=10; 
	params = "role=csr;cfg=./res/bin/csr-k2-20240123001.bin;use_bin=1;use_thread=0;use_lex=0;use_hint=1;";//left_margin=10;right_margin=10; 
	// params = "role=csr;cfg=./res/bin/csr-20240103.bin;use_bin=1;use_thread=0;use_lex=0;";//left_margin=10;right_margin=10; 
	//  params = "role=csr;cfg=./res/bin/csr-20240119.bin;use_bin=1;use_thread=0;use_lex=0;";//left_margin=10;right_margin=10; 
	e = qtk_engine_new(session,params);
	if(!e) {
		wtk_debug("new failed.\n");
		exit(1);
	}

	ret = qtk_engine_set_notify(e,NULL,(qtk_engine_notify_f)test_ecsr_on_notify);
	if(ret != 0) {
		wtk_debug("set notify failed.\n");
		exit(1);
	}
	qtk_engine_start(e);
	//fn = "./data/test_csr.wav";
	fn = infn;
	data=file_read_buf(fn,&len);
	double tm;
	tm = time_get_ms();
#if 1
	int pos,step;
	pos = 44;
	while(pos<len){
		step = min(640,len-pos);
		qtk_engine_feed(e,data+pos,step,0);
		pos += step;
		wtk_msleep(20);
	}
#else
	qtk_engine_feed(e,data+44,len-44,0);
#endif
	qtk_engine_feed(e,NULL,0,1);
	sleep(3);
	tm = time_get_ms() - tm;
	printf("==========>>>>>>>>>>>>>>rate=%f %f %f\n",tm/(len/32.0),tm,len/32.0);
	qtk_engine_reset(e);
	qtk_engine_cancel(e);
	qtk_engine_delete(e);
	wtk_free(data);
}

static void test_module_on_errcode(qtk_session_t *session, int errcode,char *errstr)
{
        printf("====> errcode :%d errstr: %s\n",errcode,errstr);
}

int main(int argc,char *argv[])
{
	qtk_session_t *session;
	char *params;

	params = "appid=57fe0d38-bd8a-11ed-b526-00163e13c8a2;secretkey=42ddc64a-1c61-39bc-9cfd-0aa8bdd1009c;"
						"cache_path=./qvoice;use_log=1;log_wav=0;use_timer=1;timeout=2000;use_srvsel=1;";

	session = qtk_session_init(params,QTK_WARN,NULL,(qtk_errcode_handler)test_module_on_errcode);
	if(!session) {
			printf("session init failed.\n");
			exit(1);
	}

	test_ecsr(session, argv[1]);

	qtk_session_exit(session);
	return 0;
}
