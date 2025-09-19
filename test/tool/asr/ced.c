#include "wtk/asr/ced/wtk_ced.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/wtk_riff.h"

#define string_print(s) printf("%.*s\t", (s)->len, (s)->data)
static int cur_index = 0;
void ced_notify(void *ths, wtk_ced_event_type_e event_type, int start,
                int end) {
    wtk_ced_t *ced = NULL;
    ced = (wtk_ced_t *)ths;
    if (-1 == end) {
        cur_index = ced->prediction_index;
    }
    if (-1 != end && cur_index) {
        string_print(ced->fn);
        printf("%d\t", cur_index);
        printf("%d\t%d\t", start, end);
        switch (event_type) {
        case WTK_CED_CRY:
            printf("crying\n");
            break;
        case WTK_CED_SHOUT:
            printf("shout\n");
            break;
        case WTK_CED_OTHER:
            printf("other\n");
            break;
        default:
            break;
        }
        cur_index = 0;
    }
}

static void print_usage(int argc, char **argv) {
    printf("Usage:\n"
           "\t: asr_ced -c ced.cfg -wav wav_fn/-scp scp_fn \n"
           "\t: asr_ced -b ced.bin -wav wav_fn/-scp scp_fn\n"
           "\t\t -c engine cfg file\n"
           "\t\t -b engine bin file\n"
           "\t\t -wav input wave file\n"
           "\t\t -scp input wave file list\n");
}

void feed_ced(wtk_ced_t *ced, char *wav_fn) {
    char buf[1024];
    int ret = 0;
    wtk_riff_t *riff = NULL;

    riff = wtk_riff_new();
    wtk_riff_open(riff, wav_fn);
    ced->fn = wtk_string_dup_data(wav_fn, strlen(wav_fn));
    do {
        ret = wtk_riff_read(riff, buf, 1024);
        if (ret > 0) {
            wtk_ced_feed(ced, buf, ret, 0);
        }
    } while (ret > 0);
    wtk_ced_feed(ced, NULL, 0, 1);

    wtk_string_delete(ced->fn);
    wtk_riff_close(riff);
    wtk_riff_delete(riff);
    wtk_ced_reset(ced);
}

void feed_scp(wtk_ced_t *ced, char *scp_fn) {
    wtk_flist_t *f = NULL;
    wtk_fitem_t *item = NULL;
    wtk_queue_node_t *qn = NULL;

    f = wtk_flist_new(scp_fn);

    for (qn = f->queue.pop; qn; qn = qn->next) {
        item = data_offset(qn, wtk_fitem_t, q_n);
        feed_ced(ced, item->str->data);
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
    wtk_ced_t *ced = NULL;
    wtk_ced_cfg_t *cfg = NULL;
    wtk_arg_t *arg = NULL;

    arg = wtk_arg_new(argc, argv);
    wtk_arg_get_str_s(arg, "c", &cfg_fn);
    wtk_arg_get_str_s(arg, "b", &bin_fn);
    wtk_arg_get_str_s(arg, "scp", &scp_fn);
    wtk_arg_get_str_s(arg, "wav", &wav_fn);

    if (!cfg_fn && !bin_fn) {
        print_usage(argc, argv);
        return 0;
    }

    if (!wav_fn && !scp_fn) {
        print_usage(argc, argv);
        return 0;
    }

    if (cfg_fn) {
        main_cfg = wtk_main_cfg_new_type(wtk_ced_cfg, cfg_fn);
        if (!main_cfg) {
            wtk_debug("load configure failed.\n");
            return 0;
        }
        cfg = (wtk_ced_cfg_t *)main_cfg->cfg;
        ced = wtk_ced_new(cfg);
        cfg->hook = main_cfg;
    } else if (bin_fn) {
        mbin_cfg = wtk_mbin_cfg_new_type(wtk_ced_cfg, bin_fn, "./ced.cfg");
        if (!mbin_cfg) {
            wtk_debug("load configure failed.\n");
            return 0;
        }
        cfg = (wtk_ced_cfg_t *)mbin_cfg->cfg;
        cfg->hook = mbin_cfg;
        ced = wtk_ced_new(cfg);
        wtk_rbin2_delete(mbin_cfg->rbin);
        mbin_cfg->rbin = NULL;
    }

    if (ced) {
        wtk_ced_set_notify(ced, (wtk_ced_notify_f)ced_notify, ced);
    }
    if (wav_fn) {
        feed_ced(ced, wav_fn);
    } else if (scp_fn) {
        feed_scp(ced, scp_fn);
    }

    if (ced) {
        wtk_ced_delete(ced);
    }
    if (main_cfg) {
        wtk_main_cfg_delete(main_cfg);
    } else if (mbin_cfg) {
        wtk_mbin_cfg_delete(mbin_cfg);
    }
    if (arg) {
        wtk_arg_delete(arg);
    }

    return 0;
}
