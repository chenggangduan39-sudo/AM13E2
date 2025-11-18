#ifndef __QTK_DEV_EVENT_H__
#define __QTK_DEV_EVENT_H__
/*try use in RK DEVICEIO KEY*/
/*qdreamer rk3308b dmc10项目的硬件消息*/
/*线程不安全不要多线程使用*/
#include <linux/input.h>
//#include <DeviceIo/Rk_key.h>
//#include "third/siZheng/DeviceIO/include/DeviceIo/Rk_key.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_thread.h"
#include "wtk/os/wtk_lockhoard.h"
//#include "qtk/core/timer/qtk_timer.h"
#include "wtk/os/wtk_log.h"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <linux/input.h>
#include <stdbool.h>

#include <sys/types.h>  
#include <asm/types.h>  
#include <linux/netlink.h>  
#include <linux/rtnetlink.h>  
#include <string.h>  

#ifdef __cplusplus
extern "C"{
#endif

#define USE_USB

typedef struct qtk_dev_event qtk_dev_event_t;
typedef void (*qtk_dev_event_notify_f)(void*user_data,int type);
typedef void (*qtk_dev_event_value_notify_f)(void*user_data,int type,int value);

typedef enum qtk_event_stat {
	//net state
    NET0_UP,
    NET0_DOWN,
    NET1_UP,
    NET1_DOWN,
	//usb state
    CONNECTED_E,
    CONFIGURED_E,
    DISCONNECTED_E,
    VBUS_UP,
    VBUS_DOWN,
    //uac1 status
    UAC1_ON,
    UAC1_OFF,
	//power state
	VOLUME_CHANGED,
    EXTCON_USB_POWER,
    EXTCON_48V_POWER,
    EXTCON_48V_POWER_DOWN,
    POE_POWER_TRUE,
    POE_POWER_FALSE,
    POE_48V_BOTH_TRUE,
    POE_48V_BOTH_FALSE,
    //key state
    EKEY_BLUETOOTH,
    EKEY_VOLUMEDOWN,
    EKEY_VOLUMEDOWN_UNPRESS,
    EKEY_VOLUMEDOWN_LONGPRESS,
    EKEY_VOLUMEUP,
    EKEY_VOLUMEUP_UNPRESS,
    EKEY_VOLUMEUP_LONGPRESS,
    EKEY_MUTE,
    EUSB_RCD_MUTE,
    EUSB_PLAY_MUTE,
    EUSB_PPM,
    EKEY_FN_F1,
    EKEY_FN_F1_UNPRESS,
    EKEY_FN_F2,
    EKEY_FN_F2_UNPRESS,
    EKEY_PRODUCTION_MODE,
}qtk_event_stat_t;

struct qtk_dev_event{
    int th_run;
    int netlink_run;
    // qtk_timer_t *timer;
    void *user_data;
    qtk_dev_event_notify_f notify;
    qtk_dev_event_value_notify_f value_notify;
    wtk_blockqueue_t msg_list;
    wtk_lockhoard_t msg_hoard;
    wtk_thread_t trans_th;
    wtk_thread_t netlink_th;
    wtk_thread_t usb_stat_th;
	wtk_log_t *log;

    int net0_stat;      // eth0
    int net1_stat;      // eth1
    int pow_stat;       // power
    int uac_stat;       // usb
    int uac1_status;    // uac1
    int uac_volume_temp;

    int key_volup_stat;
    int key_voldown_stat;

    unsigned flag;
    int netlinkfd[2];
};

#define QTK_DEV_NET_EVENT (0x01)
#define QTK_DEV_USB_EVENT (0x01<<1)
#define QTK_DEV_KEY_EVENT (0x01<<2)
#define QTK_DEV_POWER_EVENT (0x01<<3)

#define QTK_DEV_ALL_EVENT (QTK_DEV_NET_EVENT | QTK_DEV_USB_EVENT | \
                            QTK_DEV_KEY_EVENT | QTK_DEV_POWER_EVENT)

qtk_dev_event_t *qtk_dev_event_new(void);
qtk_dev_event_t *qtk_dev_event_new2(unsigned flag);
int qtk_dev_event_set_notify(qtk_dev_event_t *event,qtk_dev_event_notify_f cb,void *m);
int qtk_dev_event_set_value_notify(qtk_dev_event_t *event,qtk_dev_event_value_notify_f cb,void *m);
int qtk_dev_event_start(qtk_dev_event_t *event);
int qtk_dev_event_start2(qtk_dev_event_t *event);
int qtk_dev_event_stop(qtk_dev_event_t *event);
int qtk_dev_event_stop2(qtk_dev_event_t *event);
int qtk_dev_event_delete(qtk_dev_event_t *event);
int qtk_dev_event_delete2(qtk_dev_event_t *event);
int qtk_dev_event_power(void);

int get_net_stat(int type);
int get_br0_stat(void);

int get_adb_flag(void);             // ./res/adb
int get_adb_status(void);           // /tmp/.usb_config
int get_uac_status(void);
int _get_usb_otgport(void);
int _get_hidg0_exist(void);
int _get_device_usb2(void);

int get_os_type(void);
int get_is_windows(void);

int qtk_dev_event_uac_stat(void);

int _dev_event_long_press_hb_callback(const int key_code, u_int32_t time);
int _dev_event_long_press_callback(const int key_code, u_int32_t time);

int get_ip_exists();
char* get_local_ip();

//int wtk_mapping_volumedata_mappint_percentage(int signed_volume);

#ifdef __cplusplus
};
#endif

#endif
