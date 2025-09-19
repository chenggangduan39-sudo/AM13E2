#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/asr/qtk_wakeup_wrapper_cfg.h"
#include "wtk/asr/qtk_wakeup_wrapper.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/math/wtk_matrix.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/os/wtk_pid.h"
#include "wtk/asr/vad/wtk_vad.h"
#include "wtk/core/json/wtk_json_parse.h"
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
int wake_cnt = 0;
void test_wav_file(qtk_wakeup_wrapper_t *dec,char *fn,wtk_rate_t *rate)
{
	wtk_riff_t *riff = NULL;
	char buf[1024];
	int read_cnt;
	int is_end = 0,ret = 0;

	riff = wtk_riff_new();
	wtk_riff_open(riff, fn);
	//wtk_string_t v;
	//printf("%s ",fn);
	qtk_wakeup_wrapper_start(dec);
	{
		while (is_end == 0) {
			read_cnt = wtk_riff_read(riff, buf, sizeof(buf));
	//            qtk_wakeup_wrapper_get_hint_result(dec,&v);
	//            printf("%.*s\n",v.len,v.data);
			if (read_cnt < sizeof(buf)) {
				is_end = 1;
			}
			ret = qtk_wakeup_wrapper_feed(dec, buf, read_cnt,0);
			if(ret == 1){
				wake_cnt++;
				wtk_debug("wakeup\n");
			}
		}
		ret = qtk_wakeup_wrapper_feed(dec,0,0,1);
		if(ret == 1){
			wake_cnt++;
			wtk_debug("wakeup\n");
		}
	}

	// qtk_wakeup_wrapper_get_result(dec,&v);
	// printf("%.*s\n",v.len,v.data);
	qtk_wakeup_wrapper_reset(dec);
	wtk_riff_delete(riff);
}

void test_scp(qtk_wakeup_wrapper_t *dec,char *fn)
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
			printf("%s\n",item->str->data);
			test_wav_file(dec,item->str->data,&rate);
		}
//		printf("xtot: time rate=%.3f\n",rate.time/rate.wave_time);
		wtk_flist_delete(f);
	}
	//wtk_debug("tot:%d\n",wake_cnt);
	//wtk_flist_delete(f);
}


int main(int argc,char **argv)
{
	wtk_main_cfg_t *main_cfg=0;
	wtk_arg_t *arg;
	qtk_wakeup_wrapper_t *dec=0;
	qtk_wakeup_wrapper_cfg_t *cfg=NULL;
	char *cfg_fn=0;
	char *scp_fn=0;
	char *wav_fn=0;
	char *h_fn=0;
	int bin=0;
	arg=wtk_arg_new(argc,argv);
	wtk_arg_get_str_s(arg,"hw",&h_fn);
	wtk_arg_get_str_s(arg,"c",&cfg_fn);
	wtk_arg_get_str_s(arg,"scp",&scp_fn);
	wtk_arg_get_str_s(arg,"wav",&wav_fn);
	wtk_arg_get_int_s(arg,"b",&bin);

	//wtk_debug("======== rec is ready =============\n");
	if(cfg_fn)
	{
		if(bin)
		{
			cfg=qtk_wakeup_wrapper_cfg_new_bin(cfg_fn);
		}else
		{
			main_cfg=wtk_main_cfg_new_type(qtk_wakeup_wrapper_cfg,cfg_fn);
			if(!main_cfg)
			{
				wtk_debug("load configure failed.\n");
				goto end;
			}
			cfg=(qtk_wakeup_wrapper_cfg_t*)main_cfg->cfg;
		}
		dec=qtk_wakeup_wrapper_new(cfg);
		if(!dec)
		{
			wtk_debug("create k2 failed.\n");
			goto end;
		}
	}

	if(h_fn)
	{
		char *data;
		int len;
		data = file_read_buf(h_fn,&len);
		qtk_wakeup_wrapper_set_context_wakeup(dec,data,len);
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
		qtk_wakeup_wrapper_delete(dec);
	}
	if(main_cfg)
	{
		wtk_main_cfg_delete(main_cfg);
	}else
	{
		if(cfg)
		{
			qtk_wakeup_wrapper_cfg_delete_bin(cfg);
		}
	}
	wtk_arg_delete(arg);

	return 0;
}


