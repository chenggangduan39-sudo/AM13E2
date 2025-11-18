#include "wtk/bfio/wtk_bfio3.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/json/wtk_json_parse.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_wavfile.h"

static int theta=0;
static int phi=0;
static     wtk_wavfile_t *out_wav;
static int wake_cnt=0;
static int out_len=0;

static void print_usage()
{
	printf("bfio3 usage:\n");
	printf("\t-c cfg\n");
	printf("\t-i input wav file\n");
	printf("\t-o output wav file\n");
	printf("\n");
}

static void test_bfio3_on(wtk_bfio3_t *bfio3,wtk_bfio3_cmd_t cmd,short *data,int len)
{
	static int ki=0;
	static wtk_wavfile_t *wlog = NULL;
        wtk_json_parser_t *jsp = NULL;
        wtk_json_item_t *res, *conf;
        char buf[512];

    switch(cmd)
    {
    case WTK_BFIO3_VAD_START:
		wtk_debug("vad start fs=%f \n",bfio3->vad_ts);
		++ki;
		wlog = wtk_wavfile_new(bfio3->cfg->rate);
		wlog->max_pend = 0;
		sprintf(buf, "vx/%d.wav", ki);
		wtk_debug("open vad=%s\n", buf);
		wtk_wavfile_open(wlog, buf);
        break;
    case WTK_BFIO3_VAD_DATA:
		if(len>0)
		{
			if(wlog)
			{
				wtk_wavfile_write(wlog,(char *)data,len*sizeof(short));
			}
			wtk_wavfile_write(out_wav,(char *)data,len*sizeof(short));
            out_len+=len;
		}
        break;
    case WTK_BFIO3_VAD_END:
		wtk_debug("vad end fe=%f \n",bfio3->vad_te);
		if(wlog)
		{
			wtk_wavfile_close(wlog);
			wtk_wavfile_delete(wlog);
			wlog = NULL;
		}
        break;
    case WTK_BFIO3_WAKE:
        printf("[wake] %f %f\n", bfio3->wake_fs, bfio3->wake_fe);
        ++wake_cnt;
        //// --ki;
        break;		
    case WTK_BFIO3_WAKE_SSL:
        printf("[ssl] theta=%d\n", bfio3->wake_theta);
        break;	
    case WTK_BFIO3_VAD_CANCEL:
        out_len-=len;
		wtk_wavfile_cancel(out_wav, len*sizeof(short));
        break;
    case WTK_BFIO3_ASR_RES:
        jsp = wtk_json_parser_new();
        wtk_json_parser_parse(jsp, (char *)data, len);
        conf = wtk_json_obj_get_s(jsp->json->main, "conf");
        res = wtk_json_obj_get_s(jsp->json->main, "rec");
        printf("[asr] %f %f %.*s/%f\n", bfio3->vad_ts, bfio3->vad_te,
               res->v.str->len, res->v.str->data, conf->v.number);
        break;
    case WTK_BFIO3_WAKE_RES:
        break;
    default:
        break;
    }
    if (jsp) {
        wtk_json_parser_delete(jsp);
    }
}

static void test_bfio3_file2(wtk_bfio3_t *bfio3,char *scp,char *odir)
{
    wtk_flist_it_t *it;
    wtk_riff_t *riff;
    wtk_wavfile_t *wav;
    short *pv;
    short **data;
    int channel=bfio3->channel+1;
    int len,bytes;
    int ret;
    int i,j;
    char *ifn=NULL,*ofn=NULL;
    wtk_strbuf_t *buf,*buf2;

    buf=wtk_strbuf_new(256,1);
    buf2=wtk_strbuf_new(256,1);

    wav=wtk_wavfile_new(bfio3->cfg->rate);
    wav->max_pend=0;
    riff=wtk_riff_new();

    len=20*16;
    bytes=sizeof(short)*channel*len;
    pv=(short *)wtk_malloc(sizeof(short)*channel*len);

    data=(short **)wtk_malloc(sizeof(short *)*channel);
    for(i=0;i<channel;++i)
    {
        data[i]=(short *)wtk_malloc(sizeof(short)*len);
    }
    wtk_bfio3_set_notify(bfio3,bfio3,(wtk_bfio3_notify_f)test_bfio3_on);
    wtk_bfio3_start(bfio3);

	it=wtk_flist_it_new(scp);
	while(1)
	{
		ifn=wtk_flist_it_next(it);
		if(!ifn)
		{ 
            break;
		}


        printf("%s\n",ifn);
        wtk_strbuf_reset(buf);
        wtk_strbuf_push(buf,ifn,strlen(ifn)+1);
        ofn=buf->data+(buf->pos-7);
        wtk_strbuf_reset(buf2);
        wtk_strbuf_push(buf2,odir,strlen(odir));
        wtk_strbuf_push(buf2,"/",1);
        wtk_strbuf_push(buf2,ofn,strlen(ofn)+1);
        ofn=buf2->data;
        printf("%s\n",ofn);

        wtk_wavfile_open(wav,ofn);
        wtk_riff_open(riff,ifn);

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
            wtk_bfio3_feed(bfio3,data,len,0);
        }

        wtk_riff_close(riff);
        wtk_wavfile_close(wav);
	}
    wtk_bfio3_feed(bfio3,NULL,0,1);

    wtk_strbuf_delete(buf);

    wtk_flist_it_delete(it);
    wtk_riff_delete(riff);
    wtk_wavfile_delete(wav);
    wtk_free(pv);
    for(i=0;i<channel;++i)
    {
        wtk_free(data[i]);
    }
    wtk_free(data);
}


