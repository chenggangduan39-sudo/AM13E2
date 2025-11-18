#include "wtk/asr/vad/kvad/wtk_kvad.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/cfg/wtk_opt.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/core/wtk_os.h"

FILE *xmlf=NULL;

int test_kvad_file(wtk_kvad_t *vad,char *fn)
{
	wtk_strbuf_t **bufs;
	int n,i;
	double t;
	char *s,*e;
	int step,cnt;
	int x1=0;
	int rate=8000;

	bufs=wtk_riff_read_channel(fn,&n);
	if(!bufs)
	{
		return 0;
	}
	wtk_kvad_start(vad);
	t=time_get_ms();
	s=bufs[0]->data;
	e=s+bufs[0]->pos;
	//s+=(int)(rate*2*1.6);

	//print_short((short*)s,105);

	wtk_kvad_bytes(vad);

	cnt=0;
	while(s<e)
	{
		step=min(e-s,32);
		cnt+=step/2;
		x1+=step/2;
		wtk_kvad_feed(vad,(short*)s,step/2,0);
		s+=step;
	}

	wtk_kvad_bytes(vad);

	wtk_kvad_feed(vad,NULL,0,1);
	wtk_debug("hoard=%d output=%d\n",vad->frame_hoard.use_length,vad->output_q.length);
	t=time_get_ms()-t;
	//wtk_kvad_print_mlf(vad);

	if(xmlf)
	{
		wtk_string_t *v,*v1;

		v=wtk_basename(fn,'/');
		v1=wtk_str_left(v->data,v->len,'.');
		//wtk_debug("[%.*s]/[%.*s]\n",v->len,v->data,v1->len,v1->data);
		fprintf(xmlf,"\"*/%.*s.mlf\"\n",v1->len,v1->data);
		wtk_string_delete(v1);
		wtk_string_delete(v);
		wtk_kvad_print_mlf2(vad,xmlf);
	}else
	{
		wtk_kvad_print_mlf(vad);
	}

	//wtk_mvad_print_speech(vad);
	wtk_debug("time=%f rate=%f\n",t,t/(cnt*1000.0/rate));
	wtk_kvad_reset(vad);
	for(i=0;i<n;++i)
	{
		wtk_strbuf_delete(bufs[i]);
	}
	wtk_free(bufs);
	wtk_kvad_bytes(vad);
	return 0;
}

int main(int argc,char **argv)
{
	wtk_main_cfg_t *main_cfg=NULL;
	wtk_mbin_cfg_t *mbin_cfg=NULL;
	wtk_arg_t *arg;
	wtk_kvad_t *vad=NULL;
	wtk_kvad_cfg_t *cfg;
	double t;
	char *ifn;
	char *cfn;
	char *bfn;
	char *mlf=NULL;

	cfn=bfn=NULL;
	arg=wtk_arg_new(argc,argv);
	wtk_arg_get_str_s(arg,"c",&cfn);
	wtk_arg_get_str_s(arg,"b",&bfn);
	wtk_arg_get_str_s(arg,"mlf",&mlf);
	if(!cfn  && !bfn)
	{
		goto end;
	}
	if(mlf)
	{
		xmlf=fopen(mlf,"w");
		fprintf(xmlf,"#!MLF!#\n");
	}
	if(bfn)
	{
		mbin_cfg=wtk_mbin_cfg_new_type(wtk_kvad_cfg,bfn,"./vad.cfg");
		cfg=(wtk_kvad_cfg_t*)(mbin_cfg->cfg);
	}else
	{
		main_cfg=wtk_main_cfg_new_type(wtk_kvad_cfg,cfn);
		cfg=(wtk_kvad_cfg_t*)(main_cfg->cfg);
	}
	if(cfg->parm.use_knn && cfg->parm.knn.use_bin==0 && cfg->parm.knn.bin_fn && 1)
	{
		wtk_debug("write %s\n",cfg->parm.knn.bin_fn);
		wtk_knn_cfg_write_bin(&(cfg->parm.knn),cfg->parm.knn.bin_fn);
		//exit(0);
	}
	{
		int bytes;

		bytes=wtk_kvad_cfg_bytes(cfg);
		wtk_debug("cfg=%.3fKB\n",bytes*1.0/1024);
	}
//	if(cfg->parm2.use_bin==0)
//	{
//		wtk_mvad_cfg_write_bin(cfg,cfg->parm2.bin_fn);
//	}
	vad=wtk_kvad_new(cfg);
	//exit(0);
	wtk_arg_get_str_s(arg,"i",&ifn);
	if(ifn)
	{
		int i;
		int ret;

		for(i=0;i<1;++i)
		{
			ret=test_kvad_file(vad,ifn);
			if(ret>0)
			{
				printf("+++ %s\n",ifn);
			}else
			{
				printf("### %s\n",ifn);
			}
		}
	}
	wtk_arg_get_str_s(arg,"scp",&ifn);
	if(ifn)
	{
		wtk_flist_it_t *it;
		char *fn;
		int cnt=0;
		int i=0;
		int ret;

		t=time_get_ms();
		it=wtk_flist_it_new(ifn);
		while(1)
		{
			fn=wtk_flist_it_next(it);
			if(!fn){break;}
			++cnt;
			ret=test_kvad_file(vad,fn);
			if(ret>0)
			{
				++i;
				//exit(0);
				printf("+++ %s\n",fn);
			}else
			{
				printf("### %s\n",fn);
			}
			wtk_debug("================ %.3f( %d / %d ) ===========\n",i*1.0/cnt,i,cnt);
		}
		printf("==>tot: %.3f( %d / %d )  ===========\n",i*1.0/cnt,i,cnt);
		wtk_flist_it_delete(it);
		t=time_get_ms()-t;
		//wtk_debug("i=%d/%d time=%f\n",i,cnt,t);
	}
end:
	if(xmlf)
	{
		fclose(xmlf);
	}
	if(vad)
	{
		wtk_kvad_delete(vad);
	}
	if(main_cfg)
	{
		wtk_main_cfg_delete(main_cfg);
	}
	if(mbin_cfg)
	{
		wtk_mbin_cfg_delete(mbin_cfg);
	}
	if(arg)
	{
		wtk_arg_delete(arg);
	}
	return 0;
}
