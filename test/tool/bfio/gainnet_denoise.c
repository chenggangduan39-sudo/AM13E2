#include "wtk/core/wtk_type.h"
#include "wtk/bfio/maskdenoise/wtk_gainnet_denoise.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/cfg/wtk_main_cfg.h"


static void print_usage()
{
	printf("gainnet_denoise usage:\n");
	printf("\t-c cfg\n");
	printf("\t-i input wav file\n");
	printf("\t-o output wav file\n");
    printf("\t-tr_fn output wav file\n");
    printf("\t-tr train or test\n");
	printf("\n");
}

static void test_gainnet_denoise_on(wtk_wavfile_t *wav,short *data,int len)
{
    int i;

    if(len>0)
    {
        for(i=0;i<len;++i)
        {
            data[i]*=1;
        }
        wtk_wavfile_write(wav,(char *)data,len*sizeof(short));
    }
}

static void test_gainnet_denoise_on_trfeat(FILE *f_out,float *feat,int len,float *target_g,int g_len,float  vad)
{
    fwrite(feat, sizeof(float), len, f_out);
    fwrite(target_g, sizeof(float), g_len, f_out);
    fwrite(&vad, sizeof(float), 1, f_out);
}

static void test_gainnet_denoise_get_trfeat(wtk_gainnet_denoise_t *gainnet_denoise,char *ifn,char *tr_fn)
{
    wtk_riff_t *riff;
    int len;
    int i;
    double t;
    int cnt;
	short *pv[10];
	wtk_strbuf_t **input;
    int n;

    FILE *f_out;

    riff=wtk_riff_new();
    wtk_riff_open(riff,ifn);

    f_out = fopen(tr_fn, "wb");
    wtk_gainnet_denoise_set_notify_tr(gainnet_denoise,f_out,(wtk_gainnet_denoise_notify_trfeat_f)test_gainnet_denoise_on_trfeat);

    t=time_get_ms();
    input=wtk_riff_read_channel(ifn,&n);
    for(i=0;i<n;++i)
    {
        pv[i]=(short*)(input[i]->data+56);
    }
    len=cnt=(input[0]->pos-56)/sizeof(short);
    t=time_get_ms();

    wtk_gainnet_denoise_feed_train(gainnet_denoise, pv, len);

    t=time_get_ms()-t;
    wtk_debug("rate=%f t=%f\n",t/(cnt/16.0),t);

    wtk_riff_delete(riff);
    wtk_strbufs_delete(input, n);
    fclose(f_out);
}

static void test_gainnet_denoise_file(wtk_gainnet_denoise_t *gainnet_denoise,char *ifn,char *ofn)
{
    wtk_riff_t *riff;
    wtk_wavfile_t *wav;
    short *pv;
    int len,bytes;
    int ret;
    double t;
    int cnt;

    wav=wtk_wavfile_new(16000);
    wav->max_pend=0;
    wtk_wavfile_open(wav,ofn);

    riff=wtk_riff_new();
    wtk_riff_open(riff,ifn);

    len=20*16;
    bytes=sizeof(short)*len;
    pv=(short *)wtk_malloc(sizeof(short)*len);

    wtk_gainnet_denoise_set_notify(gainnet_denoise,wav,(wtk_gainnet_denoise_notify_f)test_gainnet_denoise_on);

    // wtk_riff_read(riff,(char *)pv,56);

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
        wtk_gainnet_denoise_feed(gainnet_denoise,pv,len,0);
    }
    wtk_gainnet_denoise_feed(gainnet_denoise,NULL,0,1);
    wtk_gainnet_denoise_reset(gainnet_denoise);


    t=time_get_ms()-t;
    wtk_debug("rate=%f t=%f\n",t/(cnt/16.0),t);

    wtk_riff_delete(riff);
    wtk_wavfile_delete(wav);
    wtk_free(pv);
}


int main(int argc,char **argv)
{
    wtk_main_cfg_t *main_cfg=NULL;
    wtk_gainnet_denoise_cfg_t *cfg;
    wtk_gainnet_denoise_t *gainnet_denoise;
    wtk_arg_t *arg;
    char *cfg_fn,*ifn,*ofn;
    int tr=0;
    char *tr_fn;

    arg=wtk_arg_new(argc,argv);
    if(!arg){goto end;}
    wtk_arg_get_str_s(arg,"c",&cfg_fn);
    wtk_arg_get_str_s(arg,"i",&ifn);
    wtk_arg_get_str_s(arg,"o",&ofn);
    wtk_arg_get_str_s(arg,"tr_fn",&tr_fn);
    wtk_arg_get_int_s(arg,"tr",&tr);
    if(!cfg_fn || (!tr&&(!ifn || !ofn)) ||  (tr&&(!ifn || !tr_fn)))
    {
        print_usage();
        goto end;
    }

    main_cfg=wtk_main_cfg_new_type(wtk_gainnet_denoise_cfg,cfg_fn);
    if(!main_cfg){goto end;}

    cfg=(wtk_gainnet_denoise_cfg_t *)(main_cfg->cfg);
    gainnet_denoise=wtk_gainnet_denoise_new(cfg);

    if(tr)
    {
        test_gainnet_denoise_get_trfeat(gainnet_denoise,ifn,tr_fn);
    }else
    {
        test_gainnet_denoise_file(gainnet_denoise,ifn,ofn);
    }

    wtk_gainnet_denoise_delete(gainnet_denoise);
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