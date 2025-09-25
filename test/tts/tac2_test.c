#include <stdio.h>
#include "qtk/tts/acoustic/tac2_syn/qtk_tts_tac2_syn.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "qtk/tts/qtk_tts_tac2_lpcnet.h"

int id3[] = {
    48, 5, 46, 35, 7, 2, 28, 15, 31, 38, 7, 5, 35, 15, 9, 2, 27,
     15, 7, 5, 35, 20, 31, 38, 8, 5, 33, 29, 10, 2, 33, 15, 31, 38, 9, 
     5, 32, 15, 7, 5, 39, 20, 8, 4, 30, 29, 8, 2, 18, 20, 6, 5, 26, 15, 31, 
     38, 9, 5, 28, 29, 10, 3, 18, 31, 23, 7, 5, 40, 15, 6, 2, 22, 24, 7, 5, 32, 
     15, 6, 2, 16, 12, 8, 5, 27, 31, 38, 7, 4, 48, 5, 1,
};

int id4[] = {
    48, 5, 36, 20, 6, 5, 45, 24, 6, 2, 40, 15, 7, 5, 16, 42, 23, 9, 5, 45, 29, 
    7, 3, 37, 31, 23, 8, 5, 28, 20, 44, 9, 2, 14, 29, 6, 5, 11, 15, 9, 2, 30, 
    24, 8, 5, 26, 15, 31, 38, 9, 5, 39, 13, 23, 6, 5, 36, 15, 6, 3, 17, 15, 9,
    5, 30, 15, 9, 5, 11, 15, 46, 6, 3, 30, 15, 8, 2, 35, 42, 38, 7, 5, 39, 13, 
    23, 6, 5, 45, 15, 9, 5, 38, 42, 23, 7, 2, 30, 31, 38, 7, 5, 11, 15, 13, 6, 2, 
    37, 21, 7, 5, 45, 20, 8, 5, 33, 29, 10, 3, 14, 29, 6, 5, 11, 15, 9, 5, 26, 15, 
    23, 7, 2, 40, 15, 8, 5, 30, 29, 9, 4, 17, 15, 9, 2, 45, 15, 9, 5, 38, 42, 23, 
    7, 2, 30, 20, 8, 5, 30, 15, 38, 6, 5, 11, 15, 9, 5, 17, 20, 9, 2, 34, 31, 23, 
    7, 5, 30, 29, 9, 5, 28, 15, 8, 3, 14, 20, 12, 9, 5, 36, 20, 9, 2, 18, 42, 23, 
    7, 5, 45, 31, 23, 8, 5, 33, 29, 10, 3, 30, 15, 6, 5, 45, 15, 6, 2, 26, 15, 38, 
    6, 5, 26, 15, 13, 9, 4, 45, 20, 31, 38, 6, 5, 45, 20, 9, 5, 30, 20, 7, 3, 30, 
    15, 8, 2, 16, 12, 9, 5, 14, 29, 9, 5, 41, 42, 23, 6, 2, 45, 42, 38, 9, 5, 28, 
    15, 25, 9, 2, 36, 20, 31, 38, 9, 5, 41, 46, 8, 3, 30, 20, 8, 5, 30, 15, 38, 6, 
    5, 17, 15, 7, 5, 19, 15, 25, 7, 4, 30, 20, 8, 5, 30, 15, 9, 2, 33, 20, 15, 9, 5, 
    34, 20, 46, 9, 2, 11, 15, 7, 3, 30, 20, 8, 5, 30, 15, 38, 6, 2, 34, 29, 7, 5, 18, 
    42, 23, 7, 2, 11, 15, 9, 5, 17, 20, 9, 3, 37, 21, 7, 2, 34, 29, 7, 5, 26, 15, 38, 6, 
    5, 33, 29, 10, 3, 35, 42, 38, 7, 5, 39, 13, 23, 6, 5, 45, 15, 9, 5, 38, 42, 23, 7, 
    2, 11, 15, 9, 5, 17, 20, 9, 2, 30, 31, 38, 7, 5, 11, 15, 13, 6, 4, 19, 15, 23, 9, 2, 
    17, 15, 7, 5, 26, 15, 31, 38, 9, 2, 40, 15, 7, 3, 32, 12, 9, 2, 11, 15, 6, 5, 40, 15, 
    9, 5, 35, 42, 38, 7, 4, 46, 35, 7, 5, 22, 13, 23, 7, 2, 37, 31, 38, 7, 5, 11, 20, 9, 
    2, 33, 42, 23, 8, 4, 45, 15, 9, 5, 38, 42, 23, 7, 2, 30, 15, 23, 9, 5, 11, 15, 31, 
    38, 9, 2, 28, 15, 23, 8, 5, 30, 20, 9, 5, 33, 29, 10, 3, 30, 15, 23, 9, 5, 30, 13, 
    23, 9, 4, 48, 5, 1,
};

