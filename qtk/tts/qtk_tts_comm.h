#ifndef __QTK_TTS_COMM_H__
#define __QTK_TTS_COMM_H__

#include <limits.h>

#ifdef __cplusplus
extern "C"{
#endif

#define PATH_FORMAT_TO(format,...) ({char *fn = wtk_calloc(1,PATH_MAX);sprintf(fn,format,__VA_ARGS__);fn;}) 

#ifdef __cplusplus
};
#endif

#endif