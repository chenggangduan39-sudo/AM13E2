#include "sdk/qtk_api.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#define OFFLINE_RCD
#define OFFLINE_ASR
#define OFFLINE_WAKUP
#define OFFLINE_DENOISE
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define data_offset2(q,type,link) (type*)((void*)((char*)q-offsetof(type,link)))

typedef struct strbuf_tt
{
    char *data;					//raw data;
    int pos;					//valid data size;
    int length;					//memory size of raw data;
    float rate;					//memory increase rate;
}strbuf_t;

typedef struct queue_node queue_node_t;
struct queue_node
{
	queue_node_t* next;
	queue_node_t* prev;
};

typedef struct {
	queue_node_t q_n;
	int type;
	char* data;
	int len;
}qtk_msg_node_t;

typedef enum {
	qtk_hdgd_task_rcd=0,
	qtk_hdgd_task_aec,
	qtk_hdgd_task_denoise,
	qtk_hdgd_task_asr,
	qtk_hdgd_task_wakup,
	qtk_hdgd_task_tts,
	qtk_hdgd_task_debug,
} qtk_hdgd_type_t;

typedef struct{
	void* rcd_cfg;
	void* rcd;
	qtk_engine_t *aec;             // 回声消除
	qtk_engine_t *denoise;         // 降噪, (定向增强-qform|全向增强-vboxebf)
	qtk_engine_t *asr;             // 识别
	qtk_engine_t *tts;             // 合成
	qtk_engine_t *wakup;           // 唤醒
	qtk_test_queue_t* rcd_q;            // 录音数据队列
	void* wav_rcd;                 // 录音音频
	void* wav_aec;                 // 回声消除保存音频
	void* wav_denoise;                  // 降噪后保存音频
	void* wav_tts;                 // tts保存音频
	int type;
	int debug_chans;
	unsigned int use_save_rcd:1;
	unsigned int use_save_aec:1;
	unsigned int use_save_denoise:1;
	unsigned int use_save_tts:1;
	unsigned int is_end:1;
}qtk_hdgd_t;

extern double time_get_ms(void);
extern void* qtk_record_cfg_new();
extern void qtk_record_cfg_delete(void *cfg);
extern void *qtk_record_new(void *cfg);
extern void qtk_record_delete(void *rcd);
extern void *qtk_record_read(void *rcd);
extern char* file_read_buf(char* fn, int *n);
void qtk_hdgd_delete(qtk_hdgd_t* hdgd);
extern void* wtk_wavfile_new(int sample_rate);
extern int wtk_wavfile_open(void *f,char *fn);
extern void wtk_wavfile_clean(void *f);
extern void wtk_wavfile_set_channel(void *f,int c);
extern void* wtk_arg_new(int argc,char** argv);
extern int wtk_arg_delete(void *arg);
extern int wtk_arg_get_str(void *arg,const char *key,int bytes,char** pv);
extern int wtk_arg_get_int(void *arg,const char *key,int bytes,int* number);

#define wtk_arg_get_int_s(arg,k,n) wtk_arg_get_int(arg,k,sizeof(k)-1,n)
#define wtk_arg_get_str_s(arg,k,pv) wtk_arg_get_str(arg,(char*)k,sizeof(k)-1,pv)

int run;
qtk_hdgd_t *hdgd=NULL;
char*rcd_fn="./tmp.rcd.wav";
char*aec_fn="./tmp.aec.wav";
char*denoise_fn="./tmp.denoise.wav";
char*tts_fn="./tmp.tts.wav";
double t,wav_t;

