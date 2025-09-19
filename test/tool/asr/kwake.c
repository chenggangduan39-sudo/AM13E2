#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/cfg/wtk_opt.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/asr/fextra/kparm/wtk_kxparm.h"
#include "wtk/asr/wakeup/wtk_kwake.h"

#include <sys/types.h>

typedef struct wtk_statm
{
	int size;
	int resident;
	int share;
	int text;
	int lib;
	int data;
	int dt;
}wtk_statm_t;

int wtk_statm_init(wtk_statm_t *m)
{
	char buf[256];
	FILE *f;
	int ret=-1;

	/*
	//116551 98709 520 653 0 114831 0
	* 98033 54203 664 808 0 90780 0
			   size       total program size
						  (same as VmSize in /proc/[pid]/status)
			   resident   resident set size
						  (same as VmRSS in /proc/[pid]/status)
			   share      shared pages (from shared mappings)
			   text       text (code)
			   lib        library (unused in Linux 2.6)
			   data       data + stack
			   dt         dirty pages (unused in Linux 2.6)
	*/
	sprintf(buf,"/proc/%d/statm",getpid());
	f=fopen(buf,"r");
	if(!f){goto end;}
	ret=fscanf(f,"%d %d %d %d %d %d %d",&(m->size),&(m->resident),&(m->share),&(m->text),&(m->lib),&(m->data),&(m->dt));
	if(ret!=7){ret=-1;goto end;}
	ret=0;
	end:
	if(f)
	{
		fclose(f);
	}
	return ret;
}


double wtk_proc_mem()
{
	wtk_statm_t m;
	double x=-1;
	int ret;

	ret=wtk_statm_init(&m);
	if(ret!=0){goto end;}
	x=m.resident*4.0/1024;
	//x=m.size*4.0/1024;
	end:
	return x;
}

int debug=0;

int test_kwdec2_file(wtk_kwake_t *dec,char *fn)
{
	wtk_strbuf_t **bufs;
	int n,i;
	double t;
	char *s,*e;
	int step,cnt;
	int x1=0;
	int rate;
	int ret;
	int xcnt=0;
	int xpos=0;
	float fs,fe;
	int bits;

	bits=wtk_riff_get_samples(fn);
	if(bits==3)
	{
	//	return test_kwdec2_file2(dec,fn);
	}
	rate=8000;
	bufs=wtk_riff_read_channel(fn,&n);
	if(!bufs)
	{
		return 0;
	}
	wtk_kwake_start(dec);
	t=time_get_ms();
	s=bufs[0]->data;
	e=s+bufs[0]->pos;
	//s+=(int)(rate*2*1.6);

	//print_short((short*)s,105);
	cnt=0;
	while(s<e)
	{
		step=min(e-s,128);
		cnt+=step/2;
		x1+=step/2;
		ret=wtk_kwake_feed(dec,(short*)s,step/2,0);

		//wtk_kwake_bytes(dec);

		//wtk_debug("ret=%d get=%d waked=%d\n",ret,dec->get,dec->waked);
		if(ret==1)
		{
			//wtk_debug("final=%p\n",dec->final);
			++xcnt;
			wtk_kwake_get_wake_time(dec,&fs,&fe);
			wtk_debug("wake=[%f,%f] pos=%f\n",fs,fe,xpos*1.0/rate);
			fs+=xpos*1.0/rate;
			fe+=xpos*1.0/rate;
			wtk_kwake_print(dec);
			printf("==>fn: %s\n",fn);
			printf("==> waketime: %.3f %.3f %s at  pos=%f ============\n",fs,fe,fn,xpos*1.0/rate);
			if(debug==1)
			{
				exit(0);
			}
			wtk_kwake_reset(dec);
			//exit(0);
			wtk_kwake_start(dec);
			xpos=cnt;
			//exit(0);
			if(0)
			{
				static int ki=0;

				++ki;
				if(ki>1)
				{
					wave_write_file("tmp.wav",8000,s+step,(e-s-step));
					exit(0);
				}
			}

		}
		s+=step;
	}
	ret=wtk_kwake_feed(dec,NULL,0,1);
	if(ret==1)
	{
		//wtk_debug("final=%p\n",dec->final);
		++xcnt;
		wtk_kwake_get_wake_time(dec,&fs,&fe);
		fs+=xpos*1.0/rate;
		fe+=xpos*1.0/rate;
		wtk_kwake_print(dec);
		printf("==>fn: %s\n",fn);
		printf("==> waketime: %.3f %.3f %s at  pos=%f ============\n",fs,fe,fn,xpos*1.0/rate);
	}else if(0)
	{
		wtk_debug("============= check ============\n");
		wave_write_file("tmp.wav",8000,bufs[0]->data+xpos*2,bufs[0]->pos-xpos*2);
		exit(0);
	}
//	if(dec->cfg->use_dec && dec->final)
//	{
//		wtk_kwake_print(dec);
//	}
	t=time_get_ms()-t;
	wtk_debug("time=%f rate=%f cnt=%d\n",t,t/(cnt*1000.0/rate),xcnt);
	//wtk_kwdec2_bytes(dec);
	wtk_kwake_reset(dec);
	//wtk_kwake_bytes(dec);
	for(i=0;i<n;++i)
	{
		wtk_strbuf_delete(bufs[i]);
	}
	wtk_free(bufs);
	if(xcnt>1)
	{
		printf("=== %s\n",fn);
		//exit(0);
	}
	if(xcnt>0)
	{
		//exit(0);
	}
	return xcnt;
}

