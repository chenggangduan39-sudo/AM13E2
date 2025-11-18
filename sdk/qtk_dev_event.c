#include "qtk_dev_event.h"
#include "wtk/os/wtk_malloc.h"
#include "../wtk/core/wtk_alloc.h"
#include <sys/socket.h>
#include <linux/netlink.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <linux/if.h>
#include <errno.h>
#include "wtk/core/wtk_type.h"

// #define QSOUND_AM31S
// #define QSOUND_AM31S_2

#define CAT(a,b)                        (a ## b)
#define NUM_DEVICES 3
#define BUFLEN 20480
#define MAX_UEVENT_STRINGS 30
#define UAC_KEY_AUDIO                   (2)
#define UAC_KEY_AUDIO24                 (24)
#define UAC_KEY_USB_STATE               (3)
#define UAC_KEY_DIRECTION               (4)
#define UAC_KEY_STREAM_STATE            (5)
#define UAC_KEY_SAMPLE_RATE     UAC_KEY_STREAM_STATE
#define UAC_KEY_VOLUME          UAC_KEY_STREAM_STATE
#define UAC_KEY_PPM             4
#define UAC_KEY_MUTE            UAC_KEY_STREAM_STATE

#define UAC_UEVENT_AUDIO                "SUBSYSTEM=u_audio"
#define UAC_UEVENT_USB                  "SUBSYSTEM=android_usb"
#define USB_UEVENT_PHY                  "SUBSYSTEM=phy"

#define UAC_UEVENT_SET_INTERFACE        "USB_STATE=SET_INTERFACE"
#define UAC_UEVENT_SET_VOLUME           "USB_STATE=SET_VOLUME"
#define UAC_UEVENT_SET_MUTE             "USB_STATE=SET_MUTE"
#define UAC_UEVENT_SET_AUDIO_CLK        "USB_STATE=SET_AUDIO_CLK"
#define UAC_UEVENT_GET_USB_STATE        "USB_STATE=CONFIGURED"
#define UAC_UEVENT_GET_USB_CONNECT      "USB_STATE=CONNECTED"
#define UAC_UEVENT_GET_USB_DISCONNECT   "USB_STATE=DISCONNECTED"
#define UAC_STREAM_DIRECT               "STREAM_DIRECTION="
#define UAC_STREAM_STATE                "STREAM_STATE="
#define UAC_REMOTE_PLAY                     "OUT"
#define UAC_REMOTE_CAPTURE                  "IN"
#define UAC_STREAM_START                    "ON"
#define UAC_STREAM_STOP                     "OFF"
#define UAC_STREAM_DIRECT_IN            UAC_STREAM_DIRECT UAC_REMOTE_CAPTURE
#define UAC_STREAM_DIRECT_OUT           UAC_STREAM_DIRECT UAC_REMOTE_PLAY
#define UAC_STREAM_STATE_ON             UAC_STREAM_STATE UAC_STREAM_START
#define UAC_STREAM_STATE_OFF            UAC_STREAM_STATE UAC_STREAM_STOP


typedef struct event_trans_msg{
    wtk_queue_node_t q_n;
    int type;
    int volume;
}event_trans_msg_t;

struct _uevent {
    char *strs[30];
    int size;
};

typedef void (*_read_process_f)(qtk_dev_event_t *event,int fd);

typedef struct {
    int fd;
    _read_process_f read_process;
}_dev_epoll_event_t;




typedef enum _evnet_stat{
    EVENT_START=0,
    EVENT_KEY_BLUETOOTH,
    EVENT_KEY_VOLUMEDOWN,
    EVENT_KEY_VOLUMEDOWN_LONGPRESS,
    EVENT_KEY_VOLUMEDOWN_UNPRESS,
    EVENT_KEY_VOLUMEUP,
    EVENT_KEY_VOLUMEUP_LONGPRESS,
    EVENT_KEY_VOLUMEUP_UNPRESS,
    EVENT_KEY_MUTE,
    EVENT_KEY_FN_F1,
    EVENT_KEY_FN_F1_UNPRESS,
    EVENT_KEY_FN_F2,
    EVENT_KEY_FN_F2_UNPRESS,
    EVENT_NET1_CONNECT,
    EVENT_NET1_DISCONNECT,
    EVENT_NET2_CONNECT,
    EVENT_NET2_DISCONNECT,
    EVENT_POWER_48V,
    EVENT_POWER_48V_DOWN,
    EVENT_POWER_POE,
    EVENT_POWER_POE_DOWN,
    EVENT_POWER_USB,
    EVENT_POWER_48V_POE,
    EVENT_POWER_48V_POE_EXIT,
    EVENT_UAC_UP,
    EVENT_UAC_DOWN,
    EVENT_VBUS_UP,
    EVENT_VBUS_DOWN,
    EVENT_UAC1_ON,
    EVENT_UAC1_OFF,
    EVENT_PRODUCTION_MODE,
    EVENT_UAC_VOLUME,
    EVENT_UAC_PLAY_MUTE,
    EVENT_UAC_RECORD_MUTE,
    EVENT_UAC_PPM,
}_evnet_stat_t;

void qtk_dev_event_stat_print(qtk_dev_event_t *ev);
int _get_device_power_state(int type);
int _get_device_usb(void);
int _dev_event_netlink_callback(qtk_dev_event_t *event, struct ifinfomsg *if_info, struct nlmsghdr *nh);
struct epoll_event* _netevent_epoll_create(void);
struct epoll_event* _usb_epoll_event_create(void);
int _netlink_event_process(qtk_dev_event_t *evt,wtk_thread_t *t);
void _netevent_epoll_read_process(qtk_dev_event_t *event,int fd);
void _usb_epoll_event_read(qtk_dev_event_t *dev,int fd);

static qtk_dev_event_t *g_event = NULL;

static char* local_ip = NULL;

event_trans_msg_t* event_trans_msg_new(qtk_dev_event_t *evt)
{
    event_trans_msg_t *msg = NULL;
    msg = wtk_malloc(sizeof(*msg));
    if(msg) memset(msg,0,sizeof(*msg));
    return msg;
}

int event_trans_msg_delete(event_trans_msg_t *msg)
{
    wtk_free(msg);
    return 0;
}

