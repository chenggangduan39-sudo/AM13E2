#include "wtk/asr/vad/wtk_vad.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/wtk_str.h"
//#define USE_FFMPEG

typedef struct wtk_vad_main_cfg
{
	char *cfg;
	char *scp;
	char *mlf;
	char *wav;
	int offset;
	unsigned int use_bin:1;
}wtk_vad_main_cfg_t;

wtk_vad_main_cfg_t vmain;



static void test_vad_wav_file(wtk_vad_t *vad,char *fn)
{
	char *p,*data;
	int len;
	int offset;

	offset=44;
	offset=vmain.offset;
	//wtk_debug("offset=%d\n",offset);
	//wtk_debug("%s\n",fn);
	p=data=file_read_buf(fn,&len);
	if(!p)
	{
		fprintf(stderr,"[%s] not found.\n",fn);
		return;
	}
	data+=offset;
	len-=offset;
	//wtk_debug("%d, %f ms\n",len,len*1000.0/32);
	//print_data(data,400);
	//printf("len=%d\n",len);

	//wtk_vad2_set_speech_thresh(vad,3650000);
#ifdef FULL
	wtk_vad2_feed(vad,WTK_PARM_END,data,len);
#else
	{
		char *s,*e;
		int step=512;
		int n;
		double t,f;

		t=time_get_ms();
		s=data;e=s+len;
		//step=len;
		while(s<e)
		{
			n=min(e-s,step);
			//wtk_debug("n=%d\n",n);
			wtk_vad_feed(vad,s,n,0);
			s+=n;
		}
		wtk_vad_feed(vad,0,0,1);
		//wtk_vad2_feed(vad,WTK_PARM_END,data,len);
		t=time_get_ms()-t;
		//wtk_debug("time=%f wav=%d\n",t,vad->output_queue->length*20);
		f=t/(vad->output_queue->length*20);
		printf("time=%f rate=%f\n",t,f);
	}
#endif
	/*
	wtk_debug("[%d/%d %d/%d]\n",vad->route.vad->frame_hoard.use_length,
			vad->route.vad->frame_hoard.cur_free,
			vad->route.vad->parm->feature_hoard.use_length,
			vad->route.vad->parm->feature_hoard.cur_free
			);
	*/
	free(p);
}

static void test_vad_file(wtk_vad_t *vad,char *fn)
{
	wtk_string_t *str;
	//int basename;
	//char *p;

	//wtk_debug("fn: %s\n",fn);
	//p=strstr(fn,".mp3");
	str=wtk_basename(fn,'.');
	//wtk_debug("[%.*s]\n",str->len,str->data);
	//exit(0);
	if(wtk_string_cmp_s(str,"mp3")==0)
	{
#ifdef USE_FFMPEG
		test_vad_mp3_file(vad,mp3_cfg,fn);
#else
		wtk_debug("%s not found.\n",fn);
#endif
	}else
	{
		test_vad_wav_file(vad,fn);
	}
	wtk_string_delete(str);
	//exit(0);
}



static void test_vad_mlf(wtk_vad_t *vad,char *scp,char *mlf)
{
	wtk_flist_t *fl;
	wtk_queue_node_t *n;
	wtk_fitem_t *item;
	FILE* fout;
	int i;
	float dur;
	//FILE *fp;
	//int use=0;
	wtk_strbuf_t *buf;
	char *p;

	buf=wtk_strbuf_new(1024,1);
	if(vad->cfg->use_dnn)
	{
		dur=vad->cfg->dnnvad.parm.frame_dur;
	}else if(vad->cfg->use_gmm2)
	{
		dur=vad->cfg->gmmvad2.frame_size*1.0/16000;
	}
	else
	{
		dur=0;
	}
	//wtk_debug("dur=%f /%d/%d\n",dur,vad->cfg->evad.frame_step,vad->cfg->use_evad);
	//exit(0);
	if(mlf)
	{
		fout=fopen(mlf,"w");
		if(!fout)
		{
			wtk_debug("can't open %s to write.\n",mlf);
			goto end;
		}
	}else
	{
		fout=stdout;
	}
	//wtk_debug("dur=%f\n",dur);
	fprintf(fout,"#!MLF!#\n");
	fl=wtk_flist_new(scp);
	for(i=1,n=fl->queue.pop;n;n=n->next,++i)
	{
		item=data_offset(n,wtk_fitem_t,q_n);
		//xvad=wtk_vad_new(vad->cfg,vad->output_queue);
		wtk_vad_start(vad);
		//wtk_debug("vad=%p,v=%p\n",xvad->frame_buffer,vad);
		p=item->str->data;
		wtk_strbuf_reset(buf);
		wtk_strbuf_push_string(buf,p);
		wtk_strbuf_strip(buf);
		wtk_strbuf_push_c(buf,0);
		p=buf->data;
		fprintf(fout,"\"%s\"\n",p);
		test_vad_file(vad,p);
		wtk_vad_queue_print_mlf(vad->output_queue,dur,fout);
		fflush(fout);
		if(mlf)
		{
			printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b%d/%d",i,fl->queue.length);
			fflush(stdout);
		}
		wtk_vad_reset(vad);
		//wtk_vad_delete(xvad);
	}
	if(mlf)
	{
		printf("\n");
		fclose(fout);
	}
	wtk_flist_delete(fl);
end:
	wtk_strbuf_delete(buf);
	return;
}


