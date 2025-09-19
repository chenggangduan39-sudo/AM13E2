#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/asr/wfst/k2dec/qtk_k2_wrapper_cfg.h"
#include "wtk/asr/wfst/k2dec/qtk_k2_dec.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/math/wtk_matrix.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/os/wtk_shm.h"
#include "wtk/os/wtk_pid.h"

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

void test_wav_file(qtk_k2_dec_t *dec,char *fn,wtk_rate_t *rate)
{
    double t,t2;
    double wav_t;
    wtk_riff_t *riff = NULL;
    char buf[1024];
    int read_cnt;
    int is_end = 0;
    char *data;
    int len;

    riff = wtk_riff_new();
    wtk_riff_open(riff, fn);
    wtk_string_t v;
    data=file_read_buf(fn,&len);
    wav_t=((len-44)*1000.0/32000);
    if(rate)
    {
            rate->wave_time+=wav_t;
    }
    t2=time_get_cpu();
    t=time_get_ms();

    //printf("%s ",fn);
	qtk_k2_dec_start(dec);
	{
       while (is_end == 0) {
           read_cnt = wtk_riff_read(riff, buf, sizeof(buf));
//            qtk_k2_dec_get_hint_result(dec,&v);
//            printf("%.*s\n",v.len,v.data);
           if (read_cnt < sizeof(buf)) {
               is_end = 1;
           }
           qtk_k2_dec_feed(dec, buf, read_cnt,0);
            //qtk_k2_dec_feed(dec, data+44, len-44,0);
        }
		qtk_k2_dec_feed(dec,0,0,1);
	}

	qtk_k2_dec_get_result(dec,&v);
	printf("%.*s\n",v.len,v.data);
    t2=time_get_cpu()-t2;
    t=time_get_ms()-t;
    if(rate)
    {
            rate->cpu_time+=t2;
            rate->time+=t;
    }
    //wtk_debug("%f time=%f,timerate=%f,cpurate=%f\n",wav_t,t,t/wav_t,t2/wav_t);

	qtk_k2_dec_reset(dec);
	wtk_free(data);
	wtk_riff_delete(riff);
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

	m=wtk_matrix_new2(cnt,3045);
//	m=wtk_matrix_new2(cnt,5373);

	ret=wtk_source_read_matrix(src,m,0);
//	wtk_matrix_print(m);
	tmul->col=3045;
//	tmul->col=5373;
	tmul->m=m;

	return ret;
}

void test_scp(qtk_k2_dec_t *dec,char *fn)
{
	wtk_flist_t *f;
	wtk_queue_node_t *n;
	wtk_fitem_t *item;
	//test_mul_t* tmul=(test_mul_t*)malloc(sizeof(test_mul_t));
	wtk_rate_t rate;
	int i,len;

	f=wtk_flist_new(fn);
	wtk_rate_init(&(rate));
    wtk_array_t *a;
    wtk_heap_t *heap = wtk_heap_new(4096);
    wtk_string_t **strs;
	if(f)
	{
		for(i=0,n=f->queue.pop;n;n=n->next,++i)
		{
			item=data_offset(n,wtk_fitem_t,q_n);
            a = wtk_str_to_array(heap, item->str->data, item->str->len, ' ');
            strs = (wtk_string_t**) a->slot;
            len = strs[1]->len;
            strs[1]->data[len]='\0';            
            printf("%.*s ",strs[0]->len,strs[0]->data);
			test_wav_file(dec,strs[1]->data,&rate);
		}
		//printf("xtot: time rate=%.3f\n",rate.time/rate.wave_time);
		wtk_flist_delete(f);
	}
	//wtk_flist_delete(f);
}

float test_scp3(qtk_k2_dec_t *dec,wtk_flist_t *f,int s,int e)
{
        wtk_fitem_t *item;
        wtk_queue_node_t *n;
        //int ret;
        wtk_rate_t rate;
        int i;
        //char *p;
        int cnt=0;

        wtk_rate_init(&(rate));
        for(i=0,n=f->queue.pop;n;n=n->next,++i)
        {
                if(i<s || (e>0 && i>=e))
                {
                        continue;
                }
                item=data_offset(n,wtk_fitem_t,q_n);
                //if(log!=stdout)
                {
                        printf("%d/%d\n",i,f->queue.length);
                        //printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
            //fflush(stdout);
                }

                ++cnt;
                //fprintf(log,"\"%*.*s.rec\"\n",len,len,item->str->data);
                //wtk_debug("%*.*s\n",len,len,item->str->data);
                test_wav_file(dec,item->str->data,&rate);
                printf("tot: cpu rate=%.3f\n",rate.cpu_time/rate.wave_time);
                printf("tot: time rate=%.3f\n",rate.time/rate.wave_time);
                //printf("tot: eps rate=%.3f t=%.3fms\n",rate.eps_time/rate.wave_time,rate.eps_time/cnt);
                printf("tot: res rate=%.3f t=%.3fms\n",rate.rescore_time/rate.wave_time,rate.rescore_time/cnt);
                printf("tot: rec mem=%.3f M\n",rate.rec_mem/(cnt*1024*1024));
                printf("tot: res mem=%.3f M\n",rate.rescore_mem/(cnt*1024*1024));
        }
        printf("\n\n============ tot result=====================\n");
        printf("wavetime:\t%.3f\n",rate.wave_time);
        printf("xtot: cpu rate=%.3f\n",rate.cpu_time/rate.wave_time);
        printf("xtot: time rate=%.3f\n",rate.time/rate.wave_time);
        //printf("xtot: eps rate=%.3f t=%.3fms\n",rate.eps_time/rate.wave_time,rate.eps_time/cnt);
        printf("xtot: res rate=%.3f t=%.3fms\n",rate.rescore_time/rate.wave_time,rate.rescore_time/cnt);
        printf("xtot: rec mem=%.3f M\n",rate.rec_mem/(cnt*1024*1024));
        printf("xtot: res mem=%.3f M\n",rate.rescore_mem/(cnt*1024*1024));
        printf("=================================================\n\n");
        //ret=0;
        return rate.time/rate.wave_time;
}

