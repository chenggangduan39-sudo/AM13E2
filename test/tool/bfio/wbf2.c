#include "wtk/core/wtk_type.h"
#include "wtk/bfio/wtk_wbf2.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/cfg/wtk_main_cfg.h"


static void print_usage()
{
	printf("wbf2 usage:\n");
	printf("\t-c cfg\n");
	printf("\t-i input wav file\n");
	printf("\t-o output wav file\n");
	printf("\n");
}

static void test_wbf2_on(wtk_wavfile_t **wav,short *data,int len,int idx, int is_end)
{
    int i;

    if(len>0)
    {
        for(i=0;i<len;++i)
        {
            data[i]*=1;
        }
        wtk_wavfile_write(wav[idx],(char *)data,len*sizeof(short));
    }
}

static void test_wbf2_file(wtk_wbf2_t *wbf2,char *ifn,char *ofn)
{
    wtk_riff_t *riff;
    wtk_wavfile_t *wav[20];
    short *pv;
    short **data;
    int channel=wbf2->cfg->stft2.channel;
    int len,bytes;
    int ret;
    int i,j;
    double t;
    int cnt;
    char buf[512];


    for(i=0;i<wbf2->cfg->wbf2_cnt;++i)
    {
        wav[i]=wtk_wavfile_new(16000);
        wav[i]->max_pend=0;
        sprintf(buf, "%s%d.wav", ofn,i+1);
        wtk_wavfile_open(wav[i],buf);
    }

    riff=wtk_riff_new();
    wtk_riff_open(riff,ifn);

    len=20*16;
    bytes=sizeof(short)*channel*len;
    pv=(short *)wtk_malloc(sizeof(short)*channel*len);

    data=(short **)wtk_malloc(sizeof(short *)*channel);
    for(i=0;i<channel;++i)
    {
        data[i]=(short *)wtk_malloc(sizeof(short)*len);
    }
    wtk_wbf2_set_notify(wbf2,wav,(wtk_wbf2_notify_f)test_wbf2_on);
    wtk_wbf2_start(wbf2);

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
        len=ret/(channel*sizeof(short));
        for(i=0;i<len;++i)
        {
            for(j=0;j<channel;++j)
            {
                data[j][i]=pv[i*channel+j];
            }
        }
        cnt+=len;
        wtk_wbf2_feed(wbf2,data,len,0);
    }
    wtk_wbf2_feed(wbf2,NULL,0,1);


    t=time_get_ms()-t;
    wtk_debug("rate=%f t=%f\n",t/(cnt/16.0),t);

    wtk_riff_delete(riff);
    for(i=0;i<wbf2->cfg->wbf2_cnt;++i)
    {
        wtk_wavfile_delete(wav[i]);
    }
    wtk_free(pv);
    for(i=0;i<channel;++i)
    {
        wtk_free(data[i]);
    }
    wtk_free(data);
}


int main(int argc,char **argv)
{
    wtk_main_cfg_t *main_cfg=NULL;
    wtk_wbf2_cfg_t *cfg;
    wtk_wbf2_t *wbf2;
    wtk_arg_t *arg;
    char *cfg_fn,*ifn,*ofn;

    arg=wtk_arg_new(argc,argv);
    if(!arg){goto end;}
    wtk_arg_get_str_s(arg,"c",&cfg_fn);
    wtk_arg_get_str_s(arg,"i",&ifn);
    wtk_arg_get_str_s(arg,"o",&ofn);
    if(!cfg_fn || !ifn || !ofn)
    {
        print_usage();
        goto end;
    }

    main_cfg=wtk_main_cfg_new_type(wtk_wbf2_cfg,cfg_fn);
    if(!main_cfg){goto end;}

    cfg=(wtk_wbf2_cfg_t *)(main_cfg->cfg);

    wbf2=wtk_wbf2_new(cfg);

    test_wbf2_file(wbf2,ifn,ofn);

    wtk_wbf2_delete(wbf2);
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