int _trans_handler(qtk_dev_event_t *evt,wtk_thread_t *t)
{
    int ret = 0;
    wtk_queue_node_t *node = NULL;
    event_trans_msg_t *msg = NULL;
    while(evt->th_run){
        node = wtk_blockqueue_pop(&evt->msg_list,-1,NULL);
        if(node == NULL) continue;
        msg = data_offset2(node,event_trans_msg_t,q_n);
        //msg process
        switch(msg->type){
            // 屏蔽网口事件处理
            // case EVENT_NET1_CONNECT:    // NET0
            //     if(evt->notify) evt->notify(evt->user_data,NET0_UP);
            //     // wtk_log_log0(glb_log, "EVENT_NET0_CONNEC");
            //     // wtk_debug("EVENT_NET0_CONNEC\n");
            //     break;
            // case EVENT_NET1_DISCONNECT: // NET0
            //     if(evt->notify) evt->notify(evt->user_data,NET0_DOWN);
            //     // wtk_log_log0(glb_log, "EVENT_NET0_DISCONNECT");
            //     // wtk_debug("EVENT_NET0_DISCONNECT\n");
            //     break;
            // case EVENT_NET2_CONNECT:    // NET1
            //     if(evt->notify) evt->notify(evt->user_data,NET1_UP);
            //     // wtk_log_log0(glb_log, "EVENT_NET1_CONNECT");
            //     // wtk_debug("EVENT_NET1_CONNECT\n");
            //     break;
            // case EVENT_NET2_DISCONNECT: // NET1
            //     if(evt->notify) evt->notify(evt->user_data,NET1_DOWN);
            //     // wtk_log_log0(glb_log, "EVENT_NET1_DISCONNECT");
            //     // wtk_debug("EVENT_NET1_DISCONNECT\n");
            //     break;
            case EVENT_POWER_48V_DOWN:
                // evt->pow_stat = 0;
                if(evt->notify) evt->notify(evt->user_data, EXTCON_48V_POWER_DOWN);
                // wtk_log_log0(glb_log, "EVENT_POWER_48V_DOWN");
                // wtk_debug("EVENT_POWER_48V_DOWN\n");
                break;
            case EVENT_POWER_48V:
                // evt->pow_stat = 2;
                if(evt->notify) evt->notify(evt->user_data,EXTCON_48V_POWER);
                // wtk_log_log0(glb_log, "EVENT_POWER_48V");
                // wtk_debug("EVENT_POWER_48V\n");
                break;
            case EVENT_POWER_POE_DOWN:
                // evt->pow_stat = 0;
                if(evt->notify) evt->notify(evt->user_data, POE_POWER_FALSE);
                // wtk_log_log0(glb_log, "EVENT_POWER_POE_DOWN");
                // wtk_debug("EVENT_POWER_POE_DOWN\n");
                break;
            case EVENT_POWER_POE:
                // evt->pow_stat = 1;
                if(evt->notify) evt->notify(evt->user_data,POE_POWER_TRUE);
                // wtk_log_log0(glb_log, "EVENT_POWER_POE");
                // wtk_debug("EVENT_POWER_POE\n");
                break;
            case EVENT_POWER_USB:
                // evt->pow_stat = 0;
                if(evt->notify) evt->notify(evt->user_data,EXTCON_USB_POWER);
                // wtk_log_log0(glb_log, "EVENT_POWER_USB");
                // wtk_debug("EVENT_POWER_USB\n");
                break;
            case EVENT_POWER_48V_POE:
                // evt->pow_stat = 3;
                if(evt->notify) evt->notify(evt->user_data, POE_48V_BOTH_TRUE);
                // wtk_log_log0(glb_log, "EVENT_POWER_48V_POE");
                // wtk_debug("EVENT_POWER_48V_POE\n");
                break;
            case EVENT_UAC_DOWN:
                // evt->uac_stat = 0;
                if(evt->notify) evt->notify(evt->user_data,DISCONNECTED_E);
                // wtk_log_log0(glb_log, "EVENT_UAC_DOWN");
                // wtk_debug("EVENT_UAC_DOWN\n");
                break;
            case EVENT_UAC_UP:
                // evt->uac_stat = 1;
                if(evt->notify) evt->notify(evt->user_data,CONFIGURED_E);
                // wtk_log_log0(glb_log, "EVENT_UAC_UP");
                // wtk_debug("EVENT_UAC_UP\n");
                break;
            case EVENT_VBUS_UP:
                if(evt->notify) evt->notify(evt->user_data,VBUS_UP);
                // wtk_log_log0(glb_log, "EVENT_UAC_UP");
                break;
            case EVENT_VBUS_DOWN:
                if(evt->notify) evt->notify(evt->user_data,VBUS_DOWN);
                // wtk_log_log0(glb_log, "EVENT_UAC_UP");
                break;
            case EVENT_UAC1_ON:
                // evt->uac1_status = 1;
                if(evt->notify) evt->notify(evt->user_data,UAC1_ON);
                // wtk_log_log0(glb_log, "EVENT_UAC1_ON");
                // wtk_debug("EVENT_UAC1_ON\n");
                break;
            case EVENT_UAC1_OFF:
                // evt->uac1_status = 0;
                if(evt->notify) evt->notify(evt->user_data,UAC1_OFF);
                // wtk_log_log0(glb_log, "EVENT_UAC1_OFF");
                // wtk_debug("EVENT_UAC1_OFF\n");
                break;
            case EVENT_KEY_BLUETOOTH:
                if(evt->notify) evt->notify(evt->user_data,EKEY_BLUETOOTH);
                // wtk_log_log0(glb_log, "EVENT_KEY_BLUETOOTH");
                // wtk_debug("EVENT_KEY_BLUETOOTH\n");
                break;
            case EVENT_KEY_VOLUMEDOWN:
                if(evt->notify) evt->notify(evt->user_data,EKEY_VOLUMEDOWN);
                // wtk_log_log0(glb_log, "EVENT_KEY_VOLUMEDOWN");
                // wtk_debug("EVENT_KEY_VOLUMEDOWN\n");
                break;
            case EVENT_KEY_VOLUMEDOWN_LONGPRESS:
                if(evt->notify) evt->notify(evt->user_data,EKEY_VOLUMEDOWN_LONGPRESS);
                // wtk_log_log0(glb_log, "EVENT_KEY_VOLUMEDOWN_LONGPRESS");
                // wtk_debug("EVENT_KEY_VOLUMEDOWN_LONGPRESS\n");
                break;  
            case EVENT_KEY_VOLUMEDOWN_UNPRESS:
                if(evt->notify) evt->notify(evt->user_data,EKEY_VOLUMEDOWN_UNPRESS);
                // wtk_log_log0(glb_log, "EVENT_KEY_VOLUMEDOWN_UNPRESS");
                // wtk_debug("EVENT_KEY_VOLUMEDOWN_UNPRESS\n");
                break;
            case EVENT_KEY_VOLUMEUP:
                if(evt->notify) evt->notify(evt->user_data,EKEY_VOLUMEUP);
                // wtk_log_log0(glb_log, "EVENT_KEY_VOLUMEUP\n");
                // wtk_debug("EVENT_KEY_VOLUMEUP\n");
                break;
            case EVENT_KEY_VOLUMEUP_LONGPRESS:
                if(evt->notify) evt->notify(evt->user_data,EKEY_VOLUMEUP_LONGPRESS);
                // wtk_log_log0(glb_log, "EVENT_KEY_VOLUMEUP_LONGPRESS");
                // wtk_debug("EVENT_KEY_VOLUMEUP_LONGPRESS\n");
                break;
            case EVENT_KEY_VOLUMEUP_UNPRESS:
                if(evt->notify) evt->notify(evt->user_data,EKEY_VOLUMEUP_UNPRESS);
                // wtk_log_log0(glb_log, "EVENT_KEY_VOLUMEUP_UNPRESS");
                // wtk_debug("EVENT_KEY_VOLUMEUP_UNPRESS\n");
                break;
            case EVENT_KEY_MUTE:
                if(evt->notify) evt->notify(evt->user_data,EKEY_MUTE);
                // wtk_log_log0(glb_log, "EVENT_KEY_MUTE");
                // wtk_debug("EVENT_KEY_MUTE\n");
                break;
            case EVENT_KEY_FN_F1:
                if(evt->notify) evt->notify(evt->user_data, EKEY_FN_F1);
                // wtk_log_log0(glb_log, "EVENT_KEY_FN_F1");
                // wtk_debug("EVENT_KEY_FN_F1\n");
                break;
            case EVENT_KEY_FN_F1_UNPRESS:
                if(evt->notify) evt->notify(evt->user_data, EKEY_FN_F1_UNPRESS);
                // wtk_log_log0(glb_log, "EVENT_KEY_FN_F1_UNPRESS");
                // wtk_debug("EVENT_KEY_FN_F1_UNPRESS\n");
                break;
            case EVENT_KEY_FN_F2:
                if(evt->notify) evt->notify(evt->user_data, EKEY_FN_F2);
                // wtk_log_log0(glb_log, "EVENT_KEY_FN_F2");
                // wtk_debug("EVENT_KEY_FN_F2\n");
                break;
            case EVENT_KEY_FN_F2_UNPRESS:
                if(evt->notify) evt->notify(evt->user_data, EKEY_FN_F2_UNPRESS);
                // wtk_log_log0(glb_log, "EVENT_KEY_FN_F2_UNPRESS");
                // wtk_debug("EVENT_KEY_FN_F2_UNPRESS\n");
                break;
            case EVENT_UAC_VOLUME:
                if(evt->value_notify) evt->value_notify(evt->user_data, VOLUME_CHANGED, msg->volume);
                // wtk_log_log(glb_log, "EVENT_UAC_VOLUME %d", msg->volume);
                /*wtk_debug("EVENT_UAC_VOLUME %d\n",msg->volume);*/
                break;
            case EVENT_UAC_PLAY_MUTE:
                if(evt->value_notify) evt->value_notify(evt->user_data, EUSB_PLAY_MUTE, msg->volume);
                // wtk_log_log0(evt->log, "EVENT_UAC_PLAY_MUTE");
                // wtk_log_log(evt->log,"EVENT_UAC_PLAY_MUTE %d\n",msg->volume);
                break;
            case EVENT_UAC_RECORD_MUTE:
                if(evt->value_notify) evt->value_notify(evt->user_data, EUSB_RCD_MUTE, msg->volume);
                // wtk_log_log0(glb_log, "EVENT_UAC_RECORD_MUTE");
                // wtk_debug("EVENT_UAC_RECORD_MUTE %d\n",msg->volume);
                break;
            case EVENT_UAC_PPM:
                if(evt->value_notify) evt->value_notify(evt->user_data, EUSB_PPM, msg->volume);
                // wtk_log_log0(glb_log, "EVENT_UAC_PPM");
                // wtk_debug("EVENT_UAC_PPM %d\n",msg->volume);
                break;
            case EVENT_PRODUCTION_MODE:
                if(evt->notify) evt->notify(evt->user_data, EKEY_PRODUCTION_MODE);
                // wtk_log_log0(glb_log, "EVENT_PRODUCTION_MODE");
                // wtk_debug("EVENT_PRODUCTION_MODE \n");
                break;
            default:
                break;
        }
        wtk_lockhoard_push(&evt->msg_hoard,msg);
    }
    while((node=wtk_blockqueue_pop(&evt->msg_list,0,NULL))){
        msg = data_offset2(node,event_trans_msg_t,q_n);
        wtk_lockhoard_push(&evt->msg_hoard,msg);
    }
    return ret;
}

int _netlink_netevent_handler(qtk_dev_event_t *event,wtk_thread_t *th)
{
    int fd, retval;
    char *buf = wtk_malloc(BUFLEN);
    // int len = BUFLEN;

    struct sockaddr_nl addr;
    struct nlmsghdr *nh;
    struct ifinfomsg *ifinfo;
    // struct rtattr *attr;

    fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    // setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &len, sizeof(len));
    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_groups = RTNLGRP_LINK;
    bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    while (1)  
    {
        retval = read(fd, buf, BUFLEN);     //TODO:
        // wtk_log_log(glb_log, "cccccccc %d \n",retval);
        if(retval < 0) break;
        // wtk_log_log(glb_log, "netlink event handler.\n");
        for (nh = (struct nlmsghdr *)buf; NLMSG_OK(nh, retval); nh = NLMSG_NEXT(nh, retval))
        {  
            if (nh->nlmsg_type == NLMSG_DONE){
                // wtk_log_log0(glb_log, "NLMSG_DONE");
                break;
            }else if (nh->nlmsg_type == NLMSG_ERROR){
                // wtk_log_log0(glb_log, "NLMSG_ERROR");
                wtk_free(buf);
                return -1;
            }else if (nh->nlmsg_type != RTM_NEWLINK){
                continue;
            }
            ifinfo = NLMSG_DATA(nh);
            _dev_event_netlink_callback(event, ifinfo, nh);
        }  
    } 
    
    wtk_free(buf);
    close(fd);
    return 0;
}

static int compare(const char* dst, const char* srt)
{
    
    if ((dst == NULL) || (srt == NULL)) {
        
        return 0;
    }
    // 添加长度检查，避免空字符串
    size_t dst_len = strlen(dst);
    size_t srt_len = strlen(srt);
    
    
    if (dst_len == 0 || srt_len == 0) {
       
        return 0;
    }
        
    int m = min(dst_len, srt_len);
   
    // int m = min(strlen(dst),strlen(srt));
    if (!strncmp(dst, srt, m)) {
        
        return 1;
    }
   
    return 0;
}

void usb_event_set_state(qtk_dev_event_t *event, struct _uevent *uevent, int state)
{
    // wtk_log_log(glb_log, "usb_event_set_state >>> state = %d\n", state);

    event_trans_msg_t *msg = NULL;
    wtk_queue_node_t *node = NULL;

    node = wtk_lockhoard_pop(&event->msg_hoard);
    msg = data_offset2(node,event_trans_msg_t,q_n);
    if(state){
        if(!_get_device_usb()){
            wtk_lockhoard_push(&event->msg_hoard, msg);
            // wtk_log_log0(glb_log, "warring: get USB config event, but _get_device_usb == 0.");
            return;
        }
        // wtk_log_log(glb_log, " ============= USB EVENT state = %d, EVENT_UAC_UP\n", state);
        // event->uac_stat = state;
        g_event->uac_stat = state;
        msg->type = EVENT_UAC_UP;
    }else if(!state){
        // wtk_log_log(glb_log, " ============= USB EVENT state = %d, EVENT_UAC_DOWN\n", state);
        // event->uac_stat = 0;
        g_event->uac_stat = 0;
        msg->type = EVENT_UAC_DOWN;
    }else{
        // wtk_log_log(glb_log, " ============= USB EVENT state = %d\n", state);
        wtk_lockhoard_push(&event->msg_hoard, msg);
        return;
    }
    // wtk_log_log(glb_log, "Set usb state = %d\n", state);
    wtk_blockqueue_push(&event->msg_list,&msg->q_n);
    return;
}

static void usb_otgport_change_event(qtk_dev_event_t *event, struct _uevent *uevent)
{
    event_trans_msg_t *msg = NULL;
    wtk_queue_node_t *node = NULL;
    int state = 0;

    state = _get_usb_otgport();
    if(state == -1) return;

    node = wtk_lockhoard_pop(&event->msg_hoard);
    msg = data_offset2(node,event_trans_msg_t,q_n);
    if(state){
        msg->type = EVENT_VBUS_UP;
    }else if(!state){
        msg->type = EVENT_VBUS_DOWN;
    }else{
        wtk_lockhoard_push(&event->msg_hoard, msg);
        return;
    }
    wtk_blockqueue_push(&event->msg_list,&msg->q_n);
    return;
}

