
#ifndef QTK_API_1_WAKEUP_QTK_WAKEUP
#define QTK_API_1_WAKEUP_QTK_WAKEUP

#include "wtk/asr/img/qtk_img.h"
#include "wtk/bfio/wtk_kvadwake.h"

#include "qtk_wakeup_cfg.h"
#include "sdk/qtk_api.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef void (*qtk_wakeup_notify_f)(void *ths, int res, float prob, int start, int end);
typedef struct qtk_wakeup qtk_wakeup_t;

struct qtk_wakeup {
    qtk_wakeup_cfg_t *cfg;
    qtk_session_t *session;
    qtk_img_rec_t *img;
    wtk_kvadwake_t *kvwake;
    qtk_wakeup_notify_f notify;
    void *wakethis;
};

qtk_wakeup_t *qtk_wakeup_new(qtk_wakeup_cfg_t *cfg, qtk_session_t *session);
void qtk_wakeup_delete(qtk_wakeup_t *wakup);

int qtk_wakeup_start(qtk_wakeup_t *wakup);
int qtk_wakeup_feed(qtk_wakeup_t *wakup, char *data, int len, int is_end);
int qtk_wakeup_reset(qtk_wakeup_t *wakup);

void qtk_wakeup_set_notify(qtk_wakeup_t *wakup, void *notify_ths,
		qtk_wakeup_notify_f notify_f);

void qtk_wakeup_set_coreType(qtk_wakeup_t *wakup, char *data, int len);
void qtk_wakeup_set_res(qtk_wakeup_t *wakup, char *data, int len);
#ifdef __cplusplus
};
#endif
#endif
