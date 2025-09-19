#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/asr/miccheck/qtk_mic_check.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/cfg/wtk_source.h"
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

void test_wav_file(qtk_mic_check_t *dec,char *fn,wtk_rate_t *rate)
{
	int i,n,*res;
	qtk_mic_check_feed(dec,fn);
    res = qtk_mic_check_get_result(dec, &n);
	for(i = 0; i < n; i++){
		printf("%d\n",res[i]);
	}
    qtk_mic_check_reset(dec);
}

void test_scp(qtk_mic_check_t *dec,char *fn)
{
	wtk_flist_t *f;
	wtk_queue_node_t *n;
	wtk_fitem_t *item;
	wtk_rate_t rate;
	int i;

	f=wtk_flist_new(fn);
	wtk_rate_init(&(rate));

	if(f)
	{
		for(i=0,n=f->queue.pop;n;n=n->next,++i)
		{
			item=data_offset(n,wtk_fitem_t,q_n);
			test_wav_file(dec,item->str->data,&rate);
		}
		//printf("xtot: time rate=%.3f\n",rate.time/rate.wave_time);
		wtk_flist_delete(f);
	}
}

int main(int argc,char **argv)
{
	wtk_main_cfg_t *main_cfg = 0;
	wtk_mbin_cfg_t *mbin_cfg = 0;
	wtk_arg_t *arg;
	qtk_mic_check_t *dec=0;
	qtk_mic_check_cfg_t *cfg=NULL;
	char *cfg_fn=0;
	char *scp_fn=0;
	char *wav_fn=0;
	int bin=0;

	arg=wtk_arg_new(argc,argv);

	wtk_arg_get_str_s(arg,"c",&cfg_fn);
    wtk_arg_get_str_s(arg,"scp",&scp_fn);
	wtk_arg_get_str_s(arg,"wav",&wav_fn);
	wtk_arg_get_int_s(arg,"b",&bin);

    if(cfg_fn){
		if(bin){
			mbin_cfg = wtk_mbin_cfg_new_type(qtk_mic_check_cfg,cfg_fn,"./mc.cfg");
			cfg=(qtk_mic_check_cfg_t*)(mbin_cfg->cfg);
		}else{
			main_cfg=wtk_main_cfg_new_type(qtk_mic_check_cfg,cfg_fn);
			if(!main_cfg){
				wtk_debug("load configure failed.\n");
				goto end;
			}
			cfg=(qtk_mic_check_cfg_t*)main_cfg->cfg;
		}
		dec=qtk_mic_check_new(cfg);
		if(!dec){
			wtk_debug("create mc failed.\n");
			goto end;
		}
    }

	if(wav_fn){
		test_wav_file(dec,wav_fn,NULL);
	}

	if(scp_fn){
		test_scp(dec,scp_fn);
	}

end:

	if(dec){
		qtk_mic_check_delete(dec);
	}
	if(main_cfg){
		wtk_main_cfg_delete(main_cfg);
	}
	if(mbin_cfg){
		wtk_mbin_cfg_delete(mbin_cfg);
	}
	wtk_arg_delete(arg);

	return 0;
}
