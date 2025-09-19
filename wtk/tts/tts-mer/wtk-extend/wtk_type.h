#ifndef WTK_MER_TYPE_H_
#define WTK_MER_TYPE_H_
#include "tts-mer/wtk-extend/wtk_tts_common.h"
#ifdef __cplusplus
extern "C" {
#endif

#define wtk_exit(i) printf("%s:%d:",__FUNCTION__,__LINE__);printf("\n=== mark exit point ===\n");fflush(stdout);exit(i);
//#define wtk_exit_debug(...) printf("%s:%d:",__FUNCTION__,__LINE__);printf(__VA_ARGS__);fflush(stdout);exit(1);

//#ifdef USE_DEBUG
//#else
//#undef wtk_debug
//#define wtk_debug(...)
//#endif

#ifdef __cplusplus
}
#endif
#endif
