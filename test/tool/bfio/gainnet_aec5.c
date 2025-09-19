#include "wtk/core/wtk_type.h"
#include "wtk/bfio/aec/wtk_gainnet_aec5.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/cfg/wtk_main_cfg.h"


static void print_usage()
{
	printf("gainnet_aec5 usage:\n");
	printf("\t-c cfg\n");
	printf("\t-i input wav file\n");
    printf("\t-scp input scp file\n");
	printf("\t-o output wav file\n");
    printf("\t-tr train or test\n");
	printf("\n");
}
static int stabe=1;
static int nchannel=2;

static void test_gainnet_aec5_on(wtk_wavfile_t *wav,short **data, int len, int is_end)
{
    if(len>0)
    {
        wtk_wavfile_write_mc(wav,data, len);
        // wtk_wavfile_write(wav,(char *)data[0], len<<1);
    }
}

static void test_gainnet_aec5_on_trfeat(FILE *f_out,float *feat,int len,float *feat2,int len2,float *target_agc,float *target_g,float *target_ge,int g_len)
{
    fwrite(feat, sizeof(float), len, f_out);
    fwrite(feat2, sizeof(float), len2, f_out);
    // fwrite(target_agc, sizeof(float), g_len, f_out);
    fwrite(target_g, sizeof(float), g_len, f_out);
    fwrite(target_ge, sizeof(float), g_len, f_out);
    // int i;
    // for(i=0;i<len;++i)
    // {
    //     printf("%f  ",feat[i]);
    // }
    // printf("\n");
}

static void test_gainnet_aec5_get_trfeat(wtk_gainnet_aec5_t *gainnet_aec5,char *ifn,char *tr_fn, int enr)
{
    wtk_riff_t *riff;
    int len;
    int i;
    // double t;
    int cnt;
	short *pv[10];
	wtk_strbuf_t **input;
    int n;

    FILE *f_out;

    riff=wtk_riff_new();
    wtk_riff_open(riff,ifn);
    f_out = fopen(tr_fn, "wb");
    wtk_gainnet_aec5_set_notify_tr(gainnet_aec5,f_out,(wtk_gainnet_aec5_notify_trfeat_f)test_gainnet_aec5_on_trfeat);

    // t=time_get_ms();
    input=wtk_riff_read_channel(ifn,&n);
    for(i=0;i<n;++i)
    {
        pv[i]=(short*)(input[i]->data+56);
    }
    len=cnt=(input[0]->pos-56)/sizeof(short);

    wtk_gainnet_aec5_feed_train(gainnet_aec5, pv, len, n, enr, stabe);
    // t=time_get_ms()-t;
    // wtk_debug("rate=%f t=%f\n",t/(cnt/16.0),t);

    wtk_riff_delete(riff);
    wtk_strbufs_delete(input, n);
    fclose(f_out);
}

// static void test_gainnet_aec5_get_trfeat(wtk_gainnet_aec5_t *gainnet_aec5,char *ifn,char *tr_fn, int enr)
// {
//     wtk_riff_t *riff;
//     int len;
//     int i;
//     // double t;
//     int cnt;
// 	short *pv[10];
// 	wtk_strbuf_t **input;
//     int n;

//     FILE *f_out;

//     riff=wtk_riff_new();
//     wtk_riff_open(riff,ifn);
//     // f_out = fopen(tr_fn, "wb");
//     // wtk_gainnet_aec5_set_notify_tr(gainnet_aec5,f_out,(wtk_gainnet_aec5_notify_trfeat_f)test_gainnet_aec5_on_trfeat);

//     // t=time_get_ms();
//     input=wtk_riff_read_channel(ifn,&n);
//     // for(i=0;i<n;++i)
//     // {
//     //     pv[i]=(short*)(input[i]->data+56);
//     // }
//     len=cnt=(input[0]->pos)/sizeof(short);
//     // t=time_get_ms();
//     int pos=0;
//     int b=0;
//     wtk_strbuf_t *buf;
//     buf=wtk_strbuf_new(256,1);

//     while(pos<=len)
//     {
//         for(i=0;i<n;++i)
//         {
//             pv[i]=(short*)(input[i]->data+pos);
//         }
//         if(len-pos<102400)
//         {
//             break;
//         }
//         wtk_strbuf_reset(buf);
//         wtk_strbuf_push_string(buf, tr_fn);
//         wtk_strbuf_push_s(buf, wtk_itoa(b));
//         wtk_strbuf_push_c(buf, 0);
//         f_out = fopen(buf->data, "wb");
        
