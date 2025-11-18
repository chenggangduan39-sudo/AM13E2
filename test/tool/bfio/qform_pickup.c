#include "wtk/core/wtk_type.h"
#include "wtk/bfio/qform/wtk_qform_pickup.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/rbin/wtk_flist.h"

static int theta=0;
static int phi=0;

static void print_usage()
{
	printf("qform usage:\n");
	printf("\t-c cfg\n");
	printf("\t-i input wav file\n");
	printf("\t-o output wav file\n");
	printf("\n");
}

static void test_qform_pickup_on(wtk_wavfile_t *wav,short *data,int len)
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

static void test_qform_pickup_file(wtk_qform_pickup_t *qform,char *ifn,char *ofn,char *gravfn)
{
    wtk_riff_t *riff;
    wtk_wavfile_t *wav;
    short *pv;
    short **data;
    int channel=qform->cfg->stft.channel;
    int len,bytes;
    int ret;
    int i,j;
    double t;
    int cnt;
    wtk_flist_it_t *it;
    char *line;
    short x,y,z;

    wav=wtk_wavfile_new(qform->cfg->bf.rate);
    wav->max_pend=0;
    wtk_wavfile_open(wav,ofn);

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
    wtk_qform_pickup_set_notify(qform,wav,(wtk_qform_pickup_notify_f)test_qform_pickup_on);
    wtk_qform_pickup_start(qform,theta,phi);

    cnt=0;
    t=time_get_ms();
    // wtk_qform_pickup_set_grav(qform,10,10,10);

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
            wtk_qform_pickup_set_grav(qform,x,y,z);
        }
        cnt+=len;
        wtk_qform_pickup_feed(qform,data,len,0);
    }
    wtk_qform_pickup_feed(qform,NULL,0,1);

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
    wtk_qform_pickup_cfg_t *cfg;
    wtk_qform_pickup_t *qform;
    wtk_arg_t *arg;
    char *cfg_fn,*ifn,*ofn,*gravfn;

    arg=wtk_arg_new(argc,argv);
    if(!arg){goto end;}
    wtk_arg_get_str_s(arg,"c",&cfg_fn);
    wtk_arg_get_str_s(arg,"i",&ifn);
    wtk_arg_get_str_s(arg,"o",&ofn);
    wtk_arg_get_int_s(arg,"theta",&theta);
    wtk_arg_get_int_s(arg,"phi",&phi);
    wtk_arg_get_str_s(arg,"gravfn",&gravfn);
    if(!cfg_fn || !ifn || !ofn)
    {
        print_usage();
        goto end;
    }

    main_cfg=wtk_main_cfg_new_type(wtk_qform_pickup_cfg,cfg_fn);
    if(!main_cfg){goto end;}

    cfg=(wtk_qform_pickup_cfg_t *)(main_cfg->cfg);

    qform=wtk_qform_pickup_new(cfg);

    test_qform_pickup_file(qform,ifn,ofn,gravfn);

    wtk_qform_pickup_delete(qform);
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