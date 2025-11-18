#include "wtk/bfio/qtk_fixed_angle_wasr.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/json/wtk_json_parse.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/wtk_riff.h"

static int _on_wasr(wtk_json_parser_t *jsp, qtk_wasr_res_t *res) {
    wtk_json_item_t *conf, *rec;

    switch (res->t) {
    case QTK_WASR_REC:
        wtk_json_parser_reset(jsp);
        wtk_json_parser_parse(jsp, cast(char *, res->v.rec.res),
                              res->v.rec.len);
        conf = wtk_json_obj_get_s(jsp->json->main, "conf");
        rec = wtk_json_obj_get_s(jsp->json->main, "rec");
        printf("[asr] %f %f %.*s/%f\n", res->fs, res->fe, rec->v.str->len,
               rec->v.str->data, conf->v.number);
        break;
    case QTK_WASR_WAKEUP:
        printf("[wake] %f %f\n", res->fs, res->fe);
        break;
    }
    return 0;
}

static void _test(qtk_fixed_angle_wasr_t *w, char *wav) {
    wtk_riff_t *riff;
    int c, nchan;
    wtk_strbuf_t **audio;
    short **dp;
    int i, len;

    riff = wtk_riff_new();
    audio = wtk_riff_read_channel(wav, &nchan);
    dp = cast(short **, wtk_malloc(sizeof(short *) * nchan));

    len = audio[0]->pos / 2;

    for (i = 0; i < nchan; i++) {
        dp[i] = cast(short *, audio[i]->data);
    }

    qtk_fixed_angle_wasr_feed(w, dp, len);
    qtk_fixed_angle_wasr_feed_end(w);

    for (c = 0; c < nchan; c++) {
        wtk_strbuf_delete(audio[c]);
    }
    wtk_free(audio);
    wtk_free(dp);
    wtk_riff_delete(riff);
}

int main(int argc, char *argv[]) {
    wtk_main_cfg_t *main_cfg;
    wtk_arg_t *arg;
    qtk_fixed_angle_wasr_t *w;
    char *cfn;
    char *wav;
    wtk_json_parser_t *jsp = NULL;

    arg = wtk_arg_new(argc, argv);

    jsp = wtk_json_parser_new();
    wtk_arg_get_str_s(arg, "c", &cfn);
    wtk_arg_get_str_s(arg, "i", &wav);

    main_cfg = wtk_main_cfg_new_type(qtk_fixed_angle_wasr_cfg, cfn);

    w = qtk_fixed_angle_wasr_new(
        cast(qtk_fixed_angle_wasr_cfg_t *, main_cfg->cfg));
    qtk_fixed_angle_wasr_set_wasr_notify(w, cast(qtk_wasr_notify_t, _on_wasr),
                                         jsp);
    _test(w, wav);
    qtk_fixed_angle_wasr_delete(w);
    wtk_main_cfg_delete(main_cfg);
    wtk_arg_delete(arg);
    wtk_json_parser_delete(jsp);

    return 0;
}