void* args;
void* record_thread_entry(void *ths)
{
	qtk_hdgd_t* hdgd=(qtk_hdgd_t*)ths;
	void *rcd_q = hdgd->rcd_q;
	qtk_msg_node_t *msg, *msg1;
	strbuf_t *buf;
	int len;
	char* data;

	t=time_get_ms();
	wav_t=0;
	hdgd->debug_chans=10;
#ifdef OFFLINE_RCD
//	char *wavfn="./data/aec_8_1c.wav";  //need adjust res cfg, 8+1
//	char *wavfn="./data/test_gdgd.10c.wav"; //need adjust res cfg, 8+2
//	char *wavfn="/home/dm/Downloads/tmp.rcd.wav"; //need adjust res cfg, 8+2
//	char *wavfn="/home/dm/Downloads/record20230721/record front(inside)/gam-5/tmp.rcd.wav";
	char *wavfn=NULL;
	wtk_arg_get_str_s(args, "wav", &wavfn);
	if (NULL==wavfn)
	{
    	printf("Please give wave or wave-list\n");
    	printf("Usage: ./hdgd -t type [-wav wavfn] [-scp scpfn]\n");
    	exit(0);
	}
    data = file_read_buf(wavfn, &len);
//    printf("len=%d\n", len);
    if (len <= 0) {
        printf("not find wav\n");
        goto end;
    }
    len=len-44;
    data=data+44;
#else
    printf("======prepare record, please speaking...\n");
#endif

	while(run == 1){
#ifndef OFFLINE_RCD
		buf = qtk_record_read(hdgd->rcd);
		if(buf->pos == 0){
			printf("buf->pos = %d\n", buf->pos);
			continue;
		}
		len=buf->pos;
		data=buf->data;
#endif
		if (hdgd->wav_rcd)
		{
			wtk_wavfile_write(hdgd->wav_rcd, data, len);
		}

		if (hdgd->type == qtk_hdgd_task_rcd){
#ifdef OFFLINE_RCD
			break;
#endif
			continue;
		}
		wav_t +=len/hdgd->debug_chans;
		msg = (qtk_msg_node_t*)malloc(sizeof(*msg) + len);
		msg->type=0;
		msg->len = len;
		msg->data = ((char*)msg)+sizeof(*msg);
		memcpy(msg->data, data, len);
//		printf("============push %p\n", msg);
		qtk_queue_push(rcd_q, &(msg->q_n));
#ifdef OFFLINE_RCD
		break;
#endif
	}

	if (hdgd->type == qtk_hdgd_task_rcd){
		printf("rcd task finish, see %s\n", rcd_fn);
		goto end;
	}
	//notify other task end
	msg = (qtk_msg_node_t*)malloc(sizeof(*msg));
	msg->type = 1;
	msg->len = 0;
	msg->data = NULL;
	qtk_queue_push(hdgd->rcd_q, &(msg->q_n));

end:
	return NULL;
}

static void test_etts_on_notify(qtk_session_t *session, qtk_var_t *var) {
	int len;
	char*data;

	if (var->type != QTK_TTS_DATA) return;
	data = var->v.str.data;
	len = var->v.str.len;
	printf("data: %p len=%d\n", data, len);
    if (hdgd->wav_tts && len > 0)
            wtk_wavfile_write(hdgd->wav_tts, data, len);
}

void test_wakeup_on_notify(qtk_session_t *session, qtk_var_t *var) {
	int res;

	res = var->v.i;
	printf("res %d\n",res);
}

void test_easr_on_notify(qtk_session_t *session, qtk_var_t *var) {
//	printf("=============test_easr_on_notify var->type=%d\n", var->type);
    t=time_get_ms()-t;
    wav_t=(wav_t*1000.0/32000);
    printf("dur-wav=%f dur-time=%f,timerate=%f\n",wav_t,t,t/wav_t);
    switch (var->type) {
    case QTK_ASR_TEXT:
        printf("ASR result: %.*s\n", var->v.str.len, var->v.str.data);
//        wtk_sem_release(&sem, 1);
        break;
    case QTK_VAR_ERR:
    	printf("ASR result: no result\n");
//        wtk_sem_release(&sem, 1);
        break;
    default:
        break;
    }
}

