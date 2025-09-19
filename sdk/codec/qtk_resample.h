#ifndef QTK_CORE_QTK_RESAMPLE_H_
#define QTK_CORE_QTK_RESAMPLE_H_

#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include "qtk_api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_resample qtk_resample_t;
typedef void (*qtk_resample_notify_f)(void *ths, char *data, int len);

DLL_API qtk_resample_t* qtk_resample_new(void);
DLL_API void qtk_resample_delete(qtk_resample_t* b);
DLL_API int qtk_resample_start(qtk_resample_t *s,int src_rate, int dst_rate);
DLL_API int qtk_resample_feed(qtk_resample_t *s,const char *data,int len);
DLL_API void qtk_resample_reset(qtk_resample_t *s);
DLL_API void qtk_resample_set_notify(qtk_resample_t *s,void *ths, qtk_resample_notify_f notify);

#ifdef __cplusplus
};
#endif
#endif
