#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/core/cfg/wtk_opt.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/wtk_os.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/bfio/resample/wtk_resample.h"

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
	wtk_strbuf_t **bufs;
	char *pv;

	ifn=argv[1];
	ofn=argv[2];

	riff=wtk_riff_new();
	wtk_riff_open(riff,ifn);
	rate=riff->fmt.sample_rate;
	samples=riff->fmt.bit_per_sample/8;
	channel=riff->fmt.channels;

	bufs=(wtk_strbuf_t**)wtk_malloc(channel*sizeof(wtk_strbuf_t*));
	for(i=0;i<channel;++i)
	{
		bufs[i]=wtk_strbuf_new(10240,1);
	}
	nx=channel*samples;
	wtk_debug("nx=%d\n",nx);
	pv=(char*)wtk_malloc(nx);
	while(1)
	{
		ret=wtk_riff_read(riff,(char*)pv,nx);
		if(ret!=nx)
		{
			break;
		}
		for(i=0;i<channel;++i)
		{
			wtk_strbuf_push(bufs[i],(char*)(pv+i*samples),samples);
		}
	}

	buf=wtk_strbuf_new(1024,1);
	resample=wtk_resample_new(1024);
	wtk_resample_start(resample,rate,8000);
	wtk_resample_set_notify(resample,buf,(wtk_resample_notify_f)wtk_strbuf_push);

	wtk_resample_feed(resample,bufs[0]->data,bufs[0]->pos,1);
	wave_write_file(ofn,8000,buf->data,buf->pos);


	for(i=0;i<channel;++i)
	{
		wtk_strbuf_delete(bufs[i]);
	}
	wtk_resample_delete(resample);
	wtk_strbuf_delete(buf);
	return 0;
}
