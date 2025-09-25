#include "qtk/acoustic/durian/qtk_durian.h"
#include "wtk/core/cfg/wtk_main_cfg.h"

void print_arg(void)
{
    printf("the function is :\n");
    printf("\t\t -c cfg file fn\n");
    // printf("\t\t -scp scp file fn\n");
    // printf("\t\t -o out dir/file\n");
    return;
}

int en1[] = {
    1, 42, 35, 9, 29, 20, 42, 42, 35, 17, 42, 42, 5, 45, 21, 10, 40, 42, 42, 
    35, 17, 42, 42, 24, 5, 45, 27, 14, 42, 43, 12, 40, 42, 42, 29, 3, 35, 42, 
    42, 39, 17, 42, 42, 3, 32, 42, 42, 37, 9, 45, 32, 13, 42, 42, 5, 45, 35, 32, 
    4, 26, 45, 35, 12, 37, 42, 44, 18, 5, 35, 42, 42, 39, 6, 32, 42, 42, 33, 35, 
    17, 45, 31, 5, 20, 42, 43, 35, 17, 42, 42, 27, 9, 35, 42, 42, 5, 42, 42, 31, 
    10, 45, 33, 5, 29, 42, 42, 27, 5, 37, 42, 42, 20, 12, 32, 45, 27, 13, 42, 44, 1, 42
};

int get_mel(void *user_data,wtk_matf_t *mel,int is_end)
{
    // wtk_mer_matf_shape_print(mel);
    wtk_mer_matf_write_file(mel,"./mel/en_mel_man_809_80.bin",1);
    return 0;
}

int main(int argc,char **argv)
{
    wtk_main_cfg_t *main_cfg = NULL;
    qtk_durian_cfg_t *durian_cfg = NULL;
    qtk_durian_t *durian = NULL;
    wtk_arg_t *arg = NULL;
    char *cfn = NULL;
    wtk_veci_t *token = NULL;

    arg = wtk_arg_new(argc,argv);
    if(arg == NULL){
        goto end;
    }
    wtk_arg_get_str_s(arg,"c",&cfn);
    if(cfn == NULL){print_arg();goto end;}
    main_cfg = wtk_main_cfg_new_type(qtk_durian_cfg,cfn);
    durian_cfg = main_cfg->cfg;
    durian = qtk_durian_new(durian_cfg);
    token = wtk_veci_new(sizeof(en1)/sizeof(int));
    memcpy(token->p,en1,sizeof(en1));
    qtk_durian_set_notify(durian,get_mel,NULL);
    struct timeval times;
    gettimeofday(&times,NULL);
    qtk_durian_process(durian,token,1);
    struct timeval timee;
    gettimeofday(&timee,NULL);
    float t = (timee.tv_sec*1000+timee.tv_usec/1000)-(times.tv_sec*1000+times.tv_usec/1000);
    wtk_debug("use time %f\n",t/1000);
end:
    if(token) wtk_veci_delete(token);
    if(durian) qtk_durian_delete(durian);
    if(main_cfg) wtk_main_cfg_delete(main_cfg);
    if(arg) wtk_arg_delete(arg);
    return 0;
}