void test_denoise_on_notify(qtk_session_t *session, qtk_var_t *var) {
	int len=0;
	char *data=NULL;
	if (var->type == QTK_SPEECH_DATA_PCM)
	{
		//printf("denoise...len=%d\n", var->v.str.len);
//		printf("type=%d\n", hdgd->type);
        if (var->v.str.len > 0)
        {
        	data = var->v.str.data;
        	len = var->v.str.len;   //Notes: samples number
        	//save wav
        	if (hdgd->wav_denoise)
        		wtk_wavfile_write(hdgd->wav_denoise, data, len*2);

        	if (hdgd->type == qtk_hdgd_task_asr && hdgd->asr)  //asr
                qtk_engine_feed(hdgd->asr, data, len * 2, 0);
        	else if (hdgd->type == qtk_hdgd_task_wakup && hdgd->wakup)  //wakeup
        		qtk_engine_feed(hdgd->wakup, data, len , 0);
        }
	}
}

void test_aec_on_notify(qtk_session_t *session, qtk_var_t *var) {
	int len, i,j;
	short *rdata, *data;
	if (var->type == QTK_SPEECH_DATA_PCM)
	{
//		printf("aec...len=%d\n", var->v.str.len);
        if (var->v.str.len > 0)
        {
        	if(hdgd->wav_aec)
        		wtk_wavfile_write(hdgd->wav_aec, var->v.str.data, var->v.str.len);
        	if (hdgd->type == qtk_hdgd_task_aec) return;
//        	printf("========type=%d %d\n", hdgd->type, qtk_hdgd_task_aec);
        	if (hdgd->denoise){
            	rdata =(short*)var->v.str.data;
            	len=var->v.str.len/2;
            	data = malloc(len/2 * sizeof(short));
            	for(j=0; j < len/8; j++)
            		for (i=0; i < 8; i++)
            		{
            			if (i==0 || i==1 || i==2 || i==3)
            			{
            				data[j*4 + i] = rdata[j*8+i];
            			}
            		}
        		qtk_engine_feed(hdgd->denoise, data, len/8, 0);
        		free(data);
        	}
        }
	}
}

void* qdmmudule_thread_entry(void *ths)
{
	qtk_hdgd_t* hdgd=(qtk_hdgd_t*)ths;
	queue_node_t *qn;
	qtk_msg_node_t *msg;

	while(1){
		qn = qtk_queue_pop(hdgd->rcd_q, -1, NULL);
		if(!qn) break;
		msg = (qtk_msg_node_t*)data_offset2(qn,qtk_msg_node_t,q_n);
		if (msg->type==1)
		{
			if (hdgd->asr && hdgd->type==qtk_hdgd_task_asr)
				qtk_engine_feed(hdgd->asr, 0, 0, 1);
			break;
		}
	    if (msg->len > 0) {
	    	if (hdgd->aec)
	    		qtk_engine_feed(hdgd->aec, msg->data, msg->len, 0);
	    }
	    free(msg);
	}

    return NULL;
}

