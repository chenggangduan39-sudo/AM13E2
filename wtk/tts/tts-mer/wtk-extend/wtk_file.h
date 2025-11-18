#ifndef WTK_MER_FILE_H
#define WTK_MER_FILE_H
#include "tts-mer/wtk-extend/wtk_tts_common.h"
#ifdef __cplusplus
extern "C" {
#endif

FILE* wtk_mer_getfp(char *name, char *opt);

void wtk_mer_file_write_number(FILE *fp, int isbin, char type, void *p, size_t type_size, int n);
void wtk_mer_file_write_float( char *fn, int isbin, float *p, int len);

#ifdef __cplusplus
}
#endif
#endif
