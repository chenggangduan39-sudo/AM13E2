#include "qtk/qtk_global.h"
#include "qtk/ult/qtk_ult_track.h"
#include "qtk/ult/qtk_ult_track_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/core/wtk_type.h"
#include "qtk/ult/qtk_ult_track_post.h"

static int on_track_(qtk_ult_track_post_t *post, int nframe, int nobj,
                     qtk_ult_track_result_t *res) {
    for (int i = 0; i < nobj; i++) {
        printf("%d:%f,%f,%f\n", nframe, res[i].r, res[i].theta, res[i].y_theta);
    }
    if (nobj == 1) {
        qtk_ult_track_post_feed(post, res);
    }
    return 0;
}

static void test_wav_(qtk_ult_track_t *ges, char *wav, char *channel_desc) {
    int channel;
    wtk_strbuf_t **audio = wtk_riff_read_channel(wav, &channel);
    int len = audio[0]->pos / 2;
    int step = 4096;
    int feed_pos = 0;
    int chn_a[1024];
    int nchn, i = 0;
    char *cursor, *sep;
    cursor = channel_desc;
    while ((sep = strchr(cursor, ','))) {
        *sep = '\0';
        chn_a[i++] = atoi(cursor);
        cursor = sep + 1;
    }
    chn_a[i++] = atoi(cursor);
    nchn = i;
    feed_pos = 17190;
    feed_pos = 0;
    while (feed_pos < len) {
        short *_f[1024];
        int feed_cnt = min(step, len - feed_pos);
        for (i = 0; i < nchn; i++) {
            _f[i] = (short *)(audio[chn_a[i]]->data) + feed_pos;
        }
        qtk_ult_track_feed(ges, _f, feed_cnt);
        feed_pos += feed_cnt;
    }
    qtk_ult_track_feed_end(ges);
    wtk_strbufs_delete(audio, channel);
}

int main(int argc, char *argv[]) {
    wtk_arg_t *arg = wtk_arg_new(argc, argv);
    wtk_main_cfg_t *main_cfg;
    qtk_ult_track_t *ges;
    qtk_ult_track_post_t *post;

    qtk_global_init();

    char *wav = NULL;
    char *cfg = NULL;
    char *channel = NULL;
    wtk_arg_get_str_s(arg, "c", &cfg);
    wtk_arg_get_str_s(arg, "i", &wav);
    wtk_arg_get_str_s(arg, "chn", &channel);
    char *channel_dup = strdup(channel);

    qtk_ult_track_post_cfg_t post_cfg;
    qtk_ult_track_post_cfg_init(&post_cfg);
    post = qtk_ult_track_post_new(&post_cfg);

#if 1
    main_cfg = wtk_main_cfg_new_type(qtk_ult_track_cfg, cfg);
    ges = qtk_ult_track_new((qtk_ult_track_cfg_t *)main_cfg->cfg);
    qtk_ult_track_set_notifier(ges, post,
                               (qtk_ult_track_notifier_t)on_track_);
    test_wav_(ges, wav, channel);
#endif

    wtk_arg_delete(arg);
    qtk_ult_track_delete(ges);
    wtk_main_cfg_delete(main_cfg);
    qtk_ult_track_post_delete(post);

    free(channel_dup);
    qtk_global_clean();
    return 0;
}
