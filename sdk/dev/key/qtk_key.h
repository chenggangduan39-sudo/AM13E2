#ifndef __QTK_KEY_H__
#define __QTK_KEY_H__
#include "qtk_key_cfg.h"
#include "wtk/os/wtk_thread.h"

#define POWR_KEY  (114)
#define VOL_KEY   (115)

typedef enum{
	QTK_POWER_KEY,
	QTK_POWER_KEY_LONG,
	QTK_VOL_KEY,
	QTK_VOL_KEY_LONG,
}qtk_key_cmd_t;

typedef void (*qtk_key_notify_f)(void *ths, qtk_key_cmd_t cmd);
typedef struct qtk_key{
	qtk_key_cfg_t *cfg;
	wtk_thread_t thread;
	void *ths;
	qtk_key_notify_f notify;
	int fd;
	unsigned int run:1;
}qtk_key_t;

qtk_key_t *qtk_key_new(qtk_key_cfg_t *cfg);
void qtk_key_delete(qtk_key_t *key);
int qtk_key_start(qtk_key_t *key);
int qtk_key_stop(qtk_key_t *key);
void qtk_key_set_notify(qtk_key_t *key, void *ths, qtk_key_notify_f notify);
#endif
