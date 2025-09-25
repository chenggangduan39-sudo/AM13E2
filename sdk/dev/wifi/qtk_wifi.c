#include "qtk_wifi.h"

void qtk_wifi_event_cb(struct Manager *w, int event_label);

qtk_wifi_t *qtk_wifi_new(qtk_wifi_cfg_t *cfg)
{
	qtk_wifi_t *wifi;
	int event_latel = 0;
	int ret;

	wifi = (qtk_wifi_t *)wtk_calloc(1, sizeof(*wifi));
	event_latel = rand();
	wifi->wifi_interface = aw_wifi_on(qtk_wifi_event_cb,  event_latel);
	if(!wifi->wifi_interface){
		wtk_debug("wifi on failed.\n");
		ret = -1;
		goto end;
	}
	ret = 0;
end:
	if(ret != 0){
		qtk_wifi_delete(wifi);
		wifi = NULL;
	}
	return wifi;
}

void qtk_wifi_delete(qtk_wifi_t *wifi)
{
	if(wifi->wifi_interface){
		aw_wifi_off(wifi->wifi_interface);
	}
	wtk_free(wifi);
}

void qtk_wifi_connect(qtk_wifi_t *wifi)
{
	int event_label = 1;
	wifi->wifi_interface->connect_ap(wifi->cfg->ssid.data, wifi->cfg->passwd.data, event_label);
}

void qtk_wifi_connect2(qtk_wifi_t *wifi, char *ssid, char *passwd)
{
	int event_label = 1;
	wifi->wifi_interface->connect_ap(ssid, passwd, event_label);
}
void qtk_wifi_reconnect(qtk_wifi_t *wifi)
{
	int event_label = 2;
	wifi->wifi_interface->connect_ap_auto(event_label);
}

void qtk_wifi_disconnect(qtk_wifi_t *wifi)
{
	int event_label = 3;
	wifi->wifi_interface->disconnect_ap(event_label);
}

void qtk_wifi_set_notify(qtk_wifi_t *wifi, void *ths, qtk_wifi_noitfy_f notify)
{
	wifi->ths = ths;
	wifi->notify = notify;
}

void qtk_wifi_event_cb(struct Manager *w, int event_label)
{
	switch(w->StaEvt.state)
	{
		 case CONNECTING:
			 wtk_debug("Connecting to the network(%s)......\n",w->ssid);
			 break;
		 case CONNECTED:
			 wtk_debug("Connected to the AP(%s)\n",w->ssid);
			 start_udhcpc();
			 break;
		 case OBTAINING_IP:
			 wtk_debug("Getting ip address(%s)......\n",w->ssid);
			 break;
		 case NETWORK_CONNECTED:
			 wtk_debug("Successful network connection(%s)\n",w->ssid);
			 break;
		case DISCONNECTED:
			wtk_debug("Disconnected,the reason:%s\n",wmg_event_txt(w->StaEvt.event));
			break;
	}
}
