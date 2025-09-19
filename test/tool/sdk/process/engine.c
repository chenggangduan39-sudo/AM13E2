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
}eengine_t;

extern double time_get_ms();

#define min(a,b) (((a) < (b)) ? (a) : (b))

uint64_t eengine_file_length(FILE *f)
{
	uint64_t len;

    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);
    //wtk_debug("len=%ld\n",len);
    return len;
}

char* eengine_file_read_buf(char* fn, int *n)
{
	FILE* file = fopen(fn, "rb");
	char* p = 0;
	int len;

	if (file)
	{
        len=eengine_file_length(file);
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

void eengine_on_notify(eengine_t *eb,qtk_var_t *v)
{
	switch(v->type){
	case QTK_SPEECH_START:
		printf("============>>>>>>>QTK_SPEECH_START\n");
		break;
	case QTK_SPEECH_END:
		printf("============>>>>>>>QTK_SPEECH_END\n");
		break;
	case QTK_SPEECH_DATA_PCM:
		//v->v.str.data; v->v.str.len;
		// printf(">>>>> pcm data len = %d\n",v->v.str.len);
		fwrite(v->v.str.data, v->v.str.len, 1, eb->echoFile);
		fflush(eb->echoFile);
		break;
	case QTK_VAR_SOURCE_AUDIO:
		fwrite(v->v.str.data, v->v.str.len, 1, eb->echoFile);
		fflush(eb->echoFile);
		printf("==============>>>>>>>>>>audio =%d\n",v->v.str.len);
		break;
	case QTK_AUDIO_ESTIMATE:
		printf("audio estimate[%d] =%d\n",v->v.str2.idx,v->v.str2.len);
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
	case QTK_AUDIO_RANGE_IDX:
		//printf("=========>>>>>>>>>>>>>>>>idx=%d\n",v->v.i);
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
	case QTK_AEC_WAKE:
		printf("=============>>>>>>>>>>wake\n");
		break;
	default:
		break;
	}
}
void test_eengine(qtk_session_t *s, char *infn, char *outfn, int channel)
{
	eengine_t *eb;
	char *params;
	char *wav_fn;
	char *data = NULL;
	int len=0;
	double tm;

	eb = (eengine_t*)malloc(sizeof(eengine_t));
	if(!eb){
		printf("malloc failed\n");
		exit(0);
	}
	memset(eb,0,sizeof(eengine_t));

	// params = "role=ssl;cfg=./res/res-ssl/engine.cfg;syn=1;use_thread=0;";
	 params = "role=vboxebf;cfg=./res/res-vboxebf/engine.cfg;syn=1;use_thread=0;";
	// params = "role=vboxebf;cfg=./res/res-vboxebf/engine.cfg;use_bin=0;use_resample=1;out_channel=2;resample_out_rate=48000;";
	// params = "role=vboxebf;cfg=./audio/engine.cfg;syn=1;use_thread=0;";
	// params = "role=gainnetbf;cfg=./res/res-gainnetbf/engine.cfg;syn=1;use_thread=0;";
	// params = "role=eqform;cfg=./res/res-eqform/engine.cfg;syn=1;use_thread=0;";
	//params = "role=consist;cfg=./consist-6mic.bin;use_bin=1;syn=1;playfn=./biu.wav;";
	//params = "role=ult;cfg=./res/res-ultgesture/engine.cfg;syn=1;";
	// params = "role=ultevm;cfg=./res/res-ultevm/engine.cfg;syn=1;use_thread=0;";
	// params = "role=aec;cfg=./res/res-aec/engine.cfg;syn=1;use_thread=0;";
	// params = "role=wakeup;cfg=./res/res-wakeup/engine.cfg;syn=1;use_thread=0;";
	// params = "role=wdec;cfg=./res/res-wdec/engine.cfg;syn=1;use_thread=0;";
	//params = "role=estimate;cfg=./res/res-estimate/engine.cfg;syn=1;use_thread=0;";
	//params = "role=bfio;cfg=./res/res-bfio/bfio.desaixiwei.4mic-20250320.bin;use_bin=1;syn=1;use_thread=0;mic_shift=3.0;spk_shift=3.0;echo_shift=1.1;";
	// params = "role=resample;cfg=./res/res-vboxebf/engine.cfg;syn=1;use_thread=0;channel=1;resample_in_rate=48000;resample_out_rate=16000;";
	// params = "role=soundscreen;cfg=./res/res-soundscreen/engine.cfg;syn=1;use_thread=0;";
	// params = "role=bfio2;cfg=./res/res-bfio/engine.cfg;use_bin=0;syn=1;use_thread=0;mic_shift=3.0;spk_shift=3.0;echo_shift=1.1;";
	// params = "role=vad;cfg=./res/bin/vad-20231225.bin;use_bin=1;syn=1;use_thread=0;";
	// params = "role=agc;cfg=./res/res-agc/engine.cfg;syn=1;use_thread=0;";
	// params = "role=maskbfnet;cfg=./res/res-maskbfnet/engine.cfg;syn=1;use_thread=0;";
	
	eb->b = qtk_engine_new(s,params);
	if(!eb->b){
		printf("engine new failed\n");
		goto end;
	}
	qtk_engine_set_notify(eb->b,eb,(qtk_engine_notify_f)eengine_on_notify);
	wav_fn = infn;

	eb->echoFile = fopen(outfn,"wb+");
	data = eengine_file_read_buf(wav_fn,&len);
	printf("wav:%s len = %d\n",wav_fn,len);
	if(len<=0){
		printf("the wav is NULL\n");
		goto end;
	}
	//qtk_engine_set(eb->b, "code_generate=1;");
	//qtk_engine_set(eb->b, "wakewrd=\"你好飞燕\";");

	// qtk_engine_set(eb->b, "ssl_enable=0;");
	// qtk_engine_set(eb->b, "agc_enable=0;");
	// qtk_engine_set(eb->b, "echo_enable=0;");
	// qtk_engine_set(eb->b, "denoise_enable=0;");
	// qtk_engine_set(eb->b, "noise_suppress=-25;");
	// qtk_engine_set(eb->b, "agc_level=3000;");
	
	qtk_engine_start(eb->b);

	// qtk_engine_set(eb->b, "agc_enable=0;");
	// qtk_engine_set(eb->b, "denoise_enable=0;");
	// qtk_engine_set(eb->b, "noise_suppress=50;");
	// qtk_engine_set(eb->b, "out_scale=1.0;");

	tm = time_get_ms();
	double ftm;
#if 1
	int pos,step,slen;
	pos = 44;
	// step = 32*channel*32;
	step=32*channel*96*2;
	char *output=malloc(step/channel);
	int olen;
	memset(output, 0, step/channel);
	while(pos<len){
		slen = min(len-pos,step);
		// ftm = time_get_ms();
		// printf("================>>>>>>>>>>>>>..slen=%d pos=%d len=%d\n",slen,pos,len);
		// qtk_engine_feed(eb->b,data+pos,slen,0);
		// printf("============>>>>>>>>>>>>>>feed time=%f\n",time_get_ms() - ftm);
		//usleep(20*1000);
		qtk_engine_feed2(eb->b, data+pos, slen, output, &olen, 0);
		// printf("===================>>>>>>>>>>>>>>ooooutlen=%d\n",olen);
		if(olen > 0){
			fwrite(output, olen, 1, eb->echoFile);
			fflush(eb->echoFile);
		}
		pos += slen;
	}
	// printf("+==========================>>>>>>>>>>>>eeeeeeeeeeeeeeeeeeeee=%d\n",slen);
	// qtk_engine_feed(eb->b,NULL,0,1);
	qtk_engine_feed2(eb->b, NULL, 0, NULL, NULL, 1);
#else
	qtk_engine_feed(eb->b,data+44,(len-44),1);
#endif
	tm = time_get_ms() - tm;
	// printf("====================>>>%f/%f=%f\n",tm,len/(32.0*channel),tm/(len/(32.0*channel)));
	printf("====================>>>%f/%f=%f\n",tm,len/(96.0*2*channel),tm/(len/(96.0*2*channel)));
	qtk_engine_reset(eb->b);
end:
	if(data){
		free(data);
	}
	if(output){
		free(output);
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

	test_eengine(session,argv[1],argv[2],atoi(argv[3]));

	qtk_session_exit(session);
	return 0;
}
