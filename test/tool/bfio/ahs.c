#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_opt.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/bfio/ahs/qtk_ahs.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/core/rbin/wtk_flist.h"

void print_usage()
{
	printf("ahs usage:\n");
	printf("\t-c configure file\n");
	printf("\t-b bin configure file\n");
	printf("\t-i input wav  file\n");
	printf("\t-scp input scp file\n");
	printf("\t-o output dir\n");
	printf("\n\n");
}


static int out_len=0;

static void test_ahs_on(wtk_wavfile_t *wav,short *data,int len)
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

static void ahs_test_file(qtk_ahs_t *ahs,char *ifn,char *ofn)
{
    wtk_riff_t *riff;
    wtk_wavfile_t *wav;
    short *pv;
    int channel=1;
    int len,bytes;
    int ret;
    double t;
    int cnt;
    // wtk_strbuf_t **input;
    // int n;
    // short *pv[10];
    // int i;

    wav=wtk_wavfile_new(ahs->cfg->rate);
    wav->max_pend=0;
    wtk_wavfile_open(wav,ofn);

    riff=wtk_riff_new();
    wtk_riff_open(riff,ifn);

    len=32*16;
    bytes=sizeof(short)*channel*len;
    pv=(short *)wtk_malloc(sizeof(short)*channel*len);

	qtk_ahs_start(ahs);
    qtk_ahs_set_notify(ahs,wav,(qtk_ahs_notify_f)test_ahs_on);

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
        qtk_ahs_feed(ahs,pv,len,0);
		// wtk_debug("deley %f\n",(cnt-out_len)/16.0);
    }
    qtk_ahs_feed(ahs,NULL,0,1);
    // char c[32];


    // for(i=0;i<320;++i)
    // {
    //     input=wtk_riff_read_channel(ifn,&n);
    //     pv[0]=(short*)(input[0]->data+i*2);
    //     pv[1]=(short*)(input[1]->data);
    //     len=(input[0]->pos-i*2)/sizeof(short);

    //     wav=wtk_wavfile_new(16000);
    //     wav->max_pend=0;
    //     sprintf(c, "%s%d", ofn,i);
    //     wtk_wavfile_open(wav,c);
        
    //     qtk_ahs_set_notify(ahs,wav,(qtk_ahs_notify_f)test_ahs_on);
    //     qtk_ahs_feed(ahs,pv,len,1);
    //     qtk_ahs_reset(ahs);

    //     wtk_wavfile_delete(wav);
    // }


    t=time_get_ms()-t;
    wtk_debug("rate=%f t=%f\n",t/(cnt/16.0),t);

    wtk_riff_delete(riff);
    wtk_wavfile_delete(wav);
    wtk_free(pv);
}


int main(int argc,char **argv)
{
	qtk_ahs_t *ahs=NULL;
	qtk_ahs_cfg_t *cfg=NULL;
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
	if((!cfg_fn && !bin_fn)||(!scp && !wav))
	{
		print_usage();
		goto end;
	}
	if(cfg_fn)
	{
		cfg=qtk_ahs_cfg_new(cfg_fn);
		if(cfg==NULL)
		{
			print_usage();
			goto end;
		}
	}else if(bin_fn)
	{
		cfg=qtk_ahs_cfg_new_bin(bin_fn);
		if(cfg==NULL)
		{
			print_usage();
			goto end;
		}
	}

	ahs=qtk_ahs_new(cfg);
	if(wav)
	{
		int i;

		for(i=0;i<1;++i)
		{
			ahs_test_file(ahs,wav,output);
		}
//		ahs_test_file(ahs,wav,"2.wav");
//		ahs_test_file(ahs,wav,"3.wav");
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
				ahs_test_file(ahs,ifn,buf->data);
			}
			wtk_string_delete(v);
		}
		wtk_strbuf_delete(buf);
		wtk_flist_it_delete(it);
	}
end:
	if(ahs)
	{
		qtk_ahs_delete(ahs);
	}
	if(cfg)
	{
		if(cfg_fn)
		{
			qtk_ahs_cfg_delete(cfg);
		}else
		{
			qtk_ahs_cfg_delete_bin(cfg);
		}
	}
	if(arg)
	{
		wtk_arg_delete(arg);
	}
	return  0;
}