static void uac_state_event(qtk_dev_event_t *event, struct _uevent *uevent)
{
    char *state = uevent->strs[UAC_KEY_USB_STATE];
    

    if (state == NULL) {
        
        return;
    }
    

    bool setState = compare(state, UAC_UEVENT_GET_USB_STATE);                   //CONFIGURED
    bool setState_disconnect = compare(state, UAC_UEVENT_GET_USB_DISCONNECT);   //DISCONNECT
    // wtk_log_log(glb_log, "[event] USB event: setState %d, setPower %d, disconnect %d", setState, setPower, setState_disconnect);

    if(setState && event->uac_stat != 1) {
        // wtk_log_log(glb_log, "uac_state_event CHANGE USB\n");
        usb_event_set_state(event, uevent, 1);
    } else if(setState_disconnect && event->uac_stat != 0){
        usb_event_set_state(event, uevent, 0);
    }
}

static void audio_set_interface(qtk_dev_event_t *dev_event,const struct _uevent *uevent)
{
    char *status = uevent->strs[UAC_KEY_STREAM_STATE];
    char *direct = uevent->strs[UAC_KEY_DIRECTION];
    event_trans_msg_t *msg = NULL;
    wtk_queue_node_t *node = NULL;

    if(compare(direct, UAC_STREAM_DIRECT_IN)){
        
        node = wtk_lockhoard_pop(&dev_event->msg_hoard);
        msg = data_offset2(node,event_trans_msg_t,q_n);
        if(compare(status, UAC_STREAM_STATE_ON)){
            msg->type = EVENT_UAC1_ON;
            dev_event->uac1_status = 1;
        }else{
            msg->type = EVENT_UAC1_OFF;
            dev_event->uac1_status = 0;
        }
        wtk_blockqueue_push(&dev_event->msg_list,&msg->q_n);
    } else if(compare(direct, UAC_STREAM_DIRECT_OUT)){
        
    }
    return;
}

