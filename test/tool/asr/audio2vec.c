#include "qbl/os/qbl_file.h"
#include "qbl/serde/qbl_kaldiio.h"
#include "wtk/asr/kws/qtk_kws.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/wtk_type.h"

static void on_kws(void *ths, int res, char *name, int name_len) {}

struct Upval {
    qtk_kws_t *dec;
    FILE *out;
};

static int on_kaldiio_(struct Upval *upval, qbl_kaldiio_elem_t *elem) {
    wtk_riff_t *riff;
    wtk_vecf_t *embedding;
    char buf[4096];
    int read_cnt;
    int tot_cnt = 0, repeated = 0;
    int min_cnt = 16000;
    qbl_kaldiio_elem_t emb_elem;

    emb_elem.fmt = QBL_KALDIIO_FV;
    riff = wtk_riff_new();
    qtk_kws_enroll(upval->dec, elem->token.data, elem->token.len);
    for (repeated = 0; 1; repeated++) {
        wtk_riff_open(riff, elem->xfilename.data);
        while (1) {
            read_cnt = wtk_riff_read(riff, buf, sizeof(buf));
            if (read_cnt < sizeof(buf)) {
                if (read_cnt > 0) {
                    tot_cnt += read_cnt;
                    qtk_kws_feed(upval->dec, buf, read_cnt, 0);
                }
                break;
            }
            tot_cnt += read_cnt;
            qtk_kws_feed(upval->dec, buf, read_cnt, 0);
            if (repeated > 0 && tot_cnt > min_cnt) {
                goto finish;
            }
        }
        if (tot_cnt > min_cnt) {
            goto finish;
        }
    }
finish:
    qtk_kws_feed(upval->dec, NULL, 0, 1);
    embedding = upval->dec->svprint->x->spk_mean;
    emb_elem.fv.len = embedding->len;
    emb_elem.fv.data = embedding->p;
    emb_elem.token = elem->token;
    qbl_kaldiio_save_ark(NULL, &emb_elem, 1, (qbl_io_writer)qbl_file_write,
                         upval->out);
    wtk_vecf_zero(embedding);
    qtk_kws_reset(upval->dec);
    wtk_riff_delete(riff);
    return 0;
}

static void do_(qtk_kws_t *dec, const char *scp_fn, const char *out_ark) {
    qbl_kaldiio_t ki;
    struct Upval upval;
    FILE *out_fp;
    FILE *scp_fp;
    out_fp = fopen(out_ark, "wb");
    scp_fp = fopen(scp_fn, "rt");
    upval.dec = dec;
    upval.out = out_fp;
    qbl_kaldiio_init(&ki, (int (*)(void *, qbl_kaldiio_elem_t *))on_kaldiio_,
                     &upval);
    qbl_kaldiio_load_scp(&ki, (qbl_io_reader)qbl_file_read, scp_fp);
    qbl_kaldiio_clean(&ki);
    fclose(out_fp);
    fclose(scp_fp);
}

int main(int argc, char *argv[]) {
    wtk_main_cfg_t *main_cfg = 0;
    wtk_arg_t *arg;
    qtk_kws_cfg_t *cfg = NULL;
    qtk_kws_t *dec = 0;
    char *cfg_fn, *scp_fn, *out_ark;

    arg = wtk_arg_new(argc, argv);
    wtk_arg_get_str_s(arg, "c", &cfg_fn);
    wtk_arg_get_str_s(arg, "scp", &scp_fn);
    wtk_arg_get_str_s(arg, "ark", &out_ark);

    main_cfg = wtk_main_cfg_new_type(qtk_kws_cfg, cfg_fn);
    if (!main_cfg) {
        wtk_debug("load configure failed.\n");
        return -1;
    }
    cfg = (qtk_kws_cfg_t *)main_cfg->cfg;

    dec = qtk_kws_new(cfg);
    if (!dec) {
        wtk_debug("create decoder failed.\n");
        return -1;
    }
    qtk_kws_set_notify(dec, (qtk_kws_res_notify_f)on_kws, NULL);
    do_(dec, scp_fn, out_ark);
    wtk_arg_delete(arg);
    qtk_kws_delete(dec);
    wtk_main_cfg_delete(main_cfg);
}
