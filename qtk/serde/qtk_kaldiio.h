#ifndef G_H8ZKC071_ZSS5_D2DL_MG3O_2AAWOKBX0GP1
#define G_H8ZKC071_ZSS5_D2DL_MG3O_2AAWOKBX0GP1
#pragma once
#include "qtk/core/qtk_io.h"
#include "wtk/core/wtk_str.h"
#include "wtk/core/wtk_strbuf.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    QBL_KALDIIO_FM,
    QBL_KALDIIO_FV,
    QBL_KALDIIO_I32V,
    QBL_KALDIIO_SCP_ITEM,
} qtk_kaldiio_elem_fmt_t;

typedef struct {
    qtk_kaldiio_elem_fmt_t fmt;
    wtk_string_t token;
    union {
        struct {
            float *data;
            int len;
        } fv;
        struct {
            int32_t *data;
            int len;
        } i32v;
        struct {
            float *data;
            int row;
            int col;
        } fm;
        wtk_string_t xfilename;
    };
} qtk_kaldiio_elem_t;

typedef struct qtk_kaldiio qtk_kaldiio_t;

struct qtk_kaldiio {
    int (*notifier)(void *upval, qtk_kaldiio_elem_t *elem);
    void *upval;
    wtk_strbuf_t *buf;
};

int qtk_kaldiio_init(qtk_kaldiio_t *ki,
                     int (*notifier)(void *upval, qtk_kaldiio_elem_t *elem),
                     void *upval);
int qtk_kaldiio_load_ark(qtk_kaldiio_t *ki, qtk_io_reader r, void *upval);
int qtk_kaldiio_save_ark(qtk_kaldiio_t *ki, qtk_kaldiio_elem_t *elem,
                         int binary, qtk_io_writer w, void *upval);
int qtk_kaldiio_load_scp(qtk_kaldiio_t *ki, qtk_io_reader r, void *upval);
int qtk_kaldiio_clean(qtk_kaldiio_t *ki);

#ifdef __cplusplus
};
#endif
#endif /* G_H8ZKC071_ZSS5_D2DL_MG3O_2AAWOKBX0GP1 */
