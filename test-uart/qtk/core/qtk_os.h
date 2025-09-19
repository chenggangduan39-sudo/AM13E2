#ifndef WLIB_CORE_QTK_INIT_H_
#define WLIB_CORE_QTK_INIT_H_
#include <stdio.h>
#include "qtk_type.h"
#include "qtk_strbuf.h"
#ifdef __cplusplus
extern "C" {
#endif

uint64_t file_length(FILE *f);
char* file_read_buf(char* fn, int *n);

#ifdef __cplusplus
};
#endif
#endif
