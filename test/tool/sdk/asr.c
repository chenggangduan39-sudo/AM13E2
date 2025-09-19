#include "sdk/qtk_api.h"
#include <pthread.h>
#include <stdio.h>
#include "wtk/core/wtk_arg.h"

extern char* file_read_buf(char* fn, int *n);

void test_easr_on_notify(qtk_session_t *session, qtk_var_t *var) {
    switch (var->type) {
    case QTK_ASR_TEXT:
        printf("result: %.*s\n", var->v.str.len, var->v.str.data);
//        wtk_sem_release(&sem, 1);
        break;
    case QTK_VAR_ERR:
    	printf("result: no result\n");
//        wtk_sem_release(&sem, 1);
        break;
    default:
        break;
    }
}


int main(int argc, char **argv)
{
	qtk_engine_t*e=NULL;
    qtk_session_t *s = NULL;
    wtk_arg_t* args;
    char *params;
    char *data, *wav_fn;
    int len, ret;

    args = wtk_arg_new(argc, argv);
    ret = wtk_arg_get_str_s(args, "i", &wav_fn);
    if (ret != 0) {
        printf("usage:  ./asr -i xxx.wav\n");
        goto end;
    }
    params = "appid=57fe0d38-bd8a-11ed-b526-00163e13c8a2;secretkey=42ddc64a-"
             "1c61-39bc-9cfd-0aa8bdd1009c;cache_path=/tmp/out;use_srvsel=1;use_log=1;";
    s = qtk_session_init(params, 0, NULL, NULL);
    if (!s) {
        printf("session failed.\n");
        goto end;
    }
    //asr
//    params =
//        "role=asr;cfg=res/sdk/asr/xhy.cfg;use_bin=0;use_thread=0;"
//        "rec_min_conf=1.8;usrec_min_conf=1.5;skip_space=1;winStep=100;";
    //k2-asr
    params =
        "role=asr;cfg=res/sdk/asr/hdgd.cfg;use_bin=0;use_thread=0;";
    e = qtk_engine_new(s, params);
    if (!e) {
        printf("new asr engine failed.\n");
        goto end;
    }
    qtk_engine_set_notify(e, s, (qtk_engine_notify_f)test_easr_on_notify);
    qtk_engine_start(e);
	data = file_read_buf(wav_fn, &len);
    if (len <= 0) {
        printf("not find wav\n");
        goto end;
    }
    data= data+44;
    len = len-44;
    qtk_engine_reset(e);
    qtk_engine_start(e);
    qtk_engine_feed(e, data, len, 0);  //maybe mutli-times feed
    qtk_engine_feed(e, 0, 0, 1);  //only one time
    qtk_engine_reset(e);   //end and get result
    free(data-44);
end:
if (args)
	wtk_arg_delete(args);
if (e) {
    qtk_engine_delete(e);
}
if (s) {
    qtk_session_exit(s);
}

}
