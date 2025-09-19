#ifndef QTK_MISC_DNS_QTK_DNSC
#define QTK_MISC_DNS_QTK_DNSC

#if defined(WIN32) || defined(_WIN32)
#include <winsock2.h>
#include <io.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include "wtk/core/wtk_strbuf.h"
#include "wtk/core/wtk_queue.h"
#include "wtk/core/wtk_str.h"

#ifdef __cplusplus
extern "C" {
#endif

#define QTK_DNS_SVR_PORT 53

#ifdef __ANDROID__
#define QTK_DNS_RCV_TIMEOUT 1000
#define QTK_DNS_SND_TIMEOUT 0
#elif __IPHONE_OS__
#define QTK_DNS_RCV_TIMEOUT 1000
#define QTK_DNS_SND_TIMEOUT 0
#elif defined(WIN32) || defined(_WIN32)
#define QTK_DNS_RCV_TIMEOUT 500
#define QTK_DNS_SND_TIMEOUT 100
#else
#define QTK_DNS_RCV_TIMEOUT 500
#define QTK_DNS_SND_TIMEOUT 0
#endif

#define QTK_UDP_PACKAGE_LEN_MAX 1480
#define QTK_DNS_DOMAIN_LEN_MAX 255

typedef struct qtk_dnsc qtk_dnsc_t;

typedef enum {
	QTK_DNS_IPV4,
	QTK_DNS_IPV6,
	QTK_DNS_CNAME,
	QTK_DNS_NAMESVR,
}qtk_dns_type_t;

typedef struct {
	wtk_queue_node_t q_n;
	qtk_dns_type_t type;
	time_t ttl;
	wtk_string_t *name;
	wtk_string_t *rlt;
}qtk_dnsc_item_t;

struct qtk_dnsc
{
	wtk_strbuf_t *buf;
	wtk_queue_t rlt_q;
};

qtk_dnsc_t* qtk_dnsc_new();
void qtk_dnsc_delete(qtk_dnsc_t *dns);

wtk_string_t qtk_dnsc_process(qtk_dnsc_t *dns,char *domain,int len);
void qtk_dnsc_reset(qtk_dnsc_t *dns);

void qtk_dnsc_print(qtk_dnsc_t *dns);

#ifdef __cplusplus
};
#endif
#endif
