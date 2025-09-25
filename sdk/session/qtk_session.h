#ifndef QTK_SESSION_QTK_SESSION
#define QTK_SESSION_QTK_SESSION

#include "wtk/core/wtk_type.h"
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/os/wtk_thread.h"
#include "wtk/os/wtk_log.h"
#include "wtk/os/wtk_lock.h"

#include "qtk/core/qtk_timer.h"

#include "sdk/session/errcode/qtk_errcode.h"

#ifdef __cplusplus
extern "C" {
#endif

#define _qtk_error(session, code)                                              \
    do {                                                                       \
         if (session) { qtk_session_feed_errcode(session, QTK_ERROR, code, NULL, 0); } \
    } while (0)
#define _qtk_warning(session, code)                                            \
    do {                                                                       \
        if (session) {                                                         \
            qtk_session_feed_errcode(session, QTK_WARN, code, NULL, 0);        \
        }                                                                      \
    } while (0)
#define _qtk_debug(session, code)                                              \
    do {                                                                       \
        if (session) {                                                         \
            qtk_session_feed_errcode(session, QTK_DEBUG, code, NULL, 0);       \
        }                                                                      \
    } while (0)

struct qtk_session
{
	qtk_option_t opt;
	wtk_log_t *log;
	qtk_errcode_t *ec;
	void *clog;
	qtk_timer_t *timer;
	wtk_thread_t thread;
	wtk_sem_t srvsel_sem;
	wtk_sem_t auth_sem;
	void *cldhub;
	unsigned err:1;
	unsigned run:1;
	unsigned srvsel_sem_flg:1;
	unsigned auth_sem_flg:1;
};

qtk_session_t* qtk_session_new(char *params,qtk_errcode_level_t level,void *ths,qtk_errcode_handler errhandler);
void qtk_session_delete(qtk_session_t *session);

void qtk_session_start(qtk_session_t *session);
void qtk_session_stop(qtk_session_t *session);

void qtk_session_set_usrid(qtk_session_t *session,char *usrid,int len);

int qtk_session_check(qtk_session_t *session);
void qtk_session_feed_errcode(qtk_session_t *session,qtk_errcode_level_t level,int errcode, char *extraErrdata, int len);
int qtk_session_read_errcode(qtk_session_t *session,qtk_errcode_level_t *level,int *errcode);

#ifdef __cplusplus
};
#endif
#endif
