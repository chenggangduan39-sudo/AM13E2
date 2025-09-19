#include "wtk/core/wtk_type.h"
#include "wtk/bfio/qform/wtk_qform_pickup5.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/wtk_wavfile.h"

static void print_usage()
{
	printf("qform usage:\n");
	printf("\t-c cfg\n");
	printf("\t-i input wav file\n");
	printf("\t-o output wav file\n");
	printf("\n");
}

static void test_qform_pickup5_on(wtk_wavfile_t *wav,short *data,int len)
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

static void test_qform_pickup5_file(wtk_qform_pickup5_t *qform,char *ifn,char *ofn)
{
    wtk_riff_t *riff;
    wtk_wavfile_t *wav;
    short *pv;
    short **data;
    int channel=CHANNEL;
    int len,bytes;
    int ret;
    int i,j;
    double t;
    int cnt;

    wav=wtk_wavfile_new(RATE);
    wav->max_pend=0;
    wtk_wavfile_open(wav,ofn);
    wtk_wavfile_set_channel(wav, OUT_CHANNEL);
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
    wtk_qform_pickup5_set_notify(qform,wav,(wtk_qform_pickup5_notify_f)test_qform_pickup5_on);
    wtk_qform_pickup5_start(qform);

    cnt=0;
    t=time_get_ms();
    // wtk_qform_pickup5_set_grav(qform,10,10,10);

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
        wtk_qform_pickup5_feed(qform,data,len,0);
    }
    wtk_qform_pickup5_feed(qform,NULL,0,1);

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
    wtk_qform_pickup5_t *qform;
    wtk_arg_t *arg;
    char *ifn,*ofn;

    arg=wtk_arg_new(argc,argv);
    if(!arg){goto end;}
    wtk_arg_get_str_s(arg,"i",&ifn);
    wtk_arg_get_str_s(arg,"o",&ofn);
    if(!ifn || !ofn)
    {
        print_usage();
        goto end;
    }

    qform=wtk_qform_pickup5_new();

    test_qform_pickup5_file(qform,ifn,ofn);

    wtk_qform_pickup5_delete(qform);
end:
    if(arg)
    {
        wtk_arg_delete(arg);
    }
    return 0;
}