//         wtk_gainnet_aec5_set_notify_tr(gainnet_aec5,f_out,(wtk_gainnet_aec5_notify_trfeat_f)test_gainnet_aec5_on_trfeat);

//         wtk_gainnet_aec5_feed_train_tmp(gainnet_aec5, pv, 102400, n);//, enr, stabe);
//         fclose(f_out);
//         pos+=102400;
//         ++b;
//     }
//     // t=time_get_ms()-t;
//     // wtk_debug("rate=%f t=%f\n",t/(cnt/16.0),t);

//     wtk_riff_delete(riff);
//     wtk_strbufs_delete(input, n);
//     // fclose(f_out);
// }

static void test_gainnet_aec5_file(wtk_gainnet_aec5_t *gainnet_aec5,char *ifn,char *ofn)
{
    wtk_riff_t *riff;
    wtk_wavfile_t *wav;
    short *pv;
    short **data;
    int channel=nchannel;
    int len,bytes;
    int ret;
    int i,j;
    double t;
    int cnt;
    // wtk_strbuf_t **input;
    // int n;
    // short *pv[10];
    // int i;

    wav=wtk_wavfile_new(gainnet_aec5->cfg->rate);
    wav->max_pend=0;
    wtk_wavfile_open(wav,ofn);
    wtk_wavfile_set_channel(wav, gainnet_aec5->cfg->nmicchannel);

    riff=wtk_riff_new();
    wtk_riff_open(riff,ifn);

    len=20*16;
    bytes=sizeof(short)*channel*len;
    pv=(short *)wtk_malloc(sizeof(short)*channel*len);

    data=(short **)wtk_malloc(sizeof(short *)*channel);
    for(i=0;i<channel;++i)
    {
        data[i]=(short *)wtk_malloc(sizeof(short)*len);
    }

    wtk_gainnet_aec5_set_notify(gainnet_aec5,wav,(wtk_gainnet_aec5_notify_f)test_gainnet_aec5_on);

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
        len=ret/(sizeof(short)*channel);
        for(i=0;i<len;++i)
        {
            for(j=0;j<channel;++j)
            {
                data[j][i]=pv[i*channel+j];
            }
        }
        // if(cnt<3*16000)
        // {
        // for(i=0;i<len;++i)
        // {
        //     data[0][i]=data[2][i]+data[4][i];
        //     data[1][i]=0;
        // }
        // }  

        cnt+=len;
        wtk_gainnet_aec5_feed(gainnet_aec5,data,len,0);
    }
    wtk_gainnet_aec5_feed(gainnet_aec5,NULL,0,1);
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
        
    //     wtk_gainnet_aec5_set_notify(gainnet_aec5,wav,(wtk_gainnet_aec5_notify_f)test_gainnet_aec5_on);
    //     wtk_gainnet_aec5_feed(gainnet_aec5,pv,len,1);
    //     wtk_gainnet_aec5_reset(gainnet_aec5);

    //     wtk_wavfile_delete(wav);
    // }


    t=time_get_ms()-t;
    wtk_debug("rate=%f t=%f\n",t/(cnt/16.0),t);

    wtk_riff_delete(riff);
    wtk_wavfile_delete(wav);
    wtk_free(pv);
    for(i=0;i<channel;++i)
    {
        wtk_free(data[i]);
    }
    wtk_free(data);
}


