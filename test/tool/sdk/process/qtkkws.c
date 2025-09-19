#include "qtk_api.h"
#include "sdk/api_1/kws/qtk_qkws.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
typedef struct {
	qtk_qkws_t *b;
	int vad_no;
	unsigned w:1;
}etest_t;

extern double time_get_ms();

#define min(a,b) (((a) < (b)) ? (a) : (b))

uint64_t etest_file_length(FILE *f)
{
	uint64_t len;

    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);
    //wtk_debug("len=%ld\n",len);
    return len;
}

char* etest_file_read_buf(char* fn, int *n)
{
	FILE* file = fopen(fn, "rb");
	char* p = 0;
	int len;

	if (file)
	{
        len=etest_file_length(file);
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

void etest_on_notify(etest_t *eb,qtk_var_t *v)
{
	switch(v->type){
		case QTK_SPEECH_DATA_PCM:
			break;
		case QTK_AEC_DIRECTION:
			printf("direction:nbest=%d theta %d phi %d\n",v->v.ii.nbest ,v->v.ii.theta, v->v.ii.phi);
			break;
		case QTK_ASR_TEXT:
			printf("====================>>>>>>>>>>>>>[%.*s]\n",v->v.str.len,v->v.str.data);
			break;
		default:
			break;
	}
}
void test_etest(qtk_session_t *s, char *infn, int is_enroll)
{
	etest_t *eb;
	char *params;
	char *wav_fn;
	char *data = NULL;
	int len=0;
	double tm;

	eb = (etest_t*)malloc(sizeof(etest_t));
	if(!eb){
		printf("malloc failed\n");
		exit(0);
	}
	memset(eb,0,sizeof(etest_t));

	params = "role=kws;cfg=./res-kws/engine.cfg;syn=1;use_thread=0;";
	
	qtk_qkws_cfg_t *qcfg;
	qcfg = qtk_qkws_cfg_new("./res-kws/engine.cfg");
	eb->b = qtk_qkws_new(qcfg, s);
	qtk_qkws_set_notify(eb->b, eb, (qtk_engine_notify_f)etest_on_notify);
	wav_fn = infn;

	qtk_qkws_set_enroll_fn(eb->b, "./mem.bin",strlen("./mem.bin"));
	qtk_qkws_set_enroll(eb->b, "liuyong", strlen("liuyong"), 0);

	data = etest_file_read_buf(wav_fn,&len);
	printf("wav:%s len = %d\n",wav_fn,len);
	if(len<=0){
		printf("the wav is NULL\n");
		goto end;
	}
	qtk_qkws_start(eb->b);
	tm = time_get_ms();
#if 1
	int pos,step,slen;
	pos = 44;
	step = 32*32;
	//step=768;
	while(pos<len){
		slen = min(len-pos,step);
		qtk_qkws_feed(eb->b,data+pos,slen,0);
		//usleep(20*1000);
		pos += slen;
	}
	qtk_qkws_feed(eb->b,NULL,0,1);
#else
	qtk_engine_feed(eb->b,data+44,(len-44),1);
#endif
	tm = time_get_ms() - tm;
	printf("====================>>>%f/%f=%f\n",tm,len/(32.0),tm/(len/(32.0)));
	qtk_qkws_reset(eb->b);

	qtk_qkws_set_enroll(eb->b, "liuyong", strlen("liuyong"), 1);

end:
	if(data){
		free(data);
	}
	if(eb->b){
		qtk_qkws_delete(eb->b);
	}
	if(qcfg)
	{
		qtk_qkws_cfg_delete(qcfg);
	}
	free(eb);
}

static void test_module_on_errcode(qtk_session_t *session, int errcode,char *errstr)
{
        printf("====> errcode :%d errstr: %s\n",errcode,errstr);
}

int main(int argc,char *argv[])
{
	qtk_session_t *session;
	char *params;

	params = "appid=613679ec-9cf0-11eb-b526-00163e13c8a2;secretkey=f358afec-f82a-3fb2-be2d-ed096070b83e;"
						"cache_path=./qvoice;log_wav=0;use_timer=1;";

	session = qtk_session_init(params,QTK_WARN,NULL,(qtk_errcode_handler)test_module_on_errcode);
	if(!session) {
			printf("session init failed.\n");
			exit(1);
	}

	test_etest(session,argv[1],atoi(argv[2]));

	qtk_session_exit(session);
	return 0;
}
