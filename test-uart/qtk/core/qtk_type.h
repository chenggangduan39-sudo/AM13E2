
#ifndef QTK_CORE_QTK_TYPE_H_
#define QTK_CORE_QTK_TYPE_H_

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
//#include <sys/timeb.h>
#include <sys/time.h>
#include <semaphore.h>
#include <pthread.h>
#include <signal.h>
#include <math.h>

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#ifndef __cplusplus
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif
#endif

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
};
#endif
#endif