qtk_hdgd_t* qtk_hdgd_new()
{
	qtk_hdgd_t* hdgd;
    qtk_session_t *s = NULL;
    char *params;
    int ret=0;

	hdgd = calloc(1, sizeof(*hdgd));

	hdgd->rcd_q = qtk_queue_new(1);

    params = "appid=57fe0d38-bd8a-11ed-b526-00163e13c8a2;secretkey=42ddc64a-"
             "1c61-39bc-9cfd-0aa8bdd1009c;cache_path=/tmp/out;use_srvsel=1;use_log=1;";
    s = qtk_session_init(params, 0, NULL, NULL);
    printf("session=%p\n", s);

	//rcd
	hdgd->rcd_cfg=qtk_record_cfg_new();
	hdgd->rcd = qtk_record_new(hdgd->rcd_cfg);
	if(!hdgd->rcd){
		printf("rcd new failed.\n");
		ret=-1; goto end;
	}
    //aec
    params =
        "role=aec;cfg=res/sdk/aec/aec.cfg.bin;use_bin=1;use_thread=0;";
    hdgd->aec = qtk_engine_new(s, params);
    if (!hdgd->aec) {
        printf("new aec engine failed.\n");
        ret=-1; goto end;
    }
    qtk_engine_set_notify(hdgd->aec, s, (qtk_engine_notify_f)test_aec_on_notify);
    qtk_engine_start(hdgd->aec);

    //denoise
    //qform9
  params =
//        "role=tinybf;cfg=res/sdk/tinybf/cfg;use_bin=0;use_thread=0;";
    //vboxebf
    params =
        //"role=vboxebf3;cfg=res/sdk/vboxebf/vboxebf.cfg.bin;use_bin=1;use_thread=0;";
    	"role=vboxebf3;cfg=res/sdk/vboxebf/vboxebf.cfg.bin.20230731;use_bin=1;use_thread=0;";
    hdgd->denoise = qtk_engine_new(s, params);
    if (!hdgd->denoise) {
        printf("new denoise engine failed.\n");
        ret=-1; goto end;
    }
    qtk_engine_set_notify(hdgd->denoise, s, (qtk_engine_notify_f)test_denoise_on_notify);
    qtk_engine_start(hdgd->denoise);

    //asr
//    params =
//        "role=asr;cfg=res/sdk/asr/xhy.cfg;use_bin=0;use_thread=0;"
//        "rec_min_conf=1.8;rec_min_conf=1.5;skip_space=1;winStep=100;";
    params =
        "role=asr;cfg=res/sdk/asr/hdgd.cfg;use_bin=0;use_thread=0;";  //update 2023.09.18
    hdgd->asr = qtk_engine_new(s, params);
    if (!hdgd->asr) {
        printf("new asr engine failed.\n");
        ret=-1; goto end;
    }
    qtk_engine_set_notify(hdgd->asr, s, (qtk_engine_notify_f)test_easr_on_notify);
    qtk_engine_start(hdgd->asr);

    //wakeup
    params =
        "role=wakeup;cfg=res/sdk/wakeup/cfg;use_bin=0;use_thread=0;";
    hdgd->wakup = qtk_engine_new(s, params);
    if (!hdgd->wakup) {
        printf("new wakeup engine failed.\n");
        goto end;
    }
    qtk_engine_set_notify(hdgd->wakup, s, (qtk_engine_notify_f)test_wakeup_on_notify);
//    qtk_engine_start(hdgd->wakup);

	//tts
    params = "role=tts;cfg=res/sdk/tts/hts/cfg;use_bin=0;use_thread=0;";
	hdgd->tts = qtk_engine_new(s, params);
    if (!hdgd->tts) {
        printf("new tts engine failed.\n");
        ret=-1; goto end;
    }
    qtk_engine_set_notify(hdgd->tts, s, (qtk_engine_notify_f)test_etts_on_notify);
    qtk_engine_start(hdgd->tts);


    hdgd->use_save_rcd = 1;
    hdgd->use_save_aec = 0;
    hdgd->use_save_denoise = 0;
    hdgd->use_save_tts = 0;

	if (hdgd->use_save_rcd)
	{
	    hdgd->wav_rcd = wtk_wavfile_new(16000);
	    wtk_wavfile_set_channel(hdgd->wav_rcd, 10);
	    if (wtk_wavfile_open(hdgd->wav_rcd, rcd_fn)) {
	        printf("Error Open rcd wav: %s\n", rcd_fn);
	    }
	}

	if (hdgd->use_save_aec)
	{
	    hdgd->wav_aec = wtk_wavfile_new(16000);
	    wtk_wavfile_set_channel(hdgd->wav_aec , 8);
	    if (wtk_wavfile_open(hdgd->wav_aec ,aec_fn)) {
	        printf("Error Open aec wav: %s\n", aec_fn);
	    }
	}

	if (hdgd->use_save_denoise)
	{
	    hdgd->wav_denoise = wtk_wavfile_new(16000);
	    if (wtk_wavfile_open(hdgd->wav_denoise,denoise_fn)) {
	        printf("Error Open qf wav: %s\n", denoise_fn);
	    }
	}

	if (hdgd->use_save_tts)
	{
	    hdgd->wav_tts = wtk_wavfile_new(16000);
	    if (wtk_wavfile_open(hdgd->wav_tts,tts_fn)) {
	        printf("Error Open %s\n", tts_fn);
	    }
	}
end:
	if (ret!=0)
	{
		qtk_hdgd_delete(hdgd);
		hdgd=NULL;
	}
	return hdgd;
}

