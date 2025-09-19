#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/asr/kws/qtk_kws.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/math/wtk_matrix.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_os.h"
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
	//printf("wakeup:%.*s\n",name_len,name);
}

void test_wav_file(qtk_kws_t *dec,char *fn, int fn_len)
{
	char fixwav[160480];
	double t,t2;
	double wav_t;
	char *data;
	wtk_strbuf_t **buf;
	int n,i,xx;
	int len,offset;
	buf=wtk_riff_read_channel(fn,&n);

	int ox=44;//46+32;
    printf("==>fn: %s\n",fn);
    //printf("%s  [\n",fn);
	//data=file_read_buf(fn,&len);
	//wtk_debug("%d\n",buf[0]->pos);
	offset=ox;
	//wav_t=((len-ox)*1000.0/32000);
	wav_t=((buf[0]->pos)*1.0/32);//for 16k
	wav_t=((buf[0]->pos)*1000.0/16000);//for 8k

	xx=0;
	while(xx < 160480)
	{
		n=min(buf[0]->pos,160480-xx);
		memcpy(fixwav+xx,buf[0]->data,sizeof(char)*n);
		xx+=buf[0]->pos;
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
		//tx1=time_get_ms();
//		s=data+offset;e=s+len-offset;
		//s=buf[0]->data;
		//e=s+buf[0]->pos;
		s=fixwav;
		e=fixwav+160480;
		while(s<e)
		{
			nx=min(step,e-s);
			nt+=nx/2;
			qtk_kws_feed(dec,s,nx,0);
			s+=nx;
		}
		qtk_kws_feed(dec,0,0,1);
	}
	t2=time_get_cpu()-t2;
	t=time_get_ms()-t;

	//wtk_debug("time=%f,timerate=%f,cpurate=%f\n",t,t/wav_t,t2/wav_t);
	qtk_kws_reset(dec);
//	wtk_free(data);
}

int load_wav(qtk_kws_t *dec,wtk_string_t *fn)
{
	//wtk_debug("%.*s\n",fn->len,fn->data);
	test_wav_file(dec,fn->data,fn->len);
	return 0;
}

void test_scp(qtk_kws_t *dec,char *fn,int eval)
{
	wtk_flist_t *f;
	wtk_queue_node_t *n;
	wtk_fitem_t *item;
	int i,j;
	char *x;

	wtk_string_t* name;
	f=wtk_flist_new(fn);

	if(f)
	{
		for(i=0,n=f->queue.pop;n;n=n->next,++i)
		{
			item=data_offset(n,wtk_fitem_t,q_n);
//			printf("%s\n",item->str->data);
			wtk_dir_walk(item->str->data,dec,(wtk_dir_walk_f)load_wav);
			if(eval)
			{
				x = wtk_str_split3(item->str->data,item->str->len,'/',&j);
				//wtk_debug("%.*s\n",j,x);
			    name=wtk_string_dup_data(x,j);
			    wtk_svprint_enroll2file(dec->svprint,name);
			}
		}
		wtk_flist_delete(f);
	}
}

int main(int argc,char **argv)
{
	wtk_main_cfg_t *main_cfg=0;
	wtk_arg_t *arg;
	qtk_kws_t *dec=0;
	qtk_kws_cfg_t *cfg=NULL;
	char *cfg_fn=0;
	char *scp_fn=0;
	char *scp_fn2=0;
	char *eval_dir=0;
	char *enroll_dir=0;
	char *name=0;
	int bin=0;

	arg=wtk_arg_new(argc,argv);

	wtk_arg_get_str_s(arg,"c",&cfg_fn);
	wtk_arg_get_int_s(arg,"b",&bin);

	wtk_arg_get_str_s(arg,"name",&name);//eval wav
	wtk_arg_get_str_s(arg,"ev",&eval_dir);//eval wav
	wtk_arg_get_str_s(arg,"en",&enroll_dir);//enroll wav

	wtk_arg_get_str_s(arg,"scp",&scp_fn);//eval scp dir
	wtk_arg_get_str_s(arg,"scp2",&scp_fn2);//enroll dir scp
    //wtk_debug("======== rec is ready =============\n");
    if(cfg_fn)
    {
		main_cfg=wtk_main_cfg_new_type(qtk_kws_cfg,cfg_fn);
		if(!main_cfg)
		{
			wtk_debug("load configure failed.\n");
			goto end;
		}
		cfg=(qtk_kws_cfg_t*)main_cfg->cfg;

		dec=qtk_kws_new(cfg);
		qtk_kws_set_notify(dec,(qtk_kws_res_notify_f)on_kws,NULL);
		if(!dec)
		{
			wtk_debug("create decoder failed.\n");
			goto end;
		}
    }

    wtk_string_t *names;

	if(eval_dir)
	{
	    qtk_kws_enroll(dec,"liuyong",strlen("liuyong"));
		wtk_dir_walk(eval_dir,dec,(wtk_dir_walk_f)load_wav);
		if(name)
		{
		    names=wtk_string_dup_data(name,strlen(name));
		}else
		{
		    names=wtk_string_dup_data("xj",strlen("xj"));
		}
	    wtk_svprint_enroll2file(dec->svprint,names);
	}
	if(enroll_dir)
	{
	    qtk_kws_enroll(dec,"liuyong",0);//start eval
		wtk_dir_walk(enroll_dir,dec,(wtk_dir_walk_f)load_wav);
	}

	if(scp_fn)
	{
	    qtk_kws_enroll(dec,"liuyong",strlen("liuyong"));
		test_scp(dec,scp_fn,1);
	}

	if(scp_fn2)
	{
	    //qtk_kws_enroll(dec,"liuyong",0);//start eval
	    qtk_kws_enroll_end(dec);
		test_scp(dec,scp_fn2,0);
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


