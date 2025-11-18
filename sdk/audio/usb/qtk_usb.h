#ifndef SDK_AUDIO_USB_QTK_USB
#define SDK_AUDIO_USB_QTK_USB
#ifdef USE_USB
#include "libusb/libusb.h"
#endif

#include "wtk/os/wtk_thread.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_lockhoard.h"

#include "qtk_usb_cfg.h"
#include "sdk/session/qtk_session.h"


#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_usb qtk_usb_t;
typedef void (*qtk_usb_notify_f)(void *ths,int is_err);

#ifdef USE_USB
typedef struct
{
	struct libusb_transfer *transfer;
	char *data;
	int len;
	qtk_usb_t *u;
}qtk_usb_rcdtask_t;

typedef struct
{
	wtk_queue_node_t q_n;
	struct libusb_transfer *transfer;
	wtk_strbuf_t *buf;
	qtk_usb_t *u;
}qtk_usb_plytask_t;

typedef struct
{
	wtk_queue_node_t hoard_n;
	wtk_queue_node_t q_n;
	wtk_string_t *v;
}qtk_usb_rcdnode_t;
#endif

struct qtk_usb
{
	qtk_usb_cfg_t *cfg;
#ifdef USE_USB
	qtk_session_t *session;
	void *notify_ths;
	qtk_usb_notify_f notify_f;
	wtk_thread_t thread;
	wtk_lockhoard_t rcd_hoard;
	wtk_blockqueue_t rcd_q;
	qtk_usb_rcdtask_t **rcdtasks;
	wtk_strbuf_t *rcd_buf;
	wtk_strbuf_t *rcdtmp_buf;
	wtk_blockqueue_t ply_q;
	qtk_usb_plytask_t **plytasks;
	int channel;
	libusb_context *ctx;
	libusb_device_handle *dev_hd;
	libusb_hotplug_callback_handle handle;
	wtk_lock_t lock;
	unsigned run:1;
	unsigned rcd_hint:1;
	unsigned err:1;
#endif
};

qtk_usb_t* qtk_usb_new(qtk_usb_cfg_t *cfg,qtk_session_t *session,void *notify_ths,qtk_usb_notify_f notify_f);
int qtk_usb_delete(qtk_usb_t *u);
int qtk_usb_start(qtk_usb_t *u);
int qtk_usb_stop(qtk_usb_t *u);

int qtk_usb_play_start(qtk_usb_t *u,char *sd_name,int sample_rate,int channel,int bytes_per_sample);
int qtk_usb_play_write(qtk_usb_t *u,char *data,int bytes);
void qtk_usb_play_stop(qtk_usb_t *u);

int qtk_usb_record_start(qtk_usb_t *u);
wtk_strbuf_t* qtk_usb_record_read(qtk_usb_t *u);
void qtk_usb_record_stop(qtk_usb_t *u);
int qtk_usb_send_cmd(qtk_usb_t *u,int cmd);

#ifdef __cplusplus
};
#endif
#endif
