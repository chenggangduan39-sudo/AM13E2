#include "wtk/core/wtk_type.h"
#include "wtk/bfio/qform/wtk_rtjoin2.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/cfg/wtk_main_cfg.h"


static void print_usage()
{
	printf("rtjoin2 usage:\n");
	printf("\t-c cfg\n");
	printf("\t-i input wav file\n");
	printf("\t-o output wav file\n");
	printf("\t-chn feed2 choice_chn\n");
	printf("\n");
}

static void test_rtjoin2_on(wtk_wavfile_t *wav,short *data, int len, int is_end)
{
    if(len>0)
    {
        wtk_wavfile_write(wav,(char *)data, len<<1);
    }
}

static void test_rtjoin2_file(wtk_rtjoin2_t *rtjoin2,char *ifn,char *ofn,int chn)
{
    wtk_riff_t *riff;
    wtk_wavfile_t *wav;
    short *pv;
    int channel=rtjoin2->cfg->channel;
    int len,bytes;
    int ret;
    double t;
    int cnt;

    wav=wtk_wavfile_new(rtjoin2->cfg->rate);
    wav->max_pend=0;
    wtk_wavfile_open(wav,ofn);

    riff=wtk_riff_new();
    wtk_riff_open(riff,ifn);

    len=32*16;
    bytes=sizeof(short)*channel*len;
    pv=(short *)wtk_malloc(sizeof(short)*channel*len);

    wtk_rtjoin2_start(rtjoin2);
    wtk_rtjoin2_set_notify(rtjoin2,wav,(wtk_rtjoin2_notify_f)test_rtjoin2_on);

    // wtk_riff_read(riff,(char *)pv,56*channel);

    cnt=0;
    t=time_get_ms();
    if(chn!=-1){
        wtk_debug("choice_ch=%d\n", chn);
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
            wtk_rtjoin2_feed(rtjoin2,pv,len,0);
        }
        wtk_rtjoin2_feed(rtjoin2,NULL,0,1);
    }else{
        int repreat=1;
        while(repreat--){
            wtk_riff_open(riff,ifn);
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
                wtk_rtjoin2_feed(rtjoin2,pv,len,0);
            }
            wtk_rtjoin2_feed(rtjoin2,NULL,0,1);
        }
    }

    t=time_get_ms()-t;
    wtk_debug("rate=%f t=%f\n",t/(cnt/(rtjoin2->cfg->rate/1000.0)),t);

    wtk_riff_delete(riff);
    wtk_wavfile_delete(wav);
    wtk_free(pv);
}


int main(int argc,char **argv)
{
    wtk_rtjoin2_cfg_t *cfg=NULL;
    wtk_rtjoin2_t *rtjoin2;
    wtk_arg_t *arg;
    char *cfg_fn=NULL,*ifn,*ofn,*bin_fn=NULL;
    int chn=-1;

    arg=wtk_arg_new(argc,argv);
    if(!arg){goto end;}
    wtk_arg_get_str_s(arg,"c",&cfg_fn);
    wtk_arg_get_str_s(arg,"b",&bin_fn);
    wtk_arg_get_str_s(arg,"i",&ifn);
    wtk_arg_get_str_s(arg,"o",&ofn);
    wtk_arg_get_int_s(arg,"chn",&chn);
    if((!cfg_fn&&!bin_fn) || !ifn || !ofn)
    {
        print_usage();
        goto end;
    }

    if(cfg_fn)
    {
        cfg=wtk_rtjoin2_cfg_new(cfg_fn);
        if(!cfg){goto end;}
    }else
    {
        cfg=wtk_rtjoin2_cfg_new_bin(bin_fn);
        if(!cfg){goto end;}
    }

    rtjoin2=wtk_rtjoin2_new(cfg);
 
    {
        test_rtjoin2_file(rtjoin2,ifn,ofn,chn);
    }

    wtk_rtjoin2_delete(rtjoin2);
end:
    if(arg)
    {
        wtk_arg_delete(arg);
    }
    if(cfg)
    {
        if(cfg_fn)
        {
            wtk_rtjoin2_cfg_delete(cfg);
        }else if(bin_fn)
        {
            wtk_rtjoin2_cfg_delete_bin(cfg);
        }
    }
    return 0;
}
