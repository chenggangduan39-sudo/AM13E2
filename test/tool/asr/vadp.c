#include "wtk/asr/vad/wtk_vad.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/rbin/wtk_flist.h"

void print_usage()
{
	printf("vadp: usage\n");
	printf("\t-c cfg -b 0 -ns nospeech.scp -ss speech.scp\n\n");
}

typedef struct
{
	int tot;
	int nspeech;
	int xtot;
}wtk_vada_t;

void test_vad_file(wtk_vad_t *vad,char *fn,wtk_vada_t *a)
{
	char *data;
	int len;
	wtk_queue_t *q;
	wtk_queue_node_t *qn;
	wtk_vframe_t *vf;
	int v=0;
	int set=0;
	int step=wtk_vad_get_frame_step(vad);
	int nx;

	nx=0.4*16000/step;
	q=vad->output_queue;
	data=file_read_buf(fn,&len);
	wtk_vad_feed(vad,data+44,len-44,1);
	a->xtot+=q->length;
	while(1)
	{
		qn=wtk_queue_pop(q);
		if(!qn){break;}
		vf=data_offset2(qn,wtk_vframe_t,q_n);
		if(set==0)
		{
			if(vf->state!=wtk_vframe_sil)
			{
				++v;
				if(v>nx)
				{
					++a->nspeech;
					set=1;
				}
			}else
			{
				v=0;
			}
		}
		wtk_vad_push_vframe(vad,vf);
	}
	++a->tot;
	wtk_free(data);
	wtk_vad_reset(vad);
}


void test_vad_scp(wtk_vad_t *vad,char *fl,wtk_vada_t *a)
{
	wtk_flist_it_t *it;
	char *fn;
	int i;

	wtk_debug("test: %s\n",fl);
	a->tot=0;
	a->nspeech=0;
	a->xtot=0;
	it=wtk_flist_it_new(fl);
	i=0;
	while(1)
	{
		fn=wtk_flist_it_next(it);
		if(!fn){break;}
		//wtk_debug("%s\n",fn);
		++i;
		printf("%d\b\b\b\b\b\b\b",i);
		fflush(stdout);
		test_vad_file(vad,fn,a);
	}
	wtk_flist_it_delete(it);
}

/*
召回率/准确率

说明:
召回率: 检索出的相关文档数与文档库中的相关文档数的比例,查全率;
准确率: 检索出的相关文档数与检索出的文档总数的比例，查准率;
计算方法
系统检索到的相关文档 A
系统检索到的不相关文档 B
相关但是系统没有检索到的相关文档 C
不相关且没有被系统检索到的文档 D
召回率(Recall)=A/(A+C)
准确率(Precision)=A/(A+B)
FRR|FAR

说明:
FRR: false rejection rate,拒真率;
FAR: false acception rate,认假率;
*/
int main(int argc,char **argv)
{
	wtk_arg_t *arg;
	char *cfg_fn=NULL;
	char *ns=NULL;
	char *ss=NULL;
	double bin=0;
	wtk_vad_cfg_t *cfg=NULL;
	wtk_vad_t *vad;
	wtk_queue_t q;
	wtk_vada_t n,s;
	int A,B,C;//,D;
	double t;
	int step;

	arg=wtk_arg_new(argc,argv);
	if(!arg){goto end;}
	wtk_arg_get_str_s(arg,"c",&cfg_fn);
	wtk_arg_get_str_s(arg,"ns",&ns);
	wtk_arg_get_str_s(arg,"ss",&ss);
	wtk_arg_get_number_s(arg,"b",&bin);
	if(!cfg_fn || !ns || !ss)
	{
//		printf("cfg_fn=%s\n",cfg_fn);
//		printf("ns=%s\n",ns);
//		printf("ss=%s\n",ss);
		print_usage();
		goto end;
	}
	if(bin)
	{
		cfg=wtk_vad_cfg_new_bin2(cfg_fn);
	}else
	{
		cfg=wtk_vad_cfg_new(cfg_fn);
	}
	if(!cfg)
	{
		printf("configure[%s] load failed.\n",cfg_fn);
		goto end;
	}
	cfg->left_margin=20;
	cfg->right_margin=20;
	wtk_queue_init(&(q));
	vad=wtk_vad_new(cfg,&q);
	t=time_get_ms();
	test_vad_scp(vad,ns,&n);
	test_vad_scp(vad,ss,&s);
	t=time_get_ms()-t;
	step=wtk_vad_get_frame_step(vad);
	wtk_debug("time=%f rate=%f\n",t,t/((n.xtot+s.xtot)*step*1000.0/16000));
	A=n.tot-n.nspeech;	//系统检索到的相关文档,检测到的噪音
	B=s.tot-s.nspeech;  //系统检索到的不相关文档,错误检测到的噪音
	C=n.nspeech;        //相关但是系统没有检索到的相关文档
	//D=s.nspeech;        //不相关且没有被系统检索到的文档 D

	{
		float frr,far,recall,precison,fmeasure;

		wtk_debug("ns=%d/%d\n",n.nspeech,n.tot);
		wtk_debug("ss=%d/%d\n",s.nspeech,s.tot);
		frr=(s.tot-s.nspeech)*1.0/s.tot; //FRR: false rejection rate,拒真率;
		far=(n.nspeech)*1.0/n.tot;  //FAR: false acception rate,认假率;
		printf("FAR: %f\n",far);
		printf("FRR: %f\n",frr);
		recall=A*1.0/(A+C);
		precison=A*1.0/(A+B);
		fmeasure=2.0*recall*precison/(precison+recall);
		printf("Recall: %f\n",recall);
		printf("Precison: %f\n",precison);
		printf("Fmeasure: %f\n",fmeasure);
		printf("%d/%d,%d/%d,%.3f/%.3f/%.3f\n",n.nspeech,n.tot,s.nspeech,s.tot,recall,precison,fmeasure);
	}
	wtk_vad_delete(vad);
end:
	if(cfg)
	{
		if(bin)
		{
			wtk_vad_cfg_delete_bin(cfg);
		}else
		{
			wtk_vad_cfg_delete(cfg);
		}
	}
	if(arg)
	{
		wtk_arg_delete(arg);
	}
	return 0;
}


