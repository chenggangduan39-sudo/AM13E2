#ifndef SDK_AUDIO_daemon_QTK_AUDIO_daemon
#define SDK_AUDIO_daemon_QTK_AUDIO_daemon

#include "wtk/os/wtk_log.h"
#include "wtk/os/wtk_lockhoard.h"
#include "wtk/os/wtk_pipequeue.h"
#include "wtk/os/wtk_thread.h"

#include <errno.h>
#include <sys/ioctl.h>
#ifdef USE_USB
#include <linux/usbdevice_fs.h>
#include "libusb/libusb.h"
#endif
#include "qtk_audio_daemon_cfg.h"
#include "sdk/session/qtk_session.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_audio_daemon qtk_audio_daemon_t;

typedef enum {
	QTK_AUDIO_DAEMON_STATE_LEFT,
	QTK_AUDIO_DAEMON_STATE_ARRIVE,
}qtk_audio_daemon_state_t;

typedef enum {
	QTK_AUDIO_DAEMON_LEFT,
	QTK_AUDIO_DAEMON_ARRIVE,
	QTK_AUDIO_DAEMON_ERROR,
}qtk_audio_daemon_cmd_t;

typedef void(*qtk_audio_daemon_notify_func)(void *notify_ths,qtk_audio_daemon_cmd_t cmd);

typedef struct {
	wtk_queue_node_t hoard_n;
	wtk_queue_node_t q_n;
	qtk_audio_daemon_cmd_t cmd;
}qtk_audio_daemon_msg_t;

struct qtk_audio_daemon
{
	qtk_audio_daemon_cfg_t *cfg;
	qtk_session_t  *session;
	qtk_audio_daemon_notify_func notify_func;
	void *notify_ths;
#ifdef USE_USB
	libusb_context *ctx;
	libusb_hotplug_callback_handle handle;
#endif
	wtk_thread_t thread;
	wtk_pipequeue_t msg_q;
	wtk_lockhoard_t msg_hoard;
	fd_set r_set;
	fd_set w_set;
	fd_set e_set;
	qtk_audio_daemon_state_t state;
	int max_fd;
	int vid;
	int pid;
	unsigned run:1;
};

qtk_audio_daemon_t* qtk_audio_daemon_new(qtk_audio_daemon_cfg_t *cfg,qtk_session_t *session);
void qtk_audio_daemon_delete(qtk_audio_daemon_t *ad);
void qtk_audio_daemon_set_notify(qtk_audio_daemon_t *ad,void *notify_ths,qtk_audio_daemon_notify_func notify_func);
void qtk_audio_daemon_set_vid_pid(qtk_audio_daemon_t *ad,int vid,int pid);

int qtk_audio_daemon_start(qtk_audio_daemon_t *ad);
int qtk_audio_daemon_stop(qtk_audio_daemon_t *ad);

int qtk_audio_daemon_feed(qtk_audio_daemon_t *ad,qtk_audio_daemon_cmd_t cmd);

int qtk_audio_daemon_reset_dev(qtk_audio_daemon_t *ad);

#ifdef __cplusplus
};
#endif
#endif