int qtk_hdgd_reset(qtk_hdgd_t* hdgd)
{
	if (hdgd->aec)
		qtk_engine_reset(hdgd->aec);
	if (hdgd->denoise)
		qtk_engine_reset(hdgd->denoise);
	if (hdgd->asr)
		qtk_engine_reset(hdgd->asr);

	return 0;
}

void qtk_hdgd_delete(qtk_hdgd_t* hdgd)
{
    if (hdgd->aec) {
        qtk_engine_delete(hdgd->aec);
    }
    if (hdgd->denoise ) {
        qtk_engine_delete(hdgd->denoise);
    }
    if (hdgd->asr ) {
        qtk_engine_delete(hdgd->asr);
    }
    if (hdgd->wakup) {
    	qtk_engine_delete(hdgd->wakup);
    }
    if (hdgd->tts) {
    	qtk_engine_delete(hdgd->tts);
    }
	if (hdgd->wav_rcd)
		wtk_wavfile_clean(hdgd->wav_rcd);
	if (hdgd->wav_aec)
		wtk_wavfile_clean(hdgd->wav_aec);
	if (hdgd->wav_denoise)
		wtk_wavfile_clean(hdgd->wav_denoise);
	if(hdgd->rcd){
		qtk_record_delete(hdgd->rcd);
	}
	if (hdgd->rcd_cfg)
		qtk_record_cfg_delete(hdgd->rcd_cfg);
	if (hdgd->rcd_q)
		qtk_queue_delete(hdgd->rcd_q);
	free(hdgd);
}

static void debug_test_asr(qtk_hdgd_t* hdgd, void* args)
{
	char*data, *wavfn=NULL, *scpfn=NULL, *p;
	char wav[1024];
//	FILE *wavscp;
	int len, cnt;

#ifdef OFFLINE_ASR
    printf("=======offline test asr\n");
    wtk_arg_get_str_s(args, "wav", &wavfn);
//    wtk_arg_get_str_s(args, "scp", &scpfn);
    if (NULL==wavfn &&  NULL==scpfn) {
    	printf("Please give wave or wave-list\n");
    	printf("Usage: ./hdgd -t 6 [-wav wavfn] [-scp scpfn]\n");
    	exit(0);
    }

    if (wavfn)
    {
    	//data = file_read_buf("./data/asr_test.wav", &len);
    	printf("wav: %s\n", wavfn);
    	data = file_read_buf(wavfn, &len);
        if (data==NULL || len <= 0) {
            printf("not find wav\n");
            return;
        }
        data= data+44;
        len = len-44;
        qtk_engine_start(hdgd->asr);
        qtk_engine_feed(hdgd->asr, data, len, 0);  //maybe mutli-times feed
        qtk_engine_feed(hdgd->asr, 0, 0, 1);  //only one time
        qtk_engine_reset(hdgd->asr);   //end and get result
        free(data-44);
    }else {
//        wavscp = fopen(scpfn, "r");
//        if (!wavscp) {
//            printf("fail to open wavscp: %s\n", scpfn);
//            return;
//        }
//        for (;;) {
//            if (fgets(wav, sizeof(wav), wavscp) == NULL)
//                break;
//            if (wav[0] == '#')
//                continue;
//            if ((p = strrchr(wav, '\n')) != NULL) *p = 0;
//            printf("wav: %s\n", wav);
//        	data = file_read_buf(wavfn, &len);
//            if (len <= 0) {
//                printf("not find wav\n");
//                return;
//            }
//            data= data+44;
//            len = len-44;
//            qtk_engine_start(hdgd->asr);
//            qtk_engine_feed(hdgd->asr, data, len, 0);  //maybe mutli-times feed
//            qtk_engine_feed(hdgd->asr, 0, 0, 1);  //only one time
//            qtk_engine_reset(hdgd->asr);   //end and get result
//            free(data-44);
//        }
    }
#endif
}

