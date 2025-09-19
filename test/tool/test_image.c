#include "wtk/core/wtk_arg.h"
#include "wtk/core/wtk_riff.h"
#include "qtk/image/qtk_image.h"
#include "wtk/core/wtk_os.h"

static void do_(const char *dfn, const char *ofn, int N) {
    char path[1024];
    uint8_t *outtmp;
    qtk_image_desc_t desc;
    qtk_image_desc_t outdesc;
    double tm=0.0;

    outtmp = malloc(sizeof(uint8_t) * 3 * 2160 * 3840);
    snprintf(path, sizeof(path), "%s/audio.wav", dfn);
    desc.channel = 3;
    desc.fmt = QBL_IMAGE_RGB24;
    desc.height = 1080;
    desc.width = 1920;

    outdesc.channel = 3;
    outdesc.height = 960;
    outdesc.width = 540;
    outdesc.fmt = QBL_IMAGE_RGB24;

    for (int i = 0; i < N; i++) {
        snprintf(path, sizeof(path), "%s/%05d.jpeg", dfn, i + 1);
        wtk_debug("===================>>>>>>>>>>>>path=%s\n",path);
        uint8_t *I = qtk_image_load(&desc, path);
        wtk_debug("=========>>>>>>>channel=%d height=%d width=%d ilen=%d olen=%d ipoint=%p opoint=%p\n",desc.channel,desc.height,desc.width, 3*1080*1029, 3*2160*3840, I, outtmp);

        tm = time_get_ms();

        qtk_image_resize(&desc, I, 960, 540, outtmp);
        wtk_debug("==================>>>>>>>>>>>>>>>>resize time=%f\n",time_get_ms() - tm);

        qtk_image_save_png(ofn, outtmp, &outdesc);

        wtk_free(I);
    }
    free(outtmp);
}

int main(int argc, char *argv[]) {
    wtk_arg_t *arg = wtk_arg_new(argc, argv);
    char *dfn;
    char *ofn;
    int N;

    wtk_arg_get_str_s(arg, "d", &dfn);
    wtk_arg_get_str_s(arg, "o", &ofn);
    wtk_arg_get_int_s(arg, "n", &N);

    do_(dfn, ofn, N);
    wtk_arg_delete(arg);
}
