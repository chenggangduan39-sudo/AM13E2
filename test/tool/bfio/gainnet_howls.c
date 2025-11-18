#include "wtk/core/wtk_type.h"
#include "wtk/bfio/maskform/wtk_gainnet_howls.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/cfg/wtk_main_cfg.h"


static void print_usage()
{
	printf("qdenoise usage:\n");
	// printf("\t-c cfg\n");
	printf("\t-i input wav file\n");
    // printf("\t-scp input scp file\n");
	printf("\t-o output wav file\n");
    // printf("\t-tr train or test\n");
	printf("\n");
}

static void test_gainnet_howls_on(wtk_wavfile_t *wav,short *data, int len, int is_end)
{
    if(len>0)
    {
        wtk_wavfile_write(wav,(char *)data, len<<1);
    }
}


static void test_gainnet_howls_on_trfeat(FILE *f_out,float *feat,int len, float *feat_hm,int len2,float *target_g,int g_len)
{
    fwrite(feat, sizeof(float), len, f_out);
    fwrite(feat_hm, sizeof(float), len2, f_out);
    fwrite(target_g, sizeof(float), g_len, f_out);
}

static void test_gainnet_howls_get_trfeat(wtk_gainnet_howls_t *gainnet_howls,char *ifn,char *tr_fn,int enr)
{
    wtk_riff_t *riff;
    int len;
    int i;
    // double t;
    int cnt;
	short *pv[64];
	wtk_strbuf_t **input;
    int n;

    FILE *f_out;

    riff=wtk_riff_new();
    wtk_riff_open(riff,ifn);

    f_out = fopen(tr_fn, "wb");
    wtk_gainnet_howls_set_tr_notify(gainnet_howls,f_out,(wtk_gainnet_howls_notify_trfeat_f)test_gainnet_howls_on_trfeat);

    // t=time_get_ms();
    input=wtk_riff_read_channel(ifn,&n);
    for(i=0;i<n;++i)
    {
        pv[i]=(short*)(input[i]->data);
    }
    len=cnt=(input[0]->pos)/sizeof(short);
    // t=time_get_ms();

    wtk_gainnet_howls_feed_train(gainnet_howls, pv, len, n, enr);

    // t=time_get_ms()-t;
    // wtk_debug("rate=%f t=%f\n",t/(cnt/16.0),t);

    wtk_riff_delete(riff);
    wtk_strbufs_delete(input, n);
    fclose(f_out);
}


static void test_gainnet_howls_file(wtk_gainnet_howls_t *gainnet_howls,char *ifn,char *ofn)
{
    wtk_riff_t *riff;
    wtk_wavfile_t *wav;
    short *pv;
    int channel=gainnet_howls->cfg->channel;
    int len,bytes;
    int ret;
    double t;
    int cnt;

    wav=wtk_wavfile_new(gainnet_howls->cfg->rate);
    wav->max_pend=0;
    wtk_wavfile_open(wav,ofn);

    riff=wtk_riff_new();
    wtk_riff_open(riff,ifn);

    len=32*16;
    bytes=sizeof(short)*channel*len;
    pv=(short *)wtk_malloc(sizeof(short)*channel*len);

    wtk_gainnet_howls_start(gainnet_howls);
    wtk_gainnet_howls_set_notify(gainnet_howls,wav,(wtk_gainnet_howls_notify_f)test_gainnet_howls_on);

    // wtk_riff_read(riff,(char *)pv,56*channel);

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
        // wtk_smsleep(8);
        len=ret/(sizeof(short)*channel);

        cnt+=len;
        wtk_gainnet_howls_feed(gainnet_howls,pv,len,0);
    }
    wtk_gainnet_howls_feed(gainnet_howls,NULL,0,1);

    t=time_get_ms()-t;
    wtk_debug("rate=%f t=%f\n",t/(cnt/(gainnet_howls->cfg->rate/1000.0)),t);

    wtk_riff_delete(riff);
    wtk_wavfile_delete(wav);
    wtk_free(pv);
}


