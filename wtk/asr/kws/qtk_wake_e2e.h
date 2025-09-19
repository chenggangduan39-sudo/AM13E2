#ifndef QTK_WAKE_E2E_H_
#define QTK_WAKE_E2E_H_
#include "qtk_wake_e2e_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_wake_e2e qtk_wake_e2e_t;

struct qtk_wake_e2e
{
	qtk_wake_e2e_cfg_t *cfg;
};

qtk_wake_e2e_t* qtk_wake_e2e_new(qtk_wake_e2e_cfg_t *cfg,wtk_queue_t *output_queue);
void qtk_wake_e2e_delete(qtk_wake_e2e_t *wake);
int qtk_wake_e2e_start(qtk_wake_e2e_t *wake);
int qtk_wake_e2e_reset(qtk_wake_e2e_t *wake);
int qtk_wake_e2e_feed(qtk_wake_e2e_t *wake,char *data,int bytes,int is_end);

#ifdef __cplusplus
};
#endif
#endif
