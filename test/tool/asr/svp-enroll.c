#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/asr/kws/qtk_kws.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/math/wtk_matrix.h"
#include "wtk/core/math/wtk_math.h"

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

void test_wav_file(qtk_kws_t *dec,char *fn,wtk_rate_t *rate)
{
	double t,t2;
	double wav_t;
	wtk_strbuf_t **buf;
	int n;
	buf=wtk_riff_read_channel(fn,&n);

    printf("==>fn: %s\n",fn);

	wav_t=((buf[0]->pos)*1.0/32);//for 16k
	if(rate)
	{
		rate->wave_time+=wav_t;
	}
	t2=time_get_cpu();
	t=time_get_ms();
	qtk_kws_start(dec);
	{
		char *s,*e;
		int step=4096;
		int nx;
		double nt;

		nt=0;
		s=buf[0]->data;
		e=s+buf[0]->pos;
		while(s<e)
		{
			nx=min(step,e-s);
			nt+=nx/2;
			qtk_kws_feed(dec,s,nx,0);
			s+=nx;
		}
		qtk_kws_feed(dec,0,0,1);
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
	qtk_kws_reset(dec);
        wtk_strbufs_delete(buf, n);
        //	wtk_free(data);
}

void test_scp(qtk_kws_t *dec,char *fn)
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
				qtk_kws_enroll(dec,strs[0]->data,strs[0]->len);
			}else if(!wtk_str_equal(prev->data,prev->len,strs[0]->data,strs[0]->len))
			{
				qtk_kws_enroll_end(dec);
				//qtk_kws_reset(dec);
				prev = strs[0];
				wtk_debug("xxxx\n");
				qtk_kws_enroll(dec,strs[0]->data,strs[0]->len);
			}
			test_wav_file(dec,strs[1]->data,&rate);
		}
		qtk_kws_enroll_end(dec);
		//qtk_kws_reset(dec);
		printf("xtot: time rate=%.3f\n",rate.time/rate.wave_time);
		wtk_flist_delete(f);
	}
	wtk_heap_delete(heap);
	//wtk_flist_delete(f);
}

int main(int argc,char **argv)
{
	wtk_main_cfg_t *main_cfg=0;
	wtk_mbin_cfg_t *mbin_cfg=0;	
	wtk_arg_t *arg;
	qtk_kws_t *dec=0;
	qtk_kws_cfg_t *cfg=NULL;
	char *cfg_fn=0;
	char *scp_fn=0;
	char *wav_fn=0;
	char *feat_fn=0;
	char *f_fn=0;
	int bin=0;
	char *vp_fn=0;
	arg=wtk_arg_new(argc,argv);

	wtk_arg_get_str_s(arg,"c",&cfg_fn);
    wtk_arg_get_str_s(arg,"scp",&scp_fn);
	wtk_arg_get_str_s(arg,"wav",&wav_fn);
	wtk_arg_get_str_s(arg,"featscp",&feat_fn);
	wtk_arg_get_str_s(arg,"f",&f_fn);
	wtk_arg_get_int_s(arg,"b",&bin);
	wtk_arg_get_str_s(arg,"vp",&vp_fn);	
//	if(!cfg_fn || (!scp_fn && !wav_fn))
//	{
//		print_usage(argc,argv);
//		goto end;
//	}

    //wtk_debug("======== rec is ready =============\n");
    if(cfg_fn)
    {

		if(!bin){
			main_cfg=wtk_main_cfg_new_type(qtk_kws_cfg,cfg_fn);
			if(!main_cfg)
			{
				wtk_debug("load configure failed.\n");
				goto end;
			}
			cfg=(qtk_kws_cfg_t*)main_cfg->cfg;
		}else{
			//mbin_cfg = wtk_mbin_cfg_new_type(qtk_kws_cfg,cfg_fn,"./kws.cfg");
			//cfg = (qtk_kws_cfg_t*)mbin_cfg->cfg;
			cfg = qtk_kws_cfg_new_bin(cfg_fn,"./kws.cfg");
		}

		dec=qtk_kws_new(cfg);
		qtk_kws_set_notify(dec,(qtk_kws_res_notify_f)on_kws,NULL);
		if(!dec)
		{
			wtk_debug("create decoder failed.\n");
			goto end;
		}
    }
//    qtk_kws_enroll(dec,"liuyong",0);
//    qtk_kws_enroll(dec,"liuyong",strlen("liuyong"));
	if(vp_fn){
		qtk_kws_set_enroll_fn(dec,vp_fn,sizeof(vp_fn));
	}

	if(wav_fn)
	{
		test_wav_file(dec,wav_fn,NULL);
	}

	if(scp_fn)
	{
		test_scp(dec,scp_fn);
	}

end:

	if(dec)
	{
		qtk_kws_delete(dec);
	}
	if(main_cfg)
	{
		wtk_main_cfg_delete(main_cfg);
	}
	wtk_arg_delete(arg);

	return 0;
}