static const int volume_map[] = {
    -25600, -17418, -14966, -13473, -12397, -11555, -10863, -10276, -9766, -9315,
    -8911, -8545, -8211, -7903, -7617, -7351, -7102, -6868, -6648, -6439,
    -6241, -6052, -5872, -5700, -5536, -5378, -5226, -5080, -4939, -4803,
    -4672, -4545, -4422, -4303, -4187, -4075, -3965, -3859, -3756, -3655,
    -3557, -3461, -3368, -3276, -3187, -3100, -3015, -2931, -2850, -2770,
    -2691, -2614, -2539, -2465, -2393, -2321, -2251, -2183, -2115, -2049,
    -1984, -1919, -1856, -1794, -1733, -1673, -1614, -1555, -1498, -1441,
    -1385, -1330, -1276, -1222, -1169, -1117, -1066, -1015, -965, -915,
    -866, -818, -771, -723, -677, -631, -586, -541, -496, -452,
    -409, -366, -323, -281, -240, -199, -158, -118, -78, -39, 0
};
//利用二分查找快速锁定数据
int wtk_mapping_volumedata_mapping_percentage(int signed_volume) {
    int left = 0;
    int right = 100;
    int result = 0;
    
    // 处理边界情况
    if (signed_volume <= volume_map[0]) {
        return 0;
    }
    if (signed_volume >= volume_map[100]) {
        return 100;
    }
    
    // 二分查找精确匹配t
    while (left <= right) {
        int mid = left + (right - left) / 2;
        
        if (volume_map[mid] == signed_volume) {
            // 精确匹配，直接返回对应的百分比
            return mid;
        } else if (volume_map[mid] < signed_volume) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    
    // 如果没有精确匹配，left 指向第一个大于 signed_volume 的元素位置
    left = 0;
    right = 100;
    
    while (left <= right) {
        int mid = left + (right - left) / 2;
        
        if (volume_map[mid] >= signed_volume) {
            // 当前值大于等于目标值，继续在右侧寻找更小的值
            left = mid + 1;
        } else {
            // 当前值小于目标值，记录这个位置，继续在左侧寻找
            result = mid;
            right = mid - 1;
        }
    }
    
    if (result < 100) {
        return result + 1;
    } else {
        return 100;
    }
}

static void audio_set_volume(qtk_dev_event_t *dev_event,const struct _uevent *uevent) 
{
    char *direct = uevent->strs[UAC_KEY_DIRECTION];
    char *volumeStr = uevent->strs[UAC_KEY_VOLUME];
    event_trans_msg_t *msg = NULL;
    wtk_queue_node_t *node = NULL;

    // 详细打印原始数据
    /*printf("=== audio_set_volume Debug ===\n");*/
    /*printf("direct=%s\n", direct ? direct : "NULL");*/
    /*printf("volumeStr=%s\n", volumeStr ? volumeStr : "NULL");*/
    /*printf("UAC_KEY_DIRECTION=%d, UAC_KEY_VOLUME=%d\n", UAC_KEY_DIRECTION, UAC_KEY_VOLUME);*/
    
    // 打印所有uevent字符串
    for(int i = 0; i < 30; i++) {
        if(uevent->strs[i] != NULL) {
            /*printf("uevent[%d] = %s\n", i, uevent->strs[i]);*/
        }
    }
    /*printf("=== End audio_set_volume Debug ===\n");*/

    if (compare(direct, UAC_STREAM_DIRECT)) {
        char* device = &direct[strlen(UAC_STREAM_DIRECT)];
        int volume = 100;
        
        // 支持十六进制和十进制格式的音量值解析
        short signed_volume = 0;

        if (strstr(volumeStr, "0x") != NULL || strstr(volumeStr, "0X") != NULL) {
            // 十六进制格式: VOLUME=0xf7bd
            unsigned int hex_volume = 0;
            if (sscanf(volumeStr, "VOLUME=0x%x", &hex_volume) == 1) {
                // UAC音量范围: 通常是有符号16位整数
                signed_volume = (short)hex_volume;
            } else {
                /*wtk_debug("audio_set_volume: Failed to parse hex volume: %s\n", volumeStr);*/
                signed_volume = 0;
            }
        } else {
            // 十进制格式: VOLUME=-1234
            if (sscanf(volumeStr, "VOLUME=%hd", &signed_volume) != 1) {
                /*wtk_debug("audio_set_volume: Failed to parse decimal volume: %s\n", volumeStr);*/
                signed_volume = 0;
            }
        }

        // UAC音量范围需要根据实际设备调整
        // 常见的UAC音量范围：
        // 1. 标准范围: -9600 (静音) 到 0 (最大音量)
        // 2. 扩展范围: -25600 (静音) 到 0 (最大音量)  
        // 3. 其他范围: 需要根据实际测试确定
        int uac_volume_min = -25600;//-12000;  // 扩大静音范围
        int uac_volume_max = 0;        // 最大音量

        // 将有符号音量值映射到0-100百分比
        if (uac_volume_max == uac_volume_min) {
            volume = 50; // 如果范围相同，设为中等音量
        } else if (signed_volume >= uac_volume_max) {
            volume = 100;
        } else if (signed_volume <= uac_volume_min) {
            volume = 0;
        } else {
            // 正向映射：-25600=0%, 0=100%
#if 0
            int numerator = (signed_volume - uac_volume_min) * 100;
            int denominator = uac_volume_max - uac_volume_min;
            volume = numerator / denominator;
#else                    
            volume = wtk_mapping_volumedata_mapping_percentage(signed_volume);
#endif
            //printf("Volume calculation: signed_volume=%d, numerator=%d, denominator=%d, volume=%d\n", 
                //   signed_volume, numerator, denominator, volume);
        }
        //static int iSoundButton=0;
        //int iVolumeTemp;
        //wtk_debug("SoundButton=%d,volumenum=%d\n",iSoundButton++,signed_volume);
        /*wtk_debug("audio_set_volume: signed_volume=%d, range=[%d,%d], converted_percentage=%d\n", 
                 signed_volume, uac_volume_min, uac_volume_max, volume);*/
        //printf("else if(signed_volume>%d\n && signed_volume<%d){return}\n",signed_volume,iSoundButton++);
        
        // 确保音量值在有效范围内
        if (volume < 0) volume = 0;
        if (volume > 100) volume = 100;
        
        //wtk_debug("audio_set_volume: final_volume=%d\n", volume);
		// wtk_log_log(dev_event->log, "------------->>>>>>>>>>>>audio_set_volume=%d!!!",volume);
        
        if (compare(device, UAC_REMOTE_PLAY)) {
            node = wtk_lockhoard_pop(&dev_event->msg_hoard);
            msg = data_offset2(node,event_trans_msg_t,q_n);
            msg->type = EVENT_UAC_VOLUME;
            msg->volume = volume;
            dev_event->uac_volume_temp = volume;
            wtk_blockqueue_push(&dev_event->msg_list,&msg->q_n);
        }
    }
    return;
}

static void audio_set_ppm(qtk_dev_event_t *evt, const struct _uevent *uevent) 
{
    char *ppmStr = uevent->strs[UAC_KEY_PPM];
    event_trans_msg_t *msg = NULL;
    if (compare(ppmStr, "PPM=")) {
        int ppm = 0;
        sscanf(ppmStr, "PPM=%d", &ppm);
        msg = wtk_lockhoard_pop(&evt->msg_hoard);
        msg->type = EVENT_UAC_PPM;
        msg->volume = ppm;
        wtk_blockqueue_push(&evt->msg_list, &msg->q_n);
    }
    return;
}

static void audio_set_mute(qtk_dev_event_t *evt,const struct _uevent *uevent)
{
    char *direct = uevent->strs[UAC_KEY_DIRECTION];
    char *muteStr = uevent->strs[UAC_KEY_MUTE];
    event_trans_msg_t *msg = NULL;
    wtk_queue_node_t *node = NULL;
    if (compare(direct, UAC_STREAM_DIRECT)) {
        char* device = &direct[strlen(UAC_STREAM_DIRECT)];
        int mute = 0;
        sscanf(muteStr, "MUTE=%d", &mute);
        node = wtk_lockhoard_pop(&evt->msg_hoard);
        msg = data_offset2(node,event_trans_msg_t,q_n);
        if (compare(device, UAC_REMOTE_PLAY)) {             // output mute
            msg->type = EVENT_UAC_PLAY_MUTE;
            msg->volume = mute;
            // wtk_log_log(glb_log, "***************************UAC MUTE CHANGE (OUT) = %d", msg->volume);
            // wtk_log_log(evt->log,"------->>>>>>>mute=%d",mute);
        } else if (compare(device, UAC_REMOTE_CAPTURE)) {   // in mute
            msg->volume = mute;
            msg->type = EVENT_UAC_RECORD_MUTE;
            // wtk_log_log(glb_log, "***************************UAC MUTE CHANGE (IN) = %d", msg->volume);
        }else{
            wtk_lockhoard_push(&evt->msg_hoard,msg);
            goto end;
        }
        wtk_blockqueue_push(&evt->msg_list,&msg->q_n);
    }
end:
    return;
}

static void uac_audio_event(qtk_dev_event_t *dev_event,const struct _uevent *uevent)
{
    char *event = uevent->strs[UAC_KEY_USB_STATE];
    char *direct = uevent->strs[UAC_KEY_DIRECTION];
    char *status = uevent->strs[UAC_KEY_STREAM_STATE];
    
    /*printf("=== uac_audio_event Debug ===\n");*/
    /*printf("event = %s\n", event ? event : "NULL");*/
    /*printf("direct = %s\n", direct ? direct : "NULL");*/
    /*printf("status = %s\n", status ? status : "NULL");*/
    /*printf("UAC_KEY_USB_STATE = %d\n", UAC_KEY_USB_STATE);*/
    /*printf("UAC_KEY_DIRECTION = %d\n", UAC_KEY_DIRECTION);*/
    /*printf("UAC_KEY_STREAM_STATE = %d\n", UAC_KEY_STREAM_STATE);*/
    /*printf("=== End uac_audio_event Debug ===\n");*/
    if ((event == NULL) || (direct == NULL) || (status == NULL)) {
        return;
    }

    bool setVolume = compare(event, UAC_UEVENT_SET_VOLUME);
    bool setMute = compare(event, UAC_UEVENT_SET_MUTE);
    bool setPPM = compare(event, UAC_UEVENT_SET_AUDIO_CLK);
    bool setInterface = compare(event, UAC_UEVENT_SET_INTERFACE);

    if(setVolume){
        /*wtk_debug("uac_audio_event: Processing volume event\n");*/
		// wtk_log_log0(dev_event->log, "------------->>>>>>>>>>>>audio_set_volume!!!");
        audio_set_volume(dev_event,uevent);
    }else if(setMute){
		// wtk_log_log0(dev_event->log, "------------->>>>>>>>>>>>audio_set_mute!!!");
        audio_set_mute(dev_event,uevent);
    }else if(setPPM){
        audio_set_ppm(dev_event,uevent);
    }else if(setInterface){
        audio_set_interface(dev_event,uevent);
    }
    return;
}

static void parse_event(qtk_dev_event_t *event, struct _uevent *uevent) 
{
    if (uevent->size <= 0)
        return;
    // wtk_debug("UAC_KEY_AUDIO %s\n",uevent->strs[UAC_KEY_AUDIO]);
    //wtk_log_log(glb_log, "[qtk_dev_event] --- parse_event\n");
    if (compare(uevent->strs[UAC_KEY_AUDIO], UAC_UEVENT_USB)){
        // for(int i = 0; i < 30; i++){
        //     wtk_log_log(glb_log, "uac_state_event:  %d = %s\n", i, uevent->strs[i]);
        // }
        // wtk_log_log0(glb_log, "[event] USB change.\n");
        uac_state_event(event, uevent);
    }
}

static void parse_event2(qtk_dev_event_t *event, struct _uevent *uevent) 
{
    /*printf("=== parse_event2 Debug ===\n");*/
    /*printf("uevent->size = %d\n", uevent->size);*/
    /*printf("UAC_KEY_AUDIO = %d\n", UAC_KEY_AUDIO);*/
    /*printf("UAC_KEY_DIRECTION = %d\n", UAC_KEY_DIRECTION);*/
    /*printf("UAC_KEY_VOLUME = %d\n", UAC_KEY_VOLUME);*/
    
    if (uevent->size <= 0) {
        /*printf("uevent->size <= 0, returning\n");*/
        return;
    }
     // 检查 UAC_KEY_AUDIO 索引是否有效
   
    if (UAC_KEY_AUDIO >= uevent->size) {
        /*printf("UAC_KEY_AUDIO >= uevent->size, returning\n");*/
        return;
    }
    if (uevent->strs[UAC_KEY_AUDIO] == NULL) {
        /*printf("uevent->strs[UAC_KEY_AUDIO] == NULL, returning\n");*/
        return;
    }
    
    /*printf("UAC_KEY_AUDIO content: %s\n", uevent->strs[UAC_KEY_AUDIO]);*/
    /*printf("=== End parse_event2 Debug ===\n");*/
    
   
    // wtk_debug("UAC_KEY_AUDIO %s\n",uevent->strs[UAC_KEY_AUDIO]);
    if (compare(uevent->strs[UAC_KEY_AUDIO], UAC_UEVENT_USB)){
        
        uac_state_event(event, uevent);
    }else if (compare(uevent->strs[UAC_KEY_AUDIO], UAC_UEVENT_AUDIO)) {
         
        uac_audio_event(event, uevent);
    }else if (compare(uevent->strs[UAC_KEY_AUDIO], USB_UEVENT_PHY)) {
        // wtk_log_log0(glb_log, "[qtk_dev_event] --------------- parse_event\n");
        // for(int i = 0; i < 30; i++){
        //     wtk_log_log(glb_log, "uac_state_event:  %d = %s", i, uevent->strs[i]);
        // }
        // wtk_log_log0(glb_log, "[event] USB change.---------------\n");
        
        usb_otgport_change_event(event, uevent);
    }
}

// 屏蔽网口相关的定时器回调
/*
void _timer_powmsg_debounce(void *user_data)
{
    event_trans_msg_t *msg = NULL;
    wtk_queue_node_t *node = NULL;
    qtk_dev_event_t *event = user_data;

    // wtk_log_log(glb_log, "[event] Power msg debounce , pow %d msg\n", event->pow_stat);
    node = wtk_lockhoard_pop(&event->msg_hoard);
    msg = data_offset2(node, event_trans_msg_t, q_n);
    if(event->net0_stat == 0){
        msg->type = EVENT_NET1_DISCONNECT;
    }else{
        msg->type = EVENT_NET1_CONNECT;
    }
    wtk_blockqueue_push(&event->msg_list, &msg->q_n);
    return;
}
*/

// 屏蔽网口相关的定时器回调
/*
void _timer_net0msg_debounce(void *user_data)
{
    event_trans_msg_t *msg = NULL;
    wtk_queue_node_t *node = NULL;
    qtk_dev_event_t *event = user_data;

    // wtk_log_log(glb_log, "[event] NET 0 Debounce end, ----- send event->net0_stat = %d msg\n", event->net0_stat);
    node = wtk_lockhoard_pop(&event->msg_hoard);
    msg = data_offset2(node, event_trans_msg_t, q_n);
    if(event->net0_stat == 0){
        msg->type = EVENT_NET1_DISCONNECT;
    }else{
        msg->type = EVENT_NET1_CONNECT;
    }
    wtk_blockqueue_push(&event->msg_list, &msg->q_n);
    return;
}

void _timer_net1msg_debounce(void *user_data)
{
    event_trans_msg_t *msg = NULL;
    wtk_queue_node_t *node = NULL;
    qtk_dev_event_t *event = user_data;

    // wtk_log_log(glb_log, "[event] NET 1 Debounce end, time = %lf ----- send event->net1_stat = %d msg\n", event->net1_stat, time_get_ms());
    node = wtk_lockhoard_pop(&event->msg_hoard);
    msg = data_offset2(node, event_trans_msg_t, q_n);
    if(event->net1_stat == 0){
        msg->type = EVENT_NET2_DISCONNECT;
    }else{
        msg->type = EVENT_NET2_CONNECT;
    }
    wtk_blockqueue_push(&event->msg_list, &msg->q_n);
    return;
}
*/

int _netlink_usbevent_handler(qtk_dev_event_t *dev,wtk_thread_t *th)
{
    int buf_len = 512;
    int sockfd;
    int i, j, len;
    char *buf = NULL;
    buf = wtk_malloc(buf_len*sizeof(char));
    struct iovec iov;
    struct msghdr msg;
    struct sockaddr_nl sa;
    struct _uevent event = {{0,},};

    memset(&sa, 0, sizeof(sa));
    sa.nl_family = AF_NETLINK;
    sa.nl_groups = NETLINK_KOBJECT_UEVENT;
    sa.nl_pid = 0;
    memset(&msg, 0, sizeof(msg));
    iov.iov_base = (void *)buf;
    iov.iov_len = buf_len;
    msg.msg_name = (void *)&sa;
    msg.msg_namelen = sizeof(sa);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    sockfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_KOBJECT_UEVENT);
    if (sockfd == -1) {
        goto end;
    }

    if (bind(sockfd, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
        goto end;
    }

    while (1) {     // th_run
        event.size = 0;
        len = recvmsg(sockfd, &msg, 0); //TODO:
        if (len < 0) {
            // wtk_log_log0(glb_log, "[event] netlink receive error");
        } else if (len < 32 || len > buf_len) {
            // wtk_log_log0(glb_log, "[event] netlink invalid message");
        } else {
            for (i = 0, j = 0; i < len; i++) {
                if (*(buf + i) == '\0' && (i + 1) != len) {
                    event.strs[j++] = buf + i + 1;
                    event.size = j;
                }
            }
        }
        parse_event(dev, &event);
    }
end:
    wtk_free(buf);
    return 0;
}

qtk_dev_event_t* qtk_dev_event_new(void)
{
    qtk_dev_event_t *event = wtk_malloc(sizeof(*event));

    event->flag = QTK_DEV_ALL_EVENT;
    wtk_blockqueue_init(&event->msg_list);
    wtk_thread_init(&event->trans_th,_trans_handler,event);
    wtk_thread_init(&event->netlink_th,_netlink_netevent_handler,event);
    wtk_thread_init(&event->usb_stat_th,_netlink_usbevent_handler,event);
    wtk_thread_set_name(&event->netlink_th,"netlink_event");
    wtk_thread_set_name(&event->usb_stat_th,"netlink_usb_event");

    wtk_lockhoard_init(&event->msg_hoard,offsetof(event_trans_msg_t,q_n),
        20,event_trans_msg_new,event_trans_msg_delete,event);

    // Init key stat
    event->key_voldown_stat = 0;
    event->key_volup_stat = 0;

    local_ip = wtk_malloc(32*sizeof(char));
    g_event = event;
    _dev_event_init(event);
    return g_event;
}

// 屏蔽网口相关宏定义
// #define NET_ETH0    2
// #define NET_ETH1    3
// #define NET_BR0     4

// #define NET_ETH0_DATA   "eth0"
// #define NET_ETH1_DATA   "eth1"
// #define NET_BR0_DATA    "br0"


int _dev_event_netlink_callback(qtk_dev_event_t *event, struct ifinfomsg *if_info, struct nlmsghdr *nh)
{
    event_trans_msg_t *msg = NULL;
    wtk_queue_node_t *node = NULL;

    struct rtattr *attr;

    int ifi_index;
    unsigned short ifi_type;
    unsigned int ifi_flags;
    unsigned int ifi_mask;
    int len = 20480;

    ifi_index = if_info->ifi_index;
    ifi_type = if_info->ifi_type;
    ifi_flags = if_info->ifi_flags;
    ifi_mask = if_info->ifi_change;

    int event_type = -1;

    attr = (struct rtattr*)(((char*)nh) + NLMSG_SPACE(sizeof(*if_info)));  
    len = nh->nlmsg_len - NLMSG_SPACE(sizeof(*if_info));  
    for (; RTA_OK(attr, len); attr = RTA_NEXT(attr, len))  
    {  
        if (attr->rta_type == IFLA_IFNAME)  
        {  
            // // wtk_log_log(glb_log, "[ qtk_dev_event] RTA_DATA (attr) = %s, time = %lf\n", (char*)RTA_DATA(attr), time_get_ms());
            // if(strcmp(NET_ETH0_DATA, (char*)RTA_DATA(attr)) == 0){
            //     // wtk_log_log(glb_log, "[ qtk_dev_event] netlink eth0 %d, stat %d", ifi_flags, event->net0_stat);
            //     event_type = 0;
            // } else if(strcmp(NET_ETH1_DATA, (char*)RTA_DATA(attr)) == 0){
            //     // wtk_log_log(glb_log, " [ qtk_dev_event] netlink eth1 %d, stat %d", ifi_flags, event->net1_stat);
            //     event_type = 1;
            // } else {
            //     event_type = -1;
            // }
            break;  
        }  
    }

    switch (event_type){
    case 0:
        if(ifi_flags & IFF_LOWER_UP){       // IFF_LOWER_UP >> Connect.
            //wtk_log_log(glb_log, "[ qtk_dev_event] ETH0 up.\n");
            if(event->net0_stat == 0){
                event->net0_stat = 1;
                // wtk_log_log(glb_log, "[ qtk_dev_event] ---------------------- net0 event->net0_stat = %d msg\n", event->net0_stat);

                node = wtk_lockhoard_pop(&event->msg_hoard);
                msg = data_offset2(node, event_trans_msg_t, q_n);
                msg->type = EVENT_NET1_CONNECT;
                wtk_blockqueue_push(&event->msg_list, &msg->q_n);
            }
        }
        else{
            //wtk_log_log(glb_log, "[ qtk_dev_event] ETH0 down.\n");
            if(event->net0_stat == 1){
                event->net0_stat = 0;
                // wtk_log_log(glb_log, "[ qtk_dev_event] ---------------------- net0 event->net0_stat = %d msg\n", event->net0_stat);

                node = wtk_lockhoard_pop(&event->msg_hoard);
                msg = data_offset2(node, event_trans_msg_t, q_n);
                msg->type = EVENT_NET1_DISCONNECT;
                wtk_blockqueue_push(&event->msg_list, &msg->q_n);
            }
        }
        break;
    case 1:
        if(ifi_flags & IFF_LOWER_UP){
            //wtk_log_log(glb_log, "[ qtk_dev_event] eth1 up.\n");
            if(event->net1_stat == 0){
                event->net1_stat = 1;
                // wtk_log_log(glb_log, "[ qtk_dev_event] net1 event->net1_stat = %d msg, time = %lf\n", event->net1_stat, time_get_ms());

                node = wtk_lockhoard_pop(&event->msg_hoard);
                msg = data_offset2(node, event_trans_msg_t, q_n);
                msg->type = EVENT_NET2_CONNECT;
                wtk_blockqueue_push(&event->msg_list, &msg->q_n);
            }
        }
        else{
            //wtk_log_log(glb_log, "[ qtk_dev_event] eth1 up.\n");
            if(event->net1_stat == 1){
                event->net1_stat = 0;
                // wtk_log_log(glb_log, "[ qtk_dev_event] net1 event->net1_stat = %d msg, time = %lf\n", event->net1_stat, time_get_ms());
                node = wtk_lockhoard_pop(&event->msg_hoard);
                msg = data_offset2(node, event_trans_msg_t, q_n);
                msg->type = EVENT_NET2_DISCONNECT;
                wtk_blockqueue_push(&event->msg_list, &msg->q_n);
            }
        }
        break;
    default:
        goto repush_net;
    }
    return 1;
repush_net:
    return 0;
}

#define KEY_BLUETOOTH   (237)   // not used
#define KEY_MUTE        (113)
#define KEY_VOLUMEDOWN  (114)
#define KEY_VOLUMEUP    (115)
#define KEY_FN_F1       (466)   // disc
#define KEY_FN_F2       (467)   // M
#define KEY_48V_POW       (4)
#define KEY_POE_POW       (2)
#define KEY_LINE_OUT    (100)

int _dev_event_key_compose_callback(const char* compose, const uint32_t time)
{
    event_trans_msg_t *msg = NULL;
    wtk_queue_node_t *node = NULL;
    node = wtk_lockhoard_pop(&g_event->msg_hoard);
    msg = data_offset2(node,event_trans_msg_t,q_n);
    // wtk_debug("_dev_event_key_compose_callback: compose = %s, time = %d\n", compose, time);
    // wtk_log_log(glb_log, "_dev_event_key_compose_callback: compose = %s, time = %d", compose, time);
    msg->type = EVENT_PRODUCTION_MODE;
    wtk_blockqueue_push(&g_event->msg_list, &msg->q_n);
    return 0;
}

// int _dev_event_long_press_callback(const int key_code, const uint32_t time)
// {
//     event_trans_msg_t *msg = NULL;
//     wtk_queue_node_t *node = NULL;
//     node = wtk_lockhoard_pop(&g_event->msg_hoard);
//     msg = data_offset2(node,event_trans_msg_t,q_n);
//     wtk_log_log(glb_log, "_dev_event_long_press_callback: code = %d, time %lu", key_code, time);

//     switch(key_code){
//         case KEY_48V_POW:
//             wtk_log_log0(glb_log, "@@@@@@@@@@@@@@@ KEY_48V_POW");
//             if(g_event->flag & QTK_DEV_POWER_EVENT == 0){goto repush_l;}
//             // wtk_log_log(glb_log, "[ qtk_dev_event] 48V_POW EVENT key_code %d, key_value %d", key_code, key_value); 
//             if(_get_device_power_state(1) == 0){
//                 msg->type = EVENT_POWER_48V;
//                 g_event->pow_stat = 2;
//             }else{
//                 msg->type = EVENT_POWER_48V_POE;
//                 g_event->pow_stat = 3;
//             }
//             break;
//         case KEY_POE_POW:
//             wtk_log_log0(glb_log, "@@@@@@@@@@@@@@@ KEY_POE_POW");
//             if(g_event->flag & QTK_DEV_POWER_EVENT == 0){goto repush_l;}
//             // wtk_log_log(glb_log, "[ qtk_dev_event] 48V_POW EVENT key_code %d, key_value %d", key_code, key_value); 
//             if(_get_device_power_state(2) == 0){
//                 msg->type = EVENT_POWER_POE;
//                 g_event->pow_stat = 2;
//             }else{
//                 msg->type = EVENT_POWER_48V_POE;
//                 g_event->pow_stat = 3;
//             }
//             break;
//         default:
//             // wtk_log_log(glb_log, "[ qtk_dev_event] KEY EVENT! key_code = %d, key_value = %d\n\n", key_code, key_value); 
//             goto repush_l;
//             break;
//     }
//     wtk_blockqueue_push(&g_event->msg_list, &msg->q_n);
//     // wtk_log_log(glb_log, "[ qtk_dev_event] KEY EVENT! key_code = %d, key_value = %d\n\n", key_code, key_value);
//     return 0;
// repush_l:
//     wtk_lockhoard_push(&g_event->msg_hoard, msg);
//     return 1;
// }

// 按键处理
int _dev_event_key_callback(const int key_code, const int key_value)
{
    event_trans_msg_t *msg = NULL;
    wtk_queue_node_t *node = NULL;
    node = wtk_lockhoard_pop(&g_event->msg_hoard);
    msg = data_offset2(node,event_trans_msg_t,q_n);
    // wtk_log_log(glb_log, "_dev_event_key_callback: code = %d, value = %d", key_code, key_value);

    switch(key_code){
        case KEY_BLUETOOTH:
            if(g_event->flag & QTK_DEV_KEY_EVENT == 0){goto repush3;}
            if(key_value){
                msg->type = EVENT_KEY_BLUETOOTH;
                // wtk_log_log(glb_log, "[ qtk_dev_event] EVENT_KEY_BLUETOOTH, code = %d, value = %d\n", key_code, key_value);
            }else{ goto repush3; }
            break;
        case KEY_VOLUMEDOWN:
            if(g_event->flag & QTK_DEV_KEY_EVENT == 0){goto repush3;}
            g_event->key_voldown_stat = key_value;
            if(key_value){      // 0 = up, 1 = press
                msg->type = EVENT_KEY_VOLUMEDOWN;
                // wtk_log_log(glb_log, "[ qtk_dev_event] KEY_VOLUME_DOWN, code = %d, value = %d\n", key_code, key_value);
            }else{
                msg->type = EVENT_KEY_VOLUMEDOWN_UNPRESS;
                // wtk_log_log(glb_log, "[ qtk_dev_event] KEY_VOLUME_DOWN, code = %d, value = %d\n", key_code, key_value);
            }
            break;
        case KEY_VOLUMEUP:
            if(g_event->flag & QTK_DEV_KEY_EVENT == 0){goto repush3;}
            g_event->key_volup_stat = key_value;
            if(key_value){
                msg->type = EVENT_KEY_VOLUMEUP;
                // wtk_log_log(glb_log, "[ qtk_dev_event] KEY_VOLUMEUP, code = %d, value = %d\n", key_code, key_value);
            }else{
                msg->type = EVENT_KEY_VOLUMEUP_UNPRESS;
                // wtk_log_log(glb_log, "[ qtk_dev_event] KEY_VOLUMEUP, code = %d, value = %d\n", key_code, key_value);
            }
            break;
        case KEY_MUTE:
            if(g_event->flag & QTK_DEV_KEY_EVENT == 0){goto repush3;}
            if(key_value){
                msg->type = EVENT_KEY_MUTE;
                // wtk_log_log(glb_log, "[ qtk_dev_event] KEY_MUTE, code = %d, value = %d\n", key_code, key_value);
            }else{ goto repush3; }
            break;
        case KEY_FN_F1:
            if(g_event->flag & QTK_DEV_KEY_EVENT == 0){goto repush3;}
            if(key_value){
                msg->type = EVENT_KEY_FN_F1;
            }else{
                msg->type = EVENT_KEY_FN_F1_UNPRESS;
            }
            // wtk_log_log(glb_log, "[ qtk_dev_event] FN_F1 EVENT key_code %d, key_value %d", key_code, key_value); 
            break; 
        case KEY_FN_F2:
            if(g_event->flag & QTK_DEV_KEY_EVENT == 0){goto repush3;}
            if(key_value){
                msg->type = EVENT_KEY_FN_F2;
            }else{
                msg->type = EVENT_KEY_FN_F2_UNPRESS;
            }
            // wtk_log_log(glb_log, "[ qtk_dev_event] FN_F2 EVENT key_code %d, key_value %d", key_code, key_value); 
            break;
        case KEY_48V_POW:
            if(g_event->flag & QTK_DEV_POWER_EVENT == 0){goto repush3;}
            // wtk_log_log(glb_log, "[ qtk_dev_event] 48V_POW EVENT key_code %d, key_value %d", key_code, key_value); 
            if(key_value == 0 && qtk_dev_event_power() == 0){
                msg->type = EVENT_POWER_USB;
                g_event->pow_stat = 0;
            }else if(key_value == 0 && _get_device_power_state(1) == 1){
                msg->type = EVENT_POWER_POE;
                g_event->pow_stat = 1;
            }else if(key_value == 1 && _get_device_power_state(1) == 0){
                msg->type = EVENT_POWER_48V;
                g_event->pow_stat = 2;
            }else if(key_value == 1 && _get_device_power_state(1) == 1){
                msg->type = EVENT_POWER_48V_POE;
                g_event->pow_stat = 3;
            }else{ goto repush3; }
            break;
        case KEY_POE_POW:
            if(g_event->flag & QTK_DEV_POWER_EVENT == 0){goto repush3;}
            // wtk_log_log(glb_log, "[ qtk_dev_event] POE_POW EVENT code %d, value %d", key_code, key_value); 
            if(key_value == 0 && qtk_dev_event_power() == 0){
                msg->type = EVENT_POWER_USB;
                g_event->pow_stat = 0;
            }else if(key_value == 0 && _get_device_power_state(2) == 1){
                msg->type = EVENT_POWER_48V;
                g_event->pow_stat = 2;
            }else if(key_value == 1 && _get_device_power_state(2) == 0){
                msg->type = EVENT_POWER_POE;
                g_event->pow_stat = 1;
            }else if(key_value == 1 && _get_device_power_state(2) == 1){
                msg->type = EVENT_POWER_48V_POE;
                g_event->pow_stat = 3;
            }else{ goto repush3; }
            break;
        default:
            // wtk_log_log(glb_log, "[ qtk_dev_event] KEY EVENT! key_code = %d, key_value = %d\n\n", key_code, key_value); 
            goto repush3;
            break;
    }
    wtk_blockqueue_push(&g_event->msg_list, &msg->q_n);
    // wtk_log_log(glb_log, "[ qtk_dev_event] KEY EVENT! key_code = %d, key_value = %d\n\n", key_code, key_value);
    return 0;
repush3:
    wtk_lockhoard_push(&g_event->msg_hoard, msg);
    return 1;
}

void _timer_uac1_stat_notify(qtk_dev_event_t *event)
{
    int ret = 0;
    event_trans_msg_t *msg = NULL;
    wtk_queue_node_t *node = NULL;

    ret = get_uac1_status();
    if(event->uac_stat == 0) ret = 0;   // 由于录音过程中拔出USB后, get_uac1_status始终为1, 所以这里设为0

    if(ret != event->uac1_status){
        node = wtk_lockhoard_pop(&event->msg_hoard);
        msg = data_offset2(node,event_trans_msg_t,q_n);
        if(ret == 1){
            msg->type = EVENT_UAC1_ON;
            // wtk_log_log0(glb_log, "[ qtk_dev_event] UAC1 ON");
        }
        else{
            msg->type = EVENT_UAC1_OFF;
            // wtk_log_log0(glb_log, "[ qtk_dev_event] UAC1 OFF");
        }
        wtk_blockqueue_push(&event->msg_list,&msg->q_n);
        event->uac1_status = ret;
    }
    // qtk_timer_add(event->timer, 500, event, _timer_uac1_stat_notify);
}

void _timer_notify(qtk_dev_event_t *event)
{
    int ret = 0;
    event_trans_msg_t *msg = NULL;
    wtk_queue_node_t *node = NULL;

    // 屏蔽网口状态检查
    // ret = get_net_stat(0);
    // if(ret != event->net0_stat){
    //     node = wtk_lockhoard_pop(&event->msg_hoard);
    //     msg = data_offset2(node,event_trans_msg_t,q_n);
    //     if(ret == 1)
    //         msg->type = EVENT_NET1_CONNECT;
    //     else
    //         msg->type = EVENT_NET1_DISCONNECT;
    //     event->net0_stat = ret;
    //     wtk_blockqueue_push(&event->msg_list,&msg->q_n);
    // }
    // ret = get_net_stat(1);
    // if(ret != event->net1_stat){
    //     node = wtk_lockhoard_pop(&event->msg_hoard);
    //     msg = data_offset2(node,event_trans_msg_t,q_n);
    //     if(ret == 1)
    //         msg->type = EVENT_NET2_CONNECT;
    //     else
    //         msg->type = EVENT_NET2_DISCONNECT;
    //     event->net1_stat = ret;
    //     wtk_blockqueue_push(&event->msg_list,&msg->q_n);
    // }
    ret = qtk_dev_event_power();
    if(ret != event->pow_stat){
        node = wtk_lockhoard_pop(&event->msg_hoard);
        msg = data_offset2(node,event_trans_msg_t,q_n);
        if(ret == 0)
            msg->type = EVENT_POWER_USB;
        else if(ret == 1)
            msg->type = EVENT_POWER_POE;
        else if(ret == 2)
            msg->type = EVENT_POWER_48V;
        else if(ret == 3)
            msg->type = EVENT_POWER_48V_POE;
        event->pow_stat = ret;
        wtk_blockqueue_push(&event->msg_list,&msg->q_n);
    }
    ret = qtk_dev_event_uac_stat();
    if(ret != event->uac_stat){
        node = wtk_lockhoard_pop(&event->msg_hoard);
        msg = data_offset2(node,event_trans_msg_t,q_n);
        if(ret == 1 && event->uac_stat == 0)
            msg->type = EVENT_UAC_UP;
        else if(ret == 0 && event->uac_stat == 1)
            msg->type = EVENT_UAC_DOWN;
        event->uac_stat = ret;
        wtk_blockqueue_push(&event->msg_list,&msg->q_n);
    }
    // printf("\nCheck state: \n");
    qtk_dev_event_stat_print(g_event);
    // system("cat /sys/kernel/debug/gpio");
    // wtk_log_log(glb_log, "[ qtk_dev_event] _timer_notify, net0 %d, net1 %d, power %d, usb %d.", get_net_stat(0), get_net_stat(1), qtk_dev_event_power(), qtk_dev_event_uac_stat());
    return;
}

void _dev_event_init(qtk_dev_event_t *event)
{
    // 屏蔽网口状态初始化
    // event->net0_stat = get_net_stat(0);
    // event->net1_stat = get_net_stat(1);
    /*wtk_debug("-------------------------------------------\n");*/
    // event->pow_stat = qtk_dev_event_power();
    /*wtk_debug("-------------------------------------------\n");*/
    event->uac_stat = _get_device_usb();
    /*wtk_debug("-------------------------------------------\n");*/
    event->uac1_status = get_uac1_status();
    /*wtk_debug("-------------------------------------------\n");*/
    // wtk_debug("[ qtk_dev_event] Event init: net0 %d, net1 %d, power %d, usb %d, uac1 %d, adb %d\n", 
    //             event->net0_stat, event->net1_stat, event->pow_stat, event->uac_stat, event->uac1_status, get_adb_status());
    // wtk_log_log(glb_log, "[ qtk_dev_event] Event init: net0 %d, net1 %d, power %d, usb %d, uac1 %d, adb %d", 
    //             event->net0_stat, event->net1_stat, event->pow_stat, event->uac_stat, event->uac1_status, get_adb_status());
    qtk_dev_event_stat_print(event);
    /*wtk_debug("-------------------------------------------\n");*/
    return;
}

int qtk_dev_event_start(qtk_dev_event_t *event)
{
    event->th_run = 1;
    wtk_thread_start(&event->trans_th);
    _timer_notify(event);
    wtk_thread_start(&event->usb_stat_th);
    wtk_thread_start(&event->netlink_th);
    return 0;
}

int qtk_dev_event_start2(qtk_dev_event_t *event)
{
    event->th_run = 1;
    /*wtk_debug("-----------------------------------------------------\n");*/
    wtk_thread_start(&event->trans_th);
    /*wtk_debug("-----------------------------------------------------\n");*/
    // _timer_notify(event);
    /*wtk_debug("-----------------------------------------------------\n");*/
    if((event->flag & QTK_DEV_NET_EVENT) || (event->flag & QTK_DEV_USB_EVENT)){
        event->netlink_run = 1;
        pipe(event->netlinkfd);
        wtk_thread_start(&event->netlink_th);
    }
    /*wtk_debug("-----------------------------------------------------\n");*/
    if((event->flag & QTK_DEV_POWER_EVENT) || (event->flag & QTK_DEV_KEY_EVENT)){
       
        if(event->flag & QTK_DEV_KEY_EVENT){
           
        }
    }
    /*wtk_debug("-----------------------------------------------------\n");*/
    return 0;
}

int qtk_dev_event_stop(qtk_dev_event_t *event)
{
    
    wtk_thread_join(&event->netlink_th);
    wtk_thread_join(&event->usb_stat_th);
    event->th_run = 0;
    wtk_blockqueue_wake(&event->msg_list);
    wtk_thread_join(&event->trans_th);
    return 0;    
}

int qtk_dev_event_stop2(qtk_dev_event_t *event)
{
    if((event->flag & QTK_DEV_NET_EVENT) || (event->flag & QTK_DEV_USB_EVENT)){
        event->netlink_run = 0;
        write(event->netlinkfd[1],&event->netlink_run,1);
        wtk_thread_join(&event->netlink_th);
        close(event->netlinkfd[0]);
        close(event->netlinkfd[1]);
    }
    if((event->flag & QTK_DEV_POWER_EVENT) || (event->flag & QTK_DEV_KEY_EVENT)){
    }
    event->th_run = 0;
    wtk_blockqueue_wake(&event->msg_list);
    wtk_thread_join(&event->trans_th);
    return 0;    
}

int qtk_dev_event_delete(qtk_dev_event_t *event)
{
    wtk_lockhoard_clean(&event->msg_hoard);
    wtk_free(event);
    wtk_free(local_ip);
    local_ip = NULL;
    g_event = NULL;
    return 0;
}

int qtk_dev_event_delete2(qtk_dev_event_t *event)
{
    wtk_lockhoard_clean(&event->msg_hoard);
    wtk_free(event);
    wtk_free(local_ip);
    local_ip = NULL;
    g_event = NULL;
    return 0;
}

// 录音中拔出USB ret = 1!
int get_uac1_status(void)
{
    FILE *read_fp;
    char* buffer = NULL;
    buffer = wtk_malloc(256*sizeof(char));
    memset(buffer, 0, 256);

	int ret = 0;
	read_fp = popen("cat /proc/uac1/uac1_status", "r");
	fgets(buffer, 255, read_fp);
	
	if((strstr(buffer, "1"))) ret = 1;
	pclose(read_fp);
    wtk_free(buffer);
	return ret;
}

// 屏蔽网口状态检查函数
/*
//0:eth0 1:eth1
int get_net_stat(int type)
{
    char *buf = NULL;
    int chars_read = 0;
    buf = wtk_malloc(256*sizeof(char));
    int ret = 0;
    sprintf(buf,"ifconfig eth%d | grep RUNNING",type);

    FILE *read_fp = popen(buf, "r");
	if( read_fp != NULL ){
		chars_read = fread(buf, sizeof(char), 255, read_fp);
		// printf("%s \n",buffer);
		if (chars_read > 0)
			ret = 1;    //网线连接
	}
    if(buf) wtk_free(buf);
    pclose(read_fp);
    return ret;
}
*/

// 屏蔽网口状态检查函数
/*
int get_eth_stat(int type)
{
    char *buf = NULL;
    int chars_read = 0;
    buf = wtk_malloc(256*sizeof(char));
    int ret = 0;
    sprintf(buf,"ifconfig eth%d | grep UP", type);

    FILE *read_fp = popen(buf, "r");
	if( read_fp != NULL ){
		chars_read = fread(buf, sizeof(char), 255, read_fp);
		// printf("%s \n",buffer);
		if (chars_read > 0)
			ret = 1;    //网线连接
	}
    if(buf) wtk_free(buf);
    pclose(read_fp);

    #ifdef QSOUND_AM31S_2
    return 1;
    #endif

    return ret;
}
*/

// 屏蔽网口状态检查函数
/*
int get_br0_stat(void)
{
    char *buf = NULL;
    int chars_read = 0;
    buf = wtk_malloc(256*sizeof(char));
    int ret = 0;
    sprintf(buf, "brctl show | grep eth1");

    FILE *read_fp = popen(buf, "r");
	if( read_fp != NULL ){
		chars_read = fread(buf, sizeof(char), 255, read_fp);
		// printf("%s \n",buffer);
		if (chars_read > 0)
			ret = 1;    // br0 包含 eth1
	}
    if(buf) wtk_free(buf);
    pclose(read_fp);

    #ifdef QSOUND_AM31S_2
    return 1;
    #endif
    
    return ret;
}
*/

// usb是否插入
int _get_usb_otgport(void)
{
    FILE *read_fp;
	char* buffer = NULL;
    buffer = wtk_malloc(256*sizeof(char));
    memset(buffer, 0, 256);
	int ret = -1;
	read_fp = popen("cat /sys/devices/platform/ff400000.usb/udc/ff400000.usb/is_otg", "r");
	
	fgets(buffer, 255, read_fp);
	
	if((strstr(buffer, "0"))) ret = 0;
    if((strstr(buffer, "1"))) ret = 1;
	pclose(read_fp);
    wtk_free(buffer);
	return ret;
}

// uac是否可用 1可用 0不可用
int _get_device_usb(void)
{
    int state = 1;
	// char *data = NULL;
	// int len = 0;
	// //android_usb 而是直接有CONNECTED/DISCONNECTED/CONFIGURED/SUSPENDED四个状态
    // data = file_read_buf("/sys/class/android_usb/android0/state", &len);
    // printf("[DEBUG] _get_device_usb: data=%p, len=%d\n", data, len);
    // // 检查指针是否有效
    // if (data == NULL || (unsigned long)data < 0x1000 || (unsigned long)data > 0x7fffffffffff) {
    //     printf("[DEBUG] _get_device_usb: invalid data pointer, returning 1\n");
        
    //     return 1;
    // }
	// if(len > 0 && data != NULL)
	// {
    //     // 安全地打印数据，避免访问无效内存
    //     printf("[DEBUG] _get_device_usb: data length=%d\n", len);
    //     if(len >= (int)strlen("CONFIGURED") && strncmp(data, "CONFIGURED", strlen("CONFIGURED")) == 0)//连接并进行了配置
    //     {
    //         state = 1;
    //     }
	// }
    // if (data == NULL || (unsigned long)data >= 0x1000 || (unsigned long)data <= 0x7fffffffffff) {
    //     printf("[DEBUG] _get_device_usb: invalid data pointer, returning 1\n");
    //     wtk_free(data);
    //     return 1;
    // }
     /*printf("[DEBUG] _get_device_usb: returning state=%d\n", state);*/
    return 1;
}

// usb 是否位于disconnect状态 0 = disconnected
int _get_device_usb2(void)
{
    int state = 1;
	// char *data = NULL;
	// int len = 0;
	// //android_usb 而是直接有CONNECTED/DISCONNECTED/CONFIGURED/SUSPENDED四个状态
    // data = file_read_buf("/sys/class/android_usb/android0/state", &len);
    // printf("[DEBUG] _get_device_usb2: data=%p, len=%d\n", data, len);
    // // 检查指针是否有效
    // if (data == NULL || (unsigned long)data < 0x1000 || (unsigned long)data > 0x7fffffffffff) {
    //     printf("[DEBUG] _get_device_usb2: invalid data pointer, returning 1\n");
       
    //     return 1;
    // }
	// if(len > 0 && data != NULL)
	// {
    //     // if(strncmp(data, "DISCONNECTED", strlen("DISCONNECTED")) == 0) // 未连接
    //     printf("[DEBUG] _get_device_usb2: data='%.*s'\n", len, data);
    //     if(len >= (int)strlen("DISCONNECTED") && strncmp(data, "DISCONNECTED", strlen("DISCONNECTED")) == 0) // 未连接
    //     {
    //         state = 0;
    //     }
	// }
    // if (data == NULL || (unsigned long)data >= 0x1000 || (unsigned long)data <= 0x7fffffffffff) {
    //     printf("[DEBUG] _get_device_usb: invalid data pointer, returning 1\n");
    //     wtk_free(data);
    //     return 1;
    // }
    /*printf("[DEBUG] _get_device_usb: returning state=%d\n", state);*/
    return state;
}

// adb是否处于打开状态 1 = 打开
int get_adb_status(void)
{
    char *buf = NULL;
    int chars_read = 0;
    buf = wtk_malloc(256*sizeof(char));
    int ret = 0;
    sprintf(buf, "grep adb /tmp/.usb_config");

    FILE *read_fp = popen(buf, "r");
	if( read_fp != NULL ){
		chars_read = fread(buf, sizeof(char), 255, read_fp);
		if (chars_read > 0){
            ret = 1;    // 有ADB
            // wtk_log_log0(glb_log, "get_adb_status: ADB open.");
        } else {
            // wtk_log_log0(glb_log, "get_adb_status: ADB close.");
        }
	}
    if(buf) wtk_free(buf);
    pclose(read_fp);
    return ret;
}

// adb flag状态 1 = 打开
int get_adb_flag(void)
{
    FILE *read_fp;
	char* buffer = NULL;
    buffer = wtk_malloc(256*sizeof(char));
    memset(buffer, 0, 256);
	int ret = 1;
	read_fp = popen("cat /oem/qdreamer/qsound/res/adb", "r");
		fgets(buffer, 255, read_fp);
	
	if((strstr(buffer, "0"))) ret = 0;
	pclose(read_fp);
    wtk_free(buffer);
    // wtk_log_log(glb_log, "adb_flag = %d", ret);
	return ret;
}

int get_uac_status(void)
{
    char *buf = NULL;
    int chars_read = 0;
    buf = wtk_malloc(256*sizeof(char));
    int ret = 0;
    sprintf(buf, "cat /tmp/.usb_config | grep uac");

    FILE *read_fp = popen(buf, "r");
	if( read_fp != NULL ){
		chars_read = fread(buf, sizeof(char), 255, read_fp);
		if (chars_read > 0){
            ret = 1;    // UAC打开
            
        } else {
            
        }
	}
    if(buf) wtk_free(buf);
    pclose(read_fp);
    return ret;
}

int get_os_type(void)
{
    FILE *read_fp;
	char* buffer = NULL;
    buffer = wtk_malloc(256*sizeof(char));
    memset(buffer, 0, 256);
	int ret = 7;
    if(access("/sys/kernel/debug/hidg/os_flag", F_OK) != 0){
        ret = -1;
    }
    read_fp = popen("cat /sys/kernel/debug/hidg/os_flag", "r");
	fgets(buffer, 255, read_fp);
	
	if((strstr(buffer, "1"))) ret = 1;
    if((strstr(buffer, "3"))) ret = 3;
	pclose(read_fp);
    wtk_free(buffer);
    // wtk_log_log(glb_log, "get os type = %d\n", ret);
	return ret;
}

int get_is_windows(void)    // cat /sys/kernel/debug/os_desc/is_windows
{
    FILE *read_fp;
	char* buffer = NULL;
    buffer = wtk_malloc(256*sizeof(char));
    memset(buffer, 0, 256);
	int ret = 1;
    
    if(access("/sys/kernel/debug/os_desc/is_windows", F_OK) != 0){
        wtk_free(buffer);
        return -1;
    }

    read_fp = popen("cat /sys/kernel/debug/os_desc/is_windows", "r");
	fgets(buffer, 255, read_fp);
	if((strstr(buffer, "1"))) ret = 1;
    if((strstr(buffer, "0"))) ret = 0;
	pclose(read_fp);
    wtk_free(buffer);
    // wtk_log_log(glb_log, "get is_windows = %d (os type)\n", ret);
	return ret;
}

int qtk_dev_event_uac_stat(void)
{
    return _get_device_usb();
}

//分别判断 供电状态1：poe 2：48v 
int _get_device_power_state(int type)
{
	FILE *read_fp;
	char* buffer = NULL;
    buffer = wtk_malloc(256*sizeof(char));
    memset(buffer, 0, 256);
	int ret = 0;
	if(type == 1)
		read_fp = popen("cat /sys/kernel/debug/gpio | grep POE ", "r");
	else if (type == 2)
		read_fp = popen("cat /sys/kernel/debug/gpio | grep 48v ", "r");
	
	fgets(buffer,255,read_fp);
	// printf("%s \n",buffer);
	
	if((strstr(buffer,"hi")) )
		ret = 1;//高电平，供电
	
	pclose(read_fp);
    wtk_free(buffer);
	return ret;
}

// 0:usb 1:poe 2:48v 3:48v+poe
int qtk_dev_event_power(void)
{
    #ifdef QSOUND_AM31S
    return 2;
    #endif
    int state_poe = 0;
    int state_48v = 0;

    state_poe = _get_device_power_state(1);
    state_48v = _get_device_power_state(2);

    if(state_poe + state_48v == 2){ // wrong
        return 3;
    } else if(state_poe) {          // poe
        return 1;
    } else if(state_48v) {          // 48v
        return 2;
    } else {                        // usb
        return 0;
    }
}

//print now stat
void qtk_dev_event_stat_print(qtk_dev_event_t *ev)
{
#if 0
    if(ev->net0_stat){
        /*printf("eth0 run in\n");*/
        wtk_log_log0(glb_log, "eth0 run in");
    }else{
        /*printf("eth0 break\n");*/
        wtk_log_log0(glb_log, "eth0 break");
    }
    if(ev->net1_stat){
        /*printf("eth1 run in\n");*/
        wtk_log_log0(glb_log, "eth1 run in");
    }else{
        /*printf("eth1 break\n");*/
        wtk_log_log0(glb_log, "eth1 break");
    }
    /*printf("power use ");*/
    wtk_log_log0(glb_log, "power use");
    if(ev->pow_stat == 1){
        /*printf("poe\n");*/
        wtk_log_log0(glb_log, "poe");
    }else if(ev->pow_stat == 2){
        /*printf("48v\n");*/
        wtk_log_log0(glb_log, "48v");
    }else{
        /*printf("usb\n");*/
        wtk_log_log0(glb_log, "usb");
    }
    if(ev->uac_stat){
        /*printf("uac is run in\n");*/
        wtk_log_log0(glb_log, "uac is run in");
    }else{
        /*printf("uac is break\n");*/
        wtk_log_log0(glb_log, "uac is break\n");
    }
#endif
    return;
}

int qtk_dev_event_set_notify(qtk_dev_event_t *event,qtk_dev_event_notify_f cb,void *user_data)
{
    event->notify = cb;
    event->user_data = user_data;
    return 0;
}

int qtk_dev_event_set_value_notify(qtk_dev_event_t *event,qtk_dev_event_value_notify_f cb,void *user_data)
{
    event->value_notify = cb;
    event->user_data = user_data;
    return 0;
}

// 屏蔽网口相关的IP获取函数
/*
#define ETH_NAME "br0"

int get_ip_exists()
{
	FILE *fbr;
	char readbuf[4]={0};
	int len;
	fbr = popen("ifconfig | grep \"br0\"","r");
	len = fread(readbuf, 1, 3, fbr);
    pclose(fbr);
	if(len > 0 && strncmp(readbuf, "br0", strlen("br0")) == 0)
	{
		return 0;
	}else{
		return -1;
	}
}

char* get_local_ip2()
{
	int sock;
	struct sockaddr_in sin;
	struct ifreq ifr;

	if(0 == get_ip_exists())
	{
		sock = socket(AF_INET, SOCK_DGRAM, 0);
		if (sock == -1) {
				// wtk_log_log0(glb_log, "error socket == -1");
				return NULL;
		}
		strncpy(ifr.ifr_name, ETH_NAME, IFNAMSIZ);
		ifr.ifr_name[IFNAMSIZ - 1] = 0;
		if (ioctl(sock, SIOCGIFADDR, &ifr) < 0) {
				// wtk_log_log0(glb_log, "error ioctl < 0");
				return NULL;
		}

		memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
        close(sock);
        return inet_ntoa(sin.sin_addr);
	}else{
		return NULL;
	}
}

char* get_local_ip()
{
    char * ip = get_local_ip2();
    if(ip){
        memset(local_ip,0,32);
        memcpy(local_ip, ip, strlen(ip));
        return local_ip;
    }else{
        return NULL;
    }
}
*/

//new thread
qtk_dev_event_t* qtk_dev_event_new2(unsigned flag)
{
    qtk_dev_event_t *event = wtk_malloc(sizeof(*event));
    int ret = 0;
    /*wtk_debug("------------------------------------------------\n");*/
    memset(event,0,sizeof(*event));
    event->flag = flag;
    wtk_blockqueue_init(&event->msg_list);
    /*wtk_debug("------------------------------------------------\n");*/
    wtk_thread_init(&event->trans_th,_trans_handler,event);
    /*wtk_debug("------------------------------------------------\n");*/
    wtk_thread_init(&event->netlink_th,_netlink_event_process,event);
    /*wtk_debug("------------------------------------------------\n");*/
    wtk_thread_set_name(&event->netlink_th,"netlink_event");
    /*wtk_debug("------------------------------------------------\n");*/
    wtk_lockhoard_init(&event->msg_hoard,offsetof(event_trans_msg_t,q_n),
        40,event_trans_msg_new,event_trans_msg_delete,event);
    /*wtk_debug("------------------------------------------------\n");*/
    // Init key stat
    event->log = wtk_log_new("/tmp/uac_volume.log");
    event->key_voldown_stat = 0;
    event->key_volup_stat = 0;
    event->uac_volume_temp = 50;

    local_ip = wtk_malloc(32*sizeof(char));
    g_event = event;

    //_dev_event_init(event);
    /*wtk_debug("------------------------------------------------\n");*/
    return g_event;
}

struct epoll_event* _usb_epoll_event_create(void)
{
    struct epoll_event* ev = NULL;
    int sockfd;
    struct sockaddr_nl sa;
    _dev_epoll_event_t *action = NULL;

    ev = wtk_malloc(sizeof(*ev));
    memset(ev,0,sizeof(*ev));

    sockfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_KOBJECT_UEVENT);
    memset(&sa, 0, sizeof(sa));
    sa.nl_family = AF_NETLINK;
    sa.nl_groups = NETLINK_KOBJECT_UEVENT;
    sa.nl_pid = 0;
    bind(sockfd, (struct sockaddr *)&sa, sizeof(sa));

    action = wtk_malloc(sizeof(*action));
    action->read_process = _usb_epoll_event_read;
    action->fd = sockfd;
    ev->data.ptr = action;

    return ev;
}