int main(int argc,char **argv)
{
    wtk_main_cfg_t *main_cfg=NULL;
    wtk_gainnet_howls_cfg_t *cfg;
    wtk_gainnet_howls_t *gainnet_howls;
    wtk_arg_t *arg;
    char *cfg_fn,*ifn,*ofn;
    int tr=0;
    char *scp_fn=NULL;
    wtk_flist_it_t *it;
    wtk_strbuf_t *buf;
    wtk_string_t *v;
    int enr=1;

    arg=wtk_arg_new(argc,argv);
    if(!arg){goto end;}
    wtk_arg_get_str_s(arg,"c",&cfg_fn);
    wtk_arg_get_str_s(arg,"i",&ifn);
    wtk_arg_get_str_s(arg,"o",&ofn);
    wtk_arg_get_int_s(arg,"tr",&tr);
    wtk_arg_get_str_s(arg,"scp",&scp_fn);
    wtk_arg_get_int_s(arg,"enr",&enr);
    if(!cfg_fn || (!tr&&(!ifn || !ofn)) ||  (tr&&!scp_fn&&!ifn))
    {
        print_usage();
        goto end;
    }

    main_cfg=wtk_main_cfg_new_type(wtk_gainnet_howls_cfg,cfg_fn);
    if(!main_cfg){goto end;}

    cfg=(wtk_gainnet_howls_cfg_t *)(main_cfg->cfg);


    // if(!ifn || !ofn)
    // {
    //     print_usage();
    //     goto end;
    // }

    // cfg=wtk_gainnet_howls_cfg_new_bin("./qdenoise.bin");

    gainnet_howls=wtk_gainnet_howls_new(cfg);
    if(tr)
    {
        if(scp_fn)
        {
            buf=wtk_strbuf_new(256,1);
            it=wtk_flist_it_new(scp_fn);
            while(1)
            {
                ifn=wtk_flist_it_next(it);
                if(!ifn){break;}
                wtk_strbuf_reset(buf);
                wtk_strbuf_push_string(buf, ofn);
                wtk_strbuf_push_s(buf, "/");
                v = wtk_basename(ifn, '/');				
                wtk_strbuf_push(buf, v->data, v->len);
                wtk_strbuf_push_c(buf, 0);
                test_gainnet_howls_get_trfeat(gainnet_howls,ifn,buf->data,1);
                wtk_gainnet_howls_reset(gainnet_howls);
                if(enr>=2)
                {
                    wtk_strbuf_reset(buf);
                    wtk_strbuf_push_string(buf, ofn);
                    wtk_strbuf_push_s(buf, "/");
                    wtk_strbuf_push(buf, v->data, v->len);
                    wtk_strbuf_push_s(buf, "2");
                    wtk_strbuf_push_c(buf, 0);
                    // printf("%s\n", buf->data);
                    test_gainnet_howls_get_trfeat(gainnet_howls,ifn,buf->data,2);
                    wtk_gainnet_howls_reset(gainnet_howls);
                }
                if(enr>=3)
                {
                    wtk_strbuf_reset(buf);
                    wtk_strbuf_push_string(buf, ofn);
                    wtk_strbuf_push_s(buf, "/");
                    wtk_strbuf_push(buf, v->data, v->len);
                    wtk_strbuf_push_s(buf, "3");
                    wtk_strbuf_push_c(buf, 0);
                    // printf("%s\n", buf->data);
                    test_gainnet_howls_get_trfeat(gainnet_howls,ifn,buf->data,3);
                    wtk_gainnet_howls_reset(gainnet_howls);
                }
                wtk_string_delete(v);
            }
            wtk_flist_it_delete(it);
            wtk_strbuf_delete(buf);
        }else
        {
            test_gainnet_howls_get_trfeat(gainnet_howls,ifn,ofn,1);
        }
    }else
    {
        test_gainnet_howls_file(gainnet_howls,ifn,ofn);
    }

    wtk_gainnet_howls_delete(gainnet_howls);
end:
    if(arg)
    {
        wtk_arg_delete(arg);
    }
    if(main_cfg)
    {
        wtk_main_cfg_delete(main_cfg);
    }
    return 0;
}
