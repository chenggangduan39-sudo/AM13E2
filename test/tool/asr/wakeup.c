#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/asr/wfst/wakeup/wtk_wakeup.h"
#define min(a,b)            (((a) < (b)) ? (a) : (b))

int gs,ge;

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

void wtk_print_usage()
{
   printf("need -c for the cfg file\n");
}
int  wtk_test_wav(wtk_wakeup_t *w,char *wav_fn,int *count,int detail,wtk_rate_t *rate,int duration);
int wtk_test_wav2(wtk_wakeup_t *w,char *wav_fn,int detail,int chn,int *count,wtk_rate_t *rate,int duration);
double t1 = 0.0;
int wtk_test_scp(wtk_wakeup_t *w,char *fn,int detail,int chn,int duration)
{
	double t;
	int ret = 0;
	wtk_flist_t *f;
	int count = 0;
	wtk_rate_t rate;
	wtk_rate_init(&(rate));

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

		wtk_fitem_t *item;
		wtk_queue_node_t *n;
		int i;
		//char *p;
		int cnt=0;
		for(i=0,n=f->queue.pop;n;n=n->next,++i)
		{
			item=data_offset(n,wtk_fitem_t,q_n);
			{
				//printf("%d/%d\n",i,f->queue.length);
			}
			++cnt;
			t=time_get_ms();
			wtk_test_wav2(w,item->str->data,detail,chn,&count,&rate,duration);
			//wtk_test_wav(w,item->str->data,&count,detail,&rate,duration);
			t=time_get_ms()-t;
			//printf("%f\n",t);
			t1 = t1 + t;
		}
		//float a = (float)(count)/(float)(cnt);
		//wtk_debug("all end rec:%d total:%d aver:%f\n",count,cnt,a);
		//printf("wav time:%f\n",rate.wave_time);
		//printf("rate:%f\n",t1/rate.wave_time);
	return ret;
}

int wtk_test_wav2(wtk_wakeup_t *w,char *wav_fn,int detail,int chn,int *count,wtk_rate_t *rate,int duration)
{
	int n;
	wtk_strbuf_t **bufs;
	//FILE *fp;
	char *s,*e;
	int nx,res,ret=0;
	int step=4000;
	double nt;
	double wav_t;

	res=0;
	bufs=wtk_riff_read_channel(wav_fn,&n);
//	wtk_debug("n:%d\n",n);
//	wtk_debug("n:%d\n",bufs[chn-1]->pos);
	if(chn > n || chn <= 0)
	{
		printf("Program terminated,do not have this channel\n");
		exit(0);
	}

	s=bufs[chn-1]->data;
	e=s+bufs[chn-1]->pos;
	wav_t=(bufs[chn-1]->length*1000.0/32000);
	if(rate)
	{
		rate->wave_time+=wav_t;
		//printf("wav time:%f\n",wav_t);
	}
	printf("%s",wav_fn);
	while(s<e)
	{
		nx=min(step,e-s);
		nt+=nx/2;
		ret=wtk_wakeup_feed(w,s,nx,0);
		if(ret==1){
			//printf(" %f\n",w->prob);
			break;
		}
		s+=nx;
	}
	ret=wtk_wakeup_feed(w,0,0,1);
	//printf("%s",wav_fn);
	if(ret==1)
	{
		printf(" waked %f \n",w->prob);
		//wtk_wakeup_print_win2(w,detail);
	}else
	{
		printf(" 0.0\n");
	}
	//printf(" %f\n",w->prob);
	//wtk_wakeup_print_win(w);
	//wtk_wakeup_get_result2(w,&v,&res,detail);//,duration);
	if(res == 1){
                //printf(" 1\n");
                (*count)++;
        }
	wtk_wakeup_reset(w);

	return 0;
}

