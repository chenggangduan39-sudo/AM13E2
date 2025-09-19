#include "wtk/core/wtk_type.h"
#include "wtk/bfio/wtk_eqform.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"

static int theta=0;
static int phi=0;

static void print_usage()
{
	printf("eqform usage:\n");
	printf("\t-c cfg\n");
	printf("\t-i input wav file\n");
	printf("\t-o output wav file\n");
	printf("\n");
}

static void test_eqform_on(wtk_wavfile_t *wav,short *data,int len)
{
    int i;

    if(len>0)
    {
        for(i=0;i<len;++i)
        {
            data[i]*=3;
        }
        wtk_wavfile_write(wav,(char *)data,len*sizeof(short));
    }
}

static void test_eqform_on_enrcheck(wtk_eqform_t *eqform,float enr_thresh,float enr, int on_run)
{
    printf("enr check %f/%f on_run\n", enr, enr_thresh);
}

static void test_eqform_file(wtk_eqform_t *eqform,char *ifn,char *ofn)
{
    wtk_riff_t *riff;
    wtk_wavfile_t *wav;
    short *pv;
    short **data;
    int channel=eqform->channel;
    int len,bytes;
    int ret;
    int i,j;
    double t;
    int cnt;

    wav=wtk_wavfile_new(16000);
    wav->max_pend=0;
    if(eqform->cfg->use_qform11){
        wtk_wavfile_set_channel(wav, eqform->qform11->cfg->nmulchannel);
    }
    wtk_wavfile_open(wav,ofn);

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
    wtk_eqform_set_notify(eqform,wav,(wtk_eqform_notify_f)test_eqform_on);
    wtk_eqform_set_enrcheck_notify(eqform,eqform,(wtk_eqform_notify_enrcheck_f)test_eqform_on_enrcheck);
    wtk_eqform_start(eqform,theta,phi);

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
                data[j][i]=pv[i*channel+j];//*5;
            }
        }
        cnt+=len;
        wtk_eqform_feed(eqform,data,len,0);
    }
    wtk_eqform_feed(eqform,NULL,0,1);


    t=time_get_ms()-t;
    wtk_debug("rate=%f t=%f\n",t/(cnt/16.0),t);

    wtk_riff_delete(riff);
    wtk_wavfile_delete(wav);
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
    wtk_eqform_cfg_t *cfg;
    wtk_eqform_t *eqform;
    wtk_arg_t *arg;
    char *cfg_fn,*ifn,*ofn;

    arg=wtk_arg_new(argc,argv);
    if(!arg){goto end;}
    wtk_arg_get_str_s(arg,"c",&cfg_fn);
    wtk_arg_get_str_s(arg,"i",&ifn);
    wtk_arg_get_str_s(arg,"o",&ofn);
    wtk_arg_get_int_s(arg,"theta",&theta);
    wtk_arg_get_int_s(arg,"phi",&phi);
    if(!cfg_fn || !ifn || !ofn)
    {
        print_usage();
        goto end;
    }

    main_cfg=wtk_main_cfg_new_type(wtk_eqform_cfg,cfg_fn);
    if(!main_cfg){goto end;}

    cfg=(wtk_eqform_cfg_t *)(main_cfg->cfg);

    eqform=wtk_eqform_new(cfg);

    test_eqform_file(eqform,ifn,ofn);

    wtk_eqform_delete(eqform);
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