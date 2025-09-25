#include "sdk/qtk_api.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/wtk_os.h"
#include "wtk/os/wtk_sem.h"
wtk_sem_t sem;

void* wav;
double st, t, wav_t;
void test_aec_on_notify(qtk_session_t *session, qtk_var_t *var) {
	int len;
	if (var->type == QTK_SPEECH_DATA_PCM)
	{
		printf("aec...len=%d\n", var->v.str.len);
        if (wav && var->v.str.len > 0)
                wtk_wavfile_write(wav, var->v.str.data, var->v.str.len);
	}
}

int main(int argc, char *argv[]) {
    qtk_session_t *s = NULL;
    qtk_engine_t *e = NULL;
    wtk_arg_t *arg = NULL;
    wtk_strbuf_t *buf = NULL;
    char *params;
    char *data = NULL;
    char *wav_fn, *ofn;
    int consumed;
    int rate;
    int len;
    int ret;

    wtk_sem_init(&sem, 0);
    buf = wtk_strbuf_new(3200, 1);
    arg = wtk_arg_new(argc, argv);
    ret = wtk_arg_get_str_s(arg, "i", &wav_fn);
    //wav_fn = "C:\\Users\\qdreamer\\qtk\\who.wav";
    if (ret != 0) {
        printf("usage:  ./aec -i xxx.wav\n");
        goto end;
    }
    data = file_read_buf(wav_fn, &len);
    if (len <= 0) {
        printf("not find wav\n");
        goto end;
    }
    ret = wtk_arg_get_int_s(arg, "r", &rate);
    if (ret != 0) {
        rate = 16000;
    }

    ofn="./tmp.wav";

    wav = wtk_wavfile_new(16000);
    wtk_wavfile_set_channel(wav, 8);
    if (wtk_wavfile_open(wav,ofn)) {
        printf("Error Open %s\n", ofn);
    }
    //linux
    params = "appid=57fe0d38-bd8a-11ed-b526-00163e13c8a2;secretkey=42ddc64a-"
             "1c61-39bc-9cfd-0aa8bdd1009c;cache_path=/tmp/out;use_srvsel=1;use_log=1;";
    s = qtk_session_init(params, 0, NULL, NULL);
    printf("session=%p\n", s);
//    params =
//        "role=aec;cfg=res/sdk/aec/cfg;use_bin=0;use_thread=0;";
    params =
        "role=aec;cfg=res/sdk/aec/aec.cfg.bin;use_bin=1;use_thread=0;";
    e = qtk_engine_new(s, params);
    if (!e) {
        printf("new engine failed.\n");
        goto end;
    }
    qtk_engine_set_notify(e, s, (qtk_engine_notify_f)test_aec_on_notify);
//    while(1)
//    {
    qtk_engine_start(e);
    st = time_get_ms();
    if (buf->pos > 0) {
        qtk_engine_feed(e, buf->data, buf->pos, 1);
    } else {
        qtk_engine_feed(e, data + 44, len - 44, 1);  //Note: end:1 get result, or no result.
    }
    t=(time_get_ms()-st)/1000.0;
    wav_t= (len-44) /16000.0/8.0;
    //...end for a time wave
    printf("run-time=%f(s) wav-dur= %f(s) rate= %f\n",t,wav_t, t/wav_t);
//    wtk_sem_acquire(&sem, -1);
    qtk_engine_reset(e);
//    }
    wtk_wavfile_close(wav);
    wtk_wavfile_delete(wav);

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
