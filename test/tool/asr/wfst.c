#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/asr/wfst/wtk_wfstdec.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/os/wtk_pid.h"
#include "wtk/os/wtk_sem.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"

int flush_mlf = 0;

void print_usage(int argc,char **argv)
{
	printf("usage:\n\n");
	printf("%s -c cfg -scp scp_fn -mlf mlf_fn [-vad vad_fn -use_vad_bin 1 -use_json -wav wav_fn -lat lat_dn -s start -e end([s,e) -nproc n -env -usr_dn] \n\n",argv[0]);
}

typedef struct wtk_rate
{
	double wave_time;
	double cpu_time;
	double time;
	double rescore_time;
	double rec_mem;
	double rescore_mem;
}wtk_rate_t;

wtk_sem_t* std_sem;


void wtk_rate_init(wtk_rate_t *r)
{
	r->wave_time=0;
	r->cpu_time=0;
	r->time=0;
	r->rescore_time=0;
	r->rec_mem=0;
	r->rescore_mem=0;
}

typedef struct
{
	wtk_string_t env;
	wtk_string_t usr_dn;
	char *vad_fn;
	unsigned char use_json:1;
	unsigned char use_vad_bin:1;
}wtk_fst_cfg_t;


void wtk_fst_cfg_update_arg(wtk_fst_cfg_t *cfg,wtk_arg_t *arg)
{
	char *p;
	int ret;

	ret=wtk_arg_get_str_s(arg,"env",&p);
	if(ret==0)
	{
		cfg->env.data=p;
		cfg->env.len=strlen(p);
	}else
	{
		wtk_string_set(&(cfg->env),0,0);
	}
	ret=wtk_arg_get_str_s(arg,"usr_dn",&p);
	if(ret==0)
	{
		cfg->usr_dn.data=p;
		cfg->usr_dn.len=strlen(p);
	}else
	{
		wtk_string_set(&(cfg->usr_dn),0,0);
	}
	ret=wtk_arg_get_str_s(arg,"vad",&p);
	if(ret==0)
	{
		cfg->vad_fn=p;
	}else
	{
		cfg->vad_fn=NULL;
	}
	if(wtk_arg_exist_s(arg,"use_vad_bin"))
	{
		cfg->use_vad_bin=1;
	}else
	{
		cfg->use_vad_bin=0;
	}
}

wtk_fst_cfg_t fst_cfg;
int gs,ge;



char *lat_dn="./lat";


char* test_get_lat_name(wtk_wfstdec_t *dec,char *fn)
{
	wtk_string_t *v;
	//wtk_string_t *v2;
	wtk_strbuf_t *buf=dec->rec->buf;


	wtk_strbuf_reset(buf);
	wtk_strbuf_push_string(buf,lat_dn);
	wtk_strbuf_push_s(buf,"/");

	v=wtk_basename(fn,'/');
	wtk_strbuf_push(buf,v->data,v->len);
	/*
	v2=wtk_str_left(v->data,v->len,'.');
	wtk_strbuf_push(buf,v2->data,v2->len);
	wtk_string_delete(v2);
	*/
	wtk_string_delete(v);
	wtk_strbuf_push_s(buf,".lat");
	wtk_strbuf_push_c(buf,0);
	return buf->data;
}

