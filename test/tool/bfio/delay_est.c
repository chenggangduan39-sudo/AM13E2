#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_opt.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/bfio/ahs/estimate/qtk_delay_estimate.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/core/rbin/wtk_flist.h"


float *data_read(char *fn){

	FILE *fp;
	int len = 32850 * 2;
	float *data;
	fp = fopen(fn,"rb");
	data = (float*)malloc(len*sizeof(float));
	fread(data,sizeof(float),len,fp);
	fclose(fp);
	return data;
}

static void test_wav(qtk_delay_estimate_t * dst,char *wav){
    wtk_riff_t *riff;

    riff=wtk_riff_new();
    wtk_riff_open(riff,wav);
    short *pv;
    int channel=2;
    int len,bytes;
    int ret;

    len=32*16;
    bytes=sizeof(short)*channel*len;
    pv=(short *)wtk_malloc(sizeof(short)*channel*len);	

    while(1)
    {
        ret=wtk_riff_read(riff,(char *)pv,bytes);
        if(ret<=0)
        {
            wtk_debug("break ret=%d\n",ret);
            break;
        }
		// wtk_msleep(64);
        len=ret/(sizeof(short)*channel);
        qtk_delay_estimate_feed(dst,pv,len,0);
		// wtk_debug("deley %f\n",(cnt-out_len)/16.0);
    }
	qtk_delay_estimate(dst);
}

int main(int argc,char **argv)
{
	wtk_arg_t *arg;
	char *cfg_fn=NULL;
	wtk_main_cfg_t *main_cfg = 0;
	qtk_rir_estimate_cfg_t *cfg = 0;
	char *wav=NULL;
	
	arg=wtk_arg_new(argc,argv);
	wtk_arg_get_str_s(arg,"c",&cfg_fn);
	wtk_arg_get_str_s(arg,"i",&wav);	
	main_cfg = wtk_main_cfg_new_type(qtk_rir_estimate_cfg,cfg_fn);
	cfg = (qtk_rir_estimate_cfg_t*)main_cfg->cfg;

	qtk_delay_estimate_t * dst = qtk_delay_estimate_new(cfg);
	//qtk_delay_estimate_ref_load(dst,"ref.bin");
	if(!wav){
		float *xr = data_read("test.bin");
		qtk_delay_estimate_feed_float(dst, xr, 32850, 0);
		qtk_delay_estimate(dst);
	}else{
		test_wav(dst,wav);
	}


	qtk_delay_estimate_delete(dst);
	if(arg)
	{
		wtk_arg_delete(arg);
	}
	return  0;
}
