#include "qtk/avspeech/qtk_avspeech_separator.h"
#include "qtk/image/qtk_image.h"

typedef struct {
    qtk_avspeech_separator_t *sep;
    FILE *f;
} ctx_t;

static int on_sep_(ctx_t *ctx, int id, uint32_t frame_idx,
                   qtk_avspeech_separator_result_t *result) {
    if (result->type == QTK_AVSPEECH_SEPARATOR_AUDIO) {
        fwrite(result->audio.wav, sizeof(short), result->audio.len, ctx->f);
    }

    switch (result->type)
    {
    case QTK_AVSPEECH_SEPARATOR_AVATOR:
        wtk_debug("id=%d frame_idx=%d width=%d height=%d\n", id, frame_idx, result->avator.width, result->avator.height);
        break;
    case QTK_AVSPEECH_SEPARATOR_AUDIO:
        break;
    case QTK_AVSPEECH_SEPARATOR_FACE_ROI:
        wtk_debug("id=%d frame_idx=%d x1=%f y1=%f x2=%f y2=%f\n", id, frame_idx, result->face_roi.x1, result->face_roi.y1, result->face_roi.x2, result->face_roi.y2);
        break;
    case QTK_AVSPEECH_SEPARATOR_NOPERSON:
        wtk_debug("id=%d frame_idx=%d\n",id,frame_idx);
        break;    
    default:
        break;
    }

    return 0;
}

static void do_(qtk_avspeech_separator_t *sep, const char *dfn, int N) {
    char path[1024];
    short audio_seg[40 * 16*4];
    wtk_riff_t *riff = wtk_riff_new();
    snprintf(path, sizeof(path), "%s/audio.wav", dfn);
    qtk_image_desc_t desc;
    wtk_riff_open(riff, path); 
    // uint8_t *I = qtk_image_load(&desc, "test_data/video/00001.jpeg"); // 0000000000000000
    for (int i = 0; i < N; i++) {
        int ret = wtk_riff_read(riff, (char *)audio_seg, sizeof(audio_seg));
        if (ret > 0) {
            qtk_avspeech_separator_feed_audio(sep, audio_seg, ret / 2/ 4);
        }
        snprintf(path, sizeof(path), "%s/video/%05d.jpeg", dfn, i + 1);  // 111111111111111
        uint8_t *I = qtk_image_load(&desc, path); // 111111111111
        qtk_avspeech_separator_feed_image(sep, I);
        wtk_free(I); // 1111111111111111111
        usleep(40 * 1000);
    }
    // wtk_free(I); // 00000000000000000
    wtk_riff_delete(riff);
}

int main(int argc, char *argv[]) {
    wtk_arg_t *arg = wtk_arg_new(argc, argv);
    char *cfn;
    char *dfn;
    char *ofn;
    wtk_main_cfg_t *main_cfg;
    ctx_t ctx;
    int N;

    wtk_arg_get_str_s(arg, "c", &cfn);
    wtk_arg_get_str_s(arg, "d", &dfn);
    wtk_arg_get_str_s(arg, "o", &ofn);
    wtk_arg_get_int_s(arg, "n", &N);

    qtk_nnrt_set_global_thread_pool(4, NULL);
    main_cfg = wtk_main_cfg_new_type(qtk_avspeech_separator_cfg, cfn);
    ctx.sep = qtk_avspeech_separator_new(
        (qtk_avspeech_separator_cfg_t *)main_cfg->cfg, &ctx,
        (qtk_avspeech_separator_notifier_t)on_sep_);
    ctx.f = fopen(ofn, "wb");
    do_(ctx.sep, dfn, N);
    qtk_avspeech_separator_delete(ctx.sep);
    fclose(ctx.f);
    wtk_main_cfg_delete(main_cfg);
    wtk_arg_delete(arg);
}