int  wtk_test_wav(wtk_wakeup_t *w,char *wav_fn,int *count,int detail,wtk_rate_t *rate,int duration)
{
	int ret = 0;
	char *data;
	int len,offset;
	//double t,t2;
	double wav_t;
	//#define USE_CPU
	int ox=44;

	//wtk_debug("fn=%s\n",wav_fn);
	data=file_read_buf(wav_fn,&len);
	offset=ox;

	char *s,*e;
	int step=4000;
	int nx;//,res;
	//double tx1;
	double nt;

	//res  = 0;
	nt=0;
	//tx1=time_get_ms();
	wav_t=((len-ox)*1000.0/32000);
	if(rate)
	{
		rate->wave_time+=wav_t;
		//printf("wav time:%f\n",wav_t);
	}
	s=data+offset;
	e=s+len-offset;
	//wtk_debug("%d\n",len-offset);
	while(s<e)
	{
		nx=min(step,e-s);
		nt+=nx/2;
		ret=wtk_wakeup_feed(w,s,nx,0);
		if(ret==1)
		{
			break;
		}
		s+=nx;
	}
	//wtk_debug("want end %f\n",time_get_ms()-tx1);
	//tx1=time_get_ms();
	ret=wtk_wakeup_feed(w,0,0,1);
	printf("%s",wav_fn);
	//wtk_wakeup_get_result2(w,&v,&res,detail);//,duration);
//	//printf("%.*s\n",v.len,v.data);
//	//printf("==>rec: %.*s\n",v.len,v.data);
//	/*printf("%s",wav_fn);*/
	if(ret==1)
	{
		//wtk_wakeup_print_win2(w,detail);
	}else
	{
		printf(" 0.0\n");
	}
	wtk_wakeup_reset(w);

	return ret;
}

int main(int argc,char **argv)
{
	//printf("hello world\n");
	wtk_arg_t *arg;
	wtk_main_cfg_t *main_cfg=NULL;
	wtk_wakeup_t *w;


	char *cfg_fn = 0;
	char *scp_fn = 0;
	char *wav_fn = 0;
	char *vad_fn=0;
	int wsmooth=12;
	int min_raise=3;
	int max_frame=80;
	//float cur_thresh = 0.03;
	//float pth_thresh = 0.1;
	//float lpth_thresh = 0.1;
	int i  = -1;
	int detail = 0;
	int duration=0;
	int chn  = 1;
//	int mode=1;

//	wtk_debug("========%f\n",pow(0.317954*0.946232*0.171942*0.614272,1.0/4));

	arg=wtk_arg_new(argc,argv);

	wtk_arg_get_str_s(arg,"c",&cfg_fn);
	wtk_arg_get_str_s(arg,"scp",&scp_fn);
	wtk_arg_get_str_s(arg,"wav",&wav_fn);
	wtk_arg_get_str_s(arg,"vad",&vad_fn);

    if(cfg_fn){
    	main_cfg = wtk_main_cfg_new_type(wtk_wakeup_cfg,cfg_fn);
    	w = wtk_wakeup_new((wtk_wakeup_cfg_t*)main_cfg->cfg);
    	wtk_wakeup_start(w);
    }else{
    	wtk_print_usage();
    	exit(0);
    }
    wtk_arg_get_int_s(arg,"d",&detail);
    wtk_arg_get_int_s(arg,"D",&duration);
    //wtk_debug("%d\n",detail);
	i = wtk_arg_get_int_s(arg,"ws",&wsmooth);
	if(i == 0){
		wtk_wakeup_set_smooth(w,wsmooth);
	}
	i = wtk_arg_get_int_s(arg,"m",&min_raise);
	if(i == 0){
		wtk_wakeup_set_min_raise(w,min_raise);
	}
	i = wtk_arg_get_int_s(arg,"M",&max_frame);
	if(i == 0){
		wtk_wakeup_set_max_frame(w,max_frame);
	}
//	i = wtk_arg_get_float_s(arg,"cur",&cur_thresh);
//	if(i == 0){
//		wtk_wakeup_set_cur_thresh(w,cur_thresh);
//	}
//	i = wtk_arg_get_float_s(arg,"pth",&pth_thresh);
//	if(i == 0){
//		wtk_wakeup_set_pth_thresh(w,pth_thresh);
//	}
//	i = wtk_arg_get_float_s(arg,"lpth",&lpth_thresh);
//	if(i == 0){
//		//wtk_wakeup_set_lastpth_thresh(w,lpth_thresh);
//	}
	i = wtk_arg_get_int_s(arg,"n",&chn);
	if(i != 0){
		chn = 1;
	}
//	i = wtk_arg_get_int_s(arg,"mode",&mode);
//	if(i == 0){
//		wtk_debug("1111111111111\n");
//		wtk_wakeup_set_usewin(w,mode);
//	}
	if(scp_fn){
		wtk_test_scp(w,scp_fn,detail,chn,duration);
	}
	//int a = 1;
	if(wav_fn){
		wtk_test_wav2(w,wav_fn,detail,chn,NULL,NULL,duration);
	//	wtk_test_wav(w,wav_fn,&a,detail,NULL);
	}
	return 0;
}
