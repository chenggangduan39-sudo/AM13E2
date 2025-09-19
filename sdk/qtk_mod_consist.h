#ifndef __SDK_MOD_consist_CONSIST_H__
#define __SDK_MOD_consist_CONSIST_H__
#include "qtk_mod_consist_cfg.h"
#include "wtk/os/wtk_thread.h"
#include "wtk/os/wtk_lockhoard.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_sem.h"
#include "wtk/os/wtk_log.h"
#include "sdk/codec/qtk_msg.h"
#include "sdk/codec/qtk_audio_conversion.h"

#ifndef OFFLINE_TEST
#ifndef __ANDROID__
#include "qtk/record/qtk_record.h"
#include "qtk/play/qtk_play.h"
#endif
#endif
#include "sdk/qtk_comm.h"
#include "wtk/bfio/consist/wtk_consist.h"

#include "sdk/api_1/vboxebf/qtk_vboxebf.h"
#include "sdk/api_1/vboxebf/qtk_vboxebf_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_mod_consist{
	qtk_mod_consist_cfg_t *cfg;
#ifndef OFFLINE_TEST
	qtk_record_t *rcd;
	qtk_play_t *play;
#endif
	wtk_consist_t *consist;
	wtk_log_t *log;
	wtk_thread_t rcd_t;
	wtk_thread_t qform_t;
	wtk_thread_t enspick_t;
	wtk_blockqueue_t bfio_queue;
	wtk_blockqueue_t enspick_queue;
	qtk_vboxebf_cfg_t *vboxebf_cfg;
	qtk_vboxebf_t *vboxebf;
	qtk_msg_t *msg;
	wtk_wavfile_t *mul;
	wtk_strbuf_t *mul_path;
	wtk_sem_t start_sem;
	int minlen;
	int recordlen;
	int *err_nil;
	int *err_align;
	int *err_max;
	int *err_corr;
	int *err_energy;
	unsigned int rcd_run:1;
	unsigned int qform_run:1;
	unsigned int enspick_run:1;
	unsigned int log_audio:1;
	unsigned int is_send:1;
}qtk_mod_consist_t;

qtk_mod_consist_t *qtk_mod_consist_new(qtk_mod_consist_cfg_t *cfg, float recordtime, int mode, int micgain,int backgain);
void qtk_mod_consist_delete(qtk_mod_consist_t *m);
void qtk_mod_consist_start(qtk_mod_consist_t *m, char *fn);
void qtk_mod_consist_stop(qtk_mod_consist_t *m);
#ifdef OFFLINE_TEST
void qtk_mod_consist_feed(qtk_mod_consist_t *m, char *data, int len);
#endif
#ifdef __cplusplus
};
#endif
#endif
