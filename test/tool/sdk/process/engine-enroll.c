#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/math/wtk_matrix.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_riff.h"
#include "sdk/qtk_api.h"
#include "wtk/core/wtk_wavfile.h"

wtk_wavfile_t *owav=NULL;

typedef struct wtk_rate
{
	double wave_time;
	double cpu_time;
	double time;
	double rescore_time;
	double rec_mem;
	double rescore_mem;
}wtk_rate_t;


void wtk_rate_init(wtk_rate_t *r)
{
	r->wave_time=0;
	r->cpu_time=0;
	r->time=0;
	r->rescore_time=0;
	r->rec_mem=0;
	r->rescore_mem=0;
}

void on_kws(void *ths,int res,char *name,int name_len)
{
	printf("wakeup:%.*s\n",name_len,name);
}

void test_wav_file(qtk_engine_t *dec,char *fn,wtk_rate_t *rate)
{
	double t,t2;
	double wav_t;
	wtk_strbuf_t **buf=NULL;
	int n;
	int elen;
	// buf=wtk_riff_read_channel(fn,&n);
	char *edata=file_read_buf(fn, &elen);

    printf("==>fn: %s\n",fn);

	// wav_t=((buf[0]->pos)*1.0/32);//for 16k
	wav_t = ((elen-44)*1.0/32/8.0);
	if(rate)
	{
		rate->wave_time+=wav_t;
	}
	t2=time_get_cpu();
	t=time_get_ms();
	qtk_engine_start(dec);
	{
		char *s,*e;
		int step=32*32*8;
		int nx=0;
		double nt;
		nt=0;
		// s=buf[0]->data;
		// e=s+buf[0]->pos;
		s=edata+44;
		e=s+elen-44;
		while(s<e)
		{
			nx=min(step,e-s);
			nt+=nx/2;
			qtk_engine_feed(dec,s,nx,0);
			s+=nx;
		}
		qtk_engine_feed(dec,0,0,1);
//		wtk_debug("want end delay= %f\n",time_get_ms()-tx1);
	}
	t2=time_get_cpu()-t2;
	t=time_get_ms()-t;
	if(rate)
	{
		rate->cpu_time+=t2;
		rate->time+=t;
	}
	wtk_debug("time=%f,timerate=%f,cpurate=%f\n",t,t/wav_t,t2/wav_t);
	qtk_engine_reset(dec);
	// wtk_strbufs_delete(buf,n);
	wtk_free(edata);
}

void test_scp(qtk_engine_t *dec,char *fn)
{
	wtk_flist_t *f;
	wtk_queue_node_t *n;
	wtk_fitem_t *item;
	//test_mul_t* tmul=(test_mul_t*)malloc(sizeof(test_mul_t));
	wtk_rate_t rate;
	int i,len;
	wtk_array_t *a;
	wtk_heap_t *heap = wtk_heap_new(4096);
	wtk_string_t **strs;
	wtk_string_t *prev = NULL;
	char testenroll[1024]={0};

	f=wtk_flist_new(fn);
	wtk_rate_init(&(rate));
	if(f)
	{
		for(i=0,n=f->queue.pop;n;n=n->next,++i)
		{
			item=data_offset(n,wtk_fitem_t,q_n);
//			printf("%s\n",item->str->data);
			a = wtk_str_to_array(heap, item->str->data, item->str->len, ' ');
			strs = (wtk_string_t**) a->slot;
			wtk_debug("%.*s\n",strs[0]->len,strs[0]->data);
			len = strs[1]->len;
			strs[1]->data[len]='\0';
			if(!prev)
			{
				prev = strs[0];
				// qtk_kws_enroll(dec,strs[0]->data,strs[0]->len);
				memset(testenroll, 0, 1024);
				snprintf(testenroll, 1024, "enroll_name=\"%.*s\";enroll_isend=0;",strs[0]->len,strs[0]->data);
				wtk_debug("=================>>>>>>>>>>>>>>>>>>>>data[%.*s] len=%d\n",1024,testenroll,strs[0]->len);
				qtk_engine_set(dec, testenroll);
			}else if(!wtk_str_equal(prev->data,prev->len,strs[0]->data,strs[0]->len))
			{
				// qtk_kws_enroll_end(dec);
				memset(testenroll, 0, 1024);
				snprintf(testenroll, 1024, "enroll_name=\"%s\";enroll_isend=1;","test");
				qtk_engine_set(dec, testenroll);
				//qtk_kws_reset(dec);
				prev = strs[0];
				wtk_debug("xxxx len=%d\n",strs[0]->len);
				// qtk_kws_enroll(dec,strs[0]->data,strs[0]->len);
				memset(testenroll, 0, 1024);
				snprintf(testenroll, 1024, "enroll_name=\"%.*s\";enroll_isend=0;",strs[0]->len,strs[0]->data);
				qtk_engine_set(dec, testenroll);
			}
			test_wav_file(dec,strs[1]->data,&rate);
		}
		memset(testenroll, 0, 1024);
		snprintf(testenroll, 1024, "enroll_name=\"%s\";enroll_isend=1;","test");
		qtk_engine_set(dec, testenroll);
		// qtk_kws_enroll_end(dec);
		//qtk_kws_reset(dec);
		printf("xtot: time rate=%.3f\n",rate.time/rate.wave_time);
		wtk_flist_delete(f);
	}
	wtk_heap_delete(heap);
	//wtk_flist_delete(f);
}

