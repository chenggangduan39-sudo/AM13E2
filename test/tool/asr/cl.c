#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/asr/kws/qtk_sv_cluster_cfg.h"
#include "wtk/asr/kws/qtk_sv_cluster.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/math/wtk_matrix.h"
#include "wtk/core/math/wtk_math.h"


void test_wav_file(qtk_sv_cluster_t *dec,char *fn)
{
//	double t,t2;
//	double wav_t;
	char *data;
//	wtk_strbuf_t **buf;
//	int n,i;
	int len,offset;
//	buf=wtk_riff_read_channel(fn,&n);

	int ox=44;//46+32;
    printf("==>fn: %s\n",fn);
    //printf("%s  [\n",fn);
	data=file_read_buf(fn,&len);
	offset=ox;
//	wav_t=((len-ox)*1000.0/32000);
//	wav_t=((buf[0]->pos)*1.0/32);//for 16k
//	wav_t=((buf[0]->pos)*1000.0/16000);//for 8k

	//t2=time_get_cpu();
	//t=time_get_ms();
	qtk_sv_cluster_start(dec);
	{
		char *s,*e;
		int step=4000;
		int nx;
		double nt;

		nt=0;
		s=data+offset;e=s+len-offset;
//		s=buf[0]->data;
//		e=s+buf[0]->pos;
		while(s<e)
		{
			nx=min(step,e-s);
			nt+=nx/2;
			qtk_sv_cluster_feed(dec,(short*)s,nx/2,0);
			s+=nx;
		}
		qtk_sv_cluster_feed(dec,0,0,1);
	}
	//qtk_sv_cluster_reset2(dec);
	wtk_free(data);
}

void test_scp(qtk_sv_cluster_t *dec,char *fn)
{
	wtk_flist_t *f;
	wtk_queue_node_t *n;
	wtk_fitem_t *item;
	int i;

	f=wtk_flist_new(fn);

	if(f)
	{
		for(i=0,n=f->queue.pop;n;n=n->next,++i)
		{
			item=data_offset(n,wtk_fitem_t,q_n);
//			printf("%s\n",item->str->data);
			test_wav_file(dec,item->str->data);
		}
	//printf("xtot: time rate=%.3f\n",rate.time/rate.wave_time);
	wtk_flist_delete(f);
	}
	//wtk_flist_delete(f);
}

int main(int argc,char **argv)
{
	wtk_main_cfg_t *main_cfg=0;
	wtk_arg_t *arg;
	qtk_sv_cluster_t *dec=0;
	qtk_sv_cluster_cfg_t *cfg=NULL;
	char *cfg_fn=0;
	char *scp_fn=0;
	char *wav_fn=0;
	int bin=0;

	arg=wtk_arg_new(argc,argv);

	wtk_arg_get_str_s(arg,"c",&cfg_fn);
    wtk_arg_get_str_s(arg,"scp",&scp_fn);
	wtk_arg_get_str_s(arg,"wav",&wav_fn);
	wtk_arg_get_int_s(arg,"b",&bin);

    //wtk_debug("======== rec is ready =============\n");
    if(cfg_fn)
    {
		if(bin)
		{
			cfg=qtk_sv_cluster_cfg_new_bin(cfg_fn);
		}else
		{
		main_cfg=wtk_main_cfg_new_type(qtk_sv_cluster_cfg,cfg_fn);
		if(!main_cfg)
		{
			wtk_debug("load configure failed.\n");
			goto end;
		}
		cfg=(qtk_sv_cluster_cfg_t*)main_cfg->cfg;

		}
		dec=qtk_sv_cluster_new(cfg);
		if(!dec)
		{
			wtk_debug("create decoder failed.\n");
			goto end;
		}
    }
    dec->cfg->mode=1;
	if(wav_fn)
	{
		test_wav_file(dec,wav_fn);
	}

	if(scp_fn)
	{
		test_scp(dec,scp_fn);
	}

end:

	if(dec)
	{
		qtk_sv_cluster_delete(dec);
	}
	if(main_cfg)
	{
		wtk_main_cfg_delete(main_cfg);
	}else
	{
		if(cfg)
		{
			qtk_sv_cluster_cfg_delete_bin(cfg);
		}
	}
	wtk_arg_delete(arg);

	return 0;
}


