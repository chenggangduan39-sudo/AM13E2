#ifndef __QTK_WIFI_H__
#define __QTK_WIFI_H__

#include "qtk_wifi_cfg.h"
#include "inc/wifi_intf.h"
#include "inc/wifi_udhcpc.h"

typedef enum{
	QTK_WIFI_CONNECT,
	QTK_WIFI_DISCONNECT,
}qtk_wifi_state_t;
typedef void(*qtk_wifi_noitfy_f)(void *ths, qtk_wifi_state_t state);

typedef struct qtk_wifi{
	qtk_wifi_cfg_t *cfg;
	const aw_wifi_interface_t *wifi_interface;
	void *ths;
	qtk_wifi_noitfy_f notify;

}qtk_wifi_t;

qtk_wifi_t *qtk_wifi_new(qtk_wifi_cfg_t *cfg);
void qtk_wifi_delete(qtk_wifi_t *wifi);
void qtk_wifi_connect(qtk_wifi_t *wifi);
void qtk_wifi_connect2(qtk_wifi_t *wifi, char *ssid, char *passwd);
void qtk_wifi_reconnect(qtk_wifi_t *wifi);
void qtk_wifi_disconnect(qtk_wifi_t *wifi);
void qtk_wifi_set_notify(qtk_wifi_t *wifi, void *ths, qtk_wifi_noitfy_f notify);
#endif
