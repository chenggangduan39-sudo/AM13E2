#include "wtk/core/wtk_type.h"
#include "wtk/bfio/maskform/wtk_gainnet_bf4.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/cfg/wtk_main_cfg.h"

static float theta=0;
static float phi;

static void print_usage()
{
	printf("gainnet_bf4 usage:\n");
	printf("\t-c cfg\n");
	printf("\t-i input wav file\n");
    printf("\t-scp input scp file\n");
	printf("\t-o output wav file\n");
	printf("\n");
}

static void test_gainnet_bf4_on(wtk_wavfile_t *wav,short *data, int len, int is_end)
{
    if(len>0)
    {
        wtk_wavfile_write(wav,(char *)data, len<<1);
    }
}

static void test_gainnet_bf4_on_ssl(void *ths,float ts, float te,wtk_ssl2_extp_t *nbest_extp,int nbest)
{
    int i;

    for(i=0;i<nbest;++i)
    {
        printf("[%f %f] ==> %d %d %f\n", ts, te, nbest_extp[i].theta, nbest_extp[i].phi, nbest_extp[i].nspecsum);
    }
	if(nbest==0)
	{
		printf("[%f %f] no speech\n", ts,te);
	}
}

static void test_gainnet_bf4_file(wtk_gainnet_bf4_t *gainnet_bf4,char *ifn,char *ofn)
{
    wtk_riff_t *riff;
    wtk_wavfile_t *wav;
    short *pv;
    int channel=gainnet_bf4->cfg->channel;
    int len,bytes;
    int ret;
    double t;
    int cnt;

    wav=wtk_wavfile_new(gainnet_bf4->cfg->rate);
    wav->max_pend=0;
    wtk_wavfile_open(wav,ofn);

    riff=wtk_riff_new();
    wtk_riff_open(riff,ifn);

    len=32*16;
    bytes=sizeof(short)*channel*len;
    pv=(short *)wtk_malloc(sizeof(short)*channel*len);

    wtk_gainnet_bf4_start(gainnet_bf4);
    wtk_gainnet_bf4_set_notify(gainnet_bf4,wav,(wtk_gainnet_bf4_notify_f)test_gainnet_bf4_on);
    wtk_gainnet_bf4_set_ssl_notify(gainnet_bf4,wav,(wtk_gainnet_bf4_notify_ssl_f)test_gainnet_bf4_on_ssl);

    // wtk_riff_read(riff,(char *)pv,56*channel);

    double tt=0.0;
    cnt=0;
    t=time_get_ms();
	static int count=0;
    while(1)
    {
        ret=wtk_riff_read(riff,(char *)pv,bytes);
        if(ret<=0)
        {
            wtk_debug("break ret=%d\n",ret);
            break;
        }
        len=ret/(sizeof(short)*channel);

		++count;
        cnt+=len;
	tt = time_get_ms();
	wtk_gainnet_bf4_feed(gainnet_bf4,pv,len,0);
		//if(count==10){
		//	wtk_gainnet_bf4_ssl_delay_new(gainnet_bf4);
		//}
	tt = time_get_ms() - tt;
	if(tt > 32.0)
	{
		wtk_debug("===============>>>>>>tm=%f\n",tt);
	}
    }
    wtk_gainnet_bf4_feed(gainnet_bf4,NULL,0,1);

    t=time_get_ms()-t;
    wtk_debug("rate=%f t=%f\n",t/(cnt/16.0),t);

    wtk_riff_delete(riff);
    wtk_wavfile_delete(wav);
    wtk_free(pv);
}


int main(int argc,char **argv)
{
    wtk_main_cfg_t *main_cfg=NULL;
    wtk_gainnet_bf4_cfg_t *cfg;
    wtk_gainnet_bf4_t *gainnet_bf4;
    wtk_arg_t *arg;
    char *cfg_fn,*ifn,*ofn;
    int tr=0;
    char *scp_fn=NULL;
    int enr=1;

    arg=wtk_arg_new(argc,argv);
    if(!arg){goto end;}
    wtk_arg_get_str_s(arg,"c",&cfg_fn);
    wtk_arg_get_str_s(arg,"i",&ifn);
    wtk_arg_get_str_s(arg,"o",&ofn);
    wtk_arg_get_str_s(arg,"scp",&scp_fn);
    wtk_arg_get_int_s(arg,"enr",&enr);
    wtk_arg_get_float_s(arg,"theta",&theta);
    wtk_arg_get_float_s(arg,"phi",&phi);
    if(!cfg_fn || (!tr&&(!ifn || !ofn)) ||  (tr&&!scp_fn&&!ifn))
    {
        print_usage();
        goto end;
    }

    main_cfg=wtk_main_cfg_new_type(wtk_gainnet_bf4_cfg,cfg_fn);
    if(!main_cfg){goto end;}

    cfg=(wtk_gainnet_bf4_cfg_t *)(main_cfg->cfg);
    gainnet_bf4=wtk_gainnet_bf4_new(cfg);

    {
        test_gainnet_bf4_file(gainnet_bf4,ifn,ofn);
    }

    wtk_gainnet_bf4_delete(gainnet_bf4);
end:
    if(arg)
    {
        wtk_arg_delete(arg);
    }
    if(main_cfg)
    {
        wtk_main_cfg_delete(main_cfg);
    }
    return 0;
}
