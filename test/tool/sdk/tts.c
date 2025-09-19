#include "sdk/qtk_api.h"
#include "wtk/core/wtk_os.h"
#include "wtk/os/wtk_sem.h"
wtk_sem_t sem;
extern void* wtk_wavfile_new(int sample_rate);
extern int wtk_wavfile_open(void *f,char *fn);
extern int wtk_wavfile_write(void *f,const char *data,int bytes);
extern int wtk_wavfile_close(void *f);
extern int wtk_wavfile_delete(void *f);
extern void* wtk_flist_it_new(char *fn);
extern char* wtk_flist_it_next(void *it);
extern void wtk_flist_it_delete(void *it);
extern void* wtk_arg_new(int argc,char** argv);
extern int wtk_arg_get_str(void *arg,const char *key,int bytes,char** pv);
extern int wtk_arg_get_int(void *arg,const char *key,int bytes,int* number);
extern int wtk_arg_get_float(void *arg,const char *key,int bytes,float* number);
extern int wtk_arg_delete(void *arg);
extern int wtk_arg_exist(void *arg,const char* key,int bytes);
extern double time_get_ms();

#define wtk_arg_get_str_s(arg,k,pv) wtk_arg_get_str(arg,(char*)k,sizeof(k)-1,pv)
#define wtk_arg_get_int_s(arg,k,n) wtk_arg_get_int(arg,k,sizeof(k)-1,n)
#define wtk_arg_get_float_s(arg,k,n) wtk_arg_get_float(arg,k,sizeof(k)-1,n)

double st, t, wav_t, dt=0, twav_t=0;
static void*wav;
static void test_etts_on_notify(qtk_session_t *session, qtk_var_t *var) {
	//v.v.str.data
	int len;
	char*data;

	if (var->type != QTK_TTS_DATA) return;
	data = var->v.str.data;
	len = var->v.str.len;
	printf("data: %p len=%d\n", data, len);
    if (len==0)
    {
            if (len==0)
            {
                    printf("end\n");
                    return;
            }
            t=(time_get_ms()-st)/1000.0;
            wav_t= len /16000.0;
            dt=t-twav_t;
            if (twav_t > 0 && dt < 0)
                    dt=0;
            //...end for a time wave
            printf("run-time=%f(s) wav-dur= %f(s) delay-time= %f(s) rate= %f\n",t,wav_t, dt, t/wav_t);
    }else
    {
            t=(time_get_ms()-st)/1000.0;
            wav_t=len /16000.0;
            if (twav_t < t)
                    printf("run-time=%f(s) wav-dur= %f(s) delay-time= %f(s) rate= %f\n", t, wav_t, t-twav_t, t/wav_t);
            else
                    printf("run-time=%f(s) wav-dur= %f(s) delay-time= 0.00(s) rate= %f\n", t, wav_t, t/wav_t);
            twav_t += wav_t;
    }
    st = time_get_ms();
    if (wav && len > 0)
            wtk_wavfile_write(wav, data, len);
}

int main(int argc, char *argv[]) {
    qtk_session_t *s = NULL;
    qtk_engine_t *e = NULL;
    void *arg = NULL;
    char *params, *setparams;
    char *data = NULL;
    char *ofn;
    int ret;

//    wtk_sem_init(&sem, 0);
    arg = wtk_arg_new(argc, argv);
    ret = 0;
    data = "奇梦者科技有限公司";
    ofn = "out/tts.wav";
    params = "appid=57fe0d38-bd8a-11ed-b526-00163e13c8a2;secretkey=42ddc64a-"
             "1c61-39bc-9cfd-0aa8bdd1009c;cache_path=/tmp;use_srvsel=1;use_log=1;";
    s = qtk_session_init(params, 0, NULL, NULL);
    params =
        "role=tts;cfg=res/sdk/tts/hts/cfg;use_bin=0;use_thread=0;";
    e = qtk_engine_new(s, params);

    //set param
//    setparams = "tts_speed=2.0;tts_volume=1.0;tts_pitch=1.0;";
//    qtk_engine_set(e, setparams);

    if (!e) {
        printf("new engine failed.\n");
        goto end;
    }
    qtk_engine_set_notify(e, s, (qtk_engine_notify_f)test_etts_on_notify);
    wav = wtk_wavfile_new(16000);
    if (wtk_wavfile_open(wav,ofn)) {
        printf("Error Open %s\n", ofn);
    }
//    while(1)
//    {
    qtk_engine_start(e);
    qtk_engine_feed(e, data, strlen(data), 1);

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
    return 0;
}