void _usb_epoll_event_read(qtk_dev_event_t *dev,int fd)
{
    struct _uevent event={{0,},};
    int len,i,j;
    struct msghdr msg;
    struct sockaddr_nl sa;
    struct iovec iov;
    int buf_len = 512;
    char *buf = NULL;
    
    memset(event.strs,0,sizeof(event.strs));
    event.size = 0;
    
    //buf = wtk_malloc(buf_len*sizeof(char));
    buf = wtk_malloc(buf_len*sizeof(char));
    if (buf == NULL) {
       
        return;
    }
    
    
    iov.iov_base = (void *)buf;
    iov.iov_len = buf_len;
    
    memset(&sa, 0, sizeof(sa));
    sa.nl_family = AF_NETLINK;
    sa.nl_groups = NETLINK_KOBJECT_UEVENT;
    sa.nl_pid = 0;
    
    memset(&msg, 0, sizeof(msg));
    msg.msg_name = (void *)&sa;
    msg.msg_namelen = sizeof(sa);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    

    len = recvmsg(fd, &msg, 0); //TODO:
    
    if (len < 0) {
        
    } else if (len < 32 || len > buf_len || buf == NULL) {
        
    } else {
        // 打印原始接收到的数据
        /*printf("=== Raw uevent data received ===\n");*/
        /*printf("Received %d bytes from kernel\n", len);*/
        /*printf("Raw buffer content:\n");*/
        for(int k = 0; k < len && k < 200; k++) {
            //printf("%c", buf[k] ? buf[k] : '\\0');
        }
        //printf("\n=== End raw data ===\n");
        //printf("\n");
        
        for (i = 0, j = 0; i < len && j < MAX_UEVENT_STRINGS; i++) {
            if (*(buf + i) == '\0' && (i + 1) != len) {
                event.strs[j++] = buf + i + 1;
                event.size = j;
                
            }
        }
        
        // 打印解析后的字符串数组
        /*printf("=== Parsed uevent strings ===\n");*/
        for(int k = 0; k < event.size; k++) {
            /*printf("event.strs[%d] = %s\n", k, event.strs[k]);*/
        }
        /*printf("=== End parsed strings ===\n");*/
        
    }
    
    parse_event2(dev, &event);
    if(buf){
        wtk_free(buf);
    }
    
    return;
}

