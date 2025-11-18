#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_opt.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/bfio/vbox/wtk_vboxebf6.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/core/rbin/wtk_flist.h"

void print_usage()
{
	printf("vboxebf6 usage:\n");
	printf("\t-c configure file\n");
	printf("\t-b bin configure file\n");
	printf("\t-i input wav  file\n");
	printf("\t-scp input scp file\n");
	printf("\t-o output dir\n");
	printf("\n\n");
}


static int out_len=0;
static float eng_thresh=-3;
static int repeat=0;

static void test_vboxebf6_on(wtk_wavfile_t *wav,short *data,int len)
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

static void test_vboxebf6_on_ssl(void *ths,float ts, float te,wtk_ssl2_extp_t *nbest_extp,int nbest)
{
    int i;

	if(eng_thresh==-3){
		for(i=0;i<nbest;++i)
		{
			printf("[%f %f] ==> %d %d %f\n", ts, te, nbest_extp[i].theta, nbest_extp[i].phi, nbest_extp[i].nspecsum);
		}
		if(nbest==0)
		{
			printf("[%f %f] no speech\n", ts,te);
		}
	}else if(eng_thresh==-2){
		if(nbest>0){
			printf("%f\n", nbest_extp[0].nspecsum);
		}else{
			printf("0\n");
		}
	}else if(eng_thresh==-1){

	}else if(eng_thresh==0){
		if(nbest>0){
			printf("%d\n", nbest_extp[0].theta);
		}else{
			printf("-90\n");
		}
	}else if(eng_thresh>0){
		if(nbest>0 && nbest_extp[0].nspecsum>eng_thresh){
			printf("%d\n", nbest_extp[0].theta);
		}else{
			printf("-90\n");
		}
	}
}

static void test_vboxebf6_on_eng(void *ths, float energy, float snr)
{
	// printf("%f %f\n", energy, snr);
	// printf("%f\n", energy);
	// printf("%f\n", snr);
}

static void vboxebf6_test_file(wtk_vboxebf6_t *vboxebf6,char *ifn,char *ofn)
{
    wtk_riff_t *riff;
    wtk_wavfile_t *wav;
    short *pv;
    int channel=vboxebf6->cfg->channel;
    int len,bytes;
    int ret;
    double t;
    int cnt;
    // wtk_strbuf_t **input;
    // int n;
    // short *pv[10];
    // int i;

    wav=wtk_wavfile_new(vboxebf6->cfg->bf.rate);
    wav->max_pend=0;
    wtk_wavfile_open(wav,ofn);

    riff=wtk_riff_new();
    wtk_riff_open(riff,ifn);

    len=32*16;
    bytes=sizeof(short)*channel*len;
    pv=(short *)wtk_malloc(sizeof(short)*channel*len);

    wtk_vboxebf6_set_notify(vboxebf6,wav,(wtk_vboxebf6_notify_f)test_vboxebf6_on);
    wtk_vboxebf6_set_ssl_notify(vboxebf6,wav,(wtk_vboxebf6_notify_ssl_f)test_vboxebf6_on_ssl);
    wtk_vboxebf6_set_eng_notify(vboxebf6,NULL,(wtk_vboxebf6_notify_eng_f)test_vboxebf6_on_eng);
	wtk_vboxebf6_start(vboxebf6);
    // wtk_riff_read(riff,(char *)pv,56*channel);

	wtk_vboxebf6_set_agcenable(vboxebf6, 1);
	wtk_vboxebf6_set_denoiseenable(vboxebf6, 1);

    cnt=0;
    t=time_get_ms();
	if(repeat>0){
		while(repeat--){
    		wtk_riff_open(riff,ifn);
			while(1)
			{
				ret=wtk_riff_read(riff,(char *)pv,bytes);
				if(ret<=0)
				{
					// wtk_debug("break ret=%d\n",ret);
					break;
				}
				// wtk_msleep(64);
				len=ret/(sizeof(short)*channel);
				cnt+=len;
				wtk_vboxebf6_feed(vboxebf6,pv,len,0);
				// wtk_debug("deley %f\n",(cnt-out_len)/16.0);
			}
		}
	}else{
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
			cnt+=len;
			wtk_vboxebf6_feed(vboxebf6,pv,len,0);
			// wtk_debug("deley %f\n",(cnt-out_len)/16.0);
		}
	}
	wtk_vboxebf6_feed(vboxebf6,NULL,0,1);

    t=time_get_ms()-t;
    wtk_debug("rate=%f t=%f\n",t/(cnt/vboxebf6->cfg->rate*1000),t);

    wtk_riff_delete(riff);
    wtk_wavfile_delete(wav);
    wtk_free(pv);
}


