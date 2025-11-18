#include "wtk/core/wtk_type.h"
#include "wtk/bfio/qform/wtk_qform9.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"

static int theta=0;  // 方位角
static int phi=0;  // 俯仰角
int win = 256;
int channel = 4;

typedef struct test_qform9
{
    wtk_drft_t *drft;
    float **analysis_mem;
	float *synthesis_mem;
    float *analysis_window;
    float *synthesis_window;
    float *rfft_in;
    wtk_complex_t **fft_in;
    wtk_complex_t **fft_feed;
    wtk_complex_t *fft_out;
    wtk_wavfile_t *wav;
}test_qform9_t;


test_qform9_t *test_qform9_init(int win,int channel,char *ofn,int rate)
{
    int i;
    test_qform9_t *test_qform9;
    int nbin=win/2+1;
    test_qform9=(test_qform9_t *)wtk_malloc(sizeof(test_qform9_t));
    test_qform9->drft=wtk_drft_new2(win);
    test_qform9->analysis_mem=(float **)wtk_malloc(sizeof(float *)*channel);
    for(i=0;i<channel;++i)
    {
        test_qform9->analysis_mem[i]=(float *)wtk_malloc(sizeof(float)*win);
        memset(test_qform9->analysis_mem[i],0,sizeof(float)*win);
    }
    test_qform9->synthesis_mem=(float *)wtk_malloc(sizeof(float)*win);
    memset(test_qform9->synthesis_mem,0,sizeof(float)*win);
    test_qform9->analysis_window=(float *)wtk_malloc(sizeof(float)*win);
    memset(test_qform9->analysis_window,0,sizeof(float)*win);
    test_qform9->synthesis_window=(float *)wtk_malloc(sizeof(float)*win);
    memset(test_qform9->synthesis_window,0,sizeof(float)*win);
	for (i=0;i<win;++i)
	{
		test_qform9->analysis_window[i] = sin((0.5+i)*PI/(win));
	}
	wtk_drft_init_synthesis_window(test_qform9->synthesis_window, test_qform9->analysis_window, win);
    test_qform9->rfft_in=(float *)wtk_malloc(sizeof(float)*win);
    memset(test_qform9->rfft_in,0,sizeof(float)*win);
    test_qform9->fft_in=(wtk_complex_t **)wtk_malloc(sizeof(wtk_complex_t *)*channel);
    for(i=0;i<channel;++i)
    {
        test_qform9->fft_in[i]=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*nbin);
        memset(test_qform9->fft_in[i],0,sizeof(wtk_complex_t)*nbin);
    }
    test_qform9->fft_feed=(wtk_complex_t **)wtk_malloc(sizeof(wtk_complex_t *)*nbin);
    for(i=0;i<nbin;++i)
    {
        test_qform9->fft_feed[i]=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*channel);
        memset(test_qform9->fft_feed[i],0,sizeof(wtk_complex_t)*channel);
    }
    test_qform9->fft_out=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*nbin);
    memset(test_qform9->fft_out,0,sizeof(wtk_complex_t)*nbin);

    test_qform9->wav=wtk_wavfile_new(rate);
    test_qform9->wav->max_pend=0;
    wtk_wavfile_open(test_qform9->wav,ofn);

    return test_qform9;
}

void test_qform9_delete(test_qform9_t *test_qform9)
{
    int i;
    int nbin=win/2+1;
    wtk_drft_delete2(test_qform9->drft);
    for(i=0;i<channel;++i)
    {
        wtk_free(test_qform9->analysis_mem[i]);
    }
    wtk_free(test_qform9->analysis_mem);
    wtk_free(test_qform9->synthesis_mem);
    wtk_free(test_qform9->analysis_window);
    wtk_free(test_qform9->synthesis_window);
    wtk_free(test_qform9->rfft_in);
    for(i=0;i<channel;++i)
    {
        wtk_free(test_qform9->fft_in[i]);
    }
    wtk_free(test_qform9->fft_in);
    for(i=0;i<nbin;++i)
    {
        wtk_free(test_qform9->fft_feed[i]);
    }
    wtk_free(test_qform9->fft_feed);
    wtk_free(test_qform9->fft_out);
    wtk_wavfile_close(test_qform9->wav);
    wtk_wavfile_delete(test_qform9->wav);
    wtk_free(test_qform9);
}

