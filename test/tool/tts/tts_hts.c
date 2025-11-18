#include "wtk/tts/wtk_tts.h"

extern void wave_write_file(char *fn,int rate,char *data,int bytes);

double pitch_shit=0;
double speed=1.0;

void print_usage()
{
	printf("tts usgae:\n");
	printf("\t-c configure file\n");
	printf("\t-b configure is bin or not\n");
	printf("\t-s pitch shift\n");
	printf("\t-i input file\n");
	printf("\t-f input lablist file\n");
	printf("\t-skip skip lines\n");
	printf("\t-speed speed[0.8,1.2]\n");
	printf("\nExample:\n");
	printf("\t ./tool/tts -c bin -b -i test.txt -f lablistfn -s 1.3\n\n");
}

double tx=0;

void test_tts_notify2(wtk_strbuf_t *buf,char *data,int bytes)
{
	if(tx>0)
	{
		tx=time_get_ms()-tx;
		wtk_debug("tx=%f\n",tx);
		tx=0;
	}
	//wtk_debug("bytes=%d\n",bytes);
	wtk_strbuf_push(buf,data,bytes);
}

int skip=0;
void test_tts_file(wtk_tts_t *tts,char *s,int index,int cnt)
{
	wtk_strbuf_t *buf;
	double t,f;
	char *fn="test.wav";
	char xbuf[256];
	char *txts;

	txts=s;
    while(*txts!=' ' && *txts!='\0'){
    	txts++;
    }
    if(strlen(txts)>0){
    	//sprintf(xbuf,"%s.%d.wav",pre,index);
    	sprintf(xbuf,"%.*s.wav",(int)(txts-s),s);
    	s=txts;
    }
    else{
    	sprintf(xbuf,"%d.wav",index);
    }


	fn=xbuf;
//	wtk_debug("%s %d\n",fn,(int)strlen(txts));
	//wtk_debug("index=%d/%d\n",index,cnt);
	if(index<skip)
	{
		return;
	}
	buf=wtk_strbuf_new(1024*100,1);
	wtk_tts_set_notify(tts,buf,(wtk_tts_notify_f)test_tts_notify2);
	wtk_strbuf_reset(buf);
	wtk_debug("%d:[%s %s]\n",index,fn,s);
	t=time_get_ms();
	tts->pitch_shit=pitch_shit;
	tts->speed=speed;
	tx=time_get_ms();
	wtk_tts_process(tts,s,strlen(s));
	if(1)
	{
	double x;
	x=wtk_tts_bytes(tts);
	x=x*1.0/(1024*1024);
	wtk_debug("running row: m=%fM  row_avg=%fm/s\n",x, x*1024/(buf->pos/32000));
	}
	wtk_tts_reset(tts);
	t=time_get_ms()-t;
	f=buf->pos*1000.0/(tts->cfg->syn.hmm_cfg.rate*2);
	wtk_debug("time=%f f=%f rate= %f\n",t,f, t/f);
	wave_write_file(fn,tts->cfg->syn.hmm_cfg.rate,buf->data,buf->pos);
	//wave_write_file(fn,tts->cfg->syn.hmm_cfg.rate,buf->data,buf->pos);
	//wtk_debug("save %s pos=%d\n",fn,buf->pos);
	wtk_strbuf_delete(buf);
}

void wtk_tts_notify_start_f(void *ths,char *data,int bytes)
{
	printf("==tts start==%.*s\n", bytes, data);
}

void wtk_tts_notify_end_f(void *ths,char *data,int bytes)
{
	printf("==tts end==%.*s\n", bytes, data);
}
void wtk_tts_notify_f_dur(void *ths,char *data,int bytes)
{
	//printf("====%.*s\n", bytes, data);
}

int main(int argc,char **argv)
{
	wtk_arg_t *arg;
	wtk_tts_cfg_t *cfg;
	wtk_tts_t *tts;
	char *cfg_fn;
	int bin=0;
	char *ifn=NULL;
	char *ffn=NULL;    //fulllab file

	arg=wtk_arg_new(argc,argv);
	if(!arg)
	{
		print_usage();
		goto end;
	}
	wtk_arg_get_str_s(arg,"i",&ifn);
	wtk_arg_get_str_s(arg,"f",&ffn);
	wtk_arg_get_str_s(arg,"c",&cfg_fn);
	wtk_arg_get_int_s(arg,"b",&bin);
	wtk_arg_get_int_s(arg,"skip",&skip);
	wtk_arg_get_number_s(arg,"s",&pitch_shit);
	wtk_arg_get_number_s(arg,"speed",&speed);
	if(!(ifn || ffn) || !cfg_fn)
	{
		print_usage();
		goto end;
	}
	if(bin)
	{
		cfg=wtk_tts_cfg_new_bin(cfg_fn);
	}else
	{
		cfg=wtk_tts_cfg_new(cfg_fn);
	}
	//if(0)
	if(1)
	{
		double x;

		x=wtk_tts_cfg_bytes(cfg);
		x=x*1.0/(1024*1024);
		wtk_debug("wtk_tts_cfg m=%f M",x);
		//exit(0);
	}
	tts=wtk_tts_new(cfg);

//	wtk_string_t *k, *v;
//	k=wtk_string_dup("为");
//	v=wtk_string_dup("wei4");
//	wtk_tts_defpron_setwrd(tts, k, v);

	//wtk_tts_set_volume_scale(tts,5.0);
	//tts->cfg->volume_scale=5.0;
	wtk_tts_set_info_notify(tts, NULL,(wtk_tts_notify_f)wtk_tts_notify_f_dur);
	wtk_tts_set_start_notify(tts,NULL,(wtk_tts_notify_f)wtk_tts_notify_start_f);
	wtk_tts_set_end_notify(tts,NULL,(wtk_tts_notify_f)wtk_tts_notify_end_f);
	if (ifn)
		wtk_flist_process2(ifn,tts,(wtk_flist_notify_f2)test_tts_file);

	//test
//	int i=0;
//	while(i++<1000){
//		test_tts_file(tts, "我叫小奇,来自于苏州奇梦者网络科技有限公司", i,0);
//	}

//	wtk_string_delete(k);
//	wtk_string_delete(v);
	wtk_tts_delete(tts);
	if(bin)
	{
		wtk_tts_cfg_delete_bin(cfg);
	}else
	{
		wtk_tts_cfg_delete(cfg);
	}
end:
	if(arg)
	{
		wtk_arg_delete(arg);
	}
	return 0;
}
