#include "wtk/asr/wdec/wtk_wdec.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_opt.h"
#include "wtk/core/wtk_riff.h"

int test_wdec_file(wtk_wdec_t *dec,char *fn)
{
	wtk_strbuf_t **bufs;
	int n,i;
	double t;
	char *s,*e;
	int step,cnt;
	float fs,fe;
	int b=0;

	bufs=wtk_riff_read_channel(fn,&n);
	wtk_wdec_set_words(dec,"还有一种人|小爱同学|天猫精灵",strlen("还有一种人|小爱同学|天猫精灵"));
	wtk_wdec_start(dec,NULL);
	//wtk_debug("conf=%f\n",dec->post->min_conf);
	t=time_get_ms();
	s=bufs[0]->data;
	e=s+bufs[0]->pos;
	cnt=0;
	while(s<e)
	{
		step=min(e-s,1024);
		cnt+=step/2;
		wtk_wdec_feed(dec,s,step,0);
		if(dec->found)
		{
		wtk_wdec_print(dec);
		wtk_wdec_get_final_time(dec,&fs,&fe);
		wtk_debug("time=%f [%f,%f] rate=%f\n",t,fs,fe,t/(cnt*1000.0/16000));
		wtk_wdec_reset(dec);
		wtk_wdec_start(dec,NULL);
		//	b=1;
		//	break;
		}
		s+=step;
	}
	wtk_wdec_feed(dec,NULL,0,1);
	t=time_get_ms()-t;
	b=dec->found;
	if(dec->found)
	{
		wtk_wdec_print(dec);
		wtk_wdec_get_final_time(dec,&fs,&fe);
	}else
	{
		printf(" 0.0\n");
		fs=fe=0;
	}
	wtk_debug("time=%f [%f,%f] rate=%f\n",t,fs,fe,t/(cnt*1000.0/16000));
	wtk_wdec_reset(dec);
	for(i=0;i<n;++i)
	{
		wtk_strbuf_delete(bufs[i]);
	}
	wtk_free(bufs);
	return b;
}

int main(int argc,char **argv)
{
	wtk_arg_t *arg = wtk_arg_new(argc,argv);
	wtk_wdec_t *dec=NULL;
	double t;
	float conf=-1.0;
	char *s,*c,*scp;
	wtk_arg_get_float_s(arg,"conf",&conf);
	wtk_arg_get_str_s(arg,"s",&s);
	wtk_arg_get_str_s(arg,"scp",&scp);
	wtk_arg_get_str_s(arg,"c",&c);

	wtk_wdec_cfg_t *cfg= wtk_wdec_cfg_new_bin(c);//(wtk_wdec_cfg_t*)(opt->main_cfg->cfg);

	dec=wtk_wdec_new((wtk_wdec_cfg_t*)(cfg));
	if(cfg->net.use_words_set)
	{
		wtk_wdec_set_words(dec,"还有一种人|小爱同学|天猫精灵",strlen("还有一种人|小爱同学|天猫精灵"));
	}
	//if(conf!=-1.0)
	//{
	//	wtk_wdec_set_conf(dec,conf);
	//}
	//test_wdec_file(dec,opt->input_fn);
	{
		wtk_flist_it_t *it;
		char *fn;
		int cnt=0;
		int i=0;
		int ret;

		t=time_get_ms();
		it=wtk_flist_it_new(scp);
		while(1)
		{

			fn=wtk_flist_it_next(it);
			if(!fn){break;}
			printf("%s",fn);
			++cnt;
			ret=test_wdec_file(dec,fn);
			if(ret==1)
			{
				++i;
			}
		}
		wtk_flist_it_delete(it);
		t=time_get_ms()-t;
		wtk_debug("i=%d/%d time=%f\n",i,cnt,t);
	}
	if(dec)
	{
		wtk_wdec_delete(dec);
		wtk_wdec_cfg_delete_bin(cfg);
	}
	if(arg)
	{
		wtk_arg_delete(arg);
	}
	return 0;
}
