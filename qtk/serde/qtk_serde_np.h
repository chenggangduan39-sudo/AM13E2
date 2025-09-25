#ifndef __SERDE_QBL_SERDE_NP_H__
#define __SERDE_QBL_SERDE_NP_H__
#include "qtk/core/qtk_io.h"
#include "wtk/core/wtk_strbuf.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_serde_np_hdr qtk_serde_np_hdr_t;
typedef struct qtk_serde_np qtk_serde_np_t;

typedef enum {
    QBL_SERDE_VAL_UNKNOWN = -1,
    QBL_SERDE_VAL_FLOAT32,
    QBL_SERDE_VAL_INT32,
    QBL_SERDE_VAL_INT8,
    QBL_SERDE_VAL_INT64,
    QBL_SERDE_VAL_INT16,
    QBL_SERDE_VAL_UINT8
} qtk_serde_val_type_t;

struct qtk_serde_np_hdr {
    unsigned char major;
    unsigned char minor;
    unsigned int hdr_len;
    qtk_serde_val_type_t val_t;
    int shape[32];
};

struct qtk_serde_np {
    wtk_strbuf_t *tmp_buf;
    qtk_serde_np_hdr_t hdr;
    int nmemb;
};

int qtk_serde_np_init(qtk_serde_np_t *np);
void qtk_serde_np_reset(qtk_serde_np_t *np);
void *qtk_serde_np_load(qtk_serde_np_t *np, qtk_io_reader r, void *upval);
int qtk_serde_np_clean(qtk_serde_np_t *np);
int qtk_serde_np_save(void *data, qtk_serde_val_type_t type, qtk_io_writer w,
                      void *upval, int shape, ...);
int qtk_serde_np_save1(void *data, qtk_serde_val_type_t type, qtk_io_writer w,
                       void *upval, int *shape, int rank);

#define qtk_serde_np_tofile(fn, data, type, shape, ...)                        \
    do {                                                                       \
        FILE *f__ = fopen(fn, "wb");                                           \
        qtk_serde_np_save(data, type, cast(qtk_io_writer, qtk_file_write),     \
                          f__, shape, __VA_ARGS__);                            \
        fclose(f__);                                                           \
    } while (0)

#define qtk_serde_np_f32_tofile(fn, data, shape, ...)                          \
    qtk_serde_np_tofile(fn, data, QBL_SERDE_VAL_FLOAT32, shape, __VA_ARGS__)
#define qtk_serde_np_i8_tofile(fn, data, shape, ...)                           \
    qtk_serde_np_tofile(fn, data, QBL_SERDE_VAL_INT8, shape, __VA_ARGS__)
#define qtk_serde_np_i16_tofile(fn, data, shape, ...)                          \
    qtk_serde_np_tofile(fn, data, QBL_SERDE_VAL_INT16, shape, __VA_ARGS__)
#define qtk_serde_np_i32_tofile(fn, data, shape, ...)                          \
    qtk_serde_np_tofile(fn, data, QBL_SERDE_VAL_INT32, shape, __VA_ARGS__)
#define qtk_serde_np_i64_tofile(fn, data, shape, ...)                          \
    qtk_serde_np_tofile(fn, data, QBL_SERDE_VAL_INT64, shape, __VA_ARGS__)

#define qtk_serde_np_tofile1(fn, data, type, shape, rank)                      \
    do {                                                                       \
        FILE *f__ = fopen(fn, "wb");                                           \
        qtk_serde_np_save1(data, type, cast(qtk_io_writer, qtk_file_write),    \
                           f__, shape, rank);                                  \
        fclose(f__);                                                           \
    } while (0)

#define qtk_serde_np_f32_tofile1(fn, data, shape, rank)                        \
    qtk_serde_np_tofile1(fn, data, QBL_SERDE_VAL_FLOAT32, shape, rank)
#define qtk_serde_np_i8_tofile1(fn, data, shape, rank)                         \
    qtk_serde_np_tofile1(fn, data, QBL_SERDE_VAL_INT8, shape, rank)
#define qtk_serde_np_i16_tofile1(fn, data, shape, rank)                        \
    qtk_serde_np_tofile1(fn, data, QBL_SERDE_VAL_INT16, shape, rank)
#define qtk_serde_np_i32_tofile1(fn, data, shape, rank)                        \
    qtk_serde_np_tofile1(fn, data, QBL_SERDE_VAL_INT32, shape, rank)
#define qtk_serde_np_i64_tofile1(fn, data, shape, rank)                        \
    qtk_serde_np_tofile1(fn, data, QBL_SERDE_VAL_INT64, shape, rank)
#define qtk_serde_np_u8_tofile1(fn, data, shape, rank)                         \
    qtk_serde_np_tofile1(fn, data, QBL_SERDE_VAL_UINT8, shape, rank)
#ifdef __cplusplus
};
#endif
#endif
