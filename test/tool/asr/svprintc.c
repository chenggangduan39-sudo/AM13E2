#include "wtk/asr/kws/qtk_svprintc.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/wtk_arg.h"

//float* get_feamer(int *len);

//默认是100
#define TEST2_DEFINE_MAN (100)
int test2(int argc ,char **argv);
int test1(int argc, char **argv);

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

int main(int argc ,char **argv)
{
    test1(argc,argv);
    // test2(argc,argv);
    return 0;
}

int test2_name_in(qtk_svprintc_t *svprint, float **list,int n,int *cnt,float *p)
{
    int ret = -1;
    int i = 0;
    float retd = 0.0f;
    for(i = 0;list[i] != NULL; i++){
        retd = qtk_svprintc_get_likehood(svprint,list[i],128,cnt[i]+1,p,128);
        wtk_debug("list %d cnt %d %f\n",i,cnt[i],retd);
        if(retd > 20.f){
            ret = i;
            goto end;
        }
    }
end:
    return ret;
}

int test2_name_add(float *dst,int cnt,float *src)
{
    int i = 0;
    for(i = 0; i < 128; ++i){
        dst[i] = (dst[i]*(cnt)+src[i])/(cnt+1);
    }
    return 0;
}

int test2(int argc ,char **argv)
{
    wtk_main_cfg_t *main_cfg = NULL;
    qtk_svprintc_cfg_t *cfg = NULL;
    wtk_arg_t *arg = NULL;
    qtk_svprintc_t* svprntc = NULL;
    wtk_strbuf_t *buf1 = NULL;

    char *cfn = NULL;
    arg = wtk_arg_new(argc,argv);
    wtk_arg_get_str_s(arg,"c",&cfn);

    main_cfg = wtk_main_cfg_new_type(qtk_svprintc_cfg,cfn);
    cfg = (qtk_svprintc_cfg_t*)main_cfg->cfg;

    svprntc =  qtk_svprintc_new(cfg);
    buf1 = wtk_strbuf_new(1024,1.0f);
    qtk_svprintc_set_notify(svprntc,buf1,(qtk_svprintc_notify_f*)test_on_notify);

    char *path[] = {
        // "1660040993914/1.pcm",
        // "1660040993914/2.pcm",
        // "1660040993914/3.pcm",
        // "1660040993914/4.pcm",
        // "1660040993914/5.pcm",
        // // "1660040993914/6.pcm",
        // "1660040993914/7.pcm",
        // "1660040993914/8.pcm",
        // "1660040993914/9.pcm",
        // "1660040993914/10.pcm",
        // "1660040993914/11.pcm",
        // "",
        "1660096248179/1.pcm",
        "1660096248179/2.pcm",
        "1660096248179/3.pcm",
        "1660096248179/4.pcm",
        "1660096248179/5.pcm",
        "1660096248179/6.pcm",
        "1660096248179/7.pcm",
        "1660096248179/8.pcm",
        "1660096248179/9.pcm",
        "1660096248179/10.pcm",
        "1660096248179/11.pcm",
        "1660096248179/12.pcm",
        "1660096248179/13.pcm",
        "1660096248179/14.pcm",
        "1660096248179/15.pcm",
        "1660096248179/16.pcm",
        "1660096248179/17.pcm",
        "",
    };
    int i = 0;
    float **list = wtk_malloc(sizeof(float*)*TEST2_DEFINE_MAN);
    memset(list,0,sizeof(float*)*TEST2_DEFINE_MAN);
    int *cnt = wtk_malloc(sizeof(int)*TEST2_DEFINE_MAN);
    memset(cnt,0,sizeof(int)*TEST2_DEFINE_MAN);
    int nman = 0;
    for(;strlen(path[i])>0;++i){
        wtk_debug("======> %s\n",path[i]);
        FILE *f1 = fopen(path[i],"rb");
        if(f1 == NULL){
            goto end;
        }
        fseek(f1,0,SEEK_END);
        int l = ftell(f1);
        fseek(f1,0,SEEK_SET);
        char *data = wtk_malloc(l);
        fread(data,1,l,f1);
        // qtk_svprintc_feed2(svprntc,(short*)(data+44),(l-44)/2,1);
        qtk_svprintc_feed2(svprntc,(short*)(data),(l)/2,1);
        int ret = test2_name_in(svprntc,list,nman,cnt,(float*)buf1->data);
        if(ret < 0){
            list[nman] = wtk_malloc(sizeof(float)*128);
            memcpy(list[nman],buf1->data,sizeof(float)*128);
            nman+=1;
        }else{
            cnt[ret] += 1;
            test2_name_add(list[ret],cnt[ret],(float*)buf1->data);
        }
        wtk_free(data);
        fclose(f1);
        wtk_strbuf_reset(buf1);
        qtk_svprintc_reset(svprntc);
    }

end:
    if(svprntc) qtk_svprintc_delete(svprntc);
    wtk_strbuf_delete(buf1);
    wtk_main_cfg_delete(main_cfg);
    wtk_arg_delete(arg);
    return 0;
}