static void debug_test_denoise(qtk_hdgd_t* hdgd, void* args)
{
	char*data, *wavfn=NULL, *scpfn=NULL, *p;
	char wav[1024];
	FILE *wavscp;
	int len, cnt;

#ifdef OFFLINE_DENOISE
	printf("=======offline test denoise\n");
    wtk_arg_get_str_s(args, "wav", &wavfn);
    wtk_arg_get_str_s(args, "scp", &scpfn);
    if (NULL==wavfn &&  NULL==scpfn) {
    	printf("Please give wave or wave-list\n");
    	printf("Usage: ./hdgd -t 6 [-wav wavfn] [-scp scpfn]\n");
    	exit(0);
    }
    //Note: wav must 44 heads.
    int i,j,k;
    int slen, sslen, pos;
    short* pv;
    int tgtchannel, inchannel;

    slen = 32*16;
    inchannel=5;
    tgtchannel=5;
    pv=(short *)calloc(tgtchannel*slen, sizeof(short));

    if (wavfn)
    {
    	//data = file_read_buf("./data/test_gdgd.5c.wav", &len);
    	data = file_read_buf(wavfn, &len);
        if (data==NULL || len <= 0) {
            printf("not find wav\n");
            return;
        }
        pos=44;
        while(pos < len)
        {
        	//memcpy(pv, data+pos, sslen * inchannel * sizeof(short));
        	//select some channel. for input wav > 8channel, select 0|1|2|3channel.
        	if (pos + slen * inchannel * 2 <= len)
        		sslen=slen;
        	else
        		sslen=(len-pos)/(2 * inchannel);

        	//pv = data + pos; following if select some chans.
        	for(j=0; j < sslen; j++)
        	{
        		for (k=0, i=0; i < inchannel; i++)
        		{
        			//select 0, 1, 2, 3, 7
        			if (i < tgtchannel-1 || i==inchannel-1)
        			{
        				pv[j*tgtchannel + k] = ((short*)(data+pos))[j*inchannel+i];
        				k++;
        			}
        		}
        	}

        	qtk_engine_feed(hdgd->denoise, (char*)pv, sslen, 0);
        	pos += sslen * inchannel * sizeof(short);
        }
        qtk_engine_feed(hdgd->denoise,NULL,0,1);
        if(hdgd->use_save_denoise)
        	printf("please see: %s\n", denoise_fn);
    }else
    {
//        wavscp = fopen(scpfn, "r");
//        if (!wavscp) {
//            printf("fail to open wavscp: %s\n", scpfn);
//            return;
//        }
//        for (;;) {
//            if (fgets(wav, sizeof(wav), wavscp) == NULL)
//                break;
//            if (wav[0] == '#')
//                continue;
//            if ((p = strrchr(wav, '\n')) != NULL) *p = 0;
//            printf("wav: %s\n", wav);
//        	data = file_read_buf(wavfn, &len);
//            if (len <= 0) {
//                printf("not find wav\n");
//                return;
//            }
//            pos=44;
//            while(pos < len)
//            {
//            	//memcpy(pv, data+pos, sslen * inchannel * sizeof(short));
//            	//select some channel. for input wav > 8channel, select 0|1|2|3channel.
//            	if (pos + slen * inchannel * 2 <= len)
//            		sslen=slen;
//            	else
//            		sslen=(len-pos)/(2 * inchannel);
//
//            	//pv = data + pos; following if select some chans.
//            	for(j=0; j < sslen; j++)
//            	{
//            		for (k=0, i=0; i < inchannel; i++)
//            		{
//            			//select 0, 1, 2, 3, 7
//            			if (i < tgtchannel-1 || i==inchannel-1)
//            			{
//            				pv[j*tgtchannel + k] = ((short*)(data+pos))[j*inchannel+i];
//            				k++;
//            			}
//            		}
//            	}
//
//            	qtk_engine_feed(hdgd->denoise, (char*)pv, sslen, 0);
//            	pos += sslen * inchannel * sizeof(short);
//            }
//
//            qtk_engine_feed(hdgd->denoise,NULL,0,1);
//            qtk_engine_reset(hdgd->denoise);
//        }
    }

    free(pv);
#endif
}

