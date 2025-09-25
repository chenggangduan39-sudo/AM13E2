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
}eaecnspickup_t;

extern double time_get_ms();

#define min(a,b) (((a) < (b)) ? (a) : (b))

uint64_t eaecnspickup_file_length(FILE *f)
{
	uint64_t len;

    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);
    //wtk_debug("len=%ld\n",len);
    return len;
}

char* eaecnspickup_file_read_buf(char* fn, int *n)
{
	FILE* file = fopen(fn, "rb");
	char* p = 0;
	int len;

	if (file)
	{
        len=eaecnspickup_file_length(file);
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

void eaecnspickup_on_notify(eaecnspickup_t *eb,qtk_var_t *v)
{
	switch(v->type){
		case QTK_SPEECH_DATA_PCM:
			//v->v.str.data; v->v.str.len;
			// printf(">>>>> pcm data len = %d\n",v->v.str.len);
			fwrite(v->v.str.data, v->v.str.len, 1, eb->echoFile);
			fflush(eb->echoFile);
			break;
		case QTK_AEC_DIRECTION:
			printf("direction:nbest=%d theta %d phi %d\n",v->v.ii.nbest ,v->v.ii.theta, v->v.ii.phi);
			break;
		case QTK_CONSIST_MICERR_NIL:
			printf("QTK_CONSIST_MICERR_NIL ==>channel num=%d\n",v->v.i);
			break;
        case QTK_CONSIST_MICERR_ALIGN:
			printf("QTK_CONSIST_MICERR_ALIGN ==>channel num=%d\n",v->v.i);
			break;
        case QTK_CONSIST_MICERR_MAX:
			printf("QTK_CONSIST_MICERR_MAX ==>channel num=%d\n",v->v.i);
			break;
        case QTK_CONSIST_MICERR_CORR:
			printf("QTK_CONSIST_MICERR_CORR ==>channel num=%d\n",v->v.i);
			break;
        case QTK_CONSIST_MICERR_ENERGY:
			printf("QTK_CONSIST_MICERR_ENERGY ==>channel num=%d\n",v->v.i);
			break;
        case QTK_CONSIST_SPKERR_NIL:
			printf("QTK_CONSIST_SPKERR_NIL ==>channel num=%d\n",v->v.i);
			break;
        case QTK_CONSIST_SPKERR_ALIGN:
			printf("QTK_CONSIST_SPKERR_ALIGN ==>channel num=%d\n",v->v.i);
			break;
        case QTK_CONSIST_SPKERR_MAX:
			printf("QTK_CONSIST_SPKERR_MAX ==>channel num=%d\n",v->v.i);
			break;
        case QTK_CONSIST_SPKERR_CORR:
			printf("QTK_CONSIST_SPKERR_CORR ==>channel num=%d\n",v->v.i);
			break;
        case QTK_CONSIST_SPKERR_ENERGY:
			printf("QTK_CONSIST_SPKERR_ENERGY ==>channel num=%d\n",v->v.i);
			break;
		case QTK_ULTGESTURE_TYPE:
			switch (v->v.i)
			{
			case 0:
				break;
			case 1:
				printf("向右\n");
				break;
			case 2:
				printf("向左\n");
				break;
			case 3:
				printf("前进\n");
				break;
			case 4:
				printf("后退\n");
				break;			
			default:
				break;
			}
			break;
		case QTK_ULTEVM_TYPE:
			if(v->v.i==0)
			{
				//printf("静止\n");
			}else if(v->v.i == 1)
			{
				//printf("移动\n");
			}
			printf("%d\n",v->v.i);
			break;
		default:
			break;
	}
}
void test_eaecnspickup(qtk_session_t *s, char *infn, char *outfn, int channel)
{
	eaecnspickup_t *eb;
	char *params;
	char *wav_fn;
	char *data = NULL;
	int len=0;
	double tm;

	eb = (eaecnspickup_t*)malloc(sizeof(eaecnspickup_t));
	if(!eb){
		printf("malloc failed\n");
		exit(0);
	}
	memset(eb,0,sizeof(eaecnspickup_t));

	// params = "role=ssl;cfg=./sdk_demo/res/engine_ssl.cfg;syn=1;";
	 params = "role=vboxebf;cfg=./res-vboxebf/engine.cfg;syn=1;use_thread=0;";
	// params = "role=vboxebf;cfg=./audio/engine.cfg;syn=1;use_thread=0;";
	// params = "role=gainnetbf;cfg=./res-gainnetbf/engine.cfg;syn=1;use_thread=0;";
	//params = "role=consist;cfg=./consist-6mic.bin;use_bin=1;syn=1;playfn=./biu.wav;";
	//params = "role=ult;cfg=./res-ultgesture/engine.cfg;syn=1;";
	// params = "role=ultevm;cfg=./res-ultevm/engine.cfg;syn=1;use_thread=0;";
	
	eb->b = qtk_engine_new(s,params);
	if(!eb->b){
		printf("engine new failed\n");
		goto end;
	}
	qtk_engine_set_notify(eb->b,eb,(qtk_engine_notify_f)eaecnspickup_on_notify);
	wav_fn = infn;

	eb->echoFile = fopen(outfn,"wb+");
	data = eaecnspickup_file_read_buf(wav_fn,&len);
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
	//step=768;
	while(pos<len){
		slen = min(len-pos,step);
		qtk_engine_feed(eb->b,data+pos,slen,0);
		//usleep(20*1000);
		pos += slen;
	}
	qtk_engine_feed(eb->b,NULL,0,1);
#else
	qtk_engine_feed(eb->b,data+44,(len-44),1);
#endif
	tm = time_get_ms() - tm;
	printf("====================>>>%f/%f=%f\n",tm,len/(32.0*channel),tm/(len/(32.0*channel)));
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

	test_eaecnspickup(session,argv[1],argv[2],atoi(argv[3]));

	qtk_session_exit(session);
	return 0;
}
