#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/core/cfg/wtk_opt.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/wtk_os.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/bfio/resample/wtk_resample.h"

void print_usage()
{
	printf("ahs usage:\n");
	printf("\t-i input wav  file\n");
	printf("\t-o output wav file\n");
	printf("\t-r output  sample rate\n");
	printf("\n\n");
}

static void test_on_data(wtk_wavfile_t *wav, char* data, int len ){
	int i;

    if(len>0)
    {
        for(i=0;i<len;++i)
        {
            data[i]*=1;
        }
        wtk_wavfile_write(wav,data,len);
    }
}

int main(int argc,char **argv)
{
	char *ifn,*ofn;
	wtk_resample_t *resample;
	wtk_strbuf_t *buf;
	wtk_riff_t *riff;
	int rate;
	int samples;
	int channel;
	int i,nx,ret;
	// wtk_strbuf_t **bufs;
	char *pv;
	wtk_wavfile_t *wav;
	int out_sample_rate;

	// ifn=argv[1];
	// ofn=argv[2];
	wtk_arg_t *arg=wtk_arg_new(argc,argv);
	// if(!arg){goto end;}
	wtk_arg_get_str_s(arg,"i",&ifn);
	wtk_arg_get_str_s(arg,"o",&ofn);
	wtk_arg_get_int_s(arg,"r",&out_sample_rate);
	if(ifn==NULL || ofn==NULL || out_sample_rate<=0){
		print_usage();
		return -1;
	}

	wav=wtk_wavfile_new(out_sample_rate);
    wav->max_pend=0;
	wtk_wavfile_open(wav,ofn);

	riff=wtk_riff_new();
	wtk_riff_open(riff,ifn);
	rate=riff->fmt.sample_rate;
	int byte_num=riff->fmt.bit_per_sample/8;
	channel=riff->fmt.channels;

	int len = 0.008*rate;
	// wtk_debug("nx=%d\n",nx);
	pv=(char*)wtk_malloc(len*byte_num);
	resample=wtk_resample_new2(len, rate, out_sample_rate);
	wtk_resample_start2(resample,rate,out_sample_rate);
	wtk_resample_set_notify2(resample,wav,(wtk_resample_notify_f)test_on_data);
	while(1)
	{
		ret=wtk_riff_read(riff,(char*)pv,len*byte_num);
		if(ret <= 0)
		{
			break;
		}
		// wtk_wavfile_write(wav,pv,ret);
		wtk_resample_feed2(resample,(short *)pv,ret/2,0);
	}
	// wtk_resample_feed2(resample,NULL,0,1);

	wtk_wavfile_delete(wav);
	wtk_resample_delete2(resample);
	wtk_arg_delete(arg);
	return 0;
}
