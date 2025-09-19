#include "wtk/core/wtk_type.h"
#include "wtk/bfio/aec/wtk_aec.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "speex/speex_resampler.h"

float sum=0.0;

static void test_resample_file(char *ifn,char *ofn)
{
    wtk_wavfile_t *wav;
    char *data,*ss,*ee;
    int channel=1;
    int len;
    int nx,step;
    double t;
    int cnt;
    SpeexResamplerState *in_resample;


    in_resample = speex_resampler_init(channel, 48000, 16000, SPEEX_RESAMPLER_QUALITY_DESKTOP, NULL);
    wav=wtk_wavfile_new(16000);
    wav->max_pend=0;
    wtk_wavfile_open(wav,ofn);
    wtk_wavfile_set_channel(wav, channel);

    step=32*96*channel;
    data = file_read_buf(ifn, &len);

	int inlen;
	int outlen=step*2;
	char *outresample=(char *)wtk_malloc(outlen);
    cnt=0;
    t=time_get_ms();

    ss=data+44;
    ee=data+len-44;
    while(ss<ee)
    {
        nx=min(ee-ss, step);
		memset(outresample, 0, nx);
		inlen=(nx>>1)/channel;
		outlen=inlen;

		if(in_resample)
		{
			speex_resampler_process_interleaved_int(in_resample,
										(spx_int16_t *)(ss), (spx_uint32_t *)(&inlen), 
										(spx_int16_t *)(outresample), (spx_uint32_t *)(&outlen));
		}
        if(wav)
        {
            wtk_wavfile_write(wav, outresample, outlen*channel*2);
        }

        cnt+=nx;
        ss+=nx;
    }


    t=time_get_ms()-t;
    wtk_debug("rate=%f t=%f sum=%f\n",t/(cnt/32.0),t,sum);

    wtk_wavfile_delete(wav);
    wtk_free(data);
}


int main(int argc,char **argv)
{
    wtk_arg_t *arg;
    char *ifn,*ofn;

    arg=wtk_arg_new(argc,argv);
    if(!arg){goto end;}

    wtk_arg_get_str_s(arg,"i",&ifn);
    wtk_arg_get_str_s(arg,"o",&ofn);

    test_resample_file(ifn,ofn);

end:
    if(arg)
    {
        wtk_arg_delete(arg);
    }
    return 0;
}