int main(int argc,char **argv)
{
	wtk_vboxebf6_t *vboxebf6=NULL;
	wtk_vboxebf6_cfg_t *cfg=NULL;
	wtk_arg_t *arg;
	char *cfg_fn=NULL;
	char *bin_fn=NULL;
	char *scp=NULL;
	char *output=NULL;
	char *wav=NULL;

	arg=wtk_arg_new(argc,argv);
	if(!arg){goto end;}
	wtk_arg_get_str_s(arg,"c",&cfg_fn);
	wtk_arg_get_str_s(arg,"b",&bin_fn);
	wtk_arg_get_str_s(arg,"scp",&scp);
	wtk_arg_get_str_s(arg,"i",&wav);
	wtk_arg_get_str_s(arg,"o",&output);
wtk_arg_get_float_s(arg,"e",&eng_thresh);
	wtk_arg_get_int_s(arg,"r",&repeat);
	if((!cfg_fn && !bin_fn)||(!scp && !wav))
	{
		print_usage();
		goto end;
	}
	if(cfg_fn)
	{
		cfg=wtk_vboxebf6_cfg_new(cfg_fn);
		if(cfg==NULL)
		{
			print_usage();
			goto end;
		}
	}else if(bin_fn)
	{
		cfg=wtk_vboxebf6_cfg_new_bin(bin_fn);
		if(cfg==NULL)
		{
			print_usage();
			goto end;
		}
	}

	vboxebf6=wtk_vboxebf6_new(cfg);
	if(wav)
	{
		int i;

		for(i=0;i<1;++i)
		{
			vboxebf6_test_file(vboxebf6,wav,output);
		}
//		vboxebf6_test_file(vboxebf6,wav,"2.wav");
//		vboxebf6_test_file(vboxebf6,wav,"3.wav");
	}
	if(scp)
	{
		wtk_flist_it_t *it;
		char *ifn;
		wtk_strbuf_t *buf;
		wtk_string_t *v;

		buf=wtk_strbuf_new(1024,1);
		it=wtk_flist_it_new(scp);
		while(1)
		{
			ifn=wtk_flist_it_next(it);
			if(!ifn){break;}
			wtk_strbuf_reset(buf);
			wtk_strbuf_push_string(buf,output);
			wtk_strbuf_push_s(buf,"/");
			v=wtk_basename(ifn,'/');
			wtk_strbuf_push(buf,v->data,v->len);
			wtk_strbuf_push_c(buf,0);
			wtk_debug("fn= %s\n",ifn);
			//if(wtk_file_exist(buf->data)!=0)
			{
				vboxebf6_test_file(vboxebf6,ifn,buf->data);
			}
			wtk_string_delete(v);
		}
		wtk_strbuf_delete(buf);
		wtk_flist_it_delete(it);
	}
end:
	if(vboxebf6)
	{
		wtk_vboxebf6_delete(vboxebf6);
	}
	if(cfg)
	{
		if(cfg_fn)
		{
			wtk_vboxebf6_cfg_delete(cfg);
		}else
		{
			wtk_vboxebf6_cfg_delete_bin(cfg);
		}
	}
	if(arg)
	{
		wtk_arg_delete(arg);
	}
	return  0;
}
