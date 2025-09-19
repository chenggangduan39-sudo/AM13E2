#ifndef WTK_MER_FLIST_H_
#define WTK_MER_FLIST_H_
#include "tts-mer/wtk-extend/wtk_tts_common.h"
#ifdef __cplusplus
extern "C" {
#endif

wtk_flist_t* wtk_mer_flist_new(char *fn);
void wtk_mer_flist_process3(char *fn,void *ths,wtk_flist_notify_f2 notify,int cnt);
void wtk_mer_flist_process2(char *fn,void *ths,wtk_flist_notify_f2 notify);

#ifdef __cplusplus
}
#endif
#endif
