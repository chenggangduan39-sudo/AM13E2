#ifndef WTK_MER_SOURCE_FILE_H
#define WTK_MER_SOURCE_FILE_H
#include <ctype.h>
#include "tts-mer/wtk-extend/wtk_tts_common.h"
#include "wtk_type.h"
#ifdef __cplusplus
extern "C" {
#endif

#define wtk_mer_source_loader_with_as(sl,src,fn,func) {wtk_mer_source_loader_init(sl,src,fn);func;if(sl->hook==NULL){wtk_source_clean_file(src);}};

int wtk_mer_source_read_line(wtk_source_t *s,wtk_strbuf_t *b);
int wtk_mer_source_read_string(wtk_source_t *s,wtk_strbuf_t *b);
int wtk_mer_source_read_float(wtk_source_t *s,float *f,int n,int bin);
int wtk_mer_source_read_double(wtk_source_t *s,double *f,int n,int bin);

int wtk_mer_source_loader_init(wtk_source_loader_t *sl, wtk_source_t *src, char *fn);
int wtk_mer_source_loader_load_vecf(wtk_source_loader_t *sl, wtk_source_t *src, char *fn, wtk_vecf_t *vf);
int wtk_mer_source_loader_load_matf(wtk_source_loader_t *sl, wtk_source_t *src, char *fn, wtk_matf_t *mf);
int wtk_mer_source_loader_load_matf2( wtk_heap_t *heap, wtk_source_loader_t *sl, wtk_source_t *src, char *fn, wtk_matf_t *mf);
int wtk_mer_source_loader_load_float(wtk_source_loader_t *sl, wtk_source_t *src, char *fn, float *f, int len);
int wtk_mer_source_loader_load_vecdf(wtk_source_loader_t *sl, wtk_source_t *src, char *fn, wtk_vecdf_t *vf);
int wtk_mer_source_loader_load_double(wtk_source_loader_t *sl, wtk_source_t *src, char *fn, double *f, int len);
#ifdef __cplusplus
}
#endif
#endif
