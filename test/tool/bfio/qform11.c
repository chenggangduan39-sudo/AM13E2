#include "wtk/core/wtk_type.h"
#include "wtk/bfio/qform/wtk_qform11.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/cfg/wtk_main_cfg.h"

static int theta=0;  // 方位角
static int phi=0;  // 俯仰角

static void print_usage()
{
	printf("qform11 usage:\n");
	printf("\t-c cfg\n");
	printf("\t-i input wav file\n");
	printf("\t-o output wav file\n");
	printf("\n");
}

static void test_qform11_on(wtk_wavfile_t *wav,short *data,int len,int is_end)
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

static void test_qform11_file2(wtk_qform11_t *qform11,char *scp,char *odir)
{
    wtk_flist_it_t *it;
    wtk_riff_t *riff;
    wtk_wavfile_t *wav;
    short *pv;
    short **data;
    int channel=qform11->cfg->stft2.channel;
    int len,bytes;
    int ret;
    int i,j;
    char *ifn=NULL,*ofn=NULL;
    wtk_strbuf_t *buf,*buf2;

    buf=wtk_strbuf_new(256,1);
    buf2=wtk_strbuf_new(256,1);

    wav=wtk_wavfile_new(qform11->cfg->bf.rate);
    wav->max_pend=0;
    riff=wtk_riff_new();

    len=20*16;
    bytes=sizeof(short)*channel*len;
    pv=(short *)wtk_malloc(sizeof(short)*channel*len);

    data=(short **)wtk_malloc(sizeof(short *)*channel);
    for(i=0;i<channel;++i)
    {
        data[i]=(short *)wtk_malloc(sizeof(short)*len);
    }
    wtk_qform11_set_notify(qform11,wav,(wtk_qform11_notify_f)test_qform11_on);
    wtk_qform11_start(qform11,theta,phi);

	it=wtk_flist_it_new(scp);
	while(1)
	{
		ifn=wtk_flist_it_next(it);
		if(!ifn)
		{
            break;
		}


        printf("%s\n",ifn);
        wtk_strbuf_reset(buf);
        wtk_strbuf_push(buf,ifn,strlen(ifn)+1);
        ofn=buf->data+(buf->pos-7);
        wtk_strbuf_reset(buf2);
        wtk_strbuf_push(buf2,odir,strlen(odir));
        wtk_strbuf_push(buf2,"/",1);
        wtk_strbuf_push(buf2,ofn,strlen(ofn)+1);
        ofn=buf2->data;
        printf("%s\n",ofn);

        wtk_wavfile_open(wav,ofn);
        wtk_riff_open(riff,ifn);

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
            wtk_qform11_feed(qform11,data,len,0);
        }

        wtk_riff_close(riff);
        wtk_wavfile_close(wav);
	}
    wtk_qform11_feed(qform11,NULL,0,1);

    wtk_strbuf_delete(buf);

    wtk_flist_it_delete(it);
    wtk_riff_delete(riff);
    wtk_wavfile_delete(wav);
    wtk_free(pv);
    for(i=0;i<channel;++i)
    {
        wtk_free(data[i]);
    }
    wtk_free(data);
}


static void test_qform11_file(wtk_qform11_t *qform11,char *ifn,char *ofn)
{
    wtk_riff_t *riff;
    wtk_wavfile_t *wav;
    short *pv;
    short **data;
    int channel=qform11->cfg->stft2.channel;
    int len,bytes;
    int ret;
    int i,j;
    double t;
    int cnt;

    wav=wtk_wavfile_new(qform11->cfg->bf.rate);
    wtk_wavfile_set_channel(wav, qform11->cfg->nmulchannel);
    wav->max_pend=0;
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
    wtk_qform11_set_notify(qform11,wav,(wtk_qform11_notify_f)test_qform11_on);
    wtk_qform11_start(qform11,theta,phi);

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
        wtk_qform11_feed(qform11,data,len,0);
    }
    wtk_qform11_feed(qform11,NULL,0,1);


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
    wtk_qform11_cfg_t *cfg;
    wtk_qform11_t *qform11;
    wtk_arg_t *arg;
    char *cfg_fn,*ifn,*ofn;
    char *scp=NULL;

    arg=wtk_arg_new(argc,argv);
    if(!arg){goto end;}
    wtk_arg_get_str_s(arg,"c",&cfg_fn);
    wtk_arg_get_str_s(arg,"i",&ifn);
    wtk_arg_get_str_s(arg,"o",&ofn);
    wtk_arg_get_int_s(arg,"theta",&theta);
    wtk_arg_get_int_s(arg,"phi",&phi);
    wtk_arg_get_str_s(arg,"scp",&scp);
    if(!cfg_fn || (!ifn&&!scp) || !ofn)
    {
        print_usage();
        goto end;
    }

    main_cfg=wtk_main_cfg_new_type(wtk_qform11_cfg,cfg_fn);
    if(!main_cfg){goto end;}

    cfg=(wtk_qform11_cfg_t *)(main_cfg->cfg);

    qform11=wtk_qform11_new(cfg);


    if(scp)
    {
        test_qform11_file2(qform11,scp,ofn);
    }else
    {
        test_qform11_file(qform11,ifn,ofn);
    }

    wtk_qform11_delete(qform11);
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
