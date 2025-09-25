#ifndef QTK_RCD_QTK_RECORDER_H
#define QTK_RCD_QTK_RECORDER_H

#include "qtk_alsa_recorder.h"
#include "qtk_asound_card.h"
#include "qtk_recorder_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_recorder qtk_recorder_t;
struct qtk_recorder {
    qtk_recorder_cfg_t *cfg;
    qtk_alsa_recorder_t *rcd;
    wtk_log_t *log;
    wtk_strbuf_t *buf;
    unsigned int sample_rate;
    unsigned int channel;
    unsigned int bytes_per_sample;
    unsigned int read_fail_times;
};

qtk_recorder_t *qtk_recorder_new(qtk_recorder_cfg_t *cfg, wtk_log_t *log);
void qtk_recorder_delete(qtk_recorder_t *r);
void qtk_recorder_set_fmt(qtk_recorder_t *r, int sample_rate, int channel,
                          int bytes_per_sample);

int qtk_recorder_start(qtk_recorder_t *r);
wtk_strbuf_t *qtk_recorder_read(qtk_recorder_t *r);
void qtk_recorder_stop(qtk_recorder_t *r);
void qtk_recorder_clean(qtk_recorder_t *r);

int qtk_recorder_get_channel(qtk_recorder_t *r);

#ifdef __cplusplus
};
#endif
#endif
