#include "qtk_api.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include "wtk/core/wtk_type.h"
typedef struct {
	qtk_engine_t *b;
	FILE *echoFile;
	int vad_no;
	unsigned w:1;
}etest_engine_t;

extern double time_get_ms();

#define min(a,b) (((a) < (b)) ? (a) : (b))

uint64_t etest_engine_file_length(FILE *f)
{
	uint64_t len;

    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);
    //printf("len=%ld\n",len);
    return len;
}

char* etest_engine_file_read_buf(char* fn, int *n)
{
	FILE* file = fopen(fn, "rb");
	char* p = 0;
	int len;

	if (file)
	{
        len=etest_engine_file_length(file);
		p = (char*) malloc(len + 1);
		len = fread(p, 1, len, file);
		if (n)
		{
			*n = len;
		}
		fclose(file);
		p[len] = 0;
	}
	return p;
}

void etest_engine_on_notify(etest_engine_t *eb,qtk_var_t *var)
{
	switch(var->type) {
	case QTK_SPEECH_START:
		printf("===>speech start\n");
		break;
	case QTK_SPEECH_DATA_PCM:
		break;
	case QTK_SPEECH_END:
		printf("===>speech end\n");
		break;
	case QTK_ASR_TEXT:
		printf("asr result = %.*s\n",var->v.str.len,var->v.str.data);
		break;
	case QTK_ASR_HINT:
		printf("%.*s\n",var->v.str.len,var->v.str.data);
		break;
	case QTK_ASR_DELAY:
		printf("delay=%f\n",var->v.f);
		break;
	case QTK_AEC_WAKE:
		printf("=============>>>>>>>>>>wake fs=%f fe=%f\n",var->v.wakeff.fs,var->v.wakeff.fe);
		break;
	case QTK_VAR_ERR:
		printf("error = %d\n",var->v.i);
		break;
	default:
		printf("unexpect type = %d\n",var->type);
		break;
	}
}
void test_etest_engine(qtk_session_t *s, char *infn, char *outfn, int channel)
{
	etest_engine_t *eb;
	char *params;
	char *wav_fn;
	char *data = NULL;
	int len=0;
	double tm;

	eb = (etest_engine_t*)malloc(sizeof(etest_engine_t));
	if(!eb){
		printf("malloc failed\n");
		exit(0);
	}
	memset(eb,0,sizeof(etest_engine_t));

	//params = "role=csr;cfg=./res/bin/csr-20240103.bin;use_bin=1;use_thread=0;use_lex=0;gr_min_conf=-5.0;";
	//params = "role=csr;cfg=./res/bin/csr-20240119.bin;use_bin=1;use_thread=0;use_lex=0;gr_min_conf=-5.0;";
	// params = "role=csr;cfg=./res/bin/csr-local-20240124001.bin;use_bin=1;use_thread=0;use_lex=0;gr_min_conf=-5.0;";
	// params = "role=asr;cfg=./res/asr-local/asr.cfg;use_bin=0;use_thread=0;use_lex=0;";
	// params = "role=asr;cfg=./aa/asr_cfg;use_bin=0;use_thread=0;use_lex=0;";
	// params = "role=csr;cfg=./res/bin/csr-k2-20240123001.bin;use_bin=1;use_thread=0;use_lex=0;";
	// params = "role=asr;cfg=./res/yanshi-liuyong/engine-wake-asr.cfg;use_bin=0;use_thread=0;use_lex=0;";
	// params = "role=asr;cfg=./res/yanshi-liuyong/engine-general-asr.cfg;use_bin=0;use_thread=0;use_lex=0;";
	// params = "role=csr;cfg=./res/asr/csr/cfg;use_bin=0;coreType=cn.asr.rec;res=kk1123online;left_margin=10;right_margin=10;";
	// params = "role=img;cfg=./res/asr/wasr/wake_cfg;use_bin=0;";
	 params = "role=csr;cfg=./res/asr/csr/cmd_cfg;use_bin=0;";
	
	eb->b = qtk_engine_new(s,params);
	if(!eb->b){
		printf("engine new failed\n");
		goto end;
	}
	qtk_engine_set_notify(eb->b,eb,(qtk_engine_notify_f)etest_engine_on_notify);
	wav_fn = infn;
	// char *kwdata=NULL;
	// int klen=0;
	// kwdata = etest_engine_file_read_buf("./res/asr-local/keyword", &klen);

	// qtk_engine_update_cmds(eb->b, kwdata);

	// qtk_engine_set(eb->b, "wakewrd=你好小浩;");

	eb->echoFile = fopen(outfn,"wb+");
	data = etest_engine_file_read_buf(wav_fn,&len);
	printf("wav:%s len = %d\n",wav_fn,len);
	if(len<=0){
		printf("the wav is NULL\n");
		goto end;
	}
	qtk_engine_start(eb->b);
	tm = time_get_ms();
#if 1
	int pos,step,slen;
	pos = 44;
	step = 32*channel*32;
	while(pos<len){
		slen = min(len-pos,step);
		qtk_engine_feed(eb->b,data+pos,slen,0);
		usleep(32*1000);
		pos += slen;
		// wtk_debug("=============>>>>>>>>>>>>>>>>>>pos=%f\n",pos/32.0);
	}
	qtk_engine_feed(eb->b,NULL,0,1);
#else
	qtk_engine_feed(eb->b,data+44,len-44,1);
#endif
	// qtk_var_t var;
	// qtk_engine_get_result(eb->b, &var);
	// wtk_debug("result=%d=[%.*s]\n",var.v.str.len,var.v.str.len,var.v.str.data);
	qtk_engine_reset(eb->b);
	tm = time_get_ms() - tm;
	printf("====================>>>%f/%f=%f\n",tm,len/(32.0*channel),tm/(len/(32.0*channel)));
end:
	if(data){
		free(data);
	}	
	if(eb->b){
		qtk_engine_delete(eb->b);
	}	
	if(eb->echoFile)
	{
		fclose(eb->echoFile);
	}
	free(eb);
}

static void test_module_on_errcode(qtk_session_t *session, int errcode,char *errstr)
{
        //printf("====> errcode :%d errstr: %s\n",errcode,errstr);
}

int main(int argc,char *argv[])
{
	qtk_session_t *session;
	char *params;

	params = "appid=613679ec-9cf0-11eb-b526-00163e13c8a2;secretkey=f358afec-f82a-3fb2-be2d-ed096070b83e;"
						"cache_path=./qvoice;log_wav=0;use_timer=1;use_cldhub=1;host=153.37.177.186;port=19090;";

	session = qtk_session_init(params,QTK_WARN,NULL,(qtk_errcode_handler)test_module_on_errcode);
	if(!session) {
			printf("session init failed.\n");
			exit(1);
	}

	test_etest_engine(session,argv[1],argv[2],atoi(argv[3]));

	qtk_session_exit(session);
	return 0;
}
