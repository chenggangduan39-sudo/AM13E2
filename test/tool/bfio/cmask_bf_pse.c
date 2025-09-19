#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_opt.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/bfio/maskdenoise/cmask_pse/wtk_cmask_bf_pse.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/core/rbin/wtk_flist.h"

void print_usage()
{
	printf("cmask_bf_pse usage:\n");
	printf("\t-c configure file\n");
	printf("\t-b bin configure file\n");
	printf("\t-i input wav  file\n");
	printf("\t-scp input scp file\n");
	printf("\t-vp voiceprint feature file\n");
	printf("\t-vpw voiceprint wav file\n");
	printf("\t-ovp output voiceprint feature file\n");
	printf("\t-o output dir\n");
	printf("\n\n");
}


static int out_len=0;

static void test_cmask_bf_pse_on(wtk_wavfile_t *wav,short *data,int len)
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

static void test_cmask_bf_pse_feat_on(FILE *fp,float *data,int len)
{
    if(len>0)
    {
        fwrite(data,sizeof(float),len,fp);
    }
}

static void cmask_bf_pse_get_feat(wtk_cmask_bf_pse_t *cmask_bf_pse,char* vp_feat,char* vp_wav,char* out_vp_feat)
{
	if(vp_feat){
		FILE *fp;
		float *feat;
		
		fp=fopen(vp_feat,"rb");
		if(fp==NULL)
		{
			wtk_debug("open %s failed\n",vp_feat);
			return;
		}
		feat=(float *)wtk_malloc(sizeof(float)*cmask_bf_pse->feat_len);
		fread(feat,sizeof(float),cmask_bf_pse->feat_len,fp);
		fclose(fp);
		wtk_cmask_bf_pse_start_vp_feat(cmask_bf_pse,feat,cmask_bf_pse->feat_len);
		wtk_free(feat);
	}else if(vp_wav){
    	wtk_riff_t *riff;
		FILE *fp_out=NULL;
		int len = 32;
		int bytes = len*sizeof(short);
		short *data;
		int ret;

		wtk_cmask_bf_pse_new_vp(cmask_bf_pse);

		if(out_vp_feat){
			fp_out=fopen(out_vp_feat,"wb");
			if(fp_out==NULL)
			{
				wtk_debug("open %s failed\n",out_vp_feat);
				return;
			}
			wtk_cmask_bf_pse_set_notify2(cmask_bf_pse,fp_out,(wtk_cmask_bf_pse_notify_f2)test_cmask_bf_pse_feat_on);
		}

		riff=wtk_riff_new();
		wtk_riff_open(riff,vp_wav);
		data = (short *)wtk_malloc(sizeof(short) * len);
		while (1)
		{
			ret=wtk_riff_read(riff,(char *)data,bytes);
			if(ret<=0)
			{
				break;
			}
			len=ret/sizeof(short);
			wtk_cmask_bf_pse_feed_vp(cmask_bf_pse,data,len,0);
		}
		wtk_cmask_bf_pse_feed_vp(cmask_bf_pse,NULL,0,1);
		wtk_cmask_bf_pse_delete_vp(cmask_bf_pse);
		
		if(fp_out){
			fclose(fp_out);
		}
		wtk_riff_delete(riff);
		wtk_free(data);
	}else{
		wtk_debug("vp_feat or vp_wav must be set\n");
		return;
	}
}

