#include "sdk/qtk_api.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/wtk_os.h"
#include "wtk/os/wtk_sem.h"

double st, t, wav_t;
void test_wdec_on_notify(qtk_session_t *session, qtk_var_t *var) {
    if(var->type==QTK_AEC_WAKE)
    {
	    printf("wdec wake\n");
        printf("wake_prob=%f\n",var->v.fi.theta);
    }
}

int main(int argc, char *argv[]) {
    qtk_session_t *s = NULL;
    qtk_engine_t *e = NULL;
    wtk_arg_t *arg = NULL;
    char *params;
    char *data = NULL;
    char *wav_fn;
    int len;
    int ret;
    int channel;

    arg = wtk_arg_new(argc, argv);
    ret = wtk_arg_get_str_s(arg, "i", &wav_fn);
    if (ret != 0) {
        printf("usage:  ./wdec -i xxx.wav\n");
        goto end;
    }
    ret = wtk_arg_get_str_s(arg, "ch", &channel);
    if (ret != 0) {
        printf("usage:  ./wdec -ch channel\n");
        goto end;
    }
    data = file_read_buf(wav_fn, &len);
    if (len <= 0) {
        printf("not find wav\n");
        goto end;
    }

    params = "appid=57fe0d38-bd8a-11ed-b526-00163e13c8a2;secretkey=42ddc64a-"
             "1c61-39bc-9cfd-0aa8bdd1009c;cache_path=/tmp/out;use_srvsel=1;use_log=1;";
    s = qtk_session_init(params, 0, NULL, NULL);

    params ="role=wdec;cfg=/home/zzr/jx/git/engine.cfg;use_bin=0;use_thread=0;";
    e = qtk_engine_new(s, params);
    if (!e) {
        printf("new engine failed.\n");
        goto end;
    }
    qtk_engine_set(e,"天猫精灵|小爱同学");
    qtk_engine_set_notify(e, s, (qtk_engine_notify_f)test_wdec_on_notify);

    qtk_engine_start(e);
    st = time_get_ms();

    int pos,step,slen;
	pos = 44;
	step = 32*channel*16*2;
	while(pos<len){
		slen = min(len-pos,step);
		qtk_engine_feed(e,data+pos,slen,0);
		// usleep(20*1000);
		pos += slen;
	}
	qtk_engine_feed(e,NULL,0,1);
    
    t=(time_get_ms()-st)/1000.0;
    wav_t= (len-44) /16000.0;

    printf("run-time=%f(s) wav-dur= %f(s) rate= %f\n",t,wav_t, t/wav_t);

    qtk_engine_reset(e);

end:
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
    return 0;
}
