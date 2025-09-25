#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/asr/wfst/kwdec/qtk_wakeup_decoder_cfg.h"
#include "wtk/asr/wfst/kwdec/qtk_wakeup_decoder.h"
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
static int cnt =0;

void wtk_rate_init(wtk_rate_t *r)
{
	r->wave_time=0;
	r->cpu_time=0;
	r->time=0;
	r->rescore_time=0;
	r->rec_mem=0;
	r->rescore_mem=0;
}


void test_wav_file(qtk_wakeup_decoder_t *dec,char *fn,wtk_rate_t *rate)
{
	double t,t2;
	double wav_t;
	char *data;
//	wtk_strbuf_t **buf;
	int n,i;
	int len,offset;
//	buf=wtk_riff_read_channel(fn,&n);
	int ox=44;//46+32;
    printf("==>fn: %s\n",fn);
    //printf("%s  [\n",fn);
	data=file_read_buf(fn,&len);
	offset=ox;
	int get =0;
        int b =0;
	float *f;
	int j=0;
	wav_t=((len-ox)*1000.0/32000);
	float fs=0;
	float fe=0;
//	wav_t=((buf[0]->pos)*1.0/32);//for 16k
//	wav_t=((buf[0]->pos)*1000.0/16000);//for 8k
	if(rate)
	{
		rate->wave_time+=wav_t;
	}
	t2=time_get_cpu();
	t=time_get_ms();
//	printf("===============start==================\n");
	qtk_wakeup_decoder_start(dec);
	{
		char *s,*e;
		int step=4000;
		int nx;
		double nt;

		nt=0;
		//tx1=time_get_ms();
		s=data+offset;e=s+len-offset;
//		s=buf[0]->data;
//		e=s+buf[0]->pos;
		while(s<e)
		{
			nx=min(step,e-s);
			nt+=nx/2;
			qtk_wakeup_decoder_feed(dec,s,nx,0);
			s+=nx;
			if(dec->cfg->use_mt==0)
			{
				if(dec->dec->found==1)
				{
					b = 1;
					break;
				}
			}else
			{
				if(dec->asr[0]->dec->found==1)
				{
					b = 1;
					break;
				}
			}
		}
		if(b==0)
		{
			qtk_wakeup_decoder_feed(dec,0,0,1);
		}
		if(dec->cfg->use_mt==0)
		{
			get = dec->dec->found;
		}
		else
		{
			get = dec->asr[0]->dec->found;
		}
	}
	if(get==1)
	{
		cnt ++;
//		wtk_string_t v;
//		qtk_decoder_wrapper_get_result(dec,&v);
//		printf("==>rec: %.*s\n",v.len,v.data);
		qtk_wakeup_decoder_get_wake_time(dec,&fs,&fe);
	}else
	{
		printf("===============no=wake================\n");
		fe = 0;
		fs = 0;
	}
	t2=time_get_cpu()-t2;
	t=time_get_ms()-t;
	if(rate)
	{
		rate->cpu_time+=t2;
		rate->time+=t;
	}

	wtk_debug("time=%f,timerate=%f,cpurate=%f,wake-time[%f,%f]\n",t,t/wav_t,t2/wav_t,fs,fe);
	qtk_wakeup_decoder_reset(dec);
	//for(i=0;i<n;++i)
	//{
	//	wtk_strbuf_delete(buf[i]);
	//}
    //wtk_free(buf);
	wtk_free(data);
}