static void cmask_bf_pse_test_file(wtk_cmask_bf_pse_t *cmask_bf_pse,char* vp_feat,char* vp_wav,char* out_vp_feat,char *ifn,char *ofn)
{
    wtk_riff_t *riff;
    wtk_wavfile_t *wav;
    short *pv;
    int channel=cmask_bf_pse->cfg->channel;
    int len,bytes;
    int ret;
    double t;
    int cnt;
    // wtk_strbuf_t **input;
    // int n;
    // short *pv[10];
    // int i;

	cmask_bf_pse_get_feat(cmask_bf_pse,vp_feat,vp_wav,out_vp_feat);

    wav=wtk_wavfile_new(cmask_bf_pse->cfg->rate);
    wav->max_pend=0;
    wtk_wavfile_open(wav,ofn);

    riff=wtk_riff_new();
    wtk_riff_open(riff,ifn);

    len=32*16;
    bytes=sizeof(short)*channel*len;
    pv=(short *)wtk_malloc(sizeof(short)*channel*len);

	wtk_cmask_bf_pse_start(cmask_bf_pse);
    wtk_cmask_bf_pse_set_notify(cmask_bf_pse,wav,(wtk_cmask_bf_pse_notify_f)test_cmask_bf_pse_on);

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
		// wtk_msleep(64);
        len=ret/(sizeof(short)*channel);
        cnt+=len;
        wtk_cmask_bf_pse_feed(cmask_bf_pse,pv,len,0);
		// wtk_debug("deley %f\n",(cnt-out_len)/16.0);
    }
    wtk_cmask_bf_pse_feed(cmask_bf_pse,NULL,0,1);
    t=time_get_ms()-t;
    wtk_debug("rate=%f t=%f\n",t/(cnt/16.0),t);

    wtk_riff_delete(riff);
    wtk_wavfile_delete(wav);
    wtk_free(pv);
}


int main(int argc,char **argv)
{
	wtk_cmask_bf_pse_t *cmask_bf_pse=NULL;
	wtk_cmask_bf_pse_cfg_t *cfg=NULL;
	wtk_arg_t *arg;
	char *cfg_fn=NULL;
	char *bin_fn=NULL;
	char *scp=NULL;
	char *output=NULL;
	char *wav=NULL;
	char *vp_feat=NULL, *vp_wav=NULL, *out_vp_feat=NULL;

	arg=wtk_arg_new(argc,argv);
	if(!arg){goto end;}
	wtk_arg_get_str_s(arg,"c",&cfg_fn);
	wtk_arg_get_str_s(arg,"b",&bin_fn);
	wtk_arg_get_str_s(arg,"scp",&scp);
	wtk_arg_get_str_s(arg,"i",&wav);
	wtk_arg_get_str_s(arg,"vp",&vp_feat);
	wtk_arg_get_str_s(arg,"vpw",&vp_wav);
	wtk_arg_get_str_s(arg,"ovp",&out_vp_feat);
	wtk_arg_get_str_s(arg,"o",&output);
	if((!cfg_fn && !bin_fn)||(!scp && !wav))
	{
		print_usage();
		goto end;
	}
	if(cfg_fn)
	{
		cfg=wtk_cmask_bf_pse_cfg_new(cfg_fn);
		if(cfg==NULL)
		{
			print_usage();
			goto end;
		}
	}else if(bin_fn)
	{
		cfg=wtk_cmask_bf_pse_cfg_new_bin(bin_fn);
		if(cfg==NULL)
		{
			print_usage();
			goto end;
		}
	}

	cmask_bf_pse=wtk_cmask_bf_pse_new(cfg);
	if(wav)
	{
		cmask_bf_pse_test_file(cmask_bf_pse,vp_feat,vp_wav,out_vp_feat,wav,output);
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
				cmask_bf_pse_test_file(cmask_bf_pse,NULL,NULL,NULL,ifn,buf->data);
			}
			wtk_string_delete(v);
		}
		wtk_strbuf_delete(buf);
		wtk_flist_it_delete(it);
	}
end:
	if(cmask_bf_pse)
	{
		wtk_cmask_bf_pse_delete(cmask_bf_pse);
	}
	if(cfg)
	{
		if(cfg_fn)
		{
			wtk_cmask_bf_pse_cfg_delete(cfg);
		}else
		{
			wtk_cmask_bf_pse_cfg_delete_bin(cfg);
		}
	}
	if(arg)
	{
		wtk_arg_delete(arg);
	}
	return  0;
}