void wtk_vad_main_cfg_init(wtk_vad_main_cfg_t *cfg)
{
	cfg->cfg=0;
	cfg->scp=0;
	cfg->mlf=0;
	cfg->offset=44;
	cfg->use_bin=0;
	cfg->wav=NULL;
}

void print_usage()
{
	printf("vad version 0.0.6.%s.%s\n\n",__DATE__,__TIME__);
	printf("usage:\n");
	printf("\t-c vad configure file\n");
	printf("\t-b bin file\n");
	printf("\t-scp scp wave file list\n");
	printf("\t-mlf output mlf file, optional,if not set will output to console\n");
	printf("\t-offset wave header offset, default is 44\n");
	printf("\n./tool/vad  -c ./res/vad/vad.0.1/cfg -scp 1.scp -offset 44\n");
	printf("\n");
}

void wtk_vad_main_cfg_update_arg(wtk_vad_main_cfg_t *cfg,wtk_arg_t *arg)
{
	double v;
	char *fn;

	wtk_arg_get_str_s(arg,"c",&(cfg->cfg));
	//wtk_debug("%s\n",cfg->cfg);
	wtk_arg_get_str_s(arg,"scp",&(cfg->scp));
	wtk_arg_get_str_s(arg,"mlf",&(cfg->mlf));
	wtk_arg_get_str_s(arg,"wav",&(cfg->wav));
	if(wtk_arg_exist_s(arg,"offset"))
	{
		wtk_arg_get_number_s(arg,"offset",&v);
		cfg->offset=v;
	}
	fn=0;
	wtk_arg_get_str_s(arg,"b",&fn);
	if(fn)
	{
		cfg->cfg=fn;
		cfg->use_bin=1;
	}
	//wtk_debug("%s\n",cfg->cfg);
	if(!cfg->cfg)
	{
		print_usage();
		exit(0);
	}
	//wtk_debug("%s\n",cfg->cfg);
}


int main(int argc,char **argv)
{
	wtk_vad_cfg_t *cfg;
	wtk_vad_t *vad;
	wtk_queue_t queue;
	wtk_main_cfg_t *main_cfg=0;
	wtk_arg_t *arg;

	//test_qlas();
	arg=wtk_arg_new(argc,argv);
	if(!arg){print_usage();goto end;}
	wtk_vad_main_cfg_init(&(vmain));
	wtk_vad_main_cfg_update_arg(&(vmain),arg);
	//wtk_debug("%s\n",vmain.cfg);
	wtk_queue_init(&queue);
	//wtk_debug("%s\n",vmain.cfg);
	if(vmain.use_bin)
	{
		//wtk_debug("%s\n",vmain.cfg);
		cfg=wtk_vad_cfg_new_bin2(vmain.cfg);
		if(!cfg)
		{
			wtk_debug("create configure failed.\n");
			goto end;
		}
	}else
	{
		main_cfg=wtk_main_cfg_new4(sizeof(wtk_vad_cfg_t),
				(wtk_main_cfg_init_f)wtk_vad_cfg_init,
				(wtk_main_cfg_clean_f)wtk_vad_cfg_clean,
				(wtk_main_cfg_update_local_f)wtk_vad_cfg_update_local,
				(wtk_main_cfg_update_f)wtk_vad_cfg_update,vmain.cfg,arg);
		if(!main_cfg)
		{
			wtk_debug("load %s failed.\n",vmain.cfg);
			goto end;
		}
		cfg=(wtk_vad_cfg_t*)main_cfg->cfg;
	}
	//wtk_qlas_cfg_write_fix_bin(&(cfg->dnnvad.parm.dnn.qlas),"./res/vad/vad.0.1/qlas.bin");
	//cfg->vad.cache_size=1;
	//cfg->vad.parm.cache_size=1;
	vad=wtk_vad_new(cfg,&queue);
	//wtk_debug("left=%d right=%d\n",vad->left_margin,vad->right_margin);
	if(vmain.wav)
	{
		float dur;

		if(vad->cfg->use_dnn)
		{
			dur=vad->cfg->dnnvad.parm.frame_dur;
		}else if(vad->cfg->use_gmm2)
		{
			dur=vad->cfg->gmmvad2.frame_size*1.0/16000;
		}
		else if(vad->cfg->use_k)
		{
			dur=vad->cfg->kvad.parm.parm.frame_step_ms/1000;
		}else{
			dur=0;
		}
		{
			int i;
			double t;

			t=time_get_ms();
			for(i=0;i<1;++i)
			{
				wtk_vad_reset(vad);
				test_vad_file(vad,vmain.wav);
			}
			t=time_get_ms()-t;
			printf("time=%f\n",t);
		}
		wtk_vad_queue_print_mlf(vad->output_queue,dur,stdout);
	}
	if(vmain.scp)
	{
		test_vad_mlf(vad,vmain.scp,vmain.mlf);
	}
	wtk_vad_delete(vad);
end:
	if(main_cfg)
	{
		wtk_main_cfg_delete(main_cfg);
	}
	if(arg)
	{
		wtk_arg_delete(arg);
	}

	return 0;
}
