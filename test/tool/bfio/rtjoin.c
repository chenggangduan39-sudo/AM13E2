#include "wtk/core/wtk_type.h"
#include "wtk/bfio/qform/wtk_rtjoin.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/cfg/wtk_main_cfg.h"


static void print_usage()
{
	printf("rtjoin usage:\n");
	printf("\t-c cfg\n");
	printf("\t-i input wav file\n");
	printf("\t-o output wav file\n");
	printf("\n");
}

static void test_rtjoin_on(wtk_wavfile_t *wav,short *data, int len, int is_end)
{
    if(len>0)
    {
        wtk_wavfile_write(wav,(char *)data, len<<1);
    }
}

static void test_rtjoin_file(wtk_rtjoin_t *rtjoin,char *ifn,char *ofn)
{
    wtk_riff_t *riff;
    wtk_wavfile_t *wav;
    short *pv;
    int channel=rtjoin->cfg->channel;
    int len,bytes;
    int ret;
    double t;
    int cnt;

    wav=wtk_wavfile_new(rtjoin->cfg->rate);
    wav->max_pend=0;
    wtk_wavfile_open(wav,ofn);

    riff=wtk_riff_new();
    wtk_riff_open(riff,ifn);

    len=32*16;
    bytes=sizeof(short)*channel*len;
    pv=(short *)wtk_malloc(sizeof(short)*channel*len);

    wtk_rtjoin_start(rtjoin);
    wtk_rtjoin_set_notify(rtjoin,wav,(wtk_rtjoin_notify_f)test_rtjoin_on);

    // wtk_riff_read(riff,(char *)pv,56*channel);

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
        len=ret/(sizeof(short)*channel);

        cnt+=len;
        wtk_rtjoin_feed(rtjoin,pv,len,0);
    }
    wtk_rtjoin_feed(rtjoin,NULL,0,1);

    t=time_get_ms()-t;
    wtk_debug("rate=%f t=%f\n",t/(cnt/(rtjoin->cfg->rate/1000.0)),t);

    wtk_riff_delete(riff);
    wtk_wavfile_delete(wav);
    wtk_free(pv);
}


int main(int argc,char **argv)
{
    wtk_rtjoin_cfg_t *cfg=NULL;
    wtk_rtjoin_t *rtjoin;
    wtk_arg_t *arg;
    char *cfg_fn=NULL,*ifn,*ofn,*bin_fn=NULL;

    arg=wtk_arg_new(argc,argv);
    if(!arg){goto end;}
    wtk_arg_get_str_s(arg,"c",&cfg_fn);
    wtk_arg_get_str_s(arg,"b",&bin_fn);
    wtk_arg_get_str_s(arg,"i",&ifn);
    wtk_arg_get_str_s(arg,"o",&ofn);
    if((!cfg_fn&&!bin_fn) || !ifn || !ofn)
    {
        print_usage();
        goto end;
    }

    if(cfg_fn)
    {
        cfg=wtk_rtjoin_cfg_new(cfg_fn);
        if(!cfg){goto end;}
    }else
    {
        cfg=wtk_rtjoin_cfg_new_bin(bin_fn);
        if(!cfg){goto end;}
    }

    rtjoin=wtk_rtjoin_new(cfg);
 
    {
        test_rtjoin_file(rtjoin,ifn,ofn);
    }

    wtk_rtjoin_delete(rtjoin);
end:
    if(arg)
    {
        wtk_arg_delete(arg);
    }
    if(cfg)
    {
        if(cfg_fn)
        {
            wtk_rtjoin_cfg_delete(cfg);
        }else if(bin_fn)
        {
            wtk_rtjoin_cfg_delete_bin(cfg);
        }
    }
    return 0;
}
