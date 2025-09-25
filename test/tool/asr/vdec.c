#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_opt.h"
#include "wtk/asr/vdec/wtk_vdec.h"
#include "wtk/asr/vdec/wtk_vdec_cfg.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/wtk_os.h"

static void test_audio(wtk_vdec_t *v,char *fn)//,FILE *log)
{
	wtk_transcription_t *trans;
	char *data;
	int n;
	wtk_strbuf_t *buf;
	printf("%s ",fn);
	//fn="/home/madch/asr-lz/haoweilai/fenlei2/tmp/cut/13028_20170616091223190.61.wav";
	data=file_read_buf(fn,&n);
	wtk_vdec_start(v);
	wtk_vdec_feed(v,1,data+44,n-44);
	trans=v->trans;
	if(trans)
	{
		//wtk_transcription_print3(trans,log);
		//wtk_transcription_print_hresults(trans,log);
		buf=wtk_strbuf_new(256,1);
		//wtk_transcription_to_string(v->trans,buf);
		wtk_transcription_to_string2(v->trans,buf,'\n');
		//fprintf(log,"%.*s\n",buf->pos,buf->data);
		printf("%.*s\n",buf->pos,buf->data);
		wtk_strbuf_delete(buf);
	}else
	{
		//fprintf(log,".\n");
	}
	//fflush(log);
	wtk_vdec_reset(v);
	free(data);
}


void test_opt(wtk_opt_t *opt)
{
	wtk_vdec_t *v;
	wtk_flist_t *fl;
	wtk_queue_node_t *qn;
	wtk_fitem_t *item;
	int i;
	wtk_string_t *p;

	v=wtk_vdec_new((wtk_vdec_cfg_t*)opt->main_cfg->cfg);

	fl=wtk_flist_new(opt->input_fn);

	//fprintf(opt->log,"#!MLF!#\n");
	for(i=0,qn=fl->queue.pop;qn;qn=qn->next,++i)
	{
		//printf("%d/%d\n",i,fl->queue.length);
		item=data_offset(qn,wtk_fitem_t,q_n);
		//printf("%.*s\n",item->str->len,item->str->data);
		p=wtk_basename(item->str->data,'/');
		//wtk_debug("%.*s\n",p->len,p->data);
		//fprintf(opt->log,"\"*/%.*s.rec\"\n",p->len-4,p->data);
		wtk_free(p);
		//test_audio(v,item->str->data,opt->log);
	}
	wtk_flist_delete(fl);
	wtk_vdec_delete(v);
}

void test_scp(wtk_vdec_t *vdec,char *scp_fn)
{
	wtk_flist_t *f;

	if(strcmp(scp_fn,"stdin")==0)
	{
		f=NULL;
	}else
	{
		f=wtk_flist_new(scp_fn);
		if(!f)
		{
			wtk_debug("[%s] not found.\n",scp_fn);
			exit(0);
		}
	}

	wtk_fitem_t *item;
	wtk_queue_node_t *n;
	int i;
	for(i=0,n=f->queue.pop;n;n=n->next,++i)
	{
		item=data_offset(n,wtk_fitem_t,q_n);
		test_audio(vdec,item->str->data);
	}
	wtk_flist_delete(f);
	printf("all finished\n");
}

int main(int argc,char **argv)
{
	//wtk_opt_t *opt;
	wtk_arg_t *arg;
	char *cfg_fn;
	char *scp_fn;
	wtk_vdec_t *vdec=0;
	wtk_vdec_cfg_t *cfg=NULL;

	arg=wtk_arg_new(argc,argv);
	wtk_arg_get_str_s(arg,"c",&cfg_fn);
	wtk_arg_get_str_s(arg,"scp",&scp_fn);

	cfg=wtk_vdec_cfg_new_bin(cfg_fn);
	vdec=wtk_vdec_new(cfg);

	if(scp_fn)
	{
		test_scp(vdec,scp_fn);
	}
	//opt=wtk_opt_new_type(argc,argv,wtk_vdec_cfg);
	//if(!opt){goto end;}
	//test_opt(opt);
//end:
//	if(opt)
//	{
//		wtk_opt_delete(opt);
//	}
	if(arg){
		wtk_arg_delete(arg);
	}
	return 0;
}
