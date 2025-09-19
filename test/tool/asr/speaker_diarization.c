#include "qbl/core/qbl_io.h"
#include "qbl/os/qbl_file.h"
#include "qbl/serde/qbl_kaldiio.h"
#include "qtk/vprint/qtk_speaker_diarization.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/wtk_riff.h"

static void test_wav_(qtk_speaker_diarization_t *sd, char *wav_fn,
                      qbl_string_t *tag) {
    wtk_riff_t *riff;
    char buf[4096];
    int is_end = 0;
    int read_cnt;
    qtk_speaker_diarization_segment_t *segements;
    int N;
    int *labels;

    riff = wtk_riff_new();
    wtk_riff_open(riff, wav_fn);
    if (riff->fmt.sample_rate != 16000 || riff->fmt.bit_per_sample != 16 ||
        riff->fmt.channels != 1) {
        wtk_debug("invalid wavfile: %s\n", wav_fn);
        wtk_riff_delete(riff);
        return;
    }

    while (is_end == 0) {
        read_cnt = wtk_riff_read(riff, buf, sizeof(buf));
        if (read_cnt < sizeof(buf)) {
            is_end = 1;
        }
        qtk_speaker_diarization_feed(sd, (short *)buf, read_cnt / 2);
    }

    qtk_speaker_diarization_feed_end(sd, &segements, &labels, &N);

    for (int i = 0; i < N; i++) {
        float s, dur;
        s = segements[i].start_ms / 1000.0;
        dur = (segements[i].end_ms - segements[i].start_ms) / 1000.0;
        printf("SPEAKER %.*s 1 %f %f <NA> <NA> %d <NA> <NA>\n", tag->len,
               tag->data, s, dur, labels[i]);
    }
    wtk_riff_delete(riff);
    qtk_speaker_diarization_reset(sd);
}

static int on_scp_elem_(qtk_speaker_diarization_t *sd,
                        qbl_kaldiio_elem_t *elem) {
    test_wav_(sd, elem->xfilename.data, &elem->token);
    return 0;
}

static void test_scp_(qtk_speaker_diarization_t *sd, char *scp_fn) {
    FILE *fp;
    qbl_kaldiio_t ki;
    fp = fopen(scp_fn, "rt");
    qbl_kaldiio_init(&ki, (int (*)(void *, qbl_kaldiio_elem_t *))on_scp_elem_,
                     sd);
    qbl_kaldiio_load_scp(&ki, (qbl_io_reader)qbl_file_read, fp);
    qbl_kaldiio_clean(&ki);
    fclose(fp);
}

int main(int argc, char *argv[]) {
    wtk_arg_t *arg;
    char *scp_fn, *cfg_fn;
    wtk_main_cfg_t *main_cfg;
    qtk_speaker_diarization_t *sd;

    arg = wtk_arg_new(argc, argv);
    wtk_arg_get_str_s(arg, "c", &cfg_fn);
    wtk_arg_get_str_s(arg, "scp", &scp_fn);

    main_cfg = wtk_main_cfg_new_type(qtk_speaker_diarization_cfg, cfg_fn);
    sd = qtk_speaker_diarization_new(
        (qtk_speaker_diarization_cfg_t *)main_cfg->cfg);
    test_scp_(sd, scp_fn);
    qtk_speaker_diarization_delete(sd);
    wtk_arg_delete(arg);
    wtk_main_cfg_delete(main_cfg);
}
