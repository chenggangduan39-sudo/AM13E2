
#ifndef QTK_API_TTSC_QTK_CLDTTS
#define QTK_API_TTSC_QTK_CLDTTS

#include "wtk/core/json/wtk_json.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_thread.h"

#include "qtk_cldtts_cfg.h"
#include "sdk/spx/qtk_spx.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_cldtts qtk_cldtts_t;
typedef void (*qtk_cldtts_notify_f)(void *ths, char *data, int bytes);

struct qtk_cldtts {
    qtk_cldtts_cfg_t *cfg;
    qtk_session_t *session;
    qtk_spx_t *spx;
    qtk_cldtts_notify_f notify_f;
    void *notify_ths;
    unsigned stop_hint : 1;
};

qtk_cldtts_t *qtk_cldtts_new(qtk_cldtts_cfg_t *cfg, qtk_session_t *session);
void qtk_cldtts_start(qtk_cldtts_t *c);
void qtk_cldtts_reset(qtk_cldtts_t *c);
int qtk_cldtts_process(qtk_cldtts_t *c, char *data, int len);
void qtk_cldtts_set_notify(qtk_cldtts_t *c, void *notify_ths,
                           qtk_cldtts_notify_f notify_f);
void qtk_cldtts_delete(qtk_cldtts_t *c);

void qtk_cldtts_set_stop_hint(qtk_cldtts_t *c);
void qtk_cldtts_set_speed(qtk_cldtts_t *c, float speed);
void qtk_cldtts_set_pitch(qtk_cldtts_t *c, float pitch);
void qtk_cldtts_set_volume(qtk_cldtts_t *c, float volume);
void qtk_cldtts_set_coreType(qtk_cldtts_t *c, char *data, int len);
void qtk_cldtts_set_res(qtk_cldtts_t *c, char *data, int len);
void qtk_cldtts_set_useStream(qtk_cldtts_t *c, int useStream);

#ifdef __cplusplus
};
#endif
#endif
