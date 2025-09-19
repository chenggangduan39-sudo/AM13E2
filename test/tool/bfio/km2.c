#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_opt.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/bfio/ahs/qtk_kalman2.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/core/rbin/wtk_flist.h"

void print_usage()
{
	printf("km2 usage:\n");
	printf("\t-c configure file\n");
	printf("\t-i1 mic wav file\n");
	printf("\t-i1 ref wav file\n");
	printf("\t-o output wav file\n");
	printf("\n\n");
}


static void test_file(qtk_ahs_kalman2_t *km,char *ifn2, char *ifn1, char *ofn)
{
    wtk_riff_t *riff;
	wtk_riff_t *riff2;
    wtk_wavfile_t *wav;
    short *pv,*pv2;
    int channel=1;
    int len,bytes;
    int ret;
    double t;
    int cnt;

    wav = wtk_wavfile_new(16000);
    wav->max_pend=0;
    wtk_wavfile_open(wav,"ofn");

    riff = wtk_riff_new();
    wtk_riff_open(riff,ifn1);
    riff2 = wtk_riff_new();
    wtk_riff_open(riff2,ifn2);

    len = 128;
    bytes=sizeof(short)*channel*len;
    pv=(short *)wtk_malloc(sizeof(short)*channel*len);
    pv2=(short *)wtk_malloc(sizeof(short)*channel*len);
	char tmp[512 * sizeof(short)];
	wtk_strbuf_t *mic_buf = wtk_strbuf_new(1024,1);
	wtk_strbuf_push_nc(mic_buf, 0, 128 * sizeof(float));
	int i;
	float ref[128];
	short *ow;
	ow = wtk_malloc(128 * sizeof(short));
	memset(ow,0,sizeof(short) * 128);
    cnt=0;
    t=time_get_ms();
	FILE *fp = fopen(ofn,"wb");
	//ret=wtk_riff_read(riff,tmp,sizeof(short) * 512);
	// if(ret <= 0){
	// 	exit(0);
	// }

    while(1)
    {
        ret=wtk_riff_read(riff,(char *)pv,bytes);
		if(ret<=0)
        {
            wtk_debug("break ret=%d\n",ret);
            break;
        }
		for(i = 0;i < len;++i){
			ref[i] = pv[i]/32768.0f;
		}
		wtk_strbuf_push_float(mic_buf, ref, 128);
		ret=wtk_riff_read(riff2,(char *)pv2,bytes);
        if(ret<=0)
        {
            wtk_debug("break ret=%d\n",ret);
            break;
        }
		for(i = 0;i < len;++i){
			ref[i] = pv2[i]/32768.0f;
		}
		//wtk_debug("%d\n",mic_buf->pos/sizeof(float));
		if(mic_buf->pos/sizeof(float) >= 256){
			qtk_kalman2_feed(km, (float*)mic_buf->data, ref);
			qtk_kalman2_update(km);
			wtk_strbuf_pop(mic_buf, NULL, 128 * sizeof(float));
			for(i = 0;i < len;++i){
				ow[i] = km->e[i] * 32768.0;
			}
			fwrite(ow,sizeof(short),128,fp);
			fflush(fp);
			//wtk_wavfile_write(wav, (char *)ow,sizeof(short) * 128);
		}
		// wtk_msleep(64);
        len=ret/(sizeof(short)*channel);
        cnt+=len;
		// wtk_debug("deley %f\n",(cnt-out_len)/16.0);
    }
	fclose(fp);
    t=time_get_ms()-t;
    wtk_debug("rate=%f t=%f\n",t/(cnt/16.0),t);

    wtk_riff_delete(riff);
	wtk_riff_delete(riff2);
    wtk_wavfile_delete(wav);
	wtk_strbuf_delete(mic_buf);
    wtk_free(pv);
}


int main(int argc,char **argv)
{
	wtk_main_cfg_t *main_cfg = 0;
	qtk_ahs_kalman2_t *km = NULL;
	qtk_ahs_kalman2_cfg_t *cfg = NULL;
	wtk_arg_t *arg;
	char *cfg_fn = NULL;
	char *output = NULL;
	char *wav1 = NULL;
	char *wav2 = NULL;

	arg=wtk_arg_new(argc,argv);
	if(!arg){goto end;}
	wtk_arg_get_str_s(arg,"c",&cfg_fn);
	wtk_arg_get_str_s(arg,"i1",&wav1);
	wtk_arg_get_str_s(arg,"i2",&wav2);
	wtk_arg_get_str_s(arg,"o",&output);
	if((!cfg_fn)||(!wav1))
	{
		print_usage();
		goto end;
	}
	if(cfg_fn)
	{
		main_cfg = wtk_main_cfg_new_type(qtk_ahs_kalman2_cfg,cfg_fn);
		if(!main_cfg)
		{
			print_usage();
			goto end;
		}
		cfg = (qtk_ahs_kalman2_cfg_t *)(main_cfg->cfg);
	}
	km = qtk_kalman2_new(cfg);
	if(wav1 && wav2)
	{
		test_file(km, wav1, wav2, output);
	}

end:
	if(km)
	{
		qtk_kalman2_delete(km);
	}
	if(cfg)
	{
		if(main_cfg)
		{
			wtk_main_cfg_delete(main_cfg);
		}
	}
	if(arg)
	{
		wtk_arg_delete(arg);
	}
	return  0;
}
