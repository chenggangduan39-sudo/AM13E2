
#ifndef QTK_API_CSR_QTK_CSR
#define QTK_API_CSR_QTK_CSR

#include "wtk/asr/vad/wtk_vad.h"

#include "qtk_csr_cfg.h"
#include "sdk/api_1/asr/qtk_asr.h"
#include "sdk/qtk_api.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_csr qtk_csr_t;

typedef void (*qtk_csr_notify_f)(void *ths, qtk_var_t *var);

struct qtk_csr {
    qtk_csr_cfg_t *cfg;
    qtk_session_t *session;
    qtk_asr_t *asr;
    wtk_vad_t *vad;
    wtk_queue_t vad_q;
    qtk_csr_notify_f notify;
    void *notify_ths;
    int vad_count;
    int feed_bytes;
    float start_time;
    float end_time;
    unsigned sil : 1;
    unsigned valid : 1;
    unsigned cancel : 1;
};

qtk_csr_t *qtk_csr_new(qtk_csr_cfg_t *cfg, qtk_session_t *session);
void qtk_csr_delete(qtk_csr_t *c);
void qtk_csr_set_notify(qtk_csr_t *c, void *ths, qtk_csr_notify_f notify);
void qtk_csr_set_idle_time(qtk_csr_t *c, int itime);

int qtk_csr_start(qtk_csr_t *c, int left, int right);
int qtk_csr_feed(qtk_csr_t *c, char *data, int bytes, int is_end);
void qtk_csr_reset(qtk_csr_t *c);
void qtk_csr_cancel(qtk_csr_t *c);

#ifdef __cplusplus
};
#endif
#endif
