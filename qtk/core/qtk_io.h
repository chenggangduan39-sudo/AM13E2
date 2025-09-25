#ifndef __QBL_CORE_QBL_IO_H__
#define __QBL_CORE_QBL_IO_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*qtk_io_writer)(void *upval, char *d, int l);
typedef int (*qtk_io_reader)(void *upval, char *d, int l);
int qtk_io_getc(void *rval, qtk_io_reader r);
int qtk_io_copy(void *rval, qtk_io_reader r, void *wval, qtk_io_writer w);
int qtk_io_readn(void *rval, qtk_io_reader r, char *d, int n);
int qtk_io_writen(void *wval, qtk_io_writer w, char *d, int n);

#define qtk_io_write_strliteral(upval, w, s)                                   \
    qtk_io_writen(upval, w, s, sizeof(s) - 1)

#ifdef __cplusplus
};
#endif
#endif
