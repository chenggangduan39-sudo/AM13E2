#include "wtk/core/wtk_type.h"
#include "wtk/bfio/ssl/wtk_ssl.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/cfg/wtk_main_cfg.h"

static void print_usage()
{
	printf("ssl usage:\n");
	printf("\t-c cfg\n");
	printf("\t-i input wav file\n");
	printf("\n");
}

static void test_ssl_file(wtk_ssl_t *ssl,char *ifn)
{
    wtk_riff_t *riff;
    short *pv;
    short **data;
    int channel=ssl->cfg->stft2.channel;
    int len,bytes;
    int ret;
    int i,j;
    double t;
    int cnt;

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
        wtk_ssl_feed(ssl,data,len,0);
    }
    wtk_ssl_feed(ssl,NULL,0,1);
    wtk_ssl_print(ssl);


    t=time_get_ms()-t;
    wtk_debug("rate=%f t=%f\n",t/(cnt/16.0),t);

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
    wtk_main_cfg_t *main_cfg=NULL;
    wtk_ssl_cfg_t *cfg;
    wtk_ssl_t *ssl;
    wtk_arg_t *arg;
    char *cfg_fn,*ifn;

    arg=wtk_arg_new(argc,argv);
    if(!arg){goto end;}
    wtk_arg_get_str_s(arg,"c",&cfg_fn);
    wtk_arg_get_str_s(arg,"i",&ifn);
    if(!cfg_fn || !ifn)
    {
        print_usage();
        goto end;
    }

    main_cfg=wtk_main_cfg_new_type(wtk_ssl_cfg,cfg_fn);
    if(!main_cfg){goto end;}

    cfg=(wtk_ssl_cfg_t *)(main_cfg->cfg);

    ssl=wtk_ssl_new(cfg);

    test_ssl_file(ssl,ifn);

    wtk_ssl_delete(ssl);
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