static void print_usage()
{
	printf("qform9 usage:\n");
	printf("\t-c cfg\n");
	printf("\t-i input wav file\n");
	printf("\t-o output wav file\n");
	printf("\n");
}

static void test_qform9_on(test_qform9_t *test_qform9,wtk_complex_t *fft,int is_end)
{
    float *out;
    short *pv;
    int i;
    out=(float *)wtk_malloc(sizeof(float)*win);
    wtk_drft_frame_synthesis22(test_qform9->drft, test_qform9->rfft_in, test_qform9->synthesis_mem, fft, out, win, test_qform9->synthesis_window);
    pv=(short *)out;
    for(i=0;i<win/2;++i){
        pv[i]=floor(out[i]+0.5);
    }
    wtk_wavfile_write(test_qform9->wav,(char *)pv,win/2*sizeof(short));

    wtk_free(out);
}

void test_qform9_feed(wtk_qform9_t *qform9,test_qform9_t *test_qform9,short **data,int len,int is_end)
{
    int i, j;
    int nbin=qform9->nbin;
    if(data){
        for(i=0;i<channel;++i){
            wtk_drft_frame_analysis22(test_qform9->drft,test_qform9->rfft_in, test_qform9->analysis_mem[i], test_qform9->fft_in[i], data[i], win, test_qform9->analysis_window);
        }
        for(i=0;i<nbin;++i){
            for(j=0;j<channel;++j){
                test_qform9->fft_feed[i][j]=test_qform9->fft_in[j][i];
            }
        }
        wtk_qform9_feed_fft(qform9,test_qform9->fft_feed,is_end);
    }
}


static void test_qform9_file(wtk_qform9_t *qform9,char *ifn,char *ofn)
{
    wtk_riff_t *riff;
    test_qform9_t *test_qform9;
    short *pv;
    short **data;
    int len,bytes;
    int ret;
    int i,j;
    double t;
    int cnt;

    riff=wtk_riff_new();
    wtk_riff_open(riff,ifn);

    test_qform9=test_qform9_init(win,channel,ofn,qform9->cfg->bf.rate);

    len=win/2;
    bytes=sizeof(short)*channel*len;
    pv=(short *)wtk_malloc(sizeof(short)*channel*len);

    data=(short **)wtk_malloc(sizeof(short *)*channel);
    for(i=0;i<channel;++i)
    {
        data[i]=(short *)wtk_malloc(sizeof(short)*len);
    }
    wtk_qform9_set_notify3(qform9,test_qform9,(wtk_qform9_notify_f3)test_qform9_on);
    wtk_qform9_start3(qform9,theta,phi);

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
        test_qform9_feed(qform9,test_qform9,data,len,0);
    }
    test_qform9_feed(qform9,test_qform9,NULL,0,1);


    t=time_get_ms()-t;
    wtk_debug("rate=%f t=%f\n",t/(cnt/16.0),t);

    wtk_riff_delete(riff);
    wtk_free(pv);
    test_qform9_delete(test_qform9);
    for(i=0;i<channel;++i)
    {
        wtk_free(data[i]);
    }
    wtk_free(data);
}


int main(int argc,char **argv)
{
    wtk_main_cfg_t *main_cfg=NULL;
    wtk_qform9_cfg_t *cfg;
    wtk_qform9_t *qform9;
    wtk_arg_t *arg;
    char *cfg_fn,*ifn,*ofn;

    arg=wtk_arg_new(argc,argv);
    if(!arg){goto end;}
    wtk_arg_get_str_s(arg,"c",&cfg_fn);
    wtk_arg_get_str_s(arg,"i",&ifn);
    wtk_arg_get_str_s(arg,"o",&ofn);
    wtk_arg_get_int_s(arg,"theta",&theta);
    wtk_arg_get_int_s(arg,"phi",&phi);
    if(!cfg_fn || !ofn)
    {
        print_usage();
        goto end;
    }

    main_cfg=wtk_main_cfg_new_type(wtk_qform9_cfg,cfg_fn);
    if(!main_cfg){goto end;}

    cfg=(wtk_qform9_cfg_t *)(main_cfg->cfg);

    qform9=wtk_qform9_new3(cfg, win, channel);

    test_qform9_file(qform9,ifn,ofn);

    wtk_qform9_delete3(qform9);
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