void test_scp2(char *cfg_fn,char *fn, int nproc)
{
	wtk_flist_t *f;
	wtk_rate_t rate;
    wtk_shm_t *shm;
    float *fp;
    int i,s,e,n;
    pid_t pid;
    wtk_main_cfg_t *main_cfg=0;
    qtk_k2_dec_t *dec=0;
    qtk_k2_wrapper_cfg_t *cfg=NULL;
    float tot_rate;

	f=wtk_flist_new(fn);
	wtk_rate_init(&(rate));

    shm=wtk_shm_new(sizeof(float)*nproc);
    fp=(float*)shm->addr;
    n=f->queue.length/nproc;

    for(i=0,s=0,e=n;i<nproc;++i)
    {
        if(i==(nproc-1))
        {
                e=-1;
        }
        pid=fork();
        if(pid==0)
        {
            float r;

            {
                    main_cfg=wtk_main_cfg_new_type(qtk_k2_wrapper_cfg,cfg_fn);
                    if(!main_cfg)
                    {
                            wtk_debug("load configure failed.\n");
                    }
                    cfg=(qtk_k2_wrapper_cfg_t*)main_cfg->cfg;
            }
    		dec=qtk_k2_dec_new(cfg);
    		r = test_scp3(dec,f,s,e);
            fp[i]=r;
        }else
        {
            wtk_debug("fork pid=[%x]\n",pid);
        }

        s+=n;
        e+=n;
    }

    for(i=0;i<nproc;++i)
    {
            pid=waitpid(-1,0,0);
            wtk_debug("wait pid=[%x]\n",pid);
    }
    tot_rate=0;
    for(i=0;i<nproc;++i)
    {
            tot_rate+=fp[i];
    }
    wtk_debug("=================== rate ================\n");
    printf("nproc:\t%d\n",nproc);
    printf("rate:\t%.3f\n",tot_rate/nproc);
    wtk_shm_delete(shm);

	//wtk_flist_delete(f);
}


int main(int argc,char **argv)
{
	wtk_main_cfg_t *main_cfg=0;
	wtk_arg_t *arg;
	qtk_k2_dec_t *dec=0;
	qtk_k2_wrapper_cfg_t *cfg=NULL;
	char *cfg_fn=0;
	char *scp_fn=0;
	char *wav_fn=0;
	char *feat_fn=0;
	char *f_fn=0;
    int nproc=1;
	int bin=0;
	arg=wtk_arg_new(argc,argv);

	wtk_arg_get_str_s(arg,"c",&cfg_fn);
	wtk_arg_get_str_s(arg,"scp",&scp_fn);
	wtk_arg_get_str_s(arg,"wav",&wav_fn);
	wtk_arg_get_str_s(arg,"featscp",&feat_fn);
	wtk_arg_get_str_s(arg,"f",&f_fn);
	wtk_arg_get_int_s(arg,"b",&bin);
    wtk_arg_get_int_s(arg,"nproc",&nproc);

    if(nproc > 1)
    {
		test_scp2(cfg_fn,scp_fn,nproc);
    	return 0;
    }
//	if(!cfg_fn || (!scp_fn && !wav_fn))
//	{
//		print_usage(argc,argv);
//		goto end;
//	}

    //wtk_debug("======== rec is ready =============\n");
    if(cfg_fn)
    {
		if(bin)
		{
			cfg=qtk_k2_wrapper_cfg_new_bin(cfg_fn);
		}else
		{
		main_cfg=wtk_main_cfg_new_type(qtk_k2_wrapper_cfg,cfg_fn);
		if(!main_cfg)
		{
			wtk_debug("load configure failed.\n");
			goto end;
		}
		cfg=(qtk_k2_wrapper_cfg_t*)main_cfg->cfg;

		}
		dec=qtk_k2_dec_new(cfg);
		if(!dec)
		{
			wtk_debug("create k2 failed.\n");
			goto end;
		}
    }
	if(wav_fn)
	{
		test_wav_file(dec,wav_fn,NULL);
	}
	if(scp_fn)
	{
		test_scp(dec,scp_fn);
	}
/*	main_cfg=wtk_main_cfg_new_type(qtk_k2_dec_cfg,cfg_fn);
    if(!main_cfg)
    {
            wtk_debug("load configure failed.\n");
            goto end;
    }
    cfg=(qtk_k2_dec_cfg_t*)main_cfg->cfg;


    dec=qtk_k2_dec_new(cfg);
    if(!dec)
    {
            wtk_debug("create k2 failed.\n");
            goto end;
    }*/
	//if(scp_fn)
	//{
		//test_wav_scp(cfg_fn,scp_fn,log,nproc,bin,usr_net);
	//}
end:

	if(dec)
	{
		qtk_k2_dec_delete(dec);
	}
	if(main_cfg)
	{
		wtk_main_cfg_delete(main_cfg);
	}else
	{
		if(cfg)
		{
			qtk_k2_wrapper_cfg_delete_bin(cfg);
		}
	}
	wtk_arg_delete(arg);

	return 0;
}


