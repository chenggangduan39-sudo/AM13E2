#include "qbl/sci/clustering/qbl_clustering_spectral.h"
#include "wtk/asr/kws/qtk_svprintc.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/wtk_arg.h"

void test_on_notify(wtk_strbuf_t *buf,float *data,int len)
{
    // int i = 0;
    // for(i = 0; i < len; ++i){
    //     printf("%f,\n",data[i]);
    // }
    // wtk_debug("%d \n",len);
    wtk_strbuf_push(buf,(char*)data,sizeof(float)*len);
    
    return;
}

int test(int argc,char **argv);


int test(int argc,char **argv)
{
    char *reg[] = {
        // "1660207577361/1.pcm",
        // "1660207577361/2.pcm",
        // "1660207577361/3.pcm",
        // "1660207577361/4.pcm",
        // "1660207577361/5.pcm",
        // "1660207577361/6.pcm",
        // "1660207577361/7.pcm",
        // "1660207577361/8.pcm",
        // "1660207577361/9.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/1.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/2.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/3.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/4.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/5.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/6.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/7.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/8.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/9.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/10.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/11.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/12.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/13.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/14.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/15.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/16.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/17.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/18.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/19.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/20.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/21.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/22.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/23.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/24.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/25.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/26.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/27.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/28.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/29.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/30.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/25.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/26.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/27.pcm",
        "/home/shensy/work/asr-kaldi-decode-old/1660291701536/28.pcm",
        "",
    };
    wtk_main_cfg_t *main_cfg = NULL;
    qtk_svprintc_cfg_t *cfg = NULL;
    wtk_arg_t *arg = NULL;
    qtk_svprintc_t* svprntc = NULL;
    char *cfn = NULL;
    //new
    arg = wtk_arg_new(argc,argv);
    wtk_arg_get_str_s(arg,"c",&cfn);
    if(cfn == NULL){
        printf("-c cfg\n");
        return -1;
    }
    main_cfg = wtk_main_cfg_new_type(qtk_svprintc_cfg,cfn);
    cfg = (qtk_svprintc_cfg_t*)main_cfg->cfg;

    wtk_strbuf_t *buf = wtk_strbuf_new(1024,1.0f);
    svprntc = qtk_svprintc_new(cfg);
    qbl_clustering_spectral_t* cs = qbl_clustering_spectral_new(-1,128,2);
    qtk_svprintc_set_notify(svprntc,buf,(qtk_svprintc_notify_f*)test_on_notify);
    //reg
    int i = 0;
    float *reg_c = NULL;
    for(i = 0; strlen(reg[i])>0;++i){
        qtk_svprintc_reset(svprntc);
        wtk_strbuf_reset(buf);
        FILE *f1 = fopen(reg[i],"rb");
        if(f1 == NULL){
            goto end;
        }
        fseek(f1,0,SEEK_END);
        int l = ftell(f1);
        fseek(f1,0,SEEK_SET);
        char *data = wtk_malloc(l);
        fread(data,1,l,f1);
        fclose(f1);
        qtk_svprintc_start(svprntc);
        qtk_svprintc_feed2(svprntc,(short*)(data),(l)/2,1);
        if(reg_c == NULL){
            reg_c = (float*)wtk_malloc(buf->pos);
            memset(reg_c,0,buf->pos);
        }
        // test2_name_add(reg_c,i,(float*)buf->data);
        qbl_clustering_spectral_add(cs,(float*)buf->data);
        wtk_free(data);
    }

    int *c = qbl_clustering_spectral_get_result(cs);
    for(i = 0;i < cs->N; ++i){
        printf("[%d]=%d\n",i+1,c[i]);
    }
end:
    //delete
    qbl_clustering_spectral_reset(cs);
    qbl_clustering_spectral_delete(cs);
    qtk_svprintc_delete(svprntc);
    wtk_main_cfg_delete(main_cfg);
    wtk_arg_delete(arg);
    return 0;
}

int main(int argc,char **argv)
{
    test(argc,argv);
    return 0; 
}