static void test_bfio3_file(wtk_bfio3_t *bfio3,char *ifn,char *ofn)
{
    wtk_riff_t *riff;

    short *pv;
    short **data;
    int channel;
    int len,bytes;
    int ret;
    int i,j;
    double t;
    int cnt;

    out_wav=wtk_wavfile_new(bfio3->cfg->rate);
    out_wav->max_pend=0;
    wtk_wavfile_open(out_wav,ofn);

    riff=wtk_riff_new();
    wtk_riff_open(riff,ifn);

    channel = riff->fmt.channels;
    len=20*16;
    bytes=sizeof(short)*channel*len;
    pv=(short *)wtk_malloc(sizeof(short)*channel*len);

    data=(short **)wtk_malloc(sizeof(short *)*channel);
    for(i=0;i<channel;++i)
    {
        data[i]=(short *)wtk_malloc(sizeof(short)*len);
    }
    wtk_bfio3_set_notify(bfio3,bfio3,(wtk_bfio3_notify_f)test_bfio3_on);
    wtk_bfio3_start(bfio3);

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
        wtk_bfio3_feed(bfio3,data,len,0);
    }
    wtk_bfio3_feed(bfio3,NULL,0,1);


    t=time_get_ms()-t;
    wtk_debug("===>>> wake_cnt=%d rate=%f t=%f cnt=%d out=%d\n",wake_cnt,t/(cnt/16.0),t,cnt,out_len);

    wtk_riff_delete(riff);
    wtk_wavfile_delete(out_wav);
    wtk_free(pv);
    for(i=0;i<channel;++i)
    {
        wtk_free(data[i]);
    }
    wtk_free(data);
}


int main(int argc,char **argv)
{
    wtk_bfio3_cfg_t *cfg;
    wtk_bfio3_t *bfio3;
    wtk_arg_t *arg;
    char *cfg_fn,*ifn,*ofn,*bin_fn;
    char *scp=NULL;

    arg=wtk_arg_new(argc,argv);
    if(!arg){goto end;}
    wtk_arg_get_str_s(arg,"c",&cfg_fn);
    wtk_arg_get_str_s(arg,"b",&bin_fn);
    wtk_arg_get_str_s(arg,"i",&ifn);
    wtk_arg_get_str_s(arg,"o",&ofn);
    wtk_arg_get_int_s(arg,"theta",&theta);
    wtk_arg_get_int_s(arg,"phi",&phi);
    wtk_arg_get_str_s(arg,"scp",&scp);
    if((!cfg_fn && !bin_fn) || (!ifn&&!scp) || !ofn)
    {
        print_usage();
        goto end;
    }

    if(cfg_fn)
    {
        cfg=wtk_bfio3_cfg_new(cfg_fn);
    }
    if(bin_fn)
    {
        cfg=wtk_bfio3_cfg_new_bin(bin_fn);
    }
    wtk_bfio3_cfg_set_wakeword(cfg, "小爱同学");
    bfio3=wtk_bfio3_new(cfg);

    if(scp)
    {
        test_bfio3_file2(bfio3,scp,ofn);
    }else
    {
        test_bfio3_file(bfio3,ifn,ofn);
    }

    wtk_bfio3_delete(bfio3);
end:
    if(arg)
    {
        wtk_arg_delete(arg);
    }
    if(bin_fn)
    {
        wtk_bfio3_cfg_delete_bin(cfg);
    }
    if(cfg_fn)
    {
        wtk_bfio3_cfg_delete(cfg);
    }
    return 0;
}
