#ifndef QTK_SESSION_VERIFY_QTK_VERIFY
#define QTK_SESSION_VERIFY_QTK_VERIFY

#include "wtk/core/wtk_type.h"

#ifdef __cplusplus
extern "C" {
#endif

int qtk_verify_proc(char *usrid,int bytes);
#ifndef _WIN32
int qtk_verify_proc_limitdays(int limitdays);
#endif

#ifdef __cplusplus
};
#endif
#endif