// 屏蔽网口相关的epoll创建函数
/*
struct epoll_event* _netevent_epoll_create(void)
{
    struct epoll_event *ev;
    int fd;
    struct sockaddr_nl addr;
    _dev_epoll_event_t *action = NULL;

    ev = wtk_malloc(sizeof(*ev));
    memset(ev,0,sizeof(*ev));

    fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_groups = RTNLGRP_LINK;
    bind(fd, (struct sockaddr*)&addr, sizeof(addr));

    action = wtk_malloc(sizeof(*action));
    action->read_process = _netevent_epoll_read_process;
    action->fd = fd;
    ev->data.ptr = action;

    return ev;
}
*/

// 屏蔽网口相关的netlink事件处理函数
/*
void _netevent_epoll_read_process(qtk_dev_event_t *event,int fd)
{
    int retval;
    struct nlmsghdr *nh;
    struct ifinfomsg *ifinfo;
    char *buf = wtk_malloc(BUFLEN);

    retval = read(fd, buf, BUFLEN);     //TODO:
    // wtk_log_log(glb_log, "cccccccc %d \n",retval);
    if(retval < 0) return;
    // wtk_log_log(glb_log, "netlink event handler.\n");
    for (nh = (struct nlmsghdr *)buf; NLMSG_OK(nh, retval); nh = NLMSG_NEXT(nh, retval))
    {  
        if (nh->nlmsg_type == NLMSG_DONE){
            // wtk_log_log0(glb_log, "NLMSG_DONE");
            break;
        }else if (nh->nlmsg_type == NLMSG_ERROR){
            // wtk_log_log0(glb_log, "NLMSG_ERROR");
            if(buf) wtk_free(buf);
            return -1;
        }else if (nh->nlmsg_type != RTM_NEWLINK){
            continue;
        }
        ifinfo = NLMSG_DATA(nh);
        _dev_event_netlink_callback(event, ifinfo, nh);
    }
    if(buf) wtk_free(buf);
    return;
}
*/