void test_wav_file(wtk_wfstdec_t *dec,char *fn,FILE *log,wtk_rate_t *rate)
{
	char *data;
	int len,offset;
	double t,t2;
	double wav_t;
//#define USE_CPU
	int ox=44;

    wtk_debug("fn=%s\n",fn);
	data=file_read_buf(fn,&len);
	offset=ox;
	wav_t=((len-ox)*1000.0/32000);
	if(rate)
	{
		rate->wave_time+=wav_t;
	}
	t2=time_get_cpu();
	t=time_get_ms();
	wtk_wfstdec_start2(dec,fst_cfg.env.data,fst_cfg.env.len);
	{
		char *s,*e;
		int step=4000;
		int nx;
		double tx1;//,tx2;
		double nt;
		//wtk_string_t v;

		nt=0;
		tx1=time_get_ms();
		s=data+offset;e=s+len-offset;
		while(s<e)
		{
			nx=min(step,e-s);
			//tx2=time_get_ms();
			//wtk_debug("nx=%d\n",nx);
			nt+=nx/2;
			//print_short2((short*)s,100);
			//exit(0);
			wtk_wfstdec_feed(dec,s,nx,0);
			//printf("time: tx[%d]=%f\n",dec->rec->frame,tx2-tx1);
			//tx1=tx2;
//			if(wtk_fst_decoder_can_be_end(dec,&v))
//			{
//				wtk_debug("[%.*s]\n",v.len,v.data);
//				exit(0);
//			}
			s+=nx;
		}
		wtk_debug("want end %f time=%f\n",time_get_ms()-tx1,nt*1.0/8000);
		tx1=time_get_ms();
		wtk_wfstdec_feed(dec,0,0,1);
		wtk_debug("want end delay= %f\n",time_get_ms()-tx1);
		//wtk_debug("end\n");
		//tx2=time_get_ms()-tx1;
		//wtk_debug("te=%f\n",tx2);
	}
	//wtk_debug("t3=%f\n",time_get_ms());
	t2=time_get_cpu()-t2;
	t=time_get_ms()-t;
	wtk_debug("cpu=%f time=%f\n",t2,t);

	{
		//fprintf(log,"\"%.*s.rec\"\n",(int)(strlen(fn)-sizeof("wav")),fn);
		//wtk_fst_decoder_print_mlf(dec,log);
	    fflush(log);
		if(fst_cfg.use_json)
		{
			wtk_string_t v;

			wtk_wfstdec_get_result(dec,&v);
			wtk_sem_acquire(std_sem,-1);
			printf("==>fn: %s\n",fn);
			printf("==>rec: %.*s\n",v.len,v.data);
			wtk_sem_release(std_sem,1);
		}
		if(flush_mlf)
		{
			fprintf(log,"\"%.*s.rec\"\n",(int)(strlen(fn)-sizeof("wav")),fn);
			wtk_wfstdec_print_mlf(dec,log);
		    fflush(log);
		}
	}
	if(rate)
	{
		rate->cpu_time+=t2;
		rate->time+=t;

		rate->rec_mem+=wtk_wfstr_bytes(dec->rec);
	}
	wtk_debug("================ pid=%d ===============\n",(int)getpid());
	//wtk_fst_decoder_cfg_bytes(dec->cfg);
	wtk_wfstdec_print(dec);
	//wtk_debug("bytes: %f M\n",wtk_fst_decoder_bytes(dec)*1.0/(1024*1024));
	wtk_debug("time=%f,timerate=%f,cpurate=%f\n",t,t/wav_t,t2/wav_t);
	if(t/wav_t>1.0)
	{
		wtk_debug("=============> found bug\n");
		//exit(0);
	}
	t=time_get_ms();
	wtk_wfstdec_reset(dec);

	t=time_get_ms()-t;
	wtk_debug("reset time=%f\n",t);
	wtk_free(data);
	wtk_debug("==>proc: %f M\n",wtk_proc_mem());
	//wtk_msleep(1000.0);
	//wtk_debug("proc: %f M\n",wtk_proc_mem());
}


float test_wav_scp2(wtk_wfstdec_t *dec,wtk_flist_t *f,FILE *log,int s,int e)
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
		test_wav_file(dec,item->str->data,log,&rate);
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

void test_pipe(wtk_wfstdec_t *dec,FILE *log)
{
	/*
	wtk_string_t *

	//fprintf(log,"\"%*.*s.rec\"\n",len,len,item->str->data);
	//wtk_debug("%*.*s\n",len,len,item->str->data);
	test_wav_file(dec,item->str->data,log,&rate);
	printf("tot: cpu rate=%.3f\n",rate.cpu_time/rate.wave_time);
	printf("tot: time rate=%.3f\n",rate.time/rate.wave_time);
	printf("tot: eps rate=%.3f t=%.3fms\n",rate.eps_time/rate.wave_time,rate.eps_time/cnt);
	printf("tot: res rate=%.3f t=%.3fms\n",rate.rescore_time/rate.wave_time,rate.rescore_time/cnt);
	printf("tot: rec mem=%.3f M\n",rate.rec_mem/(cnt*1024*1024));
	printf("tot: res mem=%.3f M\n",rate.rescore_mem/(cnt*1024*1024));
	*/
}

