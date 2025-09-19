#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/asr/wfst/kaldifst/qtk_decoder_wrapper_cfg.h"
#include "wtk/asr/wfst/kaldifst/qtk_decoder_wrapper.h"
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


void test_wav_file(qtk_decoder_wrapper_t *dec,char *fn,wtk_rate_t *rate)
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
	data=file_read_buf(fn,&len);
	offset=ox;
	wav_t=((len-ox)*1000.0/32000);
//	wav_t=((buf[0]->pos)*1.0/32);//for 16k
//	wav_t=((buf[0]->pos)*1000.0/16000);//for 8k
	if(rate)
	{
		rate->wave_time+=wav_t;
	}
	t2=time_get_cpu();
	t=time_get_ms();
        wtk_string_t v2;
	qtk_decoder_wrapper_start(dec);
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
			qtk_decoder_wrapper_feed(dec,s,nx,0);
// sleep(1);
 qtk_decoder_wrapper_get_hint_result(dec,&v2);
//			qtk_decoder_wrapper_feed_test_feat(dec,0);
			s+=nx;
		}
//		wtk_debug("want end %f time=%f\n",time_get_ms()-tx1,nt*1.0/8000);
//		qtk_decoder_wrapper_feed_test_feat(dec,1);
		qtk_decoder_wrapper_feed(dec,0,0,1);
//		wtk_debug("want end delay= %f\n",time_get_ms()-tx1);
	}
	wtk_string_t v;
	qtk_decoder_wrapper_get_result(dec,&v);
	printf("==>rec: %.*s\n",v.len,v.data);
	t2=time_get_cpu()-t2;
	t=time_get_ms()-t;
	if(rate)
	{
		rate->cpu_time+=t2;
		rate->time+=t;
	}
	//wtk_debug("time=%f,timerate=%f,cpurate=%f\n",t,t/wav_t,t2/wav_t);
	qtk_decoder_wrapper_reset(dec);
	//for(i=0;i<n;++i)
	//{
	//	wtk_strbuf_delete(buf[i]);
	//}
    //wtk_free(buf);
	wtk_free(data);
}

typedef struct test_mul test_mul_t;
struct test_mul
{
	wtk_matrix_t *m;
	int row;
	int col;
};

int read_matrix(test_mul_t *tmul,wtk_source_t *src)
{
	int ret;
	int cnt;
	//int v;
	wtk_matrix_t *m=0;
	//wtk_strbuf_t *buf;

	//buf=wtk_strbuf_new(256,1);
	ret=wtk_source_read_int(src,&cnt,1,0);
	tmul->row=cnt;

	m=wtk_matrix_new2(cnt,5692);
//	m=wtk_matrix_new2(cnt,5373);

	ret=wtk_source_read_matrix(src,m,0);
//	wtk_matrix_print(m);
	tmul->col=5692;
//	tmul->col=5373;
	tmul->m=m;

	return ret;
}

void test_feat(qtk_decoder_wrapper_t *dec,char *fn)
{

        wtk_source_loader_t sl;
        test_mul_t* tmul=(test_mul_t*)malloc(sizeof(test_mul_t));
        //int i;
	float t;
        sl.hook=0;
        sl.vf=wtk_source_load_file_v;

                        printf("%s\n",fn);
                        wtk_source_loader_load(&sl,tmul,(wtk_source_load_handler_t)read_matrix,fn);
                        qtk_decoder_wrapper_start(dec);
        t=time_get_ms();                
	qtk_decoder_wrapper_feed_test_feat2(dec,tmul->m,tmul->row);
                        //qtk_decoder_wrapper_reset(dec);
	t=time_get_ms()-t;                        //wtk_free(tmul->m);
}


