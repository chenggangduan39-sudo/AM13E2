#include "wtk/core/wtk_type.h"
#include "wtk/bfio/aspec/wtk_aspecm.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"

int win = 512;
int nbin = 257;
int channel = 4;

typedef struct test_aspecm
{
    wtk_drft_t *drft;
    float **analysis_mem;
	float *synthesis_mem;
    float *analysis_window;
    float *synthesis_window;
    float *rfft_in;
    wtk_complex_t **fft_in;
    wtk_complex_t *fft_out;
    wtk_wavfile_t *wav;
}test_aspecm_t;


test_aspecm_t *test_aspecm_init(int win,int channel,char *ofn,int rate)
{
    int i;
    test_aspecm_t *test_aspecm;
    test_aspecm=(test_aspecm_t *)wtk_malloc(sizeof(test_aspecm_t));
    test_aspecm->drft=wtk_drft_new2(win);
    test_aspecm->analysis_mem=(float **)wtk_malloc(sizeof(float *)*channel);
    for(i=0;i<channel;++i)
    {
        test_aspecm->analysis_mem[i]=(float *)wtk_malloc(sizeof(float)*win);
        memset(test_aspecm->analysis_mem[i],0,sizeof(float)*win);
    }
    test_aspecm->synthesis_mem=(float *)wtk_malloc(sizeof(float)*win);
    memset(test_aspecm->synthesis_mem,0,sizeof(float)*win);
    test_aspecm->analysis_window=(float *)wtk_malloc(sizeof(float)*win);
    memset(test_aspecm->analysis_window,0,sizeof(float)*win);
    test_aspecm->synthesis_window=(float *)wtk_malloc(sizeof(float)*win);
    memset(test_aspecm->synthesis_window,0,sizeof(float)*win);
	for (i=0;i<win;++i)
	{
		test_aspecm->analysis_window[i] = sin((0.5+i)*PI/(win));
	}
	wtk_drft_init_synthesis_window(test_aspecm->synthesis_window, test_aspecm->analysis_window, win);
    test_aspecm->rfft_in=(float *)wtk_malloc(sizeof(float)*win);
    memset(test_aspecm->rfft_in,0,sizeof(float)*win);
    test_aspecm->fft_in=(wtk_complex_t **)wtk_malloc(sizeof(wtk_complex_t *)*channel);
    for(i=0;i<channel;++i)
    {
        test_aspecm->fft_in[i]=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*nbin);
        memset(test_aspecm->fft_in[i],0,sizeof(wtk_complex_t)*nbin);
    }
    test_aspecm->fft_out=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*nbin);
    memset(test_aspecm->fft_out,0,sizeof(wtk_complex_t)*nbin);

    test_aspecm->wav=wtk_wavfile_new(rate);
    test_aspecm->wav->max_pend=0;
    wtk_wavfile_open(test_aspecm->wav,ofn);

    return test_aspecm;
}

void test_aspecm_delete(test_aspecm_t *test_aspecm)
{
    int i;
    wtk_drft_delete2(test_aspecm->drft);
    for(i=0;i<channel;++i)
    {
        wtk_free(test_aspecm->analysis_mem[i]);
    }
    wtk_free(test_aspecm->analysis_mem);
    wtk_free(test_aspecm->synthesis_mem);
    wtk_free(test_aspecm->analysis_window);
    wtk_free(test_aspecm->synthesis_window);
    wtk_free(test_aspecm->rfft_in);
    for(i=0;i<channel;++i)
    {
        wtk_free(test_aspecm->fft_in[i]);
    }
    wtk_free(test_aspecm->fft_in);
    wtk_free(test_aspecm->fft_out);
    wtk_wavfile_close(test_aspecm->wav);
    wtk_wavfile_delete(test_aspecm->wav);
    wtk_free(test_aspecm);
}

static void print_usage()
{
	printf("aspecm usage:\n");
	printf("\t-c cfg\n");
	printf("\t-i input wav file\n");
	printf("\t-o output wav file\n");
	printf("\n");
}

