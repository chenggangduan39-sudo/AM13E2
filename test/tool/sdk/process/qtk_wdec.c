#include "qtk_api.h"
#include "sdk/api_1/wdec/qtk_wdec.h"
#include "sdk/api_1/wdec/qtk_wdec_cfg.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
typedef struct {
	qtk_wdec_t *b;
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
                case QTK_AEC_WAKE:
                        printf("=============>>>>>>>>>>wake\n");
                        break;
		default:
			break;
	}
}
void test_etest(char *infn)
{
	etest_t *eb;
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
	
	qtk_wdec_cfg_t *qcfg;
	qcfg = qtk_wdec_cfg_new("./res-wdec/engine.cfg");
	eb->b = qtk_wdec_new(qcfg, NULL);
	qtk_wdec_set_notify(eb->b, eb, (qtk_engine_notify_f)etest_on_notify);
	wav_fn = infn;

	data = etest_file_read_buf(wav_fn,&len);
	printf("wav:%s len = %d\n",wav_fn,len);
	if(len<=0){
		printf("the wav is NULL\n");
		goto end;
	}
	qtk_wdec_start(eb->b);
	tm = time_get_ms();
#if 1
	int pos,step,slen;
	pos = 44;
	step = 32*32;
	//step=768;
	while(pos<len){
		slen = min(len-pos,step);
		qtk_wdec_feed(eb->b,data+pos,slen,0);
		//usleep(20*1000);
		pos += slen;
	}
	qtk_wdec_feed(eb->b,NULL,0,1);
#else
	qtk_engine_feed(eb->b,data+44,(len-44),1);
#endif
	tm = time_get_ms() - tm;
	printf("====================>>>%f/%f=%f\n",tm,len/(32.0),tm/(len/(32.0)));
	qtk_wdec_reset(eb->b);

end:
	if(data){
		free(data);
	}
	if(eb->b){
		qtk_wdec_delete(eb->b);
	}
	if(qcfg)
	{
		qtk_wdec_cfg_delete(qcfg);
	}
	free(eb);
}

static void test_module_on_errcode(qtk_session_t *session, int errcode,char *errstr)
{
        printf("====> errcode :%d errstr: %s\n",errcode,errstr);
}

int main(int argc,char *argv[])
{
	test_etest(argv[1]);

	return 0;
}