void test_notify(void *user_data,wtk_matf_t *mel,int is_end)
{
    return;
}

void test_notify2(wtk_mer_wav_stream_t *user_data,short *data,int len,int is_end)
{
    wtk_mer_wav_stream_push_short(user_data,data,len);
    return;
}

int test1(int argc,char **argv)
{
    wtk_arg_t *arg = NULL;
    wtk_main_cfg_t *main_cfg = NULL;
    qtk_tts_tac2_syn_cfg_t *syn_cfg = NULL;
    qtk_tts_tac2_syn_t *syn = NULL;
    char *cfn = NULL;
    wtk_veci_t *veci = NULL;

    arg = wtk_arg_new(argc,argv);
    wtk_arg_get_str_s(arg,"c",&cfn);

    main_cfg = wtk_main_cfg_new_type(qtk_tts_tac2_syn_cfg,cfn);
    syn_cfg = main_cfg->cfg;

    syn = qtk_tts_tac2_syn_new(syn_cfg);
    qtk_tts_tac2_syn_set_notify(syn,NULL,test_notify);
    veci = wtk_veci_new(sizeof(id3)/sizeof(int));
    memcpy(veci->p,id3,sizeof(id3));
    qtk_tts_tac2_syn_process(syn,veci, 1);

    qtk_tts_tac2_syn_delete(syn);
    wtk_veci_delete(veci);
    wtk_main_cfg_delete(main_cfg);
    wtk_arg_delete(arg);
    return 0;
}

int test2(int argc,char **argv)
{
    wtk_arg_t *arg = NULL;
    char *cfn = NULL, *ofn = NULL;
    wtk_main_cfg_t *main_cfg = NULL;
    qtk_tts_tac2_lpcnet_cfg_t *cfg = NULL;
    wtk_veci_t *veci = NULL;
    qtk_tts_tac2_lpcnet_t *syn = NULL;
    wtk_mer_wav_stream_t *wav = NULL;

    arg = wtk_arg_new(argc,argv);
    wtk_arg_get_str_s(arg,"c",&cfn);
    wtk_arg_get_str_s(arg,"o",&ofn);

    main_cfg = wtk_main_cfg_new_type(qtk_tts_tac2_lpcnet_cfg,cfn);
    cfg = main_cfg->cfg;
    wav = wtk_mer_wav_stream_new(2*16000*60,16000,5.0f);
    syn = qtk_tts_tac2_lpcnet_new(cfg);
    qtk_tts_tac2_lpcnet_set_notify(syn,wav,(qtk_tts_tac2_lpcnet_notify_f)test_notify2);
    veci = wtk_veci_new(sizeof(id4)/sizeof(int));

    memcpy(veci->p,id4,sizeof(id4));
    qtk_tts_tac2_lpcnet_process(syn,veci,1);

    wtk_mer_wav_stream_savefile(wav,ofn,0.0f);
    wtk_veci_delete(veci);
    qtk_tts_tac2_lpcnet_delete(syn);
    wtk_mer_wav_stream_delete(wav);
    wtk_main_cfg_delete(main_cfg);
    wtk_arg_delete(arg);
    return 0;
}

int main(int argc,char **argv)
{
    test2(argc,argv);
    return 0;
}
