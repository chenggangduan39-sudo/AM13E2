#include "wtk/bfio/wtk_kvadwake.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"

void print_usage()
{
	printf("kvadwake usage:\n");
	printf("\t-c cfg\n");
	printf("\t-i input wav file\n");
	printf("\t-o output wav file\n");
	printf("\n");
}

static int wake_cnt=0;

void test_kvadwake_on_wake(void *ths,wtk_kvadwake_cmd_t cmd,float fs,float fe,short *data, int len)
{
	if(cmd==WTK_KVADWAKE_WAKE)
	{
		++wake_cnt;
		wtk_debug("wakeup start %f end %f \n",fs,fe);
	}
}

void test_kvadwake_file(wtk_kvadwake_t *kvadwake,char *fn)
{
	wtk_riff_t *riff;
    short *pv;
    int len,bytes;
    int ret;
    double t;
    int cnt;

    len=20*16;
    bytes=sizeof(short)*len;
    pv=(short *)wtk_malloc(sizeof(short)*len);

    riff=wtk_riff_new();
    wtk_riff_open(riff,fn);

	wtk_kvadwake_set_notify(kvadwake,NULL,(wtk_kvadwake_notify_f)test_kvadwake_on_wake);
	wtk_kvadwake_start(kvadwake);

    cnt=0;
    t=time_get_ms();
    while(1)
    {
        ret=wtk_riff_read(riff,(char *)pv,bytes);
        if(ret<=0)
        {
            wtk_debug("break ret=%d\n",ret);
            break;
        }
        len=ret/sizeof(short);

        cnt+=len;
        wtk_kvadwake_feed(kvadwake,pv,len,0);
    }

	wtk_kvadwake_feed(kvadwake,NULL,0,1);

    t=time_get_ms()-t;
    wtk_debug("wake cnt =%d rate=%f t=%f\n",wake_cnt,t/(cnt/16.0),t);

	wtk_kvadwake_reset(kvadwake);

	wtk_riff_delete(riff);
	wtk_free(pv);
}

int main(int argc,char **argv)
{
	wtk_arg_t *arg=NULL;
	wtk_kvadwake_t *m=NULL;
	wtk_kvadwake_cfg_t *cfg=NULL;
	char *fn,*cfg_fn,*bin_fn;
	int i;
	wtk_main_cfg_t *main_cfg=NULL;

	fn=cfg_fn=bin_fn=NULL;
	arg=wtk_arg_new(argc,argv);
	wtk_arg_get_str_s(arg,"i",&fn);
	wtk_arg_get_str_s(arg,"c",&cfg_fn);
	if(!cfg_fn)
	{
		print_usage();
		goto end;
	}

	main_cfg=wtk_main_cfg_new_type(wtk_kvadwake_cfg,cfg_fn);
	cfg=(wtk_kvadwake_cfg_t*)(main_cfg->cfg);

	m=wtk_kvadwake_new(cfg);
	if(fn)
	{
		for(i=0;i<1;++i)
		{
			test_kvadwake_file(m,fn);
		}
	}
end:
	if(m)
	{
		wtk_kvadwake_delete(m);
	}
	if(main_cfg)
	{
		wtk_main_cfg_delete(main_cfg);
	}
	if(arg)
	{
		wtk_arg_delete(arg);
	}
	return  0;
}
