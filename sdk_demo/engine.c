#include "qtk_api.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
typedef struct {
	qtk_engine_t *b;
	FILE *echoFile;
	int vad_no;
	unsigned w:1;
}engine_t;

#define min(a,b) (((a) < (b)) ? (a) : (b))

extern double time_get_ms();

uint64_t engine_file_length(FILE *f)
{
	uint64_t len;

    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);
    return len;
}

char* engine_file_read_buf(char* fn, int *n)
{
	FILE* file = fopen(fn, "rb");
	char* p = 0;
	int len;

	if (file)
	{
        len=engine_file_length(file);
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

void engine_on_notify(engine_t *eb,qtk_var_t *v)
{
	switch(v->type){
		case QTK_SPEECH_DATA_PCM:
			fwrite(v->v.str.data, v->v.str.len, 1, eb->echoFile);
			fflush(eb->echoFile);
			break;
		case QTK_AEC_DIRECTION:
			printf("direction:nbest=%d/%f theta %d phi %d\n",v->v.ii.nbest ,v->v.ii.nspecsum,v->v.ii.theta, v->v.ii.phi);
			break;
		default:
			break;
	}
}
void test_engine(qtk_session_t *s, char *infn, char *outfn, int channel)
{
	engine_t *eb;
	char *params;
	char *wav_fn;
	char *data = NULL;
	int len=0;
	double tm;

	eb = (engine_t*)malloc(sizeof(engine_t));
	if(!eb){
		printf("malloc failed\n");
		exit(0);
	}
	memset(eb,0,sizeof(engine_t));

	params = "role=vboxebf;cfg=./res/vboxebf/engine.cfg;syn=1;";

	eb->b = qtk_engine_new(s,params);
	if(!eb->b){
		printf("engine new failed\n");
		goto end;
	}
	qtk_engine_set_notify(eb->b,eb,(qtk_engine_notify_f)engine_on_notify);
	wav_fn = infn;

	eb->echoFile = fopen(outfn,"wb+");
	data = engine_file_read_buf(wav_fn,&len);
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
		//usleep(32*1000);
		pos += slen;
	}
	qtk_engine_feed(eb->b,NULL,0,1);
	tm = time_get_ms() - tm;
#else
	qtk_engine_feed(eb->b,data+44,len-44,1);
#endif
	printf("============================>>>>>%f %f %f\n",tm,pos/320.0f,tm/(pos/320.0f));
	qtk_engine_reset(eb->b);
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

	test_engine(session,argv[1],argv[2],atoi(argv[3]));

	qtk_session_exit(session);
	return 0;
}