void test_feat_scp(qtk_decoder_wrapper_t *dec,char *fn)
{
	wtk_flist_t *f;
	wtk_queue_node_t *n;
	wtk_fitem_t *item;
	wtk_source_loader_t sl;
	test_mul_t* tmul=(test_mul_t*)malloc(sizeof(test_mul_t));
	int i;
	double t=0.0;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;
	f=wtk_flist_new(fn);
	t=time_get_ms();
	//wtk_debug("start:%f\n",t);
	if(f)
	{
		for(i=0,n=f->queue.pop;n;n=n->next,++i)
		{
			item=data_offset(n,wtk_fitem_t,q_n);
//			printf("%s\n",item->str->data);
			wtk_source_loader_load(&sl,tmul,(wtk_source_load_handler_t)read_matrix,item->str->data);
			qtk_decoder_wrapper_start(dec);

			qtk_decoder_wrapper_feed_test_feat2(dec,tmul->m,tmul->row);
			//t=time_get_ms()-t;

			qtk_decoder_wrapper_reset(dec);
			//wtk_free(tmul->m);
		}
	}
	wtk_debug("cal rate: %f\n",time_get_ms()-t);
}

void test_scp(qtk_decoder_wrapper_t *dec,char *fn)
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
		printf("xtot: time rate=%.3f\n",rate.time/rate.wave_time);
	wtk_flist_delete(f);
	}
	//wtk_flist_delete(f);
}

int main(int argc,char **argv)
{
	wtk_main_cfg_t *main_cfg=0;
	wtk_arg_t *arg;
	qtk_decoder_wrapper_t *dec=0;
	qtk_decoder_wrapper_cfg_t *cfg=NULL;
	char *cfg_fn=0;
	char *scp_fn=0;
	char *wav_fn=0;
	char *feat_fn=0;
	char *f_fn=0;
	int flag=1;
//	int bin=0;

	arg=wtk_arg_new(argc,argv);

	wtk_arg_get_str_s(arg,"c",&cfg_fn);
    wtk_arg_get_str_s(arg,"scp",&scp_fn);
	wtk_arg_get_str_s(arg,"wav",&wav_fn);
	wtk_arg_get_str_s(arg,"featscp",&feat_fn);
	wtk_arg_get_str_s(arg,"f",&f_fn);
//	if(!cfg_fn || (!scp_fn && !wav_fn))
//	{
//		print_usage(argc,argv);
//		goto end;
//	}

    wtk_debug("======== rec is ready =============\n");
    if(cfg_fn)
    {
		main_cfg=wtk_main_cfg_new_type(qtk_decoder_wrapper_cfg,cfg_fn);
		if(!main_cfg)
		{
			wtk_debug("load configure failed.\n");
			goto end;
		}
		cfg=(qtk_decoder_wrapper_cfg_t*)main_cfg->cfg;


		dec=qtk_decoder_wrapper_new(cfg);
		if(!dec)
		{
			wtk_debug("create decoder failed.\n");
			goto end;
		}
    }
	if(wav_fn)
	{
		flag=0;
		test_wav_file(dec,wav_fn,NULL);
	}
	if(feat_fn)
	{
		flag=0;
		test_feat_scp(dec,feat_fn);
	}
	if(scp_fn)
	{
		flag=0;
		test_scp(dec,scp_fn);
	}
/*	main_cfg=wtk_main_cfg_new_type(qtk_decoder_wrapper_cfg,cfg_fn);
                if(!main_cfg)
                {
                        wtk_debug("load configure failed.\n");
                        goto end;
                }
                cfg=(qtk_decoder_wrapper_cfg_t*)main_cfg->cfg;


                dec=qtk_decoder_wrapper_new(cfg);
                if(!dec)
                {
                        wtk_debug("create decoder failed.\n");
                        goto end;
                }*/
	if(f_fn)
	{	flag=0;
		test_feat(dec,f_fn);
	}
	if(flag)
	{
		qtk_decoder_wrapper_start(dec);
		qtk_decoder_wrapper_feed_test_feat(dec,0);
	}

	//if(scp_fn)
	//{
		//test_wav_scp(cfg_fn,scp_fn,log,nproc,bin,usr_net);
	//}
end:

	if(dec)
	{
		qtk_decoder_wrapper_delete(dec);
	}
	if(main_cfg)
	{
		wtk_main_cfg_delete(main_cfg);
	}//else
//	{
//		if(cfg)
//		{
//			wtk_wfstdec_cfg_delete_bin(cfg);
//		}
//	}
	wtk_arg_delete(arg);

	return 0;
}


