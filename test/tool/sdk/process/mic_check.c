#include "wtk/bfio/consist/wtk_mic_check.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_opt.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/wtk_riff.h"

void print_usage() {
    printf("mic_check usage:\n");
    printf("\t-c configure file\n");
    printf("\t-b bin configure file\n");
    printf("\t-i input wav  file\n");
    printf("\n\n");
}

static int repeat = 0;

static void test_mic_check_on(void *ths, wtk_mic_check_err_type_t *type, int chn) {
    int i;
    for (i = 0; i < chn; ++i) {
        printf("%d:%d\n", i, type[i]);
    }
}

static void mic_check_test_file(wtk_mic_check_t *mic_check, char *ifn) {
    wtk_riff_t *riff;
    short *pv;
    int channel = mic_check->cfg->channel;
    int len, bytes;
    int ret;
    double t;
    int cnt;
    // wtk_strbuf_t **input;
    // int n;
    // short *pv[10];
    // int i;

    riff = wtk_riff_new();
    wtk_riff_open(riff, ifn);

    len = 32 * 16;
    bytes = sizeof(short) * channel * len;
    pv = (short *)wtk_malloc(sizeof(short) * channel * len);

    wtk_mic_check_start(mic_check);
    wtk_mic_check_set_notify(mic_check, NULL,
                             (wtk_mic_check_notify_f)test_mic_check_on);

    float *play_vol = (float *)wtk_malloc(sizeof(float) * mic_check->cfg->nmicchannel);
    for (int i = 0; i < mic_check->cfg->nmicchannel; ++i) {
        play_vol[i] = 1.0;
    }
    
    cnt = 0;
    t = time_get_ms();
    if (repeat > 0) {
        while (repeat--) {
            wtk_riff_open(riff, ifn);
            while (1) {
                ret = wtk_riff_read(riff, (char *)pv, bytes);
                if (ret <= 0) {
                    // wtk_debug("break ret=%d\n",ret);
                    break;
                }
                // wtk_msleep(64);
                len = ret / (sizeof(short) * channel);
                cnt += len;
                wtk_mic_check_feed(mic_check, pv, len, play_vol, 0);
                // wtk_debug("deley %f\n",(cnt-out_len)/16.0);
            }
        }
    } else {
        while (1) {
            ret = wtk_riff_read(riff, (char *)pv, bytes);
            if (ret <= 0) {
                // wtk_debug("break ret=%d\n", ret);
                break;
            }
            // wtk_msleep(64);
            len = ret / (sizeof(short) * channel);
            cnt += len;
            wtk_mic_check_feed(mic_check, pv, len, play_vol, 0);
            // wtk_debug("deley %f\n",(cnt-out_len)/16.0);
        }
    }
    wtk_mic_check_feed(mic_check, NULL, 0, NULL, 1);

    t = time_get_ms() - t;
    // wtk_debug("rate=%f t=%f\n", t / (cnt / (mic_check->cfg->rate / 1000.0)), t);

    wtk_riff_delete(riff);
    wtk_free(pv);
    wtk_free(play_vol);
}

int main(int argc, char **argv) {
    wtk_mic_check_t *mic_check = NULL;
    wtk_mic_check_cfg_t *cfg = NULL;
    wtk_arg_t *arg;
    char *cfg_fn = NULL;
    char *bin_fn = NULL;
    char *scp = NULL;
    char *wav = NULL;

    arg = wtk_arg_new(argc, argv);
    if (!arg) {
        goto end;
    }
    wtk_arg_get_str_s(arg, "c", &cfg_fn);
    wtk_arg_get_str_s(arg, "b", &bin_fn);
    wtk_arg_get_str_s(arg, "i", &wav);
    wtk_arg_get_int_s(arg, "r", &repeat);
    if ((!cfg_fn && !bin_fn) || (!scp && !wav)) {
        print_usage();
        goto end;
    }
    if (cfg_fn) {
        cfg = wtk_mic_check_cfg_new(cfg_fn);
        if (cfg == NULL) {
            print_usage();
            goto end;
        }
    } else if (bin_fn) {
        cfg = wtk_mic_check_cfg_new_bin(bin_fn);
        if (cfg == NULL) {
            print_usage();
            goto end;
        }
    }

    mic_check = wtk_mic_check_new(cfg);
    if (wav) {
        mic_check_test_file(mic_check, wav);
    }
end:
    if (mic_check) {
        wtk_mic_check_delete(mic_check);
    }
    if (cfg) {
        if (cfg_fn) {
            wtk_mic_check_cfg_delete(cfg);
        } else {
            wtk_mic_check_cfg_delete_bin(cfg);
        }
    }
    if (arg) {
        wtk_arg_delete(arg);
    }
    return 0;
}
