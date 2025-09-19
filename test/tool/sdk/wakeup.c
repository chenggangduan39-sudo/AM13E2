#include "sdk/qtk_api.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/wtk_os.h"
#include "wtk/os/wtk_sem.h"
wtk_sem_t sem;
double st, t, wav_t;
void test_wakeup_on_notify(qtk_session_t *session, qtk_var_t *var) {
	int res;

	res = var->v.i;
	printf("res %d\n",res);
}

int main(int argc, char *argv[]) {
    qtk_session_t *s = NULL;
    qtk_engine_t *e = NULL;
    wtk_arg_t *arg = NULL;
    wtk_strbuf_t *buf = NULL;
    char *params;
    char *data = NULL;
    char *wav_fn;
    int consumed;
    int rate;
    int len;
    int ret;

//    wtk_sem_init(&sem, 0);
    buf = wtk_strbuf_new(3200, 1);
    arg = wtk_arg_new(argc, argv);
    ret = wtk_arg_get_str_s(arg, "i", &wav_fn);
    ret = 0;
//    wav_fn = "C:\\Users\\qdreamer\\qtk\\who.wav";
    if (ret != 0) {
        printf("usage:  ./wakeup -i xxx.wav\n");
        goto end;
    }
    data = file_read_buf(wav_fn, &len);
    if (len <= 0) {
        printf("not find wav\n");
        goto end;
    }
    //linux
    params = "appid=57fe0d38-bd8a-11ed-b526-00163e13c8a2;secretkey=42ddc64a-"
             "1c61-39bc-9cfd-0aa8bdd1009c;cache_path=/tmp/out;use_srvsel=1;use_log=1;";
    s = qtk_session_init(params, 0, NULL, NULL);
    params =
        "role=wakeup;cfg=res/sdk/wakeup/cfg;use_bin=0;use_thread=0;";
    e = qtk_engine_new(s, params);
    if (!e) {
        printf("new engine failed.\n");
        goto end;
    }
    qtk_engine_set_notify(e, s, (qtk_engine_notify_f)test_wakeup_on_notify);
//    int cnt=10;
//    while(1)
//    {
    qtk_engine_start(e);
    st = time_get_ms();

    if (buf->pos > 0) {
        qtk_engine_feed(e, buf->data, buf->pos, 1);
    } else {
        qtk_engine_feed(e, data + 44, len - 44, 0);
    }
    t=(time_get_ms()-st)/1000.0;
    wav_t= (len-44) /16000.0;
    //...end for a time wave
    printf("run-time=%f(s) wav-dur= %f(s) rate= %f\n",t,wav_t, t/wav_t);
//    wtk_sem_acquire(&sem, -1);
    qtk_engine_reset(e);      //note: need to be done when  qtk_engine_feed(,,,1).
//    }
end:
    wtk_sem_clean(&sem);
    if (arg) {
        wtk_arg_delete(arg);
    }
    if (e) {
        qtk_engine_delete(e);
    }
    if (s) {
        qtk_session_exit(s);
    }
    if (data) {
        wtk_free(data);
    }
    if (buf) {
        wtk_strbuf_delete(buf);
    }
    return 0;
}