void test_wav_scp(char *cfg_fn,char *fn,FILE *log,int nproc,int use_bin,char *usr_net)
{
	wtk_main_cfg_t *main_cfg=0;
	wtk_wfstdec_t *dec=0;
	wtk_wfstdec_cfg_t *cfg=NULL;
	wtk_flist_t *f;
	int i,s,e,n;
	pid_t pid;
	wtk_vad_cfg_t *vad_cfg=NULL;
	wtk_vad_t *vad=NULL;

	if(strcmp(fn,"stdin")==0)
	{
		f=NULL;
	}else
	{
		f=wtk_flist_new(fn);
		if(!f)
		{
			wtk_debug("[%s] not found.\n",fn);
			exit(0);
		}
	}
	if(nproc<=1)
	{
		if(use_bin)
		{
			//mbin_cfg=wtk_mbin_cfg_new_type(wtk_fst_decoder_cfg,cfg_fn,"./fst.cfg.r");
			//cfg=(wtk_fst_decoder_cfg_t*)mbin_cfg->cfg;
			cfg=wtk_wfstdec_cfg_new_bin(cfg_fn);
		}else
		{
			main_cfg=wtk_main_cfg_new_type(wtk_wfstdec_cfg,cfg_fn);
			wtk_debug("cfg=%p\n",main_cfg);
			if(!main_cfg)
			{
				wtk_debug("load configure failed.\n");
				goto end;
			}
			cfg=(wtk_wfstdec_cfg_t*)main_cfg->cfg;
		}
		if(usr_net)
		{
			wtk_debug("set usr net\n");
			wtk_wfstdec_cfg_set_ebnf_net(cfg,usr_net);
		}
		wtk_debug("bytes=%.3fM\n",wtk_wfstdec_cfg_bytes(cfg)*1.0/(1024*1024));
		//wtk_debug("use %d\n",cfg->parm.dnn.use_linear_output);
		//cfg->parm.dnn.use_linear_output=0;
		//wtk_debug("use %d\n",cfg->parm.dnn.use_linear_output);
		//exit(0);
		dec=wtk_wfstdec_new(cfg);
		//wtk_debug("%s\n",fst_cfg.vad_fn);
		if(fst_cfg.vad_fn)
		{
			if(fst_cfg.use_vad_bin)
			{
				vad_cfg=wtk_vad_cfg_new_bin(fst_cfg.vad_fn,"./cfg");
			}else
			{
				vad_cfg=wtk_vad_cfg_new(fst_cfg.vad_fn);
			}
			//wtk_debug("vad=%p\n",vad_cfg);
			vad=wtk_vad_new(vad_cfg,&(dec->vad_q));
			dec->vad=vad;
			cfg->use_vad=1;
			//wtk_debug("vad2=%p\n",dec->vad2)
		}
		//exit(0);
		if(f)
		{
			test_wav_scp2(dec,f,log,gs,ge);
		}else
		{
			test_pipe(dec,log);
		}
		if(vad)
		{
			wtk_vad_delete(dec->vad);
			dec->vad=NULL;
			cfg->use_vad=0;
			if(fst_cfg.use_vad_bin)
			{
				wtk_vad_cfg_delete_bin(vad_cfg);
			}else
			{
				wtk_vad_cfg_delete(vad_cfg);
			}
		}
		wtk_wfstdec_delete(dec);
	}else
	{
		float tot_rate;
		wtk_shm_t *shm;
		float *fp;

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

				if(use_bin)
				{
					//mbin_cfg=wtk_mbin_cfg_new_type(wtk_fst_decoder_cfg,cfg_fn,"./fst.cfg.r");
					//cfg=(wtk_fst_decoder_cfg_t*)mbin_cfg->cfg;
					cfg=wtk_wfstdec_cfg_new_bin(cfg_fn);
				}else
				{
					main_cfg=wtk_main_cfg_new_type(wtk_wfstdec_cfg,cfg_fn);
					if(!main_cfg)
					{
						wtk_debug("load configure failed.\n");
						goto end;
					}
					cfg=(wtk_wfstdec_cfg_t*)main_cfg->cfg;
				}
				if(usr_net)
				{
					wtk_wfstdec_cfg_set_ebnf_net(cfg,usr_net);
				}
//				if(cfg->rec.ntok>1)
//				{
//					if(wtk_file_exist(lat_dn)!=0)
//					{
//						wtk_mkdir(lat_dn);
//					}
//				}
				dec=wtk_wfstdec_new(cfg);
				if(fst_cfg.vad_fn)
				{
					if(fst_cfg.use_vad_bin)
					{
						vad_cfg=wtk_vad_cfg_new_bin(fst_cfg.vad_fn,"./cfg");
					}else
					{
						vad_cfg=wtk_vad_cfg_new(fst_cfg.vad_fn);
					}
					wtk_debug("vad=%p\n",vad_cfg);
					vad=wtk_vad_new(vad_cfg,&(dec->vad_q));
					dec->vad=vad;
					cfg->use_vad=1;
				}

				r=test_wav_scp2(dec,f,log,s,e);
				if(vad)
				{
					if(fst_cfg.use_vad_bin)
					{
						wtk_vad_cfg_delete_bin(vad_cfg);
					}else
					{
						wtk_vad_cfg_delete(vad_cfg);
					}
					wtk_vad_delete(dec->vad);
					dec->vad=NULL;
					cfg->use_vad=0;
				}
				wtk_wfstdec_delete(dec);
				fp[i]=r;
				exit(0);
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
	}
end:
	if(main_cfg)
	{
		wtk_main_cfg_delete(main_cfg);
	}else
	{
		if(cfg)
		{
			wtk_wfstdec_cfg_delete_bin(cfg);
		}
	}
	wtk_flist_delete(f);
	wtk_debug("############ total end ###############\n");
	return;
}