int main(int argc,char **argv)
{
	wtk_main_cfg_t *main_cfg=NULL;
	wtk_mbin_cfg_t *mbin_cfg=NULL;
	wtk_arg_t *arg;
	wtk_kwake_t *dec=NULL;
	wtk_kwake_cfg_t *cfg;
	char *ifn;
	char *cfn;
	char *scp_fn;
	char *bfn;
	int bytes;
	float f;
	char *data=NULL;

	arg=wtk_arg_new(argc,argv);
	bfn=cfn=NULL;
	wtk_arg_get_str_s(arg,"c",&cfn);
	wtk_arg_get_str_s(arg,"b",&bfn);
	wtk_arg_get_int_s(arg,"debug",&debug);
	if(!cfn && !bfn)
	{
		goto end;
	}
	if(bfn)
	{
		int len;

		data=file_read_buf(bfn,&len);
	//	mbin_cfg=wtk_mbin_cfg_new_str_type(wtk_kwake_cfg,"./wake.cfg",data,len);
	//	//mbin_cfg=wtk_mbin_cfg_new_type(wtk_rtbf_cfg,cfn,"./rtbf.cfg");
	//	cfg=(wtk_kwake_cfg_t*)(mbin_cfg->cfg);
	}else if(cfn)
	{
		main_cfg=wtk_main_cfg_new_type(wtk_kwake_cfg,cfn);
		cfg=(wtk_kwake_cfg_t*)(main_cfg->cfg);
	}
	//bytes=0;
	bytes=wtk_kwake_cfg_bytes(cfg);

	f=wtk_proc_mem();
	wtk_debug("mem=%.3fM\n",f);
	wtk_debug("cfg=%.2fkb\n",bytes*1.0/1024);
//wtk_debug("%d ,%d, %s \n",cfg->parm.use_knn,cfg->parm.knn.use_bin,cfg->parm.knn.bin_fn);
	if(cfg->parm.use_knn && cfg->parm.knn.use_bin==0 && cfg->parm.knn.bin_fn && 1)
	{
	//	wtk_debug("write %s\n",cfg->parm.knn.bin_fn);
	//	wtk_knn_cfg_write_bin(&(cfg->parm.knn),cfg->parm.knn.bin_fn);
		//exit(0);
	}
	//exit(0);
	dec=wtk_kwake_new(cfg);
	wtk_arg_get_str_s(arg,"i",&ifn);
	if(ifn)
	{
		int i;
		int ret;

		wtk_debug("ifn=%s\n",ifn);
		for(i=0;i<1;++i)
		{
			ret=test_kwdec2_file(dec,ifn);
			printf("=== %s cnt=%d\n",ifn,ret);
			if(ret>0)
			{
				printf("+++ %s\n",ifn);
			}else
			{
				printf("### %s\n",ifn);
			}
		}
	}
	scp_fn=NULL;
	wtk_arg_get_str_s(arg,"scp",&scp_fn);
	if(scp_fn)
	{
		wtk_flist_it_t *it;
		char *ifn;
		int ret;
		int tot,cnt;

		tot=cnt=0;
		it=wtk_flist_it_new(scp_fn);
		while(1)
		{
			ifn=wtk_flist_it_next(it);
			if(!ifn){break;}
			++tot;
			ret=test_kwdec2_file(dec,ifn);
			printf("=== %s cnt=%d\n",ifn,ret);
			if(ret>0)
			{
				printf("+++ %s\n",ifn);
			}else
			{
				printf("### %s\n",ifn);
			}
			cnt+=ret;
			wtk_debug("=============== tot=%.3f(%d/%d) =================\n",cnt*100.0/tot,cnt,tot);
			//exit(0);
		}
		wtk_flist_it_delete(it);
	}
end:
	if(dec)
	{
		wtk_kwake_delete(dec);
	}
	if(main_cfg)
	{
		wtk_main_cfg_delete(main_cfg);
	}
	if(mbin_cfg)
	{
		wtk_mbin_cfg_delete(mbin_cfg);
	}
	if(data)
	{
		wtk_free(data);
	}
	if(arg)
	{
		wtk_arg_delete(arg);
	}
	return 0;
}

