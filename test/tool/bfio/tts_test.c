#include "qtk/tts/qtk_tts_tac.h"
#include "wtk/core/wtk_arg.h"
#include <limits.h>

void tts_notify(wtk_mer_wav_stream_t *wav,short *data,int len,int is_end)
{
    // short *sil = NULL;
    if(len == 0){return;}
    printf("----------> is_end %d\n",is_end);
    // sil = wtk_calloc(1,wav->fs/3*sizeof(*wav->data));
    wtk_mer_wav_stream_push_short(wav,data,len);
    // wtk_mer_wav_stream_push_short(wav,sil,wav->fs/3);
    // wtk_free(sil);
    return;
}

int test1(int argc,char **argv)
{
    wtk_main_cfg_t *main_cfg = NULL;
    qtk_tts_tac_cfg_t *tac_cfg = NULL;
    wtk_arg_t *arg = NULL;
    char *cfn = NULL,*s = NULL,*ofn = NULL,*scp = NULL;
    qtk_tts_tac_t *tac = NULL;
    wtk_mer_wav_stream_t *wav = NULL;
    wtk_flist_it_t *it = NULL;
    char *off = NULL;

    arg = wtk_arg_new(argc,argv);
    if(arg == NULL) return 1;
    wtk_arg_get_str_s(arg,"c",&cfn);
    wtk_arg_get_str_s(arg,"s",&s);
    wtk_arg_get_str_s(arg,"o",&ofn);
    wtk_arg_get_str_s(arg,"scp",&scp);

    main_cfg = wtk_main_cfg_new_type(qtk_tts_tac_cfg,cfn);
    tac_cfg = main_cfg->cfg;
    
    tac = qtk_tts_tac_new(tac_cfg);

    struct timeval start;
    struct timeval end;
    double duration;

    off = wtk_malloc(PATH_MAX);

    if(scp == NULL){
        wav = wtk_mer_wav_stream_new(2*16000*100,16000,5.0f);
        qtk_tts_tac_set_notify(tac,wav,(qtk_tac_notify_f)tts_notify);
        gettimeofday(&start,NULL);
        qtk_tts_tac_process(tac,s,strlen(s));
        gettimeofday(&end,NULL);
        qtk_tts_tac_reset(tac);
        duration = (1000000*(end.tv_sec-start.tv_sec) + end.tv_usec-start.tv_usec) /1000000.0;
        printf( "总计: %lf s,rate %lf\n", duration,duration/(wav->len*1.0f/wav->fs));
        wtk_mer_wav_stream_savefile(wav,ofn,0.0f);
        wtk_mer_wav_stream_delete(wav);
    }else{
        it = wtk_flist_it_new(scp);
        int cnt = 0;
        while(1){
            s = wtk_flist_it_next(it);
            if(s == NULL){
                break;
            }
            wav = wtk_mer_wav_stream_new(2*16000*100,16000,5.0f);
            qtk_tts_tac_set_notify(tac,wav,(qtk_tac_notify_f)tts_notify);
            gettimeofday(&start,NULL);
            wtk_debug("in %s\n",s);
            qtk_tts_tac_process(tac,s,strlen(s));
            sprintf(off,"%s/test.%d.wav",ofn,cnt);
            gettimeofday(&end,NULL);
            qtk_tts_tac_reset(tac);
            duration = (1000000*(end.tv_sec-start.tv_sec) + end.tv_usec-start.tv_usec) /1000000.0;
            printf( "总计: %lf s,rate %lf\n", duration,duration/(wav->len*1.0f/wav->fs));
            wtk_mer_wav_stream_savefile(wav,off,0.0f);
            wtk_mer_wav_stream_delete(wav);
            cnt++;
        }
        wtk_flist_it_delete(it);
    }

    wtk_free(off);
    qtk_tts_tac_delete(tac);
    wtk_main_cfg_delete(main_cfg);
    wtk_arg_delete(arg);
    return 0;
}

