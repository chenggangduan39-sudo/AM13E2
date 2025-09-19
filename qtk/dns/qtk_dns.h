#ifndef QTK_MISC_DNS_QTK_DNS
#define QTK_MISC_DNS_QTK_DNS

#include "wtk/os/wtk_log.h"

#include "qtk_dns_cfg.h"
#include "dnsc/qtk_dnsc.h"

#ifdef WIN32
#include <winSock2.h>
#include <windows.h>
#else
#include <signal.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#endif

#include <string.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_dns qtk_dns_t;
struct qtk_dns
{
	qtk_dns_cfg_t *cfg;
	wtk_log_t *log;
	qtk_dnsc_t *dnsc;
	struct sockaddr *addr;
	int addrlen;
};

qtk_dns_t* qtk_dns_new(qtk_dns_cfg_t *cfg,wtk_log_t *log);
void qtk_dns_delete(qtk_dns_t *dns);

int qtk_dns_process(qtk_dns_t *dns,char *host,int hlen,char *port,int plen);

#ifdef __cplusplus
};
#endif
#endif