int _netlink_event_process(qtk_dev_event_t *evt,wtk_thread_t *t)
{
    struct epoll_event *ev = NULL,*eva = NULL;
    struct epoll_event *ev1 = NULL,*ev2 = NULL;
    int max_event = 0;
    int ready = 0,i = 0;
    int epfd = epoll_create(10);
    _dev_epoll_event_t *action = NULL;
    int ret = 0;

    ev = wtk_malloc(sizeof(*ev));
    action = wtk_malloc(sizeof(*action));
    action->fd = evt->netlinkfd[0];
    ev->data.ptr = action;
    ev->events = EPOLLIN;
    ret = epoll_ctl(epfd,EPOLL_CTL_ADD,action->fd,ev);
    if(ret < 0){
        perror( "epoll_ctl");
        /*wtk_debug("errno %d\n",errno);*/
    }
    max_event += 1;
    // 屏蔽网口事件处理
    // if(evt->flag & QTK_DEV_NET_EVENT){
    //     ev1 = _netevent_epoll_create();
    //     ev1->events = EPOLLIN;
    //     action = ev1->data.ptr;
    //     ret = epoll_ctl(epfd,EPOLL_CTL_ADD,action->fd,ev1);
    //     if(ret < 0){
    //         perror( "epoll_ctl");
    //         wtk_debug("errno %d\n",errno);
    //     }
    //     max_event += 1;
    // }
    if(evt->flag & QTK_DEV_USB_EVENT){
        ev2 = _usb_epoll_event_create();
        ev2->events = EPOLLIN;
        action = ev2->data.ptr;
        ret = epoll_ctl(epfd,EPOLL_CTL_ADD,action->fd,ev2);
        if(ret < 0){
            perror( "epoll_ctl");
            /*wtk_debug("errno %d\n",errno);*/
        }
        max_event += 1;
    }
    eva = wtk_malloc(sizeof(*ev)*max_event);
    while(evt->netlink_run){
        ready = epoll_wait(epfd,eva,max_event,-1);
        if(ready == -1){
            continue;
        }
        for(i = 0; i < ready; ++i){
            action = eva[i].data.ptr;
            if(action->fd == evt->netlinkfd[0]){
                break;
            }else{
                action->read_process(evt,action->fd);
            }
        }
    }
    if(ev){
        action = ev->data.ptr;
        wtk_free(action);
        wtk_free(ev);
    }
    if(ev1){
        action = ev1->data.ptr;
        close(action->fd);
        free(action);
        free(ev1);
    }
    if(ev2){
        action = ev2->data.ptr;
        close(action->fd);
        free(action);
        free(ev2);
    }
    return 0;
}
