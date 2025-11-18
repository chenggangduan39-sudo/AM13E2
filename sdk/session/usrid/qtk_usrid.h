#ifndef QTK_SESSION_USRID_QTK_USRID
#define QTK_SESSION_USRID_QTK_USRID

#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/core/wtk_str.h"
#include "wtk/core/sha1.h"
#include "wtk/os/wtk_log.h"

#include "sdk/session/qtk_session.h"

#ifdef __cplusplus
extern "C" {
#endif

#define QTK_USRID_BUF_LENGTH 256

#ifndef WIN32
#define QTK_USRID_WLAN_FN "/sys/class/net/wlan0/address"
#define QTK_USRID_ETH_FN  "/sys/class/net/eth0/address"
#define QTK_USRID_USB_FN  "/sys/class/net/usb0/address"
#define QTK_USRID_ENS33_FN "/sys/class/net/ens33/address"
#endif

typedef struct qtk_usrid {
	qtk_session_t *session;
	wtk_strbuf_t *usrid;
}qtk_usrid_t;

qtk_usrid_t* qtk_usrid_new(qtk_session_t *session);
void qtk_usrid_delete(qtk_usrid_t *u);

wtk_string_t qtk_usrid_get(qtk_usrid_t *u);

#ifdef __cplusplus
};
#endif
#endif
