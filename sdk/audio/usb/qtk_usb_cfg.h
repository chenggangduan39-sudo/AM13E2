#ifndef SDK_AUDIO_USB_QTK_USB_CFG
#define SDK_AUDIO_USB_QTK_USB_CFG

#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/wtk_array.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_usb_cfg qtk_usb_cfg_t;
struct qtk_usb_cfg
{
	int vendor_id;
	int product_id;
	int in_point;
	int out_point;
	int buf_time;
	int buf_bytes;
	int sample_rate;
	int bytes_per_sample;
	int channel;
	int *skip_channels;
	int nskip;
	int rcd_cache;
	int ply_cache;
	int ply_step;
	int timeout;
	int mic_gain;   //176 - 254
	int	cb_gain;    //176 - 254
	int ply_volume; //144 - 169
	unsigned use_adjust:1;
	unsigned debug:1;
	unsigned use_asy_rcd:1;
	unsigned use_asy_ply:1;
	unsigned use_3_cb_start:1;
	unsigned use_hotplug:1;
};

int qtk_usb_cfg_init(qtk_usb_cfg_t *cfg);
int qtk_usb_cfg_clean(qtk_usb_cfg_t *cfg);
int qtk_usb_cfg_update_local(qtk_usb_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_usb_cfg_update(qtk_usb_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
