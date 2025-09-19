#ifndef QTK_CORE_QTK_STRBUF_H_
#define QTK_CORE_QTK_STRBUF_H_
#include "qtk_type.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_strbuf qtk_strbuf_t;
#define qtk_strbuf_reset(b) ((b)->pos=0)

struct qtk_strbuf
{
    char *data;					//raw data;
    int pos;					//valid data size;
    int length;					//memory size of raw data;
    float rate;					//memory increase rate;
};

qtk_strbuf_t* qtk_strbuf_new(int init_len,float rate);
int qtk_strbuf_delete(qtk_strbuf_t* b);
void qtk_strbuf_push(qtk_strbuf_t *s,const char *buf,int bytes);
int qtk_strbuf_pop(qtk_strbuf_t *s,char* data,int bytes);
void qtk_strbuf_expand(qtk_strbuf_t *s,int bytes);

#ifdef __cplusplus
};
#endif
#endif