static void test_gainnet_aec5_get_trfeat2(wtk_gainnet_aec5_t *gainnet_aec5,char *ifn, wtk_flist_it2_t *echo_it,  wtk_flist_it2_t *noise_it,char *ofn,int enr)
{
    // int len;
    // int i,k;
	// short *pv,*pv2;
	// wtk_strbuf_t **input;
    // wtk_strbuf_t **echo;
    // wtk_strbuf_t **noise;
    // wtk_strbuf_t **noise2;
    // int n;
    // FILE *f_out;
    // int nsrc;
    // int index;
    // time_t t;
    // char *ifn2;
    // wtk_strbuf_t *buf=wtk_strbuf_new(256,1);
    // wtk_string_t *v;

    // wtk_strbuf_reset(buf);
    // wtk_strbuf_push_string(buf, ofn);
    // wtk_strbuf_push_s(buf, "/");
    // v = wtk_basename(ifn, '/');				
    // wtk_strbuf_push(buf, v->data, v->len);
    // buf->pos-=4;
    // wtk_string_delete(v);

    // srand((unsigned) time(&t));

    // nsrc = rand()%5+1;

    // input=wtk_riff_read_channel(ifn,&n);

    // index = rand()%echo_it->index_q.length;
    // ifn2=wtk_flist_it2_index(echo_it, index);
    // echo=wtk_riff_read_channel(ifn2,&n);

    // v = wtk_basename(ifn2, '/');				
    // wtk_strbuf_push(buf, v->data, v->len);
    // buf->pos-=4;
    // wtk_string_delete(v);

    // index = rand()%noise_it->index_q.length;
    // ifn2=wtk_flist_it2_index(noise_it, index);
    // noise=wtk_riff_read_channel(ifn2,&n);
    // pv=(short*)(noise[0]->data);
    // len=(noise[0]->pos)/sizeof(short);

    // v = wtk_basename(ifn2, '/');		
    // wtk_strbuf_push(buf, v->data, v->len);
    // buf->pos-=4;
    // wtk_string_delete(v);

    // for(k=1;k<nsrc;++k)
    // {
    //     index = rand()%noise_it->index_q.length;
    //     ifn2=wtk_flist_it2_index(noise_it, index);
    //     noise2=wtk_riff_read_channel(ifn2,&n);
    //     pv2=(short*)(noise2[0]->data);
    //     for(i=0;i<len;i++)
    //     {
    //         pv[i]+=pv2[i];
    //     }
    //     wtk_strbufs_delete(noise2, n);
    // }

    // wtk_strbuf_push_s0(buf, ".wav");
    // f_out = fopen(buf->data, "wb");
    // wtk_gainnet_aec5_set_notify_tr(gainnet_aec5,f_out,(wtk_gainnet_aec5_notify_trfeat_f)test_gainnet_aec5_on_trfeat);
    // wtk_gainnet_aec5_feed_train3(gainnet_aec5, (short *)(input[0]->data),  (short *)(echo[0]->data), pv, (short *)(echo[1]->data),  (short *)(input[1]->data), len, n, 1, stabe);
    // wtk_gainnet_aec5_reset(gainnet_aec5);
    // fclose(f_out);

    // buf->pos-=1;
    // wtk_strbuf_push_s0(buf, "2");
    // f_out = fopen(buf->data, "wb");
    // wtk_gainnet_aec5_set_notify_tr(gainnet_aec5,f_out,(wtk_gainnet_aec5_notify_trfeat_f)test_gainnet_aec5_on_trfeat);
    // wtk_gainnet_aec5_feed_train3(gainnet_aec5, (short *)(input[0]->data),  (short *)(echo[0]->data), pv, (short *)(echo[1]->data),  (short *)(input[1]->data), len, n, 2, stabe);
    // wtk_gainnet_aec5_reset(gainnet_aec5);
    // fclose(f_out);

    // wtk_strbufs_delete(input, 3);
    // wtk_strbufs_delete(echo, 2);
    // wtk_strbufs_delete(noise, 1);
    // wtk_strbuf_delete(buf);
}


