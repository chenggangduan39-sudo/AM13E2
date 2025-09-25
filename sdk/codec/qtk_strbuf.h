#ifndef QTK_CORE_QTK_STRBUF_H_
#define QTK_CORE_QTK_STRBUF_H_

#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include "qtk_api.h"
#include <string.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_strbuf qtk_strbuf_t;

#define qtk_strbuf_reset(b) ((b)->pos=0)

#define qtk_strbuf_push_c(buf,b) \
{\
	    if(buf->length<=buf->pos) \
	    {\
	        qtk_strbuf_expand(buf,1); \
	    }\
	    buf->data[buf->pos++]=b; \
}

struct qtk_strbuf
{
    char *data;					//raw data;
    int pos;					//valid data size;
    int length;					//memory size of raw data;
    float rate;					//memory increase rate;
};

/**
 * @brief create string buffer;
 */
DLL_API qtk_strbuf_t* qtk_strbuf_new(int init_len,float rate);
DLL_API void qtk_strbuf_resize(qtk_strbuf_t *buf,int size);

/**
 * @brief delete string buffer;
 */
DLL_API int qtk_strbuf_delete(qtk_strbuf_t* b);

/**
 * @brief push data to the end of buffer;
 */
DLL_API void qtk_strbuf_push(qtk_strbuf_t *s,const char *buf,int bytes);

/**
 * @brief pop data from the front of buffer;
 */
DLL_API int qtk_strbuf_pop(qtk_strbuf_t *s,char* data,int bytes);

DLL_API void qtk_strbuf_expand(qtk_strbuf_t *s,int bytes);

#ifdef __cplusplus
};
#endif
#endif