void etest_on_notify(void *eb,qtk_var_t *v)
{
	switch(v->type){
		case QTK_SPEECH_DATA_PCM:
			if(owav && v->v.str.len > 0)
			{
				wtk_wavfile_write(owav, v->v.str.data, v->v.str.len);
			}
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

static void test_module_on_errcode(qtk_session_t *session, int errcode,char *errstr)
{
        printf("====> errcode :%d errstr: %s\n",errcode,errstr);
}

int main(int argc,char **argv)
{
	wtk_arg_t *arg;
	qtk_engine_t *ee=NULL;
	char *cfg_fn=0;
	char *scp_fn=0;
	char *wav_fn=0;
	char *vp_fn="mem.bin";
	char *o_fn=NULL;
	char *name=NULL;
	int ret=-1;

	qtk_session_t *session;
	char *params;

	params = "appid=613679ec-9cf0-11eb-b526-00163e13c8a2;secretkey=f358afec-f82a-3fb2-be2d-ed096070b83e;"
						"cache_path=./qvoice;log_wav=0;use_timer=1;";

	session = qtk_session_init(params,QTK_WARN,NULL,(qtk_errcode_handler)test_module_on_errcode);
	if(!session) {
			printf("session init failed.\n");
			exit(1);
	}

	arg=wtk_arg_new(argc,argv);

	wtk_arg_get_str_s(arg,"c",&cfg_fn);
    wtk_arg_get_str_s(arg,"scp",&scp_fn);
	wtk_arg_get_str_s(arg,"wav",&wav_fn);
	wtk_arg_get_str_s(arg,"o",&o_fn);
	wtk_arg_get_str_s(arg,"name",&name);
	ret = wtk_arg_get_str_s(arg,"vp",&vp_fn);
	if(ret != 0)
	{
		vp_fn="mem.bin";
	}
//	if(!cfg_fn || (!scp_fn && !wav_fn))
//	{
//		print_usage(argc,argv);
//		goto end;
//	}

	char params_role[128]={0};
	snprintf(params_role, 128, "role=kws;cfg=%s;syn=1;use_thread=0;",cfg_fn);
	// params = "role=kws;cfg=./res-kws/engine.cfg;syn=1;use_thread=0;";
	// params = "role=kws;cfg=./res/kws/cfg;syn=1;use_thread=0;";
	params = params_role;
	
	ee = qtk_engine_new(session,params);
	if(!ee){
		printf("engine new failed\n");
		goto end;
	}
	qtk_engine_set_notify(ee,NULL,(qtk_engine_notify_f)etest_on_notify);

	owav = NULL;
	if(o_fn)
	{
		owav = wtk_wavfile_new(16000);
		owav->max_pend = 1;
		wtk_wavfile_open(owav, o_fn);
	}

    //wtk_debug("======== rec is ready =============\n");
	memset(params_role, 0, 128);
	snprintf(params_role, 128, "enroll_fn=%s;",vp_fn);
	qtk_engine_set(ee, params_role);
	// qtk_engine_set(ee, "enroll_fn=./hyw.bin;");
	wtk_debug("wav_fn=%s vp_fn=%s\n",wav_fn, vp_fn);
	if(wav_fn)
	{
		char testcmd[128]={0};
		snprintf(testcmd, 128, "enroll_name=\"%s\";enroll_isend=0;",name);
		qtk_engine_set(ee, testcmd);
	
		test_wav_file(ee,wav_fn,NULL);

		memset(testcmd, 0, 128);
		snprintf(testcmd, 128, "enroll_name=\"%s\";enroll_isend=1;",name);
		qtk_engine_set(ee, testcmd);
	}
	// if(wav_fn)
	// {
	// 	test_wav_file(ee,wav_fn,NULL);
	// }

	if(scp_fn)
	{
		test_scp(ee,scp_fn);
	}

end:
	if(ee){
		qtk_engine_delete(ee);
	}
	qtk_session_exit(session);
	if(owav)
	{
		wtk_wavfile_close(owav);
		wtk_wavfile_delete(owav);
	}
	wtk_arg_delete(arg);
	return 0;
}