int main(int argc,char **argv)
{
    wtk_main_cfg_t *main_cfg=NULL;
    wtk_gainnet_aec5_cfg_t *cfg;
    wtk_gainnet_aec5_t *gainnet_aec5;
    wtk_arg_t *arg;
    char *cfg_fn,*ifn,*ofn;
    int tr=0;
    int tr2=0;
    char *scp_fn=NULL;
    char *scp_fn2=NULL;
    char *scp_fn3=NULL;
    wtk_flist_it_t *it;
    wtk_strbuf_t *buf;
    wtk_string_t *v;
    int enr=1;
    int echo=1;

    arg=wtk_arg_new(argc,argv);
    if(!arg){goto end;}
    wtk_arg_get_str_s(arg,"c",&cfg_fn);
    wtk_arg_get_str_s(arg,"i",&ifn);
    wtk_arg_get_str_s(arg,"o",&ofn);
    wtk_arg_get_int_s(arg,"tr",&tr);
    wtk_arg_get_int_s(arg,"tr2",&tr2);
    wtk_arg_get_int_s(arg,"nch",&nchannel);
    wtk_arg_get_str_s(arg,"scp",&scp_fn);
    wtk_arg_get_str_s(arg,"scp2",&scp_fn2);
    wtk_arg_get_str_s(arg,"scp3",&scp_fn3);
    wtk_arg_get_int_s(arg,"enr",&enr);
    wtk_arg_get_int_s(arg,"stabe",&stabe);
    wtk_arg_get_int_s(arg,"echo",&echo);
    if(!cfg_fn || (((!tr&&(!ifn || !ofn)) ||  (tr&&!scp_fn&&!ifn)) &&  (tr2&&!scp_fn)))
    {
        print_usage();
        goto end;
    }

    main_cfg=wtk_main_cfg_new_type(wtk_gainnet_aec5_cfg,cfg_fn);
    if(!main_cfg){goto end;}

    cfg=(wtk_gainnet_aec5_cfg_t *)(main_cfg->cfg);
    gainnet_aec5=wtk_gainnet_aec5_new(cfg);
	gainnet_aec5->train_echo=echo;

    if(tr && scp_fn)
    {
        buf=wtk_strbuf_new(256,1);
        it=wtk_flist_it_new(scp_fn);
        while(1)
        {
            ifn=wtk_flist_it_next(it);
            if(!ifn){break;}
            v = wtk_basename(ifn, '/');				
            wtk_strbuf_reset(buf);
            wtk_strbuf_push_string(buf, ofn);
            wtk_strbuf_push_s(buf, "/");
            wtk_strbuf_push(buf, v->data, v->len);
            wtk_strbuf_push_c(buf, 0);
            test_gainnet_aec5_get_trfeat(gainnet_aec5,ifn,buf->data,1);
            wtk_gainnet_aec5_reset(gainnet_aec5);
            if(enr>=2)
            {
                wtk_strbuf_reset(buf);
                wtk_strbuf_push_string(buf, ofn);
                wtk_strbuf_push_s(buf, "/");
                wtk_strbuf_push(buf, v->data, v->len);
                wtk_strbuf_push_s(buf, "2");
                wtk_strbuf_push_c(buf, 0);
                // printf("%s\n", buf->data);
                test_gainnet_aec5_get_trfeat(gainnet_aec5,ifn,buf->data,2);
                wtk_gainnet_aec5_reset(gainnet_aec5);
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
                test_gainnet_aec5_get_trfeat(gainnet_aec5,ifn,buf->data,3);
                wtk_gainnet_aec5_reset(gainnet_aec5);
            }
            if(enr>=4)
            {
                wtk_strbuf_reset(buf);
                wtk_strbuf_push_string(buf, ofn);
                wtk_strbuf_push_s(buf, "/");
                wtk_strbuf_push(buf, v->data, v->len);
                wtk_strbuf_push_s(buf, "4");
                wtk_strbuf_push_c(buf, 0);
                // printf("%s\n", buf->data);
                test_gainnet_aec5_get_trfeat(gainnet_aec5,ifn,buf->data,4);
                wtk_gainnet_aec5_reset(gainnet_aec5);
            }
            if(enr>=5)
            {
                wtk_strbuf_reset(buf);
                wtk_strbuf_push_string(buf, ofn);
                wtk_strbuf_push_s(buf, "/");
                wtk_strbuf_push(buf, v->data, v->len);
                wtk_strbuf_push_s(buf, "5");
                wtk_strbuf_push_c(buf, 0);
                // printf("%s\n", buf->data);
                test_gainnet_aec5_get_trfeat(gainnet_aec5,ifn,buf->data,5);
                wtk_gainnet_aec5_reset(gainnet_aec5);
            }
            if(enr>=6)
            {
                wtk_strbuf_reset(buf);
                wtk_strbuf_push_string(buf, ofn);
                wtk_strbuf_push_s(buf, "/");
                wtk_strbuf_push(buf, v->data, v->len);
                wtk_strbuf_push_s(buf, "6");
                wtk_strbuf_push_c(buf, 0);
                // printf("%s\n", buf->data);
                test_gainnet_aec5_get_trfeat(gainnet_aec5,ifn,buf->data,6);
                wtk_gainnet_aec5_reset(gainnet_aec5);
            }
            if(enr>=7)
            {
                wtk_strbuf_reset(buf);
                wtk_strbuf_push_string(buf, ofn);
                wtk_strbuf_push_s(buf, "/");
                wtk_strbuf_push(buf, v->data, v->len);
                wtk_strbuf_push_s(buf, "7");
                wtk_strbuf_push_c(buf, 0);
                // printf("%s\n", buf->data);
                test_gainnet_aec5_get_trfeat(gainnet_aec5,ifn,buf->data,7);
                wtk_gainnet_aec5_reset(gainnet_aec5);
            }
            if(enr>=8)
            {
                wtk_strbuf_reset(buf);
                wtk_strbuf_push_string(buf, ofn);
                wtk_strbuf_push_s(buf, "/");
                wtk_strbuf_push(buf, v->data, v->len);
                wtk_strbuf_push_s(buf, "8");
                wtk_strbuf_push_c(buf, 0);
                // printf("%s\n", buf->data);
                test_gainnet_aec5_get_trfeat(gainnet_aec5,ifn,buf->data,8);
                wtk_gainnet_aec5_reset(gainnet_aec5);
            }
            if(enr>=9)
            {
                wtk_strbuf_reset(buf);
                wtk_strbuf_push_string(buf, ofn);
                wtk_strbuf_push_s(buf, "/");
                wtk_strbuf_push(buf, v->data, v->len);
                wtk_strbuf_push_s(buf, "9");
                wtk_strbuf_push_c(buf, 0);
                // printf("%s\n", buf->data);
                test_gainnet_aec5_get_trfeat(gainnet_aec5,ifn,buf->data,9);
                wtk_gainnet_aec5_reset(gainnet_aec5);
            }
            if(enr>=10)
            {
                wtk_strbuf_reset(buf);
                wtk_strbuf_push_string(buf, ofn);
                wtk_strbuf_push_s(buf, "/");
                wtk_strbuf_push(buf, v->data, v->len);
                wtk_strbuf_push_s(buf, "10");
                wtk_strbuf_push_c(buf, 0);
                // printf("%s\n", buf->data);
                test_gainnet_aec5_get_trfeat(gainnet_aec5,ifn,buf->data,10);
                wtk_gainnet_aec5_reset(gainnet_aec5);
            }
            if(enr>=11)
            {
                wtk_strbuf_reset(buf);
                wtk_strbuf_push_string(buf, ofn);
                wtk_strbuf_push_s(buf, "/");
                wtk_strbuf_push(buf, v->data, v->len);
                wtk_strbuf_push_s(buf, "11");
                wtk_strbuf_push_c(buf, 0);
                // printf("%s\n", buf->data);
                test_gainnet_aec5_get_trfeat(gainnet_aec5,ifn,buf->data,11);
                wtk_gainnet_aec5_reset(gainnet_aec5);
            }
            if(enr>=12)
            {
                wtk_strbuf_reset(buf);
                wtk_strbuf_push_string(buf, ofn);
                wtk_strbuf_push_s(buf, "/");
                wtk_strbuf_push(buf, v->data, v->len);
                wtk_strbuf_push_s(buf, "12");
                wtk_strbuf_push_c(buf, 0);
                // printf("%s\n", buf->data);
                test_gainnet_aec5_get_trfeat(gainnet_aec5,ifn,buf->data,12);
                wtk_gainnet_aec5_reset(gainnet_aec5);
            }
            if(enr>=13)
            {
                wtk_strbuf_reset(buf);
                wtk_strbuf_push_string(buf, ofn);
                wtk_strbuf_push_s(buf, "/");
                wtk_strbuf_push(buf, v->data, v->len);
                wtk_strbuf_push_s(buf, "13");
                wtk_strbuf_push_c(buf, 0);
                // printf("%s\n", buf->data);
                test_gainnet_aec5_get_trfeat(gainnet_aec5,ifn,buf->data,13);
                wtk_gainnet_aec5_reset(gainnet_aec5);
            }

            wtk_string_delete(v);
        }
        wtk_flist_it_delete(it);
        wtk_strbuf_delete(buf);
    }else if(tr && ifn)
    {
        test_gainnet_aec5_get_trfeat(gainnet_aec5,ifn,ofn,enr);
    }else if(tr2)
    {
            int i;
            wtk_flist_it2_t *it2, *it3, *it4;
            it2=wtk_flist_it2_new(scp_fn);
            it3=wtk_flist_it2_new(scp_fn2);
            it4=wtk_flist_it2_new(scp_fn3);
            for(i=0;i<it2->index_q.length;++i)
            {
                ifn=wtk_flist_it2_index(it2,i);
                if(!ifn){break;}
                test_gainnet_aec5_get_trfeat2(gainnet_aec5,ifn,it3,it4,ofn,enr);
            }
            wtk_flist_it2_delete(it2);
            wtk_flist_it2_delete(it3);
            wtk_flist_it2_delete(it4);
    }else
    {
        test_gainnet_aec5_file(gainnet_aec5,ifn,ofn);
    }

    wtk_gainnet_aec5_delete(gainnet_aec5);
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