static void test_aspecm_on(test_aspecm_t *test_aspecm,wtk_complex_t *fft,int is_end)
{
    float *out;
    short *pv;
    int i;
    out=(float *)wtk_malloc(sizeof(float)*win);
    wtk_drft_frame_synthesis22(test_aspecm->drft, test_aspecm->rfft_in, test_aspecm->synthesis_mem, fft, out, win, test_aspecm->synthesis_window);
    pv=(short *)out;
    for(i=0;i<win/2;++i){
        pv[i]=floor(out[i]+0.5);
    }
    wtk_wavfile_write(test_aspecm->wav,(char *)pv,win/2*sizeof(short));

    wtk_free(out);
}

void test_aspecm_feed(wtk_aspecm_t *aspecm,test_aspecm_t *test_aspecm,short **data,int len,int is_end)
{
    int i;
    if(data){
        for(i=0;i<channel;++i){
            wtk_drft_frame_analysis22(test_aspecm->drft,test_aspecm->rfft_in, test_aspecm->analysis_mem[i], test_aspecm->fft_in[i], data[i], win, test_aspecm->analysis_window);
        }
        wtk_aspecm_feed_aspec(aspecm,test_aspecm->fft_in,test_aspecm->fft_in[0]);
        test_aspecm_on(test_aspecm,test_aspecm->fft_in[0],is_end);
    }
}


static void test_aspecm_file(wtk_aspecm_t *aspecm,char *ifn,char *ofn)
{
    wtk_riff_t *riff;
    test_aspecm_t *test_aspecm;
    short *pv;
    short **data;
    int len,bytes;
    int ret;
    int i,j;
    double t;
    int cnt;

    riff=wtk_riff_new();
    wtk_riff_open(riff,ifn);

    test_aspecm=test_aspecm_init(win,channel,ofn,aspecm->cfg->rate);

    len=win/2;
    bytes=sizeof(short)*channel*len;
    pv=(short *)wtk_malloc(sizeof(short)*channel*len);

    data=(short **)wtk_malloc(sizeof(short *)*channel);
    for(i=0;i<channel;++i)
    {
        data[i]=(short *)wtk_malloc(sizeof(short)*len);
    }
    wtk_aspecm_start(aspecm);

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
        test_aspecm_feed(aspecm,test_aspecm,data,len,0);
    }
    test_aspecm_feed(aspecm,test_aspecm,NULL,0,1);


    t=time_get_ms()-t;
    wtk_debug("rate=%f t=%f\n",t/(cnt/16.0),t);

    wtk_riff_delete(riff);
    wtk_free(pv);
    test_aspecm_delete(test_aspecm);
    for(i=0;i<channel;++i)
    {
        wtk_free(data[i]);
    }
    wtk_free(data);
}


int main(int argc,char **argv)
{
    wtk_main_cfg_t *main_cfg=NULL;
    wtk_aspecm_cfg_t *cfg;
    wtk_aspecm_t *aspecm;
    wtk_arg_t *arg;
    char *cfg_fn,*ifn,*ofn;

    arg=wtk_arg_new(argc,argv);
    if(!arg){goto end;}
    wtk_arg_get_str_s(arg,"c",&cfg_fn);
    wtk_arg_get_str_s(arg,"i",&ifn);
    wtk_arg_get_str_s(arg,"o",&ofn);
    if(!cfg_fn || !ofn)
    {
        print_usage();
        goto end;
    }

    main_cfg=wtk_main_cfg_new_type(wtk_aspecm_cfg,cfg_fn);
    if(!main_cfg){goto end;}

    cfg=(wtk_aspecm_cfg_t *)(main_cfg->cfg);

    aspecm=wtk_aspecm_new(cfg, nbin, channel);

    test_aspecm_file(aspecm,ifn,ofn);

    wtk_aspecm_delete(aspecm);
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
