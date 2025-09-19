#include "wtk/bfio/wtk_bfio4.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/json/wtk_json_parse.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_wavfile.h"

// 单通道降噪唤醒
static int wake_cnt=0;
static int out_len=0;

static void print_usage()
{
	printf("bfio4 usage:\n");
	printf("\t-c cfg\n");
	printf("\t-i input wav file\n");
	printf("\t-o output wav file\n");
	printf("\n");
}

static void test_bfio4_on(wtk_bfio4_t *bfio4,wtk_bfio4_cmd_t cmd, int id)
{
    switch(cmd)
    {
    case WTK_BFIO4_WAKE:
        printf("%f %f\n", bfio4->wake_fs, bfio4->wake_fe);
        ++wake_cnt;
        break;	
    case WTK_BFIO4_ASR:
        printf("%f %f ", bfio4->asr_fs, bfio4->asr_fe);
        switch (id)
        {
            case 1:
                printf("打开音乐");
                break;
            case 2:
                printf("播放音乐");
                break;
            case 3:
                printf("暂停播放");
                break;
            case 4:
                printf("继续播放");
                break;
            case 5:
                printf("播放上一首");
                break;
            case 6:
                printf("播放下一首");
                break;
            case 7:
                printf("增大音量");
                break;
            case 8:
                printf("减小音量");
                break;
            case 9:
                printf("上一首");
                break;
            case 10:
                printf("下一首");
                break;
            case 11:
                printf("打开健康码");
                break;
            case 12:
                printf("打开核酸码");
                break;
            default:
                break;
        }
        printf("\n");
    default:
        break;
    }
}


static void test_bfio4_file(wtk_bfio4_t *bfio4,char *ifn)
{
    wtk_riff_t *riff;

    short *pv;
    int channel;
    int len,bytes;
    int ret;
    double t;
    int cnt;

    riff=wtk_riff_new();
    wtk_riff_open(riff,ifn);

    channel = 1;
    len=20*16;
    bytes=sizeof(short)*channel*len;
    pv=(short *)wtk_malloc(sizeof(short)*channel*len);

    wtk_bfio4_set_notify(bfio4,bfio4,(wtk_bfio4_notify_f)test_bfio4_on);
    wtk_bfio4_start(bfio4);

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
        cnt+=len;
        wtk_bfio4_feed(bfio4,pv,len,0);
        // wtk_msleep(20);
    }
    wtk_bfio4_feed(bfio4,NULL,0,1);


    t=time_get_ms()-t;
    wtk_debug("===>>> wake_cnt=%d rate=%f t=%f cnt=%d out=%d\n",wake_cnt,t/(cnt/16.0),t,cnt,out_len);

    wtk_riff_delete(riff);
    wtk_free(pv);
}


int main(int argc,char **argv)
{
    wtk_bfio4_cfg_t *cfg;
    wtk_bfio4_t *bfio4;
    wtk_arg_t *arg;
    char *cfg_fn,*ifn,*bin_fn;

    arg=wtk_arg_new(argc,argv);
    if(!arg){goto end;}
    wtk_arg_get_str_s(arg,"c",&cfg_fn);
    wtk_arg_get_str_s(arg,"b",&bin_fn);
    wtk_arg_get_str_s(arg,"i",&ifn);
    if((!cfg_fn && !bin_fn) || (!ifn))
    {
        print_usage();
        goto end;
    }

    if(cfg_fn)
    {
        cfg=wtk_bfio4_cfg_new(cfg_fn);
    }
    if(bin_fn)
    {
        cfg=wtk_bfio4_cfg_new_bin(bin_fn);
    }
    // wtk_bfio4_cfg_set_wakeword(cfg, "小爱同学");
    bfio4=wtk_bfio4_new(cfg);

    test_bfio4_file(bfio4,ifn);

    wtk_bfio4_delete(bfio4);
end:
    if(arg)
    {
        wtk_arg_delete(arg);
    }
    if(bin_fn)
    {
        wtk_bfio4_cfg_delete_bin(cfg);
    }
    if(cfg_fn)
    {
        wtk_bfio4_cfg_delete(cfg);
    }
    return 0;
}
