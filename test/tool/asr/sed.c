#include "wtk/asr/sed/wtk_sed.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/wtk_riff.h"

#define string_print(s) printf("%.*s\t", (s)->len, (s)->data)

void sed_notify(void *ths, int type, float start, float end) {
    wtk_sed_t *sed = NULL;
    sed = (wtk_sed_t *)ths;
    if (type != -1) {
        string_print(sed->fn);
        printf("%.2f\t%.2f\t", start, end);
        switch (type) {
        case 0:
            printf("crying\n");
            break;
        case 1:
            printf("shout\n");
            break;
        default:
            break;
        }
    }
}

static void print_usage(int argc, char **argv) {
    printf("Usage:\n"
           "\t: ./asr_sed -c res/cfg/asr/sed/sed.cfg -wav wav_fn/-scp scp_fn \n"
           "\t: ./asr_sed -b sed.bin -wav wav_fn/-scp scp_fn\n"
           "\t\t -c engine cfg file\n"
           "\t\t -b engine bin file\n"
           "\t\t -wav input wave file\n"
           "\t\t -scp input wave file list\n");
}

void feed_sed(wtk_sed_t *sed, char *wav_fn) {
    char buf[1024];
    int ret = 0;
    wtk_riff_t *riff = NULL;

    riff = wtk_riff_new();
    wtk_riff_open(riff, wav_fn);
    sed->fn = wtk_string_dup_data(wav_fn, strlen(wav_fn));
    do {
        ret = wtk_riff_read(riff, buf, 1024);
        if (ret > 0) {
            wtk_sed_feed(sed, buf, ret, 0);
        }
    } while (ret > 0);
    wtk_sed_feed(sed, NULL, 0, 1);

    wtk_string_delete(sed->fn);
    wtk_riff_close(riff);
    wtk_riff_delete(riff);
    wtk_sed_reset(sed);
}

void feed_scp(wtk_sed_t *sed, char *scp_fn) {
    wtk_flist_t *f = NULL;
    wtk_fitem_t *item = NULL;
    wtk_queue_node_t *qn = NULL;

    f = wtk_flist_new(scp_fn);

    for (qn = f->queue.pop; qn; qn = qn->next) {
        item = data_offset(qn, wtk_fitem_t, q_n);
        feed_sed(sed, item->str->data);
    }

    if (f) {
        wtk_flist_delete(f);
    }
}

int main(int argc, char **argv) {
    char *cfg_fn = NULL;
    char *scp_fn = NULL;
    char *wav_fn = NULL;
    char *bin_fn = NULL;

    wtk_main_cfg_t *main_cfg = NULL;
    wtk_mbin_cfg_t *mbin_cfg = NULL;
    wtk_sed_t *sed = NULL;
    wtk_sed_cfg_t *cfg = NULL;
    wtk_arg_t *arg = NULL;

    int cry_smooth = 0;
    int cry_salt = 0;
    int shout_smooth = 0;
    int shout_salt = 0;
    float cry_h = 0.0;
    float cry_l = 0.0;
    float shout_h = 0.0;
    float shout_l = 0.0;

    arg = wtk_arg_new(argc, argv);
    wtk_arg_get_str_s(arg, "c", &cfg_fn);
    wtk_arg_get_str_s(arg, "b", &bin_fn);
    wtk_arg_get_str_s(arg, "scp", &scp_fn);
    wtk_arg_get_str_s(arg, "wav", &wav_fn);

    wtk_arg_get_int_s(arg, "cry_smooth", &cry_smooth);
    wtk_arg_get_int_s(arg, "cry_salt", &cry_salt);
    wtk_arg_get_int_s(arg, "shout_smooth", &shout_smooth);
    wtk_arg_get_int_s(arg, "shout_salt", &shout_salt);
    wtk_arg_get_float_s(arg, "cry_h", &cry_h);
    wtk_arg_get_float_s(arg, "cry_l", &cry_l);
    wtk_arg_get_float_s(arg, "shout_h", &shout_h);
    wtk_arg_get_float_s(arg, "shout_l", &shout_l);

    if (!cfg_fn && !bin_fn) {
        print_usage(argc, argv);
        return 0;
    }

    if (!wav_fn && !scp_fn) {
        print_usage(argc, argv);
        return 0;
    }

    if (cfg_fn) {
        main_cfg = wtk_main_cfg_new_type(wtk_sed_cfg, cfg_fn);
        if (!main_cfg) {
            wtk_debug("load configure failed.\n");
            return 0;
        }
        cfg = (wtk_sed_cfg_t *)main_cfg->cfg;
        sed = wtk_sed_new(cfg);
        cfg->hook = main_cfg;
    } else if (bin_fn) {
        mbin_cfg = wtk_mbin_cfg_new_type(wtk_sed_cfg, bin_fn, "./sed.cfg");
        if (!mbin_cfg) {
            wtk_debug("load configure failed.\n");
            return 0;
        }
        cfg = (wtk_sed_cfg_t *)mbin_cfg->cfg;
        sed = wtk_sed_new(cfg);
        cfg->hook = mbin_cfg;
    }

    // wtk_sed_set_para(sed, cry_smooth, cry_salt, cry_h, cry_l, shout_smooth,
    //                  shout_salt, shout_h, shout_l);

    if (sed) {
        wtk_sed_set_notify(sed, (wtk_sed_notify_f)sed_notify, sed);
    }
    if (wav_fn) {
        feed_sed(sed, wav_fn);
    } else if (scp_fn) {
        feed_scp(sed, scp_fn);
    }

    if (sed) {
        wtk_sed_delete(sed);
    }
    if (main_cfg) {
        wtk_main_cfg_delete(main_cfg);
    }
    if (arg) {
        wtk_arg_delete(arg);
    }

    return 0;
}
