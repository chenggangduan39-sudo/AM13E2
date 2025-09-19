#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_opt.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/bfio/compress/qtk_signal_compress.h"
#include "wtk/bfio/compress/qtk_signal_decompress.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/core/rbin/wtk_flist.h"

static int out_len=0;
static void test_on(wtk_wavfile_t *wav,short *data,int len)
{
    int i;

    out_len+=len;
    // printf("%d\n",len);
    if(len>0)
    {
        for(i=0; i<len; ++i)
        {   
            data[i]*=1;
        }
        wtk_wavfile_write(wav,(char *)data,len*sizeof(short));
    }
}



void test_compress(qtk_signal_compress_t *cp,char *wav_fn,qtk_signal_decompress_t *dp,char *out_fn){
	wtk_riff_t *riff;
    int ret;
    double t;
    short *pv;
    int channel=1;
    int len,bytes,cnt=0;
    riff=wtk_riff_new();
    wtk_riff_open(riff,wav_fn);
    len=32*16;
    bytes=sizeof(short)*channel*len;
    pv=(short *)wtk_malloc(sizeof(short)*channel*len);
    t=time_get_ms();

    wtk_wavfile_t *wav;
    wav=wtk_wavfile_new(8000);
    wtk_wavfile_open(wav,out_fn);
    qtk_signal_decompress_set_notify(dp,wav,(qtk_signal_decompress_notify_f)test_on);

    while(1)
    {
        ret=wtk_riff_read(riff,(char *)pv,bytes);
        if(ret<=0)
        {   
            //wtk_debug("break ret=%d\n",ret);
            break;
        }
        // wtk_msleep(64);
        len=ret/(sizeof(short)*channel);
        cnt+=len;
        qtk_signal_compress_feed(cp,pv,len);
        // wtk_debug("deley %f\n",(cnt-out_len)/16.0);
    }
    int64_t *codec_len;
    int codec_cnt;
    int64_t* res = qtk_signal_compress_get_result(cp,&codec_len,&codec_cnt);
    t=time_get_ms()-t;
    wtk_debug("rate=%f t=%f\n",t/(cnt/8.0),t);
    
    t = time_get_ms();
    wtk_debug("codec len=%d/%d/%d/%d\n",codec_len[0],codec_len[1],codec_len[2],codec_len[3]);
    qtk_signal_decompress_feed(dp,res,codec_len);
    qtk_signal_decompress_get_result(dp);

    t=time_get_ms()-t;
    wtk_debug("rate=%f t=%f\n",t/(cnt/8.0),t);
    wtk_wavfile_delete(wav);
    wtk_riff_delete(riff);
    wtk_free(pv);
}


int main(int argc,char **argv)
{
	wtk_arg_t *arg;
	char *cfg_fn1=NULL;
    char *cfg_fn2=NULL;
	char *wav_fn=NULL;
	char *out_fn=NULL;
    int bin=0;

	qtk_signal_compress_cfg_t *cfg=NULL;
	qtk_signal_decompress_cfg_t *cfg2=NULL;
	qtk_signal_compress_t *cp;
	qtk_signal_decompress_t *dp;
	arg=wtk_arg_new(argc,argv);
    wtk_arg_get_str_s(arg,"c1",&cfg_fn1);
    wtk_arg_get_str_s(arg,"c2",&cfg_fn2);
    wtk_arg_get_str_s(arg,"i",&wav_fn);
    wtk_arg_get_str_s(arg,"o",&out_fn);
    wtk_arg_get_int_s(arg,"b",&bin);

    if(!bin){
	    cfg = qtk_signal_compress_cfg_new(cfg_fn1);
	    cfg2 = qtk_signal_decompress_cfg_new(cfg_fn2);
    }else{
        cfg = qtk_signal_compress_cfg_new_bin(cfg_fn1);
        cfg2 = qtk_signal_decompress_cfg_new_bin(cfg_fn2);
    }

	cp=qtk_signal_compress_new(cfg);
	dp=qtk_signal_decompress_new(cfg2);

	if(wav_fn){
		test_compress(cp,wav_fn,dp,out_fn);
	}

    qtk_signal_compress_delete(cp);
    qtk_signal_decompress_delete(dp);

    if(!bin){
        qtk_signal_compress_cfg_delete(cfg);
        qtk_signal_decompress_cfg_delete(cfg2);
    }else{
        qtk_signal_compress_cfg_delete_bin(cfg);
        qtk_signal_decompress_cfg_delete_bin(cfg2);
    }
	if(arg)
	{
		wtk_arg_delete(arg);
	}
	return  0;
}
