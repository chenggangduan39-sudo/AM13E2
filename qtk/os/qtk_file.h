#ifndef __OS_qtk_FILE_H__
#define __OS_qtk_FILE_H__
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif

int qtk_file_read(FILE *f, char *d, int l);
int qtk_file_write(FILE *f, char *d, int l);

#ifdef __cplusplus
};
#endif
#endif