int test1(int argc, char **argv)
{
    wtk_main_cfg_t *main_cfg = NULL;
    qtk_svprintc_cfg_t *cfg = NULL;
    wtk_arg_t *arg = NULL;
    qtk_svprintc_t* svprntc = NULL;
    wtk_strbuf_t *buf1 = NULL;
    wtk_strbuf_t *buf2 = NULL;
    char *cfn = NULL;
    char *wav1 = NULL;
    char *wav2 = NULL;

    arg = wtk_arg_new(argc,argv);
    wtk_arg_get_str_s(arg,"c",&cfn);
    wtk_arg_get_str_s(arg,"wav1",&wav1);
    wtk_arg_get_str_s(arg,"wav2",&wav2);

    if(!cfn || !wav1 || !wav2){
        wtk_debug("arg error -c cfg -wav1 fn1 -wav2 fn2\n");
        goto end;
    }

    main_cfg = wtk_main_cfg_new_type(qtk_svprintc_cfg,cfn);
    cfg = (qtk_svprintc_cfg_t*)main_cfg->cfg;

    buf1 = wtk_strbuf_new(1024,1.0f);
    buf2 = wtk_strbuf_new(1024,1.0f);
    int i = 0;
    for(i = 0; i < 1; ++i){
        svprntc =  qtk_svprintc_new(cfg);
        if(wav1){
            qtk_svprintc_reset(svprntc);
            qtk_svprintc_set_notify(svprntc,buf1,(qtk_svprintc_notify_f*)test_on_notify);
            FILE *f1 = fopen(wav1,"rb");
            if(f1 == NULL){
                goto end;
            }
            fseek(f1,0,SEEK_END);
            int l = ftell(f1);
            fseek(f1,0,SEEK_SET);
            char *data = wtk_malloc(l);
            fread(data,1,l,f1);
            qtk_svprintc_start(svprntc);
            printf(">>>>>1\n");
            int fam_len = 0;
            //float *fam = get_feamer(&fam_len);
            int iu = 0;
            // wtk_debug("%d\n",fam_len);
            // int cc = 0;
            // for(;iu < fam_len; iu+=40){
            //     qtk_svprint_test(svprntc,fam+iu,40,1,0);
            //     cc ++;
            // }
            // qtk_svprint_test(svprntc,fam,40,358,0);
            // wtk_debug("cc %d\n",cc);
            // qtk_svprint_test(svprntc,NULL,0,0,1);
            qtk_svprintc_feed2(svprntc,(short*)(data),(l)/2,1);
            // qtk_svprintc_feed2(svprntc,(short*)(data+44),8000,1);
            printf(">>>>>1 end\n");
            fclose(f1);
            wtk_free(data);
        }
        // exit(1);
        if(wav2){
            qtk_svprintc_reset(svprntc);
            qtk_svprintc_set_notify(svprntc,buf2,(qtk_svprintc_notify_f*)test_on_notify);
            FILE *f1 = fopen(wav2,"rb");
            if(f1 == NULL){
                goto end;
            }
            fseek(f1,0,SEEK_END);
            int l = ftell(f1);
            fseek(f1,0,SEEK_SET);
            char *data = wtk_malloc(l);
            fread(data,1,l,f1);
            qtk_svprintc_start(svprntc);
            printf(">>>>>2\n");
            qtk_svprintc_feed2(svprntc,(short*)(data),(l)/2,1);
            printf(">>>>>2 end\n");
            fclose(f1);
            wtk_free(data);
        }
        float ret = qtk_svprintc_get_likehood(svprntc,(float*)buf1->data,buf1->pos/sizeof(float),1,(float*)buf2->data,buf2->pos/sizeof(float));
        wtk_debug("=======> this ret %f \n",ret);
        if(svprntc) qtk_svprintc_delete(svprntc);
        wtk_strbuf_reset(buf1);
        wtk_strbuf_reset(buf2);
    }
end:
    
    if(buf2) wtk_strbuf_delete(buf2);
    if(buf1) wtk_strbuf_delete(buf1);
    if(main_cfg){wtk_main_cfg_delete(main_cfg);}
    if(arg){
        wtk_arg_delete(arg);
    }
    return 0;
}
