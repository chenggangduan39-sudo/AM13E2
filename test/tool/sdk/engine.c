#include "sdk/qtk_api.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/wtk_os.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/os/wtk_sem.h"
#include "wtk/core/wtk_wavfile.h"
#include <stdio.h>
wtk_sem_t sem;
char *output = NULL;
//static FILE *outf = NULL;
wtk_wavfile_t *owav;
void test_easr_on_notify(qtk_session_t *session, qtk_var_t *var) {
    switch (var->type) {
    case QTK_ASR_TEXT:
        printf("result: %.*s\n", var->v.str.len, var->v.str.data);
        wtk_sem_release(&sem, 1);
        break;
    case QTK_VAR_ERR:
        wtk_sem_release(&sem, 1);
        break;
    case QTK_SPEECH_DATA_PCM:
        if(var->v.str.len>0)
        {
        	//printf("=======len: %d\n", var->v.str.len);
        	wtk_wavfile_write(owav,var->v.str.data,var->v.str.len);
        }
//        fwrite(var->v.str.data, var->v.str.len, 1, outf);
        break;
    default:
        break;
    }
}

static void print_usage() {
    printf("usage: \n");
    printf("        -i input wav\n");
    printf("        -o output wav\n");
    printf("        -p params\n");
    printf("pl:\n"
    		"\tsdk_engine -i input.wav -o output.wav -p \"role=tinybf;cfg=res/sdk/project/yuanzhoulv.cfg;use_bin=0;use_thread=0;\"\n");
}

int main(int argc, char *argv[]) {
    qtk_session_t *s = NULL;
    qtk_engine_t *e = NULL;
    wtk_arg_t *arg = NULL;
    wtk_strbuf_t *buf = NULL;
    char *params;
    char *data=NULL;
    char *wav_fn;
    char *cfg = NULL;
    wtk_riff_t *riff = NULL;
    int len,bytes;

    int ret;

    wtk_sem_init(&sem, 0);
    buf = wtk_strbuf_new(3200, 1);
    arg = wtk_arg_new(argc, argv);
    ret = wtk_arg_get_str_s(arg, "i", &wav_fn);
    if (ret != 0) {
        print_usage();
        goto end;
    }
    ret = wtk_arg_get_str_s(arg, "p", &cfg);
    if (ret != 0) {
        print_usage();
        goto end;
    }
    ret = wtk_arg_get_str_s(arg, "o", &output);
    if (ret != 0) {
        print_usage();
        goto end;
    }
    riff = wtk_riff_new();
    wtk_riff_open(riff, wav_fn);

    params = "appid=57fe0d38-bd8a-11ed-b526-00163e13c8a2;secretkey=42ddc64a-"
             "1c61-39bc-9cfd-0aa8bdd1009c;cache_path=/tmp/out/"
             "vboxebf;use_srvsel=1;use_log=1;";
    s = qtk_session_init(params, 0, NULL, NULL);

    //example
//    params =
//        "role=asr;cfg=res/sdk/asr/hdgd.cfg;use_bin=0;use_thread=0;";

    params = cfg;

    e = qtk_engine_new(s, params);
    if (!e) {
        printf("new engine failed.\n");
        goto end;
    }
    qtk_engine_set_notify(e, s, (qtk_engine_notify_f)test_easr_on_notify);
    qtk_engine_start(e);

	owav=wtk_wavfile_new(48000);
	wtk_wavfile_set_channel(owav, 2);
    owav->max_pend=0;
    wtk_wavfile_open(owav,output);

    len=32*48000/1000;
    bytes=sizeof(short)*2*len;
    data = (char *)wtk_malloc(bytes * sizeof(char));
    while (!feof(riff->file)) {
        ret = wtk_riff_read(riff, data, bytes);
        if (ret <= 0)
            break;
        qtk_engine_feed(e, data, ret, 0);
    }
    qtk_engine_feed(e, data, 0, 1);
    qtk_engine_reset(e);
    wtk_riff_close(riff);
    if (owav) {
    	wtk_wavfile_delete(owav);
    	owav=NULL;
//    	fclose(outf);
    }

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
    if (riff) {
        wtk_riff_delete(riff);
    }
    return 0;
}
