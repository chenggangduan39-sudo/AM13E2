#include "wtk/core/wtk_type.h"
#include "wtk/core/checkmic/wtk_checkmic.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/cfg/wtk_main_cfg.h"

static void print_usage()
{
	printf("checkmic usage:\n");
	printf("\t-c cfg\n");
	printf("\t-i input wav file\n");
	printf("\n");
}

static void test_checkmic_on(wtk_checkmic_t *checkmic, void *ths, wtk_checkmic_diff_t *diff)
{
    int i;
    for(i=0;i<checkmic->combination;++i){
        printf("mic %d and mic %d phase=[%d] st_phase=[%d]\n", diff[i].channel[0]+1, diff[i].channel[1]+1, diff[i].phase, diff[i].st_phase);
    }
}

static void test_checkmic_file(wtk_checkmic_t *checkmic,char *ifn)
{
    wtk_riff_t *riff;
    short *pv;
    int channel=checkmic->channel;
    int len,bytes;
    int ret;
    double t;
    int cnt;
    short **data;
    int i,j;

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

    wtk_checkmic_set_notify(checkmic,NULL,(wtk_checkmic_notify_t)test_checkmic_on);

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
        wtk_checkmic_feed(checkmic,data,len,0);
    }
    wtk_checkmic_feed(checkmic,NULL,0,1);
    // wtk_checkmic_print(checkmic);


    t=time_get_ms()-t;
    wtk_debug("rate=%f t=%f\n",t/(cnt/16.0),t);

    wtk_riff_delete(riff);

    for(i=0;i<channel;++i)
    {
        wtk_free(data[i]);
    }
    wtk_free(data);
    wtk_free(pv);
}


int main(int argc,char **argv)
{
    wtk_main_cfg_t *main_cfg=NULL;
    wtk_checkmic_cfg_t *cfg;
    wtk_checkmic_t *checkmic;
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

    main_cfg=wtk_main_cfg_new_type(wtk_checkmic_cfg,cfg_fn);
    if(!main_cfg){goto end;}

    cfg=(wtk_checkmic_cfg_t *)(main_cfg->cfg);

    checkmic=wtk_checkmic_new(cfg);

    test_checkmic_file(checkmic,ifn);

    wtk_checkmic_delete(checkmic);
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