void test_wav_file2(qtk_wakeup_decoder_t *dec,char *fn,wtk_rate_t *rate)
{
	double t,t2;
	double wav_t;
//	char *data;
	wtk_strbuf_t **buf;
	int n,i;
	int len,offset;
	buf=wtk_riff_read_channel(fn,&n);
        printf("==>fn: %s\n",fn);
        //printf("%s  [\n",fn);
//	data=file_read_buf(fn,&len);
	int get =0;
        int b =0;
	float *f;
	int j=0;
//	wav_t=((len-ox)*1000.0/32000);
	float fs=0;
	float fe=0;
	wav_t=((buf[0]->pos)*1.0/32);//for 16k
//	wav_t=((buf[0]->pos)*1000.0/16000);//for 8k
	if(rate)
	{
		rate->wave_time+=wav_t;
	}
	t2=time_get_cpu();
	t=time_get_ms();
//	printf("===============start==================\n");
	qtk_wakeup_decoder_start(dec);
	{
		char *s,*e;
		int step=4000;
		int nx;
		double nt;
		nt=0;
		//tx1=time_get_ms();
		s=buf[0]->data;
		e=s+buf[0]->pos;
		while(s<e)
		{
			nx=min(step,e-s);
			nt+=nx/2;
			qtk_wakeup_decoder_feed(dec,s,nx,0);
			s+=nx;
			if(dec->cfg->use_mt==0)
			{
				if(dec->dec->found==1)
				{
					b = 1;
					break;
				}
			}else
			{
				if(dec->asr[0]->dec->found==1)
				{
					b = 1;
					break;
				}
			}
		}
		if(b==0)
		{
			qtk_wakeup_decoder_feed(dec,0,0,1);
		}
		if(dec->cfg->use_mt==0)
		{
			get = dec->dec->found;
		}
		else
		{
			get = dec->asr[0]->dec->found;
		}
	}
	if(get==1)
	{
		cnt ++;
//		wtk_string_t v;
//		qtk_decoder_wrapper_get_result(dec,&v);
//		printf("==>rec: %.*s\n",v.len,v.data);
		qtk_wakeup_decoder_get_wake_time(dec,&fs,&fe);
	}else
	{
		fs = fe =0;
		printf("===============no=wake================\n");
	}
	t2=time_get_cpu()-t2;
	t=time_get_ms()-t;
	if(rate)
	{
		rate->cpu_time+=t2;
		rate->time+=t;
	}
        wtk_debug("time=%f,timerate=%f,cpurate=%f,wake-time[%f,%f]\n",t,t/wav_t,t2/wav_t,fs,fe);
        qtk_wakeup_decoder_reset(dec);

	for(i=0;i<n;++i)
	{
		wtk_strbuf_delete(buf[i]);
	}
	wtk_free(buf);
}

void test_scp(qtk_wakeup_decoder_t *dec,char *fn)
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
		//	printf("%s\n",item->str->data);
			test_wav_file2(dec,item->str->data,&rate);
		}
		printf("xtot: time rate=%.3f\n",rate.time/rate.wave_time);
		printf("wake_rate: %d/%d\n",cnt,i);
	wtk_flist_delete(f);
	}
	//wtk_flist_delete(f);
}

int main(int argc,char **argv)
{
	wtk_main_cfg_t *main_cfg=0;
	wtk_arg_t *arg;
	qtk_wakeup_decoder_t *dec=0;
	qtk_wakeup_decoder_cfg_t *cfg=NULL;
	char *cfg_fn=0;
	char *scp_fn=0;
	char *wav_fn=0;
	char *feat_fn=0;
	char *f_fn=0;
	int flag=1;
	int bin=0;
	char* fea_fn=0;

	arg=wtk_arg_new(argc,argv);

	wtk_arg_get_str_s(arg,"c",&cfg_fn);
    wtk_arg_get_str_s(arg,"scp",&scp_fn);
	wtk_arg_get_str_s(arg,"wav",&wav_fn);
	wtk_arg_get_str_s(arg,"f",&f_fn);
	wtk_arg_get_int_s(arg,"b",&bin);
//	if(!cfg_fn || (!scp_fn && !wav_fn))
//	{
//		print_usage(argc,argv);
//		goto end;
//	}

    if(cfg_fn)
    {
		if(bin)
		{
			cfg=qtk_wakeup_decoder_cfg_new_bin(cfg_fn);
		}else
		{
		main_cfg=wtk_main_cfg_new_type(qtk_wakeup_decoder_cfg,cfg_fn);
		if(!main_cfg)
		{
			goto end;
		}
		cfg=(qtk_wakeup_decoder_cfg_t*)main_cfg->cfg;

		}
		if(cfg->kwdec.use_words_set)
		{
			char* words[]={"读三遍","跟我读"};
			int n=2;
			qtk_kwdec_cfg_words_set(&(cfg->kwdec),words,n);
		}
		dec=qtk_wakeup_decoder_new(cfg);
		if(!dec)
		{
			goto end;
		}
    }
	if(wav_fn)
	{
		flag=0;
		test_wav_file(dec,wav_fn,NULL);
	}
//	if(feat_fn)
//	{
//		flag=0;
//		test_feat_scp(dec,feat_fn);
//	}
	if(scp_fn)
	{
		flag=0;
		test_scp(dec,scp_fn);
	}
	wtk_debug("==============total_wake_cnt=%d=====================\n",cnt);

	//if(scp_fn)
	//{
		//test_wav_scp(cfg_fn,scp_fn,log,nproc,bin,usr_net);
	//}
end:

	if(dec)
	{
		qtk_wakeup_decoder_delete(dec);
	}
	if(main_cfg)
	{
		wtk_main_cfg_delete(main_cfg);
	}else
	{
		if(cfg)
		{
			qtk_wakeup_decoder_cfg_delete_bin(cfg);
		}
	}
	wtk_arg_delete(arg);

	return 0;
}