int test2(int argc,char **argv)
{
    #define TEST_N (5)
    wtk_main_cfg_t *main_cfg = NULL;
    qtk_tts_tac_cfg_t *tac_cfg = NULL;
    wtk_arg_t *arg = NULL;
    char *cfn = NULL,*ofn = NULL,*s = NULL;
    char *scp = NULL;
    qtk_tts_tac_t *tac = NULL;
    wtk_mer_wav_stream_t *wav = NULL;
    // int i;
    wtk_flist_it_t *it = NULL;
    char *off = NULL;
    off = wtk_malloc(PATH_MAX);
    // int j;
    // char *text[]={
    //     "Youth is not a time of life;",
    //     // " it is a state of mind.",
    //     // " It is not a matter of rosy cheeks,",
    //     // " red lips and supple knees.",
    //     // " It is a matter of the will,",
    //     // " a quality of the imagination,",
    //     // " vigor of the emotions;",
    //     // " it is the freshness of the deep spring of life.",
    //     // "how are you?",
    //     // "How old are you.",
    //     // "What is your name?",
    //     // "How old are you.",
    //     // "Ladies and gentlemen,",
    //     // "welcome to our school.",
    //     // "What is your name?",
    //     // "How old are you.",
    //     // "What is your name?",
    //     // "How old are you"
    // };
        
    arg = wtk_arg_new(argc,argv);
    if(arg == NULL) return 1;
    wtk_arg_get_str_s(arg,"c",&cfn);
    wtk_arg_get_str_s(arg,"s",&s);
    wtk_arg_get_str_s(arg,"o",&ofn);
    wtk_arg_get_str_s(arg,"scp",&scp);
    

    main_cfg = wtk_main_cfg_new_type(qtk_tts_tac_cfg,cfn);
    tac_cfg = main_cfg->cfg;
    
    // wav = wtk_mer_wav_stream_new(2*16000*30,16000,5.0f);
    tac = qtk_tts_tac_new(tac_cfg);
    // qtk_tts_tac_set_notify(tac,wav,(qtk_tts_notify_f)tts_notify);
    
    struct timeval start;
    struct timeval end;
    double duration;

    // gettimeofday(&start,NULL);
    

    // for(j=0;j<100;++j)
    // {
        // for(i=0;i<sizeof(text)/sizeof(char*);++i)
        // {
        //     printf("==> [%s] %d %ld\n",text[i],i,sizeof(text)/sizeof(char*));
        //     qtk_tts_tac_process(tac,text[i],strlen(text[i]));
        // }
    // }
    //  qtk_tts_tac_process(tac,s,strlen(s));

    if(scp == NULL){
        wav = wtk_mer_wav_stream_new(2*16000*100,16000,5.0f);
        qtk_tts_tac_set_notify(tac,wav,(qtk_tac_notify_f)tts_notify);
        gettimeofday(&start,NULL);
        qtk_tts_tac_process(tac,s,strlen(s));
        gettimeofday(&end,NULL);
        qtk_tts_tac_reset(tac);
        duration = (1000000*(end.tv_sec-start.tv_sec) + end.tv_usec-start.tv_usec) /1000000.0;
        printf( "总计: %lf s,rate %lf\n", duration,duration/(wav->len*1.0f/wav->fs));
        wtk_mer_wav_stream_savefile(wav,ofn,0.0f);
        wtk_mer_wav_stream_delete(wav);
    }else{
        it = wtk_flist_it_new(scp);
        int cnt = 0;
        while(1){
            s = wtk_flist_it_next(it);
            if(s == NULL){
                break;
            }
            //去个尾巴
            // *(s+strlen(s)-1) = '\0';
            wav = wtk_mer_wav_stream_new(2*16000*100,16000,5.0f);
            qtk_tts_tac_set_notify(tac,wav,(qtk_tac_notify_f)tts_notify);
            gettimeofday(&start,NULL);
            wtk_debug("in %s\n",s);
            qtk_tts_tac_process(tac,s,strlen(s));
            sprintf(off,"%s/test.%d.wav",ofn,cnt);
            gettimeofday(&end,NULL);
            qtk_tts_tac_reset(tac);
            duration = (1000000*(end.tv_sec-start.tv_sec) + end.tv_usec-start.tv_usec) /1000000.0;
            printf( "总计: %lf s,rate %lf\n", duration,duration/(wav->len*1.0f/wav->fs));
            wtk_mer_wav_stream_savefile(wav,off,0.0f);
            wtk_mer_wav_stream_delete(wav);
            cnt++;
        }
        wtk_flist_it_delete(it);
    }

    // gettimeofday(&end,NULL);
    // duration = (1000000*(end.tv_sec-start.tv_sec) + end.tv_usec-start.tv_usec) /1000000.0;
    // printf( "总计: %lf s,rate %lf\n", duration,duration/(wav->len*1.0f/wav->fs));

    // wtk_mer_wav_stream_savefile(wav,ofn,0.0f);

    // wtk_mer_wav_stream_delete(wav);
    wtk_free(off);
    qtk_tts_tac_delete(tac);
    wtk_main_cfg_delete(main_cfg);
    wtk_arg_delete(arg);
    return 0;
}

int main(int argc,char **argv)
{
    test2(argc,argv);
    // test1(argc,argv);
    return 0;
}
