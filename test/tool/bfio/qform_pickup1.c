#include "wtk/core/wtk_type.h"
#include "wtk/bfio/qform/wtk_qform_pickup1.h"
#include "wtk/bfio/qform/wtk_qform_pickup2.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/wtk_wavfile.h"

static  wtk_wavfile_t *wav1=NULL;
static int theta=0;
static int phi=0;
static wtk_qform_pickup2_t *qform2=NULL;
static int out_len=0;
static int out2_len=0;

static void print_usage()
{
	printf("qform_pickup 1-2 usage:\n");
	printf("\t-c cfg\n");
	printf("\t-i input wav file\n");
	printf("\t-o1 output wav file\n");
    printf("\t-o2 output wav file\n");
	printf("\n");
}

static void test_qform_pickup2_on(wtk_wavfile_t *wav,short *data,int len)
{
    // int i;

    out2_len+=len;
    if(len>0)
    {
        // for(i=0;i<len;++i)
        // {
        //     data[i]*=1;
        // }
        wtk_wavfile_write(wav,(char *)data,len*sizeof(short));
    }
}

static void test_qform_pickup1_on(wtk_qform_pickup2_t *qform2,short *data,int len,char *cohv,int nbin)
{
    if(len>0)
    {
        wtk_wavfile_write(wav1,(char *)data,len*sizeof(short));
    }

    out_len+=len;
    wtk_qform_pickup2_feed(qform2, data, len, cohv, nbin, 0);
}

static void test_qform_pickup1_file(wtk_qform_pickup1_t *qform,char *ifn,char *gravfn)
{
    wtk_riff_t *riff;
    short *pv;
    short **data;
    int channel=qform->cfg->stft2.channel;
    int len,bytes;
    int ret;
    int i,j;
    double t;
    int cnt;
    wtk_flist_it_t *it;
    char *line;
    short x,y,z;

    riff=wtk_riff_new();
    wtk_riff_open(riff,ifn);

    len=32*16;
    bytes=sizeof(short)*channel*len;
    pv=(short *)wtk_malloc(sizeof(short)*channel*len);

    data=(short **)wtk_malloc(sizeof(short *)*channel);
    for(i=0;i<channel;++i)
    {
        data[i]=(short *)wtk_malloc(sizeof(short)*len);
    }
    wtk_qform_pickup1_start(qform,theta,phi);

    cnt=0;
    t=time_get_ms();
    // wtk_qform_pickup1_set_grav(qform,10,10,10);

    it=NULL;
    if(gravfn)
    {
        it=wtk_flist_it_new(gravfn);
    }
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
                data[j][i]=pv[i*channel+j];//*5;
            }
        }
        if(it)
        {
            line=wtk_flist_it_next(it);
            if(!line){break;}
            x=atoi(strtok(line,"=="));
		    y=atoi(strtok(NULL,"=="));
            z=atoi(strtok(NULL,"=="));
            // wtk_debug("[ %f - %f ] ms ==> \n",cnt/16.0,(cnt+len)/16.0);
            wtk_qform_pickup1_set_grav(qform,x,y,z);
        }
        cnt+=len;
        wtk_qform_pickup1_feed(qform,data,len,0);
    }
    wtk_qform_pickup1_feed(qform,NULL,0,1);

    wtk_qform_pickup2_feed(qform2,NULL,0,NULL,0,1);

    wtk_qform_pickup1_reset(qform);
    wtk_qform_pickup2_reset(qform2);

    t=time_get_ms()-t;
    wtk_debug("rate=%f t=%f  %d/%d/%d\n",t/(cnt/16.0),t, cnt,out_len,out2_len);

    wtk_riff_delete(riff);
    wtk_free(pv);
    for(i=0;i<channel;++i)
    {
        wtk_free(data[i]);
    }
    wtk_free(data);
}


int main(int argc,char **argv)
{
    wtk_main_cfg_t *main_cfg=NULL, *main_cfg2=NULL;
    wtk_qform_pickup1_cfg_t *cfg;
    wtk_qform_pickup2_cfg_t *cfg2;
    wtk_qform_pickup1_t *qform;
    wtk_arg_t *arg;
    char *cfg_fn,*cfg_fn2,*ifn,*ofn1,*ofn2,*gravfn;
    wtk_wavfile_t *wav2=NULL;

    arg=wtk_arg_new(argc,argv);
    if(!arg){goto end;}
    wtk_arg_get_str_s(arg,"c",&cfg_fn);
    wtk_arg_get_str_s(arg,"c2",&cfg_fn2);
    wtk_arg_get_str_s(arg,"i",&ifn);
    wtk_arg_get_str_s(arg,"o1",&ofn1);
    wtk_arg_get_str_s(arg,"o2",&ofn2);
    wtk_arg_get_int_s(arg,"theta",&theta);
    wtk_arg_get_int_s(arg,"phi",&phi);
    wtk_arg_get_str_s(arg,"gravfn",&gravfn);
    if(!cfg_fn || !ifn || !ofn1 ||  !ofn2)
    {
        print_usage();
        goto end;
    }

    main_cfg=wtk_main_cfg_new_type(wtk_qform_pickup1_cfg,cfg_fn);
    if(!main_cfg){goto end;}
    cfg=(wtk_qform_pickup1_cfg_t *)(main_cfg->cfg);
    qform=wtk_qform_pickup1_new(cfg);

    wav2=wtk_wavfile_new(qform->cfg->bf.rate);
    wav2->max_pend=0;
    wtk_wavfile_open(wav2,ofn2);

    wav1=wtk_wavfile_new(qform->cfg->bf.rate);
    wav1->max_pend=0;
    wtk_wavfile_open(wav1,ofn1);

    main_cfg2=wtk_main_cfg_new_type(wtk_qform_pickup2_cfg,cfg_fn2);
    if(!main_cfg2){goto end;}
    cfg2=(wtk_qform_pickup2_cfg_t *)(main_cfg2->cfg);
    qform2=wtk_qform_pickup2_new(cfg2);

    wtk_qform_pickup1_set_notify(qform,qform2,(wtk_qform_pickup1_notify_f)test_qform_pickup1_on);
    wtk_qform_pickup2_set_notify(qform2,wav2,(wtk_qform_pickup2_notify_f)test_qform_pickup2_on);

    test_qform_pickup1_file(qform,ifn,gravfn);

    wtk_qform_pickup1_delete(qform);
    wtk_qform_pickup2_delete(qform2);
end:
    if(wav1)
    {
        wtk_wavfile_delete(wav1);
    }
    if(wav2)
    {
        wtk_wavfile_delete(wav2);
    }
    if(arg)
    {
        wtk_arg_delete(arg);
    }
    if(main_cfg)
    {
        wtk_main_cfg_delete(main_cfg);
    }
    if(main_cfg2)
    {
        wtk_main_cfg_delete(main_cfg2);
    }
    return 0;
}