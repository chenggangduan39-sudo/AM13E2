#ifndef __QTK_RECORD__H__
#define __QTK_RECORD__H__
#include "qtk_alsa_recorder.h"
#include "wtk/core/wtk_strbuf.h"
#include "qtk_record_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_record{
    qtk_record_cfg_t *cfg;
    qtk_alsa_recorder_t *alsa;
    wtk_strbuf_t *buf;
}qtk_record_t;

qtk_record_t *qtk_record_new(qtk_record_cfg_t *cfg);
void qtk_record_delete(qtk_record_t *rcd);
wtk_strbuf_t *qtk_record_read(qtk_record_t *rcd);

void qtk_record_set_mic_gain(qtk_record_t *rcd);
void qtk_record_set_cb_gain(qtk_record_t *rcd);

#ifdef __cplusplus
};
#endif
#endif