int main(int argc,char **argv)
{
	wtk_main_cfg_t *main_cfg=0;
	wtk_arg_t *arg;
	wtk_wfstdec_t *dec=0;
	wtk_wfstdec_cfg_t *cfg=NULL;
	char *cfg_fn=0;
	char *scp_fn=0;
	char *mlf_fn=0;
	char *wav_fn=0;
	FILE *xlog=0;
	FILE *log;
	int nproc=1;
	int bin=0;
	char *usr_net=NULL;

	std_sem=wtk_sem_new_shared();
	wtk_sem_release(std_sem,1);
	arg=wtk_arg_new(argc,argv);
	if(wtk_arg_exist_s(arg,"use_json"))
	{
		fst_cfg.use_json=1;
	}
	wtk_fst_cfg_update_arg(&(fst_cfg),arg);
	wtk_arg_get_str_s(arg,"c",&cfg_fn);
	wtk_arg_get_str_s(arg,"scp",&scp_fn);
	wtk_arg_get_str_s(arg,"mlf",&mlf_fn);
	wtk_arg_get_str_s(arg,"wav",&wav_fn);
	wtk_arg_get_str_s(arg,"lat",&lat_dn);
	wtk_arg_get_int_s(arg,"nproc",&nproc);
	wtk_arg_get_int_s(arg,"b",&bin);
	wtk_arg_get_str_s(arg,"u",&usr_net);

	gs=-1;ge=-1;
	wtk_arg_get_int_s(arg,"s",&gs);
	wtk_arg_get_int_s(arg,"e",&ge);
	if(!lat_dn)
	{
		lat_dn="./lat";
	}
	if(wtk_file_exist(lat_dn)!=0)
	{
		wtk_mkdir(lat_dn);
	}
	if(!cfg_fn || (!scp_fn && !wav_fn))
	{
		print_usage(argc,argv);
		goto end;
	}
	if(mlf_fn)
	{
		xlog=fopen(mlf_fn,"w");
		log=xlog;
		flush_mlf = 1;
	}else
	{
		log=stdout;
	}
    wtk_debug("======== rec is ready =============\n");
	if(wav_fn)
	{
		if(bin)
		{
			cfg=wtk_wfstdec_cfg_new_bin(cfg_fn);
			//mbin_cfg=wtk_mbin_cfg_new_type(wtk_fst_decoder_cfg,cfg_fn,"./fst.cfg.r");
			//cfg=(wtk_fst_decoder_cfg_t*)mbin_cfg->cfg;
			if(usr_net)
			{
				wtk_wfstdec_cfg_set_ebnf_net(cfg,usr_net);
			}
		}else
		{
			main_cfg=wtk_main_cfg_new_type(wtk_wfstdec_cfg,cfg_fn);
			if(!main_cfg)
			{
				wtk_debug("load configure failed.\n");
				goto end;
			}
			cfg=(wtk_wfstdec_cfg_t*)main_cfg->cfg;
			if(usr_net)
			{
				wtk_wfstdec_cfg_set_ebnf_net(cfg,usr_net);
			}
		}
//		if(cfg->rec.ntok>1)
//		{
//			if(wtk_file_exist(lat_dn)!=0)
//			{
//				wtk_mkdir(lat_dn);
//			}
//		}
		dec=wtk_wfstdec_new(cfg);
		if(!dec)
		{
			wtk_debug("create decoder failed.\n");
			goto end;
		}
		test_wav_file(dec,wav_fn,log,0);
	}
	if(scp_fn)
	{
		test_wav_scp(cfg_fn,scp_fn,log,nproc,bin,usr_net);
	}
end:
	if(xlog)
	{
		fclose(xlog);
	}
	if(dec)
	{
		wtk_wfstdec_delete(dec);
	}
	if(main_cfg)
	{
		wtk_main_cfg_delete(main_cfg);
	}else
	{
		if(cfg)
		{
			wtk_wfstdec_cfg_delete_bin(cfg);
		}
	}
	wtk_arg_delete(arg);
	wtk_sem_delete_shared(std_sem);
	return 0;
}


