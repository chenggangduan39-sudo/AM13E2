#ifndef QTK_API_TTSC_QTK_TTS
#define QTK_API_TTSC_QTK_TTS

#include "wtk/tts/wtk_tts.h"

#include "cldtts/qtk_cldtts.h"
#include "qtk_tts_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_tts qtk_tts_t;

struct qtk_tts {
    qtk_tts_cfg_t *cfg;
    qtk_session_t *session;
    union {
        qtk_cldtts_t *cldtts;
        wtk_tts_t *tts;
    } ins;
};

qtk_tts_t *qtk_tts_new(qtk_tts_cfg_t *cfg, qtk_session_t *session);
void qtk_tts_delete(qtk_tts_t *tts);

int qtk_tts_start(qtk_tts_t *tts);
int qtk_tts_feed(qtk_tts_t *tts, char *data, int len);
int qtk_tts_reset(qtk_tts_t *tts);

void qtk_tts_set_notify(qtk_tts_t *tts, void *notify_ths,
                        qtk_cldtts_notify_f notify_f);

void qtk_tts_set_stop_hint(qtk_tts_t *tts);

void qtk_tts_set_speed(qtk_tts_t *tts, float speed);
void qtk_tts_set_pitch(qtk_tts_t *tts, float pitch);
void qtk_tts_set_volume(qtk_tts_t *tts, float volume);
void qtk_tts_set_coreType(qtk_tts_t *tts, char *data, int len);
void qtk_tts_set_res(qtk_tts_t *tts, char *data, int len);
void qtk_tts_set_useStream(qtk_tts_t *tts, int useStream);
#ifdef __cplusplus
};
#endif
#endif
