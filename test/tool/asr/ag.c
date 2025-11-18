#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/asr/nnwrap/qtk_nnwrap.h"
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

void on_kws(void *ths,int res,char *name,int name_len)
{
	printf("wakeup:%.*s\n",name_len,name);
}

void test_wav_file(qtk_nnwrap_t *dec,char *fn,wtk_rate_t *rate)
{
    wtk_riff_t *riff = NULL;
    char buf[1024];
    int read_cnt;
    int is_end = 0;
    int age, gender;

    riff = wtk_riff_new();
    wtk_riff_open(riff, fn);
    //fprintf(fppp,"%s ",fn);
    printf("%s ",fn);
	qtk_nnwrap_start(dec);
    {
        while (is_end == 0) {
            read_cnt = wtk_riff_read(riff, buf, sizeof(buf));
            //printf("%.*s\n",v.len,v.data);
            if (read_cnt < sizeof(buf)) {
                is_end = 1;
            }
			qtk_nnwrap_feed(dec,buf,read_cnt,0);
        }
		qtk_nnwrap_feed(dec,0,0,1);
    }
    qtk_nnwrap_get_result(dec, &age, &gender);
    wtk_debug("%d %d\n", age, gender);
    qtk_nnwrap_reset(dec);
}

void test_scp(qtk_nnwrap_t *dec,char *fn)
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
	qtk_nnwrap_t *dec=0;
	qtk_nnwrap_cfg_t *cfg=NULL;
	char *cfg_fn=0;
	char *scp_fn=0;
	char *wav_fn=0;
	char *feat_fn=0;
	char *f_fn=0;
	int bin=0;

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
    if(cfg_fn)
    {

		main_cfg=wtk_main_cfg_new_type(qtk_nnwrap_cfg,cfg_fn);
		if(!main_cfg)
		{
			wtk_debug("load configure failed.\n");
			goto end;
		}
		cfg=(qtk_nnwrap_cfg_t*)main_cfg->cfg;

		dec=qtk_nnwrap_new(cfg);
		if(!dec)
		{
			wtk_debug("create decoder failed.\n");
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

end:

	if(dec)
	{
		qtk_nnwrap_delete(dec);
	}
	if(main_cfg)
	{
		wtk_main_cfg_delete(main_cfg);
	}
	wtk_arg_delete(arg);

	return 0;
}


