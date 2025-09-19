#ifndef __QTK_USB_UEVENT__
#define __QTK_USB_UEVENT__

#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <ctype.h>  
#include <sys/un.h>  
#include <sys/ioctl.h>  
#include <sys/socket.h>  
#include <linux/types.h>  
#include <linux/netlink.h>  
#include <errno.h>  
#include <unistd.h>  
#include <arpa/inet.h>  
#include <netinet/in.h>
#include "wtk/os/wtk_thread.h"
#include "wtk/core/wtk_str.h"
#include "wtk/os/wtk_blockqueue.h"
#include "sdk/codec/qtk_msg.h"

#ifdef __cplusplus
extern "C" { 
#endif

typedef enum{
    QTK_USB_STATE_PLAYER_START,
    QTK_USB_STATE_PLAYER_STOP,
    QTK_USB_STATE_SYSTEM_REBOOT,
    QTK_USB_STATE_PULL_UP,
}qtk_usb_uevent_state_t;

typedef struct qtk_usb_uevent qtk_usb_uevent_t;
typedef void (*qtk_usb_uevent_notify_f)(void *ths, qtk_usb_uevent_state_t state, int sample_rate);

/**
 *		wtk_string("ACTION"),							//0   开始
		wtk_string("DEVPATH"),							//1   设备路径
		wtk_string("SUBSYSTEM"),						//2   子系统
		wtk_string("USB_STATE"),						//3   usb状态
		wtk_string("STREAM_DIRECTION"),					//4   音频流方向
		wtk_string("STREAM_STATE"),						//5   流状态
		wtk_string("SEQNUM"),							//6   字符长度
 *  
 */

struct qtk_usb_uevent
{
    wtk_thread_t uu_t;
    wtk_thread_t msg_t;
    wtk_blockqueue_t msg_queue;
    qtk_msg_t *msg;
    void *ths;
    char *action;
    char *devpath;
    char *subsystem;
    char *usb_state;
    char *stream_direction;
    char *stream_state;
    int sample_rate;
    qtk_usb_uevent_notify_f notify;
    unsigned uu_run:1;
    unsigned msg_run:1;
    unsigned use_audio:1;
    unsigned use_android_usb:1;
    unsigned use_stream_state:1;
    unsigned use_stream_direction:1;
    unsigned use_sample_rate:1;
};

qtk_usb_uevent_t * qtk_usb_uevent_new();
void qtk_usb_uevent_delete(qtk_usb_uevent_t *uu);
void qtk_usb_uevent_set_notify(qtk_usb_uevent_t *uu, void *ths, qtk_usb_uevent_notify_f notify);


#ifdef __cplusplus
};
#endif
#endif
