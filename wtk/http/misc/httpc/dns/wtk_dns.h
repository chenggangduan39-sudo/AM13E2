#ifndef WTK_HTTP_MISC_HTTPC_DNS_WTK_DNS
#define WTK_HTTP_MISC_HTTPC_DNS_WTK_DNS

#include "wtk_dns_cfg.h"
#include "wtk_dns_cache.h"

#include "qtk/dns/dnsc/qtk_dnsc.h"

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
typedef struct wtk_dns wtk_dns_t;
struct wtk_dns
{
	wtk_dns_cfg_t *cfg;
	wtk_log_t *log;
	qtk_dnsc_t *client;
	wtk_dns_cache_t *cache;
	struct sockaddr *addr;
	int addrlen;
};

wtk_dns_t* wtk_dns_new(wtk_dns_cfg_t *cfg,wtk_log_t *log);
void wtk_dns_delete(wtk_dns_t *dns);

int wtk_dns_process(wtk_dns_t *dns,char *host,int hlen,char *port,int plen);
void wtk_dns_clean_cache(wtk_dns_t *dns,char *host,int len);

#ifdef __cplusplus
};
#endif
#endif
