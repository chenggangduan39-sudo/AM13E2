#include "qtk_usrid.h"
#include <errno.h>
#ifdef WIN32
#include <windef.h>
#include <Nb30.h>

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"netapi32.lib")
#endif

#ifdef WIN32
int qtk_usrid_getdeviceId(qtk_usrid_t *u)
{
	wtk_strbuf_t *buf = u->usrid;
	NCB ncb;
	typedef struct _ASTAT_
	{
		ADAPTER_STATUS adapt;
		NAME_BUFFER nameBuff[30];
	}ASTAT, *PASTAT;

	ASTAT Adapter;
	typedef struct _LANA_ENUM
	{
		UCHAR length;
		UCHAR lana[MAX_LANA];
	}LANA_ENUM;

	LANA_ENUM lana_enum;
	UCHAR uRetCode;
	int ret;

	memset(&ncb, 0, sizeof(ncb));
	memset(&lana_enum, 0, sizeof(lana_enum));
	ncb.ncb_command = NCBENUM;
	ncb.ncb_buffer = (unsigned char*)&lana_enum;
	ncb.ncb_length = sizeof(LANA_ENUM);
	uRetCode = Netbios(&ncb);
	if (uRetCode != NRC_GOODRET) {
		wtk_log_warn(u->session->log,"Netbios uRetCode = %d.",uRetCode);
		return -1;
	}

	for (int lana = 0; lana < lana_enum.length; lana++)
	{
		ncb.ncb_command = NCBRESET;
		ncb.ncb_lana_num = lana_enum.lana[lana];
		uRetCode = Netbios(&ncb);
		if (uRetCode == NRC_GOODRET)
		{
			break;
		}
	}
	if (uRetCode != NRC_GOODRET)
	{
		wtk_log_warn(u->session->log,"Netbios uRetCode = %d.",uRetCode);
		return -1;
	}

	memset(&ncb, 0, sizeof(ncb));
	ncb.ncb_command = NCBASTAT;
	ncb.ncb_lana_num = lana_enum.lana[0];
	strcpy((char*)ncb.ncb_callname, "*");
	ncb.ncb_buffer = (unsigned char*)&Adapter;
	ncb.ncb_length = sizeof(Adapter);
	uRetCode = Netbios(&ncb);
	if (uRetCode != NRC_GOODRET) {
		wtk_log_warn(u->session->log,"Netbios uRetCode = %d.",uRetCode);
		return -1;
	}

	wtk_strbuf_push_f(buf,"%02X%02X%02X%02X%02X%02X",
			Adapter.adapt.adapter_address[0],
			Adapter.adapt.adapter_address[1],
			Adapter.adapt.adapter_address[2],
			Adapter.adapt.adapter_address[3],
			Adapter.adapt.adapter_address[4],
			Adapter.adapt.adapter_address[5]
			);
	return 0;
}
#else

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
void port_status(unsigned int flags)
{
	if(flags & IFF_UP) {
       printf("is up\n");
	}

	if(flags & IFF_BROADCAST) {
       printf("is broadcast\n");
	}

	if(flags & IFF_LOOPBACK) {
       printf("is loop back\n");
	}

	if(flags & IFF_POINTOPOINT) {
       printf("is point to point\n");
	}

	if(flags & IFF_RUNNING) {
       printf("is running\n");
	}

	if(flags & IFF_PROMISC) {
       printf("is promisc\n");
	}
}

