#include "tts-tac/cfg/wtk_tac_cfg_syn_lpcnet.h"
#include "tts-tac/model/wtk_tac_lpcnet.h"
#include "wtk/core/wtk_arg.h"

void print_arg(void)
{
    printf("the function is :\n");
    printf("\t\t-c cfg file fn\n");
    printf("\t\t-b cfg file is bin file\n");
    printf("\t\t-i mel file fn\n");
    printf("\t\t-o out dir\n");
    return;
}

double ts,te;

void test2_notify(void *user_data,float *data,int len,int is_end)
{
    wtk_mer_wav_stream_t *wav = user_data;
    int i = 0;
    short *out = malloc(sizeof(short)*len);
    for(i = 0; i < len; ++i){
        out[i] = data[i];
    }
    te = time_get_ms();
    printf("----> %lf\n",te-ts);
    ts = time_get_ms();
    wtk_mer_wav_stream_push_short(wav,out,len);
    
    free(out);
    return;
}


//lpcnet 接受mel频谱进行处理
int test1(int argc,char **argv)
{
    wtk_arg_t *arg = NULL;
    char *cfn = NULL;
    char *ifn = NULL;
    char *ofn = NULL;
    int is_bin = 0;
    wtk_tac_cfg_syn_lpcnet_t *cfg = NULL;
    wtk_main_cfg_t *main_cfg = NULL;
    wtk_mbin_cfg_t *mbin_cfg = NULL;
    wtk_matf_t *mel = NULL;
    wtk_source_loader_t sl;
    wtk_source_t source;
    wtk_mer_wav_stream_t *wav = NULL;
    

    arg = wtk_arg_new(argc,argv);
    if(arg == NULL) goto end;
    wtk_arg_get_str_s(arg, "i", &ifn);
    wtk_arg_get_str_s(arg,"o",&ofn);
    wtk_arg_get_str_s(arg,"c",&cfn);
    wtk_arg_get_int_s(arg,"b",&is_bin);

    if(!cfn || (!ifn)||!ofn)
    {
        print_arg();
        goto end;
    }

    if(cfn && is_bin){
        mbin_cfg = wtk_mbin_cfg_new_str_type(wtk_tac_cfg_syn_lpcnet,"./cfg",cfn,strlen(cfn));
        cfg = mbin_cfg->cfg;
    }else if(cfn){
        wtk_debug("%s\n",cfn);
        main_cfg = wtk_main_cfg_new_type(wtk_tac_cfg_syn_lpcnet,cfn);
        cfg = main_cfg->cfg;
    }

    mel = wtk_matf_new(809,80);
    // mel = wtk_matf_new(50,80);
    wtk_source_loader_init_file(&sl);
    wtk_mer_source_loader_load_matf(&sl,&source,ifn,mel);
    
    wav = wtk_mer_wav_stream_new(sizeof(short)*512*1024,16000,5.0f);

    wtk_tac_lpcnet_process(cfg,mel,wav,1);

    wtk_mer_wav_stream_savefile(wav,ofn,0.0);

    wtk_mer_wav_stream_delete(wav);
    wtk_matf_delete(mel);
end:
    if(mbin_cfg){
        wtk_mbin_cfg_delete(mbin_cfg);
    }
    if(main_cfg){
        wtk_main_cfg_delete(main_cfg);
    }
    if(arg){
        wtk_arg_delete(arg);
    }
    return 0;
}

int test2(int argc,char **argv)
{
    wtk_arg_t *arg = NULL;
    char *cfn = NULL;
    char *ifn = NULL;
    char *ofn = NULL;
    int is_bin = 0;
    wtk_tac_cfg_syn_lpcnet_t *cfg = NULL;
    wtk_main_cfg_t *main_cfg = NULL;
    wtk_mbin_cfg_t *mbin_cfg = NULL;
    wtk_matf_t *mel = NULL;
    wtk_source_loader_t sl;
    wtk_source_t source;
    wtk_mer_wav_stream_t *wav = NULL;
    

    arg = wtk_arg_new(argc,argv);
    if(arg == NULL) goto end;
    wtk_arg_get_str_s(arg, "i", &ifn);
    wtk_arg_get_str_s(arg,"o",&ofn);
    wtk_arg_get_str_s(arg,"c",&cfn);
    wtk_arg_get_int_s(arg,"b",&is_bin);

    if(!cfn || (!ifn)||!ofn)
    {
        print_arg();
        goto end;
    }

    if(cfn && is_bin){
        mbin_cfg = wtk_mbin_cfg_new_str_type(wtk_tac_cfg_syn_lpcnet,"./cfg",cfn,strlen(cfn));
        cfg = mbin_cfg->cfg;
    }else if(cfn){
        wtk_debug("%s\n",cfn);
        main_cfg = wtk_main_cfg_new_type(wtk_tac_cfg_syn_lpcnet,cfn);
        cfg = main_cfg->cfg;
    }

    // mel = wtk_matf_new(1964,80);
    mel = wtk_matf_new(30,80);
    wtk_source_loader_init_file(&sl);
    wtk_mer_source_loader_load_matf(&sl,&source,ifn,mel);
    
    wav = wtk_mer_wav_stream_new(sizeof(short)*512*1024,16000,5.0f);
    wtk_tac_lpcnet_set_notify(cfg,test2_notify,wav);

    int i = 0;
    ts = time_get_ms();
    wtk_matf_t in;
    for(i = 0; i < mel->row;++i){
        in.p = mel->p+mel->col*i;
        in.row = 1;
        in.col = mel->col;
        if(i < mel->row - 1){
            wtk_tac_lpcnet_process_stream(cfg,&in,0);
        }else{
            wtk_tac_lpcnet_process_stream(cfg,&in,1);
        }
    }

    wtk_mer_wav_stream_savefile(wav,ofn,0.0);

    wtk_mer_wav_stream_delete(wav);
    wtk_matf_delete(mel);
end:
    if(mbin_cfg){
        wtk_mbin_cfg_delete(mbin_cfg);
    }
    if(main_cfg){
        wtk_main_cfg_delete(main_cfg);
    }
    if(arg){
        wtk_arg_delete(arg);
    }
    return 0;
}

int main(int argc, char **argv)
{
    test1(argc,argv);
    // test2(argc,argv); 
    return 0;
}