static void debug_test_wakeup(qtk_hdgd_t* hdgd, void* args)
{
	char*data;
	int len, cnt;

#ifdef OFFLINE_WAKUP
	printf("=======offline test wakeup\n");
	//char* data = file_read_buf("./data/wakeup.xiaoaitongxue.wav", &len);
//		char* wakeupfn= "./data/wakeup.xk.wav";
	char* wakeupfn="/home/dm/Downloads/tmp.denoise.wav";
	data = file_read_buf(wakeupfn, &len);
    if (data==NULL || len <= 0) {
        printf("not find wav\n");
        return;
    }
    data= data+44;
    len = len-44;
    qtk_engine_start(hdgd->wakup);
    cnt=0;
    while(cnt++<3)
    {
    	qtk_engine_feed(hdgd->wakup, data, len, 0);
    }
//        qtk_engine_reset(hdgd->wakup);    //Note: need when qtk_engine_feed(hdgd->wakup, data, len, 1);
    free(data-44);
#endif
}

int main(int argc, char **argv)
{
	pthread_t do_t=0, rcd_t=0;
	int ret=0;
	int type=0;

	args = wtk_arg_new(argc, argv);
    ret = wtk_arg_get_int_s(args, "t", &type);
    if (ret != 0) {
		printf("Usage: %s -t engine-type [-wav wavfn -scp wavscpfn]\n"
				"\t engine-type:\n"
				"\t    0: rcd, ref qtk_hdgd_task_rcd\n"
				"\t    1: aec, ref qtk_hdgd_task_aec\n"
				"\t    2: denoise, ref qtk_hdgd_task_denoise\n"
				"\t    3: asr, ref qtk_hdgd_task_asr\n"
				"\t    4: wakup, ref qtk_hdgd_task_wakup\n"
				"\t    5: tts, ref qtk_hdgd_task_tts\n"
				"\t    6: debug, \n"
				"\t       ./hdgd 6 -wav asr_test.wav\n"
				"\n", argv[0]);
		goto end;
	}

	hdgd = qtk_hdgd_new();
	if (hdgd==NULL) goto end;

	hdgd->type = type;
	switch(type){
	case qtk_hdgd_task_rcd:
		run = 1;
		ret=pthread_create(&rcd_t, NULL, record_thread_entry, hdgd);
		if (ret!=0) goto end;
		getchar();
		run = 0;
		pthread_join(rcd_t, NULL);
		break;
	case qtk_hdgd_task_aec:
	case qtk_hdgd_task_denoise:
	case qtk_hdgd_task_asr:
	case qtk_hdgd_task_wakup:
		ret=pthread_create(&do_t, NULL, qdmmudule_thread_entry, hdgd);
		if (ret!=0) goto end;
		//rcd
		run = 1;
		ret=pthread_create(&rcd_t, NULL, record_thread_entry, hdgd);
		if (ret!=0) goto end;
		getchar();
		run = 0;
		pthread_join(do_t, NULL);
		pthread_join(rcd_t, NULL);
		break;
	case qtk_hdgd_task_tts:
		if (hdgd->tts)
		{
			char* text="苏州奇梦者科技有限公司";
		    qtk_engine_start(hdgd->tts);
		    qtk_engine_feed(hdgd->tts, text, strlen(text), 1);
		    qtk_engine_reset(hdgd->tts);
		    printf("finish tts, see ./tmp.tts.wav\n");
		}
		break;
	case qtk_hdgd_task_debug:
		debug_test_denoise(hdgd, args);
//		debug_test_asr(hdgd, args);
//		debug_test_wakeup(hdgd, args);
		break;
	default:
		printf("type error\n");
	}
end:
	if (hdgd)
		qtk_hdgd_delete(hdgd);
	if (args)
		wtk_arg_delete(args);
}