int qtk_usrid_getdeviceId(qtk_usrid_t *u)
{
#define QTK_USRID_MAX_INTERFACE 16
	int sockfd;
	struct ifconf ifcf;
	struct ifreq ifreq[QTK_USRID_MAX_INTERFACE];
	int i,n;
	int ret = -1;

#ifndef __caddr_t
	typedef char *__caddr_t;
#endif

	ifcf.ifc_len = sizeof(ifreq);
	ifcf.ifc_ifcu.ifcu_buf = (__caddr_t)ifreq;

	sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(sockfd < 0) {
		goto end;
	}

	ret = ioctl(sockfd,SIOCGIFCONF,&ifcf);
	if(ret != 0) {
		goto end;
	}

	n = ifcf.ifc_len / sizeof(struct ifreq);

	wtk_log_log(u->session->log,"network interface nums = %d.",n);
	for(i=0;i<n;++i) {
		wtk_log_log(u->session->log,"%d\t%s.",i,ifreq[i].ifr_ifrn.ifrn_name);
	}

	wtk_strbuf_reset(u->usrid);

	for(i=0;i<n;++i){
		wtk_log_log(u->session->log,"process %d / %s",i,ifreq[i].ifr_ifrn.ifrn_name);
		ret = ioctl(sockfd,SIOCGIFFLAGS,(char*)&ifreq[i]);
		if(ret) {
			wtk_log_warn0(u->session->log,"get SIOCGIFFLAGS failed.");
			continue;
		}

		//port_status(ifreq[i].ifr_ifru.ifru_flags);

		if(ifreq[i].ifr_ifru.ifru_flags & IFF_LOOPBACK) {
			wtk_log_log0(u->session->log,"interface loopback");
			continue;
		}

		ret = ioctl(sockfd,SIOCGIFHWADDR,(char*)&ifreq[i]);
		if(ret) {
			continue;
		}

		wtk_strbuf_push_f(u->usrid,"%02x:%02x:%02x:%02x:%02x:%02x",
				(unsigned   char)ifreq[i].ifr_hwaddr.sa_data[0],
				(unsigned   char)ifreq[i].ifr_hwaddr.sa_data[1],
				(unsigned   char)ifreq[i].ifr_hwaddr.sa_data[2],
				(unsigned   char)ifreq[i].ifr_hwaddr.sa_data[3],
				(unsigned   char)ifreq[i].ifr_hwaddr.sa_data[4],
				(unsigned   char)ifreq[i].ifr_hwaddr.sa_data[5]
				);
		wtk_log_log(u->session->log,"%s / %.*s",ifreq[i].ifr_ifrn.ifrn_name,u->usrid->pos,u->usrid->data);
		break;
	}

	if(u->usrid->pos <= 0) {
		ret = -1;
		goto end;
	}

	ret = 0;
end:
	if(sockfd >= 0) {
		close(sockfd);
	}
	return ret;
}

int qtk_usrid_getdeviceId2(qtk_usrid_t *u)
{
	FILE *f;
	wtk_strbuf_t *buf = u->usrid;
	char mac[QTK_USRID_BUF_LENGTH];
	int ret = -1;

	f = fopen(QTK_USRID_WLAN_FN, "rb");
	if (!f){
		wtk_log_warn(u->session->log,"wlan fn [%s] not exist  errstr:%s",QTK_USRID_WLAN_FN, strerror(errno));
		f = fopen(QTK_USRID_ETH_FN, "rb");
	}
	if (!f){
		wtk_log_warn(u->session->log,"eth fn [%s] not exist,errstr:%s",QTK_USRID_ETH_FN,strerror(errno));
		f = fopen(QTK_USRID_USB_FN, "rb");
	}
	if (!f) {
		wtk_log_warn(u->session->log,"usb fn [%s] not exist,errstr:%s",QTK_USRID_USB_FN,strerror(errno));
		f = fopen(QTK_USRID_ENS33_FN,"rb");
	}
	if (!f){
		wtk_log_warn(u->session->log,"ens33 fn [%s] not exist,errstr:%s",QTK_USRID_ENS33_FN,strerror(errno));
		goto end;
	}


	wtk_strbuf_reset(buf);
	while(1) {
		ret = fread(mac, 1, QTK_USRID_BUF_LENGTH, f);
		if(ret <= 0) {
			break;
		}
		wtk_strbuf_push(buf,mac,ret);
	}
	if(buf->pos <= 0) {
		ret = -1;
		goto end;
	}

	if(buf->data[buf->pos-1] == '\n') {
		--buf->pos;
	}

	ret = 0;
end:
	if (f){
		fclose(f);
	}
	return ret;
}
#endif

void qtk_usrid_init(qtk_usrid_t *u)
{
	u->session = NULL;
	u->usrid = NULL;
}

qtk_usrid_t* qtk_usrid_new(qtk_session_t *session)
{
	qtk_usrid_t *u;

	u = (qtk_usrid_t*)wtk_malloc(sizeof(qtk_usrid_t));
	qtk_usrid_init(u);

	u->session = session;
	u->usrid = wtk_strbuf_new(QTK_USRID_BUF_LENGTH,1);
	return u;
}

void qtk_usrid_delete(qtk_usrid_t *u)
{
	if(u->usrid) {
		wtk_strbuf_delete(u->usrid);
	}
	wtk_free(u);
}

wtk_string_t qtk_usrid_get(qtk_usrid_t *u)
{
	wtk_string_t v;
	char sha1[64];
	int ret;

	ret = qtk_usrid_getdeviceId(u);
#ifndef WIN32
	if(ret != 0) {
		ret = qtk_usrid_getdeviceId2(u);
	}
#endif

	if(ret != 0) {
		wtk_string_set(&v,0,0);
	} else {
		wtk_log_log(u->session->log,"mac = %.*s",u->usrid->pos,u->usrid->data);
		QTK_SHA1_hex(sha1,(const char*)u->usrid->data,u->usrid->pos);
		wtk_strbuf_reset(u->usrid);
		wtk_strbuf_push(u->usrid,sha1,32);
		wtk_string_set(&v,u->usrid->data,u->usrid->pos);
	}
	return v;
}
