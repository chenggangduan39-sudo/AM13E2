#include "qtk/cv/tracking/qtk_mot_sort.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/wtk_str_parser.h"

typedef struct {
    qtk_mot_sort_cfg_t cfg;
    qtk_mot_sort_t sort;
    wtk_strbuf_t *buf;
} tracking_t;

static void on_flist_item_(tracking_t *trk, char *str) {
    int i, ndet;
    wtk_string_spliter_t spliter;
    wtk_string_spliter_init_s(&spliter, str, strlen(str), ",");
    while (1) {
        qtk_cv_bbox_t bbox;
        wtk_string_spliter_next(&spliter);
        if (spliter.v.len == 0) {
            break;
        }
        bbox.x1 = wtk_str_atof(spliter.v.data, spliter.v.len);
        wtk_string_spliter_next(&spliter);
        bbox.y1 = wtk_str_atof(spliter.v.data, spliter.v.len);
        wtk_string_spliter_next(&spliter);
        bbox.x2 = wtk_str_atof(spliter.v.data, spliter.v.len);
        wtk_string_spliter_next(&spliter);
        bbox.y2 = wtk_str_atof(spliter.v.data, spliter.v.len);
        wtk_strbuf_push(trk->buf, (char *)&bbox, sizeof(qtk_cv_bbox_t));
    }
    ndet = trk->buf->pos / sizeof(qtk_cv_bbox_t);
    qtk_mot_sort_update(&trk->sort, ndet, (qtk_cv_bbox_t *)trk->buf->data);
    for (i = 0; i < ndet; i++) {
        printf("%d ", trk->sort.result[i]);
    }
    printf("\n");
    wtk_strbuf_reset(trk->buf);
}

int main(int argc, char *argv[]) {
    wtk_arg_t *arg = wtk_arg_new(argc, argv);
    char *fn = NULL;
    tracking_t trk;
    wtk_arg_get_str_s(arg, "f", &fn);
    qtk_mot_sort_cfg_init(&trk.cfg);
    qtk_mot_sort_init(&trk.sort, &trk.cfg);
    trk.buf = wtk_strbuf_new(1024, 1);
    if (fn) {
        wtk_flist_process(fn, &trk, (wtk_flist_notify_f)on_flist_item_);
    }
    wtk_strbuf_delete(trk.buf);
    qtk_mot_sort_clean(&trk.sort);
    wtk_arg_delete(arg);
}
