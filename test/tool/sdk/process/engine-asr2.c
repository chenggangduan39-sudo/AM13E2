#include "wtk/core/cfg/wtk_main_cfg.h"
#include "sdk/qtk_api.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/math/wtk_matrix.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_riff.h"

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
wtk_string_t *eval_name;
int cor = 0;
int tot = 0;
void on_kws(void *ths,int res,char *name,int name_len)
{
	tot++;
	printf("wakeup:%.*s\n",name_len,name);
	if(name && wtk_str_equal(name,name_len,eval_name->data,eval_name->len))
	{
		cor++;
	}
	printf("cor/tot: %d/%d = %f\n",cor,tot,cor*1.0/tot);
}

void test_wav_file(qtk_engine_t *dec,char *fn,wtk_rate_t *rate)
{
	double t,t2;
	double wav_t;
	int n;
	int elen;
	wtk_strbuf_t **buf;
	buf=wtk_riff_read_channel(fn,&n);
	char *edata=file_read_buf(fn, &elen);

	// printf("fn=%s\n",fn);
    printf("%s ",fn);

//	qtk_engine_set(dec, "vad_starttime=1.0;vad_endtime=2.0;");
	// qtk_engine_set(dec, "spk_nums=4;");
	wav_t=((buf[0]->pos)*1.0/32);//for 16k
//	wav_t=((buf[0]->pos)*1000.0/16000);//for 8k
	if(rate)
	{
		rate->wave_time+=wav_t;
	}
	t2=time_get_cpu();
	t=time_get_ms();
	qtk_engine_start(dec);
	{
		char *s,*e;
		int step=1024*n;
		step=4096;
		int nx=0;
		double nt;

		nt=0;
		//tx1=time_get_ms();
//		s=data+offset;e=s+len-offset;
		// s=buf[0]->data;
		// e=s+buf[0]->pos;
		s=edata+44;
		e=s+elen-44;
		while(s<e)
		{
			nx=min(step,e-s);
			nt+=nx/2;
			qtk_engine_feed(dec,s,nx,0);
			// if(nt/(1000*16) > 1)
			// // if(0)
			// {
			// 	qtk_var_t var;
			// 	qtk_engine_get_result(dec, &var);
			// 	if(var.v.str.len > 0)
			// 	{
			// 		wtk_debug("result=%d=[%.*s]\n",var.v.str.len,var.v.str.len,var.v.str.data);
			// 	}
			// 	// break;
			// }
			usleep(128*1000);
			s+=nx;
		}
//		wtk_debug("want end %f time=%f\n",time_get_ms()-tx1,nt*1.0/8000);
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
	// wtk_debug("time=%f,timerate=%f,cpurate=%f\n",t,t/wav_t,t2/wav_t);
	qtk_var_t var;
	qtk_engine_get_result(dec, &var);
	// wtk_debug("result=%d=[%.*s]\n",var.v.str.len,var.v.str.len,var.v.str.data);
	printf("%.*s\n",var.v.str.len,var.v.str.data);
	qtk_engine_reset(dec);
	wtk_strbufs_delete(buf, n);
	wtk_free(edata);
}

void test_scp(qtk_engine_t *dec,char *fn)
{
	wtk_flist_t *f;
	wtk_queue_node_t *n;
	wtk_fitem_t *item;
	//test_mul_t* tmul=(test_mul_t*)malloc(sizeof(test_mul_t));
	wtk_rate_t rate;
	int i;

	f=wtk_flist_new(fn);
	wtk_rate_init(&(rate));

	if(f)
	{
		for(i=0,n=f->queue.pop;n;n=n->next,++i)
		{
			item=data_offset(n,wtk_fitem_t,q_n);
//			printf("%s\n",item->str->data);
			test_wav_file(dec,item->str->data,&rate);
		}
//		printf("xtot: time rate=%.3f\n",rate.time/rate.wave_time);
		wtk_flist_delete(f);
	}
	//wtk_flist_delete(f);
}

void etest_on_notify(qtk_engine_t *eb,qtk_var_t *v)
{
	switch(v->type){
		case QTK_SPEECH_DATA_PCM:
			break;
		case QTK_AEC_DIRECTION:
			printf("direction:nbest=%d theta %d phi %d\n",v->v.ii.nbest ,v->v.ii.theta, v->v.ii.phi);
			break;
		case QTK_ASR_TEXT:
		case QTK_EVAL_TEXT:
			// printf("====================>>>>>>>>>>>>>[%.*s]\n",v->v.str.len,v->v.str.data);
			tot++;
			// printf("idx=%d res=[%.*s] prob=%f\n",v->v.str2.idx,v->v.str2.len,v->v.str2.data,v->v.str2.prob);
			// printf("res:%.*s prob=%f\n",v->v.str.len,v->v.str.data, qtk_engine_get_prob(eb));
			printf("res:%.*s\n",v->v.str.len,v->v.str.data);
			// if(v->v.str.data && wtk_str_equal(v->v.str.data,v->v.str.len,eval_name->data,eval_name->len))
			// {
			// 	cor++;
			// }
			// printf("cor/tot: %d/%d = %f\n",cor,tot,cor*1.0/tot);
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
	char *feat_fn=0;
	char *f_fn=0;
	int bin=0;


	qtk_session_t *session;
	char *params;

	params = "appid=613679ec-9cf0-11eb-b526-00163e13c8a2;secretkey=f358afec-f82a-3fb2-be2d-ed096070b83e;"
						"cache_path=./qvoice;log_wav=0;use_timer=1;";

	session = qtk_session_init(params,QTK_WARN,NULL,(qtk_errcode_handler)test_module_on_errcode);
	if(!session) {
			printf("session init failed.\n");
			exit(1);
	}

	params = "role=asr;cfg=./res-tmp/csr/csr/asr_cfg;use_bin=0;use_thread=0;use_lex=0;";
	// params = "role=asr;cfg=./res/asr-local/asr.cfg;use_bin=0;use_thread=0;use_lex=0;";
	
	ee = qtk_engine_new(session,params);
	if(!ee){
		printf("engine new failed\n");
		goto end;
	}
	qtk_engine_set_notify(ee,ee,(qtk_engine_notify_f)etest_on_notify);


	arg=wtk_arg_new(argc,argv);

	wtk_arg_get_str_s(arg,"c",&cfg_fn);
    wtk_arg_get_str_s(arg,"scp",&scp_fn);
	wtk_arg_get_str_s(arg,"wav",&wav_fn);
	wtk_arg_get_str_s(arg,"featscp",&feat_fn);
	wtk_arg_get_str_s(arg,"f",&f_fn);
	wtk_arg_get_int_s(arg,"b",&bin);
//	if(!cfg_fn || (!scp_fn && !wav_fn))
//	{
//		print_usage(argc,argv);
//		goto end;
//	}

    //wtk_debug("======== rec is ready =============\n");
	// qtk_engine_set(ee, "enroll_fn=./mem.bin;");
	// qtk_engine_set(ee, "enroll_fn=./vpres/vp.bin;");
	// qtk_engine_set(ee, "enroll_fn=./vp.bin;");
	//qtk_engine_set(ee, "enroll_name=test;enroll_namelen=0;enroll_isend=0;");
	if(wav_fn)
	{
		test_wav_file(ee,wav_fn,NULL);
	}

	if(scp_fn)
	{
		test_scp(ee,scp_fn);
	}

end:
	if(ee){
		qtk_engine_delete(ee);
	}
	qtk_session_exit(session);
	wtk_arg_delete(arg);

	return 0